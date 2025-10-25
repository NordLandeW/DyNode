import os
import sys
import zipfile
import subprocess
import tempfile
import argparse
import urllib.parse
import shutil
import re
import requests


def get_version_from_git():
    """
    Retrieves the latest git tag and formats it as X.X.X.X.
    """
    try:
        # Get the latest tag
        git_executable = shutil.which("git")
        if not git_executable:
            raise FileNotFoundError("Could not find 'git' executable in PATH. Ensure git is installed and accessible.")
        command = [git_executable, "describe", "--tags", "--abbrev=0"]
        tag_bytes = subprocess.check_output(command)
        tag = tag_bytes.decode("utf-8").strip()

        # Parse version from tag (e.g., v1.2.3 -> 1.2.3)
        match = re.match(r"v?(\d+)\.(\d+)\.(\d+)", tag)
        if not match:
            print(f"Warning: Could not parse version from tag '{tag}'.")
            return None

        major, minor, patch = match.groups()
        version = f"{major}.{minor}.{patch}.0"
        print(f"Using version {version} from git tag '{tag}'.")
        return version

    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        print(f"Warning: Could not get git tag. Is git installed and is this a repo? {e}")
        return None


def download_rcedit(temp_dir):
    """
    Downloads rcedit to a temporary directory.
    """
    rcedit_url = "https://github.com/electron/rcedit/releases/download/v2.0.0/rcedit-x64.exe"
    rcedit_path = os.path.join(temp_dir, "rcedit.exe")
    print(f"Downloading rcedit from {rcedit_url}...")
    try:
        parsed_url = urllib.parse.urlparse(rcedit_url)
        if parsed_url.scheme not in ("http", "https"):
            raise ValueError(f"Invalid URL scheme: {parsed_url.scheme}. Only 'http' and 'https' are allowed.")
        # Use requests for safer, higher-level HTTP handling
        with requests.get(rcedit_url, stream=True, timeout=30) as response:
            response.raise_for_status()
            with open(rcedit_path, "wb") as f:
                for chunk in response.iter_content(chunk_size=8192):
                    if chunk:  # filter out keep-alive chunks
                        f.write(chunk)
        print("rcedit downloaded successfully.")
        return rcedit_path
    except requests.RequestException as e:
        print(f"Error: Failed to download rcedit. {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Error: Failed to download rcedit. {e}")
        sys.exit(1)


def main():
    """
    Unzips a file, embeds a manifest, sets the version, and then re-zips the contents.
    """
    parser = argparse.ArgumentParser(
        description="Embed manifest and version into an executable inside a zip file."
    )
    parser.add_argument("--mt-path", required=True, help="Full path to mt.exe.")
    parser.add_argument(
        "--zip-path", required=True, help="Path to the target .zip file."
    )
    parser.add_argument(
        "--manifest-path", required=True, help="Path to the manifest.xml file."
    )
    parser.add_argument(
        "--target-exe",
        required=True,
        help="Name of the target .exe file inside the zip.",
    )
    args = parser.parse_args()

    # Verify that input files exist
    for path in [args.mt_path, args.zip_path, args.manifest_path]:
        if not os.path.exists(path):
            print(f"Error: Input file not found at '{path}'")
            sys.exit(1)

    print(f"Processing zip file: {args.zip_path}")

    # Use a temporary directory to ensure atomicity and cleanup
    with tempfile.TemporaryDirectory() as temp_dir:
        print(f"Created temporary directory: {temp_dir}")

        # 1. Unzip the archive
        try:
            with zipfile.ZipFile(args.zip_path, "r") as zip_ref:
                zip_ref.extractall(temp_dir)
        except Exception as e:
            print(f"Error: Failed to extract zip file. {e}")
            sys.exit(1)

        target_exe_path = os.path.join(temp_dir, args.target_exe)
        if not os.path.exists(target_exe_path):
            print(
                f"Error: Target executable '{args.target_exe}' not found in the zip archive."
            )
            sys.exit(1)

        # 2. Run the mt.exe command to embed the manifest
        mt_command = [
            args.mt_path,
            "-manifest",
            args.manifest_path,
            f"-outputresource:{target_exe_path};#1",
        ]

        print(f"Executing command: {' '.join(mt_command)}")
        try:
            subprocess.run(mt_command, check=True, capture_output=True, text=True)
            print("mt.exe command completed successfully.")
        except subprocess.CalledProcessError as e:
            print(f"Error: mt.exe command failed with return code {e.returncode}.")
            print(f"Stderr:\n{e.stderr}")
            print(f"Stdout:\n{e.stdout}")
            sys.exit(1)

        # 3. Download rcedit and set version
        rcedit_path = download_rcedit(temp_dir)
        version = get_version_from_git()

        if version:
            rcedit_command = [
                rcedit_path,
                target_exe_path,
                "--set-file-version",
                version,
                "--set-product-version",
                version,
            ]
            print(f"Executing command: {' '.join(rcedit_command)}")
            try:
                subprocess.run(
                    rcedit_command, check=True, capture_output=True, text=True
                )
                print("rcedit command completed successfully.")
            except subprocess.CalledProcessError as e:
                print(f"Error: rcedit command failed with return code {e.returncode}.")
                print(f"Stderr:\n{e.stderr}")
                print(f"Stdout:\n{e.stdout}")
                sys.exit(1)
        else:
            print("Skipping version setting because git tag could not be determined.")

        # 4. Re-zip the contents, overwriting the original file
        print(f"Re-packing modified files into '{os.path.basename(args.zip_path)}'...")
        with zipfile.ZipFile(args.zip_path, "w", zipfile.ZIP_DEFLATED) as new_zip:
            for root, _, files in os.walk(temp_dir):
                for file in files:
                    file_path = os.path.join(root, file)
                    arcname = os.path.relpath(file_path, temp_dir)
                    new_zip.write(file_path, arcname)

        print("Re-packing successful.")

    print("Script finished successfully.")


if __name__ == "__main__":
    main()
