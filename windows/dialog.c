/*
 * Dialog functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "windows.h"
#include "dialog.h"
#include "win.h"


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

    if (!(hdc = GetDC(GetDesktopWindow()))) return FALSE;
    GetTextMetrics( hdc, &tm );
    ReleaseDC( 0, hdc );
    xBaseUnit = tm.tmAveCharWidth;
    yBaseUnit = tm.tmHeight;
#ifdef DEBUG_DIALOG
    printf( "DIALOG_Init: base units = %d,%d\n", xBaseUnit, yBaseUnit );
#endif    
    return TRUE;
}


/***********************************************************************
 *           DIALOG_GetControl
 *
 * Return the class and text of the control pointed to by ptr,
 * and return a pointer to the next control.
 */
static DLGCONTROLHEADER * DIALOG_GetControl( DLGCONTROLHEADER * ptr,
					     char ** class, char ** text )
{
    unsigned char * p = (unsigned char *)ptr;
    p += 14;  /* size of control header */
    if (*p & 0x80)
    {
	switch(*p++)
	{
	case 0x80: *class = "BUTTON"; break;
	case 0x81: *class = "EDIT"; break;
	case 0x82: *class = "STATIC"; break;
	case 0x83: *class = "LISTBOX"; break;
	case 0x84: *class = "SCROLLBAR"; break;
	case 0x85: *class = "COMBOBOX"; break;
	default:   *class = ""; break;
	}
    }
    else 
    {
	*class = p;
	p += strlen(p) + 1;
    }
    *text = p;
    p += strlen(p) + 2;
    return (DLGCONTROLHEADER *)p;
}


/***********************************************************************
 *           DIALOG_ParseTemplate
 *
 * Fill a DLGTEMPLATE structure from the dialog template, and return
 * a pointer to the first control.
 */
static DLGCONTROLHEADER * DIALOG_ParseTemplate( LPCSTR template,
					        DLGTEMPLATE * result )
{
    unsigned char * p = (unsigned char *)template;
 
    result->header = (DLGTEMPLATEHEADER *)p;
    p += 13;

    result->menuName = p;
    if (*p == 0xff) p += 3;
    else p += strlen(p) + 1;

    if (*p) result->className = p;
    else result->className = DIALOG_CLASS_NAME;
    p += strlen(p) + 1;

    result->caption = p;
    p += strlen(p) + 1;

    if (result->header->style & DS_SETFONT)
    {
	result->pointSize = *(WORD *)p;	p += sizeof(WORD);
	result->faceName = p;           p += strlen(p) + 1;
    }

    return (DLGCONTROLHEADER *)p;
}


/***********************************************************************
 *           DIALOG_DisplayTemplate
 */
#ifdef DEBUG_DIALOG
static void DIALOG_DisplayTemplate( DLGTEMPLATE * result )
{
    printf( "DIALOG %d, %d, %d, %d\n", result->header->x, result->header->y,
	    result->header->cx, result->header->cy );
    printf( " STYLE %08x\n", result->header->style );
    printf( " CAPTION '%s'\n", result->caption );
    printf( " CLASS '%s'\n", result->className );
    if (result->menuName[0] == 0xff)
	printf( " MENU %d\n", result->menuName[1] + 256*result->menuName[2] );
    else printf( " MENU '%s'\n", result->menuName );
    if (result->header->style & DS_SETFONT)
	printf( " FONT %d,'%s'\n", result->pointSize, result->faceName );
}
#endif  /* DEBUG_DIALOG */


/***********************************************************************
 *           CreateDialog   (USER.89)
 */
