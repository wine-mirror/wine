/*
 * USER initialization code
 */

#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winbase16.h"

#include "cursoricon.h"
#include "dce.h"
#include "dialog.h"
#include "display.h"
#include "global.h"
#include "input.h"
#include "keyboard.h"
#include "menu.h"
#include "message.h"
#include "module.h"
#include "mouse.h"
#include "queue.h"
#include "spy.h"
#include "sysmetrics.h"
#include "user.h"
#include "win.h"


/***********************************************************************
 *           USER initialisation routine
 */
BOOL WINAPI USER_Init(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    NE_MODULE *pModule;
    int queueSize;

    if ( USER_HeapSel ) return TRUE;

    /* Create USER heap */
    pModule = NE_GetPtr( GetModuleHandle16( "USER" ) );
    if ( pModule )
    {
        USER_HeapSel = GlobalHandleToSel16( (NE_SEG_TABLE( pModule ) + 
                                           pModule->dgroup - 1)->hSeg );
    }
    else
    {
        USER_HeapSel = GlobalAlloc16( GMEM_FIXED, 0x10000 );
        LocalInit16( USER_HeapSel, 0, 0xffff );
    }

     /* Global atom table initialisation */
    if (!ATOM_Init( USER_HeapSel )) return FALSE;

    /* Initialize window handling (critical section) */
    WIN_Init();

    /* Initialize system colors and metrics*/
    SYSMETRICS_Init();
    SYSCOLOR_Init();

    /* Create the DCEs */
    DCE_Init();

    /* Initialize timers */
    if (!TIMER_Init()) return FALSE;
    
    /* Initialize window procedures */
    if (!WINPROC_Init()) return FALSE;

    /* Initialize cursor/icons */
    CURSORICON_Init();

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
    UserRepaintDisable16( FALSE );

    return TRUE;
}
