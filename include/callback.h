/*
 * Callback functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_CALLBACK_H
#define __WINE_CALLBACK_H

#include "windef.h"
#include "winnt.h"
#include "wine/winuser16.h"

extern int (*IF1632_CallLargeStack)( int (*func)(void), void *arg );

#define CALL_LARGE_STACK(func,arg) \
    (IF1632_CallLargeStack ? \
     IF1632_CallLargeStack( (int(*)())(func), (void *)(arg) ) : \
     ((int(*)())(func))((void *)arg))

typedef struct
{
    LONG (*CallRegisterShortProc)( CONTEXT86 *, INT );
    LONG (*CallRegisterLongProc)( CONTEXT86 *, INT );
    VOID (*CallFrom16WndProc)(void);
    LRESULT (*CallWndProc)( WNDPROC16, HWND16, UINT16,
                                     WPARAM16, LPARAM );
    LRESULT (*CallDriverProc)( DRIVERPROC16, DWORD, HDRVR16,
                                        UINT16, LPARAM, LPARAM );
    LRESULT (*CallDriverCallback)( FARPROC16, HANDLE16, UINT16,
                                            DWORD, LPARAM, LPARAM );
    LRESULT (*CallTimeFuncProc)( FARPROC16, WORD, UINT16,
                                          DWORD, LPARAM, LPARAM );
    INT16 (*CallWindowsExitProc)( FARPROC16, INT16 );
    INT16 (*CallWordBreakProc)( EDITWORDBREAKPROC16, SEGPTR, INT16,
                                         INT16, INT16 );
    VOID (*CallBootAppProc)( FARPROC16, HANDLE16, HFILE16 );
    WORD (*CallLoadAppSegProc)( FARPROC16, HANDLE16, HFILE16, WORD );
    WORD (*CallLocalNotifyFunc)( FARPROC16, WORD, HLOCAL16, WORD );
    HGLOBAL16 (*CallResourceHandlerProc)( FARPROC16, HGLOBAL16, HMODULE16, HRSRC16 );
    DWORD (*CallWOWCallbackProc)( FARPROC16, DWORD );
    BOOL (*CallWOWCallback16Ex)( FARPROC16, DWORD, DWORD, LPVOID, 
                                            LPDWORD );
    DWORD (*CallUTProc)( FARPROC16, DWORD, DWORD );
    LRESULT (*CallASPIPostProc)( FARPROC16, SEGPTR );
    /* Following are the graphics driver callbacks */
    WORD (*CallDrvControlProc)( FARPROC16, SEGPTR, WORD,
                                         SEGPTR, SEGPTR );
    WORD (*CallDrvEnableProc)( FARPROC16, SEGPTR, WORD, SEGPTR,
                                        SEGPTR, SEGPTR );
    WORD (*CallDrvEnumDFontsProc)( FARPROC16, SEGPTR, SEGPTR,
                                            FARPROC16, SEGPTR );
    WORD (*CallDrvEnumObjProc)( FARPROC16, SEGPTR, WORD, FARPROC16,
                                         SEGPTR );
    WORD (*CallDrvOutputProc)( FARPROC16, SEGPTR, WORD, WORD, SEGPTR,
                                        SEGPTR, SEGPTR, SEGPTR, SEGPTR );
    DWORD (*CallDrvRealizeProc)( FARPROC16, SEGPTR, WORD, SEGPTR,
                                          SEGPTR, SEGPTR );
    WORD (*CallDrvStretchBltProc)( FARPROC16, SEGPTR, WORD, WORD,
                                            WORD, WORD, SEGPTR, WORD, WORD,
                                            WORD, WORD, DWORD, SEGPTR, SEGPTR,
                                            SEGPTR );
    DWORD (*CallDrvExtTextOutProc)( FARPROC16, SEGPTR, WORD, WORD,
                                             SEGPTR, SEGPTR, INT16, SEGPTR,
                                             SEGPTR, SEGPTR, SEGPTR, SEGPTR,
                                             WORD );
    WORD (*CallDrvGetCharWidthProc)( FARPROC16, SEGPTR, SEGPTR, WORD,
					      WORD, SEGPTR, SEGPTR, SEGPTR );
    BOOL16 (*CallDrvAbortProc)( FARPROC16, HDC16, INT16 );
} CALLBACKS_TABLE;

