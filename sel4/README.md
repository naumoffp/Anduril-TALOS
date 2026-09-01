# seL4

Talos's own seL4 code (as opposed to `examples/sel4-qemu-validation`, which
only validates the upstream seL4/QEMU/Rust toolchain).

- [`talos/`](talos/) — the Talos seL4 project: `libs/libtalos` (Talos's own
  code), `tests/` (harness + per-subsystem suites), and two rootservers,
  `apps/talos-root` (product) and `apps/talos-tests`. See its README for
  layout, build/run instructions, and how to add a subsystem.

Build and test from the repo root:

```bash
scripts/talos.sh build    # product rootserver
scripts/talos.sh test     # test rootserver; non-zero exit unless all pass
scripts/talos.sh run      # boot the product rootserver
```
