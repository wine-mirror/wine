/*
 * Callback functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_CALLBACK_H
#define __WINE_CALLBACK_H

#include "wintypes.h"
#include "winnt.h"

extern int (*IF1632_CallLargeStack)( int (*func)(void), void *arg );

#define CALL_LARGE_STACK(func,arg) \
    (IF1632_CallLargeStack ? \
     IF1632_CallLargeStack( (int(*)())(func), (void *)(arg) ) : \
     ((int(*)())(func))((void *)arg))

typedef struct
{
    LONG (CALLBACK *CallRegisterShortProc)( CONTEXT *, INT32 );
    LONG (CALLBACK *CallRegisterLongProc)( CONTEXT *, INT32 );
    VOID (CALLBACK *CallTaskRescheduleProc)(void);
    VOID (CALLBACK *CallFrom16WndProc)(void);
    LRESULT (CALLBACK *CallWndProc)( WNDPROC16, HWND16, UINT16,
                                     WPARAM16, LPARAM );
    LRESULT (CALLBACK *CallDriverProc)( DRIVERPROC16, DWORD, HDRVR16,
                                        UINT16, LPARAM, LPARAM );
    LRESULT (CALLBACK *CallDriverCallback)( FARPROC16, HANDLE16, UINT16,
                                            DWORD, LPARAM, LPARAM );
    LRESULT (CALLBACK *CallTimeFuncProc)( FARPROC16, WORD, UINT16,
                                          DWORD, LPARAM, LPARAM );
    INT16 (CALLBACK *CallWindowsExitProc)( FARPROC16, INT16 );
    INT16 (CALLBACK *CallWordBreakProc)( EDITWORDBREAKPROC16, SEGPTR, INT16,
                                         INT16, INT16 );
    VOID (CALLBACK *CallBootAppProc)( FARPROC16, HANDLE16, HFILE16 );
    WORD (CALLBACK *CallLoadAppSegProc)( FARPROC16, HANDLE16, HFILE16, WORD );
    VOID (CALLBACK *CallSystemTimerProc)( FARPROC16 );
    HGLOBAL16 (CALLBACK *CallResourceHandlerProc)( FARPROC16, HGLOBAL16, HMODULE16, HRSRC16 );
    DWORD (CALLBACK *CallWOWCallbackProc)( FARPROC16, DWORD );
    BOOL32 (CALLBACK *CallWOWCallback16Ex)( FARPROC16, DWORD, DWORD, LPVOID, 
                                            LPDWORD );
    LRESULT (CALLBACK *CallASPIPostProc)( FARPROC16, SEGPTR );
    /* Following are the graphics driver callbacks */
    WORD (CALLBACK *CallDrvControlProc)( FARPROC16, SEGPTR, WORD,
                                         SEGPTR, SEGPTR );
    WORD (CALLBACK *CallDrvEnableProc)( FARPROC16, SEGPTR, WORD, SEGPTR,
                                        SEGPTR, SEGPTR );
    WORD (CALLBACK *CallDrvEnumDFontsProc)( FARPROC16, SEGPTR, SEGPTR,
                                            FARPROC16, SEGPTR );
    WORD (CALLBACK *CallDrvEnumObjProc)( FARPROC16, SEGPTR, WORD, FARPROC16,
                                         SEGPTR );
    WORD (CALLBACK *CallDrvOutputProc)( FARPROC16, SEGPTR, WORD, WORD, SEGPTR,
                                        SEGPTR, SEGPTR, SEGPTR, SEGPTR );
    DWORD (CALLBACK *CallDrvRealizeProc)( FARPROC16, SEGPTR, WORD, SEGPTR,
                                          SEGPTR, SEGPTR );
    WORD (CALLBACK *CallDrvStretchBltProc)( FARPROC16, SEGPTR, WORD, WORD,
                                            WORD, WORD, SEGPTR, WORD, WORD,
                                            WORD, WORD, DWORD, SEGPTR, SEGPTR,
                                            SEGPTR );
    DWORD (CALLBACK *CallDrvExtTextOutProc)( FARPROC16, SEGPTR, WORD, WORD,
                                             SEGPTR, SEGPTR, INT16, SEGPTR,
                                             SEGPTR, SEGPTR, SEGPTR, SEGPTR,
                                             WORD );
    WORD (CALLBACK *CallDrvGetCharWidthProc)( FARPROC16, SEGPTR, SEGPTR, WORD,
					      WORD, SEGPTR, SEGPTR, SEGPTR );
} CALLBACKS_TABLE;

extern const CALLBACKS_TABLE *Callbacks;

#endif /* __WINE_CALLBACK_H */
