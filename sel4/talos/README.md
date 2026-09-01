# Talos seL4 project

Talos's own seL4 code — as opposed to `examples/sel4-qemu-validation`, which
only runs upstream seL4 Foundation code (sel4test, the Rust Microkit tutorial)
to validate that the toolchain works at all.

Target: RISC-V RV64 on `qemu-riscv-virt`. See
[`docs/architecture.md`](../../docs/architecture.md) for the full layout,
the subsystem-to-issue map, and where new code belongs.

## Layout

```
libs/libtalos/      Talos's own code, one file per subsystem
  include/talos/    public headers (arch.h is the only arch-dependent seam)
  src/              boot.c, partition.c, arch/riscv/paging.c
tests/              harness + one suite per subsystem -> libtalostest
apps/talos-root/    product rootserver  (thin: sequences libtalos)
apps/talos-tests/   test rootserver     (thin: runs the suites)
```

seL4 declares one rootserver per image, so which one gets built is a configure
option (`TALOS_ROOTSERVER`, default `root`). Test code is linked only by
`apps/talos-tests`, so it cannot reach a product image.

## Build and run

From the repo root:

```bash
scripts/talos.sh build          # product rootserver -> cbuild/
scripts/talos.sh build tests    # test rootserver    -> cbuild-tests/
scripts/talos.sh test           # build tests, boot, exit non-zero unless all pass
scripts/talos.sh run            # boot the product rootserver (Ctrl-A then X to exit)
scripts/talos.sh shell          # interactive shell in the build container
```

The script drives the same container, `repo`, CMake, Ninja and QEMU pipeline
already proven in `examples/sel4-qemu-validation`, through that project's own
Makefile. It configures a build directory on first use and reuses it after.

### First time only

`repo sync` has to populate `kernel/`, `projects/` and `tools/` before anything
builds. The manifest (`default.xml`) lives in this repo rather than a dedicated
manifest repo, so `repo init` points at it with `-m`, and at the **local**
checkout with `-u` — `repo` clones via git, which only sees committed history,
so these files need a local commit but **not** a push.

```bash
scripts/talos.sh shell
# then, inside the container:
cd /host/sel4/talos
repo init -u /host -b <your-branch> -m sel4/talos/default.xml
repo sync
```

Expected `scripts/talos.sh test` output ends with:

```text
==== talOS test summary ====
  tests:  5 run, 0 failed
  checks: 35 run, 0 failed
talOS tests: ALL PASSED
RESULT: PASS
```

## Why C, not Rust

The root task is C, linked against seL4's own libraries
(`sel4utils`/`vka`/`vspace`/`sel4platsupport`), per the implementation brief
§7. The official Rust seL4 bindings require nightly Rust (`-Z build-std`) and
have no RISC-V64 target, which is incompatible with Ferrocene (a
stable-channel, safety-qualified compiler). Ferrocene Rust's role is the
partition-process layer above the root task (#10), not the root task itself.

## Provenance

Every build file here is adapted from
`examples/sel4-qemu-validation/sel4test`, which we proved boots on
`qemu-riscv-virt` with `RISCV64=TRUE`. The kernel/library revisions in
`default.xml` are copied verbatim from that project's own pinned manifest. Each
file's header comment records what changed, and every seL4 API used is cited
against the header it was verified in.
