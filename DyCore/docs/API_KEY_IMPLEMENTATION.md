# API Key 实现

本文档讲述在 DyNode 项目中添加 API Key 的相应规范。

**API Key 永远不应在代码 / 日志中包含 / 输出 / 追踪。**
通过此流程配置的 Key 最终会编译进客户端二进制文件中。请仅在此配置客户端级、权限受限的公开 Key（如统计分析 ID）。

在本项目中添加 API Key 并正确使用与部署需要：

- 确定 Key 的名称与内容。
- 将 Key 以每行 `KEY_NAME=<api_key>` 的格式添加到 DyNode/.env 中以进行本地开发。
- 在 `DyCore/CMakeLists.txt` 以相同格式追加内容。

```cmake
# API secrets from environment variables
message(STATUS "Checking for API secrets in environment variables...")
set(GOOG_API_SECRET "$ENV{GOOG_API_SECRET}")
set(GOOG_MEASUREMENT_ID "$ENV{GOOG_MEASUREMENT_ID}")
# ... 于此追加：set(KEY_NAME "$ENV{KEY_NAME}")
```

- 在 Github 库中的 Settings / Secrets and variables / actions 中追加 Repository secrets。
- 在 `DyNode/.github/workflows/build.yml` 和 `DyNode/.github/workflows/codeql.yml` 以相同格式追加内容。

```yml
- name: Configure DyCore using CMake
    working-directory: ./DyCore
    env:
        GOOG_MEASUREMENT_ID: ${{ secrets.GOOG_MEASUREMENT_ID }}
        GOOG_API_SECRET: ${{ secrets.GOOG_API_SECRET }}
        # ... 于此追加 KEY_NAME: ${{ secrets.KEY_NAME }}
    run: cmake --preset=x64-release .
```

- 在 `DyCore/src/config.h.in` 以相同格式追加内容。

```cpp
#pragma once

#include <string>

const std::string GOOG_API_SECRET = "@GOOG_API_SECRET@";
const std::string GOOG_MEASUREMENT_ID = "@GOOG_MEASUREMENT_ID@";
// ... 于此追加 const std::string KEY_NAME = "@KEY_NAME@";

```

- 如果要在 DyCore 中使用 API Key，只需引用头文件 `config.h`。
- 如果 Gamemaker 侧需要读取该 API_KEY，则在 `DyCore.cpp` 中追加相应的接口，并更新 Gamemaker Extension 中的接口函数列表。

```cpp
DYCORE_API const char* DyCore_get_xxx_key() {
    return KEY_NAME.c_str();
}
```