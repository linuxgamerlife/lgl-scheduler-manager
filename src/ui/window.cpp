#include "ui/window.h"

#include "core/scheduler_data.h"
#include "core/scx_parser.h"
#include "core/validation.h"

#include <glib.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace lsm {
namespace {

Gtk::Box& make_card(const char* title) {
    auto* card = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10);
    card->add_css_class("sm-card");
    auto* heading = Gtk::make_managed<Gtk::Label>(title);
    heading->set_halign(Gtk::Align::START);
    heading->add_css_class("section-label");
    card->append(*heading);
    return *card;
}

void add_info_row(Gtk::Grid& grid, const int row, const char* label, Gtk::Label& value) {
    auto* key = Gtk::make_managed<Gtk::Label>(label);
    key->set_halign(Gtk::Align::START);
    key->add_css_class("muted");
    value.set_halign(Gtk::Align::START);
    value.set_hexpand(true);
    value.add_css_class("mono");
    grid.attach(*key, 0, row);
    grid.attach(value, 1, row);
}

Gtk::Label& table_label(const std::string_view text, const bool mono = false) {
    auto* label = Gtk::make_managed<Gtk::Label>(std::string(text));
    label->set_halign(Gtk::Align::START);
    label->set_valign(Gtk::Align::START);
    label->set_wrap(true);
    label->set_xalign(0.0F);
    label->set_margin_start(6);
    label->set_margin_end(6);
    label->set_margin_top(5);
    label->set_margin_bottom(5);
    if (mono) {
        label->add_css_class("mono");
    }
    return *label;
}

Gtk::Box& detail_block(const char* heading, const std::string_view text) {
    auto* block = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
    block->set_hexpand(true);
    auto* title = Gtk::make_managed<Gtk::Label>(heading);
    title->set_halign(Gtk::Align::START);
    title->add_css_class("reference-label");
    auto* value = Gtk::make_managed<Gtk::Label>(std::string(text));
    value->set_halign(Gtk::Align::START);
    value->set_valign(Gtk::Align::START);
    value->set_xalign(0.0F);
    value->set_wrap(true);
    value->set_wrap_mode(Pango::WrapMode::WORD_CHAR);
    block->append(*title);
    block->append(*value);
    return *block;
}

Gtk::Box& reference_item(const SchedulerInfo& entry) {
    auto* item = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10);
    item->add_css_class("reference-item");

    auto* name = Gtk::make_managed<Gtk::Label>(std::string(entry.name));
    name->set_halign(Gtk::Align::START);
    name->add_css_class("mono");
    name->add_css_class("scheduler-name");
    item->append(*name);

    auto* use_cases = Gtk::make_managed<Gtk::Grid>();
    use_cases->set_column_spacing(12);
    use_cases->set_column_homogeneous(true);
    use_cases->attach(detail_block("BEST FOR", entry.best_for), 0, 0);
    use_cases->attach(detail_block("AVOID WHEN", entry.avoid_when), 1, 0);
    item->append(*use_cases);
    item->append(detail_block("NOTES", entry.notes));
    return *item;
}

std::string copyable_flag(const std::string_view flag) {
    const auto separator = flag.find(" / ");
    if (separator == std::string_view::npos) {
        return std::string(flag);
    }
    const auto first = flag.substr(0, separator);
    const auto second = flag.substr(separator + 3);
    return std::string(first.starts_with("--") ? first : second);
}

std::string flag_syntax(const FlagInfo& entry) {
    std::string syntax = copyable_flag(entry.flag);
    if (entry.values == "(flag)" || entry.values == "-" || entry.values == "(bool)" ||
        syntax.find(' ') != std::string::npos) {
        return syntax;
    }

    std::string value(entry.values);
    if (!value.starts_with('<')) {
        std::string::size_type position = 0;
        while ((position = value.find(" / ", position)) != std::string::npos) {
            value.replace(position, 3, "|");
            ++position;
        }
        value = "<" + value + ">";
    }
    return syntax + " " + value;
}

