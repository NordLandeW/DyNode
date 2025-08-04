#include <cstring>
#include <string>
using std::string;

void bitwrite_int(char *&buffer, const int &value) {
    memcpy(buffer, &value, sizeof(int));
    buffer += sizeof(int);
}
void bitwrite_double(char *&buffer, const double &value) {
    memcpy(buffer, &value, sizeof(double));
    buffer += sizeof(double);
}
void bitwrite_string(char *&buffer, const string &value) {
    memcpy(buffer, value.data(), value.size() + 1);
    buffer += value.size() + 1;
}
void bitread_int(const char *&buffer, int &value) {
    memcpy(&value, buffer, sizeof(int));
    buffer += sizeof(int);
}
void bitread_double(const char *&buffer, double &value) {
    memcpy(&value, buffer, sizeof(double));
    buffer += sizeof(double);
}
void bitread_string(const char *&buffer, string &value) {
    int size = strlen(buffer);
    value.assign(buffer, size);
    buffer += size + 1;
}
