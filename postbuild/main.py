import os
import sys
import zipfile
import subprocess
import tempfile
import argparse


def main():
    """
    Unzips a file, embeds a manifest into a target executable using mt.exe,
    and then re-zips the contents.
    """
    parser = argparse.ArgumentParser(
        description="Embed manifest into an executable inside a zip file."
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
        command = [
            args.mt_path,
            "-manifest",
            args.manifest_path,
            f"-outputresource:{target_exe_path};#1",
        ]

        print(f"Executing command: {' '.join(command)}")
        try:
            subprocess.run(command, check=True, capture_output=True, text=True)
            print("mt.exe command completed successfully.")
        except subprocess.CalledProcessError as e:
            print(f"Error: mt.exe command failed with return code {e.returncode}.")
            print(f"Stderr:\n{e.stderr}")
            print(f"Stdout:\n{e.stdout}")
            sys.exit(1)

        # 3. Re-zip the contents, overwriting the original file
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
