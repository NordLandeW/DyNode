#pragma once

int window_init();
// Returns 1 if a foreign subclass still owns the top of the callback chain.
int window_shutdown();

constexpr int WINDOW_NOT_IMPLEMENTED = -2;

// Window operations run on the owner thread. Return 0 on success, -1 on
// failure, or WINDOW_NOT_IMPLEMENTED on platforms without an implementation.
int window_set_close_intercept(bool enabled) noexcept;
// Returns 1 and clears a pending request, 0 if none, or a negative error.
int window_take_close_request() noexcept;
int window_set_visible(bool visible) noexcept;

void disable_ime();
void enable_ime();
