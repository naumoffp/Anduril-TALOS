Small issue with using the Ferrocene Rust tool chain, with my machien and maybe others, resulting in needing to do some steps to get cargo to run properly.

For Linux:

In project folder, for example testhello, run:

mkdir -p .cargo
nano .cargo/config.toml

Then paste:

[build]
target = "x86_64-unknown-linux-gnu"

[target.x86_64-unknown-linux-gnu]
linker = "/usr/bin/gcc"
rustflags = [
  "-C",
  "link-arg=-fno-lto",
]

Save and Exit (CTRL + o, Enter, CTRL + x)

Reason being likely due to generalizations of the tool chain, cargo lacks the typcial assumed target so we add it back.
