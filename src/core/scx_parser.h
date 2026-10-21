#pragma once

#include <string>
#include <vector>

namespace lsm {

struct SchedulerStatus {
    bool running{false};
    std::string scheduler;
    std::string mode;
};

SchedulerStatus parse_status(const std::string& output);
std::vector<std::string> parse_scheduler_list(const std::string& output);

} // namespace lsm
