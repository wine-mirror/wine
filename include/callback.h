/*
 * Callback functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_CALLBACK_H
#define __WINE_CALLBACK_H

#include "wintypes.h"
#include "winnt.h"
#include "wine/winuser16.h"

extern int (*IF1632_CallLargeStack)( int (*func)(void), void *arg );

#define CALL_LARGE_STACK(func,arg) \
    (IF1632_CallLargeStack ? \
     IF1632_CallLargeStack( (int(*)())(func), (void *)(arg) ) : \
     ((int(*)())(func))((void *)arg))

typedef struct
{
    LONG (CALLBACK *CallRegisterShortProc)( CONTEXT *, INT32 );
    LONG (CALLBACK *CallRegisterLongProc)( CONTEXT *, INT32 );
    BOOL32 (CALLBACK *CallTaskRescheduleProc)(void);
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
    WORD (CALLBACK *CallLocalNotifyFunc)( FARPROC16, WORD, HLOCAL16, WORD );
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
    BOOL16 (CALLBACK *CallDrvAbortProc)( FARPROC16, HDC16, INT16 );
} CALLBACKS_TABLE;

extern const CALLBACKS_TABLE *Callbacks;

typedef struct
{
    BOOL16 WINAPI (*PeekMessage16)( LPMSG16 msg, HWND16 hwnd, 
                                    UINT16 first, UINT16 last, UINT16 flags );
    BOOL32 WINAPI (*PeekMessage32A)( LPMSG32 lpmsg, HWND32 hwnd,
                                     UINT32 min, UINT32 max, UINT32 wRemoveMsg );
    BOOL32 WINAPI (*PeekMessage32W)( LPMSG32 lpmsg, HWND32 hwnd, 
                                     UINT32 min, UINT32 max, UINT32 wRemoveMsg );

    BOOL16 WINAPI (*GetMessage16)( SEGPTR msg, HWND16 hwnd, 
                                   UINT16 first, UINT16 last );
    BOOL32 WINAPI (*GetMessage32A)( MSG32* lpmsg, HWND32 hwnd, 
                                    UINT32 min, UINT32 max );
    BOOL32 WINAPI (*GetMessage32W)( MSG32* lpmsg, HWND32 hwnd, 
                                    UINT32 min, UINT32 max );

    LRESULT WINAPI (*SendMessage16)( HWND16 hwnd, UINT16 msg, 
                                     WPARAM16 wParam, LPARAM lParam );
    LRESULT WINAPI (*SendMessage32A)( HWND32 hwnd, UINT32 msg, 
                                      WPARAM32 wParam, LPARAM lParam );
    LRESULT WINAPI (*SendMessage32W)( HWND32 hwnd, UINT32 msg, 
                                      WPARAM32 wParam, LPARAM lParam );

    BOOL16 WINAPI (*PostMessage16)( HWND16 hwnd, UINT16 message, 
                                    WPARAM16 wParam, LPARAM lParam );
    BOOL32 WINAPI (*PostMessage32A)( HWND32 hwnd, UINT32 message, 
                                     WPARAM32 wParam, LPARAM lParam );
    BOOL32 WINAPI (*PostMessage32W)( HWND32 hwnd, UINT32 message, 
                                     WPARAM32 wParam, LPARAM lParam );

    BOOL16 WINAPI (*PostAppMessage16)( HTASK16 hTask, UINT16 message, 
                                       WPARAM16 wParam, LPARAM lParam );
    BOOL32 WINAPI (*PostThreadMessage32A)( DWORD idThread , UINT32 message,
                                           WPARAM32 wParam, LPARAM lParam );
    BOOL32 WINAPI (*PostThreadMessage32W)( DWORD idThread , UINT32 message,
                                           WPARAM32 wParam, LPARAM lParam );

    BOOL16 WINAPI (*TranslateMessage16)( const MSG16 *msg );
    BOOL32 WINAPI (*TranslateMessage32)( const MSG32 *msg );

    LONG WINAPI (*DispatchMessage16)( const MSG16* msg );
    LONG WINAPI (*DispatchMessage32A)( const MSG32* msg );
    LONG WINAPI (*DispatchMessage32W)( const MSG32* msg );

    BOOL16 WINAPI (*RedrawWindow16)( HWND16 hwnd, const RECT16 *rectUpdate,
                                     HRGN16 hrgnUpdate, UINT16 flags );

    BOOL32 WINAPI (*RedrawWindow32)( HWND32 hwnd, const RECT32 *rectUpdate,
                                     HRGN32 hrgnUpdate, UINT32 flags );

    HQUEUE16 WINAPI (*InitThreadInput)( WORD unknown, WORD flags );
    void WINAPI (*UserYield)( void );
    
}  CALLOUT_TABLE;

extern CALLOUT_TABLE Callout;


#endif /* __WINE_CALLBACK_H */
