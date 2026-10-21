#include "ui/status_notifier.h"

#include <gio/gio.h>

#include <array>
#include <cstring>
#include <utility>

namespace lsm {
namespace {

constexpr const char* interface_xml = R"xml(
<node>
  <interface name="org.kde.StatusNotifierItem">
    <method name="ContextMenu"><arg type="i" direction="in"/><arg type="i" direction="in"/></method>
    <method name="Activate"><arg type="i" direction="in"/><arg type="i" direction="in"/></method>
    <method name="SecondaryActivate"><arg type="i" direction="in"/><arg type="i" direction="in"/></method>
    <method name="Scroll"><arg type="i" direction="in"/><arg type="s" direction="in"/></method>
    <signal name="NewIcon"/>
    <property name="Category" type="s" access="read"/>
    <property name="Id" type="s" access="read"/>
    <property name="Title" type="s" access="read"/>
    <property name="Status" type="s" access="read"/>
    <property name="IconName" type="s" access="read"/>
    <property name="OverlayIconName" type="s" access="read"/>
    <property name="AttentionIconName" type="s" access="read"/>
    <property name="ItemIsMenu" type="b" access="read"/>
    <property name="Menu" type="o" access="read"/>
  </interface>
  <interface name="com.canonical.dbusmenu">
    <method name="GetLayout">
      <arg name="parentId" type="i" direction="in"/>
      <arg name="recursionDepth" type="i" direction="in"/>
      <arg name="propertyNames" type="as" direction="in"/>
      <arg name="revision" type="u" direction="out"/>
      <arg name="layout" type="(ia{sv}av)" direction="out"/>
    </method>
    <method name="GetGroupProperties">
      <arg name="ids" type="ai" direction="in"/>
      <arg name="propertyNames" type="as" direction="in"/>
      <arg name="properties" type="a(ia{sv})" direction="out"/>
    </method>
    <method name="GetProperty">
      <arg name="id" type="i" direction="in"/>
      <arg name="name" type="s" direction="in"/>
      <arg name="value" type="v" direction="out"/>
    </method>
    <method name="Event">
      <arg name="id" type="i" direction="in"/>
      <arg name="eventId" type="s" direction="in"/>
      <arg name="data" type="v" direction="in"/>
      <arg name="timestamp" type="u" direction="in"/>
    </method>
    <method name="EventGroup">
      <arg name="events" type="a(isvu)" direction="in"/>
      <arg name="idErrors" type="ai" direction="out"/>
    </method>
    <method name="AboutToShow">
      <arg name="id" type="i" direction="in"/>
      <arg name="needUpdate" type="b" direction="out"/>
    </method>
    <method name="AboutToShowGroup">
      <arg name="ids" type="ai" direction="in"/>
      <arg name="updatesNeeded" type="ai" direction="out"/>
      <arg name="idErrors" type="ai" direction="out"/>
    </method>
    <signal name="LayoutUpdated"><arg name="revision" type="u"/><arg name="parent" type="i"/></signal>
    <signal name="ItemsPropertiesUpdated">
      <arg name="updatedProps" type="a(ia{sv})"/>
      <arg name="removedProps" type="a(ias)"/>
    </signal>
    <property name="Version" type="u" access="read"/>
    <property name="TextDirection" type="s" access="read"/>
    <property name="Status" type="s" access="read"/>
    <property name="IconThemePath" type="as" access="read"/>
  </interface>
</node>)xml";

constexpr std::array<int, 7> menu_ids{1, 2, 3, 4, 5, 6, 7};

GVariant* menu_properties(const int id, const bool running) {
    GVariantBuilder properties;
    g_variant_builder_init(&properties, G_VARIANT_TYPE("a{sv}"));
    const auto add_string = [&properties](const char* name, const char* value) {
        g_variant_builder_add(&properties, "{sv}", name, g_variant_new_string(value));
    };
    const auto add_boolean = [&properties](const char* name, const bool value) {
        g_variant_builder_add(&properties, "{sv}", name, g_variant_new_boolean(value));
    };

    switch (id) {
    case 1:
        add_string("label", running ? "Status: Running" : "Status: Stopped");
        add_boolean("enabled", false);
        break;
    case 2:
    case 5:
        add_string("type", "separator");
        break;
    case 3:
        add_string("label", "Start Scheduler");
        add_boolean("enabled", !running);
        break;
    case 4:
        add_string("label", "Stop Scheduler");
        add_boolean("enabled", running);
        break;
    case 6:
        add_string("label", "Show Window");
        break;
    case 7:
        add_string("label", "Quit");
        break;
    default:
        break;
    }
    return g_variant_builder_end(&properties);
}

GVariant* menu_node(const int id, const bool running) {
    GVariantBuilder children;
    g_variant_builder_init(&children, G_VARIANT_TYPE("av"));
    if (id == 0) {
        for (const int child_id : menu_ids) {
            g_variant_builder_add_value(&children,
                                        g_variant_new_variant(menu_node(child_id, running)));
        }
    }
    return g_variant_new("(i@a{sv}@av)", id, menu_properties(id, running),
                         g_variant_builder_end(&children));
}

void return_empty_int_array(GDBusMethodInvocation* invocation) {
    GVariantBuilder values;
    g_variant_builder_init(&values, G_VARIANT_TYPE("ai"));
    g_dbus_method_invocation_return_value(invocation,
                                          g_variant_new("(@ai)", g_variant_builder_end(&values)));
}

struct PendingAction {
    StatusNotifier::ActivateCallback action;
};

void schedule_menu_action(const StatusNotifier::ActivateCallback& action) {
    auto* pending = new PendingAction{action};
    g_idle_add_full(
        G_PRIORITY_DEFAULT_IDLE,
        [](void* data) {
            static_cast<PendingAction*>(data)->action();
            return G_SOURCE_REMOVE;
        },
        pending, [](void* data) { delete static_cast<PendingAction*>(data); });
}

} // namespace

