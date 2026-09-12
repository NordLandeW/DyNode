#pragma once

#include <string>

// Synchronous exit-only POST. Returns an HTTP status, or a negative Win32
// error. No application-level replay, redirects or response-body wait. Each
// phase has a finite timeout; the caller stops submitting batches at its
// budget.
int post_aptabase_events(const std::string& endpoint, const std::string& appKey,
                         const std::string& payload, int timeoutMs,
                         bool useSystemProxy = true);
