/*
 * Dialog functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "dialog.h"
#include "heap.h"
#include "win.h"
#include "ldt.h"
#include "stackframe.h"
#include "user.h"
#include "message.h"
#include "stddebug.h"
/* #define DEBUG_DIALOG */
#include "debug.h"

  /* Dialog base units */
WORD xBaseUnit = 0, yBaseUnit = 0;


/***********************************************************************
 *           DIALOG_Init
 *
 * Initialisation of the dialog manager.
 */
BOOL DIALOG_Init()
{
    TEXTMETRIC tm;
    HDC hdc;
    
      /* Calculate the dialog base units */

    if (!(hdc = GetDC( 0 ))) return FALSE;
    GetTextMetrics( hdc, &tm );
    ReleaseDC( 0, hdc );
    xBaseUnit = tm.tmAveCharWidth;
    yBaseUnit = tm.tmHeight;

      /* Dialog units are based on a proportional system font */
      /* so we adjust them a bit for a fixed font. */
    if (tm.tmPitchAndFamily & TMPF_FIXED_PITCH) xBaseUnit = xBaseUnit * 5 / 4;

    dprintf_dialog( stddeb, "DIALOG_Init: base units = %d,%d\n",
                    xBaseUnit, yBaseUnit );
    return TRUE;
}


/***********************************************************************
 *           DIALOG_GetFirstTabItem
 *
 * Return the first item of the dialog that has the WS_TABSTOP style.
 */
HWND DIALOG_GetFirstTabItem( HWND hwndDlg )
{
    WND *pWnd = WIN_FindWndPtr( hwndDlg );
    for (pWnd = pWnd->child; pWnd; pWnd = pWnd->next)
        if (pWnd->dwStyle & WS_TABSTOP) return pWnd->hwndSelf;
    return 0;
}


/***********************************************************************
 *           DIALOG_GetControl
 *
 * Return the class and text of the control pointed to by ptr,
 * fill the header structure and return a pointer to the next control.
 */
static SEGPTR DIALOG_GetControl( SEGPTR ptr, DLGCONTROLHEADER *header,
                                 SEGPTR *class, SEGPTR *text )
{
    unsigned char *base = (unsigned char *)PTR_SEG_TO_LIN( ptr );
    unsigned char *p = base;

    header->x  = GET_WORD(p); p += sizeof(WORD);
    header->y  = GET_WORD(p); p += sizeof(WORD);
    header->cx = GET_WORD(p); p += sizeof(WORD);
    header->cy = GET_WORD(p); p += sizeof(WORD);
    header->id = GET_WORD(p); p += sizeof(WORD);
    header->style = GET_DWORD(p); p += sizeof(DWORD);

    if (*p & 0x80)
    {
        *class = MAKEINTRESOURCE( *p );
        p++;
    }
    else 
    {
	*class = ptr + (WORD)(p - base);
	p += strlen(p) + 1;
    }

    if (*p == 0xff)
    {
	  /* Integer id, not documented (?). Only works for SS_ICON controls */
	*text = MAKEINTRESOURCE( GET_WORD(p+1) );
	p += 4;
    }
    else
    {
	*text = ptr + (WORD)(p - base);
	p += strlen(p) + 2;
    }
    return ptr + (WORD)(p - base);
}


/***********************************************************************
 *           DIALOG_ParseTemplate
 *
 * Fill a DLGTEMPLATE structure from the dialog template, and return
 * a pointer to the first control.
 */
static SEGPTR DIALOG_ParseTemplate( SEGPTR template, DLGTEMPLATE * result )
{
    unsigned char *base = (unsigned char *)PTR_SEG_TO_LIN(template);
    unsigned char * p = base;
 
    result->style = GET_DWORD(p); p += sizeof(DWORD);
    result->nbItems = *p++;
    result->x  = GET_WORD(p); p += sizeof(WORD);
    result->y  = GET_WORD(p); p += sizeof(WORD);
    result->cx = GET_WORD(p); p += sizeof(WORD);
    result->cy = GET_WORD(p); p += sizeof(WORD);

    /* Get the menu name */

    if (*p == 0xff)
    {
        result->menuName = MAKEINTRESOURCE( GET_WORD(p+1) );
        p += 3;
    }
    else if (*p)
    {
        result->menuName = template + (WORD)(p - base);
        p += strlen(p) + 1;
    }
    else
    {
        result->menuName = 0;
        p++;
    }

    /* Get the class name */

    if (*p) result->className = template + (WORD)(p - base);
    else result->className = DIALOG_CLASS_ATOM;
    p += strlen(p) + 1;

    /* Get the window caption */

    result->caption = template + (WORD)(p - base);
    p += strlen(p) + 1;

    /* Get the font name */

    if (result->style & DS_SETFONT)
    {
	result->pointSize = GET_WORD(p);
        p += sizeof(WORD);
	result->faceName = template + (WORD)(p - base);
        p += strlen(p) + 1;
    }

    return template + (WORD)(p - base);
}


