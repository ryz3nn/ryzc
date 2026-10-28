{
  description = "Modern C++ Development Environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
    supportedSystems = [ "x86_64-linux" "aarch64-linux"];
  forEachSystem = f: nixpkgs.lib.genAttrs supportedSystems (system: f (import nixpkgs { inherit system; }));
  in
  {
    devShells = forEachSystem (pkgs: {
        default = pkgs.mkShell.override { stdenv = pkgs.llvmPackages.stdenv; } {
        packages = with pkgs; [
# 1. Build Orchestration & Compilers
        cmake
        ninja
        pkg-config

# 2. Language Server, Formatter & Linter
        llvmPackages.clang-tools  # provides clangd, clang-format, clang-tidy
        cppcheck

# 3. Debugging & Analysis
        lldb
        gdb

# 4. Libraries

        ] ++ pkgs.lib.optionals pkgs.stdenv.isLinux [
# Linux-only profiling / memory diagnosis
        valgrind
        ];

        shellHook = ''
          echo "⚡ C++ Development Environment Loaded"
          echo "Compiler: $(clang++ --version | head -n1)"
          echo "CMake:    $(cmake --version | head -n1)"
          echo "Ninja:    $(ninja --version)"
          echo "----------------------------------------"
          '';
        };
    });
  };
}
