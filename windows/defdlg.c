/*
 * Default dialog procedure
 *
 * Copyright 1993, 1996 Alexandre Julliard
 *
 */

#include "windows.h"
#include "dialog.h"
#include "win.h"
#include "winproc.h"


/***********************************************************************
 *           DEFDLG_SetFocus
 *
 * Set the focus to a control of the dialog, selecting the text if
 * the control is an edit dialog.
 */
static void DEFDLG_SetFocus( HWND hwndDlg, HWND hwndCtrl )
{
    HWND32 hwndPrev = GetFocus32();

    if (IsChild( hwndDlg, hwndPrev ))
    {
        if (SendMessage16( hwndPrev, WM_GETDLGCODE, 0, 0 ) & DLGC_HASSETSEL)
            SendMessage16( hwndPrev, EM_SETSEL, TRUE, MAKELONG( -1, 0 ) );
    }
    if (SendMessage16( hwndCtrl, WM_GETDLGCODE, 0, 0 ) & DLGC_HASSETSEL)
        SendMessage16( hwndCtrl, EM_SETSEL, FALSE, MAKELONG( 0, -1 ) );
    SetFocus32( hwndCtrl );
}


/***********************************************************************
 *           DEFDLG_SaveFocus
 */
static BOOL DEFDLG_SaveFocus( HWND hwnd, DIALOGINFO *infoPtr )
{
    HWND32 hwndFocus = GetFocus32();

    if (!hwndFocus || !IsChild( hwnd, hwndFocus )) return FALSE;
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
        if (SendMessage16( hwndChild, WM_GETDLGCODE, 0, 0 ) & DLGC_DEFPUSHBUTTON)
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
        !(SendMessage16(hwndNew, WM_GETDLGCODE, 0, 0 ) & DLGC_UNDEFPUSHBUTTON))
        return FALSE;  /* Destination is not a push button */
    
    if (dlgInfo->msgResult)  /* There's already a default pushbutton */
    {
        HWND hwndOld = GetDlgItem( hwndDlg, dlgInfo->msgResult );
        if (SendMessage32A( hwndOld, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)
            SendMessage32A( hwndOld, BM_SETSTYLE32, BS_PUSHBUTTON, TRUE );
    }
    if (hwndNew)
    {
        SendMessage32A( hwndNew, BM_SETSTYLE32, BS_DEFPUSHBUTTON, TRUE );
        dlgInfo->msgResult = GetDlgCtrlID( hwndNew );
    }
    else dlgInfo->msgResult = 0;
    return TRUE;
}


/***********************************************************************
 *           DEFDLG_Proc
 *
 * Implementation of DefDlgProc(). Only handle messages that need special
 * handling for dialogs.
 */
