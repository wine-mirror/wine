/*
 * Win32 ordinal only exported functions that can't be stuffed somehwere else.
 *
 * Copyright 1997 Marcus Meissner
 */

#include "thread.h"
#include "winerror.h"
#include "heap.h"
#include "selectors.h"
#include "miscemu.h"
#include "winnt.h"
#include "process.h"
#include "module.h"
#include "task.h"
#include "callback.h"
#include "stackframe.h"
#include "debugtools.h"

DECLARE_DEBUG_CHANNEL(win);

/***********************************************************************
 *           RegisterShellHookWindow			[USER32.459]
 */
HRESULT WINAPI RegisterShellHookWindow ( DWORD u )
{	FIXME_(win)("0x%08lx stub\n",u);
	return 0;
	
}
/***********************************************************************
 *           DeregisterShellHookWindow			[USER32.132]
 */
HRESULT WINAPI DeregisterShellHookWindow ( DWORD u )
{	FIXME_(win)("0x%08lx stub\n",u);
	return 0;
	
}
/***********************************************************************
 *           RegisterTaskList   			[USER23.436]
 */
DWORD WINAPI RegisterTaskList (DWORD x)
{	FIXME_(win)("0x%08lx\n",x);
	return TRUE;
}

