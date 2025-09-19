@echo off
:: Build DyCore before running the project.

echo --- Building DyCore...

:: Navigate to the DyCore directory.
cd %YYprojectDir%/DyCore

:: Configure and build the project.
cmake --preset=x64-release
cmake --build out/build/x64-release

echo --- DyCore build finished.