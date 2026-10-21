#include "core/settings.h"

#include <glib.h>
#include <glib/gstdio.h>

#include <string>

namespace lsm {

Settings::Settings()
    : Settings(std::filesystem::path(g_get_user_config_dir()) / "lgl-scheduler-manager" /
               "settings.ini") {}

Settings::Settings(std::filesystem::path path) : path_(std::move(path)) { load(); }

bool Settings::system_title_bar() const { return system_title_bar_; }

void Settings::set_system_title_bar(const bool enabled) {
    system_title_bar_ = enabled;
    save();
}

void Settings::load() {
    GError* error = nullptr;
    GKeyFile* key_file = g_key_file_new();
    if (g_key_file_load_from_file(key_file, path_.c_str(), G_KEY_FILE_NONE, &error) != 0) {
        GError* value_error = nullptr;
        const gboolean value =
            g_key_file_get_boolean(key_file, "window", "systemTitleBar", &value_error);
        if (value_error == nullptr) {
            system_title_bar_ = value != 0;
        }
        g_clear_error(&value_error);
    }
    g_clear_error(&error);
    g_key_file_unref(key_file);
}

void Settings::save() const {
    const auto parent = path_.parent_path();
    if (g_mkdir_with_parents(parent.c_str(), 0700) != 0) {
        return;
    }
    static_cast<void>(g_chmod(parent.c_str(), 0700));

    GKeyFile* key_file = g_key_file_new();
    g_key_file_set_boolean(key_file, "window", "systemTitleBar", system_title_bar_);
    gsize length = 0;
    gchar* data = g_key_file_to_data(key_file, &length, nullptr);
    GError* error = nullptr;
    static_cast<void>(g_file_set_contents_full(path_.c_str(), data, static_cast<gssize>(length),
                                               G_FILE_SET_CONTENTS_CONSISTENT, 0600, &error));
    g_clear_error(&error);
    g_free(data);
    g_key_file_unref(key_file);
}

} // namespace lsm
