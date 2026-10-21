#pragma once

#include <string>

namespace lsm {

bool valid_scheduler_name(const std::string& value);
bool valid_mode(const std::string& value);
bool valid_custom_flags(const std::string& value);

} // namespace lsm
