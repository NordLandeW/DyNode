#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import json
import logging
import subprocess
from pathlib import Path
from typing import Dict, List, Tuple

logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
logger = logging.getLogger("pre_deploy")

REPO_ROOT = Path(__file__).resolve().parents[1]
CHANGELOG_PATH = REPO_ROOT / "CHANGELOG"
LANG_DIR = REPO_ROOT / "datafiles" / "lang"
OUTPUT_CHANGELOG = REPO_ROOT / "changelog.json"
OUTPUT_RELEASELOG = REPO_ROOT / "releaselog.txt"
SOURCE_LANG = "zh-cn"
MODEL_NAME = os.environ.get("GEMINI_MODEL", "gemini-2.5-pro")

def run_git(args: List[str]) -> str:
    res = subprocess.run(args, cwd=str(REPO_ROOT), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
    return res.stdout.strip()

def compute_version() -> Tuple[str, bool]:
    has_tag = False
    tag = ""
    try:
        tags_at_head = run_git(["git", "tag", "--points-at", "HEAD"])
        tags = [t for t in tags_at_head.splitlines() if t.strip()]
        if tags:
            has_tag = True
            tag = tags[0].strip()
            logger.info(f"HEAD is tagged: {tag}")
            return tag, has_tag
        # else compute based on latest tag
        latest_tag = run_git(["git", "describe", "--tags", "--abbrev=0"])
        latest_tag = latest_tag.strip()
        if latest_tag:
            commits_since = run_git(["git", "rev-list", f"{latest_tag}..HEAD", "--count"]).strip()
            version = f"{latest_tag}-{commits_since}"
            logger.info(f"Computed version from latest tag: {version}")
            return version, has_tag
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
        return version, has_tag

def detect_languages() -> List[str]:
    codes: List[str] = []
    if LANG_DIR.exists():
        for item in LANG_DIR.glob("*.json"):
            try:
                data = json.loads(item.read_text(encoding="utf-8"))
                lang_code = data.get("lang")
                if isinstance(lang_code, str) and lang_code:
                    codes.append(lang_code)
                else:
                    logger.warning("Could not find valid 'lang' key in %s, skipping.", item.name)
            except (IOError, json.JSONDecodeError) as e:
                logger.warning("Could not read/parse %s: %s", item.name, e)
    # ensure source lang present
    if SOURCE_LANG not in codes:
        codes.append(SOURCE_LANG)
    # de-duplicate preserving order
    seen = set()
    uniq = []
    for c in codes:
        if c not in seen:
            seen.add(c)
            uniq.append(c)
    logger.info("Languages detected: %s", ", ".join(uniq))
    return uniq

def read_changelog_text() -> str:
    if not CHANGELOG_PATH.exists():
        logger.error("CHANGELOG not found at %s", CHANGELOG_PATH)
        sys.exit(1)
    text = CHANGELOG_PATH.read_text(encoding="utf-8")
    if not text.strip():
        logger.error("CHANGELOG is empty.")
        sys.exit(1)
    return text

def translate_with_gemini(source_text: str, target_langs: List[str]) -> Dict[str, str]:
    # Remove source lang from targets if present
    targets = [lang for lang in target_langs if lang != SOURCE_LANG]
    if not targets:
        return {}
    try:
        from google import genai
        client = genai.Client()  # uses GEMINI_API_KEY
        languages_csv = ", ".join(targets)
        system_instruction = (
            "Translate the provided changelog (original in zh-cn) into the specified languages. "
            "Preserve original Markdown formatting and ensure all string values are properly escaped. "
            "Return a single JSON object mapping each language code to the translated content string. "
            "Keys MUST be exactly the language codes provided. Do not include any extra fields."
        )
        contents = (
            f"languages: [{languages_csv}]\n"
            "changelog (zh-cn):\n"
            "-----BEGIN-CHANGELOG-----\n"
            f"{source_text}\n"
            "-----END-CHANGELOG-----\n"
        )
        response = client.models.generate_content(
            model=MODEL_NAME,
            contents=contents,
            config={
                "response_mime_type": "application/json",
            },
        )
        raw = getattr(response, "text", None)
        if not raw:
            logger.warning("Gemini returned empty response; falling back to zh-cn for all targets.")
            return {lang: source_text for lang in targets}
        data = json.loads(raw)
        # sanitize and ensure all targets exist
        out: Dict[str, str] = {}
        for lang in targets:
            val = data.get(lang)
            if isinstance(val, str) and val.strip():
                out[lang] = val
            else:
                logger.warning("Missing translation for %s; using zh-cn content as fallback.", lang)
                out[lang] = source_text
        return out
    except Exception as e:
        logger.warning("Gemini translation failed: %s; using zh-cn content for all targets.", e)
        return {lang: source_text for lang in targets}

def write_outputs(version: str, has_tag: bool, translations: Dict[str, str], zh_cn_text: str) -> None:
    # Compose changelog.json payload
    payload: Dict[str, str] = {"version": version}
    # ensure zh-cn present
    payload[SOURCE_LANG] = zh_cn_text
    # merge others
    for k, v in translations.items():
        payload[k] = v

    OUTPUT_CHANGELOG.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    logger.info("Wrote %s", OUTPUT_CHANGELOG)

    # Build releaselog.txt: zh-cn then en-us
    zh = payload.get("zh-cn", zh_cn_text)
    en = payload.get("en-us", zh_cn_text)
    release_text = f"{zh}\n\n---------\n{en}\n"
    OUTPUT_RELEASELOG.write_text(release_text, encoding="utf-8")
    logger.info("Wrote %s", OUTPUT_RELEASELOG)

    # Full echo
    print("==== changelog.json ====")
    print(OUTPUT_CHANGELOG.read_text(encoding="utf-8"))
    print("==== releaselog.txt ====")
    print(OUTPUT_RELEASELOG.read_text(encoding="utf-8"))

    # Append to GitHub Summary if available
    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary_path:
        with open(summary_path, "a", encoding="utf-8") as f:
            f.write("## Pre-Deploy Outputs\n\n")
            f.write(f"- version: {version}\n")
            f.write(f"- has_tag: {str(has_tag).lower()}\n\n")
            f.write("### changelog.json\n\n")
            f.write("```\n")
            f.write(OUTPUT_CHANGELOG.read_text(encoding="utf-8"))
            f.write("\n```\n\n")
            f.write("### releaselog.txt\n\n")
            f.write("```\n")
            f.write(OUTPUT_RELEASELOG.read_text(encoding="utf-8"))
            f.write("\n```\n")

    # Write step outputs
    out_path = os.environ.get("GITHUB_OUTPUT")
    if out_path:
        with open(out_path, "a", encoding="utf-8") as f:
            f.write(f"version={version}\n")
            f.write(f"has_tag={'true' if has_tag else 'false'}\n")

def main():
    os.chdir(str(REPO_ROOT))
    version, has_tag = compute_version()
    langs = detect_languages()
    zh_text = read_changelog_text()
    translations = translate_with_gemini(zh_text, langs)
    write_outputs(version, has_tag, translations, zh_text)

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        logger.error("pre_deploy failed: %s", e, exc_info=True)
        sys.exit(1)