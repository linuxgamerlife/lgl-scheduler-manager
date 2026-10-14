# Changelog

All notable changes to this project will be documented in this file.

## [1.1.1] - 2026-10-14

### Added

- `--start-minimized` starts the app minimized to the system tray, for use in a desktop-session autostart entry

## [1.1.0] - 2026-09-20

### Changed

- Project renamed from `lgl-scxctl-manager` to `lgl-scheduler-manager`; the binary, desktop file, icon, RPM package name, and AppStream ID all change accordingly; the package is now published in the `lgl-toolkit` COPR and the old `lgl-scxctl-manager` COPR is deprecated, so existing installs must be removed and the new package installed (see the README)
- New application icon, shipped as a full hicolor icon set (16×16 to 1024×1024) (#5)
- The system tray now shows the app icon with a green/red/grey status badge instead of a plain coloured dot (#4)
- The Reference tab now lists schedulers in the same order as the scheduler dropdowns (#3)

### Fixed

- Only one copy of the app can run at a time; launching it again brings the existing window to the front (#2)
- Clicking the tray icon now shows/hides the window; previously only a double-click did, which many desktops never send

## [1.0.1] - 2026-04-25

### Changed

- Closing the main window now minimises the app to the system tray instead of quitting; use **Quit** from the tray context menu to exit fully

## [1.0.0] - 2026-03-23

### Initial release

- **Setup tab** – detects whether `scxctl` is on the system PATH at startup; if not, displays distro-agnostic installation guidance and disables operational tabs until resolved
- **Status tab** – live view of the active scheduler, mode, and service state; auto-refreshes every 5 seconds with change-detection to suppress log noise when idle; tools status indicators show whether the COPR repo (Fedora) and scx-tools are installed
- **Control tab** – start, stop, and switch schedulers with selectable mode (Auto, Gaming, Lowlatency, Powersave) and optional custom flags; note that not all modes are supported by every scheduler
- **Log tab** – timestamped output of all `scxctl` commands
- **Reference tab** – descriptions of every supported scheduler, recommended workloads, and workloads to avoid
- **Flags tab** – filterable per-scheduler custom flags reference
- **About menu** – version info, creator credits, social links, and licence notice
- System tray icon with colour-coded status indicator (green = running, red = stopped) and quick start/stop actions
- Polkit (`pkexec`) used for privilege elevation; `scxctl list` and `scxctl get` run without root
- "Enable on boot" checkbox to enable or disable `scx_loader.service` via systemctl
- Scheduler list populated dynamically from `scxctl list` with a built-in fallback if `scx_loader` is unavailable
- Input validation on scheduler names and custom flags before passing to privileged commands
- Application icon bundled into the binary via Qt resources
- Uses the system Qt theme — respects the user's light or dark mode preference
- Built with C++20 and Qt6