static LRESULT DEFDLG_Proc( HWND32 hwnd, UINT32 msg, WPARAM32 wParam,
                            LPARAM lParam, DIALOGINFO *dlgInfo )
{
    HWND hwndDefId;

    switch(msg)
    {
	case WM_INITDIALOG:
	    return 0;

        case WM_ERASEBKGND:
	    FillWindow( hwnd, hwnd, (HDC)wParam, (HBRUSH16)CTLCOLOR_DLG );
	    return 1;

	case WM_NCDESTROY:

	      /* Free dialog heap (if created) */
	    if (dlgInfo->hDialogHeap)
	    {
		GlobalUnlock16(dlgInfo->hDialogHeap);
		GlobalFree16(dlgInfo->hDialogHeap);
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

            /* Delete window procedure */
            WINPROC_FreeProc( dlgInfo->dlgProc );
            dlgInfo->dlgProc = (HWINDOWPROC)0;

	      /* Window clean-up */
	    return DefWindowProc32A( hwnd, msg, wParam, lParam );

	case WM_SHOWWINDOW:
	    if (!wParam) DEFDLG_SaveFocus( hwnd, dlgInfo );
	    return DefWindowProc32A( hwnd, msg, wParam, lParam );

	case WM_ACTIVATE:
	    if (wParam) DEFDLG_RestoreFocus( hwnd, dlgInfo );
	    else DEFDLG_SaveFocus( hwnd, dlgInfo );
	    return 0;

	case WM_SETFOCUS:
	    DEFDLG_RestoreFocus( hwnd, dlgInfo );
	    return 0;

        case DM_SETDEFID:
            if (dlgInfo->fEnd) return 1;
            DEFDLG_SetDefButton( hwnd, dlgInfo,
                                 wParam ? GetDlgItem( hwnd, wParam ) : 0 );
            return 1;

        case DM_GETDEFID:
            if (dlgInfo->fEnd) return 0;
	    if (dlgInfo->msgResult)
	      return MAKELONG( dlgInfo->msgResult, DC_HASDEFID );
	    hwndDefId = DEFDLG_FindDefButton( hwnd );
	    if (hwndDefId)
	      return MAKELONG( GetDlgCtrlID( hwndDefId ), DC_HASDEFID);
	    return 0;

	case WM_NEXTDLGCTL:
	    {
                HWND hwndDest = (HWND)wParam;
                if (!lParam)
                    hwndDest = GetNextDlgTabItem32(hwnd, GetFocus32(), wParam);
                if (hwndDest) DEFDLG_SetFocus( hwnd, hwndDest );
                DEFDLG_SetDefButton( hwnd, dlgInfo, hwndDest );
            }
            return 0;

        case WM_CLOSE:
            EndDialog( hwnd, TRUE );
            DestroyWindow( hwnd );
            return 0;
    }
    return 0;
}


/***********************************************************************
 *           DefDlgProc16   (USER.308)
 */
LRESULT DefDlgProc16( HWND16 hwnd, UINT16 msg, WPARAM16 wParam, LPARAM lParam )
{
    DIALOGINFO * dlgInfo;
    BOOL16 result = FALSE;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    
    if (!wndPtr) return 0;
    dlgInfo = (DIALOGINFO *)&wndPtr->wExtra;

    dlgInfo->msgResult = 0;
    if (dlgInfo->dlgProc)
    {
	  /* Call dialog procedure */
	result = (BOOL16)CallWindowProc16( (WNDPROC16)dlgInfo->dlgProc,
                                           hwnd, msg, wParam, lParam );

        /* Check if window was destroyed by dialog procedure */
        if (result || !IsWindow( hwnd )) return result;
    }
    
    switch(msg)
    {
	case WM_INITDIALOG:
        case WM_ERASEBKGND:
	case WM_NCDESTROY:
	case WM_SHOWWINDOW:
	case WM_ACTIVATE:
	case WM_SETFOCUS:
        case DM_SETDEFID:
        case DM_GETDEFID:
	case WM_NEXTDLGCTL:
        case WM_CLOSE:
            return DEFDLG_Proc( (HWND32)hwnd, msg, (WPARAM32)wParam,
                                lParam, dlgInfo );

	default:
	    return DefWindowProc16( hwnd, msg, wParam, lParam );
    }
}


/***********************************************************************
 *           DefDlgProc32A   (USER32.119)
 */
LRESULT DefDlgProc32A( HWND32 hwnd, UINT32 msg, WPARAM32 wParam, LPARAM lParam)
{
    DIALOGINFO * dlgInfo;
    BOOL16 result = FALSE;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    
    if (!wndPtr) return 0;
    dlgInfo = (DIALOGINFO *)&wndPtr->wExtra;

    dlgInfo->msgResult = 0;
    if (dlgInfo->dlgProc)
    {
	  /* Call dialog procedure */
	result = (BOOL16)CallWindowProc32A( (WNDPROC32)dlgInfo->dlgProc,
                                            hwnd, msg, wParam, lParam );

        /* Check if window was destroyed by dialog procedure */
        if (result || !IsWindow( hwnd )) return result;
    }
    
    switch(msg)
    {
	case WM_INITDIALOG:
        case WM_ERASEBKGND:
	case WM_NCDESTROY:
	case WM_SHOWWINDOW:
	case WM_ACTIVATE:
	case WM_SETFOCUS:
        case DM_SETDEFID:
        case DM_GETDEFID:
	case WM_NEXTDLGCTL:
        case WM_CLOSE:
            return DEFDLG_Proc( hwnd, msg, wParam, lParam, dlgInfo );

	default:
	    return DefWindowProc32A( hwnd, msg, wParam, lParam );
    }
}


/***********************************************************************
 *           DefDlgProc32W   (USER32.120)
 */
LRESULT DefDlgProc32W( HWND32 hwnd, UINT32 msg, WPARAM32 wParam, LPARAM lParam)
{
    DIALOGINFO * dlgInfo;
    BOOL16 result = FALSE;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    
    if (!wndPtr) return 0;
    dlgInfo = (DIALOGINFO *)&wndPtr->wExtra;

    dlgInfo->msgResult = 0;
    if (dlgInfo->dlgProc)
    {
	  /* Call dialog procedure */
	result = (BOOL16)CallWindowProc32W( (WNDPROC32)dlgInfo->dlgProc,
                                            hwnd, msg, wParam, lParam );

        /* Check if window was destroyed by dialog procedure */
        if (result || !IsWindow( hwnd )) return result;
    }
    
    switch(msg)
    {
	case WM_INITDIALOG:
        case WM_ERASEBKGND:
	case WM_NCDESTROY:
	case WM_SHOWWINDOW:
	case WM_ACTIVATE:
	case WM_SETFOCUS:
        case DM_SETDEFID:
        case DM_GETDEFID:
	case WM_NEXTDLGCTL:
        case WM_CLOSE:
            return DEFDLG_Proc( hwnd, msg, wParam, lParam, dlgInfo );

	default:
	    return DefWindowProc32W( hwnd, msg, wParam, lParam );
    }
}
