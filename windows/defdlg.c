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
static void DEFDLG_SetFocus( HWND32 hwndDlg, HWND32 hwndCtrl )
{
    HWND32 hwndPrev = GetFocus32();

    if (IsChild32( hwndDlg, hwndPrev ))
    {
        if (SendMessage16( hwndPrev, WM_GETDLGCODE, 0, 0 ) & DLGC_HASSETSEL)
            SendMessage16( hwndPrev, EM_SETSEL16, TRUE, MAKELONG( -1, 0 ) );
    }
    if (SendMessage16( hwndCtrl, WM_GETDLGCODE, 0, 0 ) & DLGC_HASSETSEL)
        SendMessage16( hwndCtrl, EM_SETSEL16, FALSE, MAKELONG( 0, -1 ) );
    SetFocus32( hwndCtrl );
}


/***********************************************************************
 *           DEFDLG_SaveFocus
 */
static BOOL32 DEFDLG_SaveFocus( HWND32 hwnd, DIALOGINFO *infoPtr )
{
    HWND32 hwndFocus = GetFocus32();

    if (!hwndFocus || !IsChild32( hwnd, hwndFocus )) return FALSE;
    infoPtr->hwndFocus = hwndFocus;
      /* Remove default button */
    return TRUE;
}


/***********************************************************************
 *           DEFDLG_RestoreFocus
 */
static BOOL32 DEFDLG_RestoreFocus( HWND32 hwnd, DIALOGINFO *infoPtr )
{
    if (!infoPtr->hwndFocus || IsIconic32(hwnd)) return FALSE;
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
static HWND32 DEFDLG_FindDefButton( HWND32 hwndDlg )
{
    HWND32 hwndChild = GetWindow32( hwndDlg, GW_CHILD );
    while (hwndChild)
    {
        if (SendMessage16( hwndChild, WM_GETDLGCODE, 0, 0 ) & DLGC_DEFPUSHBUTTON)
            break;
        hwndChild = GetWindow32( hwndChild, GW_HWNDNEXT );
    }
    return hwndChild;
}


/***********************************************************************
 *           DEFDLG_SetDefButton
 *
 * Set the new default button to be hwndNew.
 */
static BOOL32 DEFDLG_SetDefButton( HWND32 hwndDlg, DIALOGINFO *dlgInfo,
                                   HWND32 hwndNew )
{
    if (hwndNew &&
        !(SendMessage16(hwndNew, WM_GETDLGCODE, 0, 0 ) & DLGC_UNDEFPUSHBUTTON))
        return FALSE;  /* Destination is not a push button */
    
    if (dlgInfo->idResult)  /* There's already a default pushbutton */
    {
        HWND32 hwndOld = GetDlgItem32( hwndDlg, dlgInfo->idResult );
        if (SendMessage32A( hwndOld, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)
            SendMessage32A( hwndOld, BM_SETSTYLE32, BS_PUSHBUTTON, TRUE );
    }
    if (hwndNew)
    {
        SendMessage32A( hwndNew, BM_SETSTYLE32, BS_DEFPUSHBUTTON, TRUE );
        dlgInfo->idResult = GetDlgCtrlID32( hwndNew );
    }
    else dlgInfo->idResult = 0;
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
    HWND32 hwndDefId;

    switch(msg)
    {
        case WM_ERASEBKGND:
	    FillWindow( hwnd, hwnd, (HDC16)wParam, (HBRUSH16)CTLCOLOR_DLG );
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
		DeleteObject32( dlgInfo->hUserFont );
		dlgInfo->hUserFont = 0;
	    }

	      /* Delete menu */
	    if (dlgInfo->hMenu)
	    {		
		DestroyMenu32( dlgInfo->hMenu );
		dlgInfo->hMenu = 0;
	    }

            /* Delete window procedure */
            WINPROC_FreeProc( dlgInfo->dlgProc );
            dlgInfo->dlgProc = (HWINDOWPROC)0;
	    dlgInfo->fEnd    = TRUE;	/* just in case */

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
                                 wParam ? GetDlgItem32( hwnd, wParam ) : 0 );
            return 1;

        case DM_GETDEFID:
            if (dlgInfo->fEnd) return 0;
	    if (dlgInfo->idResult)
	      return MAKELONG( dlgInfo->idResult, DC_HASDEFID );
	    hwndDefId = DEFDLG_FindDefButton( hwnd );
	    if (hwndDefId)
	      return MAKELONG( GetDlgCtrlID32( hwndDefId ), DC_HASDEFID);
	    return 0;

	case WM_NEXTDLGCTL:
	    {
                HWND32 hwndDest = (HWND32)wParam;
                if (!lParam)
                    hwndDest = GetNextDlgTabItem32(hwnd, GetFocus32(), wParam);
                if (hwndDest) DEFDLG_SetFocus( hwnd, hwndDest );
                DEFDLG_SetDefButton( hwnd, dlgInfo, hwndDest );
            }
            return 0;

	case WM_GETFONT: 
	    return dlgInfo->hUserFont;

        case WM_CLOSE:
            EndDialog( hwnd, TRUE );
            DestroyWindow32( hwnd );
            return 0;
    }
    return 0;
}

