#include "core/settings.h"

#include <glib.h>
#include <glib/gstdio.h>

#include <filesystem>

namespace {
void settings_test() {
    GError* error = nullptr;
    gchar* directory = g_dir_make_tmp("lsm-settings-XXXXXX", &error);
    g_assert_no_error(error);
    g_assert_nonnull(directory);
    const auto path = std::filesystem::path(directory) / "settings.ini";
    {
        lsm::Settings settings(path);
        g_assert_false(settings.system_title_bar());
        settings.set_system_title_bar(true);
    }
    g_assert_true(lsm::Settings(path).system_title_bar());
    g_remove(path.c_str());
    g_rmdir(directory);
    g_free(directory);
}
} // namespace
int main(int argc, char** argv) {
    g_test_init(&argc, &argv, nullptr);
    g_test_add_func("/settings/round-trip", settings_test);
    return g_test_run();
}
