/*
 * Windows widgets (built-in window classes)
 *
 * Copyright 1993 Alexandre Julliard

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";
*/

#include "win.h"
#include "button.h"
#include "static.h"
#include "scroll.h"
#include "desktop.h"
#include "mdi.h"
#include "gdi.h"

LONG ListBoxWndProc  ( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG ComboBoxWndProc ( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG EditWndProc( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG PopupMenuWndProc ( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG DesktopWndProc ( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG MDIClientWndProc ( HWND hwnd, WORD message, WORD wParam, LONG lParam );


static WNDCLASS WIDGETS_BuiltinClasses[] =
{
    { CS_GLOBALCLASS | CS_PARENTDC, ButtonWndProc, 0, sizeof(BUTTONINFO), 
      0, 0, 0, 0, NULL, "BUTTON" },
    { CS_GLOBALCLASS | CS_PARENTDC, StaticWndProc, 0, sizeof(STATICINFO),
      0, 0, 0, 0, NULL, "STATIC" },
    { CS_GLOBALCLASS | CS_PARENTDC, ScrollBarWndProc, 0, sizeof(SCROLLINFO),
      0, 0, 0, 0, NULL, "SCROLLBAR" },
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS, ListBoxWndProc, 0, 8,
      0, 0, 0, 0, NULL, "LISTBOX" },
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS, ComboBoxWndProc, 0, 8,
      0, 0, 0, 0, NULL, "COMBOBOX" },
    { CS_GLOBALCLASS | CS_PARENTDC, EditWndProc, 0, 4, 
      0, 0, 0, 0, NULL, "EDIT" },
    { CS_GLOBALCLASS | CS_SAVEBITS, PopupMenuWndProc, 0, 8,
      0, 0, 0, 0, NULL, POPUPMENU_CLASS_NAME },
    { CS_GLOBALCLASS, DesktopWndProc, 0, sizeof(DESKTOPINFO),
      0, 0, 0, 0, NULL, DESKTOP_CLASS_NAME },
    { CS_GLOBALCLASS | CS_SAVEBITS, DefDlgProc, 0, DLGWINDOWEXTRA,
      0, 0, 0, 0, NULL, DIALOG_CLASS_NAME },
    { CS_GLOBALCLASS, MDIClientWndProc, 0, sizeof(MDICLIENTINFO),
      0, 0, 0, STOCK_LTGRAY_BRUSH, NULL, "MDICLIENT" }
};

#define NB_BUILTIN_CLASSES \
         (sizeof(WIDGETS_BuiltinClasses)/sizeof(WIDGETS_BuiltinClasses[0]))


/***********************************************************************
 *           WIDGETS_Init
 * 
 * Initialize the built-in window classes.
 */
BOOL WIDGETS_Init(void)
{
    int i;
    WNDCLASS *class = WIDGETS_BuiltinClasses;

    for (i = 0; i < NB_BUILTIN_CLASSES; i++, class++)
    {
	class->hCursor = LoadCursor( 0, IDC_ARROW );
	if (!RegisterClass( class )) return FALSE;
    }
    return TRUE;
}
