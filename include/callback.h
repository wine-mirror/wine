/*
 * Callback functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_CALLBACK_H
#define __WINE_CALLBACK_H

#include "windef.h"
#include "winnt.h"
#include "wingdi.h"
#include "wine/winuser16.h"

extern void SYSDEPS_SwitchToThreadStack( void (*func)(void) ) WINE_NORETURN;

typedef void (*RELAY)();
extern FARPROC THUNK_Alloc( FARPROC16 func, RELAY relay );
extern void THUNK_Free( FARPROC thunk );
extern BOOL THUNK_Init(void);
extern void THUNK_InitCallout(void);


typedef struct
{
    BOOL WINAPI     (*PeekMessageA)( LPMSG, HWND, UINT, UINT, UINT );
    BOOL WINAPI     (*GetMessageA)( MSG*, HWND, UINT, UINT );
    LRESULT WINAPI  (*SendMessageA)( HWND, UINT, WPARAM, LPARAM );
    BOOL WINAPI     (*PostMessageA)( HWND, UINT, WPARAM, LPARAM );
    BOOL WINAPI     (*TranslateMessage)( const MSG *msg );
    LONG WINAPI     (*DispatchMessageA)( const MSG* msg );
    BOOL WINAPI     (*RedrawWindow)( HWND, const RECT *, HRGN, UINT );
    WORD WINAPI     (*UserSignalProc)( UINT, DWORD, DWORD, HMODULE16 );
    void WINAPI     (*FinalUserInit16)( void );
    HQUEUE16 WINAPI (*InitThreadInput16)( WORD, WORD );
    void WINAPI     (*UserYield16)( void );
    WORD WINAPI     (*DestroyIcon32)( HGLOBAL16, UINT16 );
    DWORD WINAPI    (*WaitForInputIdle)( HANDLE, DWORD );
    DWORD WINAPI    (*MsgWaitForMultipleObjects)( DWORD, HANDLE *, BOOL, DWORD, DWORD );
    HWND WINAPI     (*WindowFromDC)( HDC );
    INT WINAPI      (*MessageBoxA)( HWND, LPCSTR, LPCSTR, UINT );
    INT WINAPI      (*MessageBoxW)( HWND, LPCWSTR, LPCWSTR, UINT );
}  CALLOUT_TABLE;

extern CALLOUT_TABLE Callout;


typedef struct {
    struct _DOSTASK* WINAPI (*Current)( void );
    struct _DOSTASK* WINAPI (*LoadDPMI)( void );
    void WINAPI             (*LoadDosExe)( LPCSTR filename, HANDLE hFile );
    BOOL WINAPI             (*Exec)( CONTEXT86 *context, LPCSTR filename, BYTE func, LPVOID paramblk );
    void WINAPI             (*Exit)( CONTEXT86 *context, BOOL cs_psp, WORD retval );
    int WINAPI              (*Enter)( CONTEXT86 *context );
    void WINAPI             (*Wait)( int read_pipe, HANDLE hObject );
    void WINAPI             (*QueueEvent)( int irq, int priority, void (*relay)(CONTEXT86*,void*), void *data );
    void WINAPI             (*OutPIC)( WORD port, BYTE val );
    void WINAPI             (*SetTimer)( unsigned ticks );
    unsigned WINAPI         (*GetTimer)( void );
} DOSVM_TABLE;

extern DOSVM_TABLE Dosvm;


#include "pshpack1.h"

typedef struct tagTHUNK
{
    BYTE             popl_eax;           /* 0x58  popl  %eax (return address)*/
    BYTE             pushl_func;         /* 0x68  pushl $proc */
    FARPROC16        proc WINE_PACKED;
    BYTE             pushl_eax;          /* 0x50  pushl %eax */
    BYTE             jmp;                /* 0xe9  jmp   relay (relative jump)*/
    RELAY            relay WINE_PACKED;
    struct tagTHUNK *next WINE_PACKED;
    DWORD            magic;
} THUNK;

#include "poppack.h"

#define CALLTO16_THUNK_MAGIC 0x54484e4b   /* "THNK" */

#define DECL_THUNK(aname,aproc,arelay) \
    THUNK aname; \
    aname.popl_eax = 0x58; \
    aname.pushl_func = 0x68; \
    aname.proc = (FARPROC16) (aproc); \
    aname.pushl_eax = 0x50; \
    aname.jmp = 0xe9; \
    aname.relay = (RELAY)((char *)(arelay) - (char *)(&(aname).next)); \
    aname.next = NULL; \
    aname.magic = CALLTO16_THUNK_MAGIC;

#endif /* __WINE_CALLBACK_H */
