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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/winuser16.h"
#include "excpt.h"
#include "wine/debug.h"
#include "kernel16_private.h"
#include "dosexe.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);
WINE_DECLARE_DEBUG_CHANNEL(io);

/* macros to set parts of a DWORD */
#define SET_LOWORD(dw,val)  ((dw) = ((dw) & 0xffff0000) | LOWORD(val))
#define SET_LOBYTE(dw,val)  ((dw) = ((dw) & 0xffffff00) | LOBYTE(val))
#define ADD_LOWORD(dw,val)  ((dw) = ((dw) & 0xffff0000) | LOWORD((DWORD)(dw)+(val)))

static inline void add_stack( CONTEXT *context, int offset )
{
    if (!IS_SELECTOR_32BIT(context->SegSs))
        ADD_LOWORD( context->Esp, offset );
    else
        context->Esp += offset;
}

static inline void *make_ptr( CONTEXT *context, DWORD seg, DWORD off, int long_addr )
{
    if (ldt_is_system(seg)) return (void *)off;
    if (!long_addr) off = LOWORD(off);
    return (char *) MapSL( MAKESEGPTR( seg, 0 ) ) + off;
}

static inline void *get_stack( CONTEXT *context )
{
    return ldt_get_ptr( context->SegSs, context->Esp );
}

#pragma pack(push,1)
struct idtr
{
    WORD  limit;
    BYTE *base;
};
#pragma pack(pop)

static LDT_ENTRY idt[256];

static inline struct idtr get_idtr(void)
{
    struct idtr ret;
#if defined(__i386__) && defined(__GNUC__)
    __asm__( "sidtl %0" : "=m" (ret) );
#else
    ret.base = (BYTE *)idt;
    ret.limit = sizeof(idt) - 1;
#endif
    return ret;
}


/***********************************************************************
 *           INSTR_ReplaceSelector
 *
 * Try to replace an invalid selector by a valid one.
 * The only selector where it is allowed to do "mov ax,40;mov es,ax"
 * is the so called 'bimodal' selector 0x40, which points to the BIOS
 * data segment. Used by (at least) Borland products (and programs compiled
 * using Borland products).
 *
 * See Undocumented Windows, Chapter 5, __0040.
 */
static BOOL INSTR_ReplaceSelector( CONTEXT *context, WORD *sel )
{
    if (*sel == 0x40)
    {
        DOSVM_start_bios_timer();
        *sel = DOSMEM_BiosDataSeg;
        return TRUE;
    }
    return FALSE;  /* Can't replace selector, crashdump */
}


