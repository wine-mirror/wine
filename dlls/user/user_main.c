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

#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"

#include "controls.h"
#include "cursoricon.h"
#include "global.h"
#include "message.h"
#include "user.h"
#include "win.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(graphics);

USER_DRIVER USER_Driver;

WINE_LOOK TWEAK_WineLook = WIN31_LOOK;

WORD USER_HeapSel = 0;  /* USER heap selector */

extern HPALETTE (WINAPI *pfnGDISelectPalette)(HDC hdc, HPALETTE hpal, WORD bkgnd );
extern UINT (WINAPI *pfnGDIRealizePalette)(HDC hdc);

static HMODULE graphics_driver;
static DWORD exiting_thread_id;

extern void WDML_NotifyThreadDetach(void);

#define GET_USER_FUNC(name) USER_Driver.p##name = (void*)GetProcAddress( graphics_driver, #name )

/* load the graphics driver */
static BOOL load_driver(void)
{
    char buffer[MAX_PATH];
    HKEY hkey;
    DWORD type, count;

    strcpy( buffer, "x11drv" );  /* default value */
    if (!RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\Wine", &hkey ))
    {
        count = sizeof(buffer);
        RegQueryValueExA( hkey, "GraphicsDriver", 0, &type, buffer, &count );
        RegCloseKey( hkey );
    }

    if (!(graphics_driver = LoadLibraryA( buffer )))
    {
        MESSAGE( "Could not load graphics driver '%s'\n", buffer );
        return FALSE;
    }

    GET_USER_FUNC(InitKeyboard);
    GET_USER_FUNC(VkKeyScan);
    GET_USER_FUNC(MapVirtualKey);
    GET_USER_FUNC(GetKeyNameText);
    GET_USER_FUNC(ToUnicode);
    GET_USER_FUNC(Beep);
    GET_USER_FUNC(InitMouse);
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
    GET_USER_FUNC(GetDC);
    GET_USER_FUNC(ForceWindowRaise);
    GET_USER_FUNC(MsgWaitForMultipleObjectsEx);
    GET_USER_FUNC(ReleaseDC);
    GET_USER_FUNC(ScrollDC);
    GET_USER_FUNC(ScrollWindowEx);
    GET_USER_FUNC(SetFocus);
    GET_USER_FUNC(SetParent);
    GET_USER_FUNC(SetWindowPos);
    GET_USER_FUNC(SetWindowRgn);
    GET_USER_FUNC(SetWindowIcon);
    GET_USER_FUNC(SetWindowStyle);
    GET_USER_FUNC(SetWindowText);
    GET_USER_FUNC(ShowWindow);
    GET_USER_FUNC(SysCommandSizeMove);

    return TRUE;
}


/***********************************************************************
 *           controls_init
 *
 * Register the classes for the builtin controls
 */
static void controls_init(void)
{
    extern const struct builtin_class_descr BUTTON_builtin_class;
    extern const struct builtin_class_descr COMBO_builtin_class;
    extern const struct builtin_class_descr COMBOLBOX_builtin_class;
    extern const struct builtin_class_descr DIALOG_builtin_class;
    extern const struct builtin_class_descr DESKTOP_builtin_class;
    extern const struct builtin_class_descr EDIT_builtin_class;
    extern const struct builtin_class_descr ICONTITLE_builtin_class;
    extern const struct builtin_class_descr LISTBOX_builtin_class;
    extern const struct builtin_class_descr MDICLIENT_builtin_class;
    extern const struct builtin_class_descr MENU_builtin_class;
    extern const struct builtin_class_descr SCROLL_builtin_class;
    extern const struct builtin_class_descr STATIC_builtin_class;

    CLASS_RegisterBuiltinClass( &BUTTON_builtin_class );
    CLASS_RegisterBuiltinClass( &COMBO_builtin_class );
    CLASS_RegisterBuiltinClass( &COMBOLBOX_builtin_class );
    CLASS_RegisterBuiltinClass( &DIALOG_builtin_class );
    CLASS_RegisterBuiltinClass( &DESKTOP_builtin_class );
    CLASS_RegisterBuiltinClass( &EDIT_builtin_class );
    CLASS_RegisterBuiltinClass( &ICONTITLE_builtin_class );
    CLASS_RegisterBuiltinClass( &LISTBOX_builtin_class );
    CLASS_RegisterBuiltinClass( &MDICLIENT_builtin_class );
    CLASS_RegisterBuiltinClass( &MENU_builtin_class );
    CLASS_RegisterBuiltinClass( &SCROLL_builtin_class );
    CLASS_RegisterBuiltinClass( &STATIC_builtin_class );
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
        pfnGDISelectPalette = InterlockedExchangePointer( ptr, SelectPalette );
    else ERR( "cannot find pfnSelectPalette in GDI32\n" );
    if ((ptr = (void**)GetProcAddress( module, "pfnRealizePalette" )))
        pfnGDIRealizePalette = InterlockedExchangePointer( ptr, UserRealizePalette );
    else ERR( "cannot find pfnRealizePalette in GDI32\n" );
}


/***********************************************************************
 *           tweak_init
 */
static void tweak_init(void)
{
    static const char *OS = "Win3.1";
    char buffer[80];
    HKEY hkey;
    DWORD type, count = sizeof(buffer);

    if (RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\Tweak.Layout", &hkey ))
        return;
    if (RegQueryValueExA( hkey, "WineLook", 0, &type, buffer, &count ))
        strcpy( buffer, "Win31" );  /* default value */
    RegCloseKey( hkey );

    /* WIN31_LOOK is default */
    if (!strncasecmp( buffer, "Win95", 5 ))
    {
        TWEAK_WineLook = WIN95_LOOK;
        OS = "Win95";
    }
    else if (!strncasecmp( buffer, "Win98", 5 ))
    {
        TWEAK_WineLook = WIN98_LOOK;
        OS = "Win98";
    }
    TRACE("Using %s look and feel.\n", OS);
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

    /* Load the graphics driver */
    tweak_init();
    if (!load_driver()) return FALSE;

    /* Initialize system colors and metrics */
    SYSMETRICS_Init();
    SYSCOLOR_Init();

    /* Setup palette function pointers */
    palette_init();

    /* Initialize built-in window classes */
    controls_init();

    /* Initialize menus */
    if (!MENU_Init()) return FALSE;

    /* Initialize message spying */
    if (!SPY_Init()) return FALSE;

    /* Create message queue of initial thread */
    InitThreadInput16( 0, 0 );

    /* Create desktop window */
    if (!WIN_CreateDesktopWindow()) return FALSE;

    /* Initialize keyboard driver */
    if (USER_Driver.pInitKeyboard) USER_Driver.pInitKeyboard( InputKeyStateTable );

    /* Initialize mouse driver */
    if (USER_Driver.pInitMouse) USER_Driver.pInitMouse( InputKeyStateTable );

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

    TIMER_RemoveThreadTimers();
    WIN_DestroyThreadWindows( GetDesktopWindow() );
    QUEUE_DeleteMsgQueue();

    exiting_thread_id = 0;
}


/***********************************************************************
 *           UserClientDllInitialize  (USER32.@)
 *
 * USER dll initialisation routine
 */
BOOL WINAPI UserClientDllInitialize( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    BOOL ret = TRUE;
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        ret = process_attach();
        break;
    case DLL_THREAD_DETACH:
        thread_detach();
        break;
    }
    return ret;
}
