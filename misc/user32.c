/*
 * Win32 user functions
 *
 * Copyright 1995 Martin von Loewis
 */

/* This file contains only wrappers to existing Wine functions or trivial
   stubs. 'Real' implementations go into context specific files */

#include <unistd.h>
#include "windows.h"
#include "winerror.h"
#include "relay32.h"
#include "alias.h"
#include "stackframe.h"

/* Structure copy functions */
static void MSG16to32(MSG *msg16,struct WIN32_MSG *msg32)
{
	msg32->hwnd=(DWORD)msg16->hwnd;
	msg32->message=msg16->message;
	msg32->wParam=msg16->wParam;
	msg32->lParam=msg16->lParam;
	msg32->time=msg16->time;
	msg32->pt.x=msg16->pt.x;
	msg32->pt.y=msg16->pt.y;
}

static void MSG32to16(struct WIN32_MSG *msg32,MSG *msg16)
{
	msg16->hwnd=(HWND)msg32->hwnd;
	msg16->message=msg32->message;
	msg16->wParam=msg32->wParam;
	msg16->lParam=msg32->lParam;
	msg16->time=msg32->time;
	msg16->pt.x=msg32->pt.x;
	msg16->pt.y=msg32->pt.y;
}

/***********************************************************************
 *           RegisterClassA      (USER32.426)
 */
ATOM USER32_RegisterClassA(WNDCLASSA* wndclass)
{
	WNDCLASS copy;
	char *s1,*s2;
	copy.style=wndclass->style;
	ALIAS_RegisterAlias(0,0,(DWORD)wndclass->lpfnWndProc);
	copy.lpfnWndProc=wndclass->lpfnWndProc;
	copy.cbClsExtra=wndclass->cbClsExtra;
	copy.cbWndExtra=wndclass->cbWndExtra;
	copy.hInstance=(HINSTANCE)wndclass->hInstance;
	copy.hIcon=(HICON)wndclass->hIcon;
	copy.hCursor=(HCURSOR)wndclass->hCursor;
	copy.hbrBackground=(HBRUSH)wndclass->hbrBackground;
	if(wndclass->lpszMenuName)
	{
		s1=alloca(strlen(wndclass->lpszMenuName)+1);
		strcpy(s1,wndclass->lpszMenuName);
		copy.lpszMenuName=MAKE_SEGPTR(s1);
	}else
		copy.lpszMenuName=0;
	if(wndclass->lpszClassName)
	{
		s2=alloca(strlen(wndclass->lpszClassName)+1);
		strcpy(s2,wndclass->lpszClassName);
		copy.lpszClassName=MAKE_SEGPTR(s2);
	}
	return RegisterClass(&copy);
}

/***********************************************************************
 *          DefWindowProcA       (USER32.125)
 */
LRESULT USER32_DefWindowProcA(DWORD hwnd,DWORD msg,DWORD wParam,DWORD lParam)
{
	/* some messages certainly need special casing. We come to that later */
	return DefWindowProc((HWND)hwnd,msg,wParam,lParam);
}

/***********************************************************************
 *          GetMessageA          (USER32.269)
 */
BOOL USER32_GetMessageA(struct WIN32_MSG* lpmsg,DWORD hwnd,DWORD min,DWORD max)
{
	BOOL ret;
	MSG msg;
	ret=GetMessage(MAKE_SEGPTR(&msg),(HWND)hwnd,min,max);
	MSG16to32(&msg,lpmsg);
	return ret;
}

/***********************************************************************
 *          BeginPaint           (USER32.9)
 */
HDC USER32_BeginPaint(DWORD hwnd,struct WIN32_PAINTSTRUCT *lpps)
{
	PAINTSTRUCT ps;
	HDC ret;
	ret=BeginPaint((HWND)hwnd,&ps);
	lpps->hdc=(DWORD)ps.hdc;
	lpps->fErase=ps.fErase;
	lpps->rcPaint.top=ps.rcPaint.top;
	lpps->rcPaint.left=ps.rcPaint.left;
	lpps->rcPaint.right=ps.rcPaint.right;
	lpps->rcPaint.bottom=ps.rcPaint.bottom;
	lpps->fRestore=ps.fRestore;
	lpps->fIncUpdate=ps.fIncUpdate;
	return ret;
}

/***********************************************************************
 *          EndPaint             (USER32.175)
 */
BOOL USER32_EndPaint(DWORD hwnd,struct WIN32_PAINTSTRUCT *lpps)
{
	PAINTSTRUCT ps;
	ps.hdc=lpps->hdc;
	ps.fErase=lpps->fErase;
	ps.rcPaint.top=lpps->rcPaint.top;
	ps.rcPaint.left=lpps->rcPaint.left;
	ps.rcPaint.right=lpps->rcPaint.right;
	ps.rcPaint.bottom=lpps->rcPaint.bottom;
	ps.fRestore=lpps->fRestore;
	ps.fIncUpdate=lpps->fIncUpdate;
	EndPaint((HWND)hwnd,&ps);
	return TRUE;
}

/***********************************************************************
 *         DispatchMessageA       (USER32.140)
 */
LONG USER32_DispatchMessageA(struct WIN32_MSG* lpmsg)
{
	MSG msg;
	LONG ret;
	MSG32to16(lpmsg,&msg);
	ret=DispatchMessage(&msg);
	MSG16to32(&msg,lpmsg);
	return ret;
}

/***********************************************************************
 *         CreateWindowExA        (USER32.82)
 */
DWORD USER32_CreateWindowExA(long flags,char* class,char *title,
	long style,int x,int y,int width,int height,DWORD parent,DWORD menu,
	DWORD instance,DWORD param)
{
    char *classbuf, *titlebuf;
    /*Have to translate CW_USEDEFAULT */
    if(x==0x80000000)x=CW_USEDEFAULT;
    if(width==0x80000000)width=CW_USEDEFAULT;
    classbuf = alloca( strlen(class)+1 );
    strcpy( classbuf, class );
    titlebuf = alloca( strlen(title)+1 );
    strcpy( titlebuf, title );
    return CreateWindowEx( flags, MAKE_SEGPTR(classbuf), MAKE_SEGPTR(titlebuf),
                           style,x,y,width,height,parent,menu,instance,param );
}
