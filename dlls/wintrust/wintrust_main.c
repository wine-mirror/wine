#include "config.h"

#include "windef.h"
#include "winerror.h"
#include "guiddef.h"

#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win32);

/***********************************************************************
 *		WinVerifyTrust (WINTRUST.@)
 */
LONG WINAPI WinVerifyTrust( HWND hwnd, GUID *ActionID,  LPVOID ActionData )
{
    FIXME("(hwnd 0x%04x ActionId %p ActionData %p): stub (nothing will be verified)\n",
            hwnd, ActionID,  ActionData);
    return ERROR_SUCCESS;
}
