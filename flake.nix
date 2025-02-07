{
  description = "Starfield95";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        pkgsStatic = nixpkgs.legacyPackages.${system}.pkgsStatic;
        staticSDL2 =
          (pkgsStatic.SDL2.override {
            x11Support = !pkgsStatic.stdenv.hostPlatform.isLinux;
          }).overrideAttrs
            (old: {
              postFixup = "";
            });
        starfield95 = pkgs.callPackage ./starfield95.nix { };
        staticStarfield = pkgsStatic.callPackage ./starfield95.nix {
          SDL2 = staticSDL2;
          pkg-config = pkgs.pkg-config;
        };
      in
      {
        packages = {
          default = starfield95;
          static = staticStarfield;
        };

        apps.default = {
          type = "app";
          program = "${starfield95}/bin/starfield95";
        };
      }
    );
}
