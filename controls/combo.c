/*
 * Interface code to COMBOBOX widget
 *
 * Copyright  Martin Ayotte, 1993
 *
 */

/*
#define DEBUG_COMBO
*/

static char Copyright[] = "Copyright Martin Ayotte, 1993";

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "windows.h"
#include "combo.h"
#include "heap.h"
#include "win.h"
#include "dirent.h"
#include <sys/stat.h>

LPHEADCOMBO ComboGetStorageHeader(HWND hwnd);
int CreateComboStruct(HWND hwnd);


void COMBOBOX_CreateComboBox(LPSTR className, LPSTR comboLabel, HWND hwnd)
{
    WND *wndPtr    = WIN_FindWndPtr(hwnd);
    WND *parentPtr = WIN_FindWndPtr(wndPtr->hwndParent);
    DWORD style;
    char widgetName[15];

#ifdef DEBUG_COMBO
    printf("combo: label = %s, x = %d, y = %d\n", comboLabel,
	   wndPtr->rectClient.left, wndPtr->rectClient.top);
    printf("        width = %d, height = %d\n",
	   wndPtr->rectClient.right - wndPtr->rectClient.left,
	   wndPtr->rectClient.bottom - wndPtr->rectClient.top);
#endif

    if (!wndPtr)
	return;

    style = wndPtr->dwStyle & 0x0000FFFF;
/*
    if ((style & LBS_NOTIFY) == LBS_NOTIFY)
*/    
    sprintf(widgetName, "%s%d", className, wndPtr->wIDmenu);
    wndPtr->winWidget = XtVaCreateManagedWidget(widgetName,
				    compositeWidgetClass,
				    parentPtr->winWidget,
				    XtNx, wndPtr->rectClient.left,
				    XtNy, wndPtr->rectClient.top,
				    XtNwidth, wndPtr->rectClient.right -
				        wndPtr->rectClient.left,
				    XtNheight, 16,
				    NULL );
    GlobalUnlock(hwnd);
    GlobalUnlock(wndPtr->hwndParent);
}


/***********************************************************************
 *           WIDGETS_ComboWndProc
 */