Gtk::Box& flag_item(const FlagInfo& entry) {
    auto* item = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
    item->add_css_class("reference-item");

    auto* identity = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    auto* scheduler = Gtk::make_managed<Gtk::Label>(std::string(entry.scheduler));
    scheduler->add_css_class("scheduler-name");
    scheduler->add_css_class("mono");
    auto* flag = Gtk::make_managed<Gtk::Label>(std::string(entry.flag));
    flag->set_halign(Gtk::Align::START);
    flag->set_hexpand(true);
    flag->set_wrap(true);
    flag->set_selectable(true);
    flag->add_css_class("flag-name");
    flag->add_css_class("mono");
    auto* values = Gtk::make_managed<Gtk::Label>(std::string(entry.values));
    values->add_css_class("value-chip");
    values->add_css_class("mono");
    identity->append(*scheduler);
    identity->append(*flag);
    identity->append(*values);
    if (!entry.flag.starts_with("(")) {
        auto* copy = Gtk::make_managed<Gtk::Button>("Copy syntax");
        copy->add_css_class("sm-small");
        const auto text = flag_syntax(entry);
        copy->set_tooltip_text("Copy " + text);
        copy->signal_clicked().connect([copy, text] {
            if (const auto display = Gdk::Display::get_default()) {
                display->get_clipboard()->set_text(text);
                copy->set_label("Copied");
            }
        });
        identity->append(*copy);
    }
    item->append(*identity);

    if (!entry.flag.starts_with("(")) {
        auto* syntax = Gtk::make_managed<Gtk::Label>("Syntax: " + flag_syntax(entry));
        syntax->set_halign(Gtk::Align::START);
        syntax->set_selectable(true);
        syntax->add_css_class("flag-syntax");
        syntax->add_css_class("mono");
        item->append(*syntax);
    }

    auto* description = Gtk::make_managed<Gtk::Label>(std::string(entry.description));
    description->set_halign(Gtk::Align::START);
    description->set_xalign(0.0F);
    description->set_wrap(true);
    item->append(*description);
    return *item;
}

std::string timestamp() {
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
    localtime_r(&now, &local);
    std::ostringstream stream;
    stream << std::put_time(&local, "%H:%M:%S");
    return stream.str();
}

bool fedora_based() {
    std::ifstream release("/etc/os-release");
    if (!release) {
        return false;
    }

    std::string line;
    while (std::getline(release, line)) {
        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        const std::string key = line.substr(0, separator);
        if (key != "ID" && key != "ID_LIKE") {
            continue;
        }

        std::string value = line.substr(separator + 1);
        if (value.size() >= 2 && value.front() == value.back() &&
            (value.front() == '"' || value.front() == '\'')) {
            value = value.substr(1, value.size() - 2);
        }
        std::istringstream tokens(value);
        std::string token;
        while (tokens >> token) {
            if (token == "fedora") {
                return true;
            }
        }
    }
    return false;
}

} // namespace

Window::Window(const Glib::RefPtr<Gtk::Application>& application, const bool start_minimized)
    : application_(application), start_minimized_(start_minimized) {
    set_application(application_);
    set_title("LGL SCX Scheduler Manager");
    set_default_size(900, 680);
    set_size_request(640, 520);

    build_header();
    build_pages();
    install_actions();
    set_child(content_);
    signal_hide().connect(sigc::mem_fun(*this, &Window::on_window_hidden));
    apply_title_bar(settings_.system_title_bar(), false);
    theme_watcher_ = std::make_unique<ThemeWatcher>(*this);

    refresh_button_.signal_clicked().connect(sigc::mem_fun(*this, &Window::refresh_status));
    stop_header_button_.signal_clicked().connect(sigc::mem_fun(*this, &Window::stop_scheduler));
    primary_button_.signal_clicked().connect([this] {
        if (running_) {
            switch_scheduler();
        } else {
            start_scheduler();
        }
    });
    autostart_check_.signal_toggled().connect(sigc::mem_fun(*this, &Window::set_service_autostart));
    flags_filter_.signal_changed().connect([this] {
        while (auto* child = flags_list_.get_first_child()) {
            flags_list_.remove(*child);
        }
        const std::string filter = flags_filter_.get_active_text().raw();
        for (const auto& entry : flag_reference()) {
            if (filter != "All" && filter != std::string(entry.scheduler)) {
                continue;
            }
            flags_list_.append(flag_item(entry));
        }
    });
    flags_filter_.set_active(-1);
    flags_filter_.set_active(0);

    if (!tool_exists("scxctl")) {
        notebook_.set_current_page(5);
        for (int page = 0; page < 3; ++page) {
            if (auto* widget = notebook_.get_nth_page(page)) {
                widget->set_sensitive(false);
            }
        }
        primary_button_.set_sensitive(false);
        stop_header_button_.set_sensitive(false);
        show_notice("scx-tools was not found; see the Setup tab.", "neutral", 10);
    } else {
        refresh_scheduler_list();
        refresh_status();
        refresh_timer_ = Glib::signal_timeout().connect_seconds(
            [this] {
                refresh_status();
                return true;
            },
            5);
    }

    if (const char* snapshot_page = g_getenv("LSM_GTK_SNAPSHOT_PAGE"); snapshot_page != nullptr) {
        const std::string page(snapshot_page);
        const std::vector<std::string> names{"Status",    "Control", "Log",
                                             "Reference", "Flags",   "Setup"};
        const auto match = std::find(names.begin(), names.end(), page);
        if (match != names.end()) {
            notebook_.set_current_page(static_cast<int>(std::distance(names.begin(), match)));
        }
    }
}

