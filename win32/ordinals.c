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
#include "debug.h"

DECLARE_DEBUG_CHANNEL(dosmem)
DECLARE_DEBUG_CHANNEL(thread)
DECLARE_DEBUG_CHANNEL(win)
DECLARE_DEBUG_CHANNEL(win32)

/**********************************************************************
 *           WOWGetDescriptor        (KERNEL32.88) (WOW32.1)
 */
BOOL WINAPI WOWGetDescriptor(SEGPTR segptr,LPLDT_ENTRY ldtent)
{
    return GetThreadSelectorEntry(GetCurrentThreadId(),segptr>>16,ldtent);
}

/***********************************************************************
 *		GetWin16DOSEnv			(KERNEL32.34)
 * Returns some internal value.... probably the default environment database?
 */
DWORD WINAPI GetWin16DOSEnv()
{
	FIXME(dosmem,"stub, returning 0\n");
	return 0;
}

/**********************************************************************
 *           GetPK16SysVar    (KERNEL32.92)
 */
LPVOID WINAPI GetPK16SysVar(void)
{
    static BYTE PK16SysVar[128];

    FIXME(win32, "()\n");
    return PK16SysVar;
}

/**********************************************************************
 *           CommonUnimpStub    (KERNEL32.17)
 */
REGS_ENTRYPOINT(CommonUnimpStub)
{
    if (EAX_reg(context))
        MSG( "*** Unimplemented Win32 API: %s\n", (LPSTR)EAX_reg(context) );

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


BOOL WINAPI _KERNEL32_100(HANDLE threadid,DWORD exitcode,DWORD x) {
	FIXME(thread,"(%d,%ld,0x%08lx): stub\n",threadid,exitcode,x);
	return TRUE;
}

DWORD WINAPI _KERNEL32_99(DWORD x) {
	FIXME(win32,"(0x%08lx): stub\n",x);
	return 1;
}
/***********************************************************************
 *           PrivateExtractIconExA			[USER32.442]
 */
HRESULT WINAPI PrivateExtractIconExA ( DWORD u, DWORD v, DWORD w, DWORD x ,DWORD y )
{	FIXME(win,"0x%08lx 0x%08lx 0x%08lx 0x%08lx 0x%08lx stub\n",u,v,w,x,y);
	return 0;
	
}
/***********************************************************************
 *           PrivateExtractIconExW			[USER32.443]
 */
HRESULT WINAPI PrivateExtractIconExW ( DWORD u, DWORD v, DWORD w, DWORD x ,DWORD y )
{	FIXME(win,"0x%08lx 0x%08lx 0x%08lx 0x%08lx 0x%08lx stub\n",u,v,w,x,y);
	return 0;
	
}
/***********************************************************************
 *           PrivateExtractIconsW			[USER32.445]
 */
HRESULT WINAPI PrivateExtractIconsW ( DWORD r, DWORD s, DWORD t, DWORD u, DWORD v, DWORD w, DWORD x, DWORD y )
{	FIXME(win,"0x%08lx 0x%08lx 0x%08lx 0x%08lx 0x%08lx 0x%08lx 0x%08lx 0x%08lx stub\n",r,s,t,u,v,w,x,y );
	return 0;
	
}
/***********************************************************************
 *           RegisterShellHookWindow			[USER32.459]
 */
HRESULT WINAPI RegisterShellHookWindow ( DWORD u )
{	FIXME(win,"0x%08lx stub\n",u);
	return 0;
	
}
/***********************************************************************
 *           DeregisterShellHookWindow			[USER32.132]
 */
HRESULT WINAPI DeregisterShellHookWindow ( DWORD u )
{	FIXME(win,"0x%08lx stub\n",u);
	return 0;
	
}
/***********************************************************************
 *           RegisterTaskList32   			[USER23.436]
 */
DWORD WINAPI RegisterTaskList (DWORD x)
{	FIXME(win,"0x%08lx\n",x);
	return TRUE;
}

