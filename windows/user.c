/*
 * Misc. USER functions
 *
 * Copyright 1993 Robert J. Amstadt
 *	     1996 Alex Korobka 
 */

#include <stdlib.h>
#include "windows.h"
#include "resource.h"
#include "heap.h"
#include "gdi.h"
#include "user.h"
#include "task.h"
#include "queue.h"
#include "win.h"
#include "clipboard.h"
#include "menu.h"
#include "hook.h"
#include "debug.h"
#include "toolhelp.h"
#include "message.h"
#include "module.h"
#include "miscemu.h"
#include "queue.h"
#include "shell.h"
#include "callback.h"

/***********************************************************************
 *           GetFreeSystemResources   (USER.284)
 */
WORD WINAPI GetFreeSystemResources( WORD resType )
{
    int userPercent, gdiPercent;

    switch(resType)
    {
    case GFSR_USERRESOURCES:
        userPercent = (int)LOCAL_CountFree( USER_HeapSel ) * 100 /
                               LOCAL_HeapSize( USER_HeapSel );
        gdiPercent  = 100;
        break;

    case GFSR_GDIRESOURCES:
        gdiPercent  = (int)LOCAL_CountFree( GDI_HeapSel ) * 100 /
                               LOCAL_HeapSize( GDI_HeapSel );
        userPercent = 100;
        break;

    case GFSR_SYSTEMRESOURCES:
        userPercent = (int)LOCAL_CountFree( USER_HeapSel ) * 100 /
                               LOCAL_HeapSize( USER_HeapSel );
        gdiPercent  = (int)LOCAL_CountFree( GDI_HeapSel ) * 100 /
                               LOCAL_HeapSize( GDI_HeapSel );
        break;

    default:
        return 0;
    }
    return (WORD)MIN( userPercent, gdiPercent );
}


/***********************************************************************
 *           SystemHeapInfo   (TOOLHELP.71)
 */
BOOL16 WINAPI SystemHeapInfo( SYSHEAPINFO *pHeapInfo )
{
    pHeapInfo->wUserFreePercent = GetFreeSystemResources( GFSR_USERRESOURCES );
    pHeapInfo->wGDIFreePercent  = GetFreeSystemResources( GFSR_GDIRESOURCES );
    pHeapInfo->hUserSegment = USER_HeapSel;
    pHeapInfo->hGDISegment  = GDI_HeapSel;
    return TRUE;
}


/***********************************************************************
 *           TimerCount   (TOOLHELP.80)
 */
BOOL16 WINAPI TimerCount( TIMERINFO *pTimerInfo )
{
    /* FIXME
     * In standard mode, dwmsSinceStart = dwmsThisVM 
     *
     * I tested this, under Windows in enhanced mode, and
     * if you never switch VM (ie start/stop DOS) these
     * values should be the same as well. 
     *
     * Also, Wine should adjust for the hardware timer
     * to reduce the amount of error to ~1ms. 
     * I can't be bothered, can you?
     */
    pTimerInfo->dwmsSinceStart = pTimerInfo->dwmsThisVM = GetTickCount();
    return TRUE;
}

static FARPROC16 __r16loader = NULL;

/**********************************************************************
 *           USER_CallDefaultRsrcHandler
 *
 * Called by the LoadDIBIcon/CursorHandler().
 */
HGLOBAL16 USER_CallDefaultRsrcHandler( HGLOBAL16 hMemObj, HMODULE16 hModule, HRSRC16 hRsrc )
{
    return Callbacks->CallResourceHandlerProc( __r16loader, hMemObj, hModule, hRsrc );
}

/**********************************************************************
 *           USER_InstallRsrcHandler
 */
static void USER_InstallRsrcHandler( HINSTANCE16 hInstance )
{
    FARPROC16 proc;

    /* SetResourceHandler() returns previous function which is set
     * when a module's resource table is loaded. */

    proc = SetResourceHandler( hInstance, RT_ICON16,
                               MODULE_GetWndProcEntry16("LoadDIBIconHandler") );
    if (!__r16loader) __r16loader = proc;

    proc = SetResourceHandler( hInstance, RT_CURSOR16,
                               MODULE_GetWndProcEntry16("LoadDIBCursorHandler") );
    if (!__r16loader) __r16loader = proc;
}

