/*
 * Win16 built-in DLLs definitions
 *
 * Copyright 1999 Ulrich Weigand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_BUILTIN16_H
#define __WINE_BUILTIN16_H

#include <windef.h>
#include <wine/windef16.h>

struct _CONTEXT86;
struct _STACK16FRAME;

#include <pshpack1.h>

#ifdef __i386__

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
    WORD   lret;                   /* lret $nArgs */
    WORD   nArgs;
    DWORD  arg_types[2];           /* type of each argument */
} CALLFROM16;

#else

typedef struct
{
    void (*target)();
    WORD   call;                   /* call CALLFROM16 */
    short  callfrom16;
} ENTRYPOINT16;

typedef struct
{
    WORD   lret;                   /* lret $nArgs */
    WORD   nArgs;
    DWORD  arg_types[2];           /* type of each argument */
} CALLFROM16;

#endif

#include <poppack.h>

/* argument type flags for relay debugging */
enum arg_types
{
    ARG_NONE = 0, /* indicates end of arg list */
    ARG_WORD,     /* unsigned word */
    ARG_SWORD,    /* signed word */
    ARG_LONG,     /* long or segmented pointer */
    ARG_PTR,      /* linear pointer */
    ARG_STR,      /* linear pointer to null-terminated string */
    ARG_SEGSTR    /* segmented pointer to null-terminated string */
};

/* flags added to arg_types[0] */
#define ARG_RET16    0x80000000  /* function returns 16-bit value */
#define ARG_REGISTER 0x40000000  /* function is register */

extern WORD __wine_call_from_16_word();
extern LONG __wine_call_from_16_long();
extern void __wine_call_from_16_regs();

#endif /* __WINE_BUILTIN16_H */
