Name:           lgl-scheduler-manager
Version:        2.0.0
Release:        1%{?dist}
Summary:        GTK interface for managing sched-ext BPF schedulers

License:        MIT
URL:            https://github.com/linuxgamerlife/lgl-scheduler-manager
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.20
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig(gtkmm-4.0) >= 4.20
BuildRequires:  pkgconfig(libadwaita-1) >= 1.6
BuildRequires:  pkgconfig(json-glib-1.0)
BuildRequires:  desktop-file-utils
BuildRequires:  appstream
Requires:       polkit
Recommends:     scx-tools
Recommends:     scx-scheds

%description
LGL SCX Scheduler Manager is a GTK 4 graphical interface for viewing, starting,
stopping, and switching sched-ext BPF schedulers through scxctl. It also provides
scheduler and custom-flag reference information.

%prep
%autosetup

%build
%cmake -DBUILD_TESTING=ON
%cmake_build

%check
%ctest

%install
%cmake_install
desktop-file-validate %{buildroot}%{_datadir}/applications/com.linuxgamerlife.lgl-scheduler-manager.desktop
appstreamcli validate --no-net %{buildroot}%{_datadir}/metainfo/com.linuxgamerlife.lgl-scheduler-manager.metainfo.xml

%files
%license LICENSE
%doc README.md CHANGELOG.md HELP.md
%{_bindir}/lgl-scheduler-manager
%{_datadir}/applications/com.linuxgamerlife.lgl-scheduler-manager.desktop
%{_datadir}/metainfo/com.linuxgamerlife.lgl-scheduler-manager.metainfo.xml
%{_datadir}/icons/hicolor/48x48/apps/lgl-scheduler-manager.png
%{_datadir}/icons/hicolor/48x48/apps/lgl-scheduler-manager-unknown.png
%{_datadir}/icons/hicolor/48x48/apps/lgl-scheduler-manager-running.png
%{_datadir}/icons/hicolor/48x48/apps/lgl-scheduler-manager-stopped.png
%{_datadir}/icons/hicolor/64x64/apps/lgl-scheduler-manager.png
%{_datadir}/icons/hicolor/128x128/apps/lgl-scheduler-manager.png
%{_datadir}/icons/hicolor/256x256/apps/lgl-scheduler-manager.png

%changelog
* Wed Oct 21 2026 LinuxGamerLife <222456170+linuxgamerlife@users.noreply.github.com> - 2.0.0-1
- Release the completed GTK 4 application as version 2.0.0
- Publish through the LGL Toolkit COPR
- Add copyable custom-flag syntax, tray menu synchronization, and menu keyboard toggles
- Fix DBusMenu GVariant ownership and release cancelled tray actions
- Reject embedded NUL bytes in custom scheduler flags
- Document the known KWin close-button workaround

* Wed Oct 21 2026 LinuxGamerLife <222456170+linuxgamerlife@users.noreply.github.com> - 1.2.0-7
- Inset window controls to keep the close button outside the compositor resize corner

* Wed Oct 21 2026 LinuxGamerLife <222456170+linuxgamerlife@users.noreply.github.com> - 1.2.0-6
- Recreate the main-menu popover after tray restoration
- Use native hide-on-close and rebuild the window surface when hidden
- Make F10 and Alt toggle the main menu from both the window and popover

* Wed Oct 21 2026 LinuxGamerLife <222456170+linuxgamerlife@users.noreply.github.com> - 1.2.0-5
- Restore primary-menu input and defer tray close hiding until GTK completes the event

* Wed Oct 21 2026 LinuxGamerLife <222456170+linuxgamerlife@users.noreply.github.com> - 1.2.0-4
- Use the repository GitHub address in package metadata

* Wed Oct 21 2026 LinuxGamerLife <222456170+linuxgamerlife@users.noreply.github.com> - 1.2.0-3
- Use GTK's native hide-on-close lifecycle for reliable tray restoration

* Wed Oct 21 2026 LinuxGamerLife <222456170+linuxgamerlife@users.noreply.github.com> - 1.2.0-2
- Fix tray menu activation and window restoration input handling
- Add copyable custom-flag syntax reference

* Wed Oct 21 2026 LinuxGamerLife <222456170+linuxgamerlife@users.noreply.github.com> - 1.2.0-1
- Migrate the maintained application from Qt 6 to GTK 4
- Add standard LGL theming, title bars, menu, shortcuts, help, settings, and tests

* Wed Oct 14 2026 LinuxGamerLife <222456170+linuxgamerlife@users.noreply.github.com> - 1.1.1-2
- Fix the version shown in the About dialog