LONG COMBOBOX_ComboBoxWndProc( HWND hwnd, WORD message,
			   WORD wParam, LONG lParam )
{    
    WORD	wRet;
    RECT	rect;
    int		y;
    int		width, height;
    WND  	*wndPtr;
    LPHEADCOMBO lphc;
    char	str[128];
    PAINTSTRUCT paintstruct;
    static RECT rectsel;
    switch(message)
    {
    case WM_CREATE:
	CreateComboStruct(hwnd);
	wndPtr = WIN_FindWndPtr(hwnd);
	lphc = ComboGetStorageHeader(hwnd);
	if (lphc == NULL) return 0;
#ifdef DEBUG_COMBO
        printf("Combo WM_CREATE %lX !\n", lphc);
#endif
	width = wndPtr->rectClient.right - wndPtr->rectClient.left;
	height = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
	lphc->hWndDrop = CreateWindow("BUTTON", "", 
        	WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
        	width - 16, 0, 16, 16, hwnd, 1, wndPtr->hInstance, 0L);
	lphc->hWndEdit = CreateWindow("STATIC", "", 
        	WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | SS_LEFT,
        	0, 0, width - 16, 16, hwnd, 1, wndPtr->hInstance, 0L);
	lphc->hWndLBox = CreateWindow("LISTBOX", "", 
        	WS_CHILD | WS_CLIPCHILDREN | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
        	wndPtr->rectClient.left, wndPtr->rectClient.top + 16, width, height, 
        	wndPtr->hwndParent, 1, wndPtr->hInstance, (LPSTR)MAKELONG(0, hwnd));
        ShowWindow(lphc->hWndLBox, SW_HIDE);
#ifdef DEBUG_COMBO
        printf("Combo Creation Drop=%X LBox=%X!\n", lphc->hWndDrop, lphc->hWndLBox);
#endif
	return 0;
    case WM_DESTROY:
	lphc = ComboGetStorageHeader(hwnd);
	if (lphc == 0) return 0;
	DestroyWindow(lphc->hWndDrop);
	DestroyWindow(lphc->hWndEdit);
/*
	DestroyWindow(lphc->hWndLBox);
*/
	free(lphc);
	*((LPHEADCOMBO *)&wndPtr->wExtra[1]) = 0;
#ifdef DEBUG_COMBO
        printf("Combo WM_DESTROY %lX !\n", lphc);
#endif
	return 0;
	
    case WM_COMMAND:
	wndPtr = WIN_FindWndPtr(hwnd);
	lphc = ComboGetStorageHeader(hwnd);
	if (lphc == NULL) return 0;
        if (LOWORD(lParam) == lphc->hWndDrop) {
	    if (HIWORD(lParam) != BN_CLICKED) return 0;
#ifdef DEBUG_COMBO
	    printf("CB_SHOWDROPDOWN !\n");
#endif
	    lphc->dwState = lphc->dwState ^ CB_SHOWDROPDOWN;
	    if ((lphc->dwState & CB_SHOWDROPDOWN) == CB_SHOWDROPDOWN) {
		ShowWindow(lphc->hWndLBox, SW_SHOW);
/*
		SetFocus(lphc->hWndLBox);
*/
		}
	    else {
/*
		SetFocus(lphc->hWndEdit);
*/
		ShowWindow(lphc->hWndLBox, SW_HIDE);
		y = SendMessage(lphc->hWndLBox, LB_GETCURSEL, 0, 0L);
		if (y != LB_ERR) {
		    SendMessage(lphc->hWndLBox, LB_GETTEXT, (WORD)y, (LPARAM)str);
		    SendMessage(lphc->hWndEdit, WM_SETTEXT, (WORD)y, (LPARAM)str);
		    }
		}
            }
        if (LOWORD(lParam) == lphc->hWndLBox) {
            switch(HIWORD(lParam))
        	{
        	case LBN_SELCHANGE:
		    lphc->dwState = lphc->dwState & (CB_SHOWDROPDOWN ^ 0xFFFF);
		    ShowWindow(lphc->hWndLBox, SW_HIDE);
		    y = SendMessage(lphc->hWndLBox, LB_GETCURSEL, 0, 0L);
		    if (y != LB_ERR) {
		    	SendMessage(lphc->hWndLBox, LB_GETTEXT, (WORD)y, (LPARAM)str);
		    	SendMessage(lphc->hWndEdit, WM_SETTEXT, (WORD)y, (LPARAM)str);
		    	}
		    SendMessage(GetParent(hwnd), WM_COMMAND, wndPtr->wIDmenu,
			        	MAKELONG(hwnd, CBN_SELCHANGE));
        	    break;
        	case LBN_DBLCLK:
		    SendMessage(GetParent(hwnd), WM_COMMAND, wndPtr->wIDmenu,
			        	MAKELONG(hwnd, CBN_DBLCLK));
        	    break;
        	}
            }
	break;
    case WM_LBUTTONDOWN:
        printf("Combo WM_LBUTTONDOWN wParam=%x lParam=%lX !\n", wParam, lParam);
	break;
    case WM_KEYDOWN:
        printf("Combo WM_KEYDOWN wParam %X!\n", wParam);
	break;
    case WM_CTLCOLOR:
    	return(SendMessage(GetParent(hwnd), WM_CTLCOLOR, wParam, lParam));
    case WM_PAINT:
	BeginPaint( hwnd, &paintstruct );
	EndPaint( hwnd, &paintstruct );
	lphc = ComboGetStorageHeader(hwnd);
        InvalidateRect(lphc->hWndEdit, NULL, TRUE);
        UpdateWindow(lphc->hWndEdit);
        InvalidateRect(lphc->hWndDrop, NULL, TRUE);
        UpdateWindow(lphc->hWndDrop);
        if ((lphc->dwState & CB_SHOWDROPDOWN) == CB_SHOWDROPDOWN) {
	    InvalidateRect(lphc->hWndLBox, NULL, TRUE);
	    UpdateWindow(lphc->hWndLBox);
	    }
	break;
    case CB_ADDSTRING:
#ifdef DEBUG_COMBO
        printf("CB_ADDSTRING '%s' !\n", (LPSTR)lParam);
#endif
	lphc = ComboGetStorageHeader(hwnd);
        return(SendMessage(lphc->hWndLBox, LB_ADDSTRING, wParam, lParam));
    case CB_GETLBTEXT:
        printf("CB_GETLBTEXT #%u !\n", wParam);
	lphc = ComboGetStorageHeader(hwnd);
        return(SendMessage(lphc->hWndLBox, LB_GETTEXT, wParam, lParam));
    case CB_GETLBTEXTLEN:
        printf("CB_GETLBTEXTLEN !\n");
	lphc = ComboGetStorageHeader(hwnd);
        return(SendMessage(lphc->hWndLBox, LB_GETTEXTLEN, wParam, lParam));
    case CB_INSERTSTRING:
        printf("CB_INSERTSTRING '%s' !\n", (LPSTR)lParam);
	lphc = ComboGetStorageHeader(hwnd);
        return(SendMessage(lphc->hWndLBox, LB_INSERTSTRING, wParam, lParam));
    case CB_DELETESTRING:
        printf("CB_DELETESTRING #%u !\n", wParam);
	lphc = ComboGetStorageHeader(hwnd);
        return(SendMessage(lphc->hWndLBox, LB_DELETESTRING, wParam, 0L));
    case CB_RESETCONTENT:
        printf("CB_RESETCONTENT !\n");
	lphc = ComboGetStorageHeader(hwnd);
        return(SendMessage(lphc->hWndLBox, LB_RESETCONTENT, 0, 0L));
    case CB_DIR:
        printf("ComboBox CB_DIR !\n");
	lphc = ComboGetStorageHeader(hwnd);
        return(SendMessage(lphc->hWndLBox, LB_DIR, wParam, lParam));
    case CB_FINDSTRING:
	lphc = ComboGetStorageHeader(hwnd);
        return(SendMessage(lphc->hWndLBox, LB_FINDSTRING, wParam, lParam));
    case CB_GETCOUNT:
	lphc = ComboGetStorageHeader(hwnd);
        return(SendMessage(lphc->hWndLBox, LB_GETCOUNT, 0, 0L));
    case CB_GETCURSEL:
        printf("ComboBox CB_GETCURSEL !\n");
	lphc = ComboGetStorageHeader(hwnd);
        return(SendMessage(lphc->hWndLBox, LB_GETCURSEL, 0, 0L));
    case CB_SETCURSEL:
        printf("ComboBox CB_SETCURSEL wParam=%X !\n", wParam);
	lphc = ComboGetStorageHeader(hwnd);
        return(SendMessage(lphc->hWndLBox, LB_SETCURSEL, wParam, 0L));
    case CB_GETEDITSEL:
        printf("ComboBox CB_GETEDITSEL !\n");
	lphc = ComboGetStorageHeader(hwnd);
/*        return(SendMessage(lphc->hWndEdit, EM_GETSEL, 0, 0L)); */
        break;
     case CB_SETEDITSEL:
        printf("ComboBox CB_SETEDITSEL lParam=%lX !\n", lParam);
	lphc = ComboGetStorageHeader(hwnd);
/*        return(SendMessage(lphc->hWndEdit, EM_SETSEL, 0, lParam)); */
	break;
    case CB_SELECTSTRING:
        printf("ComboBox CB_SELECTSTRING !\n");
	lphc = ComboGetStorageHeader(hwnd);
        break;
    case CB_SHOWDROPDOWN:
        printf("ComboBox CB_SHOWDROPDOWN !\n");
	lphc = ComboGetStorageHeader(hwnd);
	lphc->dwState = lphc->dwState | CB_SHOWDROPDOWN;
	if (wParam != 0) {
	    ShowWindow(lphc->hWndLBox, SW_SHOW);
	    }
	else {
	    lphc->dwState = lphc->dwState ^ CB_SHOWDROPDOWN;
	    ShowWindow(lphc->hWndLBox, SW_HIDE);
	    SendMessage(GetParent(hwnd), WM_COMMAND, wndPtr->wIDmenu,
			        	MAKELONG(hwnd, CBN_DROPDOWN));
	    }
        break;
    case CB_GETITEMDATA:
        printf("ComboBox CB_GETITEMDATA wParam=%X !\n", wParam);
	lphc = ComboGetStorageHeader(hwnd);
        return(SendMessage(lphc->hWndLBox, LB_GETITEMDATA, wParam, 0L));
        break;
    case CB_SETITEMDATA:
        printf("ComboBox CB_SETITEMDATA wParam=%X lParam=%lX !\n", wParam, lParam);
	lphc = ComboGetStorageHeader(hwnd);
        return(SendMessage(lphc->hWndLBox, LB_SETITEMDATA, wParam, lParam));
        break;
    case CB_LIMITTEXT:
        printf("ComboBox CB_LIMITTEXT !\n");
	lphc = ComboGetStorageHeader(hwnd);
/*        return(SendMessage(lphc->hWndEdit, EM_LIMITTEXT, wParam, 0L)); */
        break;
	
    default:
	return DefWindowProc( hwnd, message, wParam, lParam );
    }
return 0;
}



LPHEADCOMBO ComboGetStorageHeader(HWND hwnd)
{
    WND  *wndPtr;
    LPHEADCOMBO lphc;
    wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr == 0) {
    	printf("Bad Window handle on ComboBox !\n");
    	return 0;
    	}
    lphc = *((LPHEADCOMBO *)&wndPtr->wExtra[1]);
    return lphc;
}



int CreateComboStruct(HWND hwnd)
{
    WND  *wndPtr;
    LPHEADCOMBO lphc;
    wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr == 0) {
    	printf("Bad Window handle on ComboBox !\n");
    	return 0;
    	}
    lphc = (LPHEADCOMBO)malloc(sizeof(HEADCOMBO));
    *((LPHEADCOMBO *)&wndPtr->wExtra[1]) = lphc;
    lphc->dwState = 0;
    return TRUE;
}



