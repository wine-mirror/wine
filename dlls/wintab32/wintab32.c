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
#include "winuser.h"
#include "winerror.h"
#define NOFIX32
#include "wintab.h"
#include "wintab_internal.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintab32);

HWND hwndDefault = NULL;
static const WCHAR
  WC_TABLETCLASSNAME[] = {'W','i','n','e','T','a','b','l','e','t','C','l','a','s','s',0};
static CRITICAL_SECTION_DEBUG csTablet_debug =
{
    0, 0, &csTablet,
    { &csTablet_debug.ProcessLocksList, &csTablet_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": csTablet") }
};
CRITICAL_SECTION csTablet = { &csTablet_debug, -1, 0, 0, 0, 0 };

int  (CDECL *pLoadTabletInfo)(HWND hwnddefault) = NULL;
int  (CDECL *pGetCurrentPacket)(LPWTPACKET packet) = NULL;
int  (CDECL *pAttachEventQueueToTablet)(HWND hOwner) = NULL;
UINT (CDECL *pWTInfoW)(UINT wCategory, UINT nIndex, LPVOID lpOutput) = NULL;

static LRESULT WINAPI TABLET_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                          LPARAM lParam);

static VOID TABLET_Register(void)
{
    WNDCLASSW wndClass;
    ZeroMemory(&wndClass, sizeof(WNDCLASSW));
    wndClass.style = CS_GLOBALCLASS;
    wndClass.lpfnWndProc = TABLET_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hCursor = NULL;
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW +1);
    wndClass.lpszClassName = WC_TABLETCLASSNAME;
    RegisterClassW(&wndClass);
}

static VOID TABLET_Unregister(void)
{
    UnregisterClassW(WC_TABLETCLASSNAME, NULL);
}

static HMODULE load_graphics_driver(void)
{
    static const WCHAR display_device_guid_propW[] = {
        '_','_','w','i','n','e','_','d','i','s','p','l','a','y','_',
        'd','e','v','i','c','e','_','g','u','i','d',0 };
    static const WCHAR key_pathW[] = {
        'S','y','s','t','e','m','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'C','o','n','t','r','o','l','\\',
        'V','i','d','e','o','\\','{',0};
    static const WCHAR displayW[] = {'}','\\','0','0','0','0',0};
    static const WCHAR driverW[] = {'G','r','a','p','h','i','c','s','D','r','i','v','e','r',0};

    HMODULE ret = 0;
    HKEY hkey;
    DWORD size;
    WCHAR path[MAX_PATH];
    WCHAR key[ARRAY_SIZE(key_pathW) + ARRAY_SIZE(displayW) + 40];
    UINT guid_atom = HandleToULong( GetPropW( GetDesktopWindow(), display_device_guid_propW ));

    if (!guid_atom) return 0;
    memcpy( key, key_pathW, sizeof(key_pathW) );
    if (!GlobalGetAtomNameW( guid_atom, key + lstrlenW(key), 40 )) return 0;
    lstrcatW( key, displayW );
    if (RegOpenKeyW( HKEY_LOCAL_MACHINE, key, &hkey )) return 0;
    size = sizeof(path);
    if (!RegQueryValueExW( hkey, driverW, NULL, NULL, (BYTE *)path, &size )) ret = LoadLibraryW( path );
    RegCloseKey( hkey );
    TRACE( "%s %p\n", debugstr_w(path), ret );
    return ret;
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    static const WCHAR name[] = {'T','a','b','l','e','t',0};

    TRACE("%p, %x, %p\n",hInstDLL,fdwReason,lpReserved);
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            TRACE("Initialization\n");
            DisableThreadLibraryCalls(hInstDLL);
            TABLET_Register();
            hwndDefault = CreateWindowW(WC_TABLETCLASSNAME, name,
                                        WS_POPUPWINDOW,0,0,0,0,0,0,hInstDLL,0);
            if (hwndDefault)
            {
                HMODULE module = load_graphics_driver();
                pLoadTabletInfo = (void *)GetProcAddress(module, "LoadTabletInfo");
                pAttachEventQueueToTablet = (void *)GetProcAddress(module, "AttachEventQueueToTablet");
                pGetCurrentPacket = (void *)GetProcAddress(module, "GetCurrentPacket");
                pWTInfoW = (void *)GetProcAddress(module, "WTInfoW");
            }
            else
                return FALSE;
            break;
        case DLL_PROCESS_DETACH:
            if (lpReserved) break;
            TRACE("Detaching\n");
            if (hwndDefault) DestroyWindow(hwndDefault);
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
        case WM_NCCREATE:
            return TRUE;

        case WT_PACKET:
            {
                WTPACKET packet;
                LPOPENCONTEXT handler;
                if (pGetCurrentPacket)
                {
                    pGetCurrentPacket(&packet);
                    handler = AddPacketToContextQueue(&packet,(HWND)lParam);
                    if (handler && handler->context.lcOptions & CXO_MESSAGES)
                       TABLET_PostTabletMessage(handler, _WT_PACKET(handler->context.lcMsgBase),
                                   (WPARAM)packet.pkSerialNumber,
                                   (LPARAM)handler->handle, FALSE);
                }
                break;
            }
        case WT_PROXIMITY:
            {
                WTPACKET packet;
                LPOPENCONTEXT handler;
                if (pGetCurrentPacket)
                {
                    pGetCurrentPacket(&packet);
                    handler = AddPacketToContextQueue(&packet,(HWND)wParam);
                    if (handler)
                        TABLET_PostTabletMessage(handler, WT_PROXIMITY,
                                                (WPARAM)handler->handle, lParam, TRUE);
                }
                break;
            }
    }
    return 0;
}
