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
#include "xmalloc.h"
#include "struct32.h"

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

void USER32_RECT32to16(const RECT32* r32,RECT *r16)
{
	r16->left = r32->left;
	r16->right = r32->right;
	r16->top = r32->top;
	r16->bottom = r32->bottom;
}

void USER32_RECT16to32(const RECT* r16,RECT32 *r32)
{
	r32->left = r16->left;
	r32->right = r16->right;
	r32->top = r16->top;
	r32->bottom = r16->bottom;
}

/***********************************************************************
 *           RegisterClassA      (USER32.426)
 */
ATOM USER32_RegisterClassA(WNDCLASSA* wndclass)
{
	WNDCLASS copy;
	HANDLE classh = 0, menuh = 0;
	SEGPTR classsegp, menusegp;
	char *classbuf, *menubuf;

	ATOM retval;
	copy.style=wndclass->style;
	ALIAS_RegisterAlias(0,0,(DWORD)wndclass->lpfnWndProc);
	copy.lpfnWndProc=wndclass->lpfnWndProc;
	copy.cbClsExtra=wndclass->cbClsExtra;
	copy.cbWndExtra=wndclass->cbWndExtra;
	copy.hInstance=(HINSTANCE)wndclass->hInstance;
	copy.hIcon=(HICON)wndclass->hIcon;
	copy.hCursor=(HCURSOR)wndclass->hCursor;
	copy.hbrBackground=(HBRUSH)wndclass->hbrBackground;

	/* FIXME: There has to be a better way of doing this - but neither
	malloc nor alloca will work */

	if(wndclass->lpszMenuName)
	{
		menuh = GlobalAlloc(0, strlen(wndclass->lpszMenuName)+1);
		menusegp = WIN16_GlobalLock(menuh);
		menubuf = PTR_SEG_TO_LIN(menusegp);
		strcpy( menubuf, wndclass->lpszMenuName);
		copy.lpszMenuName=menusegp;
	}else
		copy.lpszMenuName=0;
	if(wndclass->lpszClassName)
	{
		classh = GlobalAlloc(0, strlen(wndclass->lpszClassName)+1);
		classsegp = WIN16_GlobalLock(classh);
		classbuf = PTR_SEG_TO_LIN(classsegp);
		strcpy( classbuf, wndclass->lpszClassName);
		copy.lpszClassName=classsegp;
	}
	retval = RegisterClass(&copy);
	GlobalFree(menuh);
	GlobalFree(classh);
	return retval;
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
	ps.hdc=(HDC)lpps->hdc;
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
 *         TranslateMessage       (USER32.555)
 */
BOOL USER32_TranslateMessage(struct WIN32_MSG* lpmsg)
{
	MSG msg;
	MSG32to16(lpmsg,&msg);
	return TranslateMessage(&msg);
}

/***********************************************************************
 *         CreateWindowExA        (USER32.82)
 */
DWORD USER32_CreateWindowExA(long flags,char* class,char *title,
	long style,int x,int y,int width,int height,DWORD parent,DWORD menu,
	DWORD instance,DWORD param)
{
    DWORD retval;
    HANDLE classh, titleh;
    SEGPTR classsegp, titlesegp;
    char *classbuf, *titlebuf;

    /*Have to translate CW_USEDEFAULT */
    if(x==0x80000000)x=CW_USEDEFAULT;
    if(y==0x80000000)y=CW_USEDEFAULT;
    if(width==0x80000000)width=CW_USEDEFAULT;
    if(height==0x80000000)height=CW_USEDEFAULT;

    /* FIXME: There has to be a better way of doing this - but neither
    malloc nor alloca will work */

    classh = GlobalAlloc(0, strlen(class)+1);
    titleh = GlobalAlloc(0, strlen(title)+1);
    classsegp = WIN16_GlobalLock(classh);
    titlesegp = WIN16_GlobalLock(titleh);
    classbuf = PTR_SEG_TO_LIN(classsegp);
    titlebuf = PTR_SEG_TO_LIN(titlesegp);
    strcpy( classbuf, class );
    strcpy( titlebuf, title );
    
    retval = (DWORD) CreateWindowEx(flags,classsegp,
				  titlesegp,style,x,y,width,height,
				  (HWND)parent,(HMENU)menu,(HINSTANCE)instance,
				  (LPVOID)param);
    GlobalFree(classh);
    GlobalFree(titleh);
    return retval;
}

/***********************************************************************
 *         InvalidateRect           (USER32.327)
 */
BOOL USER32_InvalidateRect(HWND hWnd,const RECT32 *lpRect,BOOL bErase)
{
	RECT r;
	USER32_RECT32to16(lpRect,&r);
	InvalidateRect(hWnd,&r,bErase);
	/* FIXME: Return value */
	return 0;
}

/***********************************************************************
 *          DrawTextA                (USER32.163)
 */
int USER32_DrawTextA(HDC hdc,LPCSTR lpStr,int count,RECT32* r32,UINT uFormat)
{
	RECT r;
	USER32_RECT32to16(r32,&r);
	return DrawText(hdc,lpStr,count,&r,uFormat);
}

/***********************************************************************
 *          GetClientRect            (USER32.219)
 */
BOOL USER32_GetClientRect(HWND hwnd,RECT32 *r32)
{
	RECT r;
	GetClientRect(hwnd,&r);
	USER32_RECT16to32(&r,r32);
	/* FIXME: return value */
	return 0;
}

