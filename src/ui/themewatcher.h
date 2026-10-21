#pragma once

#include <gtkmm.h>

#include <vector>

struct _GFileMonitor;
using GFileMonitor = _GFileMonitor;
struct _GSettings;
using GSettings = _GSettings;

namespace lsm {

void install_application_stylesheet();

class ThemeWatcher {
  public:
    explicit ThemeWatcher(Gtk::Window& window);
    ~ThemeWatcher();

    ThemeWatcher(const ThemeWatcher&) = delete;
    ThemeWatcher& operator=(const ThemeWatcher&) = delete;

  private:
    static void on_file_changed(GFileMonitor*, void*, void*, int, void* data);
    static int on_reload(void* data);
    static void on_style_changed(void*, void*, void* data);
    void schedule_reload();
    void apply();
    void rebuild_monitors();

    Gtk::Window& window_;
    Glib::RefPtr<Gtk::CssProvider> user_provider_;
    Glib::RefPtr<Gtk::CssProvider> monospace_provider_;
    std::vector<GFileMonitor*> monitors_;
    GSettings* desktop_settings_{nullptr};
    unsigned int reload_id_{0};
    unsigned long dark_handler_{0};
    unsigned long accent_handler_{0};
    unsigned long settings_handler_{0};
};

} // namespace lsm