/***********************************************************************
 *           DIALOG_DisplayTemplate
 */
static void DIALOG_DisplayTemplate( DLGTEMPLATE * result )
{
    dprintf_dialog(stddeb, "DIALOG %d, %d, %d, %d\n", result->x, result->y,
                   result->cx, result->cy );
    dprintf_dialog(stddeb, " STYLE %08lx\n", result->style );
    dprintf_dialog( stddeb, " CAPTION '%s'\n",
                    (char *)PTR_SEG_TO_LIN(result->caption) );

    if (HIWORD(result->className))
        dprintf_dialog( stddeb, " CLASS '%s'\n",
                        (char *)PTR_SEG_TO_LIN(result->className) );
    else
        dprintf_dialog( stddeb, " CLASS #%d\n", LOWORD(result->className) );

    if (HIWORD(result->menuName))
        dprintf_dialog( stddeb, " MENU '%s'\n",
                        (char *)PTR_SEG_TO_LIN(result->menuName) );
    else if (LOWORD(result->menuName))
	dprintf_dialog(stddeb, " MENU %04x\n", LOWORD(result->menuName) );

    if (result->style & DS_SETFONT)
	dprintf_dialog( stddeb, " FONT %d,'%s'\n", result->pointSize,
                        (char *)PTR_SEG_TO_LIN(result->faceName) );
}


/***********************************************************************
 *           CreateDialog   (USER.89)
 */
