/*
 * Function callbacks for the library
 *
 * Copyright 1997 Alexandre Julliard
 */

#include <assert.h>
#include "winuser.h"
#include "callback.h"
#include "task.h"
#include "syslevel.h"
#include "cursoricon.h"
#include "user.h"
#include "queue.h"
#include "debug.h"
#include "win.h"


/**********************************************************************
 *	     CALLBACK_CallWndProc
 */
static LRESULT WINAPI CALLBACK_CallWndProc( WNDPROC16 proc, HWND16 hwnd,
                                            UINT16 msg, WPARAM16 wParam,
                                            LPARAM lParam )
{
    LRESULT retvalue;
    int iWndsLocks;

    /* To avoid any deadlocks, all the locks on the windows structures
       must be suspended before the control is passed to the application */
    iWndsLocks = WIN_SuspendWndsLock();
    retvalue = proc( hwnd, msg, wParam, lParam );
    WIN_RestoreWndsLock(iWndsLocks);
    return retvalue;
}


/**********************************************************************
 *	     CALLBACK_CallRegisterProc
 */
static LONG WINAPI CALLBACK_CallRegisterProc( CONTEXT *context, INT offset)
{
    ERR(relay, "Cannot call a register proc in Winelib\n" );
    assert( FALSE );
    return 0;
}


/**********************************************************************
 *	     CALLBACK_CallDriverProc
 */
static LRESULT WINAPI CALLBACK_CallDriverProc( DRIVERPROC16 proc, DWORD dwId,
                                               HDRVR16 hdrvr, UINT16 msg,
                                               LPARAM lp1, LPARAM lp2 )
{
    return proc( dwId, hdrvr, msg, lp1, lp2 );
}


/**********************************************************************
 *	     CALLBACK_CallDriverCallback
 */
static LRESULT WINAPI CALLBACK_CallDriverCallback( FARPROC16 proc,
                                                   HANDLE16 hDev, UINT16 msg,
                                                   DWORD dwUser, LPARAM lp1,
                                                   LPARAM lp2 )
{
    return proc( hDev, msg, dwUser, lp1, lp2 );
}


/**********************************************************************
 *	     CALLBACK_CallTimeFuncProc
 */
static LRESULT WINAPI CALLBACK_CallTimeFuncProc( FARPROC16 proc, WORD id,
                                                 UINT16 msg, DWORD dwUser,
                                                 LPARAM lp1, LPARAM lp2 )
{
    return proc( id, msg, dwUser, lp1, lp2 );
}


/**********************************************************************
 *	     CALLBACK_CallWindowsExitProc
 */
static INT16 WINAPI CALLBACK_CallWindowsExitProc( FARPROC16 proc, INT16 type)
{
    return proc( type );
}


/**********************************************************************
 *	     CALLBACK_CallWordBreakProc
 */
static INT16 WINAPI CALLBACK_CallWordBreakProc( EDITWORDBREAKPROC16 proc,
                                                SEGPTR text, INT16 word,
                                                INT16 len, INT16 action )
{
    return proc( (LPSTR)text, word, len, action );
}


/**********************************************************************
 *	     CALLBACK_CallBootAppProc
 */
static void WINAPI CALLBACK_CallBootAppProc( FARPROC16 proc, HANDLE16 module,
                                             HFILE16 file )
{
    proc( module, file );
}


/**********************************************************************
 *	     CALLBACK_CallLoadAppSegProc
 */
static WORD WINAPI CALLBACK_CallLoadAppSegProc( FARPROC16 proc,
                                                HANDLE16 module, HFILE16 file,
                                                WORD seg )
{
    return proc( module, file, seg );
}


/**********************************************************************
 *           CALLBACK_CallLocalNotifyFunc
 */
static WORD WINAPI CALLBACK_CallLocalNotifyFunc( FARPROC16 proc,
                                                 WORD wMsg, HLOCAL16 hMem,
                                                 WORD wArg )
{
    return proc( wMsg, hMem, wArg );
}


/**********************************************************************
 *	     CALLBACK_CallResourceHandlerProc
 */
