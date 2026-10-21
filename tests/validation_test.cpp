#include "core/validation.h"

#include <glib.h>

namespace {
void validation_test() {
    g_assert_true(lsm::valid_scheduler_name("bpfland"));
    g_assert_false(lsm::valid_scheduler_name("../../bin/sh"));
    g_assert_true(lsm::valid_mode("auto"));
    g_assert_false(lsm::valid_mode("turbo"));
    g_assert_true(lsm::valid_custom_flags("--slice-us 5000"));
    g_assert_false(lsm::valid_custom_flags("--flag; reboot"));
    g_assert_false(lsm::valid_custom_flags(std::string("--flag\0value", 12)));
}
} // namespace
int main(int argc, char** argv) {
    g_test_init(&argc, &argv, nullptr);
    g_test_add_func("/validation/inputs", validation_test);
    return g_test_run();
}
