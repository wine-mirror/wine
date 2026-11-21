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

## Testing
/home/orangepi/source/wine/loader/wine dlls/ntdll/tests/ntdll_test.exe.so alpc

WINEPREFIX="$HOME/.wine-riscv64" \
WINE_RISCV_RPC_DIAG=1 \
WINE_RISCV_DIAG=1 \
WINEDEBUG=+rpc,+file,+server,+seh \
./wine cmd.exe /c echo RV64-RPC-TEST \
    > /tmp/rv64-rpc-diag.log 2>&1

echo "Wine exit status: $?"

*WINE_RISCV_SKIP_THREAD_TRACE=1*
Can be used to test if Wine's debugging infrastructure itself causes any crashes
*WINE_RISCV_BOOT_DIAG=1*

```
#!/bin/bash

TID="${1:-155298}"

if [[ ! -r "/proc/$TID/syscall" ]]; then
    echo "Cannot inspect TID $TID"
    exit 1
fi

echo "=== RPC worker syscall ==="
cat "/proc/$TID/syscall"

echo
echo "=== RPC worker file descriptors ==="

for fd in /proc/"$TID"/fd/*; do
    target=$(readlink "$fd" 2>/dev/null) || continue

    [[ "$target" == pipe:* ]] || continue

    printf '%-6s %s\n' "${fd##*/}" "$target"
done

echo
echo "=== Processes holding those pipes ==="

for fd in /proc/"$TID"/fd/*; do
    target=$(readlink "$fd" 2>/dev/null) || continue
    [[ "$target" == pipe:* ]] || continue

    echo
    echo "Searching for $target"

    for proc in /proc/[0-9]*; do
        pid="${proc##*/}"

        for other in "$proc"/fd/*; do
            other_target=$(readlink "$other" 2>/dev/null) || continue

            if [[ "$other_target" == "$target" ]]; then
                printf 'PID=%-8s FD=%-4s COMM=%s\n' \
                    "$pid" "${other##*/}" \
                    "$(cat "$proc/comm" 2>/dev/null)"
            fi
        done
    done
done
```

## GBD
```
gdb -q ./dlls/ntdll/tests/ntdll_test.exe.so core     -ex 'set pagination off'     -ex 'info registers a0 a1 a2 a3 a4 a5 a6 a7 sp ra pc'     -ex 'p $_siginfo'     -ex 'info symbol $pc'     -ex 'x/12i $pc-16'     -ex 'x/16gx $sp'     -ex 'quit'


### Find running wine processess
```
ps -eo pid,ppid,stat,wchan:30,etime,comm,args |     grep -E 'wineserver|wine|services.exe|cmd.exe|explorer.exe'
```

### And then match the pid up to see the state
```
for pid in 155283 155287 155289 155297 155303; do
    [ -d "/proc/$pid" ] || continue

    echo
    echo "===== PID $pid ====="

    ps -p "$pid" -o pid,ppid,stat,wchan:30,comm,args

    echo "File descriptors:"
    ls -l "/proc/$pid/fd" 2>/dev/null

    echo "Thread states:"
    ps -T -p "$pid" \
        -o pid,tid,stat,wchan:30,comm
done
```

### Debuging console IO with cmd.exe and conhost.exe for example

```
sudo strace -f -tt -T -s 128 \
    -e trace=read,readv,write,writev,ioctl,ppoll,epoll_pwait,futex \
    -p 164512 \
    -p 164514 \
    -o /tmp/rv64-console-input.log
```

```
. Inspect every conhost thread
Please run:
ps -T -p 164512 \
    -o pid,tid,stat,wchan:30,comm
``
Then:
for task in /proc/164512/task/*; do
    tid="${task##*/}"

    echo
    echo "===== conhost TID $tid ====="

    cat "$task/syscall" 2>/dev/null
    cat "$task/wchan" 2>/dev/null
