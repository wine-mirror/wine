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

DECLARE_DEBUG_CHANNEL(dosmem)
DECLARE_DEBUG_CHANNEL(thread)
DECLARE_DEBUG_CHANNEL(win)
DECLARE_DEBUG_CHANNEL(win32)


/***********************************************************************
 *		GetWin16DOSEnv			(KERNEL32.34)
 * Returns some internal value.... probably the default environment database?
 */
DWORD WINAPI GetWin16DOSEnv()
{
	FIXME_(dosmem)("stub, returning 0\n");
	return 0;
}

/**********************************************************************
 *           GetPK16SysVar    (KERNEL32.92)
 */
LPVOID WINAPI GetPK16SysVar(void)
{
    static BYTE PK16SysVar[128];

    FIXME_(win32)("()\n");
    return PK16SysVar;
}

/**********************************************************************
 *           CommonUnimpStub    (KERNEL32.17)
 */
void WINAPI CommonUnimpStub( CONTEXT86 *context )
{
    if (EAX_reg(context))
        MESSAGE( "*** Unimplemented Win32 API: %s\n", (LPSTR)EAX_reg(context) );

    switch ((ECX_reg(context) >> 4) & 0x0f)
    {
    case 15:  EAX_reg(context) = -1;   break;
    case 14:  EAX_reg(context) = 0x78; break;
    case 13:  EAX_reg(context) = 0x32; break;
    case 1:   EAX_reg(context) = 1;    break;
    default:  EAX_reg(context) = 0;    break;
    }

    ESP_reg(context) += (ECX_reg(context) & 0x0f) * 4;
}

/**********************************************************************
 *           HouseCleanLogicallyDeadHandles    (KERNEL32.33)
 */
void WINAPI HouseCleanLogicallyDeadHandles(void)
{
    /* Whatever this is supposed to do, our handles probably
       don't need it :-) */
}

/**********************************************************************
 *		_KERNEL32_100
 */
BOOL WINAPI _KERNEL32_100(HANDLE threadid,DWORD exitcode,DWORD x) {
	FIXME_(thread)("(%d,%ld,0x%08lx): stub\n",threadid,exitcode,x);
	return TRUE;
}

/**********************************************************************
 *		_KERNEL32_99
 */
DWORD WINAPI _KERNEL32_99(DWORD x) {
	FIXME_(win32)("(0x%08lx): stub\n",x);
	return 1;
}
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

