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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "winbase.h"
#include "winuser.h"

BOOL WINAPI Ctl3dAutoSubclass(HINSTANCE hInst)
{
    return TRUE;
}

BOOL WINAPI Ctl3dAutoSubclassEx(HINSTANCE hInst, DWORD type)
{
    return TRUE;
}

BOOL WINAPI Ctl3dColorChange(void)
{
    return TRUE;
}

HBRUSH WINAPI Ctl3dCtlColor(HDC hdc, LONG hwnd)
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
    return FALSE;
}

BOOL WINAPI Ctl3dRegister(HINSTANCE hInst)
{
    return FALSE;
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
    return FALSE;
}

BOOL WINAPI Ctl3dUnregister(HINSTANCE hInst)
{
    return TRUE;
}


BOOL WINAPI Ctl3dUnsubclassCtl(HWND hwnd)
{
    return FALSE;
}

void WINAPI Ctl3dWinIniChange(void)
{
}
