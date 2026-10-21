#include "core/theme.h"

#include <regex>

namespace lsm {

std::vector<std::string> local_css_imports(const std::string& css) {
    std::vector<std::string> paths;
    static const std::regex pattern(R"css(@import\s+(?:url\()?['"]([^'":)]+)['"]\)?)css");
    for (auto it = std::sregex_iterator(css.begin(), css.end(), pattern);
         it != std::sregex_iterator(); ++it) {
        paths.push_back((*it)[1].str());
    }
    return paths;
}

bool is_dark_rgb(const double red, const double green, const double blue) {
    const double luminance = (0.2126 * red) + (0.7152 * green) + (0.0722 * blue);
    return luminance < 0.5;
}

} // namespace lsm