void Window::build_header() {
    brand_icon_.set_from_resource("/com/linuxgamerlife/lsm/lgl-scheduler-manager.png");
    brand_icon_.set_pixel_size(28);
    brand_icon_.add_css_class("brand-icon");
    wordmark_.add_css_class("wordmark");
    brand_.set_margin_end(12);
    brand_.append(brand_icon_);
    brand_.append(wordmark_);
    header_.pack_start(brand_);

    refresh_button_.set_tooltip_text("Refresh scheduler status and list");
    stop_header_button_.set_tooltip_text("Stop the running scheduler");
    primary_button_.add_css_class("sm-primary");
    primary_button_.add_css_class("suggested-action");
    primary_button_.set_tooltip_text("Start or switch scheduler");
    header_.pack_start(refresh_button_);
    header_.pack_start(stop_header_button_);

    menu_button_.set_icon_name("open-menu-symbolic");
    menu_button_.set_tooltip_text("Main menu (F10)");
    header_.pack_end(menu_button_);
    header_.pack_end(primary_button_);
    header_.set_title_widget(*Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL));
}

void Window::build_pages() {
    overlay_.set_child(notebook_);
    notice_label_.set_wrap(true);
    notice_label_.set_max_width_chars(80);
    notice_label_.add_css_class("sm-notice");
    notice_revealer_.set_transition_type(Gtk::RevealerTransitionType::SLIDE_UP);
    notice_revealer_.set_halign(Gtk::Align::CENTER);
    notice_revealer_.set_valign(Gtk::Align::END);
    notice_revealer_.set_margin_bottom(16);
    notice_revealer_.set_can_target(false);
    notice_revealer_.set_child(notice_label_);
    overlay_.add_overlay(notice_revealer_);
    overlay_.set_hexpand(true);
    overlay_.set_vexpand(true);
    content_.append(overlay_);

    notebook_.append_page(build_status_page(), "Status");
    notebook_.append_page(build_control_page(), "Control");
    notebook_.append_page(build_log_page(), "Log");
    notebook_.append_page(build_reference_page(), "Reference");
    notebook_.append_page(build_flags_page(), "Flags");
    notebook_.append_page(build_setup_page(), "Setup");
}

Gtk::Widget& Window::build_status_page() {
    auto* body = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 12);
    body->add_css_class("body");
    auto& current = make_card("CURRENT SCHEDULER");
    status_chip_.add_css_class("sm-chip");
    status_chip_.add_css_class("negative");
    status_chip_.set_halign(Gtk::Align::START);
    current.append(status_chip_);
    auto* grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_row_spacing(10);
    grid->set_column_spacing(12);
    add_info_row(*grid, 0, "Scheduler", scheduler_value_);
    add_info_row(*grid, 1, "Mode", mode_value_);
    add_info_row(*grid, 2, "Service", service_value_);
    current.append(*grid);
    body->append(current);

    auto& tools = make_card("TOOLS");
    auto* tools_grid = Gtk::make_managed<Gtk::Grid>();
    tools_grid->set_row_spacing(10);
    tools_grid->set_column_spacing(12);
    int row = 0;
    if (fedora_based()) {
        add_info_row(*tools_grid, row++, "COPR (kernel-cachyos-addons)", copr_value_);
    }
    add_info_row(*tools_grid, row, "scx-tools installed", tools_value_);
    bool copr_enabled = false;
    const std::filesystem::path repositories{"/etc/yum.repos.d"};
    if (std::filesystem::is_directory(repositories)) {
        for (const auto& entry : std::filesystem::directory_iterator(repositories)) {
            const auto name = entry.path().filename().string();
            if (name.find("bieszczaders") != std::string::npos &&
                name.find("kernel-cachyos-addons") != std::string::npos) {
                copr_enabled = true;
                break;
            }
        }
    }
    copr_value_.set_text(tool_exists("dnf") ? (copr_enabled ? "Yes" : "No") : "N/A");
    tools_value_.set_text(tool_exists("scxctl") ? "Yes" : "No");
    tools.append(*tools_grid);
    body->append(tools);
    return *body;
}

