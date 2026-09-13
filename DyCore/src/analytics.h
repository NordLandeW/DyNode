#pragma once

#include <exception>
#include <functional>
#include <map>
#include <string>

void init_analytics();
// Owner-thread handoff for process exit. The returned job owns SDK closing;
// the Sentry module is kept mapped until process termination.
std::function<int()> take_analytics_shutdown();

void report_exception_error(const std::string exceptionType,
                            const std::exception& ex);

void report_exception_error(const std::string exceptionType,
                            const std::exception& ex,
                            const std::map<std::string, std::string>& extra);
