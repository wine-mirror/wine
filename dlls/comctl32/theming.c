/*
 * Theming - Initialization
 *
 * Copyright (c) 2005 by Frank Richter
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "comctl32.h"
#include "uxtheme.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(theming);

typedef LRESULT (CALLBACK* THEMING_SUBCLASSPROC)(HWND, UINT, WPARAM, LPARAM,
    ULONG_PTR);

extern LRESULT CALLBACK THEMING_DialogSubclassProc (HWND, UINT, WPARAM, LPARAM,
                                                    ULONG_PTR) DECLSPEC_HIDDEN;
extern LRESULT CALLBACK THEMING_ScrollbarSubclassProc (HWND, UINT, WPARAM, LPARAM,
                                                       ULONG_PTR) DECLSPEC_HIDDEN;

static const WCHAR dialogClass[] = L"#32770";

static const struct ThemingSubclass
{
    const WCHAR* className;
    THEMING_SUBCLASSPROC subclassProc;
} subclasses[] = {
    /* Note: list must be sorted by class name */
    {dialogClass,          THEMING_DialogSubclassProc},
    {WC_SCROLLBARW,        THEMING_ScrollbarSubclassProc}
};

#define NUM_SUBCLASSES        (ARRAY_SIZE(subclasses))

static WNDPROC originalProcs[NUM_SUBCLASSES];
static ATOM atRefDataProp;
static ATOM atSubclassProp;

/* Generate a number of subclass window procs.
 * With a single proc alone, we can't really reliably find out the superclass,
 * so have one for each subclass. The subclass number is also stored in a prop
 * since it's needed by THEMING_CallOriginalClass(). Then, the subclass
 * proc and ref data are fetched and the proc called.
 */
#define MAKE_SUBCLASS_PROC(N)                                               \
static LRESULT CALLBACK subclass_proc ## N (HWND wnd, UINT msg,             \
                                            WPARAM wParam, LPARAM lParam)   \
{                                                                           \
    LRESULT result;                                                         \
    ULONG_PTR refData;                                                      \
    SetPropW (wnd, (LPCWSTR)MAKEINTATOM(atSubclassProp), (HANDLE)N);        \
    refData = (ULONG_PTR)GetPropW (wnd, (LPCWSTR)MAKEINTATOM(atRefDataProp)); \
    TRACE ("%d; (%p, %x, %lx, %lx, %lx)\n", N, wnd, msg, wParam, lParam,     \
        refData);                                                           \
    result = subclasses[N].subclassProc (wnd, msg, wParam, lParam, refData);\
    TRACE ("result = %lx\n", result);                                       \
    return result;                                                          \
}

MAKE_SUBCLASS_PROC(0)
MAKE_SUBCLASS_PROC(1)

static const WNDPROC subclassProcs[NUM_SUBCLASSES] = {
    subclass_proc0,
    subclass_proc1,
};

/***********************************************************************
 * THEMING_Initialize
 *
 * Register classes for standard controls that will shadow the system
 * classes.
 */
void THEMING_Initialize (void)
{
    unsigned int i;

    if (!IsThemeActive()) return;

    atSubclassProp = GlobalAddAtomW (L"CC32ThemingSubCl");
    atRefDataProp = GlobalAddAtomW (L"CC32ThemingData");

    for (i = 0; i < NUM_SUBCLASSES; i++)
    {
        WNDCLASSEXW class;

        class.cbSize = sizeof(class);
        if (!GetClassInfoExW (NULL, subclasses[i].className, &class))
        {
            ERR("Could not retrieve information for class %s\n",
                debugstr_w (subclasses[i].className));
            continue;
        }
        originalProcs[i] = class.lpfnWndProc;
        class.lpfnWndProc = subclassProcs[i];
        
        if (!class.lpfnWndProc)
        {
            ERR("Missing proc for class %s\n", 
                debugstr_w (subclasses[i].className));
            continue;
        }

        if (!RegisterClassExW (&class))
        {
            ERR("Could not re-register class %s: %x\n",
                debugstr_w (subclasses[i].className), GetLastError ());
        }
        else
        {
            TRACE("Re-registered class %s\n", 
                debugstr_w (subclasses[i].className));
        }
    }
}

/***********************************************************************
 * THEMING_Uninitialize
 *
 * Unregister shadow classes for standard controls.
 */
void THEMING_Uninitialize (void)
{
    unsigned int i;

    if (!atSubclassProp) return;  /* not initialized */

    for (i = 0; i < NUM_SUBCLASSES; i++)
    {
        UnregisterClassW (subclasses[i].className, NULL);
    }
}

/***********************************************************************
 * THEMING_CallOriginalClass
 *
 * Determines the original window proc and calls it.
 */
LRESULT THEMING_CallOriginalClass (HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR subclass = (INT_PTR)GetPropW (wnd, (LPCWSTR)MAKEINTATOM(atSubclassProp));
    WNDPROC oldProc = originalProcs[subclass];
    return CallWindowProcW (oldProc, wnd, msg, wParam, lParam);
}