/***********************************************************************
 *           DEFDLG_Signoff
 */
static LRESULT DEFDLG_Signoff(DIALOGINFO* dlgInfo, UINT32 msg, BOOL16 fResult)
{
    /* see SDK 3.1 */

    if ((msg >= WM_CTLCOLORMSGBOX && msg <= WM_CTLCOLORSTATIC) ||
	 msg == WM_CTLCOLOR || msg == WM_COMPAREITEM ||
         msg == WM_VKEYTOITEM || msg == WM_CHARTOITEM ||
         msg == WM_QUERYDRAGICON || msg == WM_INITDIALOG)
        return fResult; 

    return dlgInfo->msgResult;
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

    if (dlgInfo->dlgProc) 	/* Call dialog procedure */
	result = (BOOL16)CallWindowProc16( (WNDPROC16)dlgInfo->dlgProc,
                                           hwnd, msg, wParam, lParam );

    /* Check if window was destroyed by dialog procedure */

    if( !result && IsWindow(hwnd))
    {
        /* callback didn't process this message */

        switch(msg)
        {
            case WM_ERASEBKGND:
            case WM_SHOWWINDOW:
            case WM_ACTIVATE:
            case WM_SETFOCUS:
            case DM_SETDEFID:
            case DM_GETDEFID:
            case WM_NEXTDLGCTL:
            case WM_GETFONT:
            case WM_CLOSE:
            case WM_NCDESTROY:
                return DEFDLG_Proc( (HWND32)hwnd, msg, 
                                    (WPARAM32)wParam, lParam, dlgInfo );
            case WM_INITDIALOG:
            case WM_VKEYTOITEM:
            case WM_COMPAREITEM:
            case WM_CHARTOITEM:
                break;

            default:
                return DefWindowProc16( hwnd, msg, wParam, lParam );
        }
    }   
    return DEFDLG_Signoff(dlgInfo, msg, result);
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

    if (dlgInfo->dlgProc)       /* Call dialog procedure */
        result = (BOOL16)CallWindowProc32A( (WNDPROC32)dlgInfo->dlgProc,
                                            hwnd, msg, wParam, lParam );

    /* Check if window was destroyed by dialog procedure */

    if( !result && IsWindow(hwnd))
    {
        /* callback didn't process this message */

        switch(msg)
        {
            case WM_ERASEBKGND:
            case WM_SHOWWINDOW:
            case WM_ACTIVATE:
            case WM_SETFOCUS:
            case DM_SETDEFID:
            case DM_GETDEFID:
            case WM_NEXTDLGCTL:
            case WM_GETFONT:
            case WM_CLOSE:
            case WM_NCDESTROY:
                 return DEFDLG_Proc( (HWND32)hwnd, msg,
                                     (WPARAM32)wParam, lParam, dlgInfo );
            case WM_INITDIALOG:
            case WM_VKEYTOITEM:
            case WM_COMPAREITEM:
            case WM_CHARTOITEM:
                 break;

            default:
                 return DefWindowProc32A( hwnd, msg, wParam, lParam );
        }
    }
    return DEFDLG_Signoff(dlgInfo, msg, result);
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

    if (dlgInfo->dlgProc)       /* Call dialog procedure */
        result = (BOOL16)CallWindowProc32W( (WNDPROC32)dlgInfo->dlgProc,
                                            hwnd, msg, wParam, lParam );

    /* Check if window was destroyed by dialog procedure */

    if( !result && IsWindow(hwnd))
    {
        /* callback didn't process this message */

        switch(msg)
        {
            case WM_ERASEBKGND:
            case WM_SHOWWINDOW:
            case WM_ACTIVATE:
            case WM_SETFOCUS:
            case DM_SETDEFID:
            case DM_GETDEFID:
            case WM_NEXTDLGCTL:
            case WM_GETFONT:
            case WM_CLOSE:
            case WM_NCDESTROY:
                 return DEFDLG_Proc( (HWND32)hwnd, msg,
                                     (WPARAM32)wParam, lParam, dlgInfo );
            case WM_INITDIALOG:
            case WM_VKEYTOITEM:
            case WM_COMPAREITEM:
            case WM_CHARTOITEM:
                 break;

            default:
                 return DefWindowProc32W( hwnd, msg, wParam, lParam );
        }
    }
    return DEFDLG_Signoff(dlgInfo, msg, result);
}
