/*
 * 16-bit CTL3D and CTL3DV2 API stubs.
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

#include "wine/winbase16.h"
#include "wine/winuser16.h"

static BOOL16 CTL3D16_is_auto_subclass = FALSE;

BOOL16 WINAPI Ctl3dAutoSubclass16(HINSTANCE16 hInst)
{
    CTL3D16_is_auto_subclass = TRUE;
    return TRUE;
}

BOOL16 WINAPI Ctl3dAutoSubclassEx16(HINSTANCE16 hInst, DWORD type)
{
    CTL3D16_is_auto_subclass = TRUE;
    return TRUE;
}

BOOL16 WINAPI Ctl3dColorChange16(void)
{
    return TRUE;
}

HBRUSH WINAPI Ctl3dCtlColor16(HDC16 hdc, LONG hwnd)
{
    return 0;
}

HBRUSH WINAPI Ctl3dCtlColorEx16(UINT16 msg, WPARAM16 wParam, LPARAM lParam)
{
    return 0;
}

LONG WINAPI Ctl3dDlgFramePaint16(HWND16 hwnd, UINT16 msg, WPARAM16 wParam, LPARAM lParam)
{
    return DefWindowProc16(hwnd, msg, wParam, lParam);
}

BOOL16 WINAPI Ctl3dEnabled16(void)
{
    return FALSE;
}

WORD WINAPI Ctl3dGetVer16(void)
{
    return MAKEWORD(31,2);
}

BOOL16 WINAPI Ctl3dIsAutoSubclass16(void)
{
    return CTL3D16_is_auto_subclass;
}

BOOL16 WINAPI Ctl3dRegister16(HINSTANCE16 hInst)
{
    return FALSE;
}

BOOL16 WINAPI Ctl3dSubclassCtl16(HWND16 hwnd)
{
    return FALSE;
}

BOOL16 WINAPI Ctl3dSubclassCtlEx16(HWND16 hwnd, INT16 type)
{
    return FALSE;
}

BOOL16 WINAPI Ctl3dSubclassDlg16(HWND16 hwnd, WORD types)
{
    return FALSE;
}

BOOL16 WINAPI Ctl3dSubclassDlgEx16(HWND16 hwnd, DWORD types)
{
    return FALSE;
}

BOOL16 WINAPI Ctl3dUnAutoSubclass16(void)
{
    CTL3D16_is_auto_subclass = FALSE;
    return FALSE;
}

BOOL16 WINAPI Ctl3dUnregister16(HINSTANCE16 hInst)
{
    CTL3D16_is_auto_subclass = FALSE;
    return TRUE;
}

BOOL16 WINAPI Ctl3dUnsubclassCtl16(HWND16 hwnd)
{
    return FALSE;
}

void WINAPI Ctl3dWinIniChange16(void)
{
}
