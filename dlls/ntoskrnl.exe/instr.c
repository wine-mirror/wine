/*
 * Emulation of privileged instructions
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 2005 Ivan Leo Puoti
 * Copyright 2005 Laurent Pinchart
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#ifdef __i386__

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "excpt.h"
#include "wine/debug.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

#include "pshpack1.h"
struct idtr
{
    WORD  limit;
    BYTE *base;
};
#include "poppack.h"

static LDT_ENTRY idt[256];

static inline struct idtr get_idtr(void)
{
    struct idtr ret;
#ifdef __GNUC__
    __asm__( "sidtl %0" : "=m" (ret) );
#else
    ret.base = (BYTE *)idt;
    ret.limit = sizeof(idt) - 1;
#endif
    return ret;
}

/* store an operand into a register */
static void store_reg_word( CONTEXT *context, BYTE regmodrm, const BYTE *addr, int long_op )
{
    switch((regmodrm >> 3) & 7)
    {
    case 0:
        if (long_op) context->Eax = *(const DWORD *)addr;
        else context->Eax = (context->Eax & 0xffff0000) | *(const WORD *)addr;
        break;
    case 1:
        if (long_op) context->Ecx = *(const DWORD *)addr;
        else context->Ecx = (context->Ecx & 0xffff0000) | *(const WORD *)addr;
        break;
    case 2:
        if (long_op) context->Edx = *(const DWORD *)addr;
        else context->Edx = (context->Edx & 0xffff0000) | *(const WORD *)addr;
        break;
    case 3:
        if (long_op) context->Ebx = *(const DWORD *)addr;
        else context->Ebx = (context->Ebx & 0xffff0000) | *(const WORD *)addr;
        break;
    case 4:
        if (long_op) context->Esp = *(const DWORD *)addr;
        else context->Esp = (context->Esp & 0xffff0000) | *(const WORD *)addr;
        break;
    case 5:
        if (long_op) context->Ebp = *(const DWORD *)addr;
        else context->Ebp = (context->Ebp & 0xffff0000) | *(const WORD *)addr;
        break;
    case 6:
        if (long_op) context->Esi = *(const DWORD *)addr;
        else context->Esi = (context->Esi & 0xffff0000) | *(const WORD *)addr;
        break;
    case 7:
        if (long_op) context->Edi = *(const DWORD *)addr;
        else context->Edi = (context->Edi & 0xffff0000) | *(const WORD *)addr;
        break;
    }
}

/* store an operand into a byte register */
static void store_reg_byte( CONTEXT *context, BYTE regmodrm, const BYTE *addr )
{
    switch((regmodrm >> 3) & 7)
    {
    case 0: context->Eax = (context->Eax & 0xffffff00) | *addr; break;
    case 1: context->Ecx = (context->Ecx & 0xffffff00) | *addr; break;
    case 2: context->Edx = (context->Edx & 0xffffff00) | *addr; break;
    case 3: context->Ebx = (context->Ebx & 0xffffff00) | *addr; break;
    case 4: context->Eax = (context->Eax & 0xffff00ff) | (*addr << 8); break;
    case 5: context->Ecx = (context->Ecx & 0xffff00ff) | (*addr << 8); break;
    case 6: context->Edx = (context->Edx & 0xffff00ff) | (*addr << 8); break;
    case 7: context->Ebx = (context->Ebx & 0xffff00ff) | (*addr << 8); break;
    }
}

/***********************************************************************
 *           INSTR_GetOperandAddr
 *
 * Return the address of an instruction operand (from the mod/rm byte).
 */
