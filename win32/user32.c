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


int USER32_wsprintfA( int *args )
{
    return vsprintf( (char *)args[0], (char *)args[1], (va_list)&args[2] );
}
