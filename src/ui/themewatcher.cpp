#include "ui/themewatcher.h"

#include "core/theme.h"

#include <adwaita.h>
#include <gio/gio.h>

#include <filesystem>
#include <set>
#include <string>

namespace lsm {
namespace {

Glib::RefPtr<Gtk::CssProvider> application_provider;

} // namespace

void install_application_stylesheet() {
    adw_init();
    application_provider = Gtk::CssProvider::create();
    application_provider->load_from_resource("/com/linuxgamerlife/lsm/style.css");
    Gtk::StyleContext::add_provider_for_display(Gdk::Display::get_default(), application_provider,
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

ThemeWatcher::ThemeWatcher(Gtk::Window& window)
    : window_(window), user_provider_(Gtk::CssProvider::create()),
      monospace_provider_(Gtk::CssProvider::create()) {
    Gtk::StyleContext::add_provider_for_display(Gdk::Display::get_default(), user_provider_,
                                                GTK_STYLE_PROVIDER_PRIORITY_USER);

    Gtk::StyleContext::add_provider_for_display(Gdk::Display::get_default(), monospace_provider_,
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);

    AdwStyleManager* manager = adw_style_manager_get_default();
    dark_handler_ = g_signal_connect(manager, "notify::dark", G_CALLBACK(on_style_changed), this);
    accent_handler_ =
        g_signal_connect(manager, "notify::accent-color", G_CALLBACK(on_style_changed), this);
    desktop_settings_ = g_settings_new("org.gnome.desktop.interface");
    settings_handler_ =
        g_signal_connect(desktop_settings_, "changed", G_CALLBACK(on_style_changed), this);
    apply();
}

ThemeWatcher::~ThemeWatcher() {
    if (reload_id_ != 0U) {
        g_source_remove(reload_id_);
    }
    AdwStyleManager* manager = adw_style_manager_get_default();
    if (dark_handler_ != 0U) {
        g_signal_handler_disconnect(manager, dark_handler_);
    }
    if (accent_handler_ != 0U) {
        g_signal_handler_disconnect(manager, accent_handler_);
    }
    for (auto* monitor : monitors_) {
        g_object_unref(monitor);
    }
    if (desktop_settings_ != nullptr) {
        if (settings_handler_ != 0U) {
            g_signal_handler_disconnect(desktop_settings_, settings_handler_);
        }
        g_object_unref(desktop_settings_);
    }
}

void ThemeWatcher::on_file_changed(GFileMonitor*, void*, void*, int, void* data) {
    static_cast<ThemeWatcher*>(data)->schedule_reload();
}

int ThemeWatcher::on_reload(void* data) {
    auto* watcher = static_cast<ThemeWatcher*>(data);
    watcher->reload_id_ = 0;
    watcher->apply();
    return G_SOURCE_REMOVE;
}

void ThemeWatcher::on_style_changed(void*, void*, void* data) {
    static_cast<ThemeWatcher*>(data)->schedule_reload();
}

void ThemeWatcher::schedule_reload() {
    if (reload_id_ != 0U) {
        g_source_remove(reload_id_);
    }
    reload_id_ = g_timeout_add(200, on_reload, this);
}

void ThemeWatcher::apply() {
    rebuild_monitors();
    const auto css_path = std::filesystem::path(g_get_user_config_dir()) / "gtk-4.0" / "gtk.css";
    if (std::filesystem::is_regular_file(css_path)) {
        try {
            user_provider_->load_from_path(css_path.string());
        } catch (const Glib::Error&) {
            // User styles commonly contain optional imports. The application stylesheet is
            // separate.
            user_provider_->load_from_data("");
        }
    } else {
        user_provider_->load_from_data("");
    }

    if (desktop_settings_ != nullptr) {
        gchar* font_name = g_settings_get_string(desktop_settings_, "monospace-font-name");
        PangoFontDescription* description = pango_font_description_from_string(font_name);
        const char* family = pango_font_description_get_family(description);
        if (family != nullptr) {
            std::string escaped(family);
            std::size_t position = 0;
            while ((position = escaped.find_first_of("\\\"", position)) != std::string::npos) {
                escaped.insert(position, "\\");
                position += 2;
            }
            monospace_provider_->load_from_data(
                ".mono, .wordmark, .section-label, .sm-chip { font-family: \"" + escaped + "\"; }");
        }
        pango_font_description_free(description);
        g_free(font_name);
    }

    AdwStyleManager* manager = adw_style_manager_get_default();
    if (adw_style_manager_get_system_supports_color_schemes(manager) == 0) {
        gboolean prefer_dark = false;
        g_object_get(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", &prefer_dark,
                     nullptr);
        adw_style_manager_set_color_scheme(manager, prefer_dark != 0 ? ADW_COLOR_SCHEME_FORCE_DARK
                                                                     : ADW_COLOR_SCHEME_DEFAULT);
    }
    const bool dark = adw_style_manager_get_dark(manager) != 0;
    if (dark) {
        window_.add_css_class("dark");
    } else {
        window_.remove_css_class("dark");
    }
}

void ThemeWatcher::rebuild_monitors() {
    for (auto* monitor : monitors_) {
        g_object_unref(monitor);
    }
    monitors_.clear();

    const auto gtk_directory = std::filesystem::path(g_get_user_config_dir()) / "gtk-4.0";
    std::set<std::filesystem::path> directories{gtk_directory};
    const auto css_path = gtk_directory / "gtk.css";
    gchar* contents = nullptr;
    gsize length = 0;
    if (g_file_get_contents(css_path.c_str(), &contents, &length, nullptr) != 0) {
        for (const auto& imported : local_css_imports(std::string(contents, length))) {
            const auto path = gtk_directory / imported;
            directories.insert(std::filesystem::is_directory(path) ? path : path.parent_path());
        }
    }
    g_free(contents);

    for (const auto& directory : directories) {
        if (!std::filesystem::is_directory(directory)) {
            continue;
        }
        GFile* file = g_file_new_for_path(directory.c_str());
        GFileMonitor* monitor =
            g_file_monitor_directory(file, G_FILE_MONITOR_NONE, nullptr, nullptr);
        g_object_unref(file);
        if (monitor != nullptr) {
            g_signal_connect(monitor, "changed", G_CALLBACK(on_file_changed), this);
            monitors_.push_back(monitor);
        }
    }
}

} // namespace lsm
