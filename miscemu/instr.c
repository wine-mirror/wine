/*
 * Emulation of priviledged instructions
 *
 * Copyright 1995 Alexandre Julliard
 */

#include "windows.h"
#include "ldt.h"
#include "global.h"
#include "module.h"
#include "dosexe.h"
#include "miscemu.h"
#include "sig_context.h"
#include "debug.h"


#define IS_V86(context) (EFL_sig(context)&V86_FLAG)
#define IS_SEL_32(context,seg) \
   (IS_V86(context) ? FALSE : IS_SELECTOR_32BIT(seg))

#define STACK_sig(context) \
   (IS_SEL_32(context,SS_sig(context)) ? ESP_sig(context) : SP_sig(context))

#define MAKE_PTR(seg,off) \
   (IS_SELECTOR_SYSTEM(seg) ? (void *)(off) : PTR_SEG_OFF_TO_LIN(seg,off))

#define MK_PTR(context,seg,off) \
   (IS_V86(context) ? DOSMEM_MapRealToLinear(MAKELONG(off,seg)) \
                    : MAKE_PTR(seg,off))

#define STACK_PTR(context) \
   (IS_V86(context) ? DOSMEM_MapRealToLinear(MAKELONG(SP_sig(context),SS_sig(context))) : \
    (IS_SELECTOR_SYSTEM(SS_sig(context)) ? (void *)ESP_sig(context) : \
     (PTR_SEG_OFF_TO_LIN(SS_sig(context),STACK_sig(context)))))


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
static WORD INSTR_ReplaceSelector( SIGCONTEXT *context, WORD sel)
{
    if (sel == 0x40)
    {
        static WORD sys_timer = 0;
        if (!sys_timer)
            sys_timer = CreateSystemTimer( 55, (FARPROC16)DOSMEM_Tick );
        return DOSMEM_BiosSeg;
    }
    return 0;  /* Can't replace selector, crashdump */
}


/***********************************************************************
 *           INSTR_GetOperandAddr
 *
 * Return the address of an instruction operand (from the mod/rm byte).
 */
