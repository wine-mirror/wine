/*
 * Win16 built-in DLLs definitions
 *
 * Copyright 1999 Ulrich Weigand
 */

#ifndef __WINE_BUILTIN16_H
#define __WINE_BUILTIN16_H

#include "windef.h"

#include "pshpack1.h"

typedef struct
{
    WORD   pushw_bp;               /* pushw %bp */
    BYTE   pushl;                  /* pushl $target */      
    DWORD  target;
    BYTE   lcall;                  /* lcall __FLATCS__:relay */
    DWORD  relay;
    WORD   flatcs;
} STD_ENTRYPOINT16;

typedef struct
{
    WORD   movw_ax;                /* movw $<ax>, %ax */
    WORD   ax;
    WORD   movw_dx;                /* movw $<dx>, %dx */
    WORD   dx;
    WORD   lret;                   /* lret $<args> */
    WORD   args;
    WORD   nopnop;                 /* nop; nop */
} RET_ENTRYPOINT16;

typedef union
{
    STD_ENTRYPOINT16  std;
    RET_ENTRYPOINT16  ret;
} ENTRYPOINT16;

#define EP_STD( target, relay ) \
    { std: { 0x5566, 0x68, (DWORD)(target), 0x9a, (DWORD)(relay), __FLATCS__ } }

#define EP_RET( retval, nargs ) \
    { ret: { 0xb866, LOWORD(retval), 0xba66, HIWORD(retval), \
             (nargs)? 0xca66 : 0xcb66, (nargs)? (nargs) : 0x9090, 0x9090 } }

#include "poppack.h"

typedef struct
{
    const char *name;              /* DLL name */
    void       *module_start;      /* 32-bit address of the module data */
    int         module_size;       /* Size of the module data */
    const BYTE *code_start;        /* 32-bit address of DLL code */
    const BYTE *data_start;        /* 32-bit address of DLL data */
} WIN16_DESCRIPTOR;


extern void RELAY_Unimplemented16(void);


#endif /* __WINE_BUILTIN16_H */
