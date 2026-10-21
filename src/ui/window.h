#pragma once

#include "core/scx_parser.h"
#include "core/settings.h"
#include "ui/command_runner.h"
#include "ui/themewatcher.h"

#include <gtkmm.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace lsm {

class Window final : public Gtk::ApplicationWindow {
  public:
    Window(const Glib::RefPtr<Gtk::Application>& application, bool start_minimized);
    ~Window() override = default;

    void bring_to_front();
    void set_tray_available(bool available);
    void set_status_callback(std::function<void(bool)> callback);
    void show_main_menu();
    void start_from_tray();
    void stop_from_tray();

  private:
    void build_header();
    void build_pages();
    Gtk::Widget& build_status_page();
    Gtk::Widget& build_control_page();
    Gtk::Widget& build_log_page();
    Gtk::Widget& build_reference_page();
    Gtk::Widget& build_flags_page();
    Gtk::Widget& build_setup_page();
    void install_actions();
    void ensure_main_menu();
    void open_main_menu();
    void close_main_menu();
    void toggle_main_menu();
    void reset_main_menu_popover();
    void on_window_hidden();
    void install_menu_key_controller(Gtk::Widget& widget);
    bool on_menu_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
    void on_menu_key_released(guint keyval, guint keycode, Gdk::ModifierType state);
    void apply_title_bar(bool system_title_bar, bool runtime);
    void show_about();
    void show_help();
    void show_notice(const std::string& text, const std::string& status = {}, int seconds = 5);
    void append_log(const std::string& text, const std::string& status = {});
    void refresh_status();
    void refresh_scheduler_list();
    void start_scheduler();
    void switch_scheduler();
    void stop_scheduler();
    void set_service_autostart();
    void run_mutation(const std::vector<std::string>& arguments, const std::string& description);
    void update_status(const SchedulerStatus& status);
    std::string selected_scheduler() const;
    bool tool_exists(const char* name) const;

    Settings settings_;
    CommandRunner runner_;
    std::unique_ptr<ThemeWatcher> theme_watcher_;
    Glib::RefPtr<Gtk::Application> application_;
    Glib::RefPtr<Gio::SimpleAction> system_title_action_;
    Glib::RefPtr<Gio::MenuModel> menu_model_;
    std::unique_ptr<Gtk::AboutDialog> about_dialog_;
    std::unique_ptr<Gtk::Window> help_window_;

    Gtk::Box content_{Gtk::Orientation::VERTICAL};
    Gtk::HeaderBar header_;
    Gtk::Box brand_{Gtk::Orientation::HORIZONTAL, 10};
    Gtk::Image brand_icon_;
    Gtk::Label wordmark_{"LGL SCX SCHEDULER MANAGER"};
    Gtk::Button refresh_button_{"Refresh"};
    Gtk::Button stop_header_button_{"Stop"};
    Gtk::Button primary_button_{"Start"};
    Gtk::MenuButton menu_button_;
    Gtk::Overlay overlay_;
    Gtk::Notebook notebook_;
    Gtk::Revealer notice_revealer_;
    Gtk::Label notice_label_;

    Gtk::Label status_chip_{"STOPPED"};
    Gtk::Label scheduler_value_{"-"};
    Gtk::Label mode_value_{"-"};
    Gtk::Label service_value_{"inactive"};
    Gtk::Label copr_value_{"-"};
    Gtk::Label tools_value_{"-"};
    Gtk::ComboBoxText scheduler_combo_;
    Gtk::ComboBoxText mode_combo_;
    Gtk::Entry flags_entry_;
    Gtk::CheckButton autostart_check_{"Enable scx_loader service on boot"};
    Gtk::TextView log_view_;
    Gtk::ComboBoxText flags_filter_;
    Gtk::Box flags_list_{Gtk::Orientation::VERTICAL, 8};

    sigc::connection refresh_timer_;
    sigc::connection notice_timer_;
    bool running_{false};
    bool status_request_active_{false};
    bool tray_available_{false};
    bool menu_open_{false};
    bool alt_only_{false};
    bool f10_down_{false};
    bool start_minimized_{false};
    bool changing_autostart_{false};
    std::function<void(bool)> status_callback_;
    std::string current_scheduler_;
    std::string current_mode_;
    std::shared_ptr<int> lifetime_{std::make_shared<int>(0)};
};

} // namespace lsm