/**********************************************************************
 *           InitApp   (USER.5)
 */
INT16 WINAPI InitApp( HINSTANCE16 hInstance )
{
    int queueSize;

      /* InitTask() calls LibMain()'s of implicitly loaded DLLs 
       * prior to InitApp() so there is no clean way to do
       * SetTaskSignalHandler() in time. So, broken Windows bypasses 
       * a pTask->userhandler on startup and simply calls a global 
       * function pointer to the default USER signal handler.
       */

    USER_InstallRsrcHandler( hInstance );

    /* Hack: restore the divide-by-zero handler */
    /* FIXME: should set a USER-specific handler that displays a msg box */
    INT_SetHandler( 0, INT_GetHandler( 0xff ) );

      /* Create task message queue */
    queueSize = GetProfileInt32A( "windows", "DefaultQueueSize", 8 );
    if (!SetMessageQueue32( queueSize )) return 0;

    return 1;
}

/**********************************************************************
 *           USER_ModuleUnload
 */
static void USER_ModuleUnload( HMODULE16 hModule )
{
    HOOK_FreeModuleHooks( hModule );
    CLASS_FreeModuleClasses( hModule );
}

/**********************************************************************
 *           USER_AppExit
 */
static void USER_AppExit( HTASK16 hTask, HINSTANCE16 hInstance, HQUEUE16 hQueue )
{
    /* FIXME: empty clipboard if needed, maybe destroy menus (Windows
     *	      only complains about them but does nothing);
     */

    WND* desktop = WIN_GetDesktop();

    /* Patch desktop window */
    if( desktop->hmemTaskQ == hQueue )
	desktop->hmemTaskQ = GetTaskQueue(TASK_GetNextTask(hTask));

    /* Patch resident popup menu window */
    MENU_PatchResidentPopup( hQueue, NULL );

    TIMER_RemoveQueueTimers( hQueue );

    QUEUE_FlushMessages( hQueue );
    HOOK_FreeQueueHooks( hQueue );

    QUEUE_SetExitingQueue( hQueue );
    WIN_ResetQueueWindows( desktop, hQueue, (HQUEUE16)0);
    CLIPBOARD_ResetLock( hQueue, 0 );
    QUEUE_SetExitingQueue( 0 );

    /* Free the message queue */

    QUEUE_DeleteMsgQueue( hQueue );

    /* ModuleUnload() in "Internals" */

    hInstance = GetExePtr( hInstance );
    if( GetModuleUsage( hInstance ) <= 1 ) 
	USER_ModuleUnload( hInstance );
}


/***********************************************************************
 *           USER_ExitWindows
 *
 * Clean-up everything and exit the Wine process.
 * This is the back-end of ExitWindows(), called when all windows
 * have agreed to be terminated.
 */
void USER_ExitWindows(void)
{
    /* Do the clean-up stuff */

    WriteOutProfiles();
    SHELL_SaveRegistry();

    exit(0);
}


/***********************************************************************
 *           USER_SignalProc (USER.314)
 */
void WINAPI USER_SignalProc( HANDLE16 hTaskOrModule, UINT16 uCode,
                             UINT16 uExitFn, HINSTANCE16 hInstance,
                             HQUEUE16 hQueue )
{
    switch( uCode )
    {
	case USIG_GPF:
	case USIG_TERMINATION:
	     USER_AppExit( hTaskOrModule, hInstance, hQueue ); /* task */
	     break;

	case USIG_DLL_LOAD:
	     USER_InstallRsrcHandler( hTaskOrModule ); /* module */
	     break;

	case USIG_DLL_UNLOAD:
	     USER_ModuleUnload( hTaskOrModule ); /* module */
	     break;

	default:
	     FIXME(msg,"Unimplemented USER signal: %i\n", (int)uCode );
    }
}


