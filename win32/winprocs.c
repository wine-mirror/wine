/*
 * Win32 WndProc function stubs
 *
 * Copyright 1995 Thomas Sandford (tdgsandf@prds-grn.demon.co.uk)
 *
 * These functions are simply lParam fixers for the Win16 routines
 */

#include <stdio.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"
#include "wincon.h"
#include "stackframe.h"
#include "stddebug.h"
#include "debug.h"

BOOL UsesLParamPtr(DWORD message)

{
    switch (message) {
	case WM_NCCREATE:
	case WM_NCCALCSIZE:
	case WM_WINDOWPOSCHANGING:
	case WM_WINDOWPOSCHANGED:
	case WM_GETTEXT:
	case WM_SETTEXT:
	case WM_GETMINMAXINFO:
	    return TRUE;
	default:
	    return FALSE;
    }
}

LRESULT USER32_DefWindowProcA(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return DefWindowProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return DefWindowProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT ButtonWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return ButtonWndProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return ButtonWndProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT StaticWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return StaticWndProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return StaticWndProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT ScrollBarWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return ScrollBarWndProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return ScrollBarWndProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT ListBoxWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return ListBoxWndProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return ListBoxWndProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT ComboBoxWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return ComboBoxWndProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return ComboBoxWndProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT EditWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return EditWndProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return EditWndProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT PopupMenuWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return PopupMenuWndProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return PopupMenuWndProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT DesktopWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return DesktopWndProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return DesktopWndProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT DefDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return DefDlgProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return DefDlgProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT MDIClientWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return MDIClientWndProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return MDIClientWndProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT DefWindowProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return DefWindowProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return DefWindowProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT DefMDIChildProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return DefMDIChildProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return DefMDIChildProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT SystemMessageBoxProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return SystemMessageBoxProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return SystemMessageBoxProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT FileOpenDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return FileOpenDlgProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return FileOpenDlgProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT FileSaveDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return FileSaveDlgProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return FileSaveDlgProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT ColorDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return ColorDlgProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return ColorDlgProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT FindTextDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return FindTextDlgProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return FindTextDlgProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT ReplaceTextDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return ReplaceTextDlgProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return ReplaceTextDlgProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT PrintSetupDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return PrintSetupDlgProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return PrintSetupDlgProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT PrintDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return PrintDlgProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return PrintDlgProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT AboutDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return AboutDlgProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return AboutDlgProc((HWND)hwnd, msg, wParam, lParam);
}

LRESULT ComboLBoxWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
    if (UsesLParamPtr(msg))
	return ComboLBoxWndProc((HWND)hwnd, msg, wParam, MAKE_SEGPTR((void *)lParam));
    else
	return ComboLBoxWndProc((HWND)hwnd, msg, wParam, lParam);
}