HWND CreateDialog( HINSTANCE hInst, LPCSTR dlgTemplate,
		   HWND owner, FARPROC dlgProc )
{
    return CreateDialogParam( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           CreateDialogParam   (USER.241)
 */
HWND CreateDialogParam( HINSTANCE hInst, LPCSTR dlgTemplate,
		        HWND owner, FARPROC dlgProc, LPARAM param )
{
    HWND hwnd = 0;
    HANDLE hres, hmem;
    LPCSTR data;

#ifdef DEBUG_DIALOG
    printf( "CreateDialogParam: %d,'%s',%d,%p,%d\n",
	    hInst, dlgTemplate, owner, dlgProc, param );
#endif
     
      /* FIXME: MAKEINTRESOURCE should be replaced by RT_DIALOG */
    if (!(hres = FindResource( hInst, dlgTemplate, MAKEINTRESOURCE(0x8005) )))
	return 0;
    if (!(hmem = LoadResource( hInst, hres ))) return 0;
    if (!(data = LockResource( hmem ))) hwnd = 0;
    else hwnd = CreateDialogIndirectParam(hInst, data, owner, dlgProc, param);
    FreeResource( hmem );
    return hwnd;
}


/***********************************************************************
 *           CreateDialogIndirect   (USER.219)
 */
HWND CreateDialogIndirect( HINSTANCE hInst, LPCSTR dlgTemplate,
			   HWND owner, FARPROC dlgProc )
{
    return CreateDialogIndirectParam( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           CreateDialogIndirectParam   (USER.242)
 */
HWND CreateDialogIndirectParam( HINSTANCE hInst, LPCSTR dlgTemplate,
			        HWND owner, FARPROC dlgProc, LPARAM param )
{
    HMENU hMenu;
    HFONT hFont = 0;
    HWND hwnd;
    WND * wndPtr;
    int i;
    DLGTEMPLATE template;
    DLGCONTROLHEADER * header;
    DIALOGINFO * dlgInfo;
    WORD xUnit = xBaseUnit;
    WORD yUnit = yBaseUnit;

      /* Parse dialog template */

    if (!dlgTemplate) return 0;
    header = DIALOG_ParseTemplate( dlgTemplate, &template );
#ifdef DEBUG_DIALOG
    DIALOG_DisplayTemplate( &template );
#endif    

      /* Load menu */

    switch (template.menuName[0])
    {
      case 0x00:
	  hMenu = 0;
	  break;
      case 0xff:
	  hMenu = LoadMenu( hInst, MAKEINTRESOURCE( template.menuName[1] +
						   256*template.menuName[2] ));
	  break;
      default:
	  hMenu = LoadMenu( hInst, template.menuName );
	  break;
    }

      /* Create custom font if needed */

    if (template.header->style & DS_SETFONT)
    {
	hFont = CreateFont( template.pointSize, 0, 0, 0, FW_DONTCARE,
			    FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
			    DEFAULT_QUALITY, FF_DONTCARE, template.faceName );
	if (hFont)
	{
	    TEXTMETRIC tm;
	    HFONT oldFont;
	    HDC hdc;

	    hdc = GetDC(GetDesktopWindow());
	    oldFont = SelectObject( hdc, hFont );
	    GetTextMetrics( hdc, &tm );
	    SelectObject( hdc, oldFont );
	    ReleaseDC( 0, hdc );
	    xUnit = tm.tmAveCharWidth;
	    yUnit = tm.tmHeight;
	}
    }
    
      /* Create dialog main window */

    hwnd = CreateWindow( template.className, template.caption,
			 template.header->style,
			 template.header->x * xUnit / 4,
			 template.header->y * yUnit / 8,
			 template.header->cx * xUnit / 4,
			 template.header->cy * yUnit / 8,
			 owner, hMenu, hInst,
			 NULL );
    if (!hwnd)
    {
	if (hFont) DeleteObject( hFont );
	if (hMenu) DestroyMenu( hMenu );
	return 0;
    }

      /* Create control windows */

#ifdef DEBUG_DIALOG
    printf( " BEGIN\n" );
#endif	

    for (i = 0; i < template.header->nbItems; i++)
    {
	DLGCONTROLHEADER * next_header;
	LPSTR class, text;
	next_header = DIALOG_GetControl( header, &class, &text );

#ifdef DEBUG_DIALOG
	printf( "   %s '%s' %d, %d, %d, %d, %d, %08x\n",
	        class, text, header->id, header->x, header->y, header->cx,
	        header->cy, header->style );
#endif
	if ((strcmp(class, "STATIC") == 0) & ((header->style & SS_ICON) == SS_ICON)) {
	    header->cx = 32;
	    header->cy = 32;
	    }
	header->style |= WS_CHILD;
	CreateWindowEx( WS_EX_NOPARENTNOTIFY, 
		        class, text, header->style,
		        header->x * xUnit / 4, header->y * yUnit / 8,
		        header->cx * xUnit / 4, header->cy * yUnit / 8,
		        hwnd, header->id, hInst, NULL );
	header = next_header;
    }    

#ifdef DEBUG_DIALOG
    printf( " END\n" );
#endif	
    
      /* Initialise dialog extra data */

    wndPtr = WIN_FindWndPtr( hwnd );
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    dlgInfo->dlgProc   = dlgProc;
    dlgInfo->hUserFont = hFont;
    dlgInfo->hMenu     = hMenu;
    dlgInfo->xBaseUnit = xUnit;
    dlgInfo->yBaseUnit = yUnit;
    dlgInfo->hwndFocus = GetNextDlgTabItem( hwnd,
			    GetWindow(wndPtr->hwndChild, GW_HWNDLAST), FALSE );

      /* Send initialisation messages and set focus */

    if (dlgInfo->hUserFont) 
	SendMessage( hwnd, WM_SETFONT, dlgInfo->hUserFont, 0);
    SendMessage( hwnd, WM_INITDIALOG, dlgInfo->hwndFocus, param );
/*    if (SendMessage( hwnd, WM_INITDIALOG, dlgInfo->hwndFocus, param ))
	SetFocus( dlgInfo->hwndFocus ); */

    return hwnd;
}


/***********************************************************************
 *           DIALOG_DoDialogBox
 */
static int DIALOG_DoDialogBox( HWND hwnd )
{
    WND * wndPtr;
    DIALOGINFO * dlgInfo;
    MSG msg;
    int retval;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return -1;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    ShowWindow( hwnd, SW_SHOW );

    while (GetMessage (&msg, 0, 0, 0))
    {
	if (!IsDialogMessage( hwnd, &msg))
	{
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
	if (dlgInfo->fEnd) break;
    }
    retval = dlgInfo->msgResult;
    DestroyWindow( hwnd );
    return retval;
}


/***********************************************************************
 *           DialogBox   (USER.87)
 */
int DialogBox( HINSTANCE hInst, LPCSTR dlgTemplate,
	       HWND owner, FARPROC dlgProc )
{
    return DialogBoxParam( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           DialogBoxParam   (USER.239)
 */
int DialogBoxParam( HINSTANCE hInst, LPCSTR dlgTemplate,
		    HWND owner, FARPROC dlgProc, LPARAM param )
{
    HWND hwnd;
    
#ifdef DEBUG_DIALOG
    printf( "DialogBoxParam: %d,'%s',%d,%p,%d\n",
	    hInst, dlgTemplate, owner, dlgProc, param );
#endif
    hwnd = CreateDialogParam( hInst, dlgTemplate, owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd );
    return -1;
}


/***********************************************************************
 *           DialogBoxIndirect   (USER.218)
 */
int DialogBoxIndirect( HINSTANCE hInst, HANDLE dlgTemplate,
		       HWND owner, FARPROC dlgProc )
{
    return DialogBoxIndirectParam( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           DialogBoxIndirectParam   (USER.240)
 */
int DialogBoxIndirectParam( HINSTANCE hInst, HANDLE dlgTemplate,
			    HWND owner, FARPROC dlgProc, LPARAM param )
{
    HWND hwnd;
    LPCSTR ptr;

    if (!(ptr = GlobalLock( dlgTemplate ))) return -1;
    hwnd = CreateDialogIndirectParam( hInst, ptr, owner, dlgProc, param );
    GlobalUnlock( dlgTemplate );
    if (hwnd) return DIALOG_DoDialogBox( hwnd );
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
#ifdef DEBUG_DIALOG
    printf( "EndDialog: %d %d\n", hwnd, retval );
#endif    
}


/***********************************************************************
 *           IsDialogMessage   (USER.90)
 */
BOOL IsDialogMessage( HWND hwndDlg, LPMSG msg )
{
    WND * wndPtr;
    
    if (!(wndPtr = WIN_FindWndPtr( hwndDlg ))) return FALSE;
    if ((hwndDlg != msg->hwnd) && !IsChild( hwndDlg, msg->hwnd )) return FALSE;

    if (msg->message != WM_KEYDOWN)
    {
	SendMessage( msg->hwnd, msg->message, msg->wParam, msg->lParam );
    }
    else
    {
	int dlgCode = SendMessage( msg->hwnd, WM_GETDLGCODE, 0, 0 );
	/* Process key message */
	/* .... */
    }
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
LONG SendDlgItemMessage(HWND hwnd, WORD id, UINT msg, WORD wParam, LONG lParam)
{
    HWND hwndCtrl = GetDlgItem( hwnd, id );
    if (hwndCtrl) return SendMessage( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SetDlgItemText   (USER.92)
 */
void SetDlgItemText( HWND hwnd, WORD id, LPSTR lpString )
{
    SendDlgItemMessage( hwnd, id, WM_SETTEXT, 0, (DWORD)lpString );
}


/***********************************************************************
 *           GetDlgItemText   (USER.93)
 */
int GetDlgItemText( HWND hwnd, WORD id, LPSTR str, WORD max )
{
    return (int)SendDlgItemMessage( hwnd, id, WM_GETTEXT, max, (DWORD)str );
}


/*******************************************************************
 *           SetDlgItemInt   (USER.94)
 */
void SetDlgItemInt( HWND hwnd, WORD id, WORD value, BOOL fSigned )
{
    HANDLE hText = LocalAlloc( LMEM_MOVEABLE, 10 );
    char * str = (char *) LocalLock( hText );

    if (fSigned) sprintf( str, "%d", value );
    else sprintf( str, "%u", value );
    SendDlgItemMessage( hwnd, id, WM_SETTEXT, 0, (DWORD)str );
    LocalUnlock( hText );
    LocalFree( hText );
}


/***********************************************************************
 *           GetDlgItemInt   (USER.95)
 */
WORD GetDlgItemInt( HWND hwnd, WORD id, BOOL * translated, BOOL fSigned )
{
    int len;
    HANDLE hText;
    long result;
    char * str;
    
    if (translated) *translated = FALSE;
    if (!(len = SendDlgItemMessage( hwnd, id, WM_GETTEXTLENGTH, 0, 0 )))
	return 0;
    if (!(hText = LocalAlloc(LMEM_MOVEABLE, len+1 )))
	return 0;
    str = (char *) LocalLock( hText );
    if (SendDlgItemMessage( hwnd, id, WM_GETTEXT, len+1, (DWORD)str ))
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
    LocalUnlock( hText );
    LocalFree( hText );
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
    HWND button = GetDlgItem( hwndDlg, firstID );
    while (button != 0)
    {
	WND * wndPtr = WIN_FindWndPtr( button );
	if (!wndPtr) break;
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
	if (wndPtr->dwStyle & WS_TABSTOP)
	{
	    hwndLast = hwnd;
	    if (!fPrevious) break;
	}
	hwnd = wndPtr->hwndNext;
    }
    return hwndLast;
    return 0;
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
	if (wndPtr->dwStyle & WS_TABSTOP)
	{
	    hwndLast = hwnd;
	    if (!fPrevious) break;
	}
	hwnd = wndPtr->hwndNext;
    }
    return hwndLast;
}