StatusNotifier::StatusNotifier(ActivateCallback start, ActivateCallback stop, ActivateCallback show,
                               ActivateCallback quit, AvailabilityCallback availability)
    : start_(std::move(start)), stop_(std::move(stop)), show_(std::move(show)),
      quit_(std::move(quit)), availability_(std::move(availability)) {
    GError* error = nullptr;
    node_info_ = g_dbus_node_info_new_for_xml(interface_xml, &error);
    if (node_info_ == nullptr) {
        g_warning("Could not parse tray interfaces: %s",
                  error == nullptr ? "unknown error" : error->message);
        g_clear_error(&error);
        availability_(false);
        return;
    }
    watcher_id_ = g_bus_watch_name(G_BUS_TYPE_SESSION, "org.kde.StatusNotifierWatcher",
                                   G_BUS_NAME_WATCHER_FLAGS_NONE, on_watcher_appeared,
                                   on_watcher_vanished, this, nullptr);
}

StatusNotifier::~StatusNotifier() {
    cancel_pending_show();
    if (watcher_id_ != 0U) {
        g_bus_unwatch_name(watcher_id_);
    }
    if (connection_ != nullptr && object_id_ != 0U) {
        g_dbus_connection_unregister_object(connection_, object_id_);
    }
    if (connection_ != nullptr && menu_object_id_ != 0U) {
        g_dbus_connection_unregister_object(connection_, menu_object_id_);
    }
    g_clear_object(&connection_);
    if (node_info_ != nullptr) {
        g_dbus_node_info_unref(node_info_);
    }
}

void StatusNotifier::set_running(const bool running) {
    running_ = running;
    icon_name_ = running ? "lgl-scheduler-manager-running" : "lgl-scheduler-manager-stopped";
    if (connection_ != nullptr && object_id_ != 0U) {
        g_dbus_connection_emit_signal(connection_, nullptr, "/StatusNotifierItem",
                                      "org.kde.StatusNotifierItem", "NewIcon", nullptr, nullptr);
        emit_menu_update();
    }
}

void StatusNotifier::emit_menu_update() {
    ++menu_revision_;
    g_dbus_connection_emit_signal(connection_, nullptr, "/MenuBar", "com.canonical.dbusmenu",
                                  "LayoutUpdated", g_variant_new("(ui)", menu_revision_, 0),
                                  nullptr);
}

void StatusNotifier::activate_menu_item(const int id, const char* event) {
    if (std::strcmp(event, "closed") == 0) {
        flush_pending_show();
        return;
    }
    if (std::strcmp(event, "clicked") != 0) {
        return;
    }
    if (id == 3 && !running_) {
        schedule_menu_action(start_);
    } else if (id == 4 && running_) {
        schedule_menu_action(stop_);
    } else if (id == 6) {
        request_show();
    } else if (id == 7) {
        schedule_menu_action(quit_);
    }
}

void StatusNotifier::request_show() {
    if (show_pending_) {
        return;
    }
    show_pending_ = true;
    show_timeout_ = g_timeout_add_full(
        G_PRIORITY_DEFAULT, 250,
        [](void* data) {
            static_cast<StatusNotifier*>(data)->flush_pending_show();
            return G_SOURCE_REMOVE;
        },
        this, nullptr);
}

void StatusNotifier::flush_pending_show() {
    if (!show_pending_) {
        return;
    }
    show_pending_ = false;
    if (show_timeout_ != 0U) {
        g_source_remove(show_timeout_);
        show_timeout_ = 0;
    }
    schedule_menu_action(show_);
}

void StatusNotifier::cancel_pending_show() {
    show_pending_ = false;
    if (show_timeout_ != 0U) {
        g_source_remove(show_timeout_);
        show_timeout_ = 0;
    }
}

void StatusNotifier::on_watcher_appeared(GDBusConnection* connection, const char*, const char*,
                                         void* data) {
    auto* notifier = static_cast<StatusNotifier*>(data);
    notifier->register_item(connection);
    notifier->availability_(notifier->object_id_ != 0U && notifier->menu_object_id_ != 0U);
}

void StatusNotifier::on_watcher_vanished(GDBusConnection*, const char*, void* data) {
    auto* notifier = static_cast<StatusNotifier*>(data);
    notifier->cancel_pending_show();
    if (notifier->connection_ != nullptr && notifier->object_id_ != 0U) {
        g_dbus_connection_unregister_object(notifier->connection_, notifier->object_id_);
        notifier->object_id_ = 0;
    }
    if (notifier->connection_ != nullptr && notifier->menu_object_id_ != 0U) {
        g_dbus_connection_unregister_object(notifier->connection_, notifier->menu_object_id_);
        notifier->menu_object_id_ = 0;
    }
    g_clear_object(&notifier->connection_);
    notifier->availability_(false);
}

void StatusNotifier::register_item(GDBusConnection* connection) {
    if (object_id_ != 0U) {
        return;
    }
    connection_ = G_DBUS_CONNECTION(g_object_ref(connection));
    static const GDBusInterfaceVTable item_table{
        reinterpret_cast<GDBusInterfaceMethodCallFunc>(on_method_call),
        reinterpret_cast<GDBusInterfaceGetPropertyFunc>(on_get_property),
        nullptr,
        {nullptr}};
    static const GDBusInterfaceVTable menu_table{
        reinterpret_cast<GDBusInterfaceMethodCallFunc>(on_menu_method_call),
        reinterpret_cast<GDBusInterfaceGetPropertyFunc>(on_get_menu_property),
        nullptr,
        {nullptr}};

    GError* error = nullptr;
    object_id_ = g_dbus_connection_register_object(connection_, "/StatusNotifierItem",
                                                   node_info_->interfaces[0], &item_table, this,
                                                   nullptr, &error);
    if (object_id_ != 0U) {
        menu_object_id_ = g_dbus_connection_register_object(
            connection_, "/MenuBar", node_info_->interfaces[1], &menu_table, this, nullptr, &error);
    }
    if (object_id_ == 0U || menu_object_id_ == 0U) {
        g_warning("Could not export tray item: %s",
                  error == nullptr ? "unknown error" : error->message);
        g_clear_error(&error);
        if (object_id_ != 0U) {
            g_dbus_connection_unregister_object(connection_, object_id_);
            object_id_ = 0;
        }
        g_clear_object(&connection_);
        return;
    }
    g_dbus_connection_call(connection_, "org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher",
                           "org.kde.StatusNotifierWatcher", "RegisterStatusNotifierItem",
                           g_variant_new("(s)", "/StatusNotifierItem"), nullptr,
                           G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr, nullptr);
}

void StatusNotifier::on_method_call(GDBusConnection*, const char*, const char*, const char*,
                                    const char*, void*, void* invocation, void*) {
    // The exported D-Bus menu is the only way to open the window from the tray. In particular,
    // ContextMenu must not present the window before the tray host displays /MenuBar.
    g_dbus_method_invocation_return_value(G_DBUS_METHOD_INVOCATION(invocation), nullptr);
}