Gtk::Widget& Window::build_control_page() {
    auto* body = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 12);
    body->add_css_class("body");
    auto& card = make_card("SCHEDULER SELECTION");
    auto* grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_row_spacing(10);
    grid->set_column_spacing(12);
    auto& scheduler_label = table_label("Scheduler");
    auto& mode_label = table_label("Mode / profile");
    auto& flags_label = table_label("Custom flags");
    scheduler_combo_.set_hexpand(true);
    mode_combo_.append("Auto");
    mode_combo_.append("Gaming");
    mode_combo_.append("Lowlatency");
    mode_combo_.append("Powersave");
    mode_combo_.set_active(0);
    flags_entry_.set_placeholder_text("e.g. --slice-us 5000 (optional)");
    flags_entry_.set_max_length(512);
    grid->attach(scheduler_label, 0, 0);
    grid->attach(scheduler_combo_, 1, 0);
    grid->attach(mode_label, 0, 1);
    grid->attach(mode_combo_, 1, 1);
    grid->attach(flags_label, 0, 2);
    grid->attach(flags_entry_, 1, 2);
    card.append(*grid);
    auto* note = Gtk::make_managed<Gtk::Label>(
        "Not every mode is supported by every scheduler. Auto is the safe default.");
    note->set_wrap(true);
    note->set_halign(Gtk::Align::START);
    note->add_css_class("muted");
    card.append(*note);
    card.append(autostart_check_);
    body->append(card);
    return *body;
}

Gtk::Widget& Window::build_log_page() {
    auto* body = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
    body->add_css_class("body");
    auto& card = make_card("COMMAND LOG");
    card.add_css_class("flush");
    auto* scroller = Gtk::make_managed<Gtk::ScrolledWindow>();
    scroller->set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    scroller->set_vexpand(true);
    log_view_.set_editable(false);
    log_view_.set_cursor_visible(false);
    log_view_.set_wrap_mode(Gtk::WrapMode::WORD_CHAR);
    log_view_.add_css_class("mono");
    scroller->set_child(log_view_);
    card.append(*scroller);
    auto* clear = Gtk::make_managed<Gtk::Button>("Clear log");
    clear->add_css_class("sm-small");
    clear->set_halign(Gtk::Align::END);
    clear->signal_clicked().connect([this] { log_view_.get_buffer()->set_text(""); });
    card.append(*clear);
    body->append(card);
    return *body;
}

Gtk::Widget& Window::build_reference_page() {
    auto* body = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 12);
    body->add_css_class("body");
    auto& card = make_card("SCHEDULER REFERENCE");
    card.add_css_class("flush");
    auto* scroller = Gtk::make_managed<Gtk::ScrolledWindow>();
    scroller->set_vexpand(true);
    auto* list = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
    list->set_margin_top(2);
    for (const auto& entry : scheduler_reference()) {
        list->append(reference_item(entry));
    }
    scroller->set_child(*list);
    card.append(*scroller);
    body->append(card);
    return *body;
}

Gtk::Widget& Window::build_flags_page() {
    auto* body = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 12);
    body->add_css_class("body");
    auto& card = make_card("CUSTOM FLAGS REFERENCE");
    card.add_css_class("flush");
    flags_filter_.append("All");
    for (const auto& scheduler : fallback_schedulers()) {
        flags_filter_.append("scx_" + std::string(scheduler));
    }
    flags_filter_.set_active(0);
    card.append(flags_filter_);
    auto* scroller = Gtk::make_managed<Gtk::ScrolledWindow>();
    scroller->set_vexpand(true);
    scroller->set_child(flags_list_);
    card.append(*scroller);
    body->append(card);
    return *body;
}

