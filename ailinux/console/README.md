# AILinux Windows Console Compatibility: ConPTY resize

Wine 11.20 has a ResizePseudoConsole stub which returns S_OK without notifying
conhost. The original installed alpha engine failed the new behavior probe.

The patch in dlls/kernelbase/console.c sends a resize message through the
private pseudoconsole signal pipe, handled in programs/conhost/conhost.c.
The active screen buffer, visible window and cursor bounds are updated and
the window-size event is emitted. These components must be deployed together.

The out-of-tree alpha Wine build was rebuilt for kernelbase x64 and x86 and
conhost x64. No installed Wine binaries or the default prefix were changed.

The PE probe tests dimensions 80x25 -> 120x50 -> 100x40; negative
sizes return E_INVALIDARG. Microsoft Windows Terminal winconpty permits
zero-size packets at the API write layer, so the host accepts them and
the guest skips applying them. A child sees 100x40 via CONOUT$.
Both x86 and x64 pass on the locally rebuilt Wine.

LIMITATION: resizing currently uses an asynchronous private signal. The
test allows 250 ms for processing; the Windows winconpty reference also returns after its signal-pipe write,
not after an acknowledgement. Verify the full behavior in genuine Windows
before promoting from alpha, and investigate allocation failure handling.

## Event dispatcher correction

The console event-loop now refreshes wait handles on every iteration and
recognizes a terminated TTY input thread separately from the ConPTY resize
signal, including consoles without a signal handle. This avoids interpreting
input-thread completion as a resize packet and observes newly created TTY
threads. The separately compiled Wine x86 and x64 ConPTY probes still pass.

## Parallel lifecycle regression (October 26, 2026)

Run the separate rebuilt Wine engine with:

    ./ailinux/console/run-lifecycle-tests.sh

The test executes normal resize, 256-message resize stress, and close-without-
child in four parallel processes for both i686 and x86_64. It enforces
30-second individual timeouts, checks all exit codes and success markers,
and records the ConPTY close duration. Override AILINUX_CONPTY_PARALLEL
with a number from 1 to 12. Tests use the disposable gui-app-smoke-alpha
prefix by default; the system Wine, installed alpha engine and default Wine
prefix are not modified. Do not interpret an isolated stress pass as proof
of correct behavior on a genuine Windows 11 reference system.

## ConPTY bidirectional I/O

The conpty-io-probe.c starts an attached child, sends a line to the
ConPTY input pipe, and checks for the child response in the output pipe.
The terminal Enter input uses CR. Earlier LF-only tests blocked or failed.
PE32 and PE32+ pass on the isolated rebuilt Wine 11.20 engine, including
four simultaneous I/O tests per architecture. Real Windows VM testing
remains open until the QEMU guest agent is available.

## Console input mode roundtrip

The bidirectional I/O probe now checks GetConsoleMode and SetConsoleMode
on CONIN$ by toggling ENABLE_ECHO_INPUT, verifying the new flags, and
restoring the original mode before reading the input line. Both PE32 and
PE32+ passed under the isolated Wine test build. Comparison on genuine
Windows remains outstanding.

## Console control and VT probes (October 26, 2026)

The new conpty-control-probe.c compiles as x86 and x64 native PE and
checks VT output escape sequences plus exploratory Ctrl+C and Ctrl+Break
signal routing. VT output passed on the isolated Wine build in both
architectures.

Signal delivery is NOT yet compliant with our probe: Ctrl+C child exits
zero but the expected child handler response is missing from ConPTY output.
Ctrl+Break exits with status 1. The two signal cases are preserved as
opt-in diagnostics; they are not treated as passing or enabled in default CI.
To reproduce and capture failures:

    AILINUX_CONPTY_DIAG_SIGNALS=1 ./ailinux/console/run-control-tests.sh

This returns nonzero if unresolved. The normal command validates VT only.
No genuine Windows measurements have been collected yet; compare against
Windows before attributing all differences to Wine.


## October 26 Ctrl+C / Ctrl+Break follow-up

Both x86 and x64 VT probes remain passing. Explicitly enabled signal
diagnostics still fail on both architectures: Ctrl+C exits without the
expected handler response, while Ctrl+Break returns a nonzero child exit.
A controlled CREATE_NEW_PROCESS_GROUP experiment did not resolve either
failure and was reverted. The signal probes remain opt-in and return a
nonzero status, rather than claiming Windows conformance. Additional
child-exit and signal-timeout diagnostics are recorded for investigation.
An actual Windows reference result remains a prerequisite for changing
the Wine signal-routing implementation.

## Signal routing debug (October 27, 2026)

The opt-in x86 and x64 Ctrl+C/Ctrl+Break cases still fail after a
parent-child output acknowledgement was added. The child now waits to read
an explicit "ack" terminal line after emitting its handler marker and
the parent drains the output pipe before acknowledging. This rules out a
simple child-exit/output-flush race as the sole reason for the failure.

The Wine WINEDEBUG=+console,+process,+seh trace shows:
- Ctrl+C: the child registers a handler and outputs READY, but the parent
  sees child exit 0 without the control handler marker.
- Ctrl+Break: GenerateConsoleCtrlEvent(1,0) is invoked, child exits
  nonzero and no handler marker is produced.
- VT (the positive control) still passes on both architectures.

Wine server/console.c maps CTRL_C_EVENT to SIGINT and CTRL_BREAK_EVENT
to SIGQUIT. The ntdll signal_x86_64.c SIGQUIT handler aborts the thread
and is also used internally for forced thread termination, so modifying
that handler globally would risk unrelated Wine features. No such
unverified modification was made. The genuine Windows comparison remains
blocked on the guest agent.

## Direct console control isolation (October 27, 2026)

console-signal-isolation.c is a *separate* control-event probe intended for
execution within an attached genuine Windows console. It does not create a
ConPTY. It checks whether a Ctrl+C or Ctrl+Break event reaches a registered
Windows console handler. When invoked directly via headless Wine, the process
has no allocated console: GenerateConsoleCtrlEvent fails with Win32 error 12
and AllocConsole fails with error 5. Those outcomes are environment setup
failures, NOT evidence for or against correct control-signal routing.

The ConPTY control probe now requires a distinct exit sentinel (77) after
the child receives and acknowledges its handler event, so an unexpected
exit code zero cannot impersonate a complete Ctrl+C success. Both x86 and
x64 ConPTY Ctrl+C/Ctrl+Break cases remain explicitly unresolved. The VT
positive-control cases continue to pass.

Next: run these probes inside the authenticated Windows reference guest,
or under a validated standalone attached Wine console; compare the two
before changing Wine's SIGINT/SIGQUIT dispatch rules.
