#pragma once

#include <string>

void init_analytics();

void report_exception_error(const std::string exceptionType,
                            const std::exception& ex);
