/*
 * Treeview control
 *
 * Copyright 1998 Eric Kohl
 *
 * NOTES
 *   This is just a dummy control. An author is needed! Any volunteers?
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - All messages.
 *   - All notifications.
 */

#include "windows.h"
#include "commctrl.h"
#include "treeview.h"
#include "heap.h"
#include "win.h"
#include "debug.h"


#define TREEVIEW_GetInfoPtr(wndPtr) ((TREEVIEW_INFO *)wndPtr->wExtra[0])



static LRESULT
TREEVIEW_SetImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
    HIMAGELIST himlTemp;

    if ((INT32)wParam == TVSIL_NORMAL) {
	himlTemp = infoPtr->himlNormal;
	infoPtr->himlNormal = (HIMAGELIST)lParam;
    }
    else if ((INT32)wParam == TVSIL_STATE) {
	himlTemp = infoPtr->himlState;
	infoPtr->himlState = (HIMAGELIST)lParam;
    }
    else
	return 0;

    return (LRESULT)himlTemp;
}



static LRESULT
TREEVIEW_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TREEVIEW_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (TREEVIEW_INFO *)HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
                                   sizeof(TREEVIEW_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
	ERR (treeview, "could not allocate info memory!\n");
	return 0;
    }

    if ((TREEVIEW_INFO*)wndPtr->wExtra[0] != infoPtr) {
	ERR (treeview, "pointer assignment error!\n");
	return 0;
    }

    /* set default settings */
    infoPtr->clrBk = GetSysColor32 (COLOR_WINDOW);
    infoPtr->clrText = GetSysColor32 (COLOR_BTNTEXT);
    infoPtr->himlNormal = NULL;
    infoPtr->himlState = NULL;


    return 0;
}


static LRESULT
TREEVIEW_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);




    /* free tree view info data */
    HeapFree (GetProcessHeap (), 0, infoPtr);

    return 0;
}


static LRESULT
TREEVIEW_EraseBackground (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
    HBRUSH32 hBrush = CreateSolidBrush32 (infoPtr->clrBk);
    RECT32 rect;

    GetClientRect32 (wndPtr->hwndSelf, &rect);
    FillRect32 ((HDC32)wParam, &rect, hBrush);
    DeleteObject32 (hBrush);
    return TRUE;
}