static HGLOBAL16 WINAPI CALLBACK_CallResourceHandlerProc( FARPROC16 proc,
                                                          HGLOBAL16 hMemObj, 
                                                          HMODULE16 hModule,
                                                          HRSRC16 hRsrc )
{
    ERR( relay, "Cannot call a 16-bit resource handler in Winelib\n" );
    assert( FALSE );
    return 0;
}


/**********************************************************************
 *	     CALLBACK_CallASPIPostProc
 */
static LRESULT WINAPI CALLBACK_CallASPIPostProc( FARPROC16 proc, SEGPTR ptr )
{
    return proc( ptr );
}


/**********************************************************************
 *	     CALLBACK_CallWOWCallbackProc
 */
static DWORD WINAPI CALLBACK_CallWOWCallbackProc( FARPROC16 proc, DWORD dw )
{
    return proc( dw );
}

/**********************************************************************
 *	     CALLBACK_CallWOWCallback16Ex
 *
 * WCB16_MAX_CBARGS (16) is the maximum number of args.
 *
 * Can call functions using CDECL or PASCAL calling conventions. The CDECL
 * ones are reversed (not 100% sure about that).
 */
static BOOL WINAPI CALLBACK_CallWOWCallback16Ex( 
	FARPROC16 proc, DWORD dwFlags, DWORD cbArgs, LPVOID xargs,LPDWORD pdwret
) {
    LPDWORD	args = (LPDWORD)xargs;
    DWORD	ret,i;

    if (dwFlags == WCB16_CDECL) {
    	/* swap the arguments */
    	args = HeapAlloc(GetProcessHeap(),0,cbArgs*sizeof(DWORD));
	for (i=0;i<cbArgs;i++)
		args[i] = ((DWORD*)xargs)[cbArgs-i-1];
    }
    switch (cbArgs) {
    case 0: ret = proc();break;
    case 1: ret = proc(args[0]);break;
    case 2: ret = proc(args[0],args[1]);break;
    case 3: ret = proc(args[0],args[1],args[2]);break;
    case 4: ret = proc(args[0],args[1],args[2],args[3]);break;
    case 5: ret = proc(args[0],args[1],args[2],args[3],args[4]);break;
    case 6: ret = proc(args[0],args[1],args[2],args[3],args[4],args[5]);
	    break;
    case 7: ret = proc(args[0],args[1],args[2],args[3],args[4],args[5],
		    args[6]
	    );
	    break;
    case 8: ret = proc(args[0],args[1],args[2],args[3],args[4],args[5],
		    args[6],args[7]
	    );
	    break;
    case 9: ret = proc(args[0],args[1],args[2],args[3],args[4],args[5],
		    args[6],args[7],args[8]
	    );
	    break;
    case 10:ret = proc(args[0],args[1],args[2],args[3],args[4],args[5],
		    args[6],args[7],args[8],args[9]
	    );
	    break;
    case 11:ret = proc(args[0],args[1],args[2],args[3],args[4],args[5],
		    args[6],args[7],args[8],args[9],args[10]
	    );
	    break;
    case 12:ret = proc(args[0],args[1],args[2],args[3],args[4],args[5],
		    args[6],args[7],args[8],args[9],args[10],args[11]
	    );
	    break;
    case 13:ret = proc(args[0],args[1],args[2],args[3],args[4],args[5],
		    args[6],args[7],args[8],args[9],args[10],args[11],
		    args[12]
	    );
	    break;
    case 14:ret = proc(args[0],args[1],args[2],args[3],args[4],args[5],
		    args[6],args[7],args[8],args[9],args[10],args[11],
		    args[12],args[13]
	    );
	    break;
    case 15:ret = proc(args[0],args[1],args[2],args[3],args[4],args[5],
		    args[6],args[7],args[8],args[9],args[10],args[11],
		    args[12],args[13],args[14]
	    );
	    break;
    case 16:ret = proc(args[0],args[1],args[2],args[3],args[4],args[5],
		    args[6],args[7],args[8],args[9],args[10],args[11],
		    args[12],args[13],args[14],args[15]
	    );
	    break;
    default:
	    WARN(relay,"(%ld) arguments not supported.\n",cbArgs);
	    if (dwFlags == WCB16_CDECL)
		HeapFree(GetProcessHeap(),0,args);
	    return FALSE;
    }
    if (dwFlags == WCB16_CDECL)
    	HeapFree(GetProcessHeap(),0,args);
    if (pdwret) 
    	*pdwret = ret;
    return TRUE;
}

