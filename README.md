# LCE

Yes.

## Building

**You will need to install Buck2, Reindeer, Rustup, Clang and a modern OpenJDK distribution >=26 (for Ruffle).**

First, apply the patches to Ruffle:

```sh
git clone https://github.com/theoparis/ruffle -b push-xntyqqztpkxt third_party/ruffle/src
cd third_party/ruffle/src
git apply ../../ruffle.patch
cd -
```

Then you can build the client.

```sh
buck2 run //Minecraft.Client:Minecraft.Client`
```
