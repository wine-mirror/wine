/*
 * Windows widgets (built-in window classes)
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <string.h>
#include <assert.h>

#include "win.h"
#include "button.h"
#include "static.h"
#include "scroll.h"
#include "desktop.h"
#include "mdi.h"
#include "gdi.h"
#include "module.h"
#include "heap.h"

/* Window procedures */

extern LRESULT WINAPI EditWndProc( HWND hwnd, UINT msg,
                                   WPARAM wParam, LPARAM lParam );
extern LRESULT WINAPI ComboWndProc( HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam );
extern LRESULT WINAPI ComboLBWndProc( HWND hwnd, UINT msg,
                                      WPARAM wParam, LPARAM lParam );
extern LRESULT WINAPI ListBoxWndProc( HWND hwnd, UINT msg,
                                      WPARAM wParam, LPARAM lParam );
extern LRESULT WINAPI PopupMenuWndProc( HWND hwnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam );
extern LRESULT WINAPI IconTitleWndProc( HWND hwnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam );

/* Built-in classes */

static WNDCLASSA WIDGETS_BuiltinClasses[BIC32_NB_CLASSES] =
{
    /* BIC32_BUTTON */
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
      ButtonWndProc, 0, sizeof(BUTTONINFO), 0, 0,
      (HCURSOR)IDC_ARROWA, 0, 0, "Button" },
    /* BIC32_EDIT */
    { CS_GLOBALCLASS | CS_DBLCLKS /*| CS_PARENTDC*/,
      EditWndProc, 0, sizeof(void *), 0, 0,
      (HCURSOR)IDC_IBEAMA, 0, 0, "Edit" },
    /* BIC32_LISTBOX */
    { CS_GLOBALCLASS | CS_DBLCLKS /*| CS_PARENTDC*/,
      ListBoxWndProc, 0, sizeof(void *), 0, 0,
      (HCURSOR)IDC_ARROWA, 0, 0, "ListBox" },
    /* BIC32_COMBO */
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS, 
      ComboWndProc, 0, sizeof(void *), 0, 0,
      (HCURSOR)IDC_ARROWA, 0, 0, "ComboBox" },
    /* BIC32_COMBOLB */
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS, ComboLBWndProc,
      0, sizeof(void *), 0, 0, (HCURSOR)IDC_ARROWA, 0, 0, "ComboLBox" },
    /* BIC32_POPUPMENU */
    { CS_GLOBALCLASS | CS_SAVEBITS, PopupMenuWndProc, 0, sizeof(HMENU),
      0, 0, (HCURSOR)IDC_ARROWA, NULL_BRUSH, 0, POPUPMENU_CLASS_NAME },
    /* BIC32_STATIC */
    { CS_GLOBALCLASS | CS_PARENTDC, StaticWndProc,
      0, sizeof(STATICINFO), 0, 0, (HCURSOR)IDC_ARROWA, 0, 0, "Static" },
    /* BIC32_SCROLL */
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
      ScrollBarWndProc, 0, sizeof(SCROLLBAR_INFO), 0, 0,
      (HCURSOR)IDC_ARROWA, 0, 0, "ScrollBar"},
    /* BIC32_MDICLIENT */
    { CS_GLOBALCLASS, MDIClientWndProc,
      0, sizeof(MDICLIENTINFO), 0, 0, 0, STOCK_LTGRAY_BRUSH, 0, "MDIClient" },
    /* BIC32_DESKTOP */
    { CS_GLOBALCLASS, DesktopWndProc, 0, sizeof(DESKTOP),
      0, 0, (HCURSOR)IDC_ARROWA, 0, 0, DESKTOP_CLASS_NAME },
    /* BIC32_DIALOG */
    { CS_GLOBALCLASS | CS_SAVEBITS, DefDlgProcA, 0, DLGWINDOWEXTRA,
      0, 0, (HCURSOR)IDC_ARROWA, 0, 0, DIALOG_CLASS_NAME },
    /* BIC32_ICONTITLE */
    { CS_GLOBALCLASS, IconTitleWndProc, 0, 0, 
      0, 0, (HCURSOR)IDC_ARROWA, 0, 0, ICONTITLE_CLASS_NAME }
};

static ATOM bicAtomTable[BIC32_NB_CLASSES];

/***********************************************************************
 *           WIDGETS_Init
 * 
 * Initialize the built-in window classes.
 */
BOOL WIDGETS_Init(void)
{
    int i;
    WNDCLASSA *cls = WIDGETS_BuiltinClasses;

    /* Create builtin classes */

    for (i = 0; i < BIC32_NB_CLASSES; i++, cls++)
    {
        char name[20];
        /* Just to make sure the string is > 0x10000 */
        strcpy( name, (char *)cls->lpszClassName );
        cls->lpszClassName = name;
        cls->hCursor = LoadCursorA( 0, (LPCSTR)cls->hCursor );
        if (!(bicAtomTable[i] = RegisterClassA( cls ))) return FALSE;
    }

    return TRUE;
}


/***********************************************************************
 *           WIDGETS_IsControl32
 *
 * Check whether pWnd is a built-in control or not.
 */
BOOL	WIDGETS_IsControl( WND* pWnd, BUILTIN_CLASS32 cls )
{
    assert( cls < BIC32_NB_CLASSES );
    return (GetClassWord(pWnd->hwndSelf, GCW_ATOM) == bicAtomTable[cls]);
}
