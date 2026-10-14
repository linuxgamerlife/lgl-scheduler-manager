# LGL SCX Scheduler Manager

A Qt6 GUI application for managing sched-ext BPF schedulers via `scxctl`.

**USE AT YOUR OWN RISK**

> **Renamed:** this project was previously called `lgl-scxctl-manager`. From 1.1.0 the package, binary, desktop file and AppStream ID are `lgl-scheduler-manager`. Existing installs do not update automatically — see [Upgrading from lgl-scxctl-manager](#upgrading-from-lgl-scxctl-manager).

![Version](https://img.shields.io/badge/version-1.1.1-blue.svg)
![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)
[![Fedora](https://img.shields.io/badge/Fedora-43%20%7C%2044-blue?logo=fedora&logoColor=white)](https://fedoraproject.org)

##   Features

- **Status tab** – live view of the running scheduler, mode, and service state; auto-refreshes every 5 seconds with change-detection (no log noise when idle)
- **Control tab** – start, stop, or switch schedulers with a chosen mode/profile and optional custom flags
- **Log tab** – timestamped output of all `scxctl` commands
- **Reference tab** – descriptions of every scheduler and what workloads each is best suited to
- **Flags tab** – filterable reference of per-scheduler custom flags with accepted values and descriptions
- **System tray** – app icon with a colour-coded status badge (green = running, red = stopped) and quick start/stop actions
- Polkit (`pkexec`) used for privilege elevation; `scxctl list` and `scxctl get` run without root

## Prerequisites

### scx-tools (provides scxctl and scx_loader)

```bash
sudo dnf copr enable bieszczaders/kernel-cachyos-addons
sudo dnf install scx-tools scx-scheds
```

### Recommended — COPR

Published in the LGL Toolkit COPR, alongside the other LGL tools:

```bash
sudo dnf copr enable linuxgamerlife/lgl-toolkit
sudo dnf install lgl-scheduler-manager
```

The COPR currently builds for Fedora 44 and newer. On Fedora 43, use the RPM from GitHub Releases below.

### RPM from Releases

Download the `.rpm` for your Fedora version from [GitHub Releases](https://github.com/linuxgamerlife/lgl-scheduler-manager/releases) and double-click to install via Discover.

-  `lgl-scheduler-manager-1.1.1-1.fc43.x86_64.rpm` — Fedora 43
-  `lgl-scheduler-manager-1.1.1-1.fc44.x86_64.rpm` — Fedora 44

> After installing from Discover, close it and launch the app from your application menu rather than from the Discover install screen.



### Build dependencies

```bash
sudo dnf install cmake gcc-c++ qt6-qtbase-devel
```

## Building

```bash
git clone https://github.com/linuxgamerlife/lgl-scheduler-manager
cd lgl-scheduler-manager
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## Running

```bash
./build/lgl-scheduler-manager
```

A polkit agent must be running (e.g. `polkit-gnome-authentication-agent-1` or `lxqt-policykit-agent`).

## Installing system-wide

```bash
sudo cmake --install build
```

Installs the binary to `/usr/local/bin`, the `.desktop` file so it appears in your application launcher, and the icon set to `/usr/local/share/icons/hicolor`.

> Note: RPM installs place the binary in `/usr/bin`. The RPM can be uninstalled via your package manager.

## Uninstalling

RPM install:

```bash
sudo dnf remove lgl-scheduler-manager
```

Installed with `cmake --install` — run from the project directory, using the same `build` directory you installed from:

```bash
sudo xargs rm < build/install_manifest.txt
```

## Upgrading from lgl-scxctl-manager

The package was renamed in 1.1.0 and moved to the LGL Toolkit COPR, so an existing `lgl-scxctl-manager` install is not upgraded in place. The old `lgl-scxctl-manager` COPR is deprecated and receives no further updates. To switch:

```bash
sudo dnf remove lgl-scxctl-manager
sudo dnf copr remove linuxgamerlife/lgl-scxctl-manager
sudo dnf copr enable linuxgamerlife/lgl-toolkit
sudo dnf install lgl-scheduler-manager
```

The app stores no settings, so there is nothing to migrate.

## Usage

### Status Tab
Shows the currently running scheduler, mode, and service state. The **Stop Scheduler** button is only enabled when a scheduler is actually running. Status auto-refreshes every 5 seconds.

### Control Tab
1. Select a scheduler from the dropdown (populated live via `scxctl list`)
2. Choose a mode: **Auto**, **Gaming**, **Lowlatency**, or **Powersave**
3. Optionally enter custom flags (see the Flags tab for reference)
4. Click **Start** (when no scheduler is running) or **Switch** (to change the active one)
5. Check **"Enable scx_loader service on boot"** to persist the service across reboots

### Reference Tab
A table of all supported schedulers with descriptions of what each is best for, what workloads to avoid, and technical notes.

### Flags Tab
A filterable table of per-scheduler custom flags. Use the dropdown to narrow by scheduler. Flags from this table can be pasted directly into the Custom flags field on the Control tab.

### Tray Icon
- Green badge on the app icon = scheduler running
- Red badge = no scheduler active (grey until the first status check)
- Closing the window minimises to tray — the app keeps running
- Launching the app again while it is running just brings the existing window to the front
- Click the icon to show/hide the window
- Right-click for quick start/stop and **Quit** to exit fully

> **Tray support on some desktop environments / window managers** (e.g. Niri, Noctalia): a StatusNotifierItem host may be required.
>
> ```bash
> sudo dnf install gnome-shell-extension-appindicator
> ```
>
> Tested on Niri Noctalia. Other environments may differ.

### Start Minimized at Login

To start the app minimized when your desktop session starts, change the `Exec` line in the existing user autostart entry at `~/.config/autostart/lgl-scheduler-manager.desktop` to:

```ini
Exec=lgl-scheduler-manager --start-minimized
```

The application-menu launcher remains unchanged, so opening it normally still shows the window.

## Available Schedulers

The scheduler list is populated dynamically from `scxctl list`. On a typical install this includes:

| Scheduler         | Best for |
|------------------|----------|
| `scx_bpfland`     | General desktop, interactive apps — recommended default |
| `scx_beerland`    | Like bpfland but lower overhead |
| `scx_cake`        | CachyOS experimental — desktop/gaming |
| `scx_cosmos`      | General desktop and server, locality-first |
| `scx_flash`       | Batch jobs, compilation, encoding |
| `scx_lavd`        | Gaming, audio, low-latency workloads |
| `scx_layered`     | Power users wanting per-app scheduling policies |
| `scx_nest`        | Lightly-loaded desktops, turbo-boost workloads |
| `scx_p2dq`        | Mixed desktop/server, fair load balancing |
| `scx_pandemonium` | CachyOS experimental — dynamic task learning |
| `scx_rusty`       | Multi-core desktops and servers, NUMA systems |
| `scx_rustland`    | Experimentation and userspace scheduler research |
| `scx_simple`      | Testing and benchmarking only |
| `scx_tickless`    | Cloud, virtualisation, HPC, server batch workloads |

See the in-app **Reference** tab for full descriptions and the **Flags** tab for tuning options.

## Notes

- `scxctl start`, `scxctl stop`, and `scxctl switch` require root; the app uses `pkexec` for elevation
- `scxctl list` and `scxctl get` run without root
- Do **not** run `scx.service` and `scx_loader.service` simultaneously
- Disable `ananicy-cpp` if you experience scheduler watchdog timeouts
- If the scheduler list shows a built-in fallback, ensure `scx_loader.service` is running: `systemctl start scx_loader`
