/*
 * Misc. USER functions
 *
 * Copyright 1993 Robert J. Amstadt
 *	     1996 Alex Korobka 
 */

#include <stdlib.h>
#include "wine/winbase16.h"
#include "winuser.h"
#include "heap.h"
#include "gdi.h"
#include "user.h"
#include "task.h"
#include "queue.h"
#include "win.h"
#include "clipboard.h"
#include "menu.h"
#include "cursoricon.h"
#include "hook.h"
#include "debug.h"
#include "toolhelp.h"
#include "message.h"
#include "module.h"
#include "miscemu.h"
#include "shell.h"
#include "callback.h"
#include "local.h"
#include "class.h"
#include "desktop.h"

/***********************************************************************
 *           GetFreeSystemResources   (USER.284)
 */
WORD WINAPI GetFreeSystemResources16( WORD resType )
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
BOOL16 WINAPI SystemHeapInfo16( SYSHEAPINFO *pHeapInfo )
{
    pHeapInfo->wUserFreePercent = GetFreeSystemResources16( GFSR_USERRESOURCES );
    pHeapInfo->wGDIFreePercent  = GetFreeSystemResources16( GFSR_GDIRESOURCES );
    pHeapInfo->hUserSegment = USER_HeapSel;
    pHeapInfo->hGDISegment  = GDI_HeapSel;
    return TRUE;
}


/***********************************************************************
 *           TimerCount   (TOOLHELP.80)
 */
BOOL16 WINAPI TimerCount16( TIMERINFO *pTimerInfo )
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

/**********************************************************************
 *           InitApp   (USER.5)
 */
INT16 WINAPI InitApp16( HINSTANCE16 hInstance )
{
    /* Hack: restore the divide-by-zero handler */
    /* FIXME: should set a USER-specific handler that displays a msg box */
    INT_SetPMHandler( 0, INT_GetPMHandler( 0xff ) );

    /* Create task message queue */
    if ( !GetFastQueue16() ) return 0;

    return 1;
}

/**********************************************************************
 *           USER_ModuleUnload
 */
static void USER_ModuleUnload( HMODULE16 hModule )
{
    HOOK_FreeModuleHooks( hModule );
    CLASS_FreeModuleClasses( hModule );
    CURSORICON_FreeModuleIcons( hModule );
}

/**********************************************************************
 *           USER_QueueCleanup
 */
