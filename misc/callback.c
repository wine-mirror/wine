/*
 * Function callbacks for the library
 *
 * Copyright 1997 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>
#include "callback.h"

extern void TASK_Reschedule(void);  /* loader/task.c */

/**********************************************************************
 *	     CALLBACK_CallWndProc
 */
static LRESULT WINAPI CALLBACK_CallWndProc( WNDPROC16 proc, HWND16 hwnd,
                                            UINT16 msg, WPARAM16 wParam,
                                            LPARAM lParam )
{
    return proc( hwnd, msg, wParam, lParam );
}


/**********************************************************************
 *	     CALLBACK_CallRegisterProc
 */
static VOID WINAPI CALLBACK_CallRegisterProc( CONTEXT *context, INT32 offset)
{
    fprintf( stderr, "Cannot call a register proc in Winelib\n" );
    assert( FALSE );
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
 *	     CALLBACK_CallSystemTimerProc
 */
static void WINAPI CALLBACK_CallSystemTimerProc( FARPROC16 proc )
{
    proc();
}


/**********************************************************************
 *	     CALLBACK_CallASPIPostProc
 */
static LRESULT WINAPI CALLBACK_CallASPIPostProc( FARPROC16 proc, SEGPTR ptr )
{
    return proc( ptr );
}

/**********************************************************************
 *	     CALLBACK_WinelibTable
 *
 * The callbacks function table for Winelib
 */
static const CALLBACKS_TABLE CALLBACK_WinelibTable =
{
    CALLBACK_CallRegisterProc,     /* CallRegisterProc */
    TASK_Reschedule,               /* CallTaskRescheduleProc */
    CALLBACK_CallWndProc,          /* CallWndProc */
    CALLBACK_CallDriverProc,       /* CallDriverProc */
    CALLBACK_CallDriverCallback,   /* CallDriverCallback */
    CALLBACK_CallTimeFuncProc,     /* CallTimeFuncProc */
    CALLBACK_CallWindowsExitProc,  /* CallWindowsExitProc */
    CALLBACK_CallWordBreakProc,    /* CallWordBreakProc */
    CALLBACK_CallBootAppProc,      /* CallBootAppProc */
    CALLBACK_CallLoadAppSegProc,   /* CallLoadAppSegProc */
    CALLBACK_CallSystemTimerProc,  /* CallSystemTimerProc */
    CALLBACK_CallASPIPostProc,     /* CallASPIPostProc */
    /* The graphics driver callbacks are never used in Winelib */
    NULL,                          /* CallDrvControlProc */
    NULL,                          /* CallDrvEnableProc */
    NULL,                          /* CallDrvEnumDFontsProc */
    NULL,                          /* CallDrvEnumObjProc */
    NULL,                          /* CallDrvOutputProc */
    NULL,                          /* CallDrvRealizeProc */
    NULL,                          /* CallDrvStretchBltProc */
    NULL                           /* CallDrvExtTextOutProc */
};

const CALLBACKS_TABLE *Callbacks = &CALLBACK_WinelibTable;
