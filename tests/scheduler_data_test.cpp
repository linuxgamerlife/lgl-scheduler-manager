#include "core/scheduler_data.h"

#include <glib.h>

#include <string>

namespace {

void reference_content_test() {
    const auto& schedulers = lsm::scheduler_reference();
    const auto& flags = lsm::flag_reference();
    g_assert_cmpuint(schedulers.size(), ==, 14U);
    g_assert_cmpuint(flags.size(), ==, 48U);
    g_assert_true(std::string(schedulers.front().notes).find("Cache-topology aware") !=
                  std::string::npos);
    g_assert_true(std::string(flags.front().description).find("Default: 5000") !=
                  std::string::npos);
}

} // namespace

int main(int argc, char** argv) {
    g_test_init(&argc, &argv, nullptr);
    g_test_add_func("/scheduler-data/full-reference", reference_content_test);
    return g_test_run();
}
