/*
 * Win32 WndProc function stubs
 *
 * Copyright 1995 Thomas Sandford (tdgsandf@prds-grn.demon.co.uk)
 *
 * These functions are simply lParam fixers for the Win16 routines
 */

#include <stdio.h>
#include "windows.h"

#ifndef WINELIB32

#include "winerror.h"
#include "kernel32.h"
#include "wintypes.h"
#include "struct32.h"
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

BOOL WIN32_CallWindowProcTo16(LRESULT(*func)(HWND,UINT,WPARAM,LPARAM),
	HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	WINDOWPOS wp;
	union{
		MINMAXINFO mmi;
		NCCALCSIZE_PARAMS nccs;
		CREATESTRUCT cs;
	} st;
	WINDOWPOS32 *pwp;
	CREATESTRUCT32 *pcs;
	LONG result;
	if(!lParam || !UsesLParamPtr(msg))
		return func(hwnd,msg,wParam,lParam);
	switch(msg)
	{
		case WM_GETMINMAXINFO:
			STRUCT32_MINMAXINFO32to16((void*)lParam,&st.mmi);
			result=func(hwnd,msg,wParam,MAKE_SEGPTR(&st.mmi));
			STRUCT32_MINMAXINFO16to32(&st.mmi,(void*)lParam);
			return result;
		case WM_WINDOWPOSCHANGING:
		case WM_WINDOWPOSCHANGED:
			STRUCT32_WINDOWPOS32to16((void*)lParam,&wp);
			result=func(hwnd,msg,wParam,MAKE_SEGPTR(&wp));
			STRUCT32_WINDOWPOS16to32(&wp,(void*)lParam);
			return result;
		 case WM_NCCALCSIZE:
		 	pwp=((NCCALCSIZE_PARAMS32*)lParam)->lppos;
		 	STRUCT32_NCCALCSIZE32to16Flat((void*)lParam,&st.nccs);
			if(pwp) {
				STRUCT32_WINDOWPOS32to16(pwp,&wp);
				st.nccs.lppos = &wp;
			}else
				st.nccs.lppos = 0;
			result=func(hwnd,msg,wParam,MAKE_SEGPTR(&st.nccs));
			STRUCT32_NCCALCSIZE16to32Flat(&st.nccs,(void*)lParam);
			if(pwp)
				STRUCT32_WINDOWPOS16to32(&wp,pwp);
			return result;
		case WM_NCCREATE:
			pcs = (CREATESTRUCT32*)lParam;
			STRUCT32_CREATESTRUCT32to16((void*)lParam,&st.cs);
			st.cs.lpszName = HIWORD(pcs->lpszName) ? 
				MAKE_SEGPTR(pcs->lpszName) : pcs->lpszName;
			st.cs.lpszClass = HIWORD(pcs->lpszClass) ? 
				MAKE_SEGPTR(pcs->lpszClass) : pcs->lpszClass;
			result=func(hwnd,msg,wParam,MAKE_SEGPTR(&st.cs));
			STRUCT32_CREATESTRUCT16to32(&st.cs,(void*)lParam);
			pcs->lpszName = HIWORD(pcs->lpszName) ? 
				PTR_SEG_TO_LIN(st.cs.lpszName) : pcs->lpszName;
			pcs->lpszClass = HIWORD(pcs-> lpszClass) ? 
				PTR_SEG_TO_LIN(st.cs.lpszClass) : pcs-> lpszClass;
			return result;
		case WM_GETTEXT:
		case WM_SETTEXT:
			return func(hwnd,msg,wParam,MAKE_SEGPTR((void*)lParam));
		default:
			fprintf(stderr,"No support for 32-16 msg 0x%x\n",msg);
	}
	return func(hwnd,msg,wParam,MAKE_SEGPTR((void*)lParam));
}


extern LRESULT AboutDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT ButtonWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT ColorDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT ComboBoxWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT DesktopWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT EditWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT FileOpenDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT FileSaveDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT FindTextDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT ListBoxWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT MDIClientWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT PopupMenuWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT PrintDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT PrintSetupDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT ReplaceTextDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT ScrollBarWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT StaticWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT SystemMessageBoxProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT ComboLBoxWndProc(HWND,UINT,WPARAM,LPARAM);

LRESULT USER32_DefWindowProcA(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(DefWindowProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT ButtonWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(ButtonWndProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT StaticWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(StaticWndProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT ScrollBarWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(ScrollBarWndProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT ListBoxWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(ListBoxWndProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT ComboBoxWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(ComboBoxWndProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT EditWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(EditWndProc,(HWND)hwnd, msg, wParam,lParam);
}

LRESULT PopupMenuWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(PopupMenuWndProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT DesktopWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(DesktopWndProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT DefDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(DefDlgProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT MDIClientWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(MDIClientWndProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT DefWindowProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(DefWindowProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT DefMDIChildProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(DefMDIChildProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT SystemMessageBoxProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(SystemMessageBoxProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT FileOpenDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(FileOpenDlgProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT FileSaveDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(FileSaveDlgProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT ColorDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(ColorDlgProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT FindTextDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(FindTextDlgProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT ReplaceTextDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(ReplaceTextDlgProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT PrintSetupDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(PrintSetupDlgProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT PrintDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(PrintDlgProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT AboutDlgProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(AboutDlgProc,(HWND)hwnd, msg, wParam, lParam);
}

LRESULT ComboLBoxWndProc32(DWORD hwnd, DWORD msg, DWORD wParam, DWORD lParam)

{
	return WIN32_CallWindowProcTo16(ComboLBoxWndProc,(HWND)hwnd, msg, wParam, lParam);
}
#endif