static BYTE *INSTR_GetOperandAddr( SIGCONTEXT *context, BYTE *instr,
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
        case 0: return (BYTE *)&EAX_sig(context);
        case 1: return (BYTE *)&ECX_sig(context);
        case 2: return (BYTE *)&EDX_sig(context);
        case 3: return (BYTE *)&EBX_sig(context);
        case 4: return (BYTE *)&ESP_sig(context);
        case 5: return (BYTE *)&EBP_sig(context);
        case 6: return (BYTE *)&ESI_sig(context);
        case 7: return (BYTE *)&EDI_sig(context);
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
            case 0: index = EAX_sig(context); break;
            case 1: index = ECX_sig(context); break;
            case 2: index = EDX_sig(context); break;
            case 3: index = EBX_sig(context); break;
            case 4: index = 0; break;
            case 5: index = EBP_sig(context); break;
            case 6: index = ESI_sig(context); break;
            case 7: index = EDI_sig(context); break;
            }
        }

        switch(rm)
        {
        case 0: base = EAX_sig(context); seg = DS_sig(context); break;
        case 1: base = ECX_sig(context); seg = DS_sig(context); break;
        case 2: base = EDX_sig(context); seg = DS_sig(context); break;
        case 3: base = EBX_sig(context); seg = DS_sig(context); break;
        case 4: base = ESP_sig(context); seg = SS_sig(context); break;
        case 5: base = EBP_sig(context); seg = SS_sig(context); break;
        case 6: base = ESI_sig(context); seg = DS_sig(context); break;
        case 7: base = EDI_sig(context); seg = DS_sig(context); break;
        }
        switch (mod)
        {
        case 0:
            if (rm == 5)  /* special case: ds:(disp32) */
            {
                GET_VAL( &base, DWORD );
                seg = DS_sig(context);
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
            base = BX_sig(context) + SI_sig(context);
            seg  = DS_sig(context);
            break;
        case 1:  /* ds:(bx,di) */
            base = BX_sig(context) + DI_sig(context);
            seg  = DS_sig(context);
            break;
        case 2:  /* ss:(bp,si) */
            base = BP_sig(context) + SI_sig(context);
            seg  = SS_sig(context);
            break;
        case 3:  /* ss:(bp,di) */
            base = BP_sig(context) + DI_sig(context);
            seg  = SS_sig(context);
            break;
        case 4:  /* ds:(si) */
            base = SI_sig(context);
            seg  = DS_sig(context);
            break;
        case 5:  /* ds:(di) */
            base = DI_sig(context);
            seg  = DS_sig(context);
            break;
        case 6:  /* ss:(bp) */
            base = BP_sig(context);
            seg  = SS_sig(context);
            break;
        case 7:  /* ds:(bx) */
            base = BX_sig(context);
            seg  = DS_sig(context);
            break;
        }

        switch(mod)
        {
        case 0:
            if (rm == 6)  /* special case: ds:(disp16) */
            {
                GET_VAL( &base, WORD );
                seg  = DS_sig(context);
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
static BOOL32 INSTR_EmulateLDS( SIGCONTEXT *context, BYTE *instr, int long_op,
                                int long_addr, int segprefix, int *len )
{
    WORD seg;
    BYTE *regmodrm = instr + 1 + (*instr == 0x0f);
    BYTE *addr = INSTR_GetOperandAddr( context, regmodrm,
                                       long_addr, segprefix, len );
    if (!addr)
        return FALSE;  /* Unable to emulate it */
    seg = *(WORD *)(addr + (long_op ? 4 : 2));

    if (!(seg = INSTR_ReplaceSelector( context, seg )))
        return FALSE;  /* Unable to emulate it */

    /* Now store the offset in the correct register */

    switch((*regmodrm >> 3) & 7)
    {
    case 0:
        if (long_op) EAX_sig(context) = *(DWORD *)addr;
        else AX_sig(context) = *(WORD *)addr;
        break;
    case 1:
        if (long_op) ECX_sig(context) = *(DWORD *)addr;
        else CX_sig(context) = *(WORD *)addr;
        break;
    case 2:
        if (long_op) EDX_sig(context) = *(DWORD *)addr;
        else DX_sig(context) = *(WORD *)addr;
        break;
    case 3:
        if (long_op) EBX_sig(context) = *(DWORD *)addr;
        else BX_sig(context) = *(WORD *)addr;
        break;
    case 4:
        if (long_op) ESP_sig(context) = *(DWORD *)addr;
        else SP_sig(context) = *(WORD *)addr;
        break;
    case 5:
        if (long_op) EBP_sig(context) = *(DWORD *)addr;
        else BP_sig(context) = *(WORD *)addr;
        break;
    case 6:
        if (long_op) ESI_sig(context) = *(DWORD *)addr;
        else SI_sig(context) = *(WORD *)addr;
        break;
    case 7:
        if (long_op) EDI_sig(context) = *(DWORD *)addr;
        else DI_sig(context) = *(WORD *)addr;
        break;
    }

    /* Store the correct segment in the segment register */

    switch(*instr)
    {
    case 0xc4: ES_sig(context) = seg; break;  /* les */
    case 0xc5: DS_sig(context) = seg; break;  /* lds */
    case 0x0f: switch(instr[1])
               {
               case 0xb2: SS_sig(context) = seg; break;  /* lss */
#ifdef FS_sig
               case 0xb4: FS_sig(context) = seg; break;  /* lfs */
#endif
#ifdef GS_sig
               case 0xb5: GS_sig(context) = seg; break;  /* lgs */
#endif
               }
               break;
    }

    /* Add the opcode size to the total length */

    *len += 1 + (*instr == 0x0f);
    return TRUE;
}


/***********************************************************************
 *           INSTR_EmulateInstruction
 *
 * Emulate a priviledged instruction. Returns TRUE if emulation successful.
 */
BOOL32 INSTR_EmulateInstruction( SIGCONTEXT *context )
{
    int prefix, segprefix, prefixlen, len, repX, long_op, long_addr;
    SEGPTR gpHandler;
    BYTE *instr;

    /* Check for page-fault */

#if defined(TRAP_sig) && defined(CR2_sig)
    if (TRAP_sig(context) == 0x0e
        && VIRTUAL_HandleFault( (LPVOID)CR2_sig(context) )) return TRUE;
#endif

    long_op = long_addr = IS_SEL_32(context,CS_sig(context));
    instr = (BYTE *)MK_PTR(context,CS_sig(context),EIP_sig(context));
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
            segprefix = CS_sig(context);
            break;
        case 0x36:
            segprefix = SS_sig(context);
            break;
        case 0x3e:
            segprefix = DS_sig(context);
            break;
        case 0x26:
            segprefix = ES_sig(context);
            break;
#ifdef FS_sig
        case 0x64:
            segprefix = FS_sig(context);
            break;
#endif
#ifdef GS_sig
        case 0x65:
            segprefix = GS_sig(context);
            break;
#endif
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
                if ((seg = INSTR_ReplaceSelector( context, seg )) != 0)
                {
                    switch(*instr)
                    {
                    case 0x07: ES_sig(context) = seg; break;
                    case 0x17: SS_sig(context) = seg; break;
                    case 0x1f: DS_sig(context) = seg; break;
                    }
                    STACK_sig(context) += long_op ? 4 : 2;
                    EIP_sig(context) += prefixlen + 1;
                    return TRUE;
                }
            }
            break;  /* Unable to emulate it */

        case 0x0f: /* extended instruction */
            switch(instr[1])
            {
	    case 0x20: /* mov cr4, eax */
	    	if (instr[2]!=0xe0) 
			break;
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
		fprintf(stderr,"mov cr4,eax at 0x%08lx\n",EIP_sig(context));
		EAX_sig(context) = 0;
		EIP_sig(context) += prefixlen+3;
		return TRUE;
#ifdef FS_sig
            case 0xa1: /* pop fs */
                {
                    WORD seg = *(WORD *)STACK_PTR( context );
                    if ((seg = INSTR_ReplaceSelector( context, seg )) != 0)
                    {
                        FS_sig(context) = seg;
                        STACK_sig(context) += long_op ? 4 : 2;
                        EIP_sig(context) += prefixlen + 2;
                        return TRUE;
                    }
                }
                break;
#endif  /* FS_sig */

#ifdef GS_sig
            case 0xa9: /* pop gs */
                {
                    WORD seg = *(WORD *)STACK_PTR( context );
                    if ((seg = INSTR_ReplaceSelector( context, seg )) != 0)
                    {
                        GS_sig(context) = seg;
                        STACK_sig(context) += long_op ? 4 : 2;
                        EIP_sig(context) += prefixlen + 2;
                        return TRUE;
                    }
                }
                break;
#endif  /* GS_sig */

            case 0xb2: /* lss addr,reg */
#ifdef FS_sig
            case 0xb4: /* lfs addr,reg */
#endif
#ifdef GS_sig
            case 0xb5: /* lgs addr,reg */
#endif
                if (INSTR_EmulateLDS( context, instr, long_op,
                                      long_addr, segprefix, &len ))
                {
                    EIP_sig(context) += prefixlen + len;
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
                          (long_addr ? ECX_sig(context) : CX_sig(context)) : 1;
	      int opsize = (typ & 1) ? (long_op ? 4 : 2) : 1;
	      int step = (EFL_sig(context) & 0x400) ? -opsize : +opsize;
	      int seg = outp ? DS_sig(context) : ES_sig(context);  /* FIXME: is this right? */

	      if (outp)
		/* FIXME: Check segment readable.  */
		(void)0;
	      else
		/* FIXME: Check segment writeable.  */
		(void)0;

	      if (repX)
              {
		if (long_addr)
		  ECX_sig(context) = 0;
		else
		  CX_sig(context) = 0;
              }

	      while (count-- > 0)
		{
		  void *data;
		  if (outp)
                  {
		      data = MAKE_PTR(seg,
                               long_addr ? ESI_sig(context) : SI_sig(context));
		      if (long_addr) ESI_sig(context) += step;
		      else SI_sig(context) += step;
                  }
		  else
                  {
		      data = MAKE_PTR(seg,
                               long_addr ? EDI_sig(context) : DI_sig(context));
		      if (long_addr) EDI_sig(context) += step;
		      else DI_sig(context) += step;
                  }
                  
		  switch (typ)
                  {
		    case 0x6c:
		      *((BYTE *)data) = IO_inport( DX_sig(context), 1);
		      TRACE(io, "0x%x < %02x @ %04x:%04x\n", DX_sig(context),
                        *((BYTE *)data), CS_sig(context), IP_sig(context));
		      break;
		    case 0x6d:
		      if (long_op)
                      {
			*((DWORD *)data) = IO_inport( DX_sig(context), 4);
                        TRACE(io, "0x%x < %08lx @ %04x:%04x\n", DX_sig(context),
                          *((DWORD *)data), CS_sig(context), IP_sig(context));
                      }
		      else
                      {
			*((WORD *)data) = IO_inport( DX_sig(context), 2);
                        TRACE(io, "0x%x < %04x @ %04x:%04x\n", DX_sig(context),
                          *((WORD *)data), CS_sig(context), IP_sig(context));
                      }
		      break;
		    case 0x6e:
                        IO_outport( DX_sig(context), 1, *((BYTE *)data));
                        TRACE(io, "0x%x > %02x @ %04x:%04x\n", DX_sig(context),
                          *((BYTE *)data), CS_sig(context), IP_sig(context));
                        break;
		    case 0x6f:
                        if (long_op)
                        {
                            IO_outport( DX_sig(context), 4, *((DWORD *)data));
                            TRACE(io, "0x%x > %08lx @ %04x:%04x\n", DX_sig(context),
                              *((DWORD *)data), CS_sig(context), IP_sig(context)); 
                        }
                        else
                        {
                            IO_outport( DX_sig(context), 2, *((WORD *)data));
                            TRACE(io, "0x%x > %04x @ %04x:%04x\n", DX_sig(context),
                              *((WORD *)data), CS_sig(context), IP_sig(context));
                        }
                        break;
		    }
		}
              EIP_sig(context) += prefixlen + 1;
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
                if (!(seg = INSTR_ReplaceSelector( context, seg )))
                    break;  /* Unable to emulate it */

                switch((instr[1] >> 3) & 7)
                {
                case 0:
                    ES_sig(context) = seg;
                    EIP_sig(context) += prefixlen + len + 1;
                    return TRUE;
                case 1:  /* cs */
                    break;
                case 2:
                    SS_sig(context) = seg;
                    EIP_sig(context) += prefixlen + len + 1;
                    return TRUE;
                case 3:
                    DS_sig(context) = seg;
                    EIP_sig(context) += prefixlen + len + 1;
                    return TRUE;
                case 4:
#ifdef FS_sig
                    FS_sig(context) = seg;
                    EIP_sig(context) += prefixlen + len + 1;
                    return TRUE;
#endif
                case 5:
#ifdef GS_sig
                    GS_sig(context) = seg;
                    EIP_sig(context) += prefixlen + len + 1;
                    return TRUE;
#endif
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
                EIP_sig(context) += prefixlen + len;
                return TRUE;
            }
            break;  /* Unable to emulate it */
            
        case 0xcd: /* int <XX> */
            if (long_op)
            {
                ERR(int, "int xx from 32-bit code is not supported.\n");
                break;  /* Unable to emulate it */
            }
            else
            {
                FARPROC16 addr = INT_GetPMHandler( instr[1] );
                WORD *stack = (WORD *)STACK_PTR( context );
                /* Push the flags and return address on the stack */
                *(--stack) = FL_sig(context);
                *(--stack) = CS_sig(context);
                *(--stack) = IP_sig(context) + prefixlen + 2;
                STACK_sig(context) -= 3 * sizeof(WORD);
                /* Jump to the interrupt handler */
                CS_sig(context)  = HIWORD(addr);
                EIP_sig(context) = LOWORD(addr);
            }
            return TRUE;

        case 0xcf: /* iret */
            if (long_op)
            {
                DWORD *stack = (DWORD *)STACK_PTR( context );
                EIP_sig(context) = *stack++;
                CS_sig(context)  = *stack++;
                EFL_sig(context) = *stack;
                STACK_sig(context) += 3*sizeof(DWORD);  /* Pop the return address and flags */
            }
            else
            {
                WORD *stack = (WORD *)STACK_PTR( context );
                EIP_sig(context) = *stack++;
                CS_sig(context)  = *stack++;
                FL_sig(context)  = *stack;
                STACK_sig(context) += 3*sizeof(WORD);  /* Pop the return address and flags */
            }
            return TRUE;

        case 0xe4: /* inb al,XX */
            AL_sig(context) = IO_inport( instr[1], 1 );
            TRACE(io, "0x%x < %02x @ %04x:%04x\n", instr[1],
                AL_sig(context), CS_sig(context), IP_sig(context));
	    EIP_sig(context) += prefixlen + 2;
            return TRUE;

        case 0xe5: /* in (e)ax,XX */
            if (long_op)
            {
                EAX_sig(context) = IO_inport( instr[1], 4 );
                TRACE(io, "0x%x < %08lx @ %04x:%04x\n", instr[1],
                    EAX_sig(context), CS_sig(context), IP_sig(context));
            }
            else
            {
                AX_sig(context) = IO_inport( instr[1], 2 );
                TRACE(io, "0x%x < %04x @ %04x:%04x\n", instr[1],
                    AX_sig(context), CS_sig(context), IP_sig(context));
            }
	    EIP_sig(context) += prefixlen + 2;
            return TRUE;

        case 0xe6: /* outb XX,al */
            IO_outport( instr[1], 1, AL_sig(context) );
            TRACE(io, "0x%x > %02x @ %04x:%04x\n", instr[1],
                AL_sig(context), CS_sig(context), IP_sig(context));
	    EIP_sig(context) += prefixlen + 2;
            return TRUE;

        case 0xe7: /* out XX,(e)ax */
            if (long_op)
            {
                IO_outport( instr[1], 4, EAX_sig(context) );
                TRACE(io, "0x%x > %08lx @ %04x:%04x\n", instr[1],
                    EAX_sig(context), CS_sig(context), IP_sig(context));
            }
            else
            {
                IO_outport( instr[1], 2, AX_sig(context) );
                TRACE(io, "0x%x > %04x @ %04x:%04x\n", instr[1],
                    AX_sig(context), CS_sig(context), IP_sig(context));
            }
  	    EIP_sig(context) += prefixlen + 2;
            return TRUE;

        case 0xec: /* inb al,dx */
            AL_sig(context) = IO_inport( DX_sig(context), 1 );
            TRACE(io, "0x%x < %02x @ %04x:%04x\n", DX_sig(context),
                AL_sig(context), CS_sig(context), IP_sig(context));
  	    EIP_sig(context) += prefixlen + 1;
            return TRUE;

        case 0xed: /* in (e)ax,dx */
            if (long_op)
            {
                EAX_sig(context) = IO_inport( DX_sig(context), 4 );
                TRACE(io, "0x%x < %08lx @ %04x:%04x\n", DX_sig(context),
                    EAX_sig(context), CS_sig(context), IP_sig(context));
            }
            else
            {
                AX_sig(context) = IO_inport( DX_sig(context), 2 );
                TRACE(io, "0x%x < %04x @ %04x:%04x\n", DX_sig(context),
                    AX_sig(context), CS_sig(context), IP_sig(context));
            }
  	    EIP_sig(context) += prefixlen + 1;
            return TRUE;

        case 0xee: /* outb dx,al */
            IO_outport( DX_sig(context), 1, AL_sig(context) );
            TRACE(io, "0x%x > %02x @ %04x:%04x\n", DX_sig(context),
                AL_sig(context), CS_sig(context), IP_sig(context));
  	    EIP_sig(context) += prefixlen + 1;
            return TRUE;
      
        case 0xef: /* out dx,(e)ax */
            if (long_op)
            {
                IO_outport( DX_sig(context), 4, EAX_sig(context) );
                TRACE(io, "0x%x > %08lx @ %04x:%04x\n", DX_sig(context),
                    EAX_sig(context), CS_sig(context), IP_sig(context));
            }
            else
            {
                IO_outport( DX_sig(context), 2, AX_sig(context) );
                TRACE(io, "0x%x > %04x @ %04x:%04x\n", DX_sig(context),
                    AX_sig(context), CS_sig(context), IP_sig(context));
            }
  	    EIP_sig(context) += prefixlen + 1;
            return TRUE;

        case 0xfa: /* cli, ignored */
  	    EIP_sig(context) += prefixlen + 1;
            return TRUE;

        case 0xfb: /* sti, ignored */
  	    EIP_sig(context) += prefixlen + 1;
            return TRUE;
    }


    /* Check for Win16 __GP handler */
    gpHandler = HasGPHandler( PTR_SEG_OFF_TO_SEGPTR( CS_sig(context),
                                                     EIP_sig(context) ) );
    if (gpHandler)
    {
        WORD *stack = (WORD *)STACK_PTR( context );
        *--stack = CS_sig(context);
        *--stack = EIP_sig(context);
        STACK_sig(context) -= 2*sizeof(WORD);

        CS_sig(context) = SELECTOROF( gpHandler );
        EIP_sig(context) = OFFSETOF( gpHandler );
        return TRUE;
    }

    MSG("Unexpected Windows program segfault"
                    " - opcode = %x\n", *instr);
    return FALSE;  /* Unable to emulate it */
}
