/*
 * Default dialog procedure
 *
 * Copyright 1993 Alexandre Julliard
 *
static char Copyright[] = "Copyright  Alexandre Julliard, 1993";
*/

#include "windows.h"
#include "dialog.h"
#include "win.h"
#include "stddebug.h"
/* #define DEBUG_DIALOG */
#include "debug.h"

/***********************************************************************
 *           DEFDLG_SetFocus
 *
 * Set the focus to a control of the dialog, selecting the text if
 * the control is an edit dialog.
 */
static void DEFDLG_SetFocus( HWND hwndDlg, HWND hwndCtrl )
{
    HWND hwndPrev = GetFocus();

    if (IsChild( hwndDlg, hwndPrev ))
    {
        if (SendMessage( hwndPrev, WM_GETDLGCODE, 0, 0 ) & DLGC_HASSETSEL)
            SendMessage( hwndPrev, EM_SETSEL, TRUE, MAKELONG( -1, 0 ) );
    }
    if (SendMessage( hwndCtrl, WM_GETDLGCODE, 0, 0 ) & DLGC_HASSETSEL)
        SendMessage( hwndCtrl, EM_SETSEL, FALSE, MAKELONG( 0, -1 ) );
    SetFocus( hwndCtrl );
}


/***********************************************************************
 *           DEFDLG_SaveFocus
 */
static BOOL DEFDLG_SaveFocus( HWND hwnd, DIALOGINFO *infoPtr )
{
    HWND hwndFocus = GetFocus();

    if (!hwndFocus || !IsChild( hwnd, hwndFocus )) return FALSE;
    if (!infoPtr->hwndFocus) return FALSE;  /* Already saved */
    infoPtr->hwndFocus = hwndFocus;
      /* Remove default button */
    return TRUE;
}


/***********************************************************************
 *           DEFDLG_RestoreFocus
 */
static BOOL DEFDLG_RestoreFocus( HWND hwnd, DIALOGINFO *infoPtr )
{
    if (!infoPtr->hwndFocus || IsIconic(hwnd)) return FALSE;
    if (!IsWindow( infoPtr->hwndFocus )) return FALSE;
    DEFDLG_SetFocus( hwnd, infoPtr->hwndFocus );
    infoPtr->hwndFocus = 0;
    return TRUE;
}


/***********************************************************************
 *           DEFDLG_FindDefButton
 *
 * Find the current default push-button.
 */
static HWND DEFDLG_FindDefButton( HWND hwndDlg )
{
    HWND hwndChild = GetWindow( hwndDlg, GW_CHILD );
    while (hwndChild)
    {
        if (SendMessage( hwndChild, WM_GETDLGCODE, 0, 0 ) & DLGC_DEFPUSHBUTTON)
            break;
        hwndChild = GetWindow( hwndChild, GW_HWNDNEXT );
    }
    return hwndChild;
}


/***********************************************************************
 *           DEFDLG_SetDefButton
 *
 * Set the new default button to be hwndNew.
 */
static BOOL DEFDLG_SetDefButton( HWND hwndDlg, DIALOGINFO *dlgInfo,
                                 HWND hwndNew )
{
    if (hwndNew &&
        !(SendMessage( hwndNew, WM_GETDLGCODE, 0, 0 ) & DLGC_UNDEFPUSHBUTTON))
        return FALSE;  /* Destination is not a push button */
    
    if (dlgInfo->msgResult)  /* There's already a default pushbutton */
    {
        HWND hwndOld = GetDlgItem( hwndDlg, dlgInfo->msgResult );
        if (SendMessage( hwndOld, WM_GETDLGCODE, 0, 0 ) & DLGC_DEFPUSHBUTTON)
            SendMessage( hwndOld, BM_SETSTYLE, BS_PUSHBUTTON, TRUE );
    }
    if (hwndNew)
    {
        SendMessage( hwndNew, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE );
        dlgInfo->msgResult = GetDlgCtrlID( hwndNew );
    }
    else dlgInfo->msgResult = 0;
    return TRUE;
}


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

    dprintf_dialog(stddeb, "DefDlgProc: %d %04x %d %08lx\n", 
		   hwnd, msg, wParam, lParam );

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

	case WM_SHOWWINDOW:
	    if (!wParam) DEFDLG_SaveFocus( hwnd, dlgInfo );
	    return DefWindowProc( hwnd, msg, wParam, lParam );

	case WM_ACTIVATE:
	    if (wParam) DEFDLG_RestoreFocus( hwnd, dlgInfo );
	    else DEFDLG_SaveFocus( hwnd, dlgInfo );
	    break;

	case WM_SETFOCUS:
	    DEFDLG_RestoreFocus( hwnd, dlgInfo );
	    break;

        case DM_SETDEFID:
            if (dlgInfo->fEnd) return TRUE;
            DEFDLG_SetDefButton( hwnd, dlgInfo,
                                 wParam ? GetDlgItem( hwnd, wParam ) : 0 );
            return TRUE;

        case DM_GETDEFID:
            if (dlgInfo->fEnd || !dlgInfo->msgResult) return 0;
            return MAKELONG( dlgInfo->msgResult, DC_HASDEFID );

	case WM_NEXTDLGCTL:
	    {
                HWND hwndDest = wParam;
                if (!lParam)
                {
                    HWND hwndPrev = GetFocus();
                    if (!hwndPrev)  /* Set focus to the first item */
                        hwndDest = DIALOG_GetFirstTabItem( hwnd );
                    else
                        hwndDest = GetNextDlgTabItem( hwnd, hwndPrev, wParam );
                }
                if (hwndDest) DEFDLG_SetFocus( hwnd, hwndDest );
                DEFDLG_SetDefButton( hwnd, dlgInfo, hwndDest );
            }
            break;

        case WM_CLOSE:
            EndDialog( hwnd, TRUE );
            DestroyWindow( hwnd );
            return 0;

	default:
	    return DefWindowProc( hwnd, msg, wParam, lParam );
    }
        
    return result;
}
