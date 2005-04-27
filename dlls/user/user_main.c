/*
 * USER initialization code
 *
 * Copyright 2000 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"

#include "controls.h"
#include "message.h"
#include "user_private.h"
#include "win.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(graphics);

USER_DRIVER USER_Driver;

WORD USER_HeapSel = 0;  /* USER heap selector */
HMODULE user32_module = 0;

static HPALETTE (WINAPI *pfnGDISelectPalette)( HDC hdc, HPALETTE hpal, WORD bkgnd );
static UINT (WINAPI *pfnGDIRealizePalette)( HDC hdc );
static HPALETTE hPrimaryPalette;

static HMODULE graphics_driver;
static DWORD exiting_thread_id;

extern void WDML_NotifyThreadDetach(void);

#define GET_USER_FUNC(name) USER_Driver.p##name = (void*)GetProcAddress( graphics_driver, #name )

/* load the graphics driver */
static BOOL load_driver(void)
{
    char buffer[MAX_PATH], *name, *next;
    HKEY hkey;

    strcpy( buffer, "x11drv,ttydrv" );  /* default value */
    if (!RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\Wine", &hkey ))
    {
        DWORD type, count = sizeof(buffer);
        RegQueryValueExA( hkey, "GraphicsDriver", 0, &type, buffer, &count );
        RegCloseKey( hkey );
    }

    name = buffer;
    while (name)
    {
        next = strchr( name, ',' );
        if (next) *next++ = 0;

        if ((graphics_driver = LoadLibraryA( name )) != 0) break;
        name = next;
    }
    if (!graphics_driver)
    {
        MESSAGE( "wine: Could not load graphics driver '%s'.\n", buffer );
        if (!strcasecmp( buffer, "x11drv" ))
            MESSAGE( "Make sure that your X server is running and that $DISPLAY is set correctly.\n" );
        ExitProcess(1);
    }

    GET_USER_FUNC(ActivateKeyboardLayout);
    GET_USER_FUNC(Beep);
    GET_USER_FUNC(GetAsyncKeyState);
    GET_USER_FUNC(GetKeyNameText);
    GET_USER_FUNC(GetKeyboardLayout);
    GET_USER_FUNC(GetKeyboardLayoutList);
    GET_USER_FUNC(GetKeyboardLayoutName);
    GET_USER_FUNC(LoadKeyboardLayout);
    GET_USER_FUNC(MapVirtualKeyEx);
    GET_USER_FUNC(SendInput);
    GET_USER_FUNC(ToUnicodeEx);
    GET_USER_FUNC(UnloadKeyboardLayout);
    GET_USER_FUNC(VkKeyScanEx);
    GET_USER_FUNC(SetCursor);
    GET_USER_FUNC(GetCursorPos);
    GET_USER_FUNC(SetCursorPos);
    GET_USER_FUNC(GetScreenSaveActive);
    GET_USER_FUNC(SetScreenSaveActive);
    GET_USER_FUNC(AcquireClipboard);
    GET_USER_FUNC(EmptyClipboard);
    GET_USER_FUNC(SetClipboardData);
    GET_USER_FUNC(GetClipboardData);
    GET_USER_FUNC(CountClipboardFormats);
    GET_USER_FUNC(EnumClipboardFormats);
    GET_USER_FUNC(IsClipboardFormatAvailable);
    GET_USER_FUNC(RegisterClipboardFormat);
    GET_USER_FUNC(GetClipboardFormatName);
    GET_USER_FUNC(EndClipboardUpdate);
    GET_USER_FUNC(ResetSelectionOwner);
    GET_USER_FUNC(ChangeDisplaySettingsExW);
    GET_USER_FUNC(EnumDisplaySettingsExW);
    GET_USER_FUNC(CreateWindow);
    GET_USER_FUNC(DestroyWindow);
    GET_USER_FUNC(GetDCEx);
    GET_USER_FUNC(MsgWaitForMultipleObjectsEx);
    GET_USER_FUNC(ReleaseDC);
    GET_USER_FUNC(ScrollDC);
    GET_USER_FUNC(SetFocus);
    GET_USER_FUNC(SetParent);
    GET_USER_FUNC(SetWindowPos);
    GET_USER_FUNC(SetWindowRgn);
    GET_USER_FUNC(SetWindowIcon);
    GET_USER_FUNC(SetWindowStyle);
    GET_USER_FUNC(SetWindowText);
    GET_USER_FUNC(ShowWindow);
    GET_USER_FUNC(SysCommandSizeMove);
    GET_USER_FUNC(WindowFromDC);
    GET_USER_FUNC(WindowMessage);

    return TRUE;
}


/***********************************************************************
 *		UserSelectPalette (Not a Windows API)
 */
static HPALETTE WINAPI UserSelectPalette( HDC hDC, HPALETTE hPal, BOOL bForceBackground )
{
    WORD wBkgPalette = 1;

    if (!bForceBackground && (hPal != GetStockObject(DEFAULT_PALETTE)))
    {
        HWND hwnd = WindowFromDC( hDC );
        if (hwnd)
        {
            HWND hForeground = GetForegroundWindow();
            /* set primary palette if it's related to current active */
            if (hForeground == hwnd || IsChild(hForeground,hwnd))
            {
                wBkgPalette = 0;
                hPrimaryPalette = hPal;
            }
        }
    }
    return pfnGDISelectPalette( hDC, hPal, wBkgPalette);
}


/***********************************************************************
 *		UserRealizePalette (USER32.@)
 */
UINT WINAPI UserRealizePalette( HDC hDC )
{
    UINT realized = pfnGDIRealizePalette( hDC );

    /* do not send anything if no colors were changed */
    if (realized && GetCurrentObject( hDC, OBJ_PAL ) == hPrimaryPalette)
    {
        /* send palette change notification */
        HWND hWnd = WindowFromDC( hDC );
        if (hWnd) SendMessageTimeoutW( HWND_BROADCAST, WM_PALETTECHANGED, (WPARAM)hWnd, 0,
                                       SMTO_ABORTIFHUNG, 2000, NULL );
    }
    return realized;
}


/***********************************************************************
 *           palette_init
 *
 * Patch the function pointers in GDI for SelectPalette and RealizePalette
 */
static void palette_init(void)
{
    void **ptr;
    HMODULE module = GetModuleHandleA( "gdi32" );
    if (!module)
    {
        ERR( "cannot get GDI32 handle\n" );
        return;
    }
    if ((ptr = (void**)GetProcAddress( module, "pfnSelectPalette" )))
        pfnGDISelectPalette = InterlockedExchangePointer( ptr, UserSelectPalette );
    else ERR( "cannot find pfnSelectPalette in GDI32\n" );
    if ((ptr = (void**)GetProcAddress( module, "pfnRealizePalette" )))
        pfnGDIRealizePalette = InterlockedExchangePointer( ptr, UserRealizePalette );
    else ERR( "cannot find pfnRealizePalette in GDI32\n" );
}


/***********************************************************************
 *           USER initialisation routine
 */
static BOOL process_attach(void)
{
    HINSTANCE16 instance;

    /* Create USER heap */
    if ((instance = LoadLibrary16( "USER.EXE" )) >= 32) USER_HeapSel = instance | 7;
    else
    {
        USER_HeapSel = GlobalAlloc16( GMEM_FIXED, 65536 );
        LocalInit16( USER_HeapSel, 32, 65534 );
    }

    /* some Win9x dlls expect keyboard to be loaded */
    if (GetVersion() & 0x80000000) LoadLibrary16( "keyboard.drv" );

    /* Load the graphics driver */
    if (!load_driver()) return FALSE;

    /* Initialize system colors and metrics */
    SYSPARAMS_Init();

    /* Setup palette function pointers */
    palette_init();

    /* Initialize built-in window classes */
    CLASS_RegisterBuiltinClasses();

    /* Initialize menus */
    if (!MENU_Init()) return FALSE;

    /* Initialize message spying */
    if (!SPY_Init()) return FALSE;

    /* Create desktop window */
    if (!WIN_CreateDesktopWindow()) return FALSE;

    return TRUE;
}


/**********************************************************************
 *           USER_IsExitingThread
 */
BOOL USER_IsExitingThread( DWORD tid )
{
    return (tid == exiting_thread_id);
}


/**********************************************************************
 *           thread_detach
 */
static void thread_detach(void)
{
    exiting_thread_id = GetCurrentThreadId();

    WDML_NotifyThreadDetach();

    WIN_DestroyThreadWindows( GetDesktopWindow() );
    CloseHandle( get_user_thread_info()->server_queue );

    exiting_thread_id = 0;
}


/**********************************************************************
 *           process_detach
 */
static void process_detach(void)
{
    memset(&USER_Driver, 0, sizeof(USER_Driver));
    FreeLibrary(graphics_driver);
}

/***********************************************************************
 *           UserClientDllInitialize  (USER32.@)
 *
 * USER dll initialisation routine (exported as UserClientDllInitialize for compatibility).
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    BOOL ret = TRUE;
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        user32_module = inst;
        ret = process_attach();
        break;
    case DLL_PROCESS_DETACH:
        process_detach();
        break;
    case DLL_THREAD_DETACH:
        thread_detach();
        break;
    }
    return ret;
}
