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

typedef void (*RELAY)();
extern FARPROC THUNK_Alloc( FARPROC16 func, RELAY relay );
extern void THUNK_Free( FARPROC thunk );

typedef struct
{
    LONG (CALLBACK *CallRegisterShortProc)( CONTEXT86 *, INT );
    LONG (CALLBACK *CallRegisterLongProc)( CONTEXT86 *, INT );
    INT16 (CALLBACK *CallWindowsExitProc)( FARPROC16, INT16 );
    INT16 (CALLBACK *CallWordBreakProc)( EDITWORDBREAKPROC16, SEGPTR, INT16,
                                         INT16, INT16 );
    VOID (CALLBACK *CallBootAppProc)( FARPROC16, HANDLE16, HFILE16 );
    WORD (CALLBACK *CallLoadAppSegProc)( FARPROC16, HANDLE16, HFILE16, WORD );
    WORD (CALLBACK *CallLocalNotifyFunc)( FARPROC16, WORD, HLOCAL16, WORD );
    HGLOBAL16 (CALLBACK *CallResourceHandlerProc)( FARPROC16, HGLOBAL16, HMODULE16, HRSRC16 );
    DWORD (CALLBACK *CallUTProc)( FARPROC16, DWORD, DWORD );
    LRESULT (CALLBACK *CallASPIPostProc)( FARPROC16, SEGPTR );
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
    void WINAPI (*FinalUserInit16)( void );

    INT16 WINAPI (*InitApp16)( HINSTANCE16 hInst );
    HQUEUE16 WINAPI (*InitThreadInput16)( WORD unknown, WORD flags );
    void WINAPI (*UserYield16)( void );
    WORD WINAPI (*DestroyIcon32)( HGLOBAL16 handle, UINT16 flags );
    DWORD WINAPI (*WaitForInputIdle)( HANDLE hProcess, DWORD dwTimeOut );
    
}  CALLOUT_TABLE;

extern CALLOUT_TABLE Callout;


#endif /* __WINE_CALLBACK_H */
