/*
 * SetupAPI device installer
 *
 */

#include "debugtools.h"
#include "windef.h"
#include "setupx16.h"
#include "heap.h"

DEFAULT_DEBUG_CHANNEL(setupapi);

/***********************************************************************
 *		DiGetClassDevs (SETUPX.304)
 * Return a list of installed system devices.
 * Uses HKLM\\ENUM to list devices.
 */
RETERR16 WINAPI DiGetClassDevs16(LPLPDEVICE_INFO16 lplpdi,
                                 LPCSTR lpszClassName, HWND16 hwndParent, INT16 iFlags)
{
    LPDEVICE_INFO16 lpdi;

    FIXME("(%p, '%s', %04x, %04x), semi-stub.\n",
          lplpdi, lpszClassName, hwndParent, iFlags);
    lpdi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DEVICE_INFO16));
    lpdi->cbSize = sizeof(DEVICE_INFO16);
    *lplpdi = lpdi;
    return OK;
}
