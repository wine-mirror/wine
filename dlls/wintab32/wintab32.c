/*
 * WinTab32 library
 *
 * Copyright 2003 CodeWeavers, Aric Stewart
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "ntuser.h"
#include "winerror.h"
#define NOFIX32
#include "wintab.h"
#include "wintab_internal.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintab32);

static HINSTANCE wintab_instance;
static HWND hwndTablet = NULL;
static CRITICAL_SECTION_DEBUG csTablet_debug =
{
    0, 0, &csTablet,
    { &csTablet_debug.ProcessLocksList, &csTablet_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": csTablet") }
};
CRITICAL_SECTION csTablet = { &csTablet_debug, -1, 0, 0, 0, 0 };

static LRESULT WINAPI TABLET_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                          LPARAM lParam);

static VOID TABLET_Register(void)
{
    WNDCLASSW wndClass;
    ZeroMemory(&wndClass, sizeof(WNDCLASSW));
    wndClass.style = CS_GLOBALCLASS;
    wndClass.lpfnWndProc = TABLET_WindowProc;
    wndClass.hInstance = wintab_instance;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hCursor = NULL;
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW +1);
    wndClass.lpszClassName = L"WineTabletClass";
    RegisterClassW(&wndClass);
}

static VOID TABLET_Unregister(void)
{
    UnregisterClassW(L"WineTabletClass", NULL);
}

static BOOL WINAPI create_internal_window(INIT_ONCE *once, void *param, void **context)
{
    TABLET_Register();
    hwndTablet = CreateWindowW(L"WineTabletClass", L"Tablet", 0,
                                0, 0, 0, 0, HWND_MESSAGE, 0, wintab_instance, 0);

    if (!hwndTablet)
        ERR("error creating internal window: %lu\n", GetLastError());

    return TRUE;
}

HWND TABLET_GetInternalWindow(void)
{
    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;
    InitOnceExecuteOnce(&init_once, create_internal_window, NULL, NULL);

    return hwndTablet;
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    TRACE("%p, %lx, %p\n",hInstDLL,fdwReason,lpReserved);
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            TRACE("Initialization\n");
            wintab_instance = hInstDLL;
            DisableThreadLibraryCalls(hInstDLL);
            break;
        case DLL_PROCESS_DETACH:
            if (lpReserved) break;
            TRACE("Detaching\n");
            if (hwndTablet) DestroyWindow(hwndTablet);
            TABLET_Unregister();
            DeleteCriticalSection(&csTablet);
            break;
    }
    return TRUE;
}


/*
 * The window proc for the default TABLET window
 */
static LRESULT WINAPI TABLET_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                          LPARAM lParam)
{
    TRACE("Incoming Message 0x%x  (0x%08x, 0x%08x)\n", uMsg, (UINT)wParam,
           (UINT)lParam);

    switch(uMsg)
    {
        case WT_PACKET:
            {
                WTPACKET packet;
                LPOPENCONTEXT handler;
                if (NtUserMessageCall(hwnd, NtUserWintabPacket, 0, 0, &packet, NtUserWintabDriverCall, FALSE))
                {
                    handler = AddPacketToContextQueue(&packet,(HWND)lParam);
                    if (handler && handler->context.lcOptions & CXO_MESSAGES)
                       TABLET_PostTabletMessage(handler, _WT_PACKET(handler->context.lcMsgBase),
                                   (WPARAM)packet.pkSerialNumber,
                                   (LPARAM)handler->handle, FALSE);
                }
                return 0;
            }
        case WT_PROXIMITY:
            {
                WTPACKET packet;
                LPOPENCONTEXT handler;
                if (NtUserMessageCall(hwnd, NtUserWintabPacket, 0, 0, &packet, NtUserWintabDriverCall, FALSE))
                {
                    handler = AddPacketToContextQueue(&packet,(HWND)wParam);
                    if (handler)
                        TABLET_PostTabletMessage(handler, WT_PROXIMITY,
                                                (WPARAM)handler->handle, lParam, TRUE);
                }
                return 0;
            }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}
