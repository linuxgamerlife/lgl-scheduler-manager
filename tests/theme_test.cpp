#include "core/theme.h"

#include <glib.h>

namespace {
void imports_test() {
    const auto paths =
        lsm::local_css_imports("@import url('colors.css');\n@import \"parts/widgets.css\";");
    g_assert_cmpuint(paths.size(), ==, 2U);
    g_assert_cmpstr(paths[0].c_str(), ==, "colors.css");
}
void darkness_test() {
    g_assert_true(lsm::is_dark_rgb(0.1, 0.1, 0.1));
    g_assert_false(lsm::is_dark_rgb(0.9, 0.9, 0.9));
}
} // namespace
int main(int argc, char** argv) {
    g_test_init(&argc, &argv, nullptr);
    g_test_add_func("/theme/imports", imports_test);
    g_test_add_func("/theme/darkness", darkness_test);
    return g_test_run();
}
