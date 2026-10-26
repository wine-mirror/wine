# AILinux Windows Console Compatibility: ConPTY resize

Wine 11.20 has a ResizePseudoConsole stub which returns S_OK without notifying
conhost. The original installed alpha engine failed the new behavior probe.

The patch in dlls/kernelbase/console.c sends a resize message through the
private pseudoconsole signal pipe, handled in programs/conhost/conhost.c.
The active screen buffer, visible window and cursor bounds are updated and
the window-size event is emitted. These components must be deployed together.

The out-of-tree alpha Wine build was rebuilt for kernelbase x64 and x86 and
conhost x64. No installed Wine binaries or the default prefix were changed.

The PE probe tests dimensions 80x25 -> 100x40; invalid dimensions return
E_INVALIDARG and a child process sees the new console size via CONOUT$.
Both x86 and x64 pass on the locally rebuilt Wine.

LIMITATION: resizing currently uses an asynchronous private signal. The
test allows 250 ms for processing; this is not Windows-level synchronous
acknowledgement or failure propagation. Before release, design a signal
acknowledgement, check failed allocation and compare with genuine Windows.