done
```


orangepi@orangepirv:~/source/wine$ gdb -q \
    ./dlls/ntdll/tests/ntdll_test.exe.so \
    core \
    -ex 'set pagination off' \
    -ex 'info threads' \
    -ex 'thread apply all bt 20' \
    -ex 'info symbol $pc' \
    -ex 'info symbol $ra' \
    -ex 'x/20i $pc-32' \
    -ex 'info proc mappings' \
    -ex 'quit'
Reading symbols from ./dlls/ntdll/tests/ntdll_test.exe.so...

warning: Can't open file /tmp/.wine-1000/server-b302-3fc1e/tmpmap-f143731f (deleted) during file-backed mapping note processing

warning: Can't open file /tmp/.wine-1000/server-b302-3fc1e/tmpmap-69e53bdf (deleted) during file-backed mapping note processing

warning: core file may not match specified executable file.
[New LWP 166795]
[New LWP 166784]
Core was generated by `dlls/ntdll/tests/ntdll_test.exe.so pipe                                       '.
Program terminated with signal SIGSEGV, Segmentation fault.
#0  0x0000003fbb87c7f6 in ?? ()
[Current thread is 1 (LWP 166795)]
  Id   Target Id         Frame 
* 1    LWP 166795        0x0000003fbb87c7f6 in ?? ()
  2    LWP 166784        0x0000003fbb8b1642 in ?? ()

Thread 2 (LWP 166784):
#0  0x0000003fbb8b1642 in ?? ()

Thread 1 (LWP 166795):
#0  0x0000003fbb87c7f6 in ?? ()
No symbol matches $pc.
No symbol matches $ra.
   0x3fbb87c7d6:	sb	a6,0(a5)
   0x3fbb87c7da:	addi	a5,a5,1
   0x3fbb87c7dc:	andi	a3,a5,7
   0x3fbb87c7e0:	bnez	a3,0x3fbb87c7d6
   0x3fbb87c7e2:	add	a2,a2,a0
   0x3fbb87c7e4:	sub	a2,a2,a5
   0x3fbb87c7e6:	srli	a6,a2,0x6
   0x3fbb87c7ea:	beqz	a6,0x3fbb87c814
   0x3fbb87c7ee:	slli	a3,a6,0x6
   0x3fbb87c7f2:	mv	a7,a5
   0x3fbb87c7f4:	add	a3,a3,a5
=> 0x3fbb87c7f6:	sd	a4,0(a5)
   0x3fbb87c7f8:	sd	a4,8(a5)
   0x3fbb87c7fa:	sd	a4,16(a5)
   0x3fbb87c7fc:	sd	a4,24(a5)
   0x3fbb87c7fe:	sd	a4,32(a5)
   0x3fbb87c800:	sd	a4,40(a5)
   0x3fbb87c802:	sd	a4,48(a5)
   0x3fbb87c804:	sd	a4,56(a5)
   0x3fbb87c806:	addi	a5,a5,64
