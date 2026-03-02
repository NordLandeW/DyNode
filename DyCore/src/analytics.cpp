#include "analytics.h"

#include <sentry.h>

#include <exception>

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
    sentry_options_set_environment(
        options, DYNODE_BUILD_TYPE == "RELEASE" ? "production" : "development");

    sentry_init(options);
}

void report_exception_error(const std::string exceptionType,
                            const std::exception& ex) {
    sentry_value_t event = sentry_value_new_event();
    sentry_value_t exc =
        sentry_value_new_exception(exceptionType.c_str(), ex.what());
    sentry_value_set_stacktrace(exc, NULL, 0);
    sentry_event_add_exception(event, exc);
    sentry_capture_event(event);
}
