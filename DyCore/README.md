# DyCore

DyNode 所使用的核心扩展。

## Build

### Windows

* 安装 Visual Studio Community 2022 、LLVM 、CMake（非Cygwin版本）与 Ninja 工具链。
* 使用 x64 Release 配置进行编译。

```bash
# 配置项目 (在 DyCore 目录下运行)
cmake --preset=x64-release

# 编译
cmake --build out/build/x64-release
```

### Linux

* 安装 clang、CMake 和 Ninja。

```bash
# 配置项目 (在 DyCore 目录下运行)
cmake --preset=linux-release

# 编译
cmake --build out/build/linux-release
```

### macOS

* 安装 Xcode Command Line Tools、CMake 和 Ninja。

```bash
# 配置项目 (在 DyCore 目录下运行)
cmake --preset=macos-debug

# 编译
cmake --build out/build/macos-debug
```
