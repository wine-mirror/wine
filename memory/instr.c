/*
 * Emulation of priviledged instructions
 *
 * Copyright 1995 Alexandre Julliard
 */

#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "ldt.h"
#include "global.h"
#include "module.h"
#include "dosexe.h"
#include "miscemu.h"
#include "selectors.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(int);
DECLARE_DEBUG_CHANNEL(io);

#ifdef __i386__

#define IS_SEL_32(context,seg) \
   (ISV86(context) ? FALSE : IS_SELECTOR_32BIT(seg))

#define STACK_reg(context) \
   (IS_SEL_32(context,SS_reg(context)) ? ESP_reg(context) : (DWORD)LOWORD(ESP_reg(context)))

#define ADD_STACK_reg(context,offset) \
   do { if (IS_SEL_32(context,SS_reg(context))) ESP_reg(context) += (offset); \
        else ADD_LOWORD(ESP_reg(context),(offset)); } while(0)

#define MAKE_PTR(seg,off) \
   (IS_SELECTOR_SYSTEM(seg) ? (void *)(off) : PTR_SEG_OFF_TO_LIN(seg,off))

#define MK_PTR(context,seg,off) \
   (ISV86(context) ? DOSMEM_MapRealToLinear(MAKELONG(off,seg)) \
                   : MAKE_PTR(seg,off))

#define STACK_PTR(context) \
   (ISV86(context) ? \
    DOSMEM_MapRealToLinear(MAKELONG(LOWORD(ESP_reg(context)),SS_reg(context))) : \
    (IS_SELECTOR_SYSTEM(SS_reg(context)) ? (void *)ESP_reg(context) : \
     (PTR_SEG_OFF_TO_LIN(SS_reg(context),STACK_reg(context)))))

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
static BOOL INSTR_ReplaceSelector( CONTEXT86 *context, WORD *sel )
{
    extern char Call16_Start, Call16_End;

    if (IS_SELECTOR_SYSTEM(CS_reg(context)))
        if (    (char *)EIP_reg(context) >= &Call16_Start 
             && (char *)EIP_reg(context) <  &Call16_End   )
        {
            /* Saved selector may have become invalid when the relay code */
            /* tries to restore it. We simply clear it. */
            *sel = 0;
            return TRUE;
        }

    if (*sel == 0x40)
    {
        static WORD sys_timer = 0;
        if (!sys_timer)
            sys_timer = CreateSystemTimer( 55, DOSMEM_Tick );
        *sel = DOSMEM_BiosDataSeg;
        return TRUE;
    }
    if (!IS_SELECTOR_SYSTEM(*sel) && !IS_SELECTOR_FREE(*sel))
        ERR("Got protection fault on valid selector, maybe your kernel is too old?\n" );
    return FALSE;  /* Can't replace selector, crashdump */
}


/***********************************************************************
 *           INSTR_GetOperandAddr
 *
 * Return the address of an instruction operand (from the mod/rm byte).
 */
