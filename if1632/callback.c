#ifndef WINELIB
/*
static char RCSId[] = "$Id: wine.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";
*/

#include <stdio.h>
#include <stdlib.h>
#include "windows.h"
#include "callback.h"
#include "stackframe.h"
#include "win.h"
#include "stddebug.h"
#include "debug.h"



/**********************************************************************
 *	     CallWindowProc    (USER.122)
 */
LONG CallWindowProc( WNDPROC func, HWND hwnd, WORD message,
		     WORD wParam, LONG lParam )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr) return 0;
    SpyMessage(hwnd, message, wParam, lParam);
    return CallWndProc( (FARPROC)func, wndPtr->hInstance, 
                        hwnd, message, wParam, lParam );
}


/**********************************************************************
 *	     Catch    (KERNEL.55)
 */
int Catch( LPCATCHBUF lpbuf )
{
    STACK16FRAME *pFrame = CURRENT_STACK16;

    dprintf_catch( stddeb, "Catch: buf=%p ss:sp=%04x:%04x\n",
                   lpbuf, IF1632_Saved16_ss, IF1632_Saved16_sp );

    /* Note: we don't save the current ss, as the catch buffer is */
    /* only 9 words long. Hopefully no one will have the silly    */
    /* idea to change the current stack before calling Throw()... */

    lpbuf[0] = IF1632_Saved16_sp;
    lpbuf[1] = LOWORD(IF1632_Saved32_esp);
    lpbuf[2] = HIWORD(IF1632_Saved32_esp);
    lpbuf[3] = pFrame->saved_ss;
    lpbuf[4] = pFrame->saved_sp;
    lpbuf[5] = pFrame->ds;
    lpbuf[6] = pFrame->bp;
    lpbuf[7] = pFrame->ip;
    lpbuf[8] = pFrame->cs;
    return 0;
}


/**********************************************************************
 *	     Throw    (KERNEL.56)
 */
int Throw( LPCATCHBUF lpbuf, int retval )
{
    STACK16FRAME *pFrame;

    dprintf_catch( stddeb, "Throw: buf=%p val=%04x ss:sp=%04x:%04x\n",
                   lpbuf, retval, IF1632_Saved16_ss, IF1632_Saved16_sp );

    IF1632_Saved16_sp  = lpbuf[0] - sizeof(WORD);
    IF1632_Saved32_esp = MAKELONG( lpbuf[1], lpbuf[2] );
    pFrame = CURRENT_STACK16;
    pFrame->saved_ss   = lpbuf[3];
    pFrame->saved_sp   = lpbuf[4];
    pFrame->ds         = lpbuf[5];
    pFrame->bp         = lpbuf[6];
    pFrame->ip         = lpbuf[7];
    pFrame->cs         = lpbuf[8];
    pFrame->es         = 0;
    return retval;
}


#endif /* !WINELIB */