void USER_QueueCleanup( HQUEUE16 hQueue )
{
    if ( hQueue )
    {
        WND* desktop = WIN_GetDesktop();

        /* Patch resident popup menu window */
        MENU_PatchResidentPopup( hQueue, NULL );

        TIMER_RemoveQueueTimers( hQueue );

        HOOK_FreeQueueHooks( hQueue );

        QUEUE_SetExitingQueue( hQueue );
        WIN_ResetQueueWindows( desktop, hQueue, (HQUEUE16)0);
        CLIPBOARD_ResetLock( hQueue, 0 );
        QUEUE_SetExitingQueue( 0 );

        /* Free the message queue */
        QUEUE_DeleteMsgQueue( hQueue );
    }
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
        desktop->hmemTaskQ = GetTaskQueue16(TASK_GetNextTask(hTask));
                  
    USER_QueueCleanup(hQueue);

    /* ModuleUnload() in "Internals" */

    hInstance = GetExePtr( hInstance );
    if( GetModuleUsage16( hInstance ) <= 1 ) 
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

    WriteOutProfiles16();
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
BOOL WINAPI ExitWindowsEx( UINT flags, DWORD reserved )
{
    int i;
    BOOL result;
    WND **list, **ppWnd;
        
    /* We have to build a list of all windows first, as in EnumWindows */

    if (!(list = WIN_BuildWinArray( WIN_GetDesktop(), 0, NULL ))) return FALSE;

    /* Send a WM_QUERYENDSESSION message to every window */

    for (ppWnd = list, i = 0; *ppWnd; ppWnd++, i++)
    {
        /* Make sure that the window still exists */
        if (!IsWindow( (*ppWnd)->hwndSelf )) continue;
	if (!SendMessage16( (*ppWnd)->hwndSelf, WM_QUERYENDSESSION, 0, 0 ))
            break;
    }
    result = !(*ppWnd);

    /* Now notify all windows that got a WM_QUERYENDSESSION of the result */

    for (ppWnd = list; i > 0; i--, ppWnd++)
    {
        if (!IsWindow( (*ppWnd)->hwndSelf )) continue;
	SendMessage16( (*ppWnd)->hwndSelf, WM_ENDSESSION, result, 0 );
    }
    HeapFree( SystemHeap, 0, list );

    if (result) USER_ExitWindows();
    return FALSE;
}


/***********************************************************************
 *           ChangeDisplaySettingA    (USER32.589)
 */
LONG WINAPI ChangeDisplaySettingsA( LPDEVMODEA devmode, DWORD flags )
{
  FIXME(system, ": stub\n");
  if (devmode==NULL)
    FIXME(system,"   devmode=NULL (return to default mode)\n");
  else if ( (devmode->dmBitsPerPel != DESKTOP_GetScreenDepth()) 
	    || (devmode->dmPelsHeight != DESKTOP_GetScreenHeight())
	    || (devmode->dmPelsWidth != DESKTOP_GetScreenWidth()) )

  {

    if (devmode->dmFields & DM_BITSPERPEL)
      FIXME(system,"   bpp=%ld\n",devmode->dmBitsPerPel);
    if (devmode->dmFields & DM_PELSWIDTH)
      FIXME(system,"   width=%ld\n",devmode->dmPelsWidth);
    if (devmode->dmFields & DM_PELSHEIGHT)
      FIXME(system,"   height=%ld\n",devmode->dmPelsHeight);
    FIXME(system," (Putting X in this mode beforehand might help)\n"); 
    /* we don't, but the program ... does not need to know */
    return DISP_CHANGE_SUCCESSFUL; 
  }
  return DISP_CHANGE_SUCCESSFUL;
}

/***********************************************************************
 *           EnumDisplaySettingsA   (USER32.592)
 * FIXME: Currently uses static list of modes.
 *
 * RETURNS
 *	TRUE if nth setting exists found (described in the LPDEVMODE32A struct)
 *	FALSE if we do not have the nth setting
 */
BOOL WINAPI EnumDisplaySettingsA(
	LPCSTR name,		/* [in] huh? */
	DWORD n,		/* [in] nth entry in display settings list*/
	LPDEVMODEA devmode	/* [out] devmode for that setting */
) {
#define NRMODES 5
#define NRDEPTHS 4
	struct {
		int w,h;
	} modes[NRMODES]={{512,384},{640,400},{640,480},{800,600},{1024,768}};
	int depths[4] = {8,16,24,32};

	TRACE(system,"(%s,%ld,%p)\n",name,n,devmode);
	if (n==0) {
		devmode->dmBitsPerPel = DESKTOP_GetScreenDepth();
		devmode->dmPelsHeight = DESKTOP_GetScreenHeight();
		devmode->dmPelsWidth = DESKTOP_GetScreenWidth();
		return TRUE;
	}
	if ((n-1)<NRMODES*NRDEPTHS) {
		devmode->dmBitsPerPel	= depths[(n-1)/NRMODES];
		devmode->dmPelsHeight	= modes[(n-1)%NRMODES].h;
		devmode->dmPelsWidth	= modes[(n-1)%NRMODES].w;
		return TRUE;
	}
	return FALSE;
}

/***********************************************************************
 *           EnumDisplaySettingsW   (USER32.593)
 */
BOOL WINAPI EnumDisplaySettingsW(LPCWSTR name,DWORD n,LPDEVMODEW devmode) {
	LPSTR nameA = HEAP_strdupWtoA(GetProcessHeap(),0,name);
	DEVMODEA	devmodeA; 
	BOOL ret = EnumDisplaySettingsA(nameA,n,&devmodeA); 

	if (ret) {
		devmode->dmBitsPerPel	= devmodeA.dmBitsPerPel;
		devmode->dmPelsHeight	= devmodeA.dmPelsHeight;
		devmode->dmPelsWidth	= devmodeA.dmPelsWidth;
		/* FIXME: convert rest too, if they are ever returned */
	}
	HeapFree(GetProcessHeap(),0,nameA);
	return ret;
}

/***********************************************************************
 *           SetEventHook   (USER.321)
 *
 *	Used by Turbo Debugger for Windows
 */
FARPROC16 WINAPI SetEventHook16(FARPROC16 lpfnEventHook)
{
	FIXME(hook, "(lpfnEventHook=%08x): stub\n", (UINT)lpfnEventHook);
	return NULL;
}

/***********************************************************************
 *           UserSeeUserDo   (USER.216)
 */
DWORD WINAPI UserSeeUserDo16(WORD wReqType, WORD wParam1, WORD wParam2, WORD wParam3)
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

/***********************************************************************
 *           RegisterLogonProcess   (USER32.434)
 */
DWORD WINAPI RegisterLogonProcess(HANDLE hprocess,BOOL x) {
	FIXME(win32,"(%d,%d),stub!\n",hprocess,x);
	return 1;
}

/***********************************************************************
 *           CreateWindowStation32W   (USER32.86)
 */
HWINSTA WINAPI CreateWindowStationW(
	LPWSTR winstation,DWORD res1,DWORD desiredaccess,
	LPSECURITY_ATTRIBUTES lpsa
) {
	FIXME(win32,"(%s,0x%08lx,0x%08lx,%p),stub!\n",debugstr_w(winstation),
		res1,desiredaccess,lpsa
	);
	return 0xdeadcafe;
}

/***********************************************************************
 *           SetProcessWindowStation   (USER32.496)
 */
BOOL WINAPI SetProcessWindowStation(HWINSTA hWinSta) {
	FIXME(win32,"(%d),stub!\n",hWinSta);
	return TRUE;
}

/***********************************************************************
 *           SetUserObjectSecurity   (USER32.514)
 */
BOOL WINAPI SetUserObjectSecurity(
	HANDLE hObj,
	/*LPSECURITY_INFORMATION*/LPVOID pSIRequested,
	PSECURITY_DESCRIPTOR pSID
) {
	FIXME(win32,"(0x%08x,%p,%p),stub!\n",hObj,pSIRequested,pSID);
	return TRUE;
}

/***********************************************************************
 *           CreateDesktop32W   (USER32.69)
 */
HDESK WINAPI CreateDesktopW(
	LPWSTR lpszDesktop,LPWSTR lpszDevice,LPDEVMODEW pDevmode,
	DWORD dwFlags,DWORD dwDesiredAccess,LPSECURITY_ATTRIBUTES lpsa
) {
	FIXME(win32,"(%s,%s,%p,0x%08lx,0x%08lx,%p),stub!\n",
		debugstr_w(lpszDesktop),debugstr_w(lpszDevice),pDevmode,
		dwFlags,dwDesiredAccess,lpsa
	);
	return 0xcafedead;
}

/***********************************************************************
 *           SetWindowStationUser   (USER32.521)
 */
DWORD WINAPI SetWindowStationUser(DWORD x1,DWORD x2) {
	FIXME(win32,"(0x%08lx,0x%08lx),stub!\n",x1,x2);
	return 1;
}

/***********************************************************************
 *           SetLogonNotifyWindow   (USER32.486)
 */
DWORD WINAPI SetLogonNotifyWindow(HWINSTA hwinsta,HWND hwnd) {
	FIXME(win32,"(0x%x,%04x),stub!\n",hwinsta,hwnd);
	return 1;
}

/***********************************************************************
 *           LoadLocalFonts   (USER32.486)
 */
VOID WINAPI LoadLocalFonts(VOID) {
	/* are loaded. */
	return;
}
/***********************************************************************
 *           GetUserObjectInformation32A   (USER32.299)
 */
BOOL WINAPI GetUserObjectInformationA( HANDLE hObj, int nIndex, LPVOID pvInfo, DWORD nLength, LPDWORD lpnLen )
{	FIXME(win32,"(0x%x %i %p %ld %p),stub!\n", hObj, nIndex, pvInfo, nLength, lpnLen );
	return TRUE;
}
/***********************************************************************
 *           GetUserObjectInformation32W   (USER32.300)
 */
BOOL WINAPI GetUserObjectInformationW( HANDLE hObj, int nIndex, LPVOID pvInfo, DWORD nLength, LPDWORD lpnLen )
{	FIXME(win32,"(0x%x %i %p %ld %p),stub!\n", hObj, nIndex, pvInfo, nLength, lpnLen );
	return TRUE;
}
/***********************************************************************
 *           GetUserObjectSecurity32   (USER32.300)
 */
BOOL WINAPI GetUserObjectSecurity(HANDLE hObj, SECURITY_INFORMATION * pSIRequested,
	PSECURITY_DESCRIPTOR pSID, DWORD nLength, LPDWORD lpnLengthNeeded)
{	FIXME(win32,"(0x%x %p %p len=%ld %p),stub!\n",  hObj, pSIRequested, pSID, nLength, lpnLengthNeeded);
	return TRUE;
}
