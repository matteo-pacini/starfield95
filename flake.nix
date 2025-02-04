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
        starfield95 = pkgs.callPackage ./starfield95.nix { };
      in
      {
        apps.default = {
          type = "app";
          program = "${starfield95}/bin/starfield95";
        };
      }
    );
}
