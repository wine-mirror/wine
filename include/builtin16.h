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

extern void RELAY_Unimplemented16(void);

extern WORD CallFrom16Word();
extern LONG CallFrom16Long();
extern void CallFrom16Register();
extern void CallFrom16Thunk();

extern WORD CALLBACK CallTo16Word( FARPROC16 target, INT nArgs );
extern LONG CALLBACK CallTo16Long( FARPROC16 target, INT nArgs );
extern LONG CALLBACK CallTo16RegisterShort( const struct _CONTEXT86 *context, INT nArgs );
extern LONG CALLBACK CallTo16RegisterLong ( const struct _CONTEXT86 *context, INT nArgs );

#include "pshpack1.h"

typedef struct
{
    WORD   pushw_bp;               /* pushw %bp */
    BYTE   pushl;                  /* pushl $target */
    void (*target)();
    WORD   call;                   /* call CALLFROM16 */
    WORD   callfrom16;
} ENTRYPOINT16;

#define EP(target, offset) { 0x5566, 0x68, (target), 0xe866, (WORD) (offset) }

typedef struct
{
    BYTE   pushl;                  /* pushl $relay */
    DWORD  relay;
    BYTE   lcall;                  /* lcall __FLATCS__:glue */
    DWORD  glue;
    WORD   flatcs;
    BYTE   prefix;                 /* lret $nArgs */
    BYTE   lret;
    WORD   nArgs;
    LPCSTR profile;                /* profile string */
} CALLFROM16;

#define CF16_WORD( relay, nArgs, profile ) \
  { 0x68, (DWORD)(relay), \
    0x9a, (DWORD)CallFrom16Word, __FLATCS__, \
    0x66, (nArgs)? 0xca : 0xcb, (nArgs)? (nArgs) : 0x9090, \
    (profile) }

#define CF16_LONG( relay, nArgs, profile ) \
  { 0x68, (DWORD)(relay), \
    0x9a, (DWORD)CallFrom16Long, __FLATCS__, \
    0x66, (nArgs)? 0xca : 0xcb, (nArgs)? (nArgs) : 0x9090, \
    (profile) }

#define CF16_REGS( relay, nArgs, profile ) \
  { 0x68, (DWORD)(relay), \
    0x9a, (DWORD)CallFrom16Register, __FLATCS__, \
    0x66, (nArgs)? 0xca : 0xcb, (nArgs)? (nArgs) : 0x9090, \
    (profile) }

#include "poppack.h"

typedef struct
{
    const char *name;              /* DLL name */
    void       *module_start;      /* 32-bit address of the module data */
    int         module_size;       /* Size of the module data */
    const BYTE *code_start;        /* 32-bit address of DLL code */
    const BYTE *data_start;        /* 32-bit address of DLL data */
    const void *rsrc;              /* resources data */
} BUILTIN16_DESCRIPTOR;

extern BOOL BUILTIN_Init(void);
extern HMODULE16 BUILTIN_LoadModule( LPCSTR name );
extern LPCSTR BUILTIN_GetEntryPoint16( struct _STACK16FRAME *frame, LPSTR name, WORD *pOrd );
extern void BUILTIN_RegisterDLL( const BUILTIN16_DESCRIPTOR *descr );

#endif /* __WINE_BUILTIN16_H */
