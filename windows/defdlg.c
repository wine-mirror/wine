/*
 * Default dialog procedure
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "windows.h"
#include "dialog.h"
#include "win.h"


/***********************************************************************
 *           DefDlgProc   (USER.308)
 */
LONG DefDlgProc( HWND hwnd, WORD msg, WORD wParam, LONG lParam )
{
    DIALOGINFO * dlgInfo;
    BOOL result = FALSE;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    
    if (!wndPtr) return 0;
    dlgInfo = (DIALOGINFO *)&wndPtr->wExtra;

#ifdef DEBUG_DIALOG
    printf( "DefDlgProc: %d %04x %d %08x\n", hwnd, msg, wParam, lParam );
#endif

    dlgInfo->msgResult = 0;
    if (dlgInfo->dlgProc)
    {
	  /* Call dialog procedure */
	result = (BOOL)CallWindowProc( dlgInfo->dlgProc, hwnd, 
				       msg, wParam, lParam );

	  /* Check if window destroyed by dialog procedure */
	wndPtr = WIN_FindWndPtr( hwnd );
	if (!wndPtr) return result;
    }
    
    if (!result) switch(msg)
    {
	case WM_INITDIALOG:
	    break;

        case WM_ERASEBKGND:
	    FillWindow( hwnd, hwnd, (HDC)wParam, (HBRUSH)CTLCOLOR_DLG );
	    return TRUE;

	case WM_NCDESTROY:

	      /* Free dialog heap (if created) */
	    if (dlgInfo->hDialogHeap)
	    {
		GlobalUnlock(dlgInfo->hDialogHeap);
		GlobalFree(dlgInfo->hDialogHeap);
		dlgInfo->hDialogHeap = 0;
	    }

	      /* Delete font */
	    if (dlgInfo->hUserFont)
	    {
		DeleteObject( dlgInfo->hUserFont );
		dlgInfo->hUserFont = 0;
	    }

	      /* Delete menu */
	    if (dlgInfo->hMenu)
	    {		
		DestroyMenu( dlgInfo->hMenu );
		dlgInfo->hMenu = 0;
	    }

	      /* Window clean-up */
	    DefWindowProc( hwnd, msg, wParam, lParam );
	    break;
	    
	default:
	    return DefWindowProc( hwnd, msg, wParam, lParam );
    }
        
    return result;
}
