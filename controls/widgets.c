/*
 * Windows widgets (built-in window classes)
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "win.h"
#include "commctrl.h"
#include "button.h"
#include "static.h"
#include "status.h"
#include "scroll.h"
#include "desktop.h"
#include "mdi.h"
#include "gdi.h"
#include "module.h"
#include "heap.h"

typedef struct
{
    UINT16     style;
    INT16      wndExtra;
    HBRUSH16   background;
    LPCSTR     procName;
    LPCSTR     className;
} BUILTIN_CLASS_INFO16;

static const BUILTIN_CLASS_INFO16 WIDGETS_BuiltinClasses16[] =
{
    { CS_GLOBALCLASS | CS_PARENTDC,
       sizeof(STATICINFO), 0, "StaticWndProc", "STATIC" },
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
      sizeof(SCROLLBAR_INFO), 0, "ScrollBarWndProc", "SCROLLBAR" },
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS,
      8, 0, "ListBoxWndProc", "LISTBOX" },
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS,
      8, 0, "ComboBoxWndProc", "COMBOBOX" },
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS,
      8, 0, "ComboLBoxWndProc", "COMBOLBOX" },
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS,
      sizeof(DWORD), 0, "EditWndProc", "EDIT" },
    { CS_GLOBALCLASS | CS_SAVEBITS,
      sizeof(HMENU32), 0, "PopupMenuWndProc", POPUPMENU_CLASS_NAME },
    { CS_GLOBALCLASS | CS_SAVEBITS,
      DLGWINDOWEXTRA, 0, "DefDlgProc", DIALOG_CLASS_NAME },
    { CS_GLOBALCLASS, sizeof(MDICLIENTINFO),
      STOCK_LTGRAY_BRUSH, "MDIClientWndProc", "MDICLIENT" }
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


static WNDCLASS32A WIDGETS_CommonControls32[] =
{
    { CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW, StatusWindowProc, 0,
      sizeof(STATUSWINDOWINFO), 0, 0, 0, 0, 0, STATUSCLASSNAME32A },
};

#define NB_COMMON_CONTROLS32 \
         (sizeof(WIDGETS_CommonControls32)/sizeof(WIDGETS_CommonControls32[0]))


/***********************************************************************
 *           WIDGETS_Init
 * 
 * Initialize the built-in window classes.
 */
BOOL WIDGETS_Init(void)
{
    int i;
    char *name;
    const BUILTIN_CLASS_INFO16 *info16 = WIDGETS_BuiltinClasses16;
    WNDCLASS16 class16;
    WNDCLASS32A *class32 = WIDGETS_BuiltinClasses32;

    if (!(name = SEGPTR_ALLOC( 20 * sizeof(char) ))) return FALSE;

    /* Create 16-bit classes */

    class16.cbClsExtra    = 0;
    class16.hInstance     = 0;
    class16.hIcon         = 0;
    class16.hCursor       = LoadCursor16( 0, IDC_ARROW );
    class16.lpszMenuName  = (SEGPTR)0;
    class16.lpszClassName = SEGPTR_GET(name);
    for (i = 0; i < NB_BUILTIN_CLASSES16; i++, info16++)
    {
        class16.style         = info16->style;
        class16.lpfnWndProc   = (WNDPROC16)MODULE_GetWndProcEntry16( info16->procName );
        class16.cbWndExtra    = info16->wndExtra;
        class16.hbrBackground = info16->background;
        strcpy( name, info16->className );
        if (!RegisterClass16( &class16 )) return FALSE;
    }

    /* Create 32-bit classes */

    for (i = 0; i < NB_BUILTIN_CLASSES32; i++, class32++)
    {
        /* Just to make sure the string is > 0x10000 */
        strcpy( name, (char *)class32->lpszClassName );
        class32->lpszClassName = name;
        class32->hCursor = LoadCursor16( 0, IDC_ARROW );
        if (!RegisterClass32A( class32 )) return FALSE;
    }

    SEGPTR_FREE(name);
    return TRUE;
}


/***********************************************************************
 *           InitCommonControls   (COMCTL32.15)
 */
void InitCommonControls(void)
{
    int i;
    char name[30];
    WNDCLASS32A *class32 = WIDGETS_CommonControls32;

    for (i = 0; i < NB_COMMON_CONTROLS32; i++, class32++)
    {
        /* Just to make sure the string is > 0x10000 */
        strcpy( name, (char *)class32->lpszClassName );
        class32->lpszClassName = name;
        class32->hCursor = LoadCursor16( 0, IDC_ARROW );
        RegisterClass32A( class32 );
    }
}