LRESULT WINAPI
TREEVIEW_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {

    case TVM_INSERTITEMA:
      FIXME (treeview, "Unimplemented msg TVM_INSERTITEMA\n");
      return 0;

    case TVM_INSERTITEMW:
      FIXME (treeview, "Unimplemented msg TVM_INSERTITEMW\n");
      return 0;

    case TVM_DELETEITEM:
      FIXME (treeview, "Unimplemented msg TVM_DELETEITEM\n");
      return 0;

    case TVM_EXPAND:
      FIXME (treeview, "Unimplemented msg TVM_EXPAND\n");
      return 0;

    case TVM_GETITEMRECT:
      FIXME (treeview, "Unimplemented msg TVM_GETITEMRECT\n");
      return 0;

    case TVM_GETCOUNT:
      FIXME (treeview, "Unimplemented msg TVM_GETCOUNT\n");
      return 0;

    case TVM_GETINDENT:
      FIXME (treeview, "Unimplemented msg TVM_GETINDENT\n");
      return 0;

    case TVM_SETINDENT:
      FIXME (treeview, "Unimplemented msg TVM_SETINDENT\n");
      return 0;

    case TVM_GETIMAGELIST:
      FIXME (treeview, "Unimplemented msg TVM_GETIMAGELIST\n");
      return 0;
      //return TREEVIEW_GetImageList (wndPtr, wParam, lParam);

	case TVM_SETIMAGELIST:
	    return TREEVIEW_SetImageList (wndPtr, wParam, lParam);

    case TVM_GETNEXTITEM:
      FIXME (treeview, "Unimplemented msg TVM_GETNEXTITEM\n");
      return 0;

    case TVM_SELECTITEM:
      FIXME (treeview, "Unimplemented msg TVM_SELECTITEM \n");
      return 0;

    case TVM_GETITEMA:
      FIXME (treeview, "Unimplemented msg TVM_GETITEMA\n");
      return 0;

    case TVM_GETITEMW:
      FIXME (treeview, "Unimplemented msg TVM_GETITEMW\n");
      return 0;

    case TVM_SETITEMA:
      FIXME (treeview, "Unimplemented msg TVM_SETITEMA\n");
      return 0;

    case TVM_SETITEMW:
      FIXME (treeview, "Unimplemented msg TVM_SETITEMW\n");
      return 0;

    case TVM_EDITLABELA:
      FIXME (treeview, "Unimplemented msg TVM_EDITLABELA \n");
      return 0;

    case TVM_EDITLABELW:
      FIXME (treeview, "Unimplemented msg TVM_EDITLABELW \n");
      return 0;

    case TVM_GETEDITCONTROL:
      FIXME (treeview, "Unimplemented msg TVM_GETEDITCONTROL\n");
      return 0;

    case TVM_GETVISIBLECOUNT:
      FIXME (treeview, "Unimplemented msg TVM_GETVISIBLECOUNT\n");
      return 0;

    case TVM_HITTEST:
      FIXME (treeview, "Unimplemented msg TVM_HITTEST\n");
      return 0;

    case TVM_CREATEDRAGIMAGE:
      FIXME (treeview, "Unimplemented msg TVM_CREATEDRAGIMAGE\n");
      return 0;

    case TVM_SORTCHILDREN:
      FIXME (treeview, "Unimplemented msg TVM_SORTCHILDREN\n");
      return 0;

    case TVM_ENSUREVISIBLE:
      FIXME (treeview, "Unimplemented msg TVM_ENSUREVISIBLE\n");
      return 0;

    case TVM_SORTCHILDRENCB:
      FIXME (treeview, "Unimplemented msg TVM_SORTCHILDRENCB\n");
      return 0;

    case TVM_ENDEDITLABELNOW:
      FIXME (treeview, "Unimplemented msg TVM_ENDEDITLABELNOW\n");
      return 0;

    case TVM_GETISEARCHSTRINGA:
      FIXME (treeview, "Unimplemented msg TVM_GETISEARCHSTRINGA\n");
      return 0;

    case TVM_GETISEARCHSTRINGW:
      FIXME (treeview, "Unimplemented msg TVM_GETISEARCHSTRINGW\n");
      return 0;

    case TVM_SETTOOLTIPS:
      FIXME (treeview, "Unimplemented msg TVM_SETTOOLTIPS\n");
      return 0;

    case TVM_GETTOOLTIPS:
      FIXME (treeview, "Unimplemented msg TVM_GETTOOLTIPS\n");
      return 0;

	case WM_CREATE:
	    return TREEVIEW_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return TREEVIEW_Destroy (wndPtr, wParam, lParam);

//	case EM_ENABLE:

	case WM_ERASEBKGND:
	    return TREEVIEW_EraseBackground (wndPtr, wParam, lParam);

	case WM_GETDLGCODE:
	    return DLGC_WANTARROWS | DLGC_WANTCHARS;

//	case WM_PAINT:
//	    return TREEVIEW_Paint (wndPtr, wParam);

//	case WM_SETFONT:

//	case WM_TIMER:

//	case WM_VSCROLL:

	default:
	    if (uMsg >= WM_USER)
	FIXME (treeview, "Unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void
TREEVIEW_Register (void)
{
    WNDCLASS32A wndClass;

    TRACE (treeview,"\n");

    if (GlobalFindAtom32A (WC_TREEVIEW32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC32)TREEVIEW_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TREEVIEW_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = WC_TREEVIEW32A;
 
    RegisterClass32A (&wndClass);
}

