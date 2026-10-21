#pragma once

#include <string>
#include <vector>

namespace lsm {

std::vector<std::string> local_css_imports(const std::string& css);
bool is_dark_rgb(double red, double green, double blue);

} // namespace lsm
