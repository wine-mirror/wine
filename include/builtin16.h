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
    void (*target)();
    BYTE   lcall;                  /* lcall __FLATCS__:relay */
    void (*relay)();
    WORD   flatcs;
} ENTRYPOINT16;

#define EP(target,relay) { 0x5566, 0x68, (target), 0x9a, (relay), __FLATCS__ }

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
