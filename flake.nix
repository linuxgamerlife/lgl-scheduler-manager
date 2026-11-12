{
  description = "LGL Scheduler Manager - GTK interface for managing sched-ext BPF schedulers";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      inherit (nixpkgs) lib;

      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      forAllSystems = f: lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});

      # Keep the version in sync with CMakeLists.txt and packaging/lgl-scheduler-manager.spec.
      mkSchedulerManager =
        pkgs:
        pkgs.stdenv.mkDerivation {
          pname = "lgl-scheduler-manager";
          version = "2.0.0";

          src = self;

          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            glib
            wrapGAppsHook4
          ];

          buildInputs = with pkgs; [
            gtkmm4
            libadwaita
            json-glib
            gsettings-desktop-schemas
          ];

          # scxctl, pkexec and systemctl are system tools resolved in PATH at runtime.

          cmakeFlags = [ "-DBUILD_TESTING=ON" ];
          doCheck = true;

          meta = {
            description = "GTK interface for managing sched-ext BPF schedulers";
            homepage = "https://github.com/linuxgamerlife/lgl-scheduler-manager";
            license = lib.licenses.mit;
            mainProgram = "lgl-scheduler-manager";
            platforms = systems;
          };
        };
    in
    {
      packages = forAllSystems (pkgs: {
        default = mkSchedulerManager pkgs;
        lgl-scheduler-manager = mkSchedulerManager pkgs;
      });

      overlays.default = final: _prev: {
        lgl-scheduler-manager = mkSchedulerManager final;
      };

      apps = forAllSystems (pkgs: {
        default = {
          type = "app";
          program = "${mkSchedulerManager pkgs}/bin/lgl-scheduler-manager";
        };
      });
    };
}
