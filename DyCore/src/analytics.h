#pragma once

#include <exception>
#include <map>
#include <string>

void init_analytics();
int shutdown_analytics();

void report_exception_error(const std::string exceptionType,
                            const std::exception& ex);

void report_exception_error(const std::string exceptionType,
                            const std::exception& ex,
                            const std::map<std::string, std::string>& extra);
