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
