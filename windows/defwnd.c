/*
 * Default window procedure
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";


#include "windows.h"
#include "win.h"
#include "class.h"
#include "user.h"

extern Display * display;

extern LONG NC_HandleNCPaint( HWND hwnd, HRGN hrgn );
extern LONG NC_HandleNCCalcSize( HWND hwnd, NCCALCSIZE_PARAMS *params );
extern LONG NC_HandleNCHitTest( HWND hwnd, POINT pt );
extern LONG NC_HandleNCMouseMsg(HWND hwnd, WORD msg, WORD wParam, LONG lParam);


/***********************************************************************
 *           DefWindowProc   (USER.107)
 */
LONG DefWindowProc( HWND hwnd, WORD msg, WORD wParam, LONG lParam )
{
    CLASS * classPtr;
    LPSTR textPtr;
    int len;
    int tempwidth, tempheight;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    
#ifdef DEBUG_MESSAGE
    printf( "DefWindowProc: %d %d %d %08x\n", hwnd, msg, wParam, lParam );
#endif

    switch(msg)
    {
    case WM_NCCREATE:
	{
	    CREATESTRUCT * createStruct = (CREATESTRUCT *)lParam;
	    if (createStruct->lpszName)
	    {
		  /* Allocate space for window text */
		wndPtr->hText = USER_HEAP_ALLOC(GMEM_MOVEABLE, 
					strlen(createStruct->lpszName) + 2);
		textPtr = (LPSTR)USER_HEAP_ADDR(wndPtr->hText);
		strcpy(textPtr, createStruct->lpszName);
		*(textPtr + strlen(createStruct->lpszName) + 1) = '\0';
		                         /* for use by edit control */
	    }
	    return 1;
	}

    case WM_NCCALCSIZE:
	return NC_HandleNCCalcSize( hwnd, (NCCALCSIZE_PARAMS *)lParam );

    case WM_NCPAINT:
	return NC_HandleNCPaint( hwnd, (HRGN)wParam );

    case WM_NCHITTEST:
	return NC_HandleNCHitTest( hwnd, MAKEPOINT(lParam) );

    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCMOUSEMOVE:
	return NC_HandleNCMouseMsg( hwnd, msg, wParam, lParam );

    case WM_NCDESTROY:
	{
	    if (wndPtr->hText) USER_HEAP_FREE(wndPtr->hText);
	    wndPtr->hText = 0;
	    return 0;
	}
	
    case WM_PAINT:
	{
	    PAINTSTRUCT paintstruct;
	    BeginPaint( hwnd, &paintstruct );
	    EndPaint( hwnd, &paintstruct );
	    return 0;
	}

    case WM_CLOSE:
	DestroyWindow( hwnd );
	return 0;

    case WM_WINDOWPOSCHANGED:
	{
	    WINDOWPOS * winPos = (WINDOWPOS *)lParam;
	    if (!(winPos->flags & SWP_NOMOVE))
		SendMessage( hwnd, WM_MOVE, 0,
		             MAKELONG( wndPtr->rectClient.left,
				       wndPtr->rectClient.top ));
	    if (!(winPos->flags & SWP_NOSIZE))
		SendMessage( hwnd, WM_SIZE, SIZE_RESTORED,
		   MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
			    wndPtr->rectClient.bottom-wndPtr->rectClient.top));
	    return 0;
	}

    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
	{
	    if (!(classPtr = CLASS_FindClassPtr( wndPtr->hClass ))) return 1;
	    if (!classPtr->wc.hbrBackground) return 1;
	    FillWindow( GetParent(hwnd), hwnd, (HDC)wParam,
		        classPtr->wc.hbrBackground );
	    return 0;
	}

    case WM_GETDLGCODE:
	return 0;

    case WM_CTLCOLOR:
	{
	    if (HIWORD(lParam) == CTLCOLOR_SCROLLBAR)
	    {
		SetBkColor( (HDC)wParam, RGB(255, 255, 255) );
		SetTextColor( (HDC)wParam, RGB(0, 0, 0) );
/*	        hbr = sysClrObjects.hbrScrollbar;
		UnrealizeObject(hbr); */
		return GetStockObject(LTGRAY_BRUSH);
	    }
	    else
	    {
		SetBkColor( (HDC)wParam, GetSysColor(COLOR_WINDOW) );
		SetTextColor( (HDC)wParam, GetSysColor(COLOR_WINDOWTEXT) );
/*	        hbr = sysClrObjects.hbrWindow; */
		return GetStockObject(WHITE_BRUSH);
	    }
	}
	
    case WM_GETTEXT:
	{
	    if (wParam)
	    {
		if (wndPtr->hText)
		{
		    textPtr = (LPSTR)USER_HEAP_ADDR(wndPtr->hText);
		    if ((int)wParam > (len = strlen(textPtr)))
		    {
			strcpy((char *)lParam, textPtr);
			return (DWORD)len;
		    }
		}
	        lParam = (DWORD)NULL;
	    }
	    return (0L);
	}

    case WM_GETTEXTLENGTH:
	{
	    if (wndPtr->hText)
	    {
		textPtr = (LPSTR)USER_HEAP_ADDR(wndPtr->hText);
		return (DWORD)strlen(textPtr);
	    }
	    return (0L);
	}

    case WM_SETTEXT:
	{
	    if (wndPtr->hText)
		USER_HEAP_FREE(wndPtr->hText);

	    wndPtr->hText = USER_HEAP_ALLOC(GMEM_MOVEABLE, 
					    strlen((LPSTR)lParam) + 1);
	    textPtr = (LPSTR)USER_HEAP_ADDR(wndPtr->hText);
	    strcpy(textPtr, (LPSTR)lParam);
	    XStoreName( display, wndPtr->window, textPtr );
	    return (0L);
	}
    case WM_SETCURSOR:
	if (wndPtr->hCursor != (HCURSOR)NULL)
	    SetCursor(wndPtr->hCursor);
	return 0L;
    case WM_SYSCOMMAND:
	switch (wParam)
	    {
	    case SC_CLOSE:
		ShowWindow(hwnd, SW_MINIMIZE);
		printf("defdwndproc WM_SYSCOMMAND SC_CLOSE !\n");
	        return SendMessage( hwnd, WM_CLOSE, 0, 0 );
	    case SC_RESTORE:
		ShowWindow(hwnd, SW_RESTORE);
		break;
	    case SC_MINIMIZE:
		ShowWindow(hwnd, SW_MINIMIZE);
		printf("defdwndproc WM_SYSCOMMAND SC_MINIMIZE !\n");
		break;
	    case SC_MAXIMIZE:
		ShowWindow(hwnd, SW_MAXIMIZE);
		break;
	    }
    	break;    	
    case WM_SYSKEYDOWN:
    	if (wParam == VK_MENU) {
    	    printf("VK_MENU Pressed // hMenu=%04X !\n", GetMenu(hwnd));
    	    }
    	break;    	
    case WM_SYSKEYUP:
    	if (wParam == VK_MENU) {
    	    printf("VK_MENU Released // hMenu=%04X !\n", GetMenu(hwnd));
    	    }
    	break;    	
    }
    return 0;
}
