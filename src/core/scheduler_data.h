#pragma once

#include <string_view>
#include <vector>

namespace lsm {

struct SchedulerInfo {
    std::string_view name;
    std::string_view best_for;
    std::string_view avoid_when;
    std::string_view notes;
};

struct FlagInfo {
    std::string_view scheduler;
    std::string_view flag;
    std::string_view values;
    std::string_view description;
};

const std::vector<SchedulerInfo>& scheduler_reference();
const std::vector<FlagInfo>& flag_reference();
const std::vector<std::string_view>& fallback_schedulers();

} // namespace lsm
