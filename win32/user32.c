/*
 * Win32 user functions
 *
 * Copyright 1995 Martin von Loewis
 */

/* This file contains only wrappers to existing Wine functions or trivial
   stubs. 'Real' implementations go into context specific files */

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include "windows.h"
#include "winerror.h"
#include "stackframe.h"
#include "xmalloc.h"
#include "handle32.h"
#include "struct32.h"
#include "resource32.h"
#include "string32.h"
#include "dialog.h"
#include "win.h"
#include "winproc.h"
#include "debug.h"
#include "stddebug.h"

/***********************************************************************
 *          GetMessageA          (USER32.269)
 */
BOOL USER32_GetMessageA(MSG32* lpmsg,DWORD hwnd,DWORD min,DWORD max)
{
	BOOL ret;
	MSG msg;
	ret=GetMessage(MAKE_SEGPTR(&msg),(HWND)hwnd,min,max);
	STRUCT32_MSG16to32(&msg,lpmsg);
	return ret;
}

/***********************************************************************
 *         DispatchMessageA       (USER32.140)
 */
LONG USER32_DispatchMessageA(MSG32* lpmsg)
{
	MSG msg;
	LONG ret;
	STRUCT32_MSG32to16(lpmsg,&msg);
	ret=DispatchMessage(&msg);
	STRUCT32_MSG16to32(&msg,lpmsg);
	return ret;
}

/***********************************************************************
 *         TranslateMessage       (USER32.555)
 */
BOOL USER32_TranslateMessage(MSG32* lpmsg)
{
	MSG msg;
	STRUCT32_MSG32to16(lpmsg,&msg);
	return TranslateMessage(&msg);
}


UINT USER32_SetTimer(HWND hwnd, UINT id, UINT timeout, void *proc)

{
    dprintf_win32(stddeb, "USER32_SetTimer: %d %d %d %08lx\n", hwnd, id, timeout,
(LONG)proc );
    return SetTimer( hwnd, id, timeout, MAKE_SEGPTR(proc));
}   

/* WARNING: It has not been verified that the signature or semantics
   of the corresponding NT function is the same */

