#pragma once

int window_init();
// Returns 1 if a foreign subclass still owns the top of the callback chain.
int window_shutdown();

void disable_ime();
void enable_ime();