/***********************************************************************
 *           ExitWindows16   (USER.7)
 */
BOOL16 WINAPI ExitWindows16( DWORD dwReturnCode, UINT16 wReserved )
{
    return ExitWindowsEx( EWX_LOGOFF, 0xffffffff );
}


/***********************************************************************
 *           ExitWindowsExec16   (USER.246)
 */
BOOL16 WINAPI ExitWindowsExec16( LPCSTR lpszExe, LPCSTR lpszParams )
{
    TRACE(system, "Should run the following in DOS-mode: \"%s %s\"\n",
	lpszExe, lpszParams);
    return ExitWindowsEx( EWX_LOGOFF, 0xffffffff );
}


/***********************************************************************
 *           ExitWindowsEx   (USER32.196)
 */
BOOL32 WINAPI ExitWindowsEx( UINT32 flags, DWORD reserved )
{
    int i;
    BOOL32 result;
    WND **list, **ppWnd;
        
    /* We have to build a list of all windows first, as in EnumWindows */

    if (!(list = WIN_BuildWinArray( WIN_GetDesktop(), 0, NULL ))) return FALSE;

    /* Send a WM_QUERYENDSESSION message to every window */

    for (ppWnd = list, i = 0; *ppWnd; ppWnd++, i++)
    {
        /* Make sure that the window still exists */
        if (!IsWindow32( (*ppWnd)->hwndSelf )) continue;
	if (!SendMessage16( (*ppWnd)->hwndSelf, WM_QUERYENDSESSION, 0, 0 ))
            break;
    }
    result = !(*ppWnd);

    /* Now notify all windows that got a WM_QUERYENDSESSION of the result */

    for (ppWnd = list; i > 0; i--, ppWnd++)
    {
        if (!IsWindow32( (*ppWnd)->hwndSelf )) continue;
	SendMessage16( (*ppWnd)->hwndSelf, WM_ENDSESSION, result, 0 );
    }
    HeapFree( SystemHeap, 0, list );

    if (result) USER_ExitWindows();
    return FALSE;
}

/***********************************************************************
 *           EnumDisplaySettingsA   (USER32.592)
 */
BOOL32 WINAPI EnumDisplaySettings32A(LPCSTR name,DWORD n,LPDEVMODE32A devmode) {
	TRACE(system,"(%s,%ld,%p)\n",name,n,devmode);
	if (n==0) {
		devmode->dmBitsPerPel = DefaultDepthOfScreen(screen);
		devmode->dmPelsHeight = screenHeight;
		devmode->dmPelsWidth = screenWidth;
		return TRUE;
	}
	return FALSE;
}


/***********************************************************************
 *           SetEventHook   (USER.321)
 *
 *	Used by Turbo Debugger for Windows
 */
FARPROC16 SetEventHook(FARPROC16 lpfnEventHook)
{
	FIXME(hook, "(lpfnEventHook=%08x): stub\n", (UINT32)lpfnEventHook);
	return NULL;
}

/***********************************************************************
 *           UserSeeUserDo   (USER.216)
 */
DWORD WINAPI UserSeeUserDo(WORD wReqType, WORD wParam1, WORD wParam2, WORD wParam3)
{
    switch (wReqType)
    {
    case USUD_LOCALALLOC:
        return LOCAL_Alloc(USER_HeapSel, wParam1, wParam3);
    case USUD_LOCALFREE:
        return LOCAL_Free(USER_HeapSel, wParam1);
    case USUD_LOCALCOMPACT:
        return LOCAL_Compact(USER_HeapSel, wParam3, 0);
    case USUD_LOCALHEAP:
        return USER_HeapSel;
    case USUD_FIRSTCLASS:
        FIXME(local, "return a pointer to the first window class.\n"); 
        return (DWORD)-1;
    default:
        WARN(local, "wReqType %04x (unknown)", wReqType);
        return (DWORD)-1;
    }
}

