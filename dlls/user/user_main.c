/*
 * USER initialization code
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
#include "global.h"
#include "input.h"
#include "keyboard.h"
#include "message.h"
#include "queue.h"
#include "spy.h"
#include "sysmetrics.h"
#include "user.h"
#include "win.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(graphics);

USER_DRIVER USER_Driver;

WINE_LOOK TWEAK_WineLook = WIN31_LOOK;

WORD USER_HeapSel = 0;  /* USER heap selector */

static HMODULE graphics_driver;

#define GET_USER_FUNC(name) \
   if (!(USER_Driver.p##name = (void*)GetProcAddress( graphics_driver, #name ))) \
      FIXME("%s not found in graphics driver\n", #name)

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

    GET_USER_FUNC(UserRepaintDisable);
    GET_USER_FUNC(InitKeyboard);
    GET_USER_FUNC(VkKeyScan);
    GET_USER_FUNC(MapVirtualKey);
    GET_USER_FUNC(GetKeyNameText);
    GET_USER_FUNC(ToUnicode);
    GET_USER_FUNC(Beep);
    GET_USER_FUNC(GetDIState);
    GET_USER_FUNC(GetDIData);
    GET_USER_FUNC(InitMouse);
    GET_USER_FUNC(SetCursor);
    GET_USER_FUNC(MoveCursor);
    GET_USER_FUNC(GetScreenSaveActive);
    GET_USER_FUNC(SetScreenSaveActive);
    GET_USER_FUNC(GetScreenSaveTimeout);
    GET_USER_FUNC(SetScreenSaveTimeout);
    GET_USER_FUNC(LoadOEMResource);
    GET_USER_FUNC(IsSingleWindow);
    GET_USER_FUNC(AcquireClipboard);
    GET_USER_FUNC(ReleaseClipboard);
    GET_USER_FUNC(SetClipboardData);
    GET_USER_FUNC(GetClipboardData);
    GET_USER_FUNC(IsClipboardFormatAvailable);
    GET_USER_FUNC(RegisterClipboardFormat);
    GET_USER_FUNC(IsSelectionOwner);
    GET_USER_FUNC(ResetSelectionOwner);
    GET_USER_FUNC(CreateWindow);
    GET_USER_FUNC(DestroyWindow);
    GET_USER_FUNC(GetDC);
    GET_USER_FUNC(EnableWindow);
    GET_USER_FUNC(ScrollWindowEx);
    GET_USER_FUNC(SetFocus);
    GET_USER_FUNC(SetParent);
    GET_USER_FUNC(SetWindowPos);
    GET_USER_FUNC(SetWindowRgn);
    GET_USER_FUNC(SetWindowIcon);
    GET_USER_FUNC(SetWindowText);
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
    if ((ptr = (void**)GetProcAddress( module, "pfnSelectPalette" ))) *ptr = SelectPalette16;
    else ERR( "cannot find pfnSelectPalette in GDI32\n" );
    if ((ptr = (void**)GetProcAddress( module, "pfnRealizePalette" ))) *ptr = UserRealizePalette;
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
BOOL WINAPI USER_Init(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    HINSTANCE16 instance;
    int queueSize;

    if ( USER_HeapSel ) return TRUE;

    /* Create USER heap */
    if ((instance = LoadLibrary16( "USER.EXE" )) < 32) return FALSE;
    USER_HeapSel = instance | 7;

     /* Global atom table initialisation */
    if (!ATOM_Init( USER_HeapSel )) return FALSE;

    /* Load the graphics driver */
    tweak_init();
    if (!load_driver()) return FALSE;

    /* Initialize system colors and metrics*/
    SYSMETRICS_Init();
    SYSCOLOR_Init();

    /* Setup palette function pointers */
    palette_init();

    /* Initialize window procedures */
    if (!WINPROC_Init()) return FALSE;

    /* Initialize built-in window classes */
    controls_init();

    /* Initialize dialog manager */
    if (!DIALOG_Init()) return FALSE;

    /* Initialize menus */
    if (!MENU_Init()) return FALSE;

    /* Initialize message spying */
    if (!SPY_Init()) return FALSE;

    /* Create system message queue */
    queueSize = GetProfileIntA( "windows", "TypeAhead", 120 );
    if (!QUEUE_CreateSysMsgQueue( queueSize )) return FALSE;

    /* Set double click time */
    SetDoubleClickTime( GetProfileIntA("windows","DoubleClickSpeed",452) );

    /* Create message queue of initial thread */
    InitThreadInput16( 0, 0 );

    /* Create desktop window */
    if (!WIN_CreateDesktopWindow()) return FALSE;

    /* Initialize keyboard driver */
    KEYBOARD_Enable( keybd_event, InputKeyStateTable );

    /* Initialize mouse driver */
    MOUSE_Enable( mouse_event );

    /* Start processing X events */
    USER_Driver.pUserRepaintDisable( FALSE );

    return TRUE;
}
