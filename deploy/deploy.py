#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Deployment script for DyNode

Responsibilities:
- Determine version from git (HEAD tag or nearest tag + commit count; fallback v0.0.0-{total})
- Zip the CI artifact directory (windows-build) into DyNode-win-{version}.zip
- Upload DyNode-win-{version}.zip and changelog.json to S3 under dyn/
- Create or update a GitHub Release for the current tag (only if HEAD has a tag), attach the zip asset,
  and set the release body to the contents of releaselog.txt
"""

import os
import sys
import io
import json
import argparse
import logging
import subprocess
import zipfile
from pathlib import Path
from typing import Optional, Tuple, Dict, Any, List

logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
logger = logging.getLogger("deploy")

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ARTIFACT_DIR = REPO_ROOT / "windows-build"
DEFAULT_CHANGELOG = REPO_ROOT / "changelog.json"
DEFAULT_RELEASELOG = REPO_ROOT / "releaselog.txt"
DEFAULT_PREFIX = "dyn/"

# S3 config via env
ENV_S3_ENDPOINT = "S3_ENDPOINT_URL"
ENV_S3_ACCESS_KEY = "S3_ACCESS_KEY"
ENV_S3_SECRET_KEY = "S3_SECRET_KEY"
ENV_S3_BUCKET = "S3_BUCKET"

# GitHub env
ENV_GITHUB_TOKEN = "GITHUB_TOKEN"
ENV_GITHUB_REPOSITORY = "GITHUB_REPOSITORY"  # owner/repo


def run_git(args: List[str]) -> str:
    res = subprocess.run(
        args, cwd=str(REPO_ROOT), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True
    )
    return res.stdout.strip()


def compute_version() -> Tuple[str, bool, Optional[str]]:
    """
    Returns: (version, has_tag, head_tag_if_any)
    """
    has_tag = False
    head_tag = None
    try:
        tags_at_head = run_git(["git", "tag", "--points-at", "HEAD"])
        tags = [t for t in tags_at_head.splitlines() if t.strip()]
        if tags:
            has_tag = True
            head_tag = tags[0].strip()
            logger.info(f"HEAD is tagged: {head_tag}")
            return head_tag, has_tag, head_tag

        # compute from latest tag
        latest_tag = run_git(["git", "describe", "--tags", "--abbrev=0"]).strip()
        if latest_tag:
            commits_since = run_git(["git", "rev-list", f"{latest_tag}..HEAD", "--count"]).strip()
            version = f"{latest_tag}-{commits_since}"
            logger.info(f"Computed version from latest tag: {version}")
            return version, has_tag, None
        else:
            raise subprocess.CalledProcessError(1, "git describe")
    except Exception:
        # no tags in repo
        total = "0"
        try:
            total = run_git(["git", "rev-list", "--count", "HEAD"]).strip()
        except Exception as e:
            logger.warning("Failed to count total commits: %s", e)
        version = f"v0.0.0-{total}"
        logger.warning("No tags found; falling back to version %s", version)
        return version, has_tag, None


def ensure_prefix(prefix: str) -> str:
    p = prefix.strip()
    if not p:
        p = DEFAULT_PREFIX
    if not p.endswith("/"):
        p += "/"
    return p


def zip_artifact_directory(artifact_dir: Path, out_zip: Path) -> None:
    if not artifact_dir.exists() or not artifact_dir.is_dir():
        logger.error("Artifact directory not found or not a directory: %s", artifact_dir)
        sys.exit(1)

    # Remove existing zip if present
    if out_zip.exists():
        out_zip.unlink()

    logger.info("Zipping directory '%s' into '%s' ...", artifact_dir, out_zip)
    with zipfile.ZipFile(out_zip, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for root, _, files in os.walk(artifact_dir):
            for fname in files:
                fpath = Path(root) / fname
                # zip with arcname relative to artifact_dir (no top-level folder)
                arcname = fpath.relative_to(artifact_dir)
                zf.write(fpath, arcname.as_posix())
    logger.info("Zip created: %s (%.2f MB)", out_zip, out_zip.stat().st_size / (1024 * 1024))


def create_s3_client():
    try:
        import boto3
    except ImportError:
        logger.error("boto3 is not installed. Please ensure the workflow installs boto3 before running this script.")
        sys.exit(1)

    endpoint = os.environ.get(ENV_S3_ENDPOINT, "").strip()
    access_key = os.environ.get(ENV_S3_ACCESS_KEY, "").strip()
    secret_key = os.environ.get(ENV_S3_SECRET_KEY, "").strip()

    if not endpoint or not access_key or not secret_key:
        logger.error(
            "Missing S3 configuration. Ensure environment variables %s, %s, %s are set.",
            ENV_S3_ENDPOINT,
            ENV_S3_ACCESS_KEY,
            ENV_S3_SECRET_KEY,
        )
        sys.exit(1)

    import boto3 as _boto3

    logger.info("Connecting S3-compatible endpoint: %s", endpoint)
    return _boto3.client(
        "s3",
        endpoint_url=endpoint,
        aws_access_key_id=access_key,
        aws_secret_access_key=secret_key,
    )


def s3_upload_file(s3_client, bucket: str, key: str, file_path: Path) -> None:
    if not file_path.exists():
        logger.error("File does not exist for S3 upload: %s", file_path)
        sys.exit(1)
    try:
        logger.info("Uploading to s3://%s/%s", bucket, key)
        s3_client.upload_file(str(file_path), bucket, key)
        logger.info("Uploaded %s", key)
    except Exception as e:
        logger.error("S3 upload failed for %s: %s", key, e)
        sys.exit(1)


def gh_request(method: str, url: str, token: str, **kwargs) -> Any:
    try:
        import requests
    except ImportError:
        logger.error("requests is not installed. Please ensure the workflow installs requests before running this script.")
        sys.exit(1)

    headers = kwargs.pop("headers", {})
    headers.setdefault("Accept", "application/vnd.github+json")
    headers.setdefault("Authorization", f"Bearer {token}")
    headers.setdefault("X-GitHub-Api-Version", "2022-11-28")

    resp = requests.request(method, url, headers=headers, **kwargs)
    if method.upper() != "GET" and resp.status_code in (200, 201, 202):
        return resp
    if resp.ok:
        return resp
    return resp


def gh_get_release_by_tag(owner: str, repo: str, tag: str, token: str) -> Optional[Dict[str, Any]]:
    url = f"https://api.github.com/repos/{owner}/{repo}/releases/tags/{tag}"
    resp = gh_request("GET", url, token)
    if resp.status_code == 404:
        return None
    if not resp.ok:
        logger.error("Failed to get release by tag %s: %s %s", tag, resp.status_code, resp.text)
        sys.exit(1)
    return resp.json()


def gh_create_release(owner: str, repo: str, tag: str, name: str, body: str, token: str) -> Dict[str, Any]:
    url = f"https://api.github.com/repos/{owner}/{repo}/releases"
    payload = {
        "tag_name": tag,
        "name": name,
        "body": body,
        "draft": False,
        "prerelease": False,
    }
    resp = gh_request("POST", url, token, json=payload)
    if not resp.ok:
        logger.error("Failed to create release: %s %s", resp.status_code, resp.text)
        sys.exit(1)
    return resp.json()


def gh_update_release(owner: str, repo: str, release_id: int, name: Optional[str], body: str, token: str) -> Dict[str, Any]:
    url = f"https://api.github.com/repos/{owner}/{repo}/releases/{release_id}"
    payload: Dict[str, Any] = {
        "body": body,
        "draft": False,
        "prerelease": False,
    }
    if name:
        payload["name"] = name
    resp = gh_request("PATCH", url, token, json=payload)
    if not resp.ok:
        logger.error("Failed to update release: %s %s", resp.status_code, resp.text)
        sys.exit(1)
    return resp.json()


def gh_list_assets(owner: str, repo: str, release_id: int, token: str) -> List[Dict[str, Any]]:
    url = f"https://api.github.com/repos/{owner}/{repo}/releases/{release_id}/assets"
    resp = gh_request("GET", url, token)
    if not resp.ok:
        logger.error("Failed to list assets: %s %s", resp.status_code, resp.text)
        sys.exit(1)
    return resp.json()


def gh_delete_asset(owner: str, repo: str, asset_id: int, token: str) -> None:
    url = f"https://api.github.com/repos/{owner}/{repo}/releases/assets/{asset_id}"
    resp = gh_request("DELETE", url, token)
    if resp.status_code not in (204,):
        logger.error("Failed to delete asset %s: %s %s", asset_id, resp.status_code, resp.text)
        sys.exit(1)


def gh_upload_asset(upload_url_template: str, asset_name: str, file_path: Path, token: str) -> Dict[str, Any]:
    try:
        import requests
    except ImportError:
        logger.error("requests is not installed. Please ensure the workflow installs requests before running this script.")
        sys.exit(1)

    base_url = upload_url_template.split("{", 1)[0]
    url = f"{base_url}?name={asset_name}"
    headers = {
        "Authorization": f"Bearer {token}",
        "Content-Type": "application/zip",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
    }
    with open(file_path, "rb") as f:
        resp = requests.post(url, headers=headers, data=f)
    if not resp.ok:
        logger.error("Failed to upload asset: %s %s", resp.status_code, resp.text)
        sys.exit(1)
    return resp.json()


def load_text(path: Path) -> str:
    if not path.exists():
        logger.error("File not found: %s", path)
        sys.exit(1)
    return path.read_text(encoding="utf-8")


def handle_github_release(version: str, head_tag: str, zip_name: str, zip_path: Path, releaselog_text: str) -> None:
    token = os.environ.get(ENV_GITHUB_TOKEN, "").strip()
    repo_slug = os.environ.get(ENV_GITHUB_REPOSITORY, "").strip()
    if not token or not repo_slug:
        logger.error(
            "Missing GitHub context. Ensure %s and %s are available in the environment.",
            ENV_GITHUB_TOKEN,
            ENV_GITHUB_REPOSITORY,
        )
        sys.exit(1)
    try:
        owner, repo = repo_slug.split("/", 1)
    except ValueError:
        logger.error("Invalid GITHUB_REPOSITORY format: %s", repo_slug)
        sys.exit(1)

    # Find or create release
    release = gh_get_release_by_tag(owner, repo, head_tag, token)
    release_name = f"DyNode {version}"
    if release is None:
        logger.info("Release for tag %s not found; creating ...", head_tag)
        release = gh_create_release(owner, repo, head_tag, release_name, releaselog_text, token)
    else:
        logger.info("Release for tag %s exists; updating ...", head_tag)
        release = gh_update_release(owner, repo, int(release["id"]), release_name, releaselog_text, token)

    # Manage asset
    assets = gh_list_assets(owner, repo, int(release["id"]), token)
    for a in assets:
        if a.get("name") == zip_name:
            logger.info("Existing asset with same name found; deleting before upload.")
            gh_delete_asset(owner, repo, int(a["id"]), token)

    logger.info("Uploading release asset: %s", zip_name)
    gh_upload_asset(release["upload_url"], zip_name, zip_path, token)
    logger.info("Release asset uploaded.")


def main():
    parser = argparse.ArgumentParser(description="Deploy DyNode release artifacts")
    parser.add_argument("--artifact-dir", default=str(DEFAULT_ARTIFACT_DIR), help="Directory of the built Windows artifact (unzipped)")
    parser.add_argument("--changelog", default=str(DEFAULT_CHANGELOG), help="Path to changelog.json generated in Pre-Deploy")
    parser.add_argument("--releaselog", default=str(DEFAULT_RELEASELOG), help="Path to releaselog.txt generated in Pre-Deploy")
    parser.add_argument("--version", default="", help="Version override (if empty, compute from git)")
    parser.add_argument("--s3-prefix", default=DEFAULT_PREFIX, help="S3 key prefix, default 'dyn/'")

    # Release control flags (default: skip release; override via CLI or SKIP_GH_RELEASE=0)
    parser.add_argument("--skip-release", dest="skip_release", action="store_true", help="Skip creating/updating GitHub Release")
    parser.add_argument("--no-skip-release", dest="skip_release", action="store_false", help="Do not skip release (override env)")
    parser.set_defaults(skip_release=None)

    args = parser.parse_args()

    artifact_dir = Path(args.artifact_dir).resolve()
    changelog_path = Path(args.changelog).resolve()
    releaselog_path = Path(args.releaselog).resolve()
    s3_prefix = ensure_prefix(args.s3_prefix)

    # Determine skip_release default if not provided via CLI
    if args.skip_release is None:
        skip_release = os.environ.get("SKIP_GH_RELEASE", "1").strip().lower() not in ("0", "false", "no")
    else:
        skip_release = args.skip_release

    # Compute and reconcile version
    computed_version, has_tag, head_tag = compute_version()
    version = args.version.strip() or computed_version
    if args.version and args.version.strip() != computed_version:
        logger.warning("Provided version '%s' does not match computed version '%s'. Using provided value.", args.version, computed_version)

    zip_name = f"DyNode-win-{version}.zip"
    zip_path = REPO_ROOT / zip_name

    # Zip artifact directory
    zip_artifact_directory(artifact_dir, zip_path)

    # Load pre-deploy outputs
    _changelog_text = load_text(changelog_path)
    _releaselog_text = load_text(releaselog_path)

    # S3 upload
    bucket = os.environ.get(ENV_S3_BUCKET, "").strip()
    if not bucket:
        logger.error("S3 bucket not specified. Please set environment variable %s.", ENV_S3_BUCKET)
        sys.exit(1)

    s3_client = create_s3_client()
    # Upload zip
    s3_zip_key = f"{s3_prefix}DyNode-win-{version}.zip"
    s3_upload_file(s3_client, bucket, s3_zip_key, zip_path)

    # Upload changelog.json (stable name per design)
    s3_changelog_key = f"{s3_prefix}changelog.json"
    # Write validated changelog to a temp file to ensure consistent encoding
    tmp_changelog = REPO_ROOT / ".deploy_changelog_tmp.json"
    tmp_changelog.write_text(_changelog_text, encoding="utf-8")
    try:
        s3_upload_file(s3_client, bucket, s3_changelog_key, tmp_changelog)
    finally:
        if tmp_changelog.exists():
            tmp_changelog.unlink()

    # GitHub Release handled by workflow action by default.
    # This script can optionally perform release when not skipped.
    if skip_release:
        logger.info("Skipping GitHub Release per configuration (--skip-release or SKIP_GH_RELEASE).")
    elif has_tag and head_tag:
        handle_github_release(version, head_tag, zip_name, zip_path, _releaselog_text)
    else:
        logger.info("HEAD is not tagged. Skipping GitHub Release (manual test mode).")

    logger.info("Deployment completed successfully.")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        logger.error("Deployment script failed: %s", e, exc_info=True)
        sys.exit(1)