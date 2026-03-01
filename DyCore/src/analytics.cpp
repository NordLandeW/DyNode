#include <sentry.h>

#include "config.h"
#include "version.h"

void init_analytics() {
    std::string version = "DyNode@" + std::string(DYNODE_VERSION);

    sentry_options_t* options = sentry_options_new();
    sentry_options_set_dsn(options, SENTRY_DSN.c_str());
    // TODO: Set to standard path for all platforms.
    sentry_options_set_database_path(options, ".sentry-native");
    sentry_options_set_release(options, version.c_str());
    sentry_options_set_debug(options, DYNODE_BUILD_TYPE != "RELEASE");
    sentry_options_set_handler_path(options, "crashpad_handler.exe");

    sentry_init(options);
}