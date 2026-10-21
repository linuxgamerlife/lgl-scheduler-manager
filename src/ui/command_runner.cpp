#include "ui/command_runner.h"

#include <gio/gio.h>

#include <algorithm>
#include <memory>
#include <utility>

namespace lsm {
namespace {

constexpr std::size_t max_output_size = std::size_t{256} * 1024U;

struct Request {
    GSubprocess* process{nullptr};
    CommandRunner::Callback callback;
    guint timeout_id{0};
    bool timed_out{false};
};

gboolean on_timeout(gpointer data) {
    auto* request = static_cast<Request*>(data);
    request->timeout_id = 0;
    request->timed_out = true;
    g_subprocess_force_exit(request->process);
    return G_SOURCE_REMOVE;
}

std::string bounded(const gchar* value) {
    if (value == nullptr) {
        return {};
    }
    std::string text(value);
    if (text.size() > max_output_size) {
        text.resize(max_output_size);
        text += "\n[output truncated]";
    }
    return text;
}

void on_complete(GObject* source, GAsyncResult* async_result, gpointer data) {
    std::unique_ptr<Request> request(static_cast<Request*>(data));
    if (request->timeout_id != 0U) {
        g_source_remove(request->timeout_id);
    }

    gchar* standard_output = nullptr;
    gchar* standard_error = nullptr;
    GError* error = nullptr;
    const gboolean communicated = g_subprocess_communicate_utf8_finish(
        G_SUBPROCESS(source), async_result, &standard_output, &standard_error, &error);

    CommandResult result;
    result.timed_out = request->timed_out;
    result.standard_output = bounded(standard_output);
    result.standard_error = bounded(standard_error);
    if (communicated != 0 && g_subprocess_get_if_exited(request->process) != 0) {
        result.exit_code = g_subprocess_get_exit_status(request->process);
    }
    if (error != nullptr && !request->timed_out) {
        result.launch_error = error->message;
    }

    g_free(standard_output);
    g_free(standard_error);
    g_clear_error(&error);
    g_object_unref(request->process);
    request->callback(std::move(result));
}

} // namespace

void CommandRunner::run(const std::vector<std::string>& arguments, Callback callback,
                        const unsigned int timeout_seconds) const {
    if (arguments.empty()) {
        CommandResult result;
        result.launch_error = "No command was specified";
        callback(std::move(result));
        return;
    }

    std::vector<const gchar*> argv;
    argv.reserve(arguments.size() + 1U);
    for (const auto& argument : arguments) {
        argv.push_back(argument.c_str());
    }
    argv.push_back(nullptr);

    GError* error = nullptr;
    GSubprocess* process =
        g_subprocess_newv(argv.data(),
                          static_cast<GSubprocessFlags>(G_SUBPROCESS_FLAGS_STDOUT_PIPE |
                                                        G_SUBPROCESS_FLAGS_STDERR_PIPE),
                          &error);
    if (process == nullptr) {
        CommandResult result;
        result.launch_error = error == nullptr ? "Unable to launch command" : error->message;
        g_clear_error(&error);
        callback(std::move(result));
        return;
    }

    auto request = std::make_unique<Request>();
    request->process = process;
    request->callback = std::move(callback);
    if (timeout_seconds > 0U) {
        request->timeout_id = g_timeout_add_seconds(timeout_seconds, on_timeout, request.get());
    }
    g_subprocess_communicate_utf8_async(process, nullptr, nullptr, on_complete, request.release());
}

} // namespace lsm
