# Wine on Linux RISC-V64: getting started and development guide

> **Experimental port — work in progress.** This guide describes the `risc-v` development branch of this fork, not an upstream-supported Wine configuration. Code merged into the branch is not necessarily verified on RISC-V hardware.

## Scope and project goals

The immediate goal is to build and run Wine's **native Linux RISC-V64 / Winelib (ELF)** path, then make thread startup, syscall dispatch, exception handling, context restoration, and unwinding reliable. This is distinct from running existing x86/x86-64 Windows binaries.

- **Current development branch:** `risc-v`. The historical `riscv64-context-experimental` branch was incorporated into it.
- **Primary target:** a Linux RISC-V64 machine, such as the Orange Pi RV, with a native GCC toolchain.
- **Near-term objective:** repeatable builds, startup and Winelib smoke tests, and architecture-specific tests for NTDLL.
- **Future possibilities:** native RISC-V64 Windows PE/COFF binaries, RISC-V vector support, and x86/x86-64 translation. None is a prerequisite for the ELF bring-up.

Prefer small, reviewable architecture-specific changes and preserve Wine's shared server protocol ABI. Do not disable assertions or assume a successful compile proves exception correctness.

## 1. Prepare the build machine

On Debian-derived RISC-V64 distributions, install the standard build dependencies required by your checkout and configuration. Useful diagnostic tools include:

```sh
sudo apt update
sudo apt install build-essential git pkg-config gdb binutils strace   autoconf automake libtool
```

Optional tools for investigating PE/COFF and repository history:

```sh
sudo apt install clang lld llvm gh xxd tree
apt-cache policy clang lld llvm
```

Package names and availability vary by distribution. `libc6-dev-riscv64-cross` is for cross-compilation and is **not required** for a native RISC-V64 build.

Confirm the native architecture:

```sh
uname -m
gcc -dumpmachine
gcc -dM -E - </dev/null | grep -E '__riscv|__SIZEOF_POINTER__'
```

A native 64-bit RISC-V compiler should report `__riscv_xlen 64`.

## 2. Check out and build

```sh
git clone https://github.com/sedwards/wine.git
cd wine
git switch risc-v
```

For an existing checkout:

```sh
git fetch origin
git switch risc-v
git pull --ff-only origin risc-v
```

Use a **separate build directory** when practical, so generated files and configuration changes are isolated:

```sh
mkdir -p ../wine-riscv-build
cd ../wine-riscv-build
../wine/configure --without-mingw
make -j2 2>&1 | tee /tmp/wine-riscv-build.log
```

The in-tree build used during bring-up is also supported by this fork's existing workflow:

```sh
cd ~/source/wine
./configure --without-mingw
make -j2 2>&1 | tee /tmp/wine-riscv-build.log
```

Choose **one** build layout and use it consistently. Do not run `make clean` or `make install` as part of routine debugging; first establish that the build and tests work. `--enable-archs=riscv64` was experimented with in older notes but is **not the default recommended configuration** for this native ELF bring-up. Confirm support in `./configure --help` before using it.

### Build one object while debugging

From the configured build directory:

```sh
make -B dlls/ntdll/unix/signal_riscv64.o
make -j2 2>&1 | tee /tmp/wine-riscv-build.log
```

Use the build system's generated compiler flags rather than replacing `CFLAGS` wholesale. The historical `-fshort-wchar` workaround is not a default requirement.

If `configure` itself was changed, regenerate it using the project's documented build procedure before reconfiguring. `autoreconf -fi` appeared in early experiments but should not be applied automatically to an otherwise configured tree.

### Whole-program ELF build coverage

The acceptance target is parity with the same Wine revision on native x86-64/AArch64,
including GUI programs and services. Existing PE binaries and CPU translation are not
required. There are currently 116 program modules: 111 native ELF targets, four Win16
exclusions (`rundll.exe16`, `winevdm`, `winhelp.exe16`, `winoldap.mod16`), and `dpnsvr`,
which upstream also excludes on native ARM64. Tests beneath program directories are
additional targets, not counted as application modules.

After building, audit the configured rules and actual ELF files from the source root:

```sh
python3 tools/riscv64-programs ../wine-riscv-build > /tmp/wine-riscv-programs.json
```

The command exits nonzero for missing rules, missing outputs, or incorrect ELF targets.
Its JSON inventory explicitly separates build status from runtime validation. PE
placeholders do not count as built ELF applications. This audit does not certify
application behavior: each applicable program still needs functional testing against
the reference build, first under full-system QEMU and then on native hardware.

For cross-builds, prepare `__tooldeps__` **and** `nls/all` in the native host-tools build;
otherwise resource compilers can fail to load `locale.nls`. Use a disk-backed build
directory with enough space for the complete build; the initial debug build exhausted
a small temporary filesystem. `--disable-tests` can prioritize application bring-up,
but it also disables `winetest`; the audit reports that as `disabled-by-config`,
not an exclusion. A test-enabled build is required for final acceptance.

