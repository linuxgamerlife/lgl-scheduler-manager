<div align="center">

# LGL Scheduler Manager

**A graphical way to manage sched-ext BPF schedulers on Fedora.**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Release](https://img.shields.io/badge/release-2.0.0-purple)](CHANGELOG.md)
[![Fedora](https://img.shields.io/badge/Fedora-44-blue?logo=fedora&logoColor=white)](https://fedoraproject.org)
[![RPM](https://img.shields.io/badge/RPM-available-brightgreen?logo=redhat&logoColor=white)](https://github.com/linuxgamerlife/lgl-scheduler-manager/releases)
[![GTK](https://img.shields.io/badge/GTK-4-green?logo=gtk&logoColor=white)](https://www.gtk.org)
[![COPR](https://img.shields.io/badge/COPR-lgl--toolkit-blue?logo=fedora&logoColor=white)](https://copr.fedorainfracloud.org/coprs/linuxgamerlife/lgl-toolkit/)
[![Support on Ko-fi](https://img.shields.io/badge/Support_on-Ko--fi-FF5E5B?logo=ko-fi&logoColor=white)](https://ko-fi.com/linuxgamerlife)

</div>

---

## Overview

LGL Scheduler Manager is a GTK 4 desktop application for managing sched-ext BPF
schedulers through `scxctl`. It provides a clear view of the running scheduler,
safe start, stop, and switch controls, and a complete reference for scheduler
modes and custom flags.

- Start, stop, and switch schedulers without using the terminal
- See the active scheduler, mode, service state, and tool availability
- Browse per-scheduler guidance and copy complete custom-flag syntax
- Keep a bounded command log with asynchronous execution and timeouts
- Control `scx_loader.service` autostart from the Control tab
- Hide the window to the system tray and restore it from the tray menu
- Follow your GTK theme, dark mode, accent colour, and user CSS live

> **Use at your own risk.** Starting, stopping, and switching kernel schedulers
> changes system-level behaviour and requires Polkit authorization.

---

## How It Works

1. Install `scx-tools` and `scx-scheds` for your kernel.
2. Open the **Control** tab and select a scheduler and mode.
3. Optional: open the **Flags** tab, filter by scheduler, and use **Copy syntax**
   to copy an argument such as `--slice-us <microseconds>`.
4. Choose **Start** or **Switch**. Administrative actions run through Polkit.
5. Close the window to hide it in the system tray. Use the tray menu's
   **Show Window** action to bring it back.

The **Status** tab refreshes every five seconds. The **Reference** tab explains
what each scheduler is best for and when to avoid it. The **Log** tab records
commands and their output. If `scxctl` is not installed, the application opens
the **Setup** tab with installation guidance instead of failing.

---

## Features

| Feature | What it does |
|---|---|
| **Live status** | Shows the running scheduler, mode, service state, and available tools |
| **Scheduler control** | Starts, stops, and switches schedulers with optional modes |
| **Custom flags** | Shows complete syntax and copies paste-ready arguments |
| **Scheduler reference** | Explains each scheduler's best use cases, limitations, and notes |
| **Command log** | Records commands and output with bounded history and timeouts |
| **Autostart control** | Enables or disables `scx_loader.service` on boot |
| **System tray** | Provides Status, Start, Stop, Show Window, and Quit actions |
| **Single instance** | Launching the app again restores the existing window |
| **Theme support** | Follows GTK colours, dark mode, accent colour, fonts, and user CSS |
| **Title-bar modes** | Switches between the app header bar and the system title bar |
| **Keyboard access** | F10 and Alt toggle the main menu; F1 opens Help; F5 refreshes |
| **Setup guidance** | Shows Fedora and upstream installation steps when `scxctl` is missing |
| **Built-in help** | Documents controls, shortcuts, tray behavior, and setup |
| **About dialog** | Shows version, project links, licence, and support information |

---

## Install

The current release is **2.0.0**.

LGL Scheduler Manager is part of the
[LGL Toolkit COPR](https://copr.fedorainfracloud.org/coprs/linuxgamerlife/lgl-toolkit/).

### Recommended - Fedora COPR

Enable the LGL Toolkit COPR and install the application:

```bash
sudo dnf copr enable linuxgamerlife/lgl-toolkit
sudo dnf install lgl-scheduler-manager
```

Install the scheduler tools from the CachyOS addons COPR:

```bash
sudo dnf copr enable bieszczaders/kernel-cachyos-addons
sudo dnf install scx-tools scx-scheds
```

Future releases will then be installed through normal Fedora system updates.

### Manual downloads

RPM release artifacts are also available from
[GitHub Releases](https://github.com/linuxgamerlife/lgl-scheduler-manager/releases).

### NixOS

The repository is a flake. Run it without installing:

```bash
nix run github:linuxgamerlife/lgl-scheduler-manager
```

To install it system-wide, add the flake as an input and use its overlay:

```nix
# flake.nix
inputs.lgl-scheduler-manager.url = "github:linuxgamerlife/lgl-scheduler-manager";
```

```nix
# configuration.nix
nixpkgs.overlays = [ inputs.lgl-scheduler-manager.overlays.default ];
environment.systemPackages = [ pkgs.lgl-scheduler-manager ];
```

Without an overlay, use the package directly:
`inputs.lgl-scheduler-manager.packages.${pkgs.system}.default`. `scxctl`,
`pkexec`, and `systemctl` are resolved from PATH at runtime, so the scheduler
tools and Polkit must already be installed on the system.

Install the scheduler tools declaratively. The Fedora `scx-tools` and
`scx-scheds` packages correspond to `scx-loader` (which provides `scxctl` and
`scx_loader.service`) and `scx.rustscheds` or `scx.full` (the scheduler
binaries) on NixOS:

```nix
# configuration.nix
services.scx-loader = {
  enable = true;
  schedsPackages = [ pkgs.scx.full ]; # or [ pkgs.scx.rustscheds ]
  config.default_sched = "scx_bpfland"; # optional
};
```

Then apply the configuration with `sudo nixos-rebuild switch`. This installs
`scxctl` and the schedulers system-wide and enables `scx_loader.service` with
its D-Bus and Polkit integration. A kernel version 6.12 or newer with
`sched_ext` support is required; use
`boot.kernelPackages = pkgs.linuxPackages_latest;` if your current kernel is
older.

The project can also be built directly from source.

---

## What's New in 2.0.0

- The maintained application is now a complete GTK 4 and gtkmm release
- Published through the `linuxgamerlife/lgl-toolkit` COPR
- Redesigned scheduler and custom-flag reference cards
- Complete copyable flag syntax, including parameters and allowed values
- StatusNotifierItem tray menu with Show Window and quick scheduler actions
- F10 and Alt now close the main menu as well as open it
- Asynchronous command execution, bounded logs, settings, help, and title-bar modes
- Unit coverage for scheduler data, parsing, settings, theme helpers, and validation

Earlier release notes are in [CHANGELOG.md](CHANGELOG.md).

---

## Known Issues

### Close button after tray restoration on KWin Wayland

On some KWin Wayland sessions, the title-bar close button can ignore clicks
after restoring the window with the tray menu's **Show Window** action. The
hamburger menu, tabs, and body controls remain usable.

Workaround: double-click the title bar to maximize the window, then double-click
again to restore it. The close button then works for the rest of the application
session. Restarting the application can bring the issue back.

This issue is tracked as a known bug and is under investigation.

---

## Build from Source

LGL Scheduler Manager is a GTK 4 and libadwaita application built by the root
CMake configuration. Its maintained code is in `src/`, resources in `data/`, and
tests in `tests/`. It requires gtkmm 4.20, libadwaita 1.6, and JSON-GLib or newer.

Install the dependencies on Fedora 44:

```bash
sudo dnf install cmake gcc-c++ gtkmm4.0-devel libadwaita-devel json-glib-devel
sudo dnf copr enable bieszczaders/kernel-cachyos-addons
sudo dnf install scx-tools scx-scheds
```

Clone, build, and test:

```bash
git clone https://github.com/linuxgamerlife/lgl-scheduler-manager.git
cd lgl-scheduler-manager
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

Run it directly from the build directory:

```bash
./build/lgl-scheduler-manager
```

Install the uninstalled build with:

```bash
sudo cmake --install build
```

The old Qt implementation is retained under `legacy/qt/` as read-only
historical reference. It is not built, tested, packaged, or a runtime dependency.

---

## Project Layout

- `src/` - maintained GTK application and widget-free core logic
- `data/` - GResource manifest, CSS, and bundled artwork
- `tests/` - GLib unit tests registered with CTest
- `packaging/` - RPM spec, desktop entry, AppStream metadata, and icons
- `legacy/qt/` - historical Qt implementation; not built or packaged

---

## License

MIT - see [LICENSE](LICENSE) for details.

---

<div align="center">
Made for <a href="https://fedoraproject.org">Fedora</a> · by <a href="https://www.youtube.com/@linuxgamerlife">LinuxGamerLife</a>
</div>