static BYTE *INSTR_GetOperandAddr( CONTEXT *context, BYTE *instr,
                                   int long_addr, int segprefix, int *len )
{
    int mod, rm, base = 0, index = 0, ss = 0, off;

#define GET_VAL(val,type) \
    { *val = *(type *)instr; instr += sizeof(type); *len += sizeof(type); }

    *len = 0;
    GET_VAL( &mod, BYTE );
    rm = mod & 7;
    mod >>= 6;

    if (mod == 3)
    {
        switch(rm)
        {
        case 0: return (BYTE *)&context->Eax;
        case 1: return (BYTE *)&context->Ecx;
        case 2: return (BYTE *)&context->Edx;
        case 3: return (BYTE *)&context->Ebx;
        case 4: return (BYTE *)&context->Esp;
        case 5: return (BYTE *)&context->Ebp;
        case 6: return (BYTE *)&context->Esi;
        case 7: return (BYTE *)&context->Edi;
        }
    }

    if (long_addr)
    {
        if (rm == 4)
        {
            BYTE sib;
            GET_VAL( &sib, BYTE );
            rm = sib & 7;
            ss = sib >> 6;
            switch((sib >> 3) & 7)
            {
            case 0: index = context->Eax; break;
            case 1: index = context->Ecx; break;
            case 2: index = context->Edx; break;
            case 3: index = context->Ebx; break;
            case 4: index = 0; break;
            case 5: index = context->Ebp; break;
            case 6: index = context->Esi; break;
            case 7: index = context->Edi; break;
            }
        }

        switch(rm)
        {
        case 0: base = context->Eax; break;
        case 1: base = context->Ecx; break;
        case 2: base = context->Edx; break;
        case 3: base = context->Ebx; break;
        case 4: base = context->Esp; break;
        case 5: base = context->Ebp; break;
        case 6: base = context->Esi; break;
        case 7: base = context->Edi; break;
        }
        switch (mod)
        {
        case 0:
            if (rm == 5)  /* special case: ds:(disp32) */
            {
                GET_VAL( &base, DWORD );
            }
            break;

        case 1:  /* 8-bit disp */
            GET_VAL( &off, BYTE );
            base += (signed char)off;
            break;

        case 2:  /* 32-bit disp */
            GET_VAL( &off, DWORD );
            base += (signed long)off;
            break;
        }
    }
    else  /* short address */
    {
        switch(rm)
        {
        case 0:  /* ds:(bx,si) */
            base = LOWORD(context->Ebx) + LOWORD(context->Esi);
            break;
        case 1:  /* ds:(bx,di) */
            base = LOWORD(context->Ebx) + LOWORD(context->Edi);
            break;
        case 2:  /* ss:(bp,si) */
            base = LOWORD(context->Ebp) + LOWORD(context->Esi);
            break;
        case 3:  /* ss:(bp,di) */
            base = LOWORD(context->Ebp) + LOWORD(context->Edi);
            break;
        case 4:  /* ds:(si) */
            base = LOWORD(context->Esi);
            break;
        case 5:  /* ds:(di) */
            base = LOWORD(context->Edi);
            break;
        case 6:  /* ss:(bp) */
            base = LOWORD(context->Ebp);
            break;
        case 7:  /* ds:(bx) */
            base = LOWORD(context->Ebx);
            break;
        }

        switch(mod)
        {
        case 0:
            if (rm == 6)  /* special case: ds:(disp16) */
            {
                GET_VAL( &base, WORD );
            }
            break;

        case 1:  /* 8-bit disp */
            GET_VAL( &off, BYTE );
            base += (signed char)off;
            break;

        case 2:  /* 16-bit disp */
            GET_VAL( &off, WORD );
            base += (signed short)off;
            break;
        }
        base &= 0xffff;
    }
    /* FIXME: we assume that all segments have a base of 0 */
    return (BYTE *)(base + (index << ss));
#undef GET_VAL
}


/***********************************************************************
 *           emulate_instruction
 *
 * Emulate a privileged instruction.
 * Returns exception continuation status.
 */
