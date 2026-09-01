# Talos repository architecture

How the repo is laid out and where new code goes. The
backlog is 23 issues spanning partitions, processes, a Partition Manager,
timing and the domain scheduler; this document exists so none of that has to be
re-litigated per issue.

The sponsor spec defines the system as five layers:

> Target hardware (RISC-V RV64, ARM Cortex-A) → seL4 microkernel → Talos runtime
> (partition manager + scheduler + health monitor) → ARINC-653-inspired runtime
> API → Rust user space software

The [Subsystem map](#subsystem-map) is that stack, one row per component.
[Challenge coverage](#challenge-coverage) tracks how much of each sponsor
challenge is actually owned by an issue — **several spec deliverables are not**,
and are listed there rather than left silently absent.

Target today: RISC-V RV64 (`qemu-riscv-virt`) only. Arm Cortex-A is part of
sponsor challenge 1, not a stretch goal — see
[Architecture boundary](#architecture-boundary).

## Layout

```
Anduril-TALOS/
├── flake.nix                   Ferrocene dev shell only — NOT yet the build (ch.1)
├── scripts/talos.sh            build / test / run driver (wraps the container)
├── docs/                       architecture (this file), apex-scope, mvp
├── tests/                      host-side artifacts: timing analysis (ch.3),
│                               fault-injection harness (ch.2) — not yet used
├── rust/                       Ferrocene Rust user space (#10) — not yet used
├── examples/
│   ├── ferrocene-host-hello/   Ferrocene toolchain smoke test
│   ├── riscv32-no-std-hello/   bare-metal RISC-V UART hello world
│   └── sel4-qemu-validation/   UPSTREAM validation only, zero Talos code
└── sel4/talos/                 THE TALOS seL4 PROJECT
    ├── default.xml             repo manifest, pinned to sel4test's revisions
    ├── CMakeLists.txt          shared build env; picks exactly one rootserver
    ├── settings.cmake          kernel config
    ├── easy-settings.cmake     build toggles, incl. TALOS_ROOTSERVER
    ├── libs/libtalos/          Talos's own code, one file per subsystem
    │   ├── include/talos/
    │   └── src/  (+ src/arch/riscv/)
    ├── tests/                  harness + one suite per subsystem (libtalostest)
    │   ├── include/talos_test.h
    │   └── src/test_<subsystem>.c
    └── apps/
        ├── talos-root/         product rootserver   (thin: sequences libtalos)
        └── talos-tests/        test rootserver      (thin: runs the suites)
```

Everything under `sel4/talos/` except the directories above is fetched by
`repo sync` or written by the build, and is gitignored: `kernel/`, `projects/`,
`tools/`, `.repo/`, `cbuild*/`, `.sel4_cache/`, `init-build.sh`, `griddle`.

## Subsystem map

One file per subsystem in `libs/libtalos`, one matching suite in `tests/`. The
"spec layer" column ties each row back to the sponsor's five-layer architecture;
rows with no issue are spec deliverables nobody owns yet.

| Subsystem | Spec layer | Source | Tests | Issues | State |
|---|---|---|---|---|---|
| Bootstrap | runtime | `src/boot.c` | — | #1 | done |
| Partition | runtime | `src/partition.c` | `test_partition.c` | #3, #6, #23 | VSpace + CSpace done; modes pending |
| Process | runtime | `src/process.c` | `test_process.c` | #5, #14, #15 | not started |
| Partition Manager | runtime — PM | `src/pm.c` | `test_pm.c` | #7, #8, #17, #19 | not started |
| Scheduler | runtime — scheduler | `src/schedule.c` | `test_schedule.c` | #22, #24 | not started |
| **Health monitor** | **runtime — HM** | `src/health.c` | `test_health.c` | **none** | **unowned** |
| IPC | runtime | `src/ipc.c` | `test_ipc.c` | **none** (ch.1, ch.2) | not started |
| Time | runtime | `src/time.c` | `test_time.c` | #11, #12, #13, #16 | not started |
| APEX ABI + client | ARINC-653 API | `libs/libapex/` | `test_apex.c` | #9, #18 | not started |
| Rust user space | Rust user space | `rust/` | #20, #21 demos | #10 | not started |

Only files with real code exist today. Create a subsystem's file when its first
issue is picked up, not before — an empty `pm.c` is clutter, this table is the
plan.

Two rows deserve their emphasis:

- **Health monitor** is one of the three components the sponsor spec names
  inside the Talos runtime, alongside the partition manager and the scheduler.
  It has no issue and no file. The ARINC 653 formal spec this project otherwise
  leans on explicitly excludes it (see [Grounding](#grounding)), so it must be
  sourced from **ARINC 653 Part 1** instead — that is a reason to change source,
  not to drop the component.
- **IPC** is split out of `pm.c` because the spec treats it as its own
  deliverable twice over: "IPC primitives" in challenge 1, and
  "capability-controlled IPC" plus a **cross-core IPC** demonstration in
  challenge 2. Folding it into the PM's APEX endpoint would bury a deliverable.

### Where a given issue's code goes

- **Kernel objects, capabilities, invocations** → `libs/libtalos`. Arch-neutral,
  no `seL4_RISCV_*` outside `src/arch/`.
- **Anything a test needs to assert** → a public function in `include/talos/`.
  Tests link the library; they do not reach into private statics.
- **Sequencing and policy constants** (which partition, at what vaddr, how many
  slots) → `apps/talos-root/src/main.c`. The library takes them as arguments.
- **The APEX client ABI** (#9, #18) → `libs/libapex`, kept separate from
  `libtalos` because Ferrocene Rust partitions link against it and the root task
  does not. #9 decides C FFI vs. pure Rust `ecall`; the split survives either.
- **Host-side tooling** (timing analysis, fault injection, log post-processing)
  → the repo-root `tests/`, not `sel4/talos/tests/`. The latter is on-target
  test code that boots in QEMU; the former is anything that runs on the
  developer's machine and produces challenge-3 artifacts.

### Rust user space must build twice

Challenge 1 asks for "a Ferrocene Rust user space runtime **shared across host
simulation and target hardware**", and the spec lists three environments:
Linux-host simulation, QEMU, and RV64/Cortex-A hardware.

So `rust/` is not simply "the partition ELF". It needs one source tree that
builds for a host target and an seL4 target, which means the platform-specific
half — how a process makes an APEX call — has to sit behind a trait or module
boundary from the start:

```
rust/
├── talos-apex/        the APEX API surface, platform-agnostic
│   └── src/backend/   seL4 (ecall or C FFI, per #9)  |  host (shim)
└── talos-partition/   example partition using talos-apex (#10, #21)
```

Deciding this before #10 is much cheaper than retrofitting it after a partition
ELF already exists.

## Challenge coverage

The sponsor spec sets three challenges. The 23 GitHub issues cover roughly
challenge 1's root task plus the temporal slice of challenge 2. This table
exists so the rest does not get mistaken for "not reached yet" when it is
actually "not tracked".

### Challenge 1 — seL4 bring-up, Nix build, Rust user space

| Deliverable | Owner | State |
|---|---|---|
| seL4 on QEMU RISC-V RV64 | #1, #6 | done |
| seL4 on QEMU ARM Cortex-A | **none** | **unowned** — seam exists (`arch.h`), 8 symbols measured |
| Root task | #6 | done |
| IPC primitives | **none** | **unowned** — `src/ipc.c` |
| Ferrocene Rust user space runtime | #10 | not started |
| ...shared with Linux-host simulation | **none** | **unowned** — see [Rust user space must build twice](#rust-user-space-must-build-twice) |
| **Nix-based reproducible build system** | **none** | **unowned** — see below |

### Challenge 2 — ARINC-653 BMP scheduler

| Deliverable | Owner | State |
|---|---|---|
| Major/minor-frame schedule | #22, #24 | not started |
| Static assignment of partitions to **cores** (BMP) | **none** | **unowned** — see below |
| Capability-controlled IPC | **none** | **unowned** — `src/ipc.c` |
| Temporal isolation | #11, #22, #24 | not started |
| Spatial isolation | #3, #6 | structural only, not demonstrated |
| Cross-core IPC demonstration | **none** | **unowned** |
| Runtime health monitoring | **none** | **unowned** — `src/health.c` |
| Fault injection | **none** | **unowned** — repo-root `tests/` |

### Challenge 3 — hardware, characterization, artifacts

| Deliverable | Owner | State |
|---|---|---|
| Physical RV64 or Cortex-A target | **none** | **unowned** — PolarFire SoC / Kria-K24 named in spec |
| Timing characterization vs QEMU/host baseline | **none** | **unowned** — repo-root `tests/` |
| Fault-containment characterization | **none** | **unowned** |
| Architecture spec | partial | this document is repo layout, not a *system* architecture spec |
| Test artifacts | partial | `sel4/talos/tests/` produces them; nothing archives them |
| Timing analysis, final report, software release | **none** | **unowned** |

### The three that most affect structure

**Nix.** Challenge 1 asks to "create a Nix-based reproducible build system", and
the Background repeats "built reproducibly with Nix". Today `flake.nix` provides
only a Ferrocene dev shell, and the actual build is Docker + `repo` + CMake +
Ninja, wrapped by `scripts/talos.sh`. That was a deliberate call — no `pkgs.seL4`
exists in nixpkgs, and wrapping seL4's CMake build in Nix is genuine unsolved
work — but it is a deferral of a named deliverable, not an alternative to it, and
every convenience added to `scripts/talos.sh` deepens the hole. Whoever takes
this should expect to keep the Docker path working during the transition.

**ARM Cortex-A.** The spec puts Cortex-A in challenge 1 next to RV64, not in a
later phase. `arch.h` is the structural answer and the cost is measured (8
symbols, below), but no issue owns the work.

**Bound multiprocessing is multi-core.** Challenge 2's headline is "static
assignment of partitions to **cores**". `settings.cmake` currently forces
`KernelMaxNumNodes 1`, and the scheduler row above only covers the major/minor
frame. Core binding is a separate mechanism (`seL4_TCB_SetAffinity` on non-MCS,
or derived from the scheduling context on MCS). It also carries the project's
largest open technical unknown — see the SMP note under
[Grounding](#grounding) — which is worth settling on QEMU early, because
challenge 2's main demonstration depends on it.

## Rootservers and tests

seL4 declares exactly one rootserver per image, so Talos has two, selected at
configure time:

```bash
../init-build.sh ... -DTALOS_ROOTSERVER=root    # apps/talos-root  (product)
../init-build.sh ... -DTALOS_ROOTSERVER=tests   # apps/talos-tests (suites)
```

`scripts/talos.sh` keeps these in separate build directories (`cbuild/` and
`cbuild-tests/`) so both stay warm. This mirrors how sel4test ships a single
rootserver, and means **no test code can reach a product image** — `libtalostest`
is linked only by `apps/talos-tests`.

Both rootservers share `talos_boot_init()`, so the bring-up sequence exists once.

### Writing tests

Tests prove behaviour using **documented seL4 error codes as oracles**, not just
absence of errors. The partition suite is the pattern to copy: rather than
asserting "`PageTable_Map` returned 0", it asserts that a *further* map returns
`seL4_DeleteFirst`, which per manual §10.10.6.1 is only possible when every
paging level for that address is populated. Similarly a frame mapping that
succeeds at `vaddr_base` and fails with `seL4_FailedLookup` one leaf-span higher
proves the paging chain, the ASID assignment, *and* that we covered exactly the
address range intended.

Adding a suite is three steps, listed in `tests/include/talos_test.h`. New files
under `tests/src/` are globbed automatically.

Tests must leave the system as they found it: unmap and free probe objects,
delete copied capabilities.

## Architecture boundary

`libs/libtalos/include/talos/arch.h` is the **only** place architecture leaks in.
seL4 has no neutral spelling for ASID or paging invocations — they are
`seL4_RISCV_*`, `seL4_ARM_*` and so on.

The boundary is sized from measurement, not guesswork: an AArch64 build of the
pre-refactor code **configured cleanly** (so the manifest, `settings.cmake` and
CMake wiring are already arch-neutral) and failed with 13 compile errors across
exactly 8 symbols, all now behind `arch.h`:

```
CONFIG_PT_LEVELS, seL4_RISCV_PageTableObject, seL4_RISCV_Default_VMAttributes,
seL4_RISCV_ASIDPool_Assign, seL4_RISCV_PageTable_{Map,Unmap},
seL4_RISCV_Page_{Map,Unmap}
```

Seven are renames. **`CONFIG_PT_LEVELS` is not.** RISC-V has a level *count* and
one PageTable object type for every level; AArch64 has no such count, it has a
distinct type per level (`PageGlobalDirectory` / `PageUpperDirectory` /
`PageDirectory` / `PageTable`). That is why `talos_arch_pagetable_type()` takes a
level argument the RISC-V implementation ignores. Porting means adding
`src/arch/aarch64/paging.c`; a non-RISC-V build currently stops at a CMake
`FATAL_ERROR` pointing at that header rather than emitting 13 confusing errors.

## Grounding

Design decisions trace to two documents, both worth reading before extending a
subsystem.

**ARINC 653 formal specification** — arXiv:2206.14189, "A Formal Specification
of Operating System based on ARINC 653" (Wang & Zhang). A Circus model, not an
seL4 paper. It gives:

- `PARTITION_KERNEL_STATE` (App. A.4) — one `KernelPageTableRef` per partition
  plus `ProcessTable`, `ReadyQueue`, `WaitingQueue`, `TimeCounterQueue`,
  `CurrentProcess`. **This is the shape `partition_t` targets**, and the fields
  it still lacks are annotated with the issues that bring them.
- Partition modes `IDLE | COLD_START | WARM_START | NORMAL` and start conditions
  `NORMAL_START | PARTITION_RESTART | HM_MODULE_RESTART | HM_PARTITION_RESTART`
  (#23).
- A `CoreKernel` scheduling partitions over **minor frames** within a major
  frame, and a `PartitionKernel` scheduling processes by priority (#22, #24).
  The paper calls these "time windows"; the sponsor spec calls them minor
  frames, and that is the vocabulary this project uses — one or more minor
  frames sum to the major frame duration.
- The 20 APEX services: 2 partition management, 14 process management, 4 time
  management.

It **excludes** inter/intra-partition communication (ports, buffers,
blackboards) and the Health Monitor — both explicitly future work in its §3.
That matters more than it sounds: the Health Monitor is one of the three
components the sponsor spec names inside the Talos runtime, so for it, and for
ports, **ARINC 653 Part 1 is the source, not this paper**. Do not infer from the
paper's scope that these are out of Talos's scope.

**seL4 Reference Manual** v16.0.0. Load-bearing sections:

- §7.1.2.6 — RV64 page sizes: 4 KiB maps at level 2, so root + 2 intermediate
  tables, which is why `CONFIG_PT_LEVELS - 1`.
- §7.1.4 — a VSpace is unusable until assigned an ASID.
- §7.3 — intermediate paging structures may **not** be shared, but pages may be,
  and **VSpaces may be shared by threads**. That is what makes "processes share
  their partition's VSpace" natively supported, and what ARINC ports will rest on
  (`seL4_CNode_Copy` the frame cap, then map it into the second VSpace).
- §10.10.5.2 / §10.10.6.1 — the `Page_Map` / `PageTable_Map` error codes the test
  suite uses as oracles.
- §6.1.6 — 256 priority levels, preemptive highest-priority-first. Maps onto
  ARINC process priority scheduling.
- **§6.3 and §10.3.2 — the domain schedule is configurable at runtime** via
  `seL4_DomainSet_ScheduleConfigure()` and `seL4_DomainSet_ScheduleSetStart()`,
  with `KernelNumDomainSchedules` entries and a zero domain+duration end marker.
  The manual states this "makes it possible to configure multiple independent
  domain schedules and switch between them atomically" — a direct fit for ARINC
  major frames and mode changes (#23, #24), and newer than most write-ups assume.
  Two caveats, the second of which is the project's biggest open unknown:
  durations are kernel ticks on non-MCS and timer ticks on MCS, so major/minor
  frame granularity depends on #11; and **the manual is silent on domains under
  SMP** — it describes a single global schedule with one starting index and no
  per-core schedules. Challenge 2's headline deliverable is partitions statically
  bound to cores under a domain-driven frame schedule, so this interaction should
  be proven on QEMU before anything is built on the assumption that it works.
- §6.1.2, §6.1.5, §6.1.10 — thread affinity, the mechanism for the "static
  assignment of partitions to cores" half of BMP, separate from the domain
  schedule that provides the frame. On non-MCS a thread defaults to the node its
  TCB was created on and `seL4_TCB_SetAffinity()` overrides it (§6.1.2); on MCS
  affinity comes from the scheduling context (§6.1.10). §6.1.5 warns that how an
  affinity value maps to a core is platform-specific and **not portable across
  platforms** — relevant with QEMU, PolarFire SoC and Kria-K24 all in scope.

## Known placeholders

Things that are deliberately provisional, so nobody mistakes them for decisions:

- `PARTITION_VADDR_BASE = 0x10000000` — arbitrary. The real value falls out of
  wherever the partition ELF loads (#10). The two intermediate page tables built
  for it may need rebuilding then.
- `PARTITION_CNODE_SIZE_BITS = 4` (16 slots) — a guess until process counts are
  real (#7).
- The partition CNode has **no guard**, so it is not yet usable as a CSpace root
  a thread resolves CPtrs through. That is the remainder of #3.
- Partition memory is best-fit from anywhere in physical memory, so a
  partition's objects are scattered. ARINC spatial partitioning ultimately wants
  a bounded reserved region per partition; decide before #10.
- `KernelNumDomains`/`KernelMaxNumNodes`/`KernelIsMCS` are forced to 1/1/OFF in
  `settings.cmake`. They become real toggles at #11 (MCS) and #24 (domains).
  `KernelMaxNumNodes 1` in particular means the build is currently single-core,
  which challenge 2's BMP scheduler cannot stay within.
- Nothing has been *prevented* from touching anything yet. Isolation is
  structural, not demonstrated — that needs a TCB that faults (#5, #7).
- `docs/apex-scope.md` and `docs/mvp.md` are empty placeholder files, not
  content. `docs/architecture.md` is repo layout; the *system* architecture spec
  challenge 3 asks for does not exist yet.
- The build is Docker-based, not Nix-based. See
  [Challenge coverage](#challenge-coverage) — this is a deferred deliverable,
  not a settled design choice.
