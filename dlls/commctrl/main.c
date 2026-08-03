/*
 * CE commctrl.dll
 *
 * Copyright 2010, 2014 André Hentschel
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
 */

#include "config.h"

#include <stdarg.h>

#include "windows.h"
#include "wine/debug.h"
#include "wine/ce.h"
#include "commctrl.h"

WINE_DEFAULT_DEBUG_CHANNEL(commctrl);

static HMODULE hcomctl32, huser32;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            hcomctl32 = LoadLibraryA("comctl32.dll");
            huser32 = LoadLibraryA("user32.dll");
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

BOOL WINAPI CommandBands_AddAdornments(HWND hwndCmdBands, HINSTANCE hInst, DWORD dwFlags, void*/*LPREBARBANDINFO*/ prbbi)
{
    /* FIXME: hack */
    return TRUE;
}

BOOL WINAPI CommandBands_AddBands(HWND hwndCmdBands, HINSTANCE hInst, UINT cBands, void*/*LPREBARBANDINFO*/ prbbi)
{
    /* FIXME: hack */
    return TRUE;
}

HWND WINAPI CommandBands_Create(HINSTANCE hInst, HWND hwndParent, UINT wID, DWORD dwStyles, HIMAGELIST himl)
{
    /* FIXME: hack */
    return hwndParent;
}

HWND WINAPI CommandBands_GetCommandBar(HWND hwndCmdBands, UINT uBand)
{
    /* FIXME: hack */
    return hwndCmdBands;
}

BOOL WINAPI CommandBands_Show(HWND hwndCmdBands, BOOL fShow)
{
    /* FIXME: hack */
    return TRUE;
}

BOOL WINAPI CommandBar_AddAdornments(HWND hwndCB, DWORD dwFlags, DWORD dwReserved)
{
    /* FIXME: hack */
    return TRUE;
}

int WINAPI CommandBar_AddBitmap(HWND hwndCB, HINSTANCE hInst, int idBitmap, int iNumImages, int iImageWidth, int iImageHeight)
{
    /* FIXME: hack */
    return 1;
}

HWND WINAPI CommandBar_Create(HINSTANCE hInst, HWND hwndParent, int idCmdBar)
{
    /* FIXME: hack */
    return hwndParent;
}

BOOL WINAPI CommandBar_InsertMenubarEx(HWND hwndCB, HINSTANCE hInst, LPSTR pszMenu, WORD iButton)
{
    /* FIXME: hack */
    return TRUE;
}

BOOL WINAPI CommandBar_InsertMenubar(HWND hwndCB, HINSTANCE hInst, WORD idMenu, WORD iButton)
{
    /* FIXME: hack */
    return TRUE;
}

FIXFUNC0(comctl32, InitCommonControls, InitCommonControls)
FIXFUNC1(comctl32, InitCommonControlsEx, InitCommonControlsEx)
FIXFUNC12(comctl32, CreateUpDownControl, CreateUpDownControl)
FIXFUNC13(comctl32, CreateToolbarEx, CreateToolbarEx)
FIXFUNC4(comctl32, CreateStatusWindowW, CreateStatusWindowW)
FIXFUNC1(comctl32, PropertySheetW, PropertySheetW)
FIXFUNC1(comctl32, CreatePropertySheetPageW, CreatePropertySheetPageW)
FIXFUNC1(comctl32, DestroyPropertySheetPage, DestroyPropertySheetPage)
FIXFUNC2(user32, InvertRect, InvertRect)
FIXFUNC2(comctl32, Str_SetPtrW, Str_SetPtrW)
