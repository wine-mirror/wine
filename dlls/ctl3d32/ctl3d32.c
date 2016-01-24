/*
 * CTL3D32 API stubs.
 *
 * Copyright (c) 2003 Dmitry Timoshkov
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

static BOOL CTL3D_is_auto_subclass = FALSE;

BOOL WINAPI Ctl3dAutoSubclass(HINSTANCE hInst)
{
    CTL3D_is_auto_subclass = TRUE;
    return TRUE;
}

BOOL WINAPI Ctl3dAutoSubclassEx(HINSTANCE hInst, DWORD type)
{
    CTL3D_is_auto_subclass = TRUE;
    return TRUE;
}

BOOL WINAPI Ctl3dColorChange(void)
{
    return TRUE;
}

HBRUSH WINAPI Ctl3dCtlColor(HDC hdc, HWND hwnd)
{
    return 0;
}

HBRUSH WINAPI Ctl3dCtlColorEx(UINT msg, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

LONG WINAPI Ctl3dDlgFramePaint(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

BOOL WINAPI Ctl3dEnabled(void)
{
    return FALSE;
}

WORD WINAPI Ctl3dGetVer(void)
{
    return MAKEWORD(31,2);
}

BOOL WINAPI Ctl3dIsAutoSubclass(void)
{
    return CTL3D_is_auto_subclass;
}

BOOL WINAPI Ctl3dRegister(HINSTANCE hInst)
{
    return TRUE;
}

BOOL WINAPI Ctl3dSubclassCtl(HWND hwnd)
{
    return FALSE;
}

BOOL WINAPI Ctl3dSubclassCtlEx(HWND hwnd, int type)
{
    return FALSE;
}

BOOL WINAPI Ctl3dSubclassDlg(HWND hwnd, WORD types)
{
    return FALSE;
}

BOOL WINAPI Ctl3dSubclassDlgEx(HWND hwnd, DWORD types)
{
    return FALSE;
}

BOOL WINAPI Ctl3dUnAutoSubclass(void)
{
    CTL3D_is_auto_subclass = FALSE;
    return FALSE;
}

BOOL WINAPI Ctl3dUnregister(HINSTANCE hInst)
{
    CTL3D_is_auto_subclass = FALSE;
    return TRUE;
}

BOOL WINAPI Ctl3dUnsubclassCtl(HWND hwnd)
{
    return FALSE;
}

void WINAPI Ctl3dWinIniChange(void)
{
}

/***********************************************************************
 *		ComboWndProc3d (CTL3D32.10)
 */
LRESULT WINAPI ComboWndProc3d(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
    return 0;
}

/***********************************************************************
 *		BtnWndProc3d (CTL3D32.7)
 */
LRESULT WINAPI BtnWndProc3d(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
    return 0;
}

/***********************************************************************
 *		StaticWndProc3d (CTL3D32.11)
 */
LRESULT WINAPI StaticWndProc3d(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
    return 0;
}

/***********************************************************************
 *		EditWndProc3d (CTL3D32.8)
 */
LRESULT WINAPI EditWndProc3d(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
    return 0;
}

/***********************************************************************
 *		ListWndProc3d (CTL3D32.9)
 */
LRESULT WINAPI ListWndProc3d(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
    return 0;
}

/***********************************************************************
 *		Ctl3dDlgProc (CTL3D32.17)
 */
LRESULT WINAPI Ctl3dDlgProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
    return 0;
}
