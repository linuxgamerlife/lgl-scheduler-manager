#pragma once

#include <filesystem>

namespace lsm {

class Settings {
  public:
    Settings();
    explicit Settings(std::filesystem::path path);

    [[nodiscard]] bool system_title_bar() const;
    void set_system_title_bar(bool enabled);

  private:
    void load();
    void save() const;

    std::filesystem::path path_;
    bool system_title_bar_{false};
};

} // namespace lsm
