#include "windef.h"
#include "wine/winuser16.h"

/***********************************************************************
 *		WINNLSEnableIME (WINNLS.16)
 */
BOOL WINAPI WINNLSEnableIME16(HWND16 hWnd, BOOL fEnable)
{
    /* fake return of previous status. is this what this function should do ? */
    return !fEnable;
}

/***********************************************************************
 *		WINNLSGetEnableStatus (WINNLS.18)
 */
BOOL WINAPI WINNLSGetEnableStatus16(HWND16 hWnd)
{
    return FALSE;
}
