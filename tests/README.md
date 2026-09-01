# Host-side tests and artifacts

Anything that runs on the **developer's machine** rather than on the target.

Not to be confused with [`sel4/talos/tests/`](../sel4/talos/tests/), which is
on-target test code compiled into the test rootserver and booted under QEMU.
That tree answers "does the runtime behave correctly"; this one answers "what
did it measure, and what do we hand the sponsor".

Empty so far. Planned contents, from the sponsor challenges:

| Contents | Challenge | Issues |
|---|---|---|
| Fault-injection harness | 2 — "demonstrate... fault injection" | none yet |
| Timing characterization vs. the QEMU/host baseline | 3 | none yet |
| Fault-containment characterization | 3 | none yet |
| Timing analysis and archived test artifacts | 3 | none yet |

See [`docs/architecture.md`](../docs/architecture.md) — the "Challenge coverage"
section tracks which of these are owned by an issue. At the time of writing,
none are.

On-target runs are driven by `scripts/talos.sh test`, which exits non-zero on a
failed check, a timeout, or a missing verdict line — so it can be used directly
as the data-collection step for the work above.