Gtk::Widget& Window::build_setup_page() {
    auto* body = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 12);
    body->add_css_class("body");
    auto& card = make_card("SCX-TOOLS SETUP");
    auto* text = Gtk::make_managed<Gtk::Label>(
        "This application requires scx-tools (scxctl). Install it with your distribution's "
        "package manager, then restart the application.\n\nFedora:\n"
        "sudo dnf copr enable bieszczaders/kernel-cachyos-addons\n"
        "sudo dnf install scx-tools scx-scheds\n\n"
        "Other distributions: https://github.com/sched-ext/scx");
    text->set_wrap(true);
    text->set_selectable(true);
    text->set_halign(Gtk::Align::START);
    text->add_css_class("mono");
    card.append(*text);
    body->append(card);

    auto& nixos = make_card("NIXOS SETUP");
    auto* nixos_text = Gtk::make_managed<Gtk::Label>(
        "Add this to configuration.nix:\n\n"
        "services.scx-loader = {\n"
        "  enable = true;\n"
        "  schedsPackages = [ pkgs.scx.full ]; # or [ pkgs.scx.rustscheds ]\n"
        "};\n\n"
        "Then run:\n"
        "sudo nixos-rebuild switch\n\n"
        "A kernel 6.12 or newer with sched_ext support is required.");
    nixos_text->set_wrap(true);
    nixos_text->set_selectable(true);
    nixos_text->set_halign(Gtk::Align::START);
    nixos_text->add_css_class("mono");
    nixos.append(*nixos_text);
    body->append(nixos);
    return *body;
}

void Window::install_actions() {
    add_action("refresh", sigc::mem_fun(*this, &Window::refresh_status));
    add_action("start", sigc::mem_fun(*this, &Window::start_scheduler));
    add_action("switch", sigc::mem_fun(*this, &Window::switch_scheduler));
    add_action("stop", sigc::mem_fun(*this, &Window::stop_scheduler));
    add_action("help", sigc::mem_fun(*this, &Window::show_help));
    add_action("about", sigc::mem_fun(*this, &Window::show_about));

    system_title_action_ =
        Gio::SimpleAction::create_bool("system-title-bar", settings_.system_title_bar());
    system_title_action_->signal_activate().connect([this](const Glib::VariantBase&) {
        bool current = false;
        system_title_action_->get_state(current);
        const bool enabled = !current;
        system_title_action_->set_state(Glib::Variant<bool>::create(enabled));
        settings_.set_system_title_bar(enabled);
        apply_title_bar(enabled, true);
    });
    add_action(system_title_action_);

    auto menu = Gio::Menu::create();
    auto operations = Gio::Menu::create();
    operations->append("Refresh", "win.refresh");
    operations->append("Start Scheduler", "win.start");
    operations->append("Switch Scheduler", "win.switch");
    operations->append("Stop Scheduler", "win.stop");
    menu->append_section(operations);
    auto appearance = Gio::Menu::create();
    appearance->append("Use System Title Bar", "win.system-title-bar");
    menu->append_section(appearance);
    auto information = Gio::Menu::create();
    information->append("Help", "win.help");
    information->append("About LGL SCX Scheduler Manager", "win.about");
    menu->append_section(information);
    auto quit = Gio::Menu::create();
    quit->append("Quit", "app.quit");
    menu->append_section(quit);
    menu_model_ = menu;
    ensure_main_menu();

    application_->set_accel_for_action("win.refresh", "F5");
    application_->set_accel_for_action("win.help", "F1");
    application_->set_accel_for_action("win.system-title-bar", "<Ctrl><Shift>M");

    install_menu_key_controller(*this);
}

void Window::install_menu_key_controller(Gtk::Widget& widget) {
    auto controller = Gtk::EventControllerKey::create();
    controller->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    controller->signal_key_pressed().connect(sigc::mem_fun(*this, &Window::on_menu_key_pressed),
                                             false);
    controller->signal_key_released().connect(sigc::mem_fun(*this, &Window::on_menu_key_released));
    widget.add_controller(controller);
}

bool Window::on_menu_key_pressed(const guint keyval, guint, Gdk::ModifierType) {
    if (keyval == GDK_KEY_F10) {
        alt_only_ = false;
        if (!f10_down_) {
            f10_down_ = true;
            toggle_main_menu();
        }
        return true;
    }
    alt_only_ = keyval == GDK_KEY_Alt_L || keyval == GDK_KEY_Alt_R;
    return false;
}

