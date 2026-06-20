#include "analytics.h"

#include <sentry.h>

#include <exception>
#include <map>
#include <string>

#include "config.h"
#include "version.h"

void init_analytics() {
    std::string version = "DyNode@" + std::string(DYNODE_VERSION);

    if (version.find("dirty") != std::string::npos) {
        return;
    }

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
    report_exception_error(exceptionType, ex, {});
}

void report_exception_error(const std::string exceptionType,
                            const std::exception& ex,
                            const std::map<std::string, std::string>& extra) {
    sentry_value_t event = sentry_value_new_event();
    sentry_value_t exc =
        sentry_value_new_exception(exceptionType.c_str(), ex.what());
    sentry_value_set_stacktrace(exc, NULL, 0);
    sentry_event_add_exception(event, exc);

    if (!extra.empty()) {
        sentry_value_t extraValue = sentry_value_new_object();
        for (const auto& [key, value] : extra) {
            sentry_value_set_by_key(extraValue, key.c_str(),
                                    sentry_value_new_string(value.c_str()));
        }
        sentry_value_set_by_key(event, "extra", extraValue);
    }

    sentry_capture_event(event);
}
