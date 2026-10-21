#include "core/validation.h"

#include <algorithm>
#include <cctype>
#include <regex>

namespace lsm {

bool valid_scheduler_name(const std::string& value) {
    static const std::regex pattern("^[a-z0-9_-]{1,64}$");
    return std::regex_match(value, pattern);
}

bool valid_mode(const std::string& value) {
    return value == "auto" || value == "gaming" || value == "lowlatency" || value == "powersave" ||
           value == "performance";
}

bool valid_custom_flags(const std::string& value) {
    if (value.size() > 512U || value.find_first_of("\r\n") != std::string::npos ||
        value.find('\0') != std::string::npos) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](const unsigned char character) {
        return std::isalnum(character) != 0 || character == ' ' || character == '-' ||
               character == '_' || character == '.' || character == '=' || character == '/' ||
               character == ':' || character == ',';
    });
}

} // namespace lsm