void Window::on_menu_key_released(const guint keyval, guint, Gdk::ModifierType) {
    if (keyval == GDK_KEY_F10) {
        f10_down_ = false;
        return;
    }
    const bool is_alt = keyval == GDK_KEY_Alt_L || keyval == GDK_KEY_Alt_R;
    if (is_alt && alt_only_) {
        toggle_main_menu();
    }
    alt_only_ = false;
}

void Window::ensure_main_menu() {
    if (!menu_model_ || menu_button_.get_menu_model() != nullptr) {
        return;
    }
    menu_button_.set_menu_model(menu_model_);
    if (auto* popover = menu_button_.get_popover()) {
        popover->signal_closed().connect([this] { menu_open_ = false; });
        install_menu_key_controller(*popover);
    }
}

void Window::open_main_menu() {
    reset_main_menu_popover();
    ensure_main_menu();
    menu_open_ = true;
    menu_button_.popup();
}

void Window::close_main_menu() {
    menu_open_ = false;
    menu_button_.popdown();
}

void Window::toggle_main_menu() {
    if (menu_open_) {
        close_main_menu();
    } else {
        open_main_menu();
    }
}

void Window::reset_main_menu_popover() {
    close_main_menu();
    menu_button_.unset_menu_model();
}

void Window::on_window_hidden() {
    f10_down_ = false;
    alt_only_ = false;
    if (!tray_available_) {
        return;
    }
    reset_main_menu_popover();
    Glib::signal_idle().connect_once([this] {
        if (!get_visible()) {
            Gtk::Widget::unrealize();
        }
    });
}

void Window::apply_title_bar(const bool system_title_bar, const bool runtime) {
    if (runtime) {
        application_->hold();
        set_visible(false);
        Gtk::Widget::unrealize();
    }
    if (system_title_bar) {
        unset_titlebar();
        header_.set_show_title_buttons(false);
        content_.prepend(header_);
    } else {
        if (header_.get_parent() == &content_) {
            content_.remove(header_);
        }
        header_.set_show_title_buttons(true);
        set_titlebar(header_);
    }
    if (runtime) {
        set_application(application_);
        present();
        scheduler_combo_.grab_focus();
        application_->release();
    }
}

void Window::show_about() {
    if (!about_dialog_) {
        about_dialog_ = std::make_unique<Gtk::AboutDialog>();
        about_dialog_->set_program_name("LGL SCX Scheduler Manager");
        about_dialog_->set_version(LSM_VERSION);
        about_dialog_->set_comments(
            "Manage sched-ext BPF schedulers through scxctl.\n\nBuilt with GTK 4.");
        about_dialog_->set_website("https://github.com/linuxgamerlife/lgl-scheduler-manager");
        about_dialog_->set_website_label("Project page and issue tracker");
        about_dialog_->set_authors({"LinuxGamerLife https://www.youtube.com/@linuxgamerlife"});
        about_dialog_->add_credit_section(
            "Support", {"Support development on Ko-fi https://ko-fi.com/G2G3V70LW"});
        about_dialog_->set_license_type(Gtk::License::MIT_X11);
        about_dialog_->set_logo(Gdk::Texture::create_from_resource(
            "/com/linuxgamerlife/lsm/lgl-scheduler-manager.png"));
        about_dialog_->set_transient_for(*this);
        about_dialog_->set_modal(true);
        about_dialog_->set_hide_on_close(true);
    }
    about_dialog_->present();
}

void Window::show_help() {
    if (!help_window_) {
        auto window = std::make_unique<Gtk::Window>();
        window->set_title("Help");
        window->set_default_size(680, 560);
        window->set_transient_for(*this);
        window->set_hide_on_close(true);
        auto scroller = Gtk::make_managed<Gtk::ScrolledWindow>();
        auto text = Gtk::make_managed<Gtk::TextView>();
        text->set_editable(false);
        text->set_cursor_visible(false);
        text->set_wrap_mode(Gtk::WrapMode::WORD);
        text->add_css_class("dialog-body");
        try {
            const auto bytes = Gio::Resource::lookup_data_global("/com/linuxgamerlife/lsm/HELP.md");
            gsize size = 0;
            const auto* data = static_cast<const char*>(bytes->get_data(size));
            text->get_buffer()->set_text(std::string(data, size));
        } catch (const Glib::Error& error) {
            text->get_buffer()->set_text(std::string("Unable to load help: ") + error.what());
        }
        scroller->set_child(*text);
        window->set_child(*scroller);
        help_window_ = std::move(window);
    }
    help_window_->present();
}