Mapped address spaces:

          Start Addr           End Addr       Size     Offset objfile
          0x7ffe0000         0x7ffe1000     0x1000        0x0 /tmp/.wine-1000/server-b302-3fc1e/tmpmap-f143731f (deleted)
         0x200100000        0x200102000     0x2000     0x1000 /home/orangepi/source/wine/loader/wine-preloader
         0x200102000        0x200103000     0x1000     0x2000 /home/orangepi/source/wine/loader/wine-preloader
         0xe63580000        0xe63582000     0x2000        0x0 /home/orangepi/source/wine/nls/l_intl.nls
         0xe63690000        0xe636a6000    0x16000        0x0 /home/orangepi/source/wine/dlls/apisetschema/riscv64-windows/apisetschema.dll
         0xe636d0000        0xe637ae000    0xde000        0x0 /home/orangepi/source/wine/nls/locale.nls
         0xe637b0000        0xe637c1000    0x11000        0x0 /home/orangepi/source/wine/nls/c_1252.nls
         0xe637d0000        0xe637e1000    0x11000        0x0 /home/orangepi/source/wine/nls/c_437.nls
         0xe638f0000        0xe63c2e000   0x33e000        0x0 /home/orangepi/source/wine/nls/sortdefault.nls
         0xe63c30000        0xe63c3f000     0xf000        0x0 /home/orangepi/source/wine/nls/normnfc.nls
         0xe63c60000        0xe63c71000    0x11000        0x0 /home/orangepi/source/wine/nls/c_20127.nls
         0xe63c80000        0xe63dc8000   0x148000        0x0 /tmp/.wine-1000/server-b302-3fc1e/tmpmap-69e53bdf (deleted)
         0xe63f50000        0xe63f61000    0x11000        0x0 /home/orangepi/source/wine/nls/c_10000.nls
        0x3fb9886000       0x3fb9890000     0xa000        0x0 /home/orangepi/source/wine/dlls/imm32/imm32.dll.so
        0x3fb9891000       0x3fb98a9000    0x18000     0xb000 /home/orangepi/source/wine/dlls/imm32/imm32.dll.so
        0x3fb98a9000       0x3fb98aa000     0x1000    0x23000 /home/orangepi/source/wine/dlls/imm32/imm32.dll.so
        0x3fb98aa000       0x3fb98ad000     0x3000    0x24000 /home/orangepi/source/wine/dlls/imm32/imm32.dll.so
        0x3fb98ad000       0x3fb98b8000     0xb000        0x0 /var/cache/fontconfig/fd07936d-327f-42e5-829a-cc2673917c54-le64.cache-7
        0x3fb98b8000       0x3fb98cb000    0x13000        0x0 /var/cache/fontconfig/d764fd5a-017c-433a-b05a-92c12178fa30-le64.cache-7
        0x3fb98cb000       0x3fb98d9000     0xe000        0x0 /var/cache/fontconfig/09671b41-fd94-4931-988f-eab2fe1bcae0-le64.cache-7
        0x3fb98d9000       0x3fb98df000     0x6000        0x0 /usr/lib/riscv64-linux-gnu/libuuid.so.1.3.0
        0x3fb98df000       0x3fb98e0000     0x1000     0x5000 /usr/lib/riscv64-linux-gnu/libuuid.so.1.3.0
        0x3fb98e0000       0x3fb98e1000     0x1000     0x6000 /usr/lib/riscv64-linux-gnu/libuuid.so.1.3.0
        0x3fb98e1000       0x3fb9900000    0x1f000        0x0 /usr/lib/riscv64-linux-gnu/libexpat.so.1.8.10
        0x3fb9900000       0x3fb9902000     0x2000    0x1f000 /usr/lib/riscv64-linux-gnu/libexpat.so.1.8.10
        0x3fb9902000       0x3fb9903000     0x1000    0x21000 /usr/lib/riscv64-linux-gnu/libexpat.so.1.8.10
        0x3fb9903000       0x3fb9936000    0x33000        0x0 /usr/lib/riscv64-linux-gnu/libfontconfig.so.1.12.0
        0x3fb9936000       0x3fb9938000     0x2000    0x32000 /usr/lib/riscv64-linux-gnu/libfontconfig.so.1.12.0
        0x3fb9938000       0x3fb9939000     0x1000    0x34000 /usr/lib/riscv64-linux-gnu/libfontconfig.so.1.12.0
        0x3fb9939000       0x3fb9959000    0x20000        0x0 /usr/lib/riscv64-linux-gnu/libbrotlicommon.so.1.0.9
        0x3fb9959000       0x3fb995a000     0x1000    0x1f000 /usr/lib/riscv64-linux-gnu/libbrotlicommon.so.1.0.9
        0x3fb995a000       0x3fb995b000     0x1000    0x20000 /usr/lib/riscv64-linux-gnu/libbrotlicommon.so.1.0.9
        0x3fb995b000       0x3fb9963000     0x8000        0x0 /usr/lib/riscv64-linux-gnu/libbrotlidec.so.1.0.9
        0x3fb9963000       0x3fb9964000     0x1000     0x7000 /usr/lib/riscv64-linux-gnu/libbrotlidec.so.1.0.9
        0x3fb9964000       0x3fb9965000     0x1000     0x8000 /usr/lib/riscv64-linux-gnu/libbrotlidec.so.1.0.9
        0x3fb9965000       0x3fb9978000    0x13000        0x0 /usr/lib/riscv64-linux-gnu/libz.so.1.2.13
        0x3fb9978000       0x3fb9979000     0x1000    0x13000 /usr/lib/riscv64-linux-gnu/libz.so.1.2.13
        0x3fb9979000       0x3fb997a000     0x1000    0x14000 /usr/lib/riscv64-linux-gnu/libz.so.1.2.13
        0x3fb997a000       0x3fb999f000    0x25000        0x0 /usr/lib/riscv64-linux-gnu/libpng16.so.16.39.0
        0x3fb999f000       0x3fb99a0000     0x1000    0x25000 /usr/lib/riscv64-linux-gnu/libpng16.so.16.39.0
        0x3fb99a0000       0x3fb99a1000     0x1000    0x26000 /usr/lib/riscv64-linux-gnu/libpng16.so.16.39.0
        0x3fb99a1000       0x3fb9a28000    0x87000        0x0 /usr/lib/riscv64-linux-gnu/libfreetype.so.6.18.3
        0x3fb9a28000       0x3fb9a2f000     0x7000    0x86000 /usr/lib/riscv64-linux-gnu/libfreetype.so.6.18.3
        0x3fb9a2f000       0x3fb9a30000     0x1000    0x8d000 /usr/lib/riscv64-linux-gnu/libfreetype.so.6.18.3
        0x3fb9a30000       0x3fb9a9b000    0x6b000        0x0 /usr/lib/riscv64-linux-gnu/libm.so.6
        0x3fb9a9b000       0x3fb9a9c000     0x1000    0x6a000 /usr/lib/riscv64-linux-gnu/libm.so.6
        0x3fb9a9c000       0x3fb9a9d000     0x1000    0x6b000 /usr/lib/riscv64-linux-gnu/libm.so.6
        0x3fb9aa0000       0x3fb9aae000     0xe000        0x0 /var/cache/fontconfig/f17bd919-57be-4cf4-bdce-e93a0728c0de-le64.cache-7
        0x3fb9aae000       0x3fb9c60000   0x1b2000        0x0 /home/orangepi/source/wine/dlls/win32u/win32u.so
        0x3fb9c60000       0x3fb9c7b000    0x1b000   0x1b1000 /home/orangepi/source/wine/dlls/win32u/win32u.so
        0x3fb9c7b000       0x3fb9c8a000     0xf000   0x1cc000 /home/orangepi/source/wine/dlls/win32u/win32u.so
        0x3fb9cf9000       0x3fb9d10000    0x17000        0x0 /home/orangepi/source/wine/dlls/win32u/win32u.dll.so
        0x3fb9d11000       0x3fb9d3c000    0x2b000    0x18000 /home/orangepi/source/wine/dlls/win32u/win32u.dll.so
        0x3fb9d3c000       0x3fb9d3d000     0x1000    0x42000 /home/orangepi/source/wine/dlls/win32u/win32u.dll.so
        0x3fb9d3d000       0x3fb9d4d000    0x10000    0x43000 /home/orangepi/source/wine/dlls/win32u/win32u.dll.so
        0x3fb9d4d000       0x3fb9d60000    0x13000        0x0 /home/orangepi/source/wine/dlls/gdi32/gdi32.dll.so
        0x3fb9d61000       0x3fb9dc1000    0x60000    0x14000 /home/orangepi/source/wine/dlls/gdi32/gdi32.dll.so
        0x3fb9dc1000       0x3fb9dc4000     0x3000    0x73000 /home/orangepi/source/wine/dlls/gdi32/gdi32.dll.so
        0x3fb9dc4000       0x3fb9dcc000     0x8000    0x76000 /home/orangepi/source/wine/dlls/gdi32/gdi32.dll.so
        0x3fb9dcc000       0x3fb9de0000    0x14000        0x0 /home/orangepi/source/wine/dlls/user32/user32.dll.so
        0x3fb9de1000       0x3fb9e8c000    0xab000    0x15000 /home/orangepi/source/wine/dlls/user32/user32.dll.so
        0x3fb9e8c000       0x3fb9e8d000     0x1000    0xbf000 /home/orangepi/source/wine/dlls/user32/user32.dll.so
        0x3fb9e8d000       0x3fb9f72000    0xe5000    0xc0000 /home/orangepi/source/wine/dlls/user32/user32.dll.so
        0x3fb9f72000       0x3fb9f90000    0x1e000        0x0 /home/orangepi/source/wine/dlls/ucrtbase/ucrtbase.dll.so
        0x3fb9f91000       0x3fba027000    0x96000    0x1f000 /home/orangepi/source/wine/dlls/ucrtbase/ucrtbase.dll.so
        0x3fba027000       0x3fba028000     0x1000    0xb5000 /home/orangepi/source/wine/dlls/ucrtbase/ucrtbase.dll.so
        0x3fba028000       0x3fba03c000    0x14000    0xb6000 /home/orangepi/source/wine/dlls/ucrtbase/ucrtbase.dll.so
        0x3fba03e000       0x3fba050000    0x12000        0x0 /home/orangepi/source/wine/dlls/sechost/sechost.dll.so
        0x3fba051000       0x3fba064000    0x13000    0x13000 /home/orangepi/source/wine/dlls/sechost/sechost.dll.so
        0x3fba064000       0x3fba065000     0x1000    0x25000 /home/orangepi/source/wine/dlls/sechost/sechost.dll.so
        0x3fba065000       0x3fba069000     0x4000    0x26000 /home/orangepi/source/wine/dlls/sechost/sechost.dll.so
        0x3fba069000       0x3fba080000    0x17000        0x0 /home/orangepi/source/wine/dlls/msvcrt/msvcrt.dll.so
        0x3fba081000       0x3fba0f6000    0x75000    0x18000 /home/orangepi/source/wine/dlls/msvcrt/msvcrt.dll.so
        0x3fba0f6000       0x3fba0f7000     0x1000    0x8c000 /home/orangepi/source/wine/dlls/msvcrt/msvcrt.dll.so
        0x3fba0f7000       0x3fba102000     0xb000    0x8d000 /home/orangepi/source/wine/dlls/msvcrt/msvcrt.dll.so
        0x3fba104000       0x3fba110000     0xc000        0x0 /home/orangepi/source/wine/dlls/advapi32/advapi32.dll.so
        0x3fba111000       0x3fba144000    0x33000     0xd000 /home/orangepi/source/wine/dlls/advapi32/advapi32.dll.so
        0x3fba144000       0x3fba145000     0x1000    0x40000 /home/orangepi/source/wine/dlls/advapi32/advapi32.dll.so
        0x3fba145000       0x3fba14e000     0x9000    0x41000 /home/orangepi/source/wine/dlls/advapi32/advapi32.dll.so
        0x3fba14e000       0x3fba160000    0x12000        0x0 /home/orangepi/source/wine/dlls/kernelbase/kernelbase.dll.so
        0x3fba161000       0x3fba1fd000    0x9c000    0x13000 /home/orangepi/source/wine/dlls/kernelbase/kernelbase.dll.so
        0x3fba1fd000       0x3fba1fe000     0x1000    0xae000 /home/orangepi/source/wine/dlls/kernelbase/kernelbase.dll.so
        0x3fba1fe000       0x3fba278000    0x7a000    0xaf000 /home/orangepi/source/wine/dlls/kernelbase/kernelbase.dll.so
        0x3fba27a000       0x3fba290000    0x16000        0x0 /home/orangepi/source/wine/dlls/kernel32/kernel32.dll.so
        0x3fba291000       0x3fba2c8000    0x37000    0x17000 /home/orangepi/source/wine/dlls/kernel32/kernel32.dll.so
        0x3fba2c8000       0x3fba2c9000     0x1000    0x4d000 /home/orangepi/source/wine/dlls/kernel32/kernel32.dll.so
        0x3fba2c9000       0x3fba2de000    0x15000    0x4e000 /home/orangepi/source/wine/dlls/kernel32/kernel32.dll.so
        0x3fba2de000       0x3fba2f0000    0x12000        0x0 /home/orangepi/source/wine/dlls/ntdll/ntdll.dll.so
        0x3fba2f1000       0x3fba383000    0x92000    0x13000 /home/orangepi/source/wine/dlls/ntdll/ntdll.dll.so
        0x3fba383000       0x3fba384000     0x1000    0xa5000 /home/orangepi/source/wine/dlls/ntdll/ntdll.dll.so
        0x3fba384000       0x3fba392000     0xe000    0xa6000 /home/orangepi/source/wine/dlls/ntdll/ntdll.dll.so
        0x3fba495000       0x3fba4b0000    0x1b000        0x0 /home/orangepi/source/wine/dlls/ntdll/tests/ntdll_test.exe.so
        0x3fba4b1000       0x3fba64c000   0x19b000    0x1c000 /home/orangepi/source/wine/dlls/ntdll/tests/ntdll_test.exe.so
        0x3fba64c000       0x3fba663000    0x17000   0x1b6000 /home/orangepi/source/wine/dlls/ntdll/tests/ntdll_test.exe.so
        0x3fba663000       0x3fba67f000    0x1c000   0x1cd000 /home/orangepi/source/wine/dlls/ntdll/tests/ntdll_test.exe.so
        0x3fbaa50000       0x3fbaa54000     0x4000        0x0 /var/cache/fontconfig/d1b3b405-3d75-4f91-b7ce-3bd29da41787-le64.cache-7
        0x3fbab54000       0x3fbae3d000   0x2e9000        0x0 /usr/lib/locale/locale-archive
        0x3fbb73d000       0x3fbb74f000    0x12000        0x0 /usr/lib/riscv64-linux-gnu/libgcc_s.so.1
        0x3fbb74f000       0x3fbb750000     0x1000    0x11000 /usr/lib/riscv64-linux-gnu/libgcc_s.so.1
        0x3fbb750000       0x3fbb751000     0x1000    0x12000 /usr/lib/riscv64-linux-gnu/libgcc_s.so.1
        0x3fbb751000       0x3fbb7c5000    0x74000        0x0 /home/orangepi/source/wine/dlls/ntdll/ntdll.so
        0x3fbb7c5000       0x3fbb7c7000     0x2000    0x74000 /home/orangepi/source/wine/dlls/ntdll/ntdll.so
        0x3fbb7c7000       0x3fbb7c9000     0x2000    0x76000 /home/orangepi/source/wine/dlls/ntdll/ntdll.so
        0x3fbb803000       0x3fbb925000   0x122000        0x0 /usr/lib/riscv64-linux-gnu/libc.so.6
        0x3fbb925000       0x3fbb928000     0x3000   0x122000 /usr/lib/riscv64-linux-gnu/libc.so.6
        0x3fbb928000       0x3fbb92a000     0x2000   0x125000 /usr/lib/riscv64-linux-gnu/libc.so.6
        0x3fbb93a000       0x3fbb93c000     0x2000        0x0 /var/cache/fontconfig/b7197d5f-5a5a-47a4-a813-5924e22523e3-le64.cache-7
        0x3fbb93c000       0x3fbb93f000     0x3000        0x0 /var/cache/fontconfig/e7a84f39-2bb6-4402-9327-009c624e530e-le64.cache-7
        0x3fbb93f000       0x3fbb940000     0x1000        0x0 /usr/lib/riscv64-linux-gnu/libpthread.so.0
        0x3fbb940000       0x3fbb941000     0x1000        0x0 /usr/lib/riscv64-linux-gnu/libpthread.so.0
        0x3fbb941000       0x3fbb942000     0x1000     0x1000 /usr/lib/riscv64-linux-gnu/libpthread.so.0
        0x3fbb942000       0x3fbb946000     0x4000        0x0 /usr/lib/riscv64-linux-gnu/libatomic.so.1.2.0
        0x3fbb946000       0x3fbb947000     0x1000     0x3000 /usr/lib/riscv64-linux-gnu/libatomic.so.1.2.0
        0x3fbb947000       0x3fbb948000     0x1000     0x4000 /usr/lib/riscv64-linux-gnu/libatomic.so.1.2.0
        0x3fbb94a000       0x3fbb966000    0x1c000        0x0 /usr/lib/riscv64-linux-gnu/ld-linux-riscv64-lp64d.so.1
        0x3fbb966000       0x3fbb967000     0x1000    0x1c000 /usr/lib/riscv64-linux-gnu/ld-linux-riscv64-lp64d.so.1
        0x3fbb967000       0x3fbb969000     0x2000    0x1d000 /usr/lib/riscv64-linux-gnu/ld-linux-riscv64-lp64d.so.1
        0x3fbb969000       0x3fbb96a000     0x1000        0x0 /home/orangepi/source/wine/loader/wine
        0x3fbb96a000       0x3fbb96b000     0x1000     0x1000 /home/orangepi/source/wine/loader/wine
        0x3fbb96b000       0x3fbb96c000     0x1000     0x2000 /home/orangepi/source/wine/loader/wine
orangepi@orangepirv:~/source/wine$ 

