/*
 * Dialog functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 *
static char Copyright[] = "Copyright  Alexandre Julliard, 1993, 1994";
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "dialog.h"
#include "win.h"
#include "ldt.h"
#include "stackframe.h"
#include "user.h"
#include "message.h"
#include "stddebug.h"
/* #define DEBUG_DIALOG */
#include "debug.h"

  /* Dialog base units */
static WORD xBaseUnit = 0, yBaseUnit = 0;


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
    HWND hwnd;
    WND *wndPtr = WIN_FindWndPtr( hwndDlg );
    hwnd = wndPtr->hwndChild;
    while(hwnd)
    {
        wndPtr = WIN_FindWndPtr( hwnd );
        if (wndPtr->dwStyle & WS_TABSTOP) break;
        hwnd = wndPtr->hwndNext;
    }
    return hwnd;
}


/***********************************************************************
 *           DIALOG_GetControl
 *
 * Return the class and text of the control pointed to by ptr,
 * and return a pointer to the next control.
 */
static SEGPTR DIALOG_GetControl( SEGPTR ptr, SEGPTR *class, SEGPTR *text )
{
    unsigned char *base = (unsigned char *)PTR_SEG_TO_LIN( ptr );
    unsigned char *p = base;

    p += 14;  /* size of control header */

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
	*text = MAKEINTRESOURCE( p[1] + 256 * p[2] );
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
 
    result->header = *(DLGTEMPLATEHEADER *)p;
    p += 13;

    /* Get the menu name */

    if (*p == 0xff)
    {
        result->menuName = MAKEINTRESOURCE( p[1] + 256 * p[2] );
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

    if (result->header.style & DS_SETFONT)
    {
	result->pointSize = *(WORD *)p;
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
    dprintf_dialog(stddeb, "DIALOG %d, %d, %d, %d\n", result->header.x, result->header.y,
	    result->header.cx, result->header.cy );
    dprintf_dialog(stddeb, " STYLE %08lx\n", result->header.style );
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

    if (result->header.style & DS_SETFONT)
	dprintf_dialog( stddeb, " FONT %d,'%s'\n", result->pointSize,
                        (char *)PTR_SEG_TO_LIN(result->faceName) );
}


/***********************************************************************
 *           CreateDialog   (USER.89)
 */
HWND CreateDialog( HINSTANCE hInst, SEGPTR dlgTemplate,
		   HWND owner, WNDPROC dlgProc )
{
    return CreateDialogParam( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           CreateDialogParam   (USER.241)
 */
HWND CreateDialogParam( HINSTANCE hInst, SEGPTR dlgTemplate,
		        HWND owner, WNDPROC dlgProc, LPARAM param )
{
    HWND hwnd = 0;
    HRSRC hRsrc;
    HGLOBAL hmem;
    SEGPTR data;

    dprintf_dialog(stddeb, "CreateDialogParam: "NPFMT",%08lx,"NPFMT",%08lx,%ld\n",
	    hInst, dlgTemplate, owner, (DWORD)dlgProc, param );
     
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
			   HWND owner, WNDPROC dlgProc )
{
    return CreateDialogIndirectParam( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           CreateDialogIndirectParam   (USER.242)
 */
HWND CreateDialogIndirectParam( HINSTANCE hInst, SEGPTR dlgTemplate,
			        HWND owner, WNDPROC dlgProc, LPARAM param )
{
    HMENU hMenu = 0;
    HFONT hFont = 0;
    HWND hwnd, hwndCtrl;
    RECT rect;
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

    if (template.header.style & DS_SETFONT)
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
    rect.right = template.header.cx * xUnit / 4;
    rect.bottom = template.header.cy * yUnit / 8;
    if (template.header.style & DS_MODALFRAME) exStyle |= WS_EX_DLGMODALFRAME;
    AdjustWindowRectEx( &rect, template.header.style, 
			hMenu ? TRUE : FALSE , exStyle );
    rect.right -= rect.left;
    rect.bottom -= rect.top;

    if ((INT)template.header.x == CW_USEDEFAULT)
        rect.left = rect.top = CW_USEDEFAULT;
    else
    {
        rect.left += template.header.x * xUnit / 4;
        rect.top += template.header.y * yUnit / 8;
        if (!(template.header.style & DS_ABSALIGN))
            ClientToScreen( owner, (POINT *)&rect );
    }

    hwnd = CreateWindowEx( exStyle, template.className, template.caption, 
			   template.header.style & ~WS_VISIBLE,
			   rect.left, rect.top, rect.right, rect.bottom,
			   owner, hMenu, hInst, (SEGPTR)0 );
    if (!hwnd)
    {
	if (hFont) DeleteObject( hFont );
	if (hMenu) DestroyMenu( hMenu );
	return 0;
    }

      /* Create control windows */

    dprintf_dialog(stddeb, " BEGIN\n" );

    wndPtr = WIN_FindWndPtr( hwnd );
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    dlgInfo->msgResult = 0;  /* This is used to store the default button id */
    dlgInfo->hDialogHeap = 0;

    for (i = 0; i < template.header.nbItems; i++)
    {
	DLGCONTROLHEADER *header;
	SEGPTR className, winName;
        HWND hwndDefButton = 0;
        char buffer[10];

        header = (DLGCONTROLHEADER *)PTR_SEG_TO_LIN( headerPtr );
	headerPtr = DIALOG_GetControl( headerPtr, &className, &winName );

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
                       header->id, header->x, header->y, 
                       header->cx, header->cy, header->style );

	if (HIWORD(className) &&
            !strcmp( (char *)PTR_SEG_TO_LIN(className), "EDIT") &&
            ((header->style & DS_LOCALEDIT) != DS_LOCALEDIT))
        {
	    if (!dlgInfo->hDialogHeap)
            {
		dlgInfo->hDialogHeap = GlobalAlloc(GMEM_FIXED, 0x10000);
		if (!dlgInfo->hDialogHeap)
                {
		    fprintf(stderr,"CreateDialogIndirectParam: Insufficient memory to create heap for edit control\n");
		    continue;
		}
		LocalInit(dlgInfo->hDialogHeap, 0, 0xffff);
	    }
	    hwndCtrl = CreateWindowEx(WS_EX_NOPARENTNOTIFY, className, winName,
                                      header->style | WS_CHILD,
                                      header->x * xUnit / 4,
                                      header->y * yUnit / 8,
                                      header->cx * xUnit / 4,
                                      header->cy * yUnit / 8,
                                      hwnd, (HMENU)header->id,
                                      dlgInfo->hDialogHeap, (SEGPTR)0 );
	}
	else
        {
	    hwndCtrl = CreateWindowEx(WS_EX_NOPARENTNOTIFY, className, winName,
                                      header->style | WS_CHILD,
                                      header->x * xUnit / 4,
                                      header->y * yUnit / 8,
                                      header->cx * xUnit / 4,
                                      header->cy * yUnit / 8,
                                      hwnd, (HMENU)header->id,
                                      hInst, (SEGPTR)0 );
	}

        /* Make the control last one in Z-order, so that controls remain
           in the order in which they were created */
	SetWindowPos( hwndCtrl, HWND_BOTTOM, 0, 0, 0, 0,
                      SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );

            /* Send initialisation messages to the control */
        if (hFont) SendMessage( hwndCtrl, WM_SETFONT, (WPARAM)hFont, 0 );
        if (SendMessage( hwndCtrl, WM_GETDLGCODE, 0, 0 ) & DLGC_DEFPUSHBUTTON)
        {
              /* If there's already a default push-button, set it back */
              /* to normal and use this one instead. */
            if (hwndDefButton)
                SendMessage( hwndDefButton, BM_SETSTYLE, BS_PUSHBUTTON, FALSE);
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
	SendMessage( hwnd, WM_SETFONT, (WPARAM)dlgInfo->hUserFont, 0 );
    if (SendMessage( hwnd, WM_INITDIALOG, (WPARAM)dlgInfo->hwndFocus, param ))
	SetFocus( dlgInfo->hwndFocus );
    if (template.header.style & WS_VISIBLE) ShowWindow(hwnd, SW_SHOW);
    return hwnd;
}


/***********************************************************************
 *           DIALOG_DoDialogBox
 */
static int DIALOG_DoDialogBox( HWND hwnd, HWND owner )
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

    while (MSG_InternalGetMessage( USER_HEAP_SEG_ADDR(msgHandle), hwnd, owner,
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
int DialogBox( HINSTANCE hInst, SEGPTR dlgTemplate,
	       HWND owner, WNDPROC dlgProc )
{
    return DialogBoxParam( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           DialogBoxParam   (USER.239)
 */
int DialogBoxParam( HINSTANCE hInst, SEGPTR dlgTemplate,
		    HWND owner, WNDPROC dlgProc, LPARAM param )
{
    HWND hwnd;
    
    dprintf_dialog(stddeb, "DialogBoxParam: "NPFMT",%08lx,"NPFMT",%08lx,%ld\n",
	    hInst, dlgTemplate, owner, (DWORD)dlgProc, param );
    hwnd = CreateDialogParam( hInst, dlgTemplate, owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxIndirect   (USER.218)
 */
int DialogBoxIndirect( HINSTANCE hInst, HANDLE dlgTemplate,
		       HWND owner, WNDPROC dlgProc )
{
    return DialogBoxIndirectParam( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           DialogBoxIndirectParam   (USER.240)
 */
int DialogBoxIndirectParam( HINSTANCE hInst, HANDLE dlgTemplate,
			    HWND owner, WNDPROC dlgProc, LPARAM param )
{
    HWND hwnd;
    SEGPTR ptr;

    if (!(ptr = (SEGPTR)WIN16_GlobalLock( dlgTemplate ))) return -1;
    hwnd = CreateDialogIndirectParam( hInst, ptr, owner, dlgProc, param );
    GlobalUnlock( dlgTemplate );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           EndDialog   (USER.88)
 */
void EndDialog( HWND hwnd, short retval )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    DIALOGINFO * dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    dlgInfo->msgResult = retval;
    dlgInfo->fEnd = TRUE;
    dprintf_dialog(stddeb, "EndDialog: "NPFMT" %d\n", hwnd, retval );
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

    dlgCode = SendMessage( msg->hwnd, WM_GETDLGCODE, 0, 0 );
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
                SendMessage( hwndDlg, WM_NEXTDLGCTL,
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
            SendMessage( hwndDlg, WM_COMMAND, 
			 MAKEWPARAM( IDCANCEL, 0 ),
                         (LPARAM)GetDlgItem(hwndDlg,IDCANCEL) );
#else
            SendMessage( hwndDlg, WM_COMMAND, IDCANCEL,
                         MAKELPARAM( GetDlgItem(hwndDlg,IDCANCEL), 0 ));
#endif
            break;

        case VK_RETURN:
            {
                DWORD dw = SendMessage( hwndDlg, DM_GETDEFID, 0, 0 );
                if (HIWORD(dw) == DC_HASDEFID)
#ifdef WINELIB32
                    SendMessage( hwndDlg, WM_COMMAND, 
				 MAKEWPARAM( LOWORD(dw), BN_CLICKED ),
                                 (LPARAM)GetDlgItem( hwndDlg, LOWORD(dw) ) );
                else
                    SendMessage( hwndDlg, WM_COMMAND, 
				 MAKEWPARAM( IDOK, 0 ),
                                 (LPARAM)GetDlgItem(hwndDlg,IDOK) );
#else
                    SendMessage( hwndDlg, WM_COMMAND, LOWORD(dw),
                                 MAKELPARAM( GetDlgItem( hwndDlg, LOWORD(dw) ),
                                             BN_CLICKED ));
                else
                    SendMessage( hwndDlg, WM_COMMAND, IDOK,
                                 MAKELPARAM( GetDlgItem(hwndDlg,IDOK), 0 ));
#endif
            }
            break;
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
    HWND curChild;
    WND * childPtr;
    WND * wndPtr;

    if (!(wndPtr = WIN_FindWndPtr( hwndDlg ))) return 0;
    curChild = wndPtr->hwndChild;
    while(curChild)
    {
	childPtr = WIN_FindWndPtr( curChild );
	if (childPtr->wIDmenu == id) return curChild;
	curChild = childPtr->hwndNext;
    }
    return 0;
}


/*******************************************************************
 *           SendDlgItemMessage   (USER.101)
 */
LRESULT SendDlgItemMessage(HWND hwnd, INT id, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndCtrl = GetDlgItem( hwnd, id );
    if (hwndCtrl) return SendMessage( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SetDlgItemText   (USER.92)
 */
void SetDlgItemText( HWND hwnd, WORD id, SEGPTR lpString )
{
    SendDlgItemMessage( hwnd, id, WM_SETTEXT, 0, (DWORD)lpString );
}


/***********************************************************************
 *           GetDlgItemText   (USER.93)
 */
int GetDlgItemText( HWND hwnd, WORD id, SEGPTR str, WORD max )
{
    return (int)SendDlgItemMessage( hwnd, id, WM_GETTEXT, max, (DWORD)str );
}


/*******************************************************************
 *           SetDlgItemInt   (USER.94)
 */
void SetDlgItemInt( HWND hwnd, WORD id, WORD value, BOOL fSigned )
{
    char str[20];

    if (fSigned) sprintf( str, "%d", (int)value );
    else sprintf( str, "%u", value );
    SendDlgItemMessage( hwnd, id, WM_SETTEXT, 0, MAKE_SEGPTR(str) );
}


/***********************************************************************
 *           GetDlgItemInt   (USER.95)
 */
WORD GetDlgItemInt( HWND hwnd, WORD id, BOOL * translated, BOOL fSigned )
{
    char str[30];
    long result = 0;
    
    if (translated) *translated = FALSE;
    if (SendDlgItemMessage( hwnd, id, WM_GETTEXT, 30, MAKE_SEGPTR(str) ))
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
    return (WORD)result;
}


/***********************************************************************
 *           CheckDlgButton   (USER.97)
 */
void CheckDlgButton( HWND hwnd, WORD id, WORD check )
{
    SendDlgItemMessage( hwnd, id, BM_SETCHECK, check, 0 );
}


/***********************************************************************
 *           IsDlgButtonChecked   (USER.98)
 */
WORD IsDlgButtonChecked( HWND hwnd, WORD id )
{
    return (WORD)SendDlgItemMessage( hwnd, id, BM_GETCHECK, 0, 0 );
}


/***********************************************************************
 *           CheckRadioButton   (USER.96)
 */
void CheckRadioButton( HWND hwndDlg, WORD firstID, WORD lastID, WORD checkID )
{
    HWND button = GetWindow( hwndDlg, GW_CHILD );
    WND *wndPtr;

    while (button)
    {
	if (!(wndPtr = WIN_FindWndPtr( button ))) return;
        if ((wndPtr->wIDmenu == firstID) || (wndPtr->wIDmenu == lastID)) break;
	button = wndPtr->hwndNext;
    }
    if (!button) return;

    if (wndPtr->wIDmenu == lastID)
        lastID = firstID;  /* Buttons are in reverse order */
    while (button)
    {
	if (!(wndPtr = WIN_FindWndPtr( button ))) return;
	SendMessage( button, BM_SETCHECK, (wndPtr->wIDmenu == checkID), 0 );
        if (wndPtr->wIDmenu == lastID) break;
	button = wndPtr->hwndNext;
    }
}


/***********************************************************************
 *           GetDialogBaseUnits   (USER.243)
 */
DWORD GetDialogBaseUnits()
{
    return MAKELONG( xBaseUnit, yBaseUnit );
}


/***********************************************************************
 *           MapDialogRect   (USER.103)
 */
void MapDialogRect( HWND hwnd, LPRECT rect )
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
    HWND hwnd, hwndStart;
    WND * dlgPtr, * ctrlPtr, * wndPtr;

    if (!(dlgPtr = WIN_FindWndPtr( hwndDlg ))) return 0;
    if (!(ctrlPtr = WIN_FindWndPtr( hwndCtrl ))) return 0;
    if (ctrlPtr->hwndParent != hwndDlg) return 0;

    if (!fPrevious && ctrlPtr->hwndNext)  /*Check if next control is in group*/
    {
	wndPtr = WIN_FindWndPtr( ctrlPtr->hwndNext );
        if (!(wndPtr->dwStyle & WS_GROUP)) return ctrlPtr->hwndNext;
    }

      /* Now we will have to find the start of the group */

    hwndStart = hwnd = dlgPtr->hwndChild;
    while (hwnd)
    {
	wndPtr = WIN_FindWndPtr( hwnd );
        if (wndPtr->dwStyle & WS_GROUP) hwndStart = hwnd;  /*Start of a group*/
	if (hwnd == hwndCtrl) break;
	hwnd = wndPtr->hwndNext;
    }

    if (!hwnd) fprintf(stderr, "GetNextDlgGroupItem: hwnd not in dialog!\n");

      /* only case left for forward search: wraparound */
    if (!fPrevious) return hwndStart;
    
    hwnd = hwndStart;
    wndPtr = WIN_FindWndPtr( hwnd );
    hwnd = wndPtr->hwndNext;
    while (hwnd && (hwnd != hwndCtrl))
    {
	wndPtr = WIN_FindWndPtr( hwnd );
	if (wndPtr->dwStyle & WS_GROUP) break;
	hwndStart = hwnd;
	hwnd = wndPtr->hwndNext;
    }
    return hwndStart;
}


/***********************************************************************
 *           GetNextDlgTabItem   (USER.228)
 */
HWND GetNextDlgTabItem( HWND hwndDlg, HWND hwndCtrl, BOOL fPrevious )
{
    HWND hwnd, hwndLast;
    WND * dlgPtr, * ctrlPtr, * wndPtr;

    if (!(dlgPtr = WIN_FindWndPtr( hwndDlg ))) return 0;
    if (!(ctrlPtr = WIN_FindWndPtr( hwndCtrl ))) return 0;
    if (ctrlPtr->hwndParent != hwndDlg) return 0;

    hwndLast = hwndCtrl;
    hwnd = ctrlPtr->hwndNext;
    while (1)
    {
	if (!hwnd) hwnd = dlgPtr->hwndChild;
	if (hwnd == hwndCtrl) break;
	wndPtr = WIN_FindWndPtr( hwnd );
	if ((wndPtr->dwStyle & WS_TABSTOP) && (wndPtr->dwStyle & WS_VISIBLE))
	{
	    hwndLast = hwnd;
	    if (!fPrevious) break;
	}
	hwnd = wndPtr->hwndNext;
    }
    return hwndLast;
}