HWND USER32_CreateDialogIndirectParamAorW(HINSTANCE hInst,LPVOID templ,
	HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam,int A)
{
	DLGTEMPLATE32 *dlgTempl=templ;
	DLGITEMTEMPLATE32 *dlgitem;
	WORD *ptr;
	DWORD MenuName=0;
	DWORD ClassName=0;
	DWORD szFontName=0;
	WORD wPointSize;
	HFONT hFont=0;
	HMENU hMenu=0;
	DWORD exStyle;
	DWORD szCaption;
	RECT16 rect;
	DIALOGINFO *dlgInfo;
	WND *wndPtr;
	WORD xUnit = xBaseUnit;
	WORD yUnit = yBaseUnit;
	int i;
	DWORD ClassId;
	DWORD Text;
	HWND hwnd,hwndCtrl;
	HWND hwndDefButton=0;
	WCHAR buffer[200];

	/* parse the dialog template header*/
	exStyle = dlgTempl->dwExtendedStyle;
	ptr = (WORD*)(dlgTempl+1);
	switch(*ptr){
		case 0: MenuName=0;ptr++;break;
		case 0xFFFF: MenuName=*(ptr+1);ptr+=2;break;
		default: MenuName = (DWORD)ptr;
			ptr += STRING32_lstrlenW(ptr)+1;
	}
	switch(*ptr){ 
		case 0: ClassName = DIALOG_CLASS_ATOM;ptr++;break;
		case 0xFFFF: ClassName = *(ptr+1);ptr+=2;break;
		default: ClassName = (DWORD)ptr;
				ptr += STRING32_lstrlenW(ptr)+1;
	}
	szCaption=(DWORD)ptr;
	ptr+=STRING32_lstrlenW(ptr)+1;
	if(dlgTempl->style & DS_SETFONT)
	{
		wPointSize = *ptr;
		ptr++;
		szFontName = (DWORD)ptr;
		ptr+=STRING32_lstrlenW(ptr)+1;
	}
	
	if(MenuName) hMenu=WIN32_LoadMenuW(hInst,(LPWSTR)MenuName);
	if(dlgTempl->style & DS_SETFONT)
	{
		fprintf(stderr,"Win32: dialog fonts not supported yet\n");
	}
	
	/* Create dialog main window */
	rect.left = rect.top = 0;
	rect.right = dlgTempl->cx * xUnit / 4;
	rect.bottom = dlgTempl->cy * yUnit / 8;

	/* FIXME: proper modalframe handling ??*/
	if (dlgTempl->style & DS_MODALFRAME) exStyle |= WS_EX_DLGMODALFRAME;

        AdjustWindowRectEx16( &rect, dlgTempl->style,
                              hMenu ? TRUE : FALSE , exStyle );
	rect.right -= rect.left;
	rect.bottom -= rect.top;

	if(dlgTempl->x == CW_USEDEFAULT16)
		rect.left = rect.top = CW_USEDEFAULT16;
	else{
		rect.left += dlgTempl->x * xUnit / 4;
		rect.top += dlgTempl->y * yUnit / 8;
		if (!(dlgTempl->style & DS_ABSALIGN))
			ClientToScreen16(hWndParent, (POINT16 *)&rect );
	}

	/* FIXME: Here is the place to consider A */
	hwnd = CreateWindowEx32W(exStyle, (LPWSTR)ClassName, (LPWSTR)szCaption,
		dlgTempl->style & ~WS_VISIBLE, 
		rect.left, rect.top, rect.right, rect.bottom,
		hWndParent, hMenu, hInst, 0);

	if(!hwnd)
	{
		if(hFont)DeleteObject(hFont);
		if(hMenu)DeleteObject(hMenu);
		return 0;
	}

	wndPtr = WIN_FindWndPtr(hwnd);

	/* FIXME: should purge junk from system menu, but windows/dialog.c
	   says this does not belong here */

	/* Create control windows */
	dprintf_dialog(stddeb, " BEGIN\n" );
	dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
	dlgInfo->msgResult = 0;	/* This is used to store the default button id */
	dlgInfo->hDialogHeap = 0;

	for (i = 0; i < dlgTempl->noOfItems; i++)
	{
		if((int)ptr&3)
			ptr++;
		dlgitem = (DLGITEMTEMPLATE32*)ptr;
		ptr = (WORD*)(dlgitem+1);
		if(*ptr == 0xFFFF) {
			/* FIXME: ignore HIBYTE? */
			ClassId = *(ptr+1);
			ptr+=2;
		}else{
			ClassId = (DWORD)ptr;
			ptr += STRING32_lstrlenW(ptr)+1;
		}
		if(*ptr == 0xFFFF) {
			Text = *(ptr+1);
			ptr+=2;
		}else{
			Text = (DWORD)ptr;
			ptr += STRING32_lstrlenW(ptr)+1;
		}
		if(!HIWORD(ClassId))
		{
			switch(LOWORD(ClassId))
			{
				case 0x80: STRING32_AnsiToUni(buffer,"BUTTON" ); break;
				case 0x81: STRING32_AnsiToUni( buffer, "EDIT" ); break;
				case 0x82: STRING32_AnsiToUni( buffer, "STATIC" ); break;
				case 0x83: STRING32_AnsiToUni( buffer, "LISTBOX" ); break;
				case 0x84: STRING32_AnsiToUni( buffer, "SCROLLBAR" ); break;
				case 0x85: STRING32_AnsiToUni( buffer, "COMBOBOX" ); break;
				default:   buffer[0] = '\0'; break;
			}
			ClassId = (DWORD)buffer;
		}
		/*FIXME: debugging output*/
		/*FIXME: local edit ?*/
		exStyle = dlgitem->dwExtendedStyle|WS_EX_NOPARENTNOTIFY;
		if(*ptr)
		{
			fprintf(stderr,"having data\n");
		}
		ptr++;
		hwndCtrl = CreateWindowEx32W(WS_EX_NOPARENTNOTIFY,
			(LPWSTR)ClassId, (LPWSTR)Text,
			dlgitem->style | WS_CHILD,
			dlgitem->x * xUnit / 4,
			dlgitem->y * yUnit / 8,
			dlgitem->cx * xUnit / 4,
			dlgitem->cy * yUnit / 8,
			hwnd, (HMENU)((DWORD)dlgitem->id),
			hInst, (SEGPTR)0 );
		SetWindowPos( hwndCtrl, HWND_BOTTOM, 0, 0, 0, 0,
			SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );
		/* Send initialisation messages to the control */
		if (hFont) SendMessage32A( hwndCtrl, WM_SETFONT, (WPARAM)hFont, 0 );
		if (SendMessage32A( hwndCtrl, WM_GETDLGCODE, 0, 0 ) & DLGC_DEFPUSHBUTTON)
		{
			/* If there's already a default push-button, set it back */
			/* to normal and use this one instead. */
			if (hwndDefButton)
				SendMessage32A( hwndDefButton, BM_SETSTYLE32, BS_PUSHBUTTON, FALSE);
			hwndDefButton = hwndCtrl;
			dlgInfo->msgResult = GetWindowWord( hwndCtrl, GWW_ID );
		}
	}
	dprintf_dialog(stddeb, " END\n" );

	 /* Initialise dialog extra data */
	dlgInfo->dlgProc   = WINPROC_AllocWinProc(lpDialogFunc,WIN_PROC_32A);
	dlgInfo->hUserFont = hFont;
	dlgInfo->hMenu     = hMenu;
	dlgInfo->xBaseUnit = xUnit;
	dlgInfo->yBaseUnit = yUnit;
	dlgInfo->hwndFocus = DIALOG_GetFirstTabItem( hwnd );

	/* Send initialisation messages and set focus */
	if (dlgInfo->hUserFont)
		SendMessage32A( hwnd, WM_SETFONT, (WPARAM)dlgInfo->hUserFont, 0 );
	if (SendMessage32A( hwnd, WM_INITDIALOG, (WPARAM)dlgInfo->hwndFocus, dwInitParam ))
		SetFocus( dlgInfo->hwndFocus );
	if (dlgTempl->style & WS_VISIBLE) ShowWindow(hwnd, SW_SHOW);
		return hwnd;
}

