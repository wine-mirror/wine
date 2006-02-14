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

/***********************************************************************
 *		Ctl3dAutoSubclass (CTL3D.16)
 *		Ctl3dAutoSubclass (CTL3DV2.16)
 */
BOOL16 WINAPI Ctl3dAutoSubclass16(HINSTANCE16 hInst)
{
    CTL3D16_is_auto_subclass = TRUE;
    return TRUE;
}

/***********************************************************************
 *		Ctl3dAutoSubclassEx (CTL3D.27)
 *		Ctl3dAutoSubclassEx (CTL3DV2.27)
 */
BOOL16 WINAPI Ctl3dAutoSubclassEx16(HINSTANCE16 hInst, DWORD type)
{
    CTL3D16_is_auto_subclass = TRUE;
    return TRUE;
}

/***********************************************************************
 *		Ctl3dColorChange (CTL3D.6)
 *		Ctl3dColorChange (CTL3DV2.6)
 */
BOOL16 WINAPI Ctl3dColorChange16(void)
{
    return TRUE;
}

/***********************************************************************
 *		Ctl3dCtlColor (CTL3D.4)
 *		Ctl3dCtlColor (CTL3DV2.4)
 */
HBRUSH WINAPI Ctl3dCtlColor16(HDC16 hdc, LONG hwnd)
{
    return 0;
}

/***********************************************************************
 *		Ctl3dCtlColorEx (CTL3D.18)
 *		Ctl3dCtlColorEx (CTL3DV2.18)
 */
HBRUSH WINAPI Ctl3dCtlColorEx16(UINT16 msg, WPARAM16 wParam, LPARAM lParam)
{
    return 0;
}

/***********************************************************************
 *		Ctl3dDlgFramePaint (CTL3D.20)
 *		Ctl3dDlgFramePaint (CTL3DV2.20)
 */
LONG WINAPI Ctl3dDlgFramePaint16(HWND16 hwnd, UINT16 msg, WPARAM16 wParam, LPARAM lParam)
{
    return DefWindowProc16(hwnd, msg, wParam, lParam);
}

/***********************************************************************
 *		Ctl3dEnabled (CTL3D.5)
 *		Ctl3dEnabled (CTL3DV2.5)
 */
BOOL16 WINAPI Ctl3dEnabled16(void)
{
    return FALSE;
}

/***********************************************************************
 *		Ctl3dGetVer (CTL3D.1)
 *		Ctl3dGetVer (CTL3DV2.1)
 */
WORD WINAPI Ctl3dGetVer16(void)
{
    return MAKEWORD(31,2);
}

/***********************************************************************
 *		Ctl3dIsAutoSubclass (CTL3D.23)
 *		Ctl3dIsAutoSubclass (CTL3DV2.23)
 */
BOOL16 WINAPI Ctl3dIsAutoSubclass16(void)
{
    return CTL3D16_is_auto_subclass;
}

/***********************************************************************
 *		Ctl3dRegister (CTL3D.12)
 *		Ctl3dRegister (CTL3DV2.12)
 */
BOOL16 WINAPI Ctl3dRegister16(HINSTANCE16 hInst)
{
    return FALSE;
}

/***********************************************************************
 *		Ctl3dSubclassCtl (CTL3D.3)
 *		Ctl3dSubclassCtl (CTL3DV2.3)
 */
BOOL16 WINAPI Ctl3dSubclassCtl16(HWND16 hwnd)
{
    return FALSE;
}

/***********************************************************************
 *		Ctl3dSubclassCtlEx (CTL3D.25)
 *		Ctl3dSubclassCtlEx (CTL3DV2.25)
 */
BOOL16 WINAPI Ctl3dSubclassCtlEx16(HWND16 hwnd, INT16 type)
{
    return FALSE;
}

/***********************************************************************
 *		Ctl3dSubclassDlg (CTL3D.2)
 *		Ctl3dSubclassDlg (CTL3DV2.2)
 */
BOOL16 WINAPI Ctl3dSubclassDlg16(HWND16 hwnd, WORD types)
{
    return FALSE;
}

/***********************************************************************
 *		Ctl3dSubclassDlgEx (CTL3D.21)
 *		Ctl3dSubclassDlgEx (CTL3DV2.21)
 */
BOOL16 WINAPI Ctl3dSubclassDlgEx16(HWND16 hwnd, DWORD types)
{
    return FALSE;
}

/***********************************************************************
 *		Ctl3dUnAutoSubclass (CTL3D.24)
 *		Ctl3dUnAutoSubclass (CTL3DV2.24)
 */
BOOL16 WINAPI Ctl3dUnAutoSubclass16(void)
{
    CTL3D16_is_auto_subclass = FALSE;
    return FALSE;
}

/***********************************************************************
 *		Ctl3dUnregister (CTL3D.13)
 *		Ctl3dUnregister (CTL3DV2.13)
 */
BOOL16 WINAPI Ctl3dUnregister16(HINSTANCE16 hInst)
{
    CTL3D16_is_auto_subclass = FALSE;
    return TRUE;
}

/***********************************************************************
 *		Ctl3dUnsubclassCtl (CTL3D.26)
 *		Ctl3dUnsubclassCtl (CTL3DV2.26)
 */
BOOL16 WINAPI Ctl3dUnsubclassCtl16(HWND16 hwnd)
{
    return FALSE;
}

/***********************************************************************
 *		Ctl3dWinIniChange (CTL3D.22)
 *		Ctl3dWinIniChange (CTL3DV2.22)
 */
void WINAPI Ctl3dWinIniChange16(void)
{
}

/***********************************************************************
 *		ComboWndProc3d (CTL3D.10)
 *		ComboWndProc3d (CTL3DV2.10)
 */
LRESULT WINAPI ComboWndProc3d16(HWND16 hwnd, UINT16 msg,WPARAM16 wparam, LPARAM lparam)
{
    return 0;
}

/***********************************************************************
 *		BtnWndProc3d (CTL3D.7)
 *		BtnWndProc3d (CTL3DV2.7)
 */
LRESULT WINAPI BtnWndProc3d16(HWND16 hwnd, UINT16 msg, WPARAM16 wparam, LPARAM lparam)
{
    return 0;
}

/***********************************************************************
 *		StaticWndProc3d (CTL3D.11)
 *		StaticWndProc3d (CTL3DV2.11)
 */
LRESULT WINAPI StaticWndProc3d16(HWND16 hwnd, UINT16 msg, WPARAM16 wparam, LPARAM lparam)
{
    return 0;
}

/***********************************************************************
 *		EditWndProc3d (CTL3D.8)
 *		EditWndProc3d (CTL3DV2.8)
 */
LRESULT WINAPI EditWndProc3d16(HWND16 hwnd, UINT16 msg, WPARAM16 wparam, LPARAM lparam)
{
    return 0;
}

/***********************************************************************
 *		ListWndProc3d (CTL3D.9)
 *		ListWndProc3d (CTL3DV2.9)
 */
LRESULT WINAPI ListWndProc3d16(HWND16 hwnd, UINT16 msg, WPARAM16 wparam, LPARAM lparam)
{
    return 0;
}

/***********************************************************************
 *		Ctl3dDlgProc (CTL3D.17)
 *		Ctl3dDlgProc (CTL3DV2.17)
 */
LRESULT WINAPI Ctl3dDlgProc16(HWND16 hwnd, UINT16 msg, WPARAM16 wparam, LPARAM lparam)
{
    return 0;
}
