@echo off
sentry-cli debug-files upload -o nulrom -p dynode --wait --include-sources "./out/build/x64-release/DyCore.pdb"
