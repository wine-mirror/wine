/*
 * Windows widgets (built-in window classes)
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <assert.h>

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

/* Window procedures */

extern LRESULT EditWndProc( HWND32 hwnd, UINT32 msg, WPARAM32 wParam,
                            LPARAM lParam );
extern LRESULT ComboWndProc( HWND32 hwnd, UINT32 msg, WPARAM32 wParam,
                             LPARAM lParam );
extern LRESULT ComboLBWndProc( HWND32 hwnd, UINT32 msg, WPARAM32 wParam,
                               LPARAM lParam );
extern LRESULT ListBoxWndProc( HWND32 hwnd, UINT32 msg, WPARAM32 wParam,
                               LPARAM lParam );
extern LRESULT PopupMenuWndProc( HWND32 hwnd, UINT32 msg, WPARAM32 wParam,
                                 LPARAM lParam );

/* Win16 class info */

typedef struct
{
    UINT16     style;
    INT16      wndExtra;
    HBRUSH16   background;
    LPCSTR     procName;
    LPCSTR     className;
} BUILTIN_CLASS_INFO16;

/* Win16 built-in classes */

static const BUILTIN_CLASS_INFO16 WIDGETS_BuiltinClasses16[] =
{
    { CS_GLOBALCLASS | CS_PARENTDC,
       sizeof(STATICINFO), 0, "StaticWndProc", "Static" },
    { CS_GLOBALCLASS, sizeof(MDICLIENTINFO),
      STOCK_LTGRAY_BRUSH, "MDIClientWndProc", "MDIClient" }
};

#define NB_BUILTIN_CLASSES16 \
         (sizeof(WIDGETS_BuiltinClasses16)/sizeof(WIDGETS_BuiltinClasses16[0]))

/* Win32 built-in classes */

static WNDCLASS32A WIDGETS_BuiltinClasses32[BIC32_NB_CLASSES] =
{
    /* BIC32_BUTTON */
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
      ButtonWndProc, 0, sizeof(BUTTONINFO), 0, 0, IDC_ARROW, 0, 0, "Button" },
    /* BIC32_EDIT */
    { CS_GLOBALCLASS | CS_DBLCLKS /*| CS_PARENTDC*/,
      EditWndProc, 0, sizeof(void *), 0, 0, IDC_IBEAM, 0, 0, "Edit" },
    /* BIC32_LISTBOX */
    { CS_GLOBALCLASS | CS_DBLCLKS /*| CS_PARENTDC*/,
      ListBoxWndProc, 0, sizeof(void *), 0, 0, IDC_ARROW, 0, 0, "ListBox" },
    /* BIC32_COMBO */
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS, 
      ComboWndProc, 0, sizeof(void *), 0, 0, IDC_ARROW, 0, 0, "ComboBox" },
    /* BIC32_COMBOLB */
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS,
      ComboLBWndProc, 0, sizeof(void *), 0, 0, IDC_ARROW, 0, 0, "ComboLBox" },
    /* BIC32_POPUPMENU */
    { CS_GLOBALCLASS | CS_SAVEBITS, PopupMenuWndProc,
      0, sizeof(HMENU32), 0, 0, IDC_ARROW, NULL_BRUSH, 0, POPUPMENU_CLASS_NAME },
    /* BIC32_SCROLL */
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
      ScrollBarWndProc, 0, sizeof(SCROLLBAR_INFO), 0, 0, IDC_ARROW, 0, 0, "ScrollBar"},
    /* BIC32_DESKTOP */
    { CS_GLOBALCLASS, DesktopWndProc, 0, sizeof(DESKTOPINFO),
      0, 0, IDC_ARROW, 0, 0, DESKTOP_CLASS_NAME },
    /* BIC32_DIALOG */
    { CS_GLOBALCLASS | CS_SAVEBITS, DefDlgProc32A, 0, DLGWINDOWEXTRA,
      0, 0, IDC_ARROW, 0, 0, DIALOG_CLASS_NAME }
};

static ATOM bicAtomTable[BIC32_NB_CLASSES];

/* Win32 common controls */

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
BOOL32 WIDGETS_Init(void)
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

    for (i = 0; i < BIC32_NB_CLASSES; i++, class32++)
    {
        /* Just to make sure the string is > 0x10000 */
        strcpy( name, (char *)class32->lpszClassName );
        class32->lpszClassName = name;
        class32->hCursor = LoadCursor16( 0, class32->hCursor );
        if (!(bicAtomTable[i] = RegisterClass32A( class32 ))) return FALSE;
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


/***********************************************************************
 *           WIDGETS_IsControl32
 *
 * Check whether pWnd is a built-in control or not.
 */
BOOL32	WIDGETS_IsControl32( WND* pWnd, BUILTIN_CLASS32 cls )
{
    assert( cls < BIC32_NB_CLASSES );
    return (pWnd->class->atomName == bicAtomTable[cls]);
}
