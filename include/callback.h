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
    WORD WINAPI     (*UserSignalProc)( UINT, DWORD, DWORD, HMODULE16 );
    WORD WINAPI     (*DestroyIcon32)( HGLOBAL16, UINT16 );
}  CALLOUT_TABLE;

extern CALLOUT_TABLE Callout;


typedef struct {
    void WINAPI             (*LoadDosExe)( LPCSTR filename, HANDLE hFile );

    /* DPMI functions */
    void WINAPI             (*CallRMInt)( CONTEXT86 *context );
    void WINAPI             (*CallRMProc)( CONTEXT86 *context, int iret );
    void WINAPI             (*AllocRMCB)( CONTEXT86 *context );
    void WINAPI             (*FreeRMCB)( CONTEXT86 *context );

    /* I/O functions */
    void WINAPI             (*SetTimer)( unsigned ticks );
    unsigned WINAPI         (*GetTimer)( void );
    BOOL WINAPI             (*inport)( int port, int size, DWORD *res );
    BOOL WINAPI             (*outport)( int port, int size, DWORD val );
    void WINAPI             (*ASPIHandler)( CONTEXT86 *context );
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
