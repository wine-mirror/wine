/*
 * Windows widgets (built-in window classes)
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "win.h"
#include "button.h"
#include "static.h"
#include "scroll.h"
#include "desktop.h"
#include "mdi.h"
#include "gdi.h"
#include "user.h"
#include "module.h"
#include "heap.h"

static WNDCLASS16 WIDGETS_BuiltinClasses16[] =
{
    { CS_GLOBALCLASS | CS_PARENTDC,
          (WNDPROC16)"StaticWndProc", 0, sizeof(STATICINFO),
          0, 0, 0, 0, 0, (SEGPTR)"STATIC" },
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
          (WNDPROC16)"ScrollBarWndProc", 0, sizeof(SCROLLINFO),
          0, 0, 0, 0, 0, (SEGPTR)"SCROLLBAR" },
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS,
          (WNDPROC16)"ListBoxWndProc", 0, 8,
          0, 0, 0, 0, 0, (SEGPTR)"LISTBOX" },
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS,
          (WNDPROC16)"ComboBoxWndProc", 0, 8,
          0, 0, 0, 0, 0, (SEGPTR)"COMBOBOX" },
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS,
          (WNDPROC16)"ComboLBoxWndProc", 0, 8,
          0, 0, 0, 0, 0, (SEGPTR)"COMBOLBOX" },
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS,
          (WNDPROC16)"EditWndProc", 0, sizeof(DWORD),
          0, 0, 0, 0, 0, (SEGPTR)"EDIT" },
    { CS_GLOBALCLASS | CS_SAVEBITS, (WNDPROC16)"PopupMenuWndProc", 0, 8,
          0, 0, 0, 0, 0, (SEGPTR)POPUPMENU_CLASS_NAME },
    { CS_GLOBALCLASS | CS_SAVEBITS, (WNDPROC16)"DefDlgProc", 0, DLGWINDOWEXTRA,
          0, 0, 0, 0, 0, (SEGPTR)DIALOG_CLASS_NAME },
    { CS_GLOBALCLASS, (WNDPROC16)"MDIClientWndProc", 0, sizeof(MDICLIENTINFO),
          0, 0, 0, STOCK_LTGRAY_BRUSH, 0, (SEGPTR)"MDICLIENT" }
};

#define NB_BUILTIN_CLASSES16 \
         (sizeof(WIDGETS_BuiltinClasses16)/sizeof(WIDGETS_BuiltinClasses16[0]))

static WNDCLASS32A WIDGETS_BuiltinClasses32[] =
{
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
          ButtonWndProc, 0, sizeof(BUTTONINFO), 0, 0, 0, 0, 0, "BUTTON" },
    { CS_GLOBALCLASS, DesktopWndProc, 0, sizeof(DESKTOPINFO),
          0, 0, 0, 0, 0, DESKTOP_CLASS_NAME }
};

#define NB_BUILTIN_CLASSES32 \
         (sizeof(WIDGETS_BuiltinClasses32)/sizeof(WIDGETS_BuiltinClasses32[0]))


/***********************************************************************
 *           WIDGETS_Init
 * 
 * Initialize the built-in window classes.
 */
BOOL WIDGETS_Init(void)
{
    int i;
    char *name;
    WNDCLASS16 *class16  = WIDGETS_BuiltinClasses16;
    WNDCLASS32A *class32 = WIDGETS_BuiltinClasses32;

    if (!(name = SEGPTR_ALLOC( 20 * sizeof(char) ))) return FALSE;

    /* Create 16-bit classes */

    for (i = 0; i < NB_BUILTIN_CLASSES16; i++, class16++)
    {
        strcpy( name, (char *)class16->lpszClassName );
        class16->lpszClassName = SEGPTR_GET(name);
        class16->hCursor = LoadCursor( 0, IDC_ARROW );
        class16->lpfnWndProc = MODULE_GetWndProcEntry16( (char *)class16->lpfnWndProc );
        if (!RegisterClass16( class16 )) return FALSE;
    }

    /* Create 32-bit classes */

    for (i = 0; i < NB_BUILTIN_CLASSES32; i++, class32++)
    {
        /* Just to make sure the string is > 0x10000 */
        strcpy( name, (char *)class32->lpszClassName );
        class32->lpszClassName = name;
        class32->hCursor = LoadCursor( 0, IDC_ARROW );
        if (!RegisterClass32A( class32 )) return FALSE;
    }

    SEGPTR_FREE(name);
    return TRUE;
}