extern const CALLBACKS_TABLE *Callbacks;

typedef struct
{
    BOOL16 WINAPI (*PeekMessage16)( LPMSG16 msg, HWND16 hwnd, 
                                    UINT16 first, UINT16 last, UINT16 flags );
    BOOL WINAPI (*PeekMessageA)( LPMSG lpmsg, HWND hwnd,
                                     UINT min, UINT max, UINT wRemoveMsg );
    BOOL WINAPI (*PeekMessageW)( LPMSG lpmsg, HWND hwnd, 
                                     UINT min, UINT max, UINT wRemoveMsg );

    BOOL16 WINAPI (*GetMessage16)( SEGPTR msg, HWND16 hwnd, 
                                   UINT16 first, UINT16 last );
    BOOL WINAPI (*GetMessageA)( MSG* lpmsg, HWND hwnd, 
                                    UINT min, UINT max );
    BOOL WINAPI (*GetMessageW)( MSG* lpmsg, HWND hwnd, 
                                    UINT min, UINT max );

    LRESULT WINAPI (*SendMessage16)( HWND16 hwnd, UINT16 msg, 
                                     WPARAM16 wParam, LPARAM lParam );
    LRESULT WINAPI (*SendMessageA)( HWND hwnd, UINT msg, 
                                      WPARAM wParam, LPARAM lParam );
    LRESULT WINAPI (*SendMessageW)( HWND hwnd, UINT msg, 
                                      WPARAM wParam, LPARAM lParam );

    BOOL16 WINAPI (*PostMessage16)( HWND16 hwnd, UINT16 message, 
                                    WPARAM16 wParam, LPARAM lParam );
    BOOL WINAPI (*PostMessageA)( HWND hwnd, UINT message, 
                                     WPARAM wParam, LPARAM lParam );
    BOOL WINAPI (*PostMessageW)( HWND hwnd, UINT message, 
                                     WPARAM wParam, LPARAM lParam );

    BOOL16 WINAPI (*PostAppMessage16)( HTASK16 hTask, UINT16 message, 
                                       WPARAM16 wParam, LPARAM lParam );
    BOOL WINAPI (*PostThreadMessageA)( DWORD idThread , UINT message,
                                           WPARAM wParam, LPARAM lParam );
    BOOL WINAPI (*PostThreadMessageW)( DWORD idThread , UINT message,
                                           WPARAM wParam, LPARAM lParam );

    BOOL16 WINAPI (*TranslateMessage16)( const MSG16 *msg );
    BOOL WINAPI (*TranslateMessage)( const MSG *msg );

    LONG WINAPI (*DispatchMessage16)( const MSG16* msg );
    LONG WINAPI (*DispatchMessageA)( const MSG* msg );
    LONG WINAPI (*DispatchMessageW)( const MSG* msg );

    BOOL16 WINAPI (*RedrawWindow16)( HWND16 hwnd, const RECT16 *rectUpdate,
                                     HRGN16 hrgnUpdate, UINT16 flags );

    BOOL WINAPI (*RedrawWindow)( HWND hwnd, const RECT *rectUpdate,
                                     HRGN hrgnUpdate, UINT flags );

    WORD WINAPI (*UserSignalProc)( UINT uCode, DWORD dwThreadOrProcessID,
                                   DWORD dwFlags, HMODULE16 hModule );

    INT16 WINAPI (*InitApp16)( HINSTANCE16 hInst );
    HQUEUE16 WINAPI (*InitThreadInput16)( WORD unknown, WORD flags );
    void WINAPI (*UserYield16)( void );
    WORD WINAPI (*DestroyIcon32)( HGLOBAL16 handle, UINT16 flags );
    
}  CALLOUT_TABLE;

extern CALLOUT_TABLE Callout;


#endif /* __WINE_CALLBACK_H */