static BYTE *INSTR_GetOperandAddr( CONTEXT86 *context, BYTE *instr,
                                   int long_addr, int segprefix, int *len )
{
    int mod, rm, base, index = 0, ss = 0, seg = 0, off;

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
        case 0: return (BYTE *)&EAX_reg(context);
        case 1: return (BYTE *)&ECX_reg(context);
        case 2: return (BYTE *)&EDX_reg(context);
        case 3: return (BYTE *)&EBX_reg(context);
        case 4: return (BYTE *)&ESP_reg(context);
        case 5: return (BYTE *)&EBP_reg(context);
        case 6: return (BYTE *)&ESI_reg(context);
        case 7: return (BYTE *)&EDI_reg(context);
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
            switch(sib >> 3)
            {
            case 0: index = EAX_reg(context); break;
            case 1: index = ECX_reg(context); break;
            case 2: index = EDX_reg(context); break;
            case 3: index = EBX_reg(context); break;
            case 4: index = 0; break;
            case 5: index = EBP_reg(context); break;
            case 6: index = ESI_reg(context); break;
            case 7: index = EDI_reg(context); break;
            }
        }

        switch(rm)
        {
        case 0: base = EAX_reg(context); seg = DS_reg(context); break;
        case 1: base = ECX_reg(context); seg = DS_reg(context); break;
        case 2: base = EDX_reg(context); seg = DS_reg(context); break;
        case 3: base = EBX_reg(context); seg = DS_reg(context); break;
        case 4: base = ESP_reg(context); seg = SS_reg(context); break;
        case 5: base = EBP_reg(context); seg = SS_reg(context); break;
        case 6: base = ESI_reg(context); seg = DS_reg(context); break;
        case 7: base = EDI_reg(context); seg = DS_reg(context); break;
        }
        switch (mod)
        {
        case 0:
            if (rm == 5)  /* special case: ds:(disp32) */
            {
                GET_VAL( &base, DWORD );
                seg = DS_reg(context);
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
            base = LOWORD(EBX_reg(context)) + LOWORD(ESI_reg(context));
            seg  = DS_reg(context);
            break;
        case 1:  /* ds:(bx,di) */
            base = LOWORD(EBX_reg(context)) + LOWORD(EDI_reg(context));
            seg  = DS_reg(context);
            break;
        case 2:  /* ss:(bp,si) */
            base = LOWORD(EBP_reg(context)) + LOWORD(ESI_reg(context));
            seg  = SS_reg(context);
            break;
        case 3:  /* ss:(bp,di) */
            base = LOWORD(EBP_reg(context)) + LOWORD(EDI_reg(context));
            seg  = SS_reg(context);
            break;
        case 4:  /* ds:(si) */
            base = LOWORD(ESI_reg(context));
            seg  = DS_reg(context);
            break;
        case 5:  /* ds:(di) */
            base = LOWORD(EDI_reg(context));
            seg  = DS_reg(context);
            break;
        case 6:  /* ss:(bp) */
            base = LOWORD(EBP_reg(context));
            seg  = SS_reg(context);
            break;
        case 7:  /* ds:(bx) */
            base = LOWORD(EBX_reg(context));
            seg  = DS_reg(context);
            break;
        }

        switch(mod)
        {
        case 0:
            if (rm == 6)  /* special case: ds:(disp16) */
            {
                GET_VAL( &base, WORD );
                seg  = DS_reg(context);
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
    if (IS_SELECTOR_SYSTEM(seg)) return (BYTE *)(base + (index << ss));
    if (((seg & 7) != 7) || IS_SELECTOR_FREE(seg)) return NULL;
    if (GET_SEL_LIMIT(seg) < (base + (index << ss))) return NULL;
    return (BYTE *)PTR_SEG_OFF_TO_LIN( seg, (base + (index << ss)) );
#undef GET_VAL
}


/***********************************************************************
 *           INSTR_EmulateLDS
 *
 * Emulate the LDS (and LES,LFS,etc.) instruction.
 */
static BOOL INSTR_EmulateLDS( CONTEXT86 *context, BYTE *instr, int long_op,
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

    switch((*regmodrm >> 3) & 7)
    {
    case 0:
        if (long_op) EAX_reg(context) = *(DWORD *)addr;
        else SET_LOWORD(EAX_reg(context),*(WORD *)addr);
        break;
    case 1:
        if (long_op) ECX_reg(context) = *(DWORD *)addr;
        else SET_LOWORD(ECX_reg(context),*(WORD *)addr);
        break;
    case 2:
        if (long_op) EDX_reg(context) = *(DWORD *)addr;
        else SET_LOWORD(EDX_reg(context),*(WORD *)addr);
        break;
    case 3:
        if (long_op) EBX_reg(context) = *(DWORD *)addr;
        else SET_LOWORD(EBX_reg(context),*(WORD *)addr);
        break;
    case 4:
        if (long_op) ESP_reg(context) = *(DWORD *)addr;
        else SET_LOWORD(ESP_reg(context),*(WORD *)addr);
        break;
    case 5:
        if (long_op) EBP_reg(context) = *(DWORD *)addr;
        else SET_LOWORD(EBP_reg(context),*(WORD *)addr);
        break;
    case 6:
        if (long_op) ESI_reg(context) = *(DWORD *)addr;
        else SET_LOWORD(ESI_reg(context),*(WORD *)addr);
        break;
    case 7:
        if (long_op) EDI_reg(context) = *(DWORD *)addr;
        else SET_LOWORD(EDI_reg(context),*(WORD *)addr);
        break;
    }

    /* Store the correct segment in the segment register */

    switch(*instr)
    {
    case 0xc4: ES_reg(context) = seg; break;  /* les */
    case 0xc5: DS_reg(context) = seg; break;  /* lds */
    case 0x0f: switch(instr[1])
               {
               case 0xb2: SS_reg(context) = seg; break;  /* lss */
               case 0xb4: FS_reg(context) = seg; break;  /* lfs */
               case 0xb5: GS_reg(context) = seg; break;  /* lgs */
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
 * input on a I/O port
 */
static DWORD INSTR_inport( WORD port, int size, CONTEXT86 *context )
{
    DWORD res = IO_inport( port, size );
    if (TRACE_ON(io))
    {
        switch(size)
        {
        case 1:
            DPRINTF( "0x%x < %02x @ %04x:%04x\n", port, LOBYTE(res),
                     (WORD)CS_reg(context), LOWORD(EIP_reg(context)));
            break;
        case 2:
            DPRINTF( "0x%x < %04x @ %04x:%04x\n", port, LOWORD(res),
                     (WORD)CS_reg(context), LOWORD(EIP_reg(context)));
            break;
        case 4:
            DPRINTF( "0x%x < %08lx @ %04x:%04x\n", port, res,
                     (WORD)CS_reg(context), LOWORD(EIP_reg(context)));
            break;
        }
    }
    return res;
}


/***********************************************************************
 *           INSTR_outport
 *
 * output on a I/O port
 */
static void INSTR_outport( WORD port, int size, DWORD val, CONTEXT86 *context )
{
    IO_outport( port, size, val );
    if (TRACE_ON(io))
    {
        switch(size)
        {
        case 1:
            DPRINTF("0x%x > %02x @ %04x:%04x\n", port, LOBYTE(val),
                    (WORD)CS_reg(context), LOWORD(EIP_reg(context)));
            break;
        case 2:
            DPRINTF("0x%x > %04x @ %04x:%04x\n", port, LOWORD(val),
                    (WORD)CS_reg(context), LOWORD(EIP_reg(context)));
            break;
        case 4:
            DPRINTF("0x%x > %08lx @ %04x:%04x\n", port, val,
                    (WORD)CS_reg(context), LOWORD(EIP_reg(context)));
            break;
        }
    }
}


/***********************************************************************
 *           INSTR_EmulateInstruction
 *
 * Emulate a priviledged instruction. Returns TRUE if emulation successful.
 */
BOOL INSTR_EmulateInstruction( CONTEXT86 *context )
{
    int prefix, segprefix, prefixlen, len, repX, long_op, long_addr;
    SEGPTR gpHandler;
    BYTE *instr;

    long_op = long_addr = IS_SEL_32(context,CS_reg(context));
    instr = (BYTE *)MK_PTR(context,CS_reg(context),EIP_reg(context));
    if (!instr) return FALSE;

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
            segprefix = CS_reg(context);
            break;
        case 0x36:
            segprefix = SS_reg(context);
            break;
        case 0x3e:
            segprefix = DS_reg(context);
            break;
        case 0x26:
            segprefix = ES_reg(context);
            break;
        case 0x64:
            segprefix = FS_reg(context);
            break;
        case 0x65:
            segprefix = GS_reg(context);
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
                WORD seg = *(WORD *)STACK_PTR( context );
                if (INSTR_ReplaceSelector( context, &seg ))
                {
                    switch(*instr)
                    {
                    case 0x07: ES_reg(context) = seg; break;
                    case 0x17: SS_reg(context) = seg; break;
                    case 0x1f: DS_reg(context) = seg; break;
                    }
                    ADD_STACK_reg(context, long_op ? 4 : 2);
                    EIP_reg(context) += prefixlen + 1;
                    return TRUE;
                }
            }
            break;  /* Unable to emulate it */

        case 0x0f: /* extended instruction */
            switch(instr[1])
            {
	    case 0x22: /* mov eax, crX */
	    	switch (instr[2]) {
		case 0xc0:
			ERR("mov eax,cr0 at 0x%08lx, EAX=0x%08lx\n",
                            EIP_reg(context),EAX_reg(context) );
			EIP_reg(context) += prefixlen+3;
			return TRUE;
		default:
			break; /*fallthrough to bad instruction handling */
		}
		break; /*fallthrough to bad instruction handling */
	    case 0x20: /* mov crX, eax */
	        switch (instr[2]) {
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
		    ERR("mov cr4,eax at 0x%08lx\n",EIP_reg(context));
		    EAX_reg(context) = 0;
		    EIP_reg(context) += prefixlen+3;
		    return TRUE;
		case 0xc0: /* mov cr0, eax */
		    ERR("mov cr0,eax at 0x%08lx\n",EIP_reg(context));
		    EAX_reg(context) = 0x10; /* FIXME: set more bits ? */
		    EIP_reg(context) += prefixlen+3;
		    return TRUE;
		default: /* fallthrough to illegal instruction */
		    break;
		}
		/* fallthrough to illegal instruction */
		break;
            case 0xa1: /* pop fs */
                {
                    WORD seg = *(WORD *)STACK_PTR( context );
                    if (INSTR_ReplaceSelector( context, &seg ))
                    {
                        FS_reg(context) = seg;
                        ADD_STACK_reg(context, long_op ? 4 : 2);
                        EIP_reg(context) += prefixlen + 2;
                        return TRUE;
                    }
                }
                break;
            case 0xa9: /* pop gs */
                {
                    WORD seg = *(WORD *)STACK_PTR( context );
                    if (INSTR_ReplaceSelector( context, &seg ))
                    {
                        GS_reg(context) = seg;
                        ADD_STACK_reg(context, long_op ? 4 : 2);
                        EIP_reg(context) += prefixlen + 2;
                        return TRUE;
                    }
                }
                break;
            case 0xb2: /* lss addr,reg */
            case 0xb4: /* lfs addr,reg */
            case 0xb5: /* lgs addr,reg */
                if (INSTR_EmulateLDS( context, instr, long_op,
                                      long_addr, segprefix, &len ))
                {
                    EIP_reg(context) += prefixlen + len;
                    return TRUE;
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
                          (long_addr ? ECX_reg(context) : LOWORD(ECX_reg(context))) : 1;
	      int opsize = (typ & 1) ? (long_op ? 4 : 2) : 1;
	      int step = (EFL_reg(context) & 0x400) ? -opsize : +opsize;
	      int seg = outp ? DS_reg(context) : ES_reg(context);  /* FIXME: is this right? */

	      if (outp)
		/* FIXME: Check segment readable.  */
		(void)0;
	      else
		/* FIXME: Check segment writeable.  */
		(void)0;

	      if (repX)
              {
		if (long_addr) ECX_reg(context) = 0;
		else SET_LOWORD(ECX_reg(context),0);
              }

	      while (count-- > 0)
		{
		  void *data;
                  WORD dx = LOWORD(EDX_reg(context));
		  if (outp)
                  {
		      data = MK_PTR(context, seg,
                                    long_addr ? ESI_reg(context) : LOWORD(ESI_reg(context)));
		      if (long_addr) ESI_reg(context) += step;
		      else ADD_LOWORD(ESI_reg(context),step);
                  }
		  else
                  {
		      data = MK_PTR(context, seg,
                                    long_addr ? EDI_reg(context) : LOWORD(EDI_reg(context)));
		      if (long_addr) EDI_reg(context) += step;
		      else ADD_LOWORD(EDI_reg(context),step);
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
              EIP_reg(context) += prefixlen + 1;
	    }
            return TRUE;

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
                    ES_reg(context) = seg;
                    EIP_reg(context) += prefixlen + len + 1;
                    return TRUE;
                case 1:  /* cs */
                    break;
                case 2:
                    SS_reg(context) = seg;
                    EIP_reg(context) += prefixlen + len + 1;
                    return TRUE;
                case 3:
                    DS_reg(context) = seg;
                    EIP_reg(context) += prefixlen + len + 1;
                    return TRUE;
                case 4:
                    FS_reg(context) = seg;
                    EIP_reg(context) += prefixlen + len + 1;
                    return TRUE;
                case 5:
                    GS_reg(context) = seg;
                    EIP_reg(context) += prefixlen + len + 1;
                    return TRUE;
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
                EIP_reg(context) += prefixlen + len;
                return TRUE;
            }
            break;  /* Unable to emulate it */
            
        case 0xcd: /* int <XX> */
            if (long_op)
            {
                ERR("int xx from 32-bit code is not supported.\n");
                break;  /* Unable to emulate it */
            }
            else
            {
                FARPROC16 addr = INT_GetPMHandler( instr[1] );
                WORD *stack = (WORD *)STACK_PTR( context );
                /* Push the flags and return address on the stack */
                *(--stack) = LOWORD(EFL_reg(context));
                *(--stack) = CS_reg(context);
                *(--stack) = LOWORD(EIP_reg(context)) + prefixlen + 2;
                ADD_STACK_reg(context, -3 * sizeof(WORD));
                /* Jump to the interrupt handler */
                CS_reg(context)  = HIWORD(addr);
                EIP_reg(context) = LOWORD(addr);
            }
            return TRUE;

        case 0xcf: /* iret */
            if (long_op)
            {
                DWORD *stack = (DWORD *)STACK_PTR( context );
                EIP_reg(context) = *stack++;
                CS_reg(context)  = *stack++;
                EFL_reg(context) = *stack;
                ADD_STACK_reg(context, 3*sizeof(DWORD));  /* Pop the return address and flags */
            }
            else
            {
                WORD *stack = (WORD *)STACK_PTR( context );
                EIP_reg(context) = *stack++;
                CS_reg(context)  = *stack++;
                SET_LOWORD(EFL_reg(context),*stack);
                ADD_STACK_reg(context, 3*sizeof(WORD));  /* Pop the return address and flags */
            }
            return TRUE;

        case 0xe4: /* inb al,XX */
            SET_LOBYTE(EAX_reg(context),INSTR_inport( instr[1], 1, context ));
	    EIP_reg(context) += prefixlen + 2;
            return TRUE;

        case 0xe5: /* in (e)ax,XX */
            if (long_op)
                EAX_reg(context) = INSTR_inport( instr[1], 4, context );
            else
                SET_LOWORD(EAX_reg(context), INSTR_inport( instr[1], 2, context ));
	    EIP_reg(context) += prefixlen + 2;
            return TRUE;

        case 0xe6: /* outb XX,al */
            INSTR_outport( instr[1], 1, LOBYTE(EAX_reg(context)), context );
	    EIP_reg(context) += prefixlen + 2;
            return TRUE;

        case 0xe7: /* out XX,(e)ax */
            if (long_op)
                INSTR_outport( instr[1], 4, EAX_reg(context), context );
            else
                INSTR_outport( instr[1], 2, LOWORD(EAX_reg(context)), context );
  	    EIP_reg(context) += prefixlen + 2;
            return TRUE;

        case 0xec: /* inb al,dx */
            SET_LOBYTE(EAX_reg(context), INSTR_inport( LOWORD(EDX_reg(context)), 1, context ) );
  	    EIP_reg(context) += prefixlen + 1;
            return TRUE;

        case 0xed: /* in (e)ax,dx */
            if (long_op)
                EAX_reg(context) = INSTR_inport( LOWORD(EDX_reg(context)), 4, context );
            else
                SET_LOWORD(EAX_reg(context), INSTR_inport( LOWORD(EDX_reg(context)), 2, context ));
  	    EIP_reg(context) += prefixlen + 1;
            return TRUE;

        case 0xee: /* outb dx,al */
            INSTR_outport( LOWORD(EDX_reg(context)), 1, LOBYTE(EAX_reg(context)), context );
  	    EIP_reg(context) += prefixlen + 1;
            return TRUE;
      
        case 0xef: /* out dx,(e)ax */
            if (long_op)
                INSTR_outport( LOWORD(EDX_reg(context)), 4, EAX_reg(context), context );
            else
                INSTR_outport( LOWORD(EDX_reg(context)), 2, LOWORD(EAX_reg(context)), context );
  	    EIP_reg(context) += prefixlen + 1;
            return TRUE;

        case 0xfa: /* cli, ignored */
  	    EIP_reg(context) += prefixlen + 1;
            return TRUE;

        case 0xfb: /* sti, ignored */
  	    EIP_reg(context) += prefixlen + 1;
            return TRUE;
    }


    /* Check for Win16 __GP handler */
    gpHandler = HasGPHandler16( PTR_SEG_OFF_TO_SEGPTR( CS_reg(context),
                                                       EIP_reg(context) ) );
    if (gpHandler)
    {
        WORD *stack = (WORD *)STACK_PTR( context );
        *--stack = CS_reg(context);
        *--stack = EIP_reg(context);
        ADD_STACK_reg(context, -2*sizeof(WORD));

        CS_reg(context) = SELECTOROF( gpHandler );
        EIP_reg(context) = OFFSETOF( gpHandler );
        return TRUE;
    }
    return FALSE;  /* Unable to emulate it */
}

#endif  /* __i386__ */