Current validation: native RISC-V loader and wineserver version commands run under
QEMU user emulation; a full application build is in progress. A successful version
command is not a startup pass. QEMU user emulation without binfmt integration cannot
transparently execute RISC-V child loaders, so service/process acceptance requires
a complete target environment. Local Unix sockets must be available to wineserver.

## 3. Smoke-test startup

From the **configured build directory**, using its Wine launcher:

```sh
export WINEPREFIX="$HOME/.wine-riscv-context-test"
WINEDEBUG=+timestamp,+pid,+seh,+process \
  timeout -s TERM -k 10s 120s ./wine cmd.exe /c ver \
  > /tmp/wine-riscv-context.log 2>&1

printf 'exit status: %s\n' "$?"
grep -E 'LdrInitializeThunk|NtContinue|err:|SIG' /tmp/wine-riscv-context.log
```

This command is a **diagnostic**, not a guarantee that a Windows PE program will start on this port. Test a small native Winelib ELF executable separately; inspect its format with `file` and `readelf -h`. Use a fresh `WINEPREFIX` to isolate initialization failures. A wineserver exit or clean shutdown **does not** by itself establish that Wine initialized successfully.

Only enable a debug channel if it exists in the current source. In particular, `+riscv64` is not guaranteed to be registered.

## 4. Current implementation status

The branch contains initial implementations in several areas. **Status here means code is present, not that it passed end-to-end hardware tests.** Earlier notes described some features as entirely absent; those descriptions are now outdated.

| Area | Branch status / next validation |
| --- | --- |
| Native RISC-V64 thread initialization | Suspended startup now waits for wineserver and applies the initial context; verify TEB, stacks, and entry transitions on hardware. |
| Syscall and Unix-call dispatch | Implemented paths under development; validate nested and cross-stack transitions. |
| `RtlCaptureContext` / `RtlRestoreContext` / `NtContinue` | Initial and experimental; verify all integer and FP registers, PC, SP, and non-returning transfer. |
| Linux signal-context transition | Experimental `rt_sigreturn`-based continuation; verify signal masks and context restoration. |
| Floating-point context | LP64D syscall/Unix-call entry now captures all 32 FP registers and FCSR; normal returns restore callee-saved FP state and FCSR. Signal-context helpers share capture/restore code. Hardware integration remains unverified. |
| Nested syscall frames | Frame-chain handling and nonlocal-continuation cleanup added; complex exception paths remain unverified. |
| Native ELF/DWARF virtual unwind | Connected to native exception dispatch and frame walking. Isolated QEMU CFI tests pass for integer/FP recovery, leaf frames, exact PCs, call-site boundaries, and end-of-stack detection; full runtime/hardware validation remains pending. |
| Windows `RtlVirtualUnwind` / `RtlUnwindEx` / SEH | Incomplete; do not infer full Windows SEH from ELF unwind work. |
| APC, remote thread context, user-mode callbacks | SIGUSR1 suspension and remote context exchange implemented; regression coverage added for suspended startup and integer/FP updates. Dispatcher-boundary races and APC integration remain unverified; user-mode callbacks remain incomplete. |
| `setjmp` / `longjmp` | Initial RISC-V64 implementation; verify jump-buffer layout and nonlocal transfers. |
| RISC-V vector extension | Future work. |
| Native Windows RISC-V PE/COFF and x86 translation | Exploratory / out of current scope. |

### RISC-V64 server-context layout

Wine's `struct context_data` protocol has a fixed **1712-byte** layout. Early RISC-V64 code serialized all 32 integer registers into the integer union, growing the structure to **1728 bytes** and triggering the server's static assertion.

The branch now uses **30 transmitted integer registers**: `x1` and `x3`–`x31`. `x0` is architecturally zero; `x2` (SP) is represented by the control-register union. The corresponding packing/unpacking code lives in `dlls/ntdll/unix/thread.c`.

**Do not change or comment out the assertion** in `server/request_handlers.h`. The goal is to retain the shared protocol layout. If the build still reports a size mismatch, verify that generated headers match the current `server/protocol.def`, and measure the layout rather than changing the expected size.

After changing `server/protocol.def`, regenerate its checked-in headers from the source root:

```sh
tools/make_requests
```

The generated `include/wine/server_protocol.h` must also contain `x[30]`. Building against an older `x[32]` header still violates the layout assertion even when the protocol source is correct. Rebuild both Wine and wineserver together after a protocol version change.

## 5. Debugging and validation

Useful starting points:

```sh
# Confirm object and executable formats
file dlls/ntdll/unix/signal_riscv64.o
readelf -h dlls/ntdll/unix/signal_riscv64.o

# Inspect native unwind metadata
readelf --debug-dump=frames dlls/ntdll/unix/signal_riscv64.o

# Trace process and signal activity
strace -f -o /tmp/wine-riscv-strace.log ./wine --version

# Inspect the first build error
grep -n -m 10 -E 'error:|undefined reference|Error [0-9]' /tmp/wine-riscv-build.log
```

