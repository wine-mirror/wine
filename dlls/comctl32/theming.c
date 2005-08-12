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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(theming);

typedef LRESULT (CALLBACK* THEMING_SUBCLASSPROC)(HWND, UINT, WPARAM, LPARAM,
    ULONG_PTR);

static const struct ThemingSubclass
{
    const WCHAR* className;
    THEMING_SUBCLASSPROC subclassProc;
} subclasses[] = {
    /* Note: list must be sorted by class name */
};

#define NUM_SUBCLASSES        (sizeof(subclasses)/sizeof(subclasses[0]))

static WNDPROC originalProcs[NUM_SUBCLASSES];
static ATOM atRefDataProp;
static ATOM atSubclassProp;

/***********************************************************************
 * THEMING_SubclassProc
 *
 * The window proc registered to the subclasses. Fetches the subclass
 * proc and ref data and call the proc.
 */
static LRESULT CALLBACK THEMING_SubclassProc (HWND wnd, UINT msg,
                                              WPARAM wParam, LPARAM lParam)
{
    int subclass = (int)GetPropW (wnd, MAKEINTATOMW (atSubclassProp));
    LRESULT result;
    ULONG_PTR refData;

    refData = (ULONG_PTR)GetPropW (wnd, MAKEINTATOMW (atRefDataProp));

    TRACE ("%d; (%p, %x, %x, %lx, %lx)", subclass, wnd, msg, wParam, lParam, 
        refData);
    result = subclasses[subclass].subclassProc (wnd, msg, wParam, lParam, refData);
    TRACE (" = %lx\n", result);
    return result;
}

/* Generate a number of subclass window procs.
 * With a single proc alone, we can't really reliably find out the superclass,
 * hence, the first time the subclass is called, these "stubs" are used which
 * just save the internal ID of the subclass.
 */
#define MAKE_SUBCLASS_STUB(N)                                               \
static LRESULT CALLBACK subclass_stub ## N (HWND wnd, UINT msg,             \
                                            WPARAM wParam, LPARAM lParam)   \
{                                                                           \
    SetPropW (wnd, MAKEINTATOMW (atSubclassProp), (HANDLE)N);               \
    SetWindowLongPtrW (wnd, GWLP_WNDPROC, (LONG_PTR)THEMING_SubclassProc);  \
    return THEMING_SubclassProc (wnd, msg, wParam, lParam);                 \
}

const static WNDPROC subclassStubs[NUM_SUBCLASSES] = {
};

/***********************************************************************
 * THEMING_Initialize
 *
 * Register classes for standard controls that will shadow the system
 * classes.
 */
void THEMING_Initialize (void)
{
    int i;
    static const WCHAR subclassPropName[] = 
        { 'C','C','3','2','T','h','e','m','i','n','g','S','u','b','C','l',0 };
    static const WCHAR refDataPropName[] = 
        { 'C','C','3','2','T','h','e','m','i','n','g','D','a','t','a',0 };

    atSubclassProp = GlobalAddAtomW (subclassPropName);
    atRefDataProp = GlobalAddAtomW (refDataPropName);

    for (i = 0; i < NUM_SUBCLASSES; i++)
    {
        WNDCLASSEXW class;

        class.cbSize = sizeof(class);
        class.style |= CS_GLOBALCLASS;
        GetClassInfoExW (NULL, subclasses[i].className, &class);
        originalProcs[i] = class.lpfnWndProc;
        class.lpfnWndProc = subclassStubs[i];
        
        if (!class.lpfnWndProc)
        {
            ERR("Missing stub for class %s\n", 
                debugstr_w (subclasses[i].className));
            continue;
        }

        if (!RegisterClassExW (&class))
        {
            ERR("Could not re-register class %s: %lx\n", 
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
 * THEMING_CallOriginalClass
 *
 * Determines the original window proc and calls it.
 */
LRESULT THEMING_CallOriginalClass (HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int subclass = (int)GetPropW (wnd, MAKEINTATOMW (atSubclassProp));
    WNDPROC oldProc = originalProcs[subclass];
    return CallWindowProcW (oldProc, wnd, msg, wParam, lParam);
}

/***********************************************************************
 * THEMING_SetSubclassData
 *
 * Update the "refData" value of the subclassed window.
 */
void THEMING_SetSubclassData (HWND wnd, ULONG_PTR refData)
{
    SetPropW (wnd, MAKEINTATOMW (atRefDataProp), (HANDLE)refData);
}
