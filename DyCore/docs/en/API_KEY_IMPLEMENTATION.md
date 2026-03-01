
# API Key Implementation

This document describes the guidelines for adding an API Key to the DyNode project.

**API keys should never be included, output, or tracked in code or logs.**
Keys configured through this process will ultimately be compiled into the client binary. Please only use this configuration for client-level, restricted-permission public keys (e.g., analytics IDs).

To add an API key to this project and ensure its proper usage and deployment, you need to:

- Determine the name and value of the key.
- Add the key to `DyNode/.env` on a new line in the format `KEY_NAME=<api_key>` for local development.
- Append it to `DyCore/CMakeLists.txt` in the following format:

```cmake
# API secrets from environment variables
message(STATUS "Checking for API secrets in environment variables...")
set(GOOG_API_SECRET "$ENV{GOOG_API_SECRET}")
set(GOOG_MEASUREMENT_ID "$ENV{GOOG_MEASUREMENT_ID}")
# ... Append here: set(KEY_NAME "$ENV{KEY_NAME}")
```

- Add it to "Repository secrets" under Settings > Secrets and variables > Actions in the GitHub repository.
- Append it to `DyNode/.github/workflows/build.yml` and `DyNode/.github/workflows/codeql.yml` in the following format:

```yml
- name: Configure DyCore using CMake
    working-directory: ./DyCore
    env:
        GOOG_MEASUREMENT_ID: ${{ secrets.GOOG_MEASUREMENT_ID }}
        GOOG_API_SECRET: ${{ secrets.GOOG_API_SECRET }}
        # ... Append here: KEY_NAME: ${{ secrets.KEY_NAME }}
    run: cmake --preset=x64-release .
```

- Append it to `DyCore/src/config.h.in` in the following format:

```cpp
#pragma once

#include <string>

const std::string GOOG_API_SECRET = "@GOOG_API_SECRET@";
const std::string GOOG_MEASUREMENT_ID = "@GOOG_MEASUREMENT_ID@";
// ... Append here: const std::string KEY_NAME = "@KEY_NAME@";

```

- If GameMaker needs to read this API_KEY, add the corresponding interface in `DyCore.cpp`, and update the interface function list in the GameMaker Extension.

```cpp
DYCORE_API const char* DyCore_get_xxx_key() {
    return KEY_NAME.c_str();
}
```