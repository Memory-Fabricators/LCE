{
  description = "Development environment for Legacy Console Edition";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/master";

  outputs =
    { nixpkgs, ... }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "riscv64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forEachSystem = nixpkgs.lib.genAttrs systems;
    in
    {
      devShells = forEachSystem (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
        in
        with pkgs;
        {
          default = pkgs.mkShell {
            nativeBuildInputs = [
              ninja
              meson
              pkg-config
              rustc
              rust-analyzer
              rustfmt
              cbindgen
              # ruffle_core's build.rs compiles its AVM1/AVM2 "playerglobal"
              # ActionScript bytecode via a bundled asc.jar - it shells out to
              # a plain `java` on PATH, so a JDK has to be present at build
              # time (not needed at runtime).
              jdk25_headless
              llvmPackages.clang-tools
            ];

            buildInputs = [
              libx11
              libxcb
              libGL
            ]
            ++ lib.optionals stdenv.hostPlatform.isDarwin [
              wayland
            ];
          };
        }
      );
    };
}