For a reproducible crash, collect the executable name, exact command, build commit, signal, `gdb bt`, `info registers`, and relevant `x/i` and `x/gx` output. For context and unwind defects, test leaf and non-leaf functions, saved `ra`/`s0`–`s11`, FP state, nested syscall callbacks, and nonlocal exception exits. Distinguish **native ELF DWARF unwinding** from **Windows PE unwind metadata**.

### Thread/context regression coverage

The RISC-V64 cases in `dlls/ntdll/tests/thread.c` check suspended startup, suspension of a running register loop, remote integer/FP updates, integer-only SP/PC preservation, FCSR, and native `gp`/`tp` preservation. With a working target Wine runtime, run:

```sh
make dlls/ntdll/tests/thread.ok
# Clear cached results before repeating:
make dlls/ntdll/tests/testclean
```

Cross-compiling the test object is only a compilation check. Isolated QEMU checks of dispatcher assembly and Linux signal-context helpers do not establish full Wine startup, suspension at every dispatcher instruction, Windows SEH, or callback correctness.

### ELF frame walking

Native exception dispatch and `RtlWalkFrameChain` now consult ELF unwind information
through the Unix builtin-unwind bridge before falling back to leaf unwinding. A saved
return address is looked up one byte before its PC to handle compressed instructions;
an interrupted instruction uses its exact PC. Leaf frames may preserve SP if PC advances,
and an undefined DWARF return register terminates the walk.

`dlls/ntdll/tests/exception.c` includes a RISC-V64 recursive frame-walk test covering
frame counts, skip flags, and output bounds. Run `make dlls/ntdll/tests/exception.ok`
with a working target runtime. The changed signal objects and exception test have
cross-compiled; isolated tests using assembler-generated ELF CFI passed under QEMU.
This does not complete `RtlUnwindEx`, language-specific handlers, or collided unwind.

### Development priorities

1. Obtain a clean, repeatable native build and basic Winelib startup.
2. Verify syscall/Unix-call entry and return, including `t6`, `gp`, `tp`, and nested frame bookkeeping.
3. Verify signal capture and `NtContinue` restoration of PC, SP, integer registers, and FP state.
4. Test ELF CIE/FDE parsing, CFA computation, register recovery, and unwind progress across multiple frames.
5. Complete Windows exception dispatch, `RtlVirtualUnwind`, `RtlUnwindEx`, and collided/nonlocal unwinding.
6. Complete APCs, thread suspension and remote context handling, callbacks, and debugger integration.
7. Add regression tests suitable for upstream review.

## 6. Historical experiments and optional future work

The following notes are **retained for research**, not recommended build steps.

### LLVM / RISC-V64 COFF experiments

An earlier attempt to generate Windows-style PE/COFF files with Clang and LLD did not establish a working Wine PE runtime. Compiler support for a target triple, COFF object generation, PE linking, Wine loader recognition, and executable startup are **separate questions**. Support varies with installed LLVM versions.

To check whether a particular Clang accepts a COFF target:

```sh
clang --version
ld.lld --version
command -v lld-link

cat >/tmp/riscv-pe-test.c <<'EOF'
int main(void) { return 0; }
EOF

clang --target=riscv64-windows-coff -c /tmp/riscv-pe-test.c \
  -o /tmp/riscv-pe-test.obj
file /tmp/riscv-pe-test.obj
```

Even a successful `.obj` compilation **does not** imply that `clang --target=riscv64-windows-coff -fuse-ld=lld ...` can link a usable executable, or that Wine can load it. Earlier bring-up logs included `STATUS_INVALID_IMAGE_WIN_64 (c000035a)`; investigate machine validation separately from compiler/linker support. Claims of full upstream LLVM RISC-V Windows COFF support in old notes were not independently verified for the installed toolchain.

### Historical local workarounds — do not apply automatically

The old scratch notes recorded these commands:

```sh
# Historical linker workaround, not a default build setting:
make -j4 LDFLAGS="-Wl,-u,__wine_riscv64_get_teb -latomic"

# Historical investigation of preloader placement:
make -C loader wine-preloader \
  WINEPRELOADER_LDFLAGS="-nostartfiles -nodefaultlibs -static -Wl,-Ttext=0x200100000" \
  -B V=1

# Historical workaround for a missing build-tool path:
ln -s ../winebuild/winebuild tools/winegcc/winebuild
```

Do **not** create the symlink, override linker flags, or force a preloader address without first reproducing and understanding the underlying issue on a clean tree. These are debugging artifacts, not project requirements.

### Future architecture options

- Native RISC-V64 PE/COFF generation and loader support.
- RISC-V vector register context and unwind handling.
- WOW64 or x86/x86-64 binary translation through a separately designed emulation layer.

None should block the native Linux/Winelib ELF milestone.

## Contributing and reporting results

Keep changes small, architecture-scoped, and consistent with existing x86-64/AArch64 Wine patterns. Prefer protocol-neutral solutions; include tests and explain ABI assumptions. For a useful bug report, record the commit (`git rev-parse HEAD`), toolchain versions, configure options, failing command, first error, and GDB trace where applicable.

This guide reflects the development branch and its known experiments as of **November 2026**. Update the status table only after confirming the implementation and its tests on the target hardware.
