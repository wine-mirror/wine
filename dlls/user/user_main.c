/*
 * USER initialization code
 */

#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "wine/winbase16.h"

#include "dce.h"
#include "dialog.h"
#include "global.h"
#include "input.h"
#include "keyboard.h"
#include "menu.h"
#include "message.h"
#include "queue.h"
#include "spy.h"
#include "sysmetrics.h"
#include "user.h"
#include "win.h"
#include "debugtools.h"


/* load the graphics driver */
static BOOL load_driver(void)
{
    char buffer[MAX_PATH];
    HKEY hkey;
    DWORD type, count;

    if (RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\Wine", 0, NULL,
                         REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ))
    {
        MESSAGE("load_driver: Cannot create config registry key\n" );
        return FALSE;
    }
    count = sizeof(buffer);
    if (RegQueryValueExA( hkey, "GraphicsDriver", 0, &type, buffer, &count ))
        strcpy( buffer, "x11drv" );  /* default value */
    RegCloseKey( hkey );

    if (!LoadLibraryA( buffer ))
    {
        MESSAGE( "Could not load graphics driver '%s'\n", buffer );
        return FALSE;
    }
    return TRUE;
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
    USER_HeapSel = GlobalHandleToSel16( instance );

     /* Global atom table initialisation */
    if (!ATOM_Init( USER_HeapSel )) return FALSE;

    /* Load the graphics driver */
    if (!load_driver()) return FALSE;

    /* Initialize window handling (critical section) */
    WIN_Init();

    /* Initialize system colors and metrics*/
    SYSMETRICS_Init();
    SYSCOLOR_Init();

    /* Create the DCEs */
    DCE_Init();

    /* Initialize window procedures */
    if (!WINPROC_Init()) return FALSE;

    /* Initialize built-in window classes */
    if (!WIDGETS_Init()) return FALSE;

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
    USER_Driver->pUserRepaintDisable( FALSE );

    return TRUE;
}
