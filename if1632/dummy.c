/*
 * Dummy function definitions
 */

#include "windef.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(relay)

long WINAPI stub_GDI_379(HDC16 hdc) { FIXME("STARTPAGE: stub\n"); return 1; }
long WINAPI stub_GDI_381(HDC16 hdc, SEGPTR proc) { FIXME("SETABORTPROC: stub\n"); return 1; }
long WINAPI stub_GDI_382(void) { FIXME("ABORTPROC: stub\n"); return 1; }
long WINAPI stub_GDI_530(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_531(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_532(LPINT16 buffer, INT16 xx) { 
      FIXME("(%p, %hd): stub\n",buffer,xx); 
      return 0; 
}
long WINAPI stub_GDI_536(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_538(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_540(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_543(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_555(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_560(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_561(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_564(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_565(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_566(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_571(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_572(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_573(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_556(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_558(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_GDI_569(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_489(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_490(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_492(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_496(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_902(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_905(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_906(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_907(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_909(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_910(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_911(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_912(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_913(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_914(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_915(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_916(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_918(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_919(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_920(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_922(void) { FIXME("stub\n"); return 0; }
long WINAPI stub_USER_923(void) { FIXME("stub\n"); return 0; }
long WINAPI KERNEL_nop(void) { return 0; }