void Window::show_notice(const std::string& text, const std::string& status, const int seconds) {
    notice_label_.set_text(text);
    for (const auto& name : {"positive", "neutral", "negative"}) {
        notice_label_.remove_css_class(name);
    }
    if (!status.empty()) {
        notice_label_.add_css_class(status);
    }
    notice_revealer_.set_reveal_child(true);
    if (notice_timer_.connected()) {
        notice_timer_.disconnect();
    }
    if (seconds > 0) {
        notice_timer_ = Glib::signal_timeout().connect_seconds(
            [this] {
                notice_revealer_.set_reveal_child(false);
                return false;
            },
            seconds);
    }
}

void Window::append_log(const std::string& text, const std::string&) {
    auto buffer = log_view_.get_buffer();
    buffer->insert(buffer->end(), "[" + timestamp() + "] " + text + "\n");
    if (buffer->get_char_count() > 100000) {
        auto end = buffer->get_iter_at_offset(20000);
        buffer->erase(buffer->begin(), end);
    }
}

bool Window::tool_exists(const char* name) const {
    gchar* executable = g_find_program_in_path(name);
    const bool found = executable != nullptr;
    g_free(executable);
    return found;
}

void Window::refresh_status() {
    if (status_request_active_ || !tool_exists("scxctl")) {
        return;
    }
    status_request_active_ = true;
    const std::weak_ptr<int> lifetime = lifetime_;
    runner_.run(
        {"scxctl", "get"},
        [this, lifetime](const CommandResult& result) {
            if (lifetime.expired()) {
                return;
            }
            status_request_active_ = false;
            if (!result.launch_error.empty() || result.timed_out) {
                update_status({});
                return;
            }
            update_status(parse_status(result.standard_output));
        },
        5);
}

void Window::refresh_scheduler_list() {
    const std::weak_ptr<int> lifetime = lifetime_;
    runner_.run(
        {"scxctl", "list"},
        [this, lifetime](const CommandResult& result) {
            if (lifetime.expired()) {
                return;
            }
            scheduler_combo_.remove_all();
            auto schedulers = result.exit_code == 0 ? parse_scheduler_list(result.standard_output)
                                                    : std::vector<std::string>{};
            if (schedulers.empty()) {
                for (const auto scheduler : fallback_schedulers()) {
                    schedulers.emplace_back(scheduler);
                }
                append_log("Could not read the scheduler list; using the built-in list.",
                           "neutral");
            }
            for (const auto& scheduler : schedulers) {
                scheduler_combo_.append("scx_" + scheduler);
            }
            scheduler_combo_.set_active(0);
            show_notice(std::to_string(schedulers.size()) + " schedulers available.", "positive");
        },
        10);
}

std::string Window::selected_scheduler() const {
    std::string selected = scheduler_combo_.get_active_text().raw();
    if (selected.starts_with("scx_")) {
        selected.erase(0, 4);
    }
    return selected;
}

