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
