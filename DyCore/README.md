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

### clangd

仓库根目录的 `.clangd` 直接使用 `DyCore/out/build/x64-release` 中由 CMake
生成的编译数据库。运行 `cmake --preset=x64-release` 即可刷新，无需复制
`compile_commands.json` 到源码目录。使用其他预设时，将 `.clangd` 中的
`CompilationDatabase` 改为对应构建目录（路径相对于仓库根目录）。

benchmark 目标始终提供编译参数；`DYCORE_BUILD_BENCHMARKS` 只控制是否加入
默认构建。也可以显式构建：

```bash
cmake --build out/build/x64-release --target DyCore_render_benchmark
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
