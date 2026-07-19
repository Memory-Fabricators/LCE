# LCE

Yes.

## Building

You will need Bazel >=9.2.0 installed and a modern OpenJDK distribution >=26.

`bazel build //...`

## Development

To generate the rust-project.json for rust-analyzer:

`bazel run @rules_rust//tools/rust_analyzer:gen_rust_project`

To generate the compile_commands.json for clangd:

`bazel run @hedron_compile_commands//:refresh_all`
