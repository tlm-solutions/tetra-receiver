{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.11";

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
                gnuradio = super.gnuradio3_8.override({
                  unwrapped = self.callPackage ./pkgs/gnuradio.nix { gnuradio = super.gnuradio3_8; };
                });
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
