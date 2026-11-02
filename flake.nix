{
  description = "An C Development Environment";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = {self, nixpkgs,}: 
    let 
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in 
    {
      devShells.${system}.default = pkgs.mkShell.override{stdenv = pkgs.clangStdenv;} {
        nativeBuildInputs = with pkgs; [
          # Build tools
          cmake
          ninja
          pkg-config

          # Development tools
          clang
          lld
          clang-tools
          lldb
        ];

        buildInputs = with pkgs; [
          sdl3
          libGL
        ];
      };
    };
}
