{
  description = "TALOS: seL4 + Ferrocene Rust APEX-style RTOS";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";

    # Pinned by rev + narHash in flake.lock. A relative `path:../` input can't
    # be used here: it resolves against the flake's store copy.
    ferrocene-nix.url = "git+https://dev.x1sys.com/tools/ferrocene-nix.git";
  };

  outputs =
    {
      self,
      nixpkgs,
      ferrocene-nix,
    }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      ferrocene = ferrocene-nix.packages.${system};
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        packages = [
          # Ferrocene Rust compiler, Cargo, and the embedded LLVM toolchain
          # (linker and related tools); `full` bundles both.
          ferrocene.full

          # Project build and emulator utilities.
          pkgs.cmake
          pkgs.gnumake
          pkgs.ninja
          pkgs.qemu
        ];
      };
    };
}