HWND CreateDialog( HINSTANCE hInst, SEGPTR dlgTemplate,
		   HWND owner, DLGPROC dlgProc )
{
    return CreateDialogParam( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           CreateDialogParam   (USER.241)
 */
HWND CreateDialogParam( HINSTANCE hInst, SEGPTR dlgTemplate,
		        HWND owner, DLGPROC dlgProc, LPARAM param )
{
    HWND hwnd = 0;
    HRSRC hRsrc;
    HGLOBAL hmem;
    SEGPTR data;

    dprintf_dialog(stddeb, "CreateDialogParam: %04x,%08lx,%04x,%08lx,%ld\n",
                   hInst, (DWORD)dlgTemplate, owner, (DWORD)dlgProc, param );
     
    if (!(hRsrc = FindResource( hInst, dlgTemplate, RT_DIALOG ))) return 0;
    if (!(hmem = LoadResource( hInst, hRsrc ))) return 0;
    if (!(data = WIN16_LockResource( hmem ))) hwnd = 0;
    else hwnd = CreateDialogIndirectParam(hInst, data, owner, dlgProc, param);
    FreeResource( hmem );
    return hwnd;
}


/***********************************************************************
 *           CreateDialogIndirect   (USER.219)
 */
HWND CreateDialogIndirect( HINSTANCE hInst, SEGPTR dlgTemplate,
			   HWND owner, DLGPROC dlgProc )
{
    return CreateDialogIndirectParam( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           CreateDialogIndirectParam   (USER.242)
 */
HWND CreateDialogIndirectParam( HINSTANCE hInst, SEGPTR dlgTemplate,
			        HWND owner, DLGPROC dlgProc, LPARAM param )
{
    HMENU hMenu = 0;
    HFONT hFont = 0;
    HWND hwnd, hwndCtrl;
    RECT16 rect;
    WND * wndPtr;
    int i;
    DLGTEMPLATE template;
    SEGPTR headerPtr;
    DIALOGINFO * dlgInfo;
    DWORD exStyle = 0;
    WORD xUnit = xBaseUnit;
    WORD yUnit = yBaseUnit;

      /* Parse dialog template */

    if (!dlgTemplate) return 0;
    headerPtr = DIALOG_ParseTemplate( dlgTemplate, &template );
    if (debugging_dialog) DIALOG_DisplayTemplate( &template );

      /* Load menu */

    if (template.menuName) hMenu = LoadMenu( hInst, template.menuName );

      /* Create custom font if needed */

    if (template.style & DS_SETFONT)
    {
          /* The font height must be negative as it is a point size */
          /* (see CreateFont() documentation in the Windows SDK).   */
	hFont = CreateFont( -template.pointSize, 0, 0, 0, FW_DONTCARE,
			    FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
			    DEFAULT_QUALITY, FF_DONTCARE,
                            (LPSTR)PTR_SEG_TO_LIN(template.faceName) );
	if (hFont)
	{
	    TEXTMETRIC tm;
	    HFONT oldFont;
	    HDC hdc;

	    hdc = GetDC(0);
	    oldFont = SelectObject( hdc, hFont );
	    GetTextMetrics( hdc, &tm );
	    SelectObject( hdc, oldFont );
	    ReleaseDC( 0, hdc );
	    xUnit = tm.tmAveCharWidth;
	    yUnit = tm.tmHeight;
            if (tm.tmPitchAndFamily & TMPF_FIXED_PITCH)
                xBaseUnit = xBaseUnit * 5 / 4;  /* See DIALOG_Init() */
	}
    }
    
      /* Create dialog main window */

    rect.left = rect.top = 0;
    rect.right = template.cx * xUnit / 4;
    rect.bottom = template.cy * yUnit / 8;
    if (template.style & DS_MODALFRAME) exStyle |= WS_EX_DLGMODALFRAME;
    AdjustWindowRectEx16( &rect, template.style, 
                          hMenu ? TRUE : FALSE , exStyle );
    rect.right -= rect.left;
    rect.bottom -= rect.top;

    if ((INT16)template.x == CW_USEDEFAULT16)
        rect.left = rect.top = CW_USEDEFAULT16;
    else
    {
        rect.left += template.x * xUnit / 4;
        rect.top += template.y * yUnit / 8;
        if (!(template.style & DS_ABSALIGN))
            ClientToScreen16( owner, (POINT16 *)&rect );
    }

    hwnd = CreateWindowEx16( exStyle, template.className, template.caption, 
			   template.style & ~WS_VISIBLE,
			   rect.left, rect.top, rect.right, rect.bottom,
			   owner, hMenu, hInst, (SEGPTR)0 );
    if (!hwnd)
    {
	if (hFont) DeleteObject( hFont );
	if (hMenu) DestroyMenu( hMenu );
	return 0;
    }
    wndPtr = WIN_FindWndPtr( hwnd );

      /* Create control windows */

    dprintf_dialog(stddeb, " BEGIN\n" );

    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    dlgInfo->msgResult = 0;  /* This is used to store the default button id */
    dlgInfo->hDialogHeap = 0;

    for (i = 0; i < template.nbItems; i++)
    {
	DLGCONTROLHEADER header;
	SEGPTR className, winName;
        HWND hwndDefButton = 0;
        char buffer[10];

	headerPtr = DIALOG_GetControl( headerPtr, &header,
                                       &className, &winName );

        if (!HIWORD(className))
        {
            switch(LOWORD(className))
            {
            case 0x80: strcpy( buffer, "BUTTON" ); break;
            case 0x81: strcpy( buffer, "EDIT" ); break;
            case 0x82: strcpy( buffer, "STATIC" ); break;
            case 0x83: strcpy( buffer, "LISTBOX" ); break;
            case 0x84: strcpy( buffer, "SCROLLBAR" ); break;
            case 0x85: strcpy( buffer, "COMBOBOX" ); break;
            default:   buffer[0] = '\0'; break;
            }
            className = MAKE_SEGPTR(buffer);
        }

        if (HIWORD(className))
            dprintf_dialog(stddeb, "   %s ", (char*)PTR_SEG_TO_LIN(className));
        else dprintf_dialog(stddeb, "   %04x ", LOWORD(className) );
	if (HIWORD(winName))
            dprintf_dialog(stddeb,"'%s'", (char *)PTR_SEG_TO_LIN(winName) );
	else dprintf_dialog(stddeb,"%04x", LOWORD(winName) );

	dprintf_dialog(stddeb," %d, %d, %d, %d, %d, %08lx\n", 
                       header.id, header.x, header.y, 
                       header.cx, header.cy, header.style );

	if (HIWORD(className) &&
            !strcmp( (char *)PTR_SEG_TO_LIN(className), "EDIT") &&
            ((header.style & DS_LOCALEDIT) != DS_LOCALEDIT))
        {
	    if (!dlgInfo->hDialogHeap)
            {
		dlgInfo->hDialogHeap = GlobalAlloc16(GMEM_FIXED, 0x10000);
		if (!dlgInfo->hDialogHeap)
                {
		    fprintf(stderr,"CreateDialogIndirectParam: Insufficient memory to create heap for edit control\n");
		    continue;
		}
		LocalInit(dlgInfo->hDialogHeap, 0, 0xffff);
	    }
	    hwndCtrl = CreateWindowEx16(WS_EX_NOPARENTNOTIFY, className, winName,
                                      header.style | WS_CHILD,
                                      header.x * xUnit / 4,
                                      header.y * yUnit / 8,
                                      header.cx * xUnit / 4,
                                      header.cy * yUnit / 8,
                                      hwnd, (HMENU)header.id,
                                      dlgInfo->hDialogHeap, (SEGPTR)0 );
	}
	else
        {
	    hwndCtrl = CreateWindowEx16( WS_EX_NOPARENTNOTIFY, className,
                                         winName,
                                         header.style | WS_CHILD,
                                         header.x * xUnit / 4,
                                         header.y * yUnit / 8,
                                         header.cx * xUnit / 4,
                                         header.cy * yUnit / 8,
                                         hwnd, (HMENU)header.id,
                                         hInst, (SEGPTR)0 );
	}

        /* Make the control last one in Z-order, so that controls remain
           in the order in which they were created */
	SetWindowPos( hwndCtrl, HWND_BOTTOM, 0, 0, 0, 0,
                      SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );

            /* Send initialisation messages to the control */
        if (hFont) SendMessage16( hwndCtrl, WM_SETFONT, (WPARAM)hFont, 0 );
        if (SendMessage16( hwndCtrl, WM_GETDLGCODE, 0, 0 ) & DLGC_DEFPUSHBUTTON)
        {
              /* If there's already a default push-button, set it back */
              /* to normal and use this one instead. */
            if (hwndDefButton)
                SendMessage32A( hwndDefButton, BM_SETSTYLE32,
                                BS_PUSHBUTTON,FALSE );
            hwndDefButton = hwndCtrl;
            dlgInfo->msgResult = GetWindowWord( hwndCtrl, GWW_ID );
        }
    }    

    dprintf_dialog(stddeb, " END\n" );
    
      /* Initialise dialog extra data */

    dlgInfo->dlgProc   = dlgProc;
    dlgInfo->hUserFont = hFont;
    dlgInfo->hMenu     = hMenu;
    dlgInfo->xBaseUnit = xUnit;
    dlgInfo->yBaseUnit = yUnit;
    dlgInfo->hwndFocus = DIALOG_GetFirstTabItem( hwnd );

      /* Send initialisation messages and set focus */

    if (dlgInfo->hUserFont)
	SendMessage16( hwnd, WM_SETFONT, (WPARAM)dlgInfo->hUserFont, 0 );
    if (SendMessage16( hwnd, WM_INITDIALOG, (WPARAM)dlgInfo->hwndFocus, param ))
	SetFocus( dlgInfo->hwndFocus );
    if (template.style & WS_VISIBLE) ShowWindow(hwnd, SW_SHOW);
    return hwnd;
}


/***********************************************************************
 *           DIALOG_DoDialogBox
 */
int DIALOG_DoDialogBox( HWND hwnd, HWND owner )
{
    WND * wndPtr;
    DIALOGINFO * dlgInfo;
    HANDLE msgHandle;
    MSG* lpmsg;
    int retval;

      /* Owner must be a top-level window */
    owner = WIN_GetTopParent( owner );
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return -1;
    if (!(msgHandle = USER_HEAP_ALLOC( sizeof(MSG) ))) return -1;
    lpmsg = (MSG *) USER_HEAP_LIN_ADDR( msgHandle );
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    EnableWindow( owner, FALSE );
    ShowWindow( hwnd, SW_SHOW );

    while (MSG_InternalGetMessage( (SEGPTR)USER_HEAP_SEG_ADDR(msgHandle), hwnd, owner,
                                   MSGF_DIALOGBOX, PM_REMOVE,
                                   !(wndPtr->dwStyle & DS_NOIDLEMSG) ))
    {
	if (!IsDialogMessage( hwnd, lpmsg))
	{
	    TranslateMessage( lpmsg );
	    DispatchMessage( lpmsg );
	}
	if (dlgInfo->fEnd) break;
    }
    retval = dlgInfo->msgResult;
    DestroyWindow( hwnd );
    USER_HEAP_FREE( msgHandle );
    EnableWindow( owner, TRUE );
    return retval;
}


/***********************************************************************
 *           DialogBox   (USER.87)
 */
INT DialogBox( HINSTANCE hInst, SEGPTR dlgTemplate,
	       HWND owner, DLGPROC dlgProc )
{
    return DialogBoxParam( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           DialogBoxParam   (USER.239)
 */
INT DialogBoxParam( HINSTANCE hInst, SEGPTR dlgTemplate,
		    HWND owner, DLGPROC dlgProc, LPARAM param )
{
    HWND hwnd;
    
    dprintf_dialog(stddeb, "DialogBoxParam: %04x,%08lx,%04x,%08lx,%ld\n",
                   hInst, (DWORD)dlgTemplate, owner, (DWORD)dlgProc, param );
    hwnd = CreateDialogParam( hInst, dlgTemplate, owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxIndirect   (USER.218)
 */
INT DialogBoxIndirect( HINSTANCE hInst, HANDLE dlgTemplate,
		       HWND owner, DLGPROC dlgProc )
{
    return DialogBoxIndirectParam( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           DialogBoxIndirectParam   (USER.240)
 */
INT DialogBoxIndirectParam( HINSTANCE hInst, HANDLE dlgTemplate,
			    HWND owner, DLGPROC dlgProc, LPARAM param )
{
    HWND hwnd;
    SEGPTR ptr;

    if (!(ptr = (SEGPTR)WIN16_GlobalLock16( dlgTemplate ))) return -1;
    hwnd = CreateDialogIndirectParam( hInst, ptr, owner, dlgProc, param );
    GlobalUnlock16( dlgTemplate );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           EndDialog   (USER.88)
 */
BOOL EndDialog( HWND hwnd, INT retval )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    DIALOGINFO * dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    dlgInfo->msgResult = retval;
    dlgInfo->fEnd = TRUE;
    dprintf_dialog(stddeb, "EndDialog: %04x %d\n", hwnd, retval );
    return TRUE;
}


/***********************************************************************
 *           IsDialogMessage   (USER.90)
 */
BOOL IsDialogMessage( HWND hwndDlg, LPMSG msg )
{
    WND * wndPtr;
    int dlgCode;

    if (!(wndPtr = WIN_FindWndPtr( hwndDlg ))) return FALSE;
    if ((hwndDlg != msg->hwnd) && !IsChild( hwndDlg, msg->hwnd )) return FALSE;

      /* Only the key messages get special processing */
    if ((msg->message != WM_KEYDOWN) &&
        (msg->message != WM_SYSCHAR) &&
	(msg->message != WM_CHAR))
        return FALSE;

    dlgCode = SendMessage16( msg->hwnd, WM_GETDLGCODE, 0, 0 );
    if (dlgCode & DLGC_WANTMESSAGE)
    {
        DispatchMessage( msg );
        return TRUE;
    }

    switch(msg->message)
    {
    case WM_KEYDOWN:
        if (dlgCode & DLGC_WANTALLKEYS) break;
        switch(msg->wParam)
        {
        case VK_TAB:
            if (!(dlgCode & DLGC_WANTTAB))
            {
                SendMessage16( hwndDlg, WM_NEXTDLGCTL,
                             (GetKeyState(VK_SHIFT) & 0x80), 0 );
                return TRUE;
            }
            break;
            
        case VK_RIGHT:
        case VK_DOWN:
            if (!(dlgCode & DLGC_WANTARROWS))
            {
                SetFocus(GetNextDlgGroupItem(hwndDlg,GetFocus(),FALSE));
                return TRUE;
            }
            break;

        case VK_LEFT:
        case VK_UP:
            if (!(dlgCode & DLGC_WANTARROWS))
            {
                SetFocus(GetNextDlgGroupItem(hwndDlg,GetFocus(),TRUE));
                return TRUE;
            }
            break;

        case VK_ESCAPE:
#ifdef WINELIB32
            SendMessage32A( hwndDlg, WM_COMMAND, 
                            MAKEWPARAM( IDCANCEL, 0 ),
                            (LPARAM)GetDlgItem(hwndDlg,IDCANCEL) );
#else
            SendMessage16( hwndDlg, WM_COMMAND, IDCANCEL,
                           MAKELPARAM( GetDlgItem(hwndDlg,IDCANCEL), 0 ));
#endif
            break;

        case VK_RETURN:
            {
                DWORD dw = SendMessage16( hwndDlg, DM_GETDEFID, 0, 0 );
                if (HIWORD(dw) == DC_HASDEFID)
#ifdef WINELIB32
                    SendMessage32A( hwndDlg, WM_COMMAND, 
                                    MAKEWPARAM( LOWORD(dw), BN_CLICKED ),
                                   (LPARAM)GetDlgItem( hwndDlg, LOWORD(dw) ) );
                else
                    SendMessage32A( hwndDlg, WM_COMMAND, 
                                    MAKEWPARAM( IDOK, 0 ),
                                    (LPARAM)GetDlgItem(hwndDlg,IDOK) );
#else
                    SendMessage16( hwndDlg, WM_COMMAND, LOWORD(dw),
                                 MAKELPARAM( GetDlgItem( hwndDlg, LOWORD(dw) ),
                                             BN_CLICKED ));
                else
                    SendMessage16( hwndDlg, WM_COMMAND, IDOK,
                                 MAKELPARAM( GetDlgItem(hwndDlg,IDOK), 0 ));
#endif
            }
            break;

	default: 
	    TranslateMessage( msg );
        }
        break;  /* case WM_KEYDOWN */

        
    case WM_CHAR:
        if (dlgCode & (DLGC_WANTALLKEYS | DLGC_WANTCHARS)) break;
        break;

    case WM_SYSCHAR:
        if (dlgCode & DLGC_WANTALLKEYS) break;
        break;
    }

      /* If we get here, the message has not been treated specially */
      /* and can be sent to its destination window. */
    DispatchMessage( msg );
    return TRUE;
}


/****************************************************************
 *         GetDlgCtrlID           (USER.277)
 */
int GetDlgCtrlID( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr) return wndPtr->wIDmenu;
    else return 0;
}
 

/***********************************************************************
 *           GetDlgItem   (USER.91)
 */
HWND GetDlgItem( HWND hwndDlg, WORD id )
{
    WND *pWnd;

    if (!(pWnd = WIN_FindWndPtr( hwndDlg ))) return 0;
    for (pWnd = pWnd->child; pWnd; pWnd = pWnd->next)
        if (pWnd->wIDmenu == id) return pWnd->hwndSelf;
    return 0;
}


/*******************************************************************
 *           SendDlgItemMessage16   (USER.101)
 */
LRESULT SendDlgItemMessage16( HWND16 hwnd, INT16 id, UINT16 msg,
                              WPARAM16 wParam, LPARAM lParam )
{
    HWND16 hwndCtrl = GetDlgItem( hwnd, id );
    if (hwndCtrl) return SendMessage16( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SendDlgItemMessage32A   (USER32.451)
 */
LRESULT SendDlgItemMessage32A( HWND32 hwnd, INT32 id, UINT32 msg,
                               WPARAM32 wParam, LPARAM lParam )
{
    HWND hwndCtrl = GetDlgItem( (HWND16)hwnd, (INT16)id );
    if (hwndCtrl) return SendMessage32A( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SendDlgItemMessage32W   (USER32.452)
 */
LRESULT SendDlgItemMessage32W( HWND32 hwnd, INT32 id, UINT32 msg,
                               WPARAM32 wParam, LPARAM lParam )
{
    HWND hwndCtrl = GetDlgItem( (HWND16)hwnd, (INT16)id );
    if (hwndCtrl) return SendMessage32W( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SetDlgItemText16   (USER.92)
 */
void SetDlgItemText16( HWND16 hwnd, INT16 id, SEGPTR lpString )
{
    SendDlgItemMessage16( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *           SetDlgItemText32A   (USER32.477)
 */
void SetDlgItemText32A( HWND32 hwnd, INT32 id, LPCSTR lpString )
{
    SendDlgItemMessage32A( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *           SetDlgItemText32W   (USER32.478)
 */
void SetDlgItemText32W( HWND32 hwnd, INT32 id, LPCWSTR lpString )
{
    SendDlgItemMessage32W( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}


/***********************************************************************
 *           GetDlgItemText16   (USER.93)
 */
INT16 GetDlgItemText16( HWND16 hwnd, INT16 id, SEGPTR str, UINT16 len )
{
    return (INT16)SendDlgItemMessage16( hwnd, id, WM_GETTEXT,
                                        len, (LPARAM)str );
}


/***********************************************************************
 *           GetDlgItemText32A   (USER32.236)
 */
INT32 GetDlgItemText32A( HWND32 hwnd, INT32 id, LPSTR str, UINT32 len )
{
    return (INT32)SendDlgItemMessage32A( hwnd, id, WM_GETTEXT,
                                         len, (LPARAM)str );
}


/***********************************************************************
 *           GetDlgItemText32W   (USER32.237)
 */
INT32 GetDlgItemText32W( HWND32 hwnd, INT32 id, LPWSTR str, UINT32 len )
{
    return (INT32)SendDlgItemMessage32W( hwnd, id, WM_GETTEXT,
                                         len, (LPARAM)str );
}


/*******************************************************************
 *           SetDlgItemInt16   (USER.94)
 */
void SetDlgItemInt16( HWND16 hwnd, INT16 id, UINT16 value, BOOL16 fSigned )
{
    char *str = (char *)SEGPTR_ALLOC( 20 * sizeof(char) );

    if (!str) return;
    if (fSigned) sprintf( str, "%d", (INT32)(INT16)value );
    else sprintf( str, "%u", value );
    SendDlgItemMessage16( hwnd, id, WM_SETTEXT, 0, (LPARAM)SEGPTR_GET(str) );
    SEGPTR_FREE(str);
}


/*******************************************************************
 *           SetDlgItemInt32   (USER32.476)
 */
void SetDlgItemInt32( HWND32 hwnd, INT32 id, UINT32 value, BOOL32 fSigned )
{
    char str[20];

    if (fSigned) sprintf( str, "%d", (INT32)value );
    else sprintf( str, "%u", value );
    SendDlgItemMessage32A( hwnd, id, WM_SETTEXT, 0, (LPARAM)str );
}


/***********************************************************************
 *           GetDlgItemInt   (USER.95)
 */
WORD GetDlgItemInt( HWND hwnd, WORD id, BOOL * translated, BOOL fSigned )
{
    char *str;
    long result = 0;
    
    if (translated) *translated = FALSE;
    if (!(str = (char *)SEGPTR_ALLOC( 30 * sizeof(char) ))) return 0;
    if (SendDlgItemMessage16( hwnd, id, WM_GETTEXT, 30, (LPARAM)SEGPTR_GET(str)))
    {
	char * endptr;
	result = strtol( str, &endptr, 10 );
	if (endptr && (endptr != str))  /* Conversion was successful */
	{
	    if (fSigned)
	    {
		if ((result < -32767) || (result > 32767)) result = 0;
		else if (translated) *translated = TRUE;
	    }
	    else
	    {
		if ((result < 0) || (result > 65535)) result = 0;
		else if (translated) *translated = TRUE;
	    }
	}
    }
    SEGPTR_FREE(str);
    return (WORD)result;
}


/***********************************************************************
 *           CheckDlgButton   (USER.97)
 */
BOOL CheckDlgButton( HWND hwnd, INT id, UINT check )
{
    SendDlgItemMessage32A( hwnd, id, BM_SETCHECK32, check, 0 );
    return TRUE;
}


/***********************************************************************
 *           IsDlgButtonChecked   (USER.98)
 */
WORD IsDlgButtonChecked( HWND hwnd, WORD id )
{
    return (WORD)SendDlgItemMessage16( hwnd, id, BM_GETCHECK16, 0, 0 );
}


/***********************************************************************
 *           CheckRadioButton   (USER.96)
 */
BOOL CheckRadioButton( HWND hwndDlg, UINT firstID, UINT lastID, UINT checkID )
{
    WND *pWnd = WIN_FindWndPtr( hwndDlg );
    if (!pWnd) return FALSE;

    for (pWnd = pWnd->child; pWnd; pWnd = pWnd->next)
        if ((pWnd->wIDmenu == firstID) || (pWnd->wIDmenu == lastID)) break;
    if (!pWnd) return FALSE;

    if (pWnd->wIDmenu == lastID)
        lastID = firstID;  /* Buttons are in reverse order */
    while (pWnd)
    {
	SendMessage32A( pWnd->hwndSelf, BM_SETCHECK32,
                        (pWnd->wIDmenu == checkID), 0 );
        if (pWnd->wIDmenu == lastID) break;
	pWnd = pWnd->next;
    }
    return TRUE;
}


/***********************************************************************
 *           GetDialogBaseUnits   (USER.243)
 */
DWORD GetDialogBaseUnits()
{
    return MAKELONG( xBaseUnit, yBaseUnit );
}


/***********************************************************************
 *           MapDialogRect16   (USER.103)
 */
void MapDialogRect16( HWND16 hwnd, LPRECT16 rect )
{
    DIALOGINFO * dlgInfo;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    rect->left   = (rect->left * dlgInfo->xBaseUnit) / 4;
    rect->right  = (rect->right * dlgInfo->xBaseUnit) / 4;
    rect->top    = (rect->top * dlgInfo->yBaseUnit) / 8;
    rect->bottom = (rect->bottom * dlgInfo->yBaseUnit) / 8;
}


/***********************************************************************
 *           MapDialogRect32   (USER32.381)
 */
void MapDialogRect32( HWND32 hwnd, LPRECT32 rect )
{
    DIALOGINFO * dlgInfo;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    rect->left   = (rect->left * dlgInfo->xBaseUnit) / 4;
    rect->right  = (rect->right * dlgInfo->xBaseUnit) / 4;
    rect->top    = (rect->top * dlgInfo->yBaseUnit) / 8;
    rect->bottom = (rect->bottom * dlgInfo->yBaseUnit) / 8;
}


/***********************************************************************
 *           GetNextDlgGroupItem   (USER.227)
 */
HWND GetNextDlgGroupItem( HWND hwndDlg, HWND hwndCtrl, BOOL fPrevious )
{
    WND *pWnd, *pWndStart, *pWndCtrl, *pWndDlg;

    if (!(pWndDlg = WIN_FindWndPtr( hwndDlg ))) return 0;
    if (!(pWndCtrl = WIN_FindWndPtr( hwndCtrl ))) return 0;
    if (pWndCtrl->parent != pWndDlg) return 0;

    if (!fPrevious && pWndCtrl->next)  /* Check if next control is in group */
    {
        if (!(pWndCtrl->next->dwStyle & WS_GROUP))
            return pWndCtrl->next->hwndSelf;
    }

      /* Now we will have to find the start of the group */

    for (pWnd = pWndStart = pWndDlg->child; pWnd; pWnd = pWnd->next)
    {
        if (pWnd->dwStyle & WS_GROUP) pWndStart = pWnd;  /* Start of a group */
	if (pWnd == pWndCtrl) break;
    }

    if (!pWnd) fprintf(stderr, "GetNextDlgGroupItem: hwnd not in dialog!\n");

      /* only case left for forward search: wraparound */
    if (!fPrevious) return pWndStart->hwndSelf;

    pWnd = pWndStart->next;
    while (pWnd && (pWnd != pWndCtrl))
    {
        if (pWnd->dwStyle & WS_GROUP) break;
        pWndStart = pWnd;
        pWnd = pWnd->next;
    }
    return pWndStart->hwndSelf;
}


/***********************************************************************
 *           GetNextDlgTabItem   (USER.228)
 */
HWND GetNextDlgTabItem( HWND hwndDlg, HWND hwndCtrl, BOOL fPrevious )
{
    WND *pWnd, *pWndLast, *pWndCtrl, *pWndDlg;

    if (!(pWndDlg = WIN_FindWndPtr( hwndDlg ))) return 0;
    if (!(pWndCtrl = WIN_FindWndPtr( hwndCtrl ))) return 0;
    if (pWndCtrl->parent != pWndDlg) return 0;

    pWndLast = pWndCtrl;
    pWnd = pWndCtrl->next;
    while (1)
    {
        if (!pWnd) pWnd = pWndDlg->child;
        if (pWnd == pWndCtrl) break;
	if ((pWnd->dwStyle & WS_TABSTOP) && (pWnd->dwStyle & WS_VISIBLE))
	{
            pWndLast = pWnd;
	    if (!fPrevious) break;
	}
        pWnd = pWnd->next;
    }
    return pWndLast->hwndSelf;
}
