/*
 * Dummy function definitions
 */

#include "windef.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(relay)

LONG WINAPI stub_GDI_379(HDC16 hdc) { FIXME("STARTPAGE: stub\n"); return 1; }
LONG WINAPI stub_GDI_381(HDC16 hdc, SEGPTR proc) { FIXME("SETABORTPROC: stub\n"); return 1; }
LONG WINAPI stub_GDI_382(void) { FIXME("ABORTPROC: stub\n"); return 1; }
LONG WINAPI stub_USER_489(void) { FIXME("stub\n"); return 0; }
LONG WINAPI stub_USER_490(void) { FIXME("stub\n"); return 0; }
LONG WINAPI stub_USER_492(void) { FIXME("stub\n"); return 0; }
LONG WINAPI stub_USER_496(void) { FIXME("stub\n"); return 0; }
LONG WINAPI KERNEL_nop(void) { return 0; }
