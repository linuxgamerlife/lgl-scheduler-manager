#include "core/scx_parser.h"

#include <glib.h>

namespace {
void status_test() {
    const auto status = lsm::parse_status("running bpfland in gaming mode");
    g_assert_true(status.running);
    g_assert_cmpstr(status.scheduler.c_str(), ==, "bpfland");
    g_assert_cmpstr(status.mode.c_str(), ==, "gaming");
    g_assert_false(lsm::parse_status("no scx scheduler running").running);
}
void list_test() {
    const auto list =
        lsm::parse_scheduler_list("supported schedulers: [\"lavd\", \"rusty\", \"lavd\"]");
    g_assert_cmpuint(list.size(), ==, 2U);
    g_assert_cmpstr(list[0].c_str(), ==, "lavd");
}
} // namespace
int main(int argc, char** argv) {
    g_test_init(&argc, &argv, nullptr);
    g_test_add_func("/scx/status", status_test);
    g_test_add_func("/scx/list", list_test);
    return g_test_run();
}
