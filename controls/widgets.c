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
#include "selectors.h"
#include "stackframe.h"
#include "alias.h"
#include "relay32.h"

static WNDCLASS WIDGETS_BuiltinClasses[] =
{
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
          (WNDPROC)"ButtonWndProc", 0, sizeof(BUTTONINFO),
          0, 0, 0, 0, 0, (SEGPTR)"BUTTON" },
    { CS_GLOBALCLASS | CS_PARENTDC,
          (WNDPROC)"StaticWndProc", 0, sizeof(STATICINFO),
          0, 0, 0, 0, 0, (SEGPTR)"STATIC" },
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
          (WNDPROC)"ScrollBarWndProc", 0, sizeof(SCROLLINFO),
          0, 0, 0, 0, 0, (SEGPTR)"SCROLLBAR" },
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS,
          (WNDPROC)"ListBoxWndProc", 0, 8,
          0, 0, 0, 0, 0, (SEGPTR)"LISTBOX" },
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS,
          (WNDPROC)"ComboBoxWndProc", 0, 8,
          0, 0, 0, 0, 0, (SEGPTR)"COMBOBOX" },
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS,
          (WNDPROC)"ComboLBoxWndProc", 0, 8,
          0, 0, 0, 0, 0, (SEGPTR)"COMBOLBOX" },
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS,
          (WNDPROC)"EditWndProc", 0, sizeof(DWORD),
          0, 0, 0, 0, 0, (SEGPTR)"EDIT" },
    { CS_GLOBALCLASS | CS_SAVEBITS, (WNDPROC)"PopupMenuWndProc", 0, 8,
          0, 0, 0, 0, 0, (SEGPTR)POPUPMENU_CLASS_NAME },
    { CS_GLOBALCLASS, (WNDPROC)"DesktopWndProc", 0, sizeof(DESKTOPINFO),
          0, 0, 0, 0, 0, (SEGPTR)DESKTOP_CLASS_NAME },
    { CS_GLOBALCLASS | CS_SAVEBITS, (WNDPROC)"DefDlgProc", 0, DLGWINDOWEXTRA,
          0, 0, 0, 0, 0, (SEGPTR)DIALOG_CLASS_NAME },
    { CS_GLOBALCLASS, (WNDPROC)"MDIClientWndProc", 0, sizeof(MDICLIENTINFO),
          0, 0, 0, STOCK_LTGRAY_BRUSH, 0, (SEGPTR)"MDICLIENT" }
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
    char name[20];
    WNDCLASS *class = WIDGETS_BuiltinClasses;

    for (i = 0; i < NB_BUILTIN_CLASSES; i++, class++)
    {
        DWORD WineProc,Win16Proc,Win32Proc;
		WIN32_builtin *dll;
        /* currently, there is no way to get the 'real' pointer at run time */
        WineProc=0;
        Win16Proc = (DWORD)GetWndProcEntry16( (char *)class->lpfnWndProc );
		dll = RELAY32_GetBuiltinDLL("WINPROCS32");
        Win32Proc = (DWORD)RELAY32_GetEntryPoint(
			dll,(char *)class->lpfnWndProc, 0);
        /* Register the alias so we don't pass Win16 pointers to Win32 apps */
        ALIAS_RegisterAlias(WineProc,Win16Proc,Win32Proc);

        strcpy( name, (char *)class->lpszClassName );
        class->lpszClassName = MAKE_SEGPTR(name);
        class->hCursor = LoadCursor( 0, IDC_ARROW );
        class->lpfnWndProc = GetWndProcEntry16( (char *)class->lpfnWndProc );
        if (!RegisterClass( class )) return FALSE;
    }
    return TRUE;
}
