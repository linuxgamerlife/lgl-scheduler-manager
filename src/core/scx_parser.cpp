#include "core/scx_parser.h"

#include <algorithm>
#include <regex>
#include <unordered_set>

namespace lsm {

SchedulerStatus parse_status(const std::string& output) {
    if (output.size() > std::size_t{64} * 1024U) {
        return {};
    }

    static const std::regex running_pattern(
        R"(running\s+([a-zA-Z0-9_-]+)\s+in\s+([a-zA-Z0-9_-]+)\s+mode)", std::regex::icase);
    std::smatch match;
    if (std::regex_search(output, match, running_pattern)) {
        return {true, match[1].str(), match[2].str()};
    }
    return {};
}

std::vector<std::string> parse_scheduler_list(const std::string& output) {
    std::vector<std::string> result;
    if (output.size() > std::size_t{256} * 1024U) {
        return result;
    }

    static const std::regex item_pattern(R"re("([a-zA-Z0-9_-]{1,64})")re");
    std::unordered_set<std::string> seen;
    for (auto it = std::sregex_iterator(output.begin(), output.end(), item_pattern);
         it != std::sregex_iterator() && result.size() < 128U; ++it) {
        const auto value = (*it)[1].str();
        if (seen.insert(value).second) {
            result.push_back(value);
        }
    }
    return result;
}

} // namespace lsm
