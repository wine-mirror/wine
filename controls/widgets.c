/*
 * Windows widgets (built-in window classes)
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "win.h"


LONG ButtonWndProc( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG StaticWndProc( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG ScrollBarWndProc( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG ListBoxWndProc  ( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG ComboBoxWndProc ( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG PopupMenuWndProc ( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG DesktopWndProc ( HWND hwnd, WORD message, WORD wParam, LONG lParam );


static WNDCLASS WIDGETS_BuiltinClasses[] =
{
    { CS_GLOBALCLASS, (LONG(*)())ButtonWndProc, 0, 2, 
      0, 0, 0, 0, NULL, "BUTTON" },
    { CS_GLOBALCLASS, (LONG(*)())StaticWndProc, 0, 0,
      0, 0, 0, 0, NULL, "STATIC" },
    { CS_GLOBALCLASS, (LONG(*)())ScrollBarWndProc, 0, 8,
      0, 0, 0, 0, NULL, "SCROLLBAR" },
    { CS_GLOBALCLASS, (LONG(*)())ListBoxWndProc, 0, 8,
      0, 0, 0, 0, NULL, "LISTBOX" },
    { CS_GLOBALCLASS, (LONG(*)())ComboBoxWndProc, 0, 8,
      0, 0, 0, 0, NULL, "COMBOBOX" },
    { CS_GLOBALCLASS, (LONG(*)())PopupMenuWndProc, 0, 8,
      0, 0, 0, 0, NULL, "POPUPMENU" },
    { CS_GLOBALCLASS, (LONG(*)())DesktopWndProc, 0, 0,
      0, 0, 0, 0, NULL, DESKTOP_CLASS_NAME },
    { CS_GLOBALCLASS, (LONG(*)())DefDlgProc, 0, DLGWINDOWEXTRA,
      0, 0, 0, 0, NULL, DIALOG_CLASS_NAME }
};

#define NB_BUILTIN_CLASSES \
         (sizeof(WIDGETS_BuiltinClasses)/sizeof(WIDGETS_BuiltinClasses[0]))


/***********************************************************************
 *           WIDGETS_Init
 * 
 * Initialize the built-in window classes.
 */
BOOL WIDGETS_Init()
{
    int i;
    for (i = 0; i < NB_BUILTIN_CLASSES; i++)
    {
	if (!RegisterClass(&WIDGETS_BuiltinClasses[i])) return FALSE;
    }
    return TRUE;
}