void Window::start_scheduler() {
    auto scheduler = selected_scheduler();
    std::string mode = mode_combo_.get_active_text().raw();
    std::transform(mode.begin(), mode.end(), mode.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    const std::string flags = flags_entry_.get_text().raw();
    if (!valid_scheduler_name(scheduler) || !valid_mode(mode) || !valid_custom_flags(flags)) {
        show_notice("Invalid scheduler, mode, or custom flags.", "negative", 10);
        return;
    }
    std::vector<std::string> arguments{"pkexec", "scxctl", "start", "-s", scheduler};
    if (mode != "auto") {
        arguments.insert(arguments.end(), {"-m", mode});
    }
    if (!flags.empty()) {
        arguments.insert(arguments.end(), {"-a", flags});
    }
    run_mutation(arguments, "Starting scx_" + scheduler);
}

void Window::switch_scheduler() {
    auto scheduler = selected_scheduler();
    std::string mode = mode_combo_.get_active_text().raw();
    std::transform(mode.begin(), mode.end(), mode.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    if (!valid_scheduler_name(scheduler) || !valid_mode(mode)) {
        show_notice("Select a valid scheduler and mode.", "negative", 10);
        return;
    }
    std::vector<std::string> arguments{"pkexec", "scxctl", "switch", "-s", scheduler};
    if (mode != "auto") {
        arguments.insert(arguments.end(), {"-m", mode});
    }
    run_mutation(arguments, "Switching to scx_" + scheduler);
}

void Window::stop_scheduler() { run_mutation({"pkexec", "scxctl", "stop"}, "Stopping scheduler"); }

void Window::set_service_autostart() {
    if (changing_autostart_) {
        return;
    }
    const bool enabled = autostart_check_.get_active();
    std::vector<std::string> command{"pkexec", "systemctl", enabled ? "enable" : "disable", "--now",
                                     "scx_loader.service"};
    append_log(std::string(enabled ? "Enabling" : "Disabling") + " scx_loader.service");
    const std::weak_ptr<int> lifetime = lifetime_;
    runner_.run(
        command,
        [this, enabled, lifetime](const CommandResult& result) {
            if (lifetime.expired()) {
                return;
            }
            if (result.exit_code != 0) {
                changing_autostart_ = true;
                autostart_check_.set_active(!enabled);
                changing_autostart_ = false;
                show_notice(result.exit_code == 126 ? "Authorization cancelled."
                                                    : "Could not change service autostart.",
                            "negative", 10);
                append_log(result.standard_error, "negative");
            } else {
                show_notice(std::string("Service ") + (enabled ? "enabled." : "disabled."),
                            "positive");
            }
            refresh_status();
        },
        60);
}

void Window::run_mutation(const std::vector<std::string>& arguments,
                          const std::string& description) {
    append_log(description);
    show_notice(description + "…", "neutral", 0);
    primary_button_.set_sensitive(false);
    stop_header_button_.set_sensitive(false);
    const std::weak_ptr<int> lifetime = lifetime_;
    runner_.run(
        arguments,
        [this, description, lifetime](const CommandResult& result) {
            if (lifetime.expired()) {
                return;
            }
            primary_button_.set_sensitive(true);
            if (!result.standard_output.empty()) {
                append_log(result.standard_output);
            }
            if (!result.standard_error.empty()) {
                append_log(result.standard_error, result.exit_code == 0 ? "neutral" : "negative");
            }
            if (result.timed_out) {
                show_notice(description + " timed out.", "negative", 10);
            } else if (result.exit_code == 126) {
                show_notice("Authorization cancelled.", "neutral", 10);
            } else if (!result.launch_error.empty()) {
                show_notice(result.launch_error, "negative", 10);
            } else if (result.exit_code != 0) {
                show_notice(description + " failed (exit " + std::to_string(result.exit_code) +
                                ").",
                            "negative", 10);
            } else {
                show_notice(description + " completed.", "positive");
            }
            refresh_status();
        },
        60);
}

void Window::update_status(const SchedulerStatus& status) {
    running_ = status.running;
    current_scheduler_ = status.scheduler;
    current_mode_ = status.mode;
    status_chip_.set_text(running_ ? "RUNNING" : "STOPPED");
    status_chip_.remove_css_class(running_ ? "negative" : "positive");
    status_chip_.add_css_class(running_ ? "positive" : "negative");
    scheduler_value_.set_text(running_ ? "scx_" + current_scheduler_ : "-");
    mode_value_.set_text(running_ ? current_mode_ : "-");
    service_value_.set_text(running_ ? "active" : "inactive");
    stop_header_button_.set_sensitive(running_);
    primary_button_.set_label(running_ ? "Switch" : "Start");
    if (status_callback_) {
        status_callback_(running_);
    }
}

void Window::bring_to_front() {
    reset_main_menu_popover();
    ensure_main_menu();
    present();
}

void Window::set_tray_available(const bool available) {
    tray_available_ = available;
    set_hide_on_close(available);
    if (start_minimized_) {
        start_minimized_ = false;
        if (available) {
            set_visible(false);
        } else {
            present();
            show_notice("No system tray host was found; showing the window instead.", "neutral",
                        10);
        }
    }
}

void Window::set_status_callback(std::function<void(bool)> callback) {
    status_callback_ = std::move(callback);
    status_callback_(running_);
}

void Window::show_main_menu() {
    present();
    open_main_menu();
}

void Window::start_from_tray() { start_scheduler(); }

void Window::stop_from_tray() { stop_scheduler(); }

} // namespace lsm
