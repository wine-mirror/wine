# RISC-V64 Wine context/SEH bring-up (experimental)

This branch is an **incremental, untested** staging branch based on `risc-v`.
Do not merge it without building and running on a RISC-V64 target.

## Commits in this series

1. Capture the integer/control context at the caller boundary in
   `RtlCaptureContext`; make `RtlRestoreContext` delegate to `NtContinue`
   instead of silently returning.
2. Return the native `CONTEXT` from `get_native_context`.

## Important limitations

- `RtlCaptureContext` captures integer registers and control state only;
  the floating-point context is **not captured** and its flag is not set.
  Its x10/a0 field is recorded as zero because the call uses a0 to pass
  the context pointer; reconstructing the original argument register is
  not generally possible at the function boundary.
- `RtlRestoreContext` depends on the existing `NtContinue` implementation,
  syscall dispatcher, and frame restore. It is **not** a complete fix.
- Remote `NtGetContextThread`/`NtSetContextThread` remain unsupported.
  Do not replace their error returns with success without implementing
  server-mediated suspend/context access.
- The syscall return path does not restore t6 or FP state. Full register
  restoration requires a carefully designed assembly trampoline.
- Nested Unix/syscall transitions still share one frame.
- APC, exception dispatch, unwind, and callback functions remain stubs.
- `__chkstk` is still a no-op, as on Wine ARM64; do not assume this is
  the source of the current initialization hang.

## Test sequence

```sh
cd ~/source/wine
git fetch origin
git switch riscv64-context-experimental
make -j2
export WINEPREFIX="$HOME/.wine-riscv-context-test"
WINEDEBUG=+timestamp,+pid,+riscv64,+seh,+process \
  timeout -s TERM -k 10s 120s ./wine cmd.exe /c ver \
  > /tmp/wine-riscv-context.log 2>&1
grep -E 'LdrInitializeThunk|NtContinue|err:|SIG' /tmp/wine-riscv-context.log
```

Test with a fresh prefix and compare with the baseline `risc-v` branch.
Do not infer success from wineserver shutdown alone.

## Remaining patch groups (not implemented)

1. Syscall and Unix-call dispatcher frame preservation, nested entry,
   FP registers, and complete context restore.
2. Current and remote thread context APIs, including server-mediated
   suspend/resume and correct signal-context handling.
3. APC and exception stack layout, dispatch, and return paths.
4. PE/DWARF unwind support, `RtlVirtualUnwind`, and `RtlUnwindEx`.
5. Exception registration, callback dispatch, and dedicated tests.
