Name:           lgl-scheduler-manager
Version:        1.1.1
Release:        1%{?dist}
Summary:        Qt6 GUI for managing sched-ext BPF schedulers via scxctl

License:        MIT
URL:            https://github.com/linuxgamerlife/lgl-scheduler-manager
Source0:        https://github.com/linuxgamerlife/%{name}/archive/refs/tags/v%{version}.zip
BuildRequires:  cmake >= 3.16
BuildRequires:  unzip
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel
Requires:       qt6-qtbase
Requires:       polkit
# scx-tools is detected at runtime — the app shows a setup guide if absent
Recommends:     scx-tools
Recommends:     scx-scheds

%description
SCX Scheduler Manager is a Qt6 graphical interface for managing sched-ext
BPF schedulers through the scxctl command-line tool.

It allows starting, stopping, and switching schedulers, viewing live
scheduler status, managing scx_loader.service autostart, and browsing
per-scheduler flags and workload reference data.

If scxctl is not installed, the application opens in setup mode and
displays installation guidance without hard-failing.

%prep
%autosetup -n %{name}-%{version}

%build
mkdir -p build
cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=%{_prefix}
%make_build

%install
# Main binary
install -Dm755 build/lgl-scheduler-manager \
    %{buildroot}%{_bindir}/lgl-scheduler-manager

# Desktop entry
install -Dm644 lgl-scheduler-manager.desktop \
    %{buildroot}%{_datadir}/applications/lgl-scheduler-manager.desktop

# Icons (hicolor theme, one per size)
for icon in packaging/hicolor/*/apps/%{name}.png; do
    size=$(basename "$(dirname "$(dirname "$icon")")")
    install -Dm644 "$icon" \
        %{buildroot}%{_datadir}/icons/hicolor/$size/apps/%{name}.png
done

# Pixmaps fallback
install -Dm644 packaging/hicolor/256x256/apps/%{name}.png \
    %{buildroot}%{_datadir}/pixmaps/lgl-scheduler-manager.png

# AppStream metainfo
install -Dm644 packaging/com.linuxgamerlife.lgl-scheduler-manager.metainfo.xml \
    %{buildroot}%{_datadir}/metainfo/com.linuxgamerlife.lgl-scheduler-manager.metainfo.xml

%post
if [ -x /usr/bin/gtk-update-icon-cache ]; then
    gtk-update-icon-cache -f -t %{_datadir}/icons/hicolor &>/dev/null || :
fi
if [ -x /usr/bin/update-desktop-database ]; then
    update-desktop-database -q %{_datadir}/applications &>/dev/null || :
fi
if [ -x /usr/bin/appstreamcli ]; then
    appstreamcli refresh --force &>/dev/null || :
fi
if [ -x /usr/bin/kbuildsycoca6 ]; then
    kbuildsycoca6 &>/dev/null || :
fi

%postun
if [ -x /usr/bin/gtk-update-icon-cache ]; then
    gtk-update-icon-cache -f -t %{_datadir}/icons/hicolor &>/dev/null || :
fi
if [ -x /usr/bin/update-desktop-database ]; then
    update-desktop-database -q %{_datadir}/applications &>/dev/null || :
fi
if [ -x /usr/bin/appstreamcli ]; then
    appstreamcli refresh --force &>/dev/null || :
fi

%files
%license LICENSE
%doc README.md CHANGELOG.md
%{_bindir}/lgl-scheduler-manager
%{_datadir}/applications/lgl-scheduler-manager.desktop
%{_datadir}/icons/hicolor/*/apps/lgl-scheduler-manager.png
%{_datadir}/pixmaps/lgl-scheduler-manager.png
%{_datadir}/metainfo/com.linuxgamerlife.lgl-scheduler-manager.metainfo.xml

%changelog
* Wed Oct 14 2026 LinuxGamerLife <contact@linuxgamerlife.com> - 1.1.1-1
- Add --start-minimized for desktop-session autostart

* Sun Sep 20 2026 LinuxGamerLife <contact@linuxgamerlife.com> - 1.1.0-1
- Project renamed from lgl-scxctl-manager to lgl-scheduler-manager
- New hicolor icon set; tray icon is now the app icon with a status badge
- Reference tab ordered to match the scheduler dropdowns
- Fix: only one instance of the app can run at a time
- Fix: clicking the tray icon shows/hides the window

* Sat Apr 25 2026 LinuxGamerLife <contact@linuxgamerlife.com> - 1.0.1-1
- Closing the main window now minimises to system tray instead of quitting

* Mon Mar 23 2026 LinuxGamerLife <contact@linuxgamerlife.com> - 1.0.0-1
- Initial release
