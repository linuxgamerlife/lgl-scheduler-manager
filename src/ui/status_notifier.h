#pragma once

#include <functional>
#include <string>

struct _GDBusConnection;
struct _GDBusNodeInfo;
using GDBusConnection = _GDBusConnection;
using GDBusNodeInfo = _GDBusNodeInfo;

namespace lsm {

class StatusNotifier {
  public:
    using ActivateCallback = std::function<void()>;
    using AvailabilityCallback = std::function<void(bool)>;

    StatusNotifier(ActivateCallback start, ActivateCallback stop, ActivateCallback show,
                   ActivateCallback quit, AvailabilityCallback availability);
    ~StatusNotifier();

    StatusNotifier(const StatusNotifier&) = delete;
    StatusNotifier& operator=(const StatusNotifier&) = delete;

    void set_running(bool running);

  private:
    static void on_watcher_appeared(GDBusConnection* connection, const char*, const char*,
                                    void* data);
    static void on_watcher_vanished(GDBusConnection*, const char*, void* data);
    static void on_method_call(GDBusConnection*, const char*, const char*, const char*, const char*,
                               void*, void* invocation, void* data);
    static void on_menu_method_call(GDBusConnection*, const char*, const char*, const char*,
                                    const char*, void* parameters, void* invocation, void* data);
    static void* on_get_property(GDBusConnection*, const char*, const char*, const char*,
                                 const char*, void*, void* data);
    static void* on_get_menu_property(GDBusConnection*, const char*, const char*, const char*,
                                      const char*, void*, void* data);
    void register_item(GDBusConnection* connection);
    void emit_menu_update();
    void activate_menu_item(int id, const char* event);
    void request_show();
    void flush_pending_show();
    void cancel_pending_show();

    ActivateCallback start_;
    ActivateCallback stop_;
    ActivateCallback show_;
    ActivateCallback quit_;
    AvailabilityCallback availability_;
    std::string icon_name_{"lgl-scheduler-manager-unknown"};
    bool running_{false};
    bool show_pending_{false};
    unsigned int show_timeout_{0};
    unsigned int menu_revision_{1};
    GDBusConnection* connection_{nullptr};
    GDBusNodeInfo* node_info_{nullptr};
    unsigned int object_id_{0};
    unsigned int menu_object_id_{0};
    unsigned int watcher_id_{0};
};

} // namespace lsm
