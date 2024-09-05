{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";

    utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, utils, nixpkgs, ... }:
    utils.lib.eachDefaultSystem
      (system:
        let
          pkgs = import nixpkgs {
            inherit system;
            overlays = [
              (self: super: {
                gnuradio3_8 = super.gnuradio3_8.override {
                  unwrapped = super.gnuradio3_8.unwrapped.override {
                    features = {
                      basic = true;
                      volk = true;
                      doxygen = false;
                      sphinx = false;
                      python-support = false;
                      testing-support = false;
                      gnuradio-runtime = true;
                      gr-ctrlport = false;
                      gnuradio-companion = false;
                      gr-blocks = true;
                      gr-fec = false;
                      gr-fft = true;
                      gr-dtv = false;
                      gr-trellis = false;
                      gr-audio = false;
                      gr-zeromq = false;
                      gr-uhd = false;
                      gr-modtool = false;
                      gr-video-sdl = false;
                      gr-vocoder = false;
                      examples = false;
                      gr-utils = false;
                      gr-qtgui = false;
                      gr-blocktool = false;
                      gr-wavelet = false;
                    };
                  };
                };
              })
            ];
          };

          gr-clenabled = pkgs.callPackage ./pkgs/gr-clenabled.nix { };
          tetra-receiver = pkgs.callPackage ./pkgs/tetra-receiver.nix { };
        in
        rec {
          checks = packages;
          packages = {
            inherit gr-clenabled tetra-receiver;
            default = tetra-receiver;
          };
        }
      ) // {
      overlays.default = final: prev: {
        inherit (self.packages."x86_64-linux")
        tetra-receiver;
      };
      nixosModules = {
        tetra-receiver = {
          imports = [
            ./nixos-modules/tetra-receiver.nix
          ];
          
          nixpkgs.overlays = [
            self.overlays.default
          ];
        };
        default = self.nixosModules.tetra-receiver;
      };
      hydraJobs =
        let
          hydraSystems = [ "x86_64-linux" ];
          hydraBlacklist = [ ];
        in
        builtins.foldl'
          (hydraJobs: system:
            builtins.foldl'
              (hydraJobs: pkgName:
                if builtins.elem pkgName hydraBlacklist
                then hydraJobs
                else
                  nixpkgs.lib.recursiveUpdate hydraJobs {
                    ${pkgName}.${system} = self.packages.${system}.${pkgName};
                  }
              )
              hydraJobs
              (builtins.attrNames self.packages.${system})
          )
          { }
          hydraSystems;
    };
}
