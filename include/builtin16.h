/*
 * Win16 built-in DLLs definitions
 *
 * Copyright 1999 Ulrich Weigand
 */

#ifndef __WINE_BUILTIN16_H
#define __WINE_BUILTIN16_H

#include "windef.h"
#include "ldt.h"

struct _CONTEXT86;
struct _STACK16FRAME;

extern WORD CallFrom16Word();
extern LONG CallFrom16Long();
extern void CallFrom16Register();
extern void CallFrom16Thunk();

extern WORD CALLBACK CallTo16Word( FARPROC16 target, INT nArgs );
extern LONG CALLBACK CallTo16Long( FARPROC16 target, INT nArgs );
extern void CALLBACK CallTo16RegisterShort( struct _CONTEXT86 *context, INT nArgs );
extern void CALLBACK CallTo16RegisterLong ( struct _CONTEXT86 *context, INT nArgs );

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

typedef struct
{
    const char *name;              /* DLL name */
    void       *module_start;      /* 32-bit address of the module data */
    int         module_size;       /* Size of the module data */
    const BYTE *code_start;        /* 32-bit address of DLL code */
    const BYTE *data_start;        /* 32-bit address of DLL data */
    const char *owner;             /* 32-bit dll that contains this dll */
    const void *rsrc;              /* resources data */
} BUILTIN16_DESCRIPTOR;

extern HMODULE16 BUILTIN_LoadModule( LPCSTR name );
extern LPCSTR BUILTIN_GetEntryPoint16( struct _STACK16FRAME *frame, LPSTR name, WORD *pOrd );
extern void BUILTIN_RegisterDLL( const BUILTIN16_DESCRIPTOR *descr );

#endif /* __WINE_BUILTIN16_H */
