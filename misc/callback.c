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
#include "debugtools.h"
#include "win.h"

DEFAULT_DEBUG_CHANNEL(relay)


/**********************************************************************
 *	     CALLBACK_CallWndProc
 */
static LRESULT CALLBACK_CallWndProc( WNDPROC16 proc, HWND16 hwnd,
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
static LONG CALLBACK_CallRegisterProc( CONTEXT86 *context, INT offset)
{
    ERR("Cannot call a register proc in Winelib\n" );
    assert( FALSE );
    return 0;
}

/**********************************************************************
 *	     CALLBACK_CallDriverProc
 */
static LRESULT CALLBACK_CallDriverProc( DRIVERPROC16 proc, DWORD dwId,
                                        HDRVR16 hdrvr, UINT16 msg,
                                        LPARAM lp1, LPARAM lp2 )
{
    ERR("Cannot call a 16-bit driver proc in Winelib\n" );
    assert( FALSE );
    return 0;
}

/**********************************************************************
 *	     CALLBACK_CallDriverCallback
 */
static LRESULT CALLBACK_CallDriverCallback( FARPROC16 proc,
                                            HANDLE16 hDev, UINT16 msg,
                                            DWORD dwUser, LPARAM lp1,
                                            LPARAM lp2 )
{
    ERR("Cannot call a 16-bit driver proc in Winelib\n" );
    assert( FALSE );
    return 0;
}

/**********************************************************************
 *	     CALLBACK_CallTimeFuncProc
 */
static LRESULT CALLBACK_CallTimeFuncProc( FARPROC16 proc, WORD id,
                                                 UINT16 msg, DWORD dwUser,
                                                 LPARAM lp1, LPARAM lp2 )
{
    return proc( id, msg, dwUser, lp1, lp2 );
}

/**********************************************************************
 *	     CALLBACK_CallWindowsExitProc
 */
static INT16 CALLBACK_CallWindowsExitProc( FARPROC16 proc, INT16 type)
{
    return proc( type );
}

/**********************************************************************
 *	     CALLBACK_CallWordBreakProc
 */
static INT16 CALLBACK_CallWordBreakProc( EDITWORDBREAKPROC16 proc,
                                                SEGPTR text, INT16 word,
                                                INT16 len, INT16 action )
{
    return proc( (LPSTR)text, word, len, action );
}

/**********************************************************************
 *	     CALLBACK_CallBootAppProc
 */
static void CALLBACK_CallBootAppProc( FARPROC16 proc, HANDLE16 module,
                                             HFILE16 file )
{
    ERR("Cannot call a 16-bit self-load handler in Winelib\n" );
    assert( FALSE );
    return;
}

/**********************************************************************
 *	     CALLBACK_CallLoadAppSegProc
 */
static WORD CALLBACK_CallLoadAppSegProc( FARPROC16 proc,
                                                HANDLE16 module, HFILE16 file,
                                                WORD seg )
{
    ERR("Cannot call a 16-bit self-load handler in Winelib\n" );
    assert( FALSE );
    return 0;
}

/**********************************************************************
 *           CALLBACK_CallLocalNotifyFunc
 */
static WORD CALLBACK_CallLocalNotifyFunc( FARPROC16 proc,
                                          WORD wMsg, HLOCAL16 hMem, WORD wArg )
{
    ERR("Cannot call a 16-bit notification handler in Winelib\n" );
    assert( FALSE );
    return 0;
}

/**********************************************************************
 *	     CALLBACK_CallResourceHandlerProc
 */
static HGLOBAL16 CALLBACK_CallResourceHandlerProc( FARPROC16 proc,
                                                   HGLOBAL16 hMemObj, 
                                                   HMODULE16 hModule,
                                                   HRSRC16 hRsrc )
{
    ERR("Cannot call a 16-bit resource handler in Winelib\n" );
    assert( FALSE );
    return 0;
}

/**********************************************************************
 *	     CALLBACK_CallASPIPostProc
 */
static LRESULT CALLBACK_CallASPIPostProc( FARPROC16 proc, SEGPTR ptr )
{
    return proc( ptr );
}

/**********************************************************************
 *	     CALLBACK_CallWOWCallbackProc
 */
static DWORD CALLBACK_CallWOWCallbackProc( FARPROC16 proc, DWORD dw )
{
    ERR("Cannot call a WOW thunk proc in Winelib\n" );
    assert( FALSE );
    return 0;
}

/**********************************************************************
 *	     CALLBACK_CallWOWCallback16Ex
 */
static BOOL CALLBACK_CallWOWCallback16Ex( FARPROC16 proc, DWORD dwFlags, 
                                          DWORD cbArgs, LPVOID xargs, LPDWORD pdwret )
{
    ERR("Cannot call a WOW thunk proc in Winelib\n" );
    assert( FALSE );
    return 0;
}

/**********************************************************************
 *	     CALLBACK_CallUTProc
 */
static DWORD CALLBACK_CallUTProc( FARPROC16 proc, DWORD w1, DWORD w2 )
{
    ERR("Cannot call a UT thunk proc in Winelib\n" );
    assert( FALSE );
    return 0;
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
    InitApp16, InitThreadInput16,
    UserYield16,
    CURSORICON_Destroy
};

