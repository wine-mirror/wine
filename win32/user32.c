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
#include "string32.h"
#include "win.h"
#include "debug.h"
#include "stddebug.h"

/***********************************************************************
 *          GetMessageA          (USER32.269)
 */
BOOL USER32_GetMessageA(MSG32* lpmsg,DWORD hwnd,DWORD min,DWORD max)
{
	BOOL ret;
	MSG16 msg;
	ret=GetMessage(MAKE_SEGPTR(&msg),(HWND)hwnd,min,max);
	STRUCT32_MSG16to32(&msg,lpmsg);
	return ret;
}

/***********************************************************************
 *         DispatchMessageA       (USER32.140)
 */
LONG USER32_DispatchMessageA(MSG32* lpmsg)
{
	MSG16 msg;
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
	MSG16 msg;
	STRUCT32_MSG32to16(lpmsg,&msg);
	return TranslateMessage(&msg);
}

/***********************************************************************
 *         PeekMessageA
 */
BOOL32 PeekMessage32A( LPMSG32 lpmsg, HWND32 hwnd,
                       UINT32 min,UINT32 max,UINT32 wRemoveMsg)
{
	MSG16 msg;
	BOOL ret;
	ret=PeekMessage16(&msg,hwnd,min,max,wRemoveMsg);
        /* FIXME: should translate the message to Win32 */
	STRUCT32_MSG16to32(&msg,lpmsg);
	return ret;
}

/***********************************************************************
 *         PeekMessageW
 */
BOOL32 PeekMessage32W( LPMSG32 lpmsg, HWND32 hwnd,
                       UINT32 min,UINT32 max,UINT32 wRemoveMsg)
{
	/* FIXME: Should perform Unicode translation on specific messages */
	return PeekMessage32A(lpmsg,hwnd,min,max,wRemoveMsg);
}