/**********************************************************************
 *	     CALLBACK_CallUTProc
 */
static DWORD WINAPI CALLBACK_CallUTProc( FARPROC16 proc, DWORD w1, DWORD w2 )
{
    ERR( relay, "Cannot call a UT thunk proc in Winelib\n" );
    assert( FALSE );
    return 0;
}

/**********************************************************************
 *	     CALLBACK_CallTaskRescheduleProc
 */
static BOOL WINAPI CALLBACK_CallTaskRescheduleProc( void )
{
    BOOL pending;

    SYSLEVEL_EnterWin16Lock();
    pending = TASK_Reschedule();
    SYSLEVEL_LeaveWin16Lock();

    return pending;
}


/**********************************************************************
 *	     CALLBACK_WinelibTable
 *
 * The callbacks function table for Winelib
 */
static const CALLBACKS_TABLE CALLBACK_WinelibTable =
{
    CALLBACK_CallRegisterProc,        /* CallRegisterShortProc */
    CALLBACK_CallRegisterProc,        /* CallRegisterLongProc */
    CALLBACK_CallTaskRescheduleProc,  /* CallTaskRescheduleProc */
    NULL,                             /* CallFrom16WndProc */
    CALLBACK_CallWndProc,             /* CallWndProc */
    CALLBACK_CallDriverProc,          /* CallDriverProc */
    CALLBACK_CallDriverCallback,      /* CallDriverCallback */
    CALLBACK_CallTimeFuncProc,        /* CallTimeFuncProc */
    CALLBACK_CallWindowsExitProc,     /* CallWindowsExitProc */
    CALLBACK_CallWordBreakProc,       /* CallWordBreakProc */
    CALLBACK_CallBootAppProc,         /* CallBootAppProc */
    CALLBACK_CallLoadAppSegProc,      /* CallLoadAppSegProc */
    CALLBACK_CallLocalNotifyFunc,     /* CallLocalNotifyFunc */
    CALLBACK_CallResourceHandlerProc, /* CallResourceHandlerProc */
    CALLBACK_CallWOWCallbackProc,     /* CallWOWCallbackProc */
    CALLBACK_CallWOWCallback16Ex,     /* CallWOWCallback16Ex */
    CALLBACK_CallUTProc,              /* CallUTProc */
    CALLBACK_CallASPIPostProc,        /* CallASPIPostProc */
    /* The graphics driver callbacks are never used in Winelib */
    NULL,                             /* CallDrvControlProc */
    NULL,                             /* CallDrvEnableProc */
    NULL,                             /* CallDrvEnumDFontsProc */
    NULL,                             /* CallDrvEnumObjProc */
    NULL,                             /* CallDrvOutputProc */
    NULL,                             /* CallDrvRealizeProc */
    NULL,                             /* CallDrvStretchBltProc */
    NULL,                             /* CallDrvExtTextOutProc */
    NULL,                             /* CallDrvGetCharWidth */
    NULL                              /* CallDrvAbortProc */
};

const CALLBACKS_TABLE *Callbacks = &CALLBACK_WinelibTable;


/**********************************************************************
 *	     CALLOUT_Table
 *
 * The callout function table for Winelib
 */

CALLOUT_TABLE Callout = 
{
    PeekMessage16, PeekMessageA, PeekMessageW,
    GetMessage16, GetMessageA, GetMessageW,
    SendMessage16, SendMessageA, SendMessageW,
    PostMessage16, PostMessageA, PostMessageW,
    PostAppMessage16, PostThreadMessageA, PostThreadMessageW,
    TranslateMessage16, TranslateMessage,
    DispatchMessage16, DispatchMessageA, DispatchMessageW,
    RedrawWindow16, RedrawWindow,
    UserSignalProc,
    InitThreadInput16,
    UserYield16,
    CURSORICON_Destroy
};

