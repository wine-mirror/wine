/*
 * Win16 built-in DLLs definitions
 *
 * Copyright 1999 Ulrich Weigand
 */

#ifndef __WINE_BUILTIN16_H
#define __WINE_BUILTIN16_H

#include "windef.h"
#include "wine/windef16.h"

struct _CONTEXT86;
struct _STACK16FRAME;

#ifdef __i386__
#include "pshpack1.h"

typedef struct
{
    WORD   pushw_bp;               /* pushw %bp */
    BYTE   pushl;                  /* pushl $target */
    void (*target)();
    WORD   call;                   /* call CALLFROM16 */
    short  callfrom16;
} ENTRYPOINT16;

typedef struct
{
    BYTE   pushl;                  /* pushl $relay */
    void  *relay;
    BYTE   lcall;                  /* lcall __FLATCS__:glue */
    void  *glue;
    WORD   flatcs;
    BYTE   prefix;                 /* lret $nArgs */
    BYTE   lret;
    WORD   nArgs;
    LPCSTR profile;                /* profile string */
} CALLFROM16;

#include "poppack.h"
#else

typedef struct
{
    void (*target)();
    int    callfrom16;
} ENTRYPOINT16;

typedef struct
{
    LPCSTR profile;
} CALLFROM16;

#endif

typedef struct
{
    const char *name;              /* DLL name */
    void       *module_start;      /* 32-bit address of the module data */
    int         module_size;       /* Size of the module data */
    void       *code_start;        /* 32-bit address of DLL code */
    void       *data_start;        /* 32-bit address of DLL data */
    const char *owner;             /* 32-bit dll that contains this dll */
    const void *rsrc;              /* resources data */
} BUILTIN16_DESCRIPTOR;

extern HMODULE16 BUILTIN_LoadModule( LPCSTR name );
extern LPCSTR BUILTIN_GetEntryPoint16( struct _STACK16FRAME *frame, LPSTR name, WORD *pOrd );

extern void __wine_register_dll_16( const BUILTIN16_DESCRIPTOR *descr );
extern WORD __wine_call_from_16_word();
extern LONG __wine_call_from_16_long();
extern void __wine_call_from_16_regs();
extern void __wine_call_from_16_thunk();

#endif /* __WINE_BUILTIN16_H */