/* store an operand into a register */
static void store_reg_word( CONTEXT *context, BYTE regmodrm, const BYTE *addr, int long_op )
{
    switch((regmodrm >> 3) & 7)
    {
    case 0:
        if (long_op) context->Eax = *(const DWORD *)addr;
        else SET_LOWORD(context->Eax, *(const WORD *)addr);
        break;
    case 1:
        if (long_op) context->Ecx = *(const DWORD *)addr;
        else SET_LOWORD(context->Ecx, *(const WORD *)addr);
        break;
    case 2:
        if (long_op) context->Edx = *(const DWORD *)addr;
        else SET_LOWORD(context->Edx, *(const WORD *)addr);
        break;
    case 3:
        if (long_op) context->Ebx = *(const DWORD *)addr;
        else SET_LOWORD(context->Ebx, *(const WORD *)addr);
        break;
    case 4:
        if (long_op) context->Esp = *(const DWORD *)addr;
        else SET_LOWORD(context->Esp, *(const WORD *)addr);
        break;
    case 5:
        if (long_op) context->Ebp = *(const DWORD *)addr;
        else SET_LOWORD(context->Ebp, *(const WORD *)addr);
        break;
    case 6:
        if (long_op) context->Esi = *(const DWORD *)addr;
        else SET_LOWORD(context->Esi, *(const WORD *)addr);
        break;
    case 7:
        if (long_op) context->Edi = *(const DWORD *)addr;
        else SET_LOWORD(context->Edi, *(const WORD *)addr);
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
    int mod, rm, base = 0, index = 0, ss = 0, seg = 0, off;

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
        case 0: base = context->Eax; seg = context->SegDs; break;
        case 1: base = context->Ecx; seg = context->SegDs; break;
        case 2: base = context->Edx; seg = context->SegDs; break;
        case 3: base = context->Ebx; seg = context->SegDs; break;
        case 4: base = context->Esp; seg = context->SegSs; break;
        case 5: base = context->Ebp; seg = context->SegSs; break;
        case 6: base = context->Esi; seg = context->SegDs; break;
        case 7: base = context->Edi; seg = context->SegDs; break;
        }
        switch (mod)
        {
        case 0:
            if (rm == 5)  /* special case: ds:(disp32) */
            {
                GET_VAL( &base, DWORD );
                seg = context->SegDs;
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
            seg  = context->SegDs;
            break;
        case 1:  /* ds:(bx,di) */
            base = LOWORD(context->Ebx) + LOWORD(context->Edi);
            seg  = context->SegDs;
            break;
        case 2:  /* ss:(bp,si) */
            base = LOWORD(context->Ebp) + LOWORD(context->Esi);
            seg  = context->SegSs;
            break;
        case 3:  /* ss:(bp,di) */
            base = LOWORD(context->Ebp) + LOWORD(context->Edi);
            seg  = context->SegSs;
            break;
        case 4:  /* ds:(si) */
            base = LOWORD(context->Esi);
            seg  = context->SegDs;
            break;
        case 5:  /* ds:(di) */
            base = LOWORD(context->Edi);
            seg  = context->SegDs;
            break;
        case 6:  /* ss:(bp) */
            base = LOWORD(context->Ebp);
            seg  = context->SegSs;
            break;
        case 7:  /* ds:(bx) */
            base = LOWORD(context->Ebx);
            seg  = context->SegDs;
            break;
        }

        switch(mod)
        {
        case 0:
            if (rm == 6)  /* special case: ds:(disp16) */
            {
                GET_VAL( &base, WORD );
                seg  = context->SegDs;
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
    if (segprefix != -1) seg = segprefix;

    /* Make sure the segment and offset are valid */
    if (ldt_is_system(seg)) return (BYTE *)(base + (index << ss));
    if ((seg & 7) != 7) return NULL;
    if (!ldt_is_valid( seg )) return NULL;
    if (ldt_get_limit( seg ) < (base + (index << ss))) return NULL;
    return (BYTE *)ldt_get_base( seg ) + base + (index << ss);
#undef GET_VAL
}


/***********************************************************************
 *           INSTR_EmulateLDS
 *
 * Emulate the LDS (and LES,LFS,etc.) instruction.
 */
static BOOL INSTR_EmulateLDS( CONTEXT *context, BYTE *instr, int long_op,
                              int long_addr, int segprefix, int *len )
{
    WORD seg;
    BYTE *regmodrm = instr + 1 + (*instr == 0x0f);
    BYTE *addr = INSTR_GetOperandAddr( context, regmodrm,
                                       long_addr, segprefix, len );
    if (!addr)
        return FALSE;  /* Unable to emulate it */
    seg = *(WORD *)(addr + (long_op ? 4 : 2));

    if (!INSTR_ReplaceSelector( context, &seg ))
        return FALSE;  /* Unable to emulate it */

    /* Now store the offset in the correct register */

    store_reg_word( context, *regmodrm, addr, long_op );

    /* Store the correct segment in the segment register */

    switch(*instr)
    {
    case 0xc4: context->SegEs = seg; break;  /* les */
    case 0xc5: context->SegDs = seg; break;  /* lds */
    case 0x0f: switch(instr[1])
               {
               case 0xb2: context->SegSs = seg; break;  /* lss */
               case 0xb4: context->SegFs = seg; break;  /* lfs */
               case 0xb5: context->SegGs = seg; break;  /* lgs */
               }
               break;
    }

    /* Add the opcode size to the total length */

    *len += 1 + (*instr == 0x0f);
    return TRUE;
}

/***********************************************************************
 *           INSTR_inport
 *
 * input on an I/O port
 */
static DWORD INSTR_inport( WORD port, int size, CONTEXT *context )
{
    DWORD res = DOSVM_inport( port, size );

    if (TRACE_ON(io))
    {
        switch(size)
        {
        case 1:
            TRACE_(io)( "0x%x < %02x @ %04x:%04x\n", port, LOBYTE(res),
                     (WORD)context->SegCs, LOWORD(context->Eip));
            break;
        case 2:
            TRACE_(io)( "0x%x < %04x @ %04x:%04x\n", port, LOWORD(res),
                     (WORD)context->SegCs, LOWORD(context->Eip));
            break;
        case 4:
            TRACE_(io)( "0x%x < %08lx @ %04x:%04x\n", port, res,
                     (WORD)context->SegCs, LOWORD(context->Eip));
            break;
        }
    }
    return res;
}


/***********************************************************************
 *           INSTR_outport
 *
 * output on an I/O port
 */
static void INSTR_outport( WORD port, int size, DWORD val, CONTEXT *context )
{
    DOSVM_outport( port, size, val );

    if (TRACE_ON(io))
    {
        switch(size)
        {
        case 1:
            TRACE_(io)("0x%x > %02x @ %04x:%04x\n", port, LOBYTE(val),
                    (WORD)context->SegCs, LOWORD(context->Eip));
            break;
        case 2:
            TRACE_(io)("0x%x > %04x @ %04x:%04x\n", port, LOWORD(val),
                    (WORD)context->SegCs, LOWORD(context->Eip));
            break;
        case 4:
            TRACE_(io)("0x%x > %08lx @ %04x:%04x\n", port, val,
                    (WORD)context->SegCs, LOWORD(context->Eip));
            break;
        }
    }
}


/***********************************************************************
 *           __wine_emulate_instruction
 *
 * Emulate a privileged instruction.
 * Returns exception continuation status.
 */
DWORD __wine_emulate_instruction( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    int prefix, segprefix, prefixlen, len, repX, long_op, long_addr;
    BYTE *instr;

    long_op = long_addr = IS_SELECTOR_32BIT(context->SegCs);
    instr = make_ptr( context, context->SegCs, context->Eip, TRUE );
    if (!instr) return ExceptionContinueSearch;

    /* First handle any possible prefix */

    segprefix = -1;  /* no prefix */
    prefix = 1;
    repX = 0;
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
	    repX = 1;
	    break;
        case 0xf3:  /* repe */
	    repX = 2;
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
        case 0x07: /* pop es */
        case 0x17: /* pop ss */
        case 0x1f: /* pop ds */
            {
                WORD seg = *(WORD *)get_stack( context );
                if (INSTR_ReplaceSelector( context, &seg ))
                {
                    switch(*instr)
                    {
                    case 0x07: context->SegEs = seg; break;
                    case 0x17: context->SegSs = seg; break;
                    case 0x1f: context->SegDs = seg; break;
                    }
                    add_stack(context, long_op ? 4 : 2);
                    context->Eip += prefixlen + 1;
                    return ExceptionContinueExecution;
                }
            }
            break;  /* Unable to emulate it */

        case 0x0f: /* extended instruction */
            switch(instr[1])
            {
            case 0x22: /* mov %eax, %crX */
                switch (instr[2])
                {
                case 0xc0:
                    FIXME("mov %%eax, %%cr0 at 0x%08lx, EAX=0x%08lx\n",
                          context->Eip,context->Eax );
                          context->Eip += prefixlen+3;
                    return ExceptionContinueExecution;
                default:
                    break; /* Fallthrough to bad instruction handling */
                }
                break; /* Fallthrough to bad instruction handling */
            case 0x20: /* mov %crX, %eax */
                switch (instr[2])
                {
                case 0xe0: /* mov %cr4, %eax */
                    /* CR4 register: See linux/arch/i386/mm/init.c, X86_CR4_ defs
                     * bit 0: VME Virtual Mode Exception ?
                     * bit 1: PVI Protected mode Virtual Interrupt
                     * bit 2: TSD Timestamp disable
                     * bit 3: DE  Debugging extensions
                     * bit 4: PSE Page size extensions
                     * bit 5: PAE Physical address extension
                     * bit 6: MCE Machine check enable
                     * bit 7: PGE Enable global pages
                     * bit 8: PCE Enable performance counters at IPL3
                     */
                    FIXME("mov %%cr4, %%eax at 0x%08lx\n",context->Eip);
                    context->Eax = 0;
                    context->Eip += prefixlen+3;
                    return ExceptionContinueExecution;
                case 0xc0: /* mov %cr0, %eax */
                    FIXME("mov %%cr0, %%eax at 0x%08lx\n",context->Eip);
                    context->Eax = 0x10; /* FIXME: set more bits ? */
                    context->Eip += prefixlen+3;
                    return ExceptionContinueExecution;
                default: /* Fallthrough to illegal instruction */
                    break;
                }
                /* Fallthrough to illegal instruction */
                break;
            case 0x21: /* mov %drX, %eax */
                switch (instr[2])
                {
                case 0xc8: /* mov %dr1, %eax */
                    TRACE("mov %%dr1, %%eax at 0x%08lx\n",context->Eip);
                    context->Eax = context->Dr1;
                    context->Eip += prefixlen+3;
                    return ExceptionContinueExecution;
                case 0xf8: /* mov %dr7, %eax */
                    TRACE("mov %%dr7, %%eax at 0x%08lx\n",context->Eip);
                    context->Eax = 0x400;
                    context->Eip += prefixlen+3;
                    return ExceptionContinueExecution;
                }
                FIXME("Unsupported DR register, eip+2 is %02x\n", instr[2]);
                /* fallthrough to illegal instruction */
                break;
            case 0x23: /* mov %eax, %drX */
                switch (instr[2])
                {
                case 0xc8: /* mov %eax, %dr1 */
                    context->Dr1 = context->Eax;
                    context->Eip += prefixlen+3;
                    return ExceptionContinueExecution;
                }
                FIXME("Unsupported DR register, eip+2 is %02x\n", instr[2]);
                /* fallthrough to illegal instruction */
                break;
            case 0xa1: /* pop fs */
                {
                    WORD seg = *(WORD *)get_stack( context );
                    if (INSTR_ReplaceSelector( context, &seg ))
                    {
                        context->SegFs = seg;
                        add_stack(context, long_op ? 4 : 2);
                        context->Eip += prefixlen + 2;
                        return ExceptionContinueExecution;
                    }
                }
                break;
            case 0xa9: /* pop gs */
                {
                    WORD seg = *(WORD *)get_stack( context );
                    if (INSTR_ReplaceSelector( context, &seg ))
                    {
                        context->SegGs = seg;
                        add_stack(context, long_op ? 4 : 2);
                        context->Eip += prefixlen + 2;
                        return ExceptionContinueExecution;
                    }
                }
                break;
            case 0xb2: /* lss addr,reg */
            case 0xb4: /* lfs addr,reg */
            case 0xb5: /* lgs addr,reg */
                if (INSTR_EmulateLDS( context, instr, long_op,
                                      long_addr, segprefix, &len ))
                {
                    context->Eip += prefixlen + len;
                    return ExceptionContinueExecution;
                }
                break;
            }
            break;  /* Unable to emulate it */

        case 0x6c: /* insb     */
        case 0x6d: /* insw/d   */
        case 0x6e: /* outsb    */
        case 0x6f: /* outsw/d  */
	    {
	      int typ = *instr;  /* Just in case it's overwritten.  */
	      int outp = (typ >= 0x6e);
	      unsigned long count = repX ?
                          (long_addr ? context->Ecx : LOWORD(context->Ecx)) : 1;
	      int opsize = (typ & 1) ? (long_op ? 4 : 2) : 1;
	      int step = (context->EFlags & 0x400) ? -opsize : +opsize;
	      int seg;

	      if (outp)
              {
		/* Check if there is a segment prefix override and honour it */
		seg = segprefix == -1 ? context->SegDs : segprefix;
		/* FIXME: Check segment is readable.  */
              }
	      else
              {
		seg = context->SegEs;
		/* FIXME: Check segment is writable.  */
              }

	      if (repX)
              {
		if (long_addr) context->Ecx = 0;
		else SET_LOWORD(context->Ecx,0);
              }

	      while (count-- > 0)
		{
		  void *data;
                  WORD dx = LOWORD(context->Edx);
		  if (outp)
                  {
                      data = make_ptr( context, seg, context->Esi, long_addr );
                      if (long_addr) context->Esi += step;
                      else ADD_LOWORD(context->Esi,step);
                  }
		  else
                  {
                      data = make_ptr( context, seg, context->Edi, long_addr );
                      if (long_addr) context->Edi += step;
                      else ADD_LOWORD(context->Edi,step);
                  }

		  switch (typ)
                  {
		    case 0x6c:
		      *(BYTE *)data = INSTR_inport( dx, 1, context );
		      break;
		    case 0x6d:
		      if (long_op)
                          *(DWORD *)data = INSTR_inport( dx, 4, context );
		      else
                          *(WORD *)data = INSTR_inport( dx, 2, context );
		      break;
		    case 0x6e:
                        INSTR_outport( dx, 1, *(BYTE *)data, context );
                        break;
		    case 0x6f:
                        if (long_op)
                            INSTR_outport( dx, 4, *(DWORD *)data, context );
                        else
                            INSTR_outport( dx, 2, *(WORD *)data, context );
                        break;
		    }
		}
              context->Eip += prefixlen + 1;
	    }
            return ExceptionContinueExecution;

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
            }
            break;  /* Unable to emulate it */

        case 0x8e: /* mov XX,segment_reg */
            {
                WORD seg;
                BYTE *addr = INSTR_GetOperandAddr(context, instr + 1,
                                                  long_addr, segprefix, &len );
                if (!addr)
                    break;  /* Unable to emulate it */
                seg = *(WORD *)addr;
                if (!INSTR_ReplaceSelector( context, &seg ))
                    break;  /* Unable to emulate it */

                switch((instr[1] >> 3) & 7)
                {
                case 0:
                    context->SegEs = seg;
                    context->Eip += prefixlen + len + 1;
                    return ExceptionContinueExecution;
                case 1:  /* cs */
                    break;
                case 2:
                    context->SegSs = seg;
                    context->Eip += prefixlen + len + 1;
                    return ExceptionContinueExecution;
                case 3:
                    context->SegDs = seg;
                    context->Eip += prefixlen + len + 1;
                    return ExceptionContinueExecution;
                case 4:
                    context->SegFs = seg;
                    context->Eip += prefixlen + len + 1;
                    return ExceptionContinueExecution;
                case 5:
                    context->SegGs = seg;
                    context->Eip += prefixlen + len + 1;
                    return ExceptionContinueExecution;
                case 6:  /* unused */
                case 7:  /* unused */
                    break;
                }
            }
            break;  /* Unable to emulate it */

        case 0xc4: /* les addr,reg */
        case 0xc5: /* lds addr,reg */
            if (INSTR_EmulateLDS( context, instr, long_op,
                                  long_addr, segprefix, &len ))
            {
                context->Eip += prefixlen + len;
                return ExceptionContinueExecution;
            }
            break;  /* Unable to emulate it */

        case 0xcd: /* int <XX> */
            context->Eip += prefixlen + 2;
            if (DOSVM_EmulateInterruptPM( context, instr[1] )) return ExceptionContinueExecution;
            context->Eip -= prefixlen + 2;  /* restore eip */
            break;  /* Unable to emulate it */

        case 0xcf: /* iret */
            if (ldt_is_system(context->SegCs)) break;  /* don't emulate it in 32-bit code */
            if (long_op)
            {
                DWORD *stack = get_stack( context );
                context->Eip = *stack++;
                context->SegCs  = *stack++;
                context->EFlags = *stack;
                add_stack(context, 3*sizeof(DWORD));  /* Pop the return address and flags */
            }
            else
            {
                WORD *stack = get_stack( context );
                context->Eip = *stack++;
                context->SegCs  = *stack++;
                SET_LOWORD(context->EFlags,*stack);
                add_stack(context, 3*sizeof(WORD));  /* Pop the return address and flags */
            }
            return ExceptionContinueExecution;

        case 0xe4: /* inb al,XX */
            SET_LOBYTE(context->Eax,INSTR_inport( instr[1], 1, context ));
            context->Eip += prefixlen + 2;
            return ExceptionContinueExecution;

        case 0xe5: /* in (e)ax,XX */
            if (long_op)
                context->Eax = INSTR_inport( instr[1], 4, context );
            else
                SET_LOWORD(context->Eax, INSTR_inport( instr[1], 2, context ));
            context->Eip += prefixlen + 2;
            return ExceptionContinueExecution;

        case 0xe6: /* outb XX,al */
            INSTR_outport( instr[1], 1, LOBYTE(context->Eax), context );
            context->Eip += prefixlen + 2;
            return ExceptionContinueExecution;

        case 0xe7: /* out XX,(e)ax */
            if (long_op)
                INSTR_outport( instr[1], 4, context->Eax, context );
            else
                INSTR_outport( instr[1], 2, LOWORD(context->Eax), context );
            context->Eip += prefixlen + 2;
            return ExceptionContinueExecution;

        case 0xec: /* inb al,dx */
            SET_LOBYTE(context->Eax, INSTR_inport( LOWORD(context->Edx), 1, context ) );
            context->Eip += prefixlen + 1;
            return ExceptionContinueExecution;

        case 0xed: /* in (e)ax,dx */
            if (long_op)
                context->Eax = INSTR_inport( LOWORD(context->Edx), 4, context );
            else
                SET_LOWORD(context->Eax, INSTR_inport( LOWORD(context->Edx), 2, context ));
            context->Eip += prefixlen + 1;
            return ExceptionContinueExecution;

        case 0xee: /* outb dx,al */
            INSTR_outport( LOWORD(context->Edx), 1, LOBYTE(context->Eax), context );
            context->Eip += prefixlen + 1;
            return ExceptionContinueExecution;

        case 0xef: /* out dx,(e)ax */
            if (long_op)
                INSTR_outport( LOWORD(context->Edx), 4, context->Eax, context );
            else
                INSTR_outport( LOWORD(context->Edx), 2, LOWORD(context->Eax), context );
            context->Eip += prefixlen + 1;
            return ExceptionContinueExecution;

        case 0xfa: /* cli */
            context->Eip += prefixlen + 1;
            return ExceptionContinueExecution;

        case 0xfb: /* sti */
            context->Eip += prefixlen + 1;
            return ExceptionContinueExecution;
    }
    return ExceptionContinueSearch;  /* Unable to emulate it */
}


/***********************************************************************
 *           INSTR_vectored_handler
 *
 * Vectored exception handler used to emulate protected instructions
 * from 32-bit code.
 */
LONG CALLBACK INSTR_vectored_handler( EXCEPTION_POINTERS *ptrs )
{
    EXCEPTION_RECORD *record = ptrs->ExceptionRecord;
    CONTEXT *context = ptrs->ContextRecord;

    if (ldt_is_system(context->SegCs) &&
        (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
         record->ExceptionCode == EXCEPTION_PRIV_INSTRUCTION))
    {
        if (__wine_emulate_instruction( record, context ) == ExceptionContinueExecution)
            return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}


/***********************************************************************
 *           DOS3Call         (KERNEL.102)
 */
void WINAPI DOS3Call( CONTEXT *context )
{
    __wine_call_int_handler16( 0x21, context );
}


/***********************************************************************
 *           NetBIOSCall      (KERNEL.103)
 */
void WINAPI NetBIOSCall16( CONTEXT *context )
{
    __wine_call_int_handler16( 0x5c, context );
}


/***********************************************************************
 *		GetSetKernelDOSProc (KERNEL.311)
 */
FARPROC16 WINAPI GetSetKernelDOSProc16( FARPROC16 DosProc )
{
    FIXME("(DosProc=%p): stub\n", DosProc);
    return NULL;
}