static DWORD emulate_instruction( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    int prefix, segprefix, prefixlen, len, long_op, long_addr;
    BYTE *instr;

    long_op = long_addr = 1;
    instr = (BYTE *)context->Eip;
    if (!instr) return ExceptionContinueSearch;

    /* First handle any possible prefix */

    segprefix = -1;  /* no prefix */
    prefix = 1;
    prefixlen = 0;
    while(prefix)
    {
        switch(*instr)
        {
        case 0x2e:
            segprefix = context->SegCs;
            break;
        case 0x36:
            segprefix = context->SegSs;
            break;
        case 0x3e:
            segprefix = context->SegDs;
            break;
        case 0x26:
            segprefix = context->SegEs;
            break;
        case 0x64:
            segprefix = context->SegFs;
            break;
        case 0x65:
            segprefix = context->SegGs;
            break;
        case 0x66:
            long_op = !long_op;  /* opcode size prefix */
            break;
        case 0x67:
            long_addr = !long_addr;  /* addr size prefix */
            break;
        case 0xf0:  /* lock */
	    break;
        case 0xf2:  /* repne */
	    break;
        case 0xf3:  /* repe */
            break;
        default:
            prefix = 0;  /* no more prefixes */
            break;
        }
        if (prefix)
        {
            instr++;
            prefixlen++;
        }
    }

    /* Now look at the actual instruction */

    switch(*instr)
    {
    case 0x0f: /* extended instruction */
        switch(instr[1])
        {
        case 0x22: /* mov eax, crX */
            switch (instr[2])
            {
            case 0xc0:
                TRACE("mov eax,cr0 at 0x%08x, EAX=0x%08x\n", context->Eip,context->Eax );
                context->Eip += prefixlen+3;
                return ExceptionContinueExecution;
            case 0xe0:
                TRACE("mov eax,cr4 at 0x%08x, EAX=0x%08x\n", context->Eip,context->Eax );
                context->Eip += prefixlen+3;
                return ExceptionContinueExecution;
            default:
                break; /*fallthrough to bad instruction handling */
            }
            ERR("Unsupported EAX -> CR register, eip+2 is %02x\n", instr[2]);
            break; /*fallthrough to bad instruction handling */
        case 0x20: /* mov crX, eax */
            switch (instr[2])
            {
            case 0xe0: /* mov cr4, eax */
                /* CR4 register . See linux/arch/i386/mm/init.c, X86_CR4_ defs
                 * bit 0: VME	Virtual Mode Exception ?
                 * bit 1: PVI	Protected mode Virtual Interrupt
                 * bit 2: TSD	Timestamp disable
                 * bit 3: DE	Debugging extensions
                 * bit 4: PSE	Page size extensions
                 * bit 5: PAE   Physical address extension
                 * bit 6: MCE	Machine check enable
                 * bit 7: PGE   Enable global pages
                 * bit 8: PCE	Enable performance counters at IPL3
                 */
                TRACE("mov cr4,eax at 0x%08x\n",context->Eip);
                context->Eax = 0;
                context->Eip += prefixlen+3;
                return ExceptionContinueExecution;
            case 0xc0: /* mov cr0, eax */
                TRACE("mov cr0,eax at 0x%08x\n",context->Eip);
                context->Eax = 0x10; /* FIXME: set more bits ? */
                context->Eip += prefixlen+3;
                return ExceptionContinueExecution;
            default: /* fallthrough to illegal instruction */
                break;
            }
            /* fallthrough to illegal instruction */
            break;
        case 0x21: /* mov drX, eax */
            switch (instr[2])
            {
            case 0xc8: /* mov dr1, eax */
                TRACE("mov dr1,eax at 0x%08x\n",context->Eip);
                context->Eax = context->Dr1;
                context->Eip += prefixlen+3;
                return ExceptionContinueExecution;
            case 0xf8: /* mov dr7, eax */
                TRACE("mov dr7,eax at 0x%08x\n",context->Eip);
                context->Eax = 0x400;
                context->Eip += prefixlen+3;
                return ExceptionContinueExecution;
            }
            ERR("Unsupported DR register -> EAX, eip+2 is %02x\n", instr[2]);
            /* fallthrough to illegal instruction */
            break;
        case 0x23: /* mov eax drX */
            switch (instr[2])
            {
            case 0xc0: /* mov eax, dr0 */
                context->Dr0 = context->Eax;
                context->Eip += prefixlen+3;
                return ExceptionContinueExecution;
            case 0xc8: /* mov eax, dr1 */
                context->Dr1 = context->Eax;
                context->Eip += prefixlen+3;
                return ExceptionContinueExecution;
            case 0xd0: /* mov eax, dr2 */
                context->Dr2 = context->Eax;
                context->Eip += prefixlen+3;
                return ExceptionContinueExecution;
            case 0xd8: /* mov eax, dr3 */
                context->Dr3 = context->Eax;
                context->Eip += prefixlen+3;
                return ExceptionContinueExecution;
            case 0xf8: /* mov eax, dr7 */
                context->Dr7 = context->Eax;
                context->Eip += prefixlen+3;
                return ExceptionContinueExecution;
            }
            ERR("Unsupported EAX -> DR register, eip+2 is %02x\n", instr[2]);
            /* fallthrough to illegal instruction */
            break;
        }
        break;  /* Unable to emulate it */

    case 0x8a: /* mov Eb, Gb */
    case 0x8b: /* mov Ev, Gv */
    {
        BYTE *data = INSTR_GetOperandAddr(context, instr + 1, long_addr,
                                          segprefix, &len);
        unsigned int data_size = (*instr == 0x8b) ? (long_op ? 4 : 2) : 1;
        struct idtr idtr = get_idtr();
        unsigned int offset = data - idtr.base;

        if (offset <= idtr.limit + 1 - data_size)
        {
            idt[1].LimitLow = 0x100; /* FIXME */
            idt[2].LimitLow = 0x11E; /* FIXME */
            idt[3].LimitLow = 0x500; /* FIXME */

            switch (*instr)
            {
            case 0x8a: store_reg_byte( context, instr[1], (BYTE *)idt + offset ); break;
            case 0x8b: store_reg_word( context, instr[1], (BYTE *)idt + offset, long_op ); break;
            }
            context->Eip += prefixlen + len + 1;
            return ExceptionContinueExecution;
        }
        break;  /* Unable to emulate it */
    }

    case 0xfa: /* cli */
    case 0xfb: /* sti */
        context->Eip += prefixlen + 1;
        return ExceptionContinueExecution;
    }
    return ExceptionContinueSearch;  /* Unable to emulate it */
}


/***********************************************************************
 *           vectored_handler
 *
 * Vectored exception handler used to emulate protected instructions
 * from 32-bit code.
 */
LONG CALLBACK vectored_handler( EXCEPTION_POINTERS *ptrs )
{
    EXCEPTION_RECORD *record = ptrs->ExceptionRecord;
    CONTEXT *context = ptrs->ContextRecord;

    if ((record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
         record->ExceptionCode == EXCEPTION_PRIV_INSTRUCTION))
    {
        if (emulate_instruction( record, context ) == ExceptionContinueExecution)
            return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

#endif  /* __i386__ */
