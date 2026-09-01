# seL4 QEMU Validation Example

This example records two validated milestones for TALOS:

1. Boot seL4 and run the official `sel4test` suite on a generic 64-bit RISC-V QEMU machine.
2. Run the official Rust `#![no_std]` Microkit Hello World protection-domain example on seL4 in QEMU.

The two upstream repositories used by this example are Git submodules:

```text
examples/sel4-qemu-validation/
├── sel4-qemu-validation-README.md
├── seL4-docker/
└── seL4-rust-tutorial/
```

This validates the upstream seL4/QEMU/Rust boot path. It is not yet the TALOS Nix-managed Ferrocene build or the TALOS APEX-inspired runtime.

## 1. Clone TALOS with its submodules

For a new clone:

```bash
git clone --recurse-submodules git@github.com:Anduril-TALOS/Anduril-TALOS.git
cd Anduril-TALOS
```

If TALOS was already cloned, fetch the submodules with:

```bash
git submodule update --init --recursive
```

## 2. Install host prerequisites

These commands are for a Linux Mint or Ubuntu-style host:

```bash
sudo apt update
sudo apt install -y docker.io qemu-system-misc make repo git
```

Start Docker and allow the current user to use it without `sudo`:

```bash
sudo systemctl enable --now docker
sudo usermod -aG docker "$USER"
```

Log out and back in once after changing the Docker group. Then verify the tools:

```bash
docker --version
qemu-system-riscv64 --version
repo --version
```

## 3. Run the official seL4 QEMU test

From the TALOS repository root, open the official seL4 build container. The validation example directory is mounted inside the container as `/host`.

```bash
make -C examples/sel4-qemu-validation/seL4-docker \
  user HOST_DIR="$PWD/examples/sel4-qemu-validation"
```

Inside the container, initialize and build the official `sel4test` project:

```bash
cd /host
mkdir sel4test
cd sel4test

repo init -u https://github.com/seL4/sel4test-manifest.git
repo sync

mkdir cbuild
cd cbuild

../init-build.sh \
  -DPLATFORM=qemu-riscv-virt \
  -DRISCV64=TRUE \
  -DSIMULATION=TRUE

ninja
./simulate
```

QEMU boots a generic RISC-V 64 machine, loads seL4, and runs the `sel4test` root task. Success ends with:

```text
All is well in the universe
```

Exit QEMU with `Ctrl-A`, then `X`.

This proves:

```text
Linux host → Docker build environment → QEMU RISC-V 64 → seL4 → user-space root task
```

## 4. Run the Rust no_std Microkit Hello World example

Open a normal host terminal, from the TALOS repository root:

```bash
cd examples/sel4-qemu-validation/seL4-rust-tutorial

make -C docker build
make -C docker run
make -C docker exec
```

The last command opens the Rust-on-seL4 tutorial container. Inside it, run:

```bash
cd workspaces/microkit/hello-world
make simulate
```

Expected output includes:

```text
Hello, World!
```

Exit QEMU with `Ctrl-A`, then `X`.

This proves:

```text
QEMU RISC-V 64 → seL4 → Microkit → Rust no_std protection domain → Hello World
```

## Scope and next steps

This example does not yet:

- build TALOS source with Ferrocene;
- implement the TALOS trusted Rust-to-seL4 bridge;
- implement APEX-inspired process, IPC, or scheduling services;
- provide full ARINC 653 temporal partitioning; or
- target the PolarFire SoC hardware.

Those are later TALOS milestones. The generated `sel4test/` checkout and its `cbuild/` outputs are local validation artifacts and must not be committed.
