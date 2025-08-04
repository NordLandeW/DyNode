#pragma once

#include <string>

void bitwrite_int(char *&buffer, const int &value);
void bitwrite_double(char *&buffer, const double &value);
void bitwrite_string(char *&buffer, const std::string &value);
void bitread_int(const char *&buffer, int &value);
void bitread_double(const char *&buffer, double &value);
void bitread_string(const char *&buffer, std::string &value);