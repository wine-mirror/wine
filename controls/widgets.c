/*
 * Windows widgets (built-in window classes)
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "windows.h"
#include "win.h"
#include "dialog.h"


LONG ButtonWndProc( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG StaticWndProc( HWND hwnd, WORD message, WORD wParam, LONG lParam );

LONG SCROLLBAR_ScrollBarWndProc( HWND hwnd, WORD message,
				   WORD wParam, LONG lParam );
LONG LISTBOX_ListBoxWndProc( HWND hwnd, WORD message,
				   WORD wParam, LONG lParam );
LONG COMBOBOX_ComboBoxWndProc( HWND hwnd, WORD message,
				   WORD wParam, LONG lParam );


static WNDCLASS WIDGETS_BuiltinClasses[] =
{
    { 0, (LONG(*)())ButtonWndProc, 0, 2, 0, 0, 0, 0, NULL, "BUTTON" },
    { 0, (LONG(*)())StaticWndProc, 0, 0, 0, 0, 0, 0, NULL, "STATIC" },
    { 0, (LONG(*)())SCROLLBAR_ScrollBarWndProc, 0, 8, 0, 0, 0, 0, NULL, "SCROLLBAR" },
    { 0, (LONG(*)())LISTBOX_ListBoxWndProc, 0, 8, 0, 0, 0, 0, NULL, "LISTBOX" },
    { 0, (LONG(*)())COMBOBOX_ComboBoxWndProc, 0, 8, 0, 0, 0, 0, NULL, "COMBOBOX" },
    { 0, (LONG(*)())DefDlgProc, 0, DLGWINDOWEXTRA, 0, 0, 0, 0, NULL, DIALOG_CLASS_NAME }
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
