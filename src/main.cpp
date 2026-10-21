#include "ui/status_notifier.h"
#include "ui/themewatcher.h"
#include "ui/window.h"

#include <gtkmm.h>

#include <cctype>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

std::string snapshot_name(const std::string& title) {
    std::string value;
    for (const unsigned char character : title) {
        value.push_back(std::isalnum(character) != 0 ? static_cast<char>(character) : '-');
    }
    return value + ".png";
}

} // namespace

int main(int argc, char* argv[]) {
    bool start_minimized = false;
    bool snapshot_mode = g_getenv("LSM_GTK_SNAPSHOT") != nullptr;
    std::vector<char*> filtered_arguments;
    filtered_arguments.reserve(static_cast<std::size_t>(argc));
    filtered_arguments.push_back(argv[0]);
    for (int index = 1; index < argc; ++index) {
        const std::string argument(argv[index]);
        if (argument == "--start-minimized") {
            start_minimized = true;
        } else if (argument == "--version") {
            std::cout << "lgl-scheduler-manager " << LSM_VERSION << '\n';
            return 0;
        } else {
            filtered_arguments.push_back(argv[index]);
        }
    }
    int filtered_count = static_cast<int>(filtered_arguments.size());

    auto flags = snapshot_mode ? Gio::Application::Flags::NON_UNIQUE
                               : Gio::Application::Flags::DEFAULT_FLAGS;
    auto application = Gtk::Application::create("com.linuxgamerlife.lgl-scheduler-manager", flags);
    std::unique_ptr<lsm::Window> window;
    std::unique_ptr<lsm::StatusNotifier> notifier;
    bool first_activation = true;
    bool tray_holding = false;

    application->signal_startup().connect([application] {
        Glib::set_application_name("LGL SCX Scheduler Manager");
        Gtk::Window::set_default_icon_name("lgl-scheduler-manager");
        lsm::install_application_stylesheet();
        application->add_action("quit", [application] {
            for (auto* active_window : application->get_windows()) {
                active_window->set_visible(false);
            }
            application->quit();
        });
        application->set_accel_for_action("app.quit", "<Ctrl>Q");
    });

    application->signal_activate().connect([&] {
        if (!window) {
            window = std::make_unique<lsm::Window>(application, start_minimized && !snapshot_mode);
            notifier = std::make_unique<lsm::StatusNotifier>(
                [&] { window->start_from_tray(); }, [&] { window->stop_from_tray(); },
                [&] { window->bring_to_front(); }, [&] { application->activate_action("quit"); },
                [&](const bool available) {
                    window->set_tray_available(available);
                    if (available && !tray_holding) {
                        application->hold();
                        tray_holding = true;
                    } else if (!available && tray_holding) {
                        application->release();
                        tray_holding = false;
                    }
                });
            window->set_status_callback([&](const bool running) {
                if (notifier) {
                    notifier->set_running(running);
                }
            });
            if (!start_minimized || snapshot_mode) {
                window->present();
            }
        } else if (!first_activation) {
            window->bring_to_front();
        }
        first_activation = false;

        if (snapshot_mode) {
            const char* folder = g_getenv("LSM_GTK_SNAPSHOT");
            Glib::signal_timeout().connect_seconds(
                [&, folder] {
                    std::filesystem::create_directories(folder);
                    GtkSnapshot* snapshot = gtk_snapshot_new();
                    GdkPaintable* paintable = gtk_widget_paintable_new(GTK_WIDGET(window->gobj()));
                    const int width = gtk_widget_get_width(GTK_WIDGET(window->gobj()));
                    const int height = gtk_widget_get_height(GTK_WIDGET(window->gobj()));
                    gdk_paintable_snapshot(paintable, snapshot, static_cast<double>(width),
                                           static_cast<double>(height));
                    GskRenderNode* node = gtk_snapshot_free_to_node(snapshot);
                    GskRenderer* renderer =
                        gtk_native_get_renderer(gtk_widget_get_native(GTK_WIDGET(window->gobj())));
                    graphene_rect_t viewport;
                    graphene_rect_init(&viewport, 0.0F, 0.0F, static_cast<float>(width),
                                       static_cast<float>(height));
                    GdkTexture* texture = gsk_renderer_render_texture(renderer, node, &viewport);
                    if (texture != nullptr) {
                        const auto path =
                            std::filesystem::path(folder) / snapshot_name(window->get_title());
                        static_cast<void>(gdk_texture_save_to_png(texture, path.c_str()));
                        g_object_unref(texture);
                    }
                    gsk_render_node_unref(node);
                    g_object_unref(paintable);
                    application->quit();
                    return false;
                },
                3);
        }
    });

    const int result = application->run(filtered_count, filtered_arguments.data());
    notifier.reset();
    window.reset();
    return result;
}
