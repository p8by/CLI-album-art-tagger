{
  description = "CLI album art tagger";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs = inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [ "x86_64-linux" "aarch64-linux" ];

      perSystem = { pkgs, ... }: {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "tag";
          version = "0-unstable-2026-09-04";

          src = ./.;

          nativeBuildInputs = [ pkgs.pkg-config ];
          buildInputs = [ pkgs.taglib pkgs.zlib ];

          installPhase = ''
            runHook preInstall
            install -Dm755 tag $out/bin/tag
            runHook postInstall
          '';
        };
      };
    };
}