void StatusNotifier::on_menu_method_call(GDBusConnection*, const char*, const char*, const char*,
                                         const char* method, void* parameters, void* invocation,
                                         void* data) {
    auto* notifier = static_cast<StatusNotifier*>(data);
    auto* call = G_DBUS_METHOD_INVOCATION(invocation);
    auto* values = static_cast<GVariant*>(parameters);

    if (std::strcmp(method, "GetLayout") == 0) {
        g_dbus_method_invocation_return_value(
            call, g_variant_new("(u@(ia{sv}av))", notifier->menu_revision_,
                                menu_node(0, notifier->running_)));
        return;
    }
    if (std::strcmp(method, "GetGroupProperties") == 0) {
        GVariantBuilder properties;
        g_variant_builder_init(&properties, G_VARIANT_TYPE("a(ia{sv})"));
        for (const int id : menu_ids) {
            g_variant_builder_add(&properties, "(i@a{sv})", id,
                                  menu_properties(id, notifier->running_));
        }
        g_dbus_method_invocation_return_value(
            call, g_variant_new("(@a(ia{sv}))", g_variant_builder_end(&properties)));
        return;
    }
    if (std::strcmp(method, "GetProperty") == 0) {
        int id = 0;
        const char* name = nullptr;
        g_variant_get(values, "(i&s)", &id, &name);
        GVariant* properties = menu_properties(id, notifier->running_);
        GVariant* value = g_variant_lookup_value(properties, name, nullptr);
        g_variant_unref(properties);
        if (value == nullptr) {
            g_dbus_method_invocation_return_value(call,
                                                  g_variant_new("(v)", g_variant_new_string("")));
            return;
        }
        g_dbus_method_invocation_return_value(call, g_variant_new("(v)", value));
        g_variant_unref(value);
        return;
    }
    if (std::strcmp(method, "Event") == 0) {
        int id = 0;
        const char* event = nullptr;
        GVariant* event_data = nullptr;
        unsigned int timestamp = 0;
        g_variant_get(values, "(i&s@vu)", &id, &event, &event_data, &timestamp);
        static_cast<void>(timestamp);
        g_variant_unref(event_data);
        notifier->activate_menu_item(id, event);
        g_dbus_method_invocation_return_value(call, nullptr);
        return;
    }
    if (std::strcmp(method, "EventGroup") == 0) {
        GVariantIter* events = nullptr;
        g_variant_get(values, "(a(isvu))", &events);
        int id = 0;
        const char* event = nullptr;
        GVariant* event_data = nullptr;
        unsigned int timestamp = 0;
        while (g_variant_iter_loop(events, "(i&svu)", &id, &event, &event_data, &timestamp)) {
            static_cast<void>(event_data);
            static_cast<void>(timestamp);
            notifier->activate_menu_item(id, event);
        }
        g_variant_iter_free(events);
        return_empty_int_array(call);
        return;
    }
    if (std::strcmp(method, "AboutToShow") == 0) {
        g_dbus_method_invocation_return_value(call, g_variant_new("(b)", false));
        return;
    }
    if (std::strcmp(method, "AboutToShowGroup") == 0) {
        GVariantBuilder updates;
        GVariantBuilder errors;
        g_variant_builder_init(&updates, G_VARIANT_TYPE("ai"));
        g_variant_builder_init(&errors, G_VARIANT_TYPE("ai"));
        g_dbus_method_invocation_return_value(call, g_variant_new("(@ai@ai)",
                                                                  g_variant_builder_end(&updates),
                                                                  g_variant_builder_end(&errors)));
        return;
    }
    g_dbus_method_invocation_return_error(call, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
                                          "Unsupported tray menu method");
}

void* StatusNotifier::on_get_property(GDBusConnection*, const char*, const char*, const char*,
                                      const char* property, void*, void* data) {
    auto* notifier = static_cast<StatusNotifier*>(data);
    if (std::strcmp(property, "Category") == 0) {
        return g_variant_new_string("SystemServices");
    }
    if (std::strcmp(property, "Id") == 0) {
        return g_variant_new_string("lgl-scheduler-manager");
    }
    if (std::strcmp(property, "Title") == 0) {
        return g_variant_new_string("LGL SCX Scheduler Manager");
    }
    if (std::strcmp(property, "Status") == 0) {
        return g_variant_new_string("Active");
    }
    if (std::strcmp(property, "IconName") == 0) {
        return g_variant_new_string(notifier->icon_name_.c_str());
    }
    if (std::strcmp(property, "OverlayIconName") == 0 ||
        std::strcmp(property, "AttentionIconName") == 0) {
        return g_variant_new_string("");
    }
    if (std::strcmp(property, "ItemIsMenu") == 0) {
        return g_variant_new_boolean(false);
    }
    if (std::strcmp(property, "Menu") == 0) {
        return g_variant_new_object_path("/MenuBar");
    }
    return nullptr;
}

void* StatusNotifier::on_get_menu_property(GDBusConnection*, const char*, const char*, const char*,
                                           const char* property, void*, void*) {
    if (std::strcmp(property, "Version") == 0) {
        return g_variant_new_uint32(3);
    }
    if (std::strcmp(property, "TextDirection") == 0) {
        return g_variant_new_string("ltr");
    }
    if (std::strcmp(property, "Status") == 0) {
        return g_variant_new_string("normal");
    }
    if (std::strcmp(property, "IconThemePath") == 0) {
        return g_variant_new_strv(nullptr, 0);
    }
    return nullptr;
}

} // namespace lsm
