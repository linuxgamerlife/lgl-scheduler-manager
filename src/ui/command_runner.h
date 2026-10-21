#pragma once

#include <functional>
#include <string>
#include <vector>

namespace lsm {

struct CommandResult {
    int exit_code{-1};
    std::string standard_output;
    std::string standard_error;
    std::string launch_error;
    bool timed_out{false};
};

class CommandRunner {
  public:
    using Callback = std::function<void(CommandResult)>;

    void run(const std::vector<std::string>& arguments, Callback callback,
             unsigned int timeout_seconds = 15) const;
};

} // namespace lsm
