/*
 * Callback functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_CALLBACK_H
#define __WINE_CALLBACK_H

#include "windef.h"
#include "winnt.h"

typedef struct {
    void (WINAPI *LoadDosExe)( LPCSTR filename, HANDLE hFile );

    /* DPMI functions */
    void (WINAPI *CallRMInt)( CONTEXT86 *context );
    void (WINAPI *CallRMProc)( CONTEXT86 *context, int iret );
    void (WINAPI *AllocRMCB)( CONTEXT86 *context );
    void (WINAPI *FreeRMCB)( CONTEXT86 *context );

    /* I/O functions */
    void (WINAPI *SetTimer)( unsigned ticks );
    unsigned (WINAPI *GetTimer)( void );
    BOOL (WINAPI *inport)( int port, int size, DWORD *res );
    BOOL (WINAPI *outport)( int port, int size, DWORD val );
    void (WINAPI *ASPIHandler)( CONTEXT86 *context );
} DOSVM_TABLE;

extern DOSVM_TABLE Dosvm;

#endif /* __WINE_CALLBACK_H */