HWND USER32_CreateDialogIndirectParamW(HINSTANCE hInst,LPVOID dlgTempl,
	HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
	return USER32_CreateDialogIndirectParamAorW(hInst,dlgTempl,hWndParent,
		lpDialogFunc,dwInitParam,0);
}

HWND USER32_CreateDialogIndirectParamA(HINSTANCE hInst,LPVOID dlgTempl,
	HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
	return USER32_CreateDialogIndirectParamAorW(hInst,dlgTempl,hWndParent,
		lpDialogFunc,dwInitParam,1);
}

HWND USER32_CreateDialogParamW(HINSTANCE hInst,LPCWSTR lpszName,
	HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
	HANDLE32 hrsrc;
	hrsrc=FindResource32(hInst,lpszName,(LPWSTR)RT_DIALOG);
	if(!hrsrc)return 0;
	return USER32_CreateDialogIndirectParamW(hInst,
		LoadResource32(hInst, hrsrc),hWndParent,lpDialogFunc,dwInitParam);
}

HWND USER32_CreateDialogParamA(HINSTANCE hInst,LPCSTR lpszName,
	HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
	HWND res;
	if(!HIWORD(lpszName))
		res = USER32_CreateDialogParamW(hInst,(LPCWSTR)lpszName,hWndParent,
				lpDialogFunc,dwInitParam);
	else{
		LPWSTR uni=STRING32_DupAnsiToUni(lpszName);
		res=USER32_CreateDialogParamW(hInst,uni,hWndParent,
			lpDialogFunc,dwInitParam);
	}
	return res;
}

int USER32_DialogBoxIndirectParamW(HINSTANCE hInstance,LPVOID dlgTempl,
	HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
	HWND hwnd;
	hwnd = USER32_CreateDialogIndirectParamW(hInstance,dlgTempl,
		hWndParent,lpDialogFunc,dwInitParam);
	if(hwnd)return DIALOG_DoDialogBox(hwnd,hWndParent);
	return -1;
}

int USER32_DialogBoxIndirectParamA(HINSTANCE hInstance,LPVOID dlgTempl,
	HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
	HWND hwnd;
	hwnd = USER32_CreateDialogIndirectParamA(hInstance,dlgTempl,
		hWndParent,lpDialogFunc,dwInitParam);
	if(hwnd)return DIALOG_DoDialogBox(hwnd,hWndParent);
	return -1;
}

int USER32_DialogBoxParamW(HINSTANCE hInstance,LPCWSTR lpszName,
	HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
	HWND hwnd;
	hwnd = USER32_CreateDialogParamW(hInstance,lpszName,
		hWndParent,lpDialogFunc,dwInitParam);
	if(hwnd)return DIALOG_DoDialogBox(hwnd,hWndParent);
	 return -1;
}

int USER32_DialogBoxParamA(HINSTANCE hInstance,LPCSTR lpszName,
	HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
	HWND hwnd;
	hwnd = USER32_CreateDialogParamA(hInstance,lpszName,
		hWndParent,lpDialogFunc,dwInitParam);
	if(hwnd)return DIALOG_DoDialogBox(hwnd,hWndParent);
	return -1;
}

int USER32_wsprintfA( int *args )
{
    return vsprintf( (char *)args[0], (char *)args[1], (va_list)&args[2] );
}
