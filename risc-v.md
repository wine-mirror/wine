# Wine RISC-V64 port: development and testing

This document tracks the native Linux RISC-V64 Wine bring-up on the `risc-v` branch. Use `risc-v` for builds and testing; the historical `riscv64-context-experimental` branch is already an ancestor of `risc-v`.

## Context and SEH bring-up

The experimental context changes already incorporated into `risc-v` include:

- `RtlCaptureContext` captures integer registers and control state at the caller boundary.
- `RtlRestoreContext` delegates the non-returning transfer to `NtContinue`.
- `get_native_context` returns the native `CONTEXT` pointer.

These are incremental implementations, **not** complete Windows exception or context support.

### Known limitations and validation requirements

- `RtlCaptureContext` does **not** capture floating-point state or set its floating-point context flag. It records `x10/a0` as zero because the context pointer occupies `a0` at the function boundary.
- `RtlRestoreContext` depends on `NtContinue`, the syscall dispatcher and the frame-restore path. A successful call into `NtContinue` does not establish complete register restoration.
- Remote `NtGetContextThread` and `NtSetContextThread` remain unsupported pending server-mediated suspend/context support.
- The syscall return trampoline does not restore `t6` as a saved context register. Native ELF `gp` and `tp` preservation must remain consistent with the Linux ABI.
- The newer `risc-v` branch has an **experimental** floating-point restore path for explicit context updates; it does not provide complete floating-point capture, signal-context support or verified end-to-end restoration. Earlier experimental-branch notes that FP restore was entirely absent therefore no longer describe the latest code.
- Nested Unix/syscall transitions still share one frame and require redesign and validation.
- APC dispatch, Windows exception dispatch, stack unwinding and user-mode callback paths remain incomplete or stubbed.
- `__chkstk` is currently a no-op, as in the referenced ARM64 implementation. Do not attribute an initialization hang to it without evidence.

## Remaining development roadmap

1. Preserve full integer/FP state across syscall and Unix-call transitions, including nested entries and context restores.
2. Implement Linux RISC-V64 signal-context capture/restore and signal-to-Windows exception translation.
3. Complete current/remote thread context operations with Wine server integration and suspend/resume.
4. Implement APC and exception stack layouts, dispatch and return paths.
5. Implement PE/DWARF unwind support, `RtlVirtualUnwind` and `RtlUnwindEx`.
6. Implement user-mode callbacks and add dedicated tests for `ntdll`, `kernel32` and `user32`.

## Build and smoke-test sequence

Run on the RISC-V64 target, from the existing configured build tree:

```sh
cd ~/source/wine
git fetch origin
git switch risc-v
git pull --ff-only origin risc-v
make -j2 2>&1 | tee /tmp/wine-riscv-build.log

export WINEPREFIX="$HOME/.wine-riscv-context-test"
WINEDEBUG=+timestamp,+pid,+seh,+process \
  timeout -s TERM -k 10s 120s ./wine cmd.exe /c ver \
  > /tmp/wine-riscv-context.log 2>&1

grep -E 'LdrInitializeThunk|NtContinue|err:|SIG' /tmp/wine-riscv-context.log
```

Use a fresh test prefix when isolating initialization issues. The `+riscv64` debug channel from the original bring-up instructions is optional and should be used only if registered in the current build. Compare test output with a known working baseline, and **do not infer success from wineserver shutdown alone**.

All runtime changes require compilation and execution on the RISC-V64 target before being considered validated.
