/*
 * Windows widgets (built-in window classes)
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "windows.h"


static LONG WIDGETS_ButtonWndProc( HWND hwnd, WORD message,
				   WORD wParam, LONG lParam );
static LONG WIDGETS_StaticWndProc( HWND hwnd, WORD message,
				   WORD wParam, LONG lParam );

#define NB_BUILTIN_CLASSES  2

static WNDCLASS WIDGETS_BuiltinClasses[NB_BUILTIN_CLASSES] =
{
    { 0, WIDGETS_ButtonWndProc, 0, 0, 0, 0, 0, 0, NULL, "BUTTON" },
    { 0, WIDGETS_StaticWndProc, 0, 0, 0, 0, 0, 0, NULL, "STATIC" }
};

static FARPROC WndProc32[NB_BUILTIN_CLASSES];


/***********************************************************************
 *           WIDGETS_Init
 * 
 * Initialize the built-in window classes.
 */
BOOL WIDGETS_Init()
{
    int i;
    WNDCLASS * pClass = WIDGETS_BuiltinClasses;
        
    for (i = 0; i < NB_BUILTIN_CLASSES; i++, pClass++)
    {
	WndProc32[i] = pClass->lpfnWndProc;
	pClass->lpfnWndProc = (FARPROC) i+1;
	if (!RegisterClass(pClass)) return FALSE;
    }
    return TRUE;
}


/**********************************************************************
 *	     WIDGETS_Call32WndProc
 *
 * Call the window procedure of a built-in class.
 */
LONG WIDGETS_Call32WndProc( FARPROC func, HWND hwnd, WORD message,
			    WORD wParam, LONG lParam )
{
    unsigned int i = (unsigned int) func;
    if (!i || (i > NB_BUILTIN_CLASSES)) return 0;    
    return (*WndProc32[i-1])( hwnd, message, wParam, lParam );
}


/***********************************************************************
 *           WIDGETS_ButtonWndProc
 */
static LONG WIDGETS_ButtonWndProc( HWND hwnd, WORD message,
				   WORD wParam, LONG lParam )
{    
    switch(message)
    {
    case WM_CREATE:
	return 0;
	
    case WM_PAINT:
	{
	    HDC hdc;
	    PAINTSTRUCT ps;
	    RECT rect;
	
	    hdc = BeginPaint( hwnd, &ps );
	    GetClientRect( hwnd, &rect );
	    DrawText(hdc, "Button", -1, &rect,
		     DT_SINGLELINE | DT_CENTER | DT_VCENTER );
	    EndPaint( hwnd, &ps );
	    return 0;
	}
	
    default:
	return DefWindowProc( hwnd, message, wParam, lParam );
    }
}


/***********************************************************************
 *           WIDGETS_StaticWndProc
 */
static LONG WIDGETS_StaticWndProc( HWND hwnd, WORD message,
				   WORD wParam, LONG lParam )
{    
    switch(message)
    {
    case WM_CREATE:
	return 0;
	
    case WM_PAINT:
	{
	    HDC hdc;
	    PAINTSTRUCT ps;
	    RECT rect;
	
	    hdc = BeginPaint( hwnd, &ps );
	    GetClientRect( hwnd, &rect );
	    DrawText(hdc, "Static", -1, &rect,
		     DT_SINGLELINE | DT_CENTER | DT_VCENTER );
	    EndPaint( hwnd, &ps );
	    return 0;
	}
	
    default:
	return DefWindowProc( hwnd, message, wParam, lParam );
    }
}
