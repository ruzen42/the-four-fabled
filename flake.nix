{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    
    iron.url = "github:ruzen42/iron-log";
    enet6.url = "github:ruzen42/enet6-flake";
  };

  outputs = { self, nixpkgs, flake-utils, iron, enet6 }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };

        ironPkg = iron.packages.${system}.default;
        enetPkg = enet6.packages.${system}.default;

        app = pkgs.stdenv.mkDerivation {
          pname = "the-four-fabled";
          version = "0.1.0";

          src = ./.;

          nativeBuildInputs = with pkgs; [ muon samurai pkg-config ];
          
          buildInputs = with pkgs; [
            raylib
            ironPkg
            enetPkg
          ];

          configurePhase = ''
            export PKG_CONFIG_PATH="${enetPkg}/lib/pkgconfig:${ironPkg}/lib/pkgconfig:${pkgs.raylib}/lib/pkgconfig:$PKG_CONFIG_PATH"
            
            export CFLAGS="-I${pkgs.raylib}/include -I${ironPkg}/include -I${enetPkg}/include"
            export LDFLAGS="-L${pkgs.raylib}/lib -L${ironPkg}/lib -L${enetPkg}/lib"            muon meson setup build --prefix=$out
          '';

          buildPhase = ''
            samu -C build
          '';

          installPhase = ''
            samu -C build install
          '';
        };

      in {
        packages.default = app;

        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            gcc 
            muon 
            samurai
            pkg-config
            pkgs.raylib
            ironPkg
            enetPkg
          ];

          shellHook = ''
            export PKG_CONFIG_PATH="${ironPkg}/lib/pkgconfig:${pkgs.raylib}/lib/pkgconfig:$PKG_CONFIG_PATH"
            export CFLAGS="-I${pkgs.raylib}/include -I${ironPkg}/include"
            export LDFLAGS="-L${pkgs.raylib}/lib -L${ironPkg}/lib"
          '';
        };
      }
    );
}
