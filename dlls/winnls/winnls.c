#include "windef.h"
#include "wine/winuser16.h"

BOOL WINAPI WINNLSEnableIME16(HWND16 hWnd, BOOL fEnable)
{
    /* fake return of previous status. is this what this function should do ? */
    return !fEnable;
}

BOOL WINAPI WINNLSGetEnableStatus16(HWND16 hWnd)
{
    return FALSE;
}
