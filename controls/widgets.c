/*
 * Windows widgets (built-in window classes)
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>

#include "win.h"
#include "button.h"
#include "combo.h"
#include "desktop.h"
#include "gdi.h"
#include "heap.h"
#include "mdi.h"
#include "menu.h"
#include "scroll.h"
#include "static.h"
#include "wine/unicode.h"

struct builtin_class
{
    LPCSTR  name;
    UINT    style;
    WNDPROC procA;
    WNDPROC procW;
    INT     extra;
    LPCSTR  cursor;
    HBRUSH  brush;
};

/* Under NT all builtin classes have both ASCII and Unicode window
 * procedures except ScrollBar, PopupMenu, Desktop, WinSwitch and
 * IconTitle which are Unicode-only.
 */
static const struct builtin_class classes[] =
{
    { "Button", CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
      ButtonWndProcA, ButtonWndProcW, sizeof(BUTTONINFO), IDC_ARROWA, 0 },
    { "Edit", CS_GLOBALCLASS | CS_DBLCLKS /*| CS_PARENTDC*/,
      EditWndProc, NULL, sizeof(void *), IDC_IBEAMA, 0 },
    { "ListBox", CS_GLOBALCLASS | CS_DBLCLKS /*| CS_PARENTDC*/,
      ListBoxWndProc, NULL, sizeof(void *), IDC_ARROWA, 0 },
    { "ComboBox", CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS,
      ComboWndProc, NULL, sizeof(void *), IDC_ARROWA, 0 },
    { "ComboLBox", CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS,
      ComboLBWndProc, NULL, sizeof(void *), IDC_ARROWA, 0 },
    { "Static", CS_GLOBALCLASS | CS_DBLCLKS | CS_PARENTDC,
      StaticWndProc, NULL, sizeof(STATICINFO), IDC_ARROWA, 0 },
    { "ScrollBar", CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
      ScrollBarWndProc, NULL, sizeof(SCROLLBAR_INFO), IDC_ARROWA, 0 },
    { "MDIClient", CS_GLOBALCLASS,
      MDIClientWndProc, NULL, sizeof(MDICLIENTINFO), IDC_ARROWA, STOCK_LTGRAY_BRUSH },
    { POPUPMENU_CLASS_NAME, CS_GLOBALCLASS | CS_SAVEBITS,
      PopupMenuWndProc, NULL, sizeof(HMENU), IDC_ARROWA, COLOR_MENU+1 },
    { DESKTOP_CLASS_NAME, CS_GLOBALCLASS,
      DesktopWndProc, NULL, sizeof(DESKTOP), IDC_ARROWA, COLOR_BACKGROUND+1 },
    { DIALOG_CLASS_NAME, CS_GLOBALCLASS | CS_SAVEBITS,
      DefDlgProcA, DefDlgProcW, DLGWINDOWEXTRA, IDC_ARROWA, 0 },
    { ICONTITLE_CLASS_NAME, CS_GLOBALCLASS,
      IconTitleWndProc, NULL, 0, IDC_ARROWA, 0 }
};


/***********************************************************************
 *           WIDGETS_Init
 * 
 * Initialize the built-in window classes.
 */
BOOL WIDGETS_Init(void)
{
    const struct builtin_class *cls = classes;
    int i;

    for (i = 0; i < sizeof(classes)/sizeof(classes[0]); i++, cls++)
    {
        if (!CLASS_RegisterBuiltinClass( cls->name, cls->style, cls->extra, cls->cursor,
                                         cls->brush, cls->procA, cls->procW ))
            return FALSE;
    }
    return TRUE;
}
