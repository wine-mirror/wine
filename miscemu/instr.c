/*
 * Emulation of priviledged instructions
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include "windows.h"
#include "ldt.h"
#include "miscemu.h"
#include "registers.h"


#define STACK_reg(context) \
   ((GET_SEL_FLAGS(SS_reg(context)) & LDT_FLAGS_32BIT) ? \
                   ESP_reg(context) : SP_reg(context))

#define STACK_PTR(context) \
    (PTR_SEG_OFF_TO_LIN(SS_reg(context),STACK_reg(context)))

/***********************************************************************
 *           INSTR_ReplaceSelector
 *
 * Try to replace an invalid selector by a valid one.
 * For now, only selector 0x40 is handled here.
 */
static WORD INSTR_ReplaceSelector( SIGCONTEXT *context, WORD sel)
{
    if (sel == 0x40)
    {
        extern void SIGNAL_StartBIOSTimer(void);
        fprintf( stderr, "Direct access to segment 0x40 (cs:ip=%04x:%04lx).\n",
                 CS_reg(context), EIP_reg(context) );
        SIGNAL_StartBIOSTimer();
        return DOSMEM_BiosSeg;
    }
    return 0;  /* Can't replace selector */
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
            base = BX_reg(context) + SI_reg(context);
            seg  = DS_reg(context);
            break;
        case 1:  /* ds:(bx,di) */
            base = BX_reg(context) + DI_reg(context);
            seg  = DS_reg(context);
            break;
        case 2:  /* ss:(bp,si) */
            base = BP_reg(context) + SI_reg(context);
            seg  = SS_reg(context);
            break;
        case 3:  /* ss:(bp,di) */
            base = BP_reg(context) + DI_reg(context);
            seg  = SS_reg(context);
            break;
        case 4:  /* ds:(si) */
            base = SI_reg(context);
            seg  = DS_reg(context);
            break;
        case 5:  /* ds:(di) */
            base = DI_reg(context);
            seg  = DS_reg(context);
            break;
        case 6:  /* ss:(bp) */
            base = BP_reg(context);
            seg  = SS_reg(context);
            break;
        case 7:  /* ds:(bx) */
            base = BX_reg(context);
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

    /* FIXME: should check limit of the segment here */
    return (BYTE *)PTR_SEG_OFF_TO_LIN( seg, (base + (index << ss)) );
}


/***********************************************************************
 *           INSTR_EmulateLDS
 *
 * Emulate the LDS (and LES,LFS,etc.) instruction.
 */
static BOOL INSTR_EmulateLDS( SIGCONTEXT *context, BYTE *instr, int long_op,
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
        if (long_op) EAX_reg(context) = *(DWORD *)addr;
        else AX_reg(context) = *(WORD *)addr;
        break;
    case 1:
        if (long_op) ECX_reg(context) = *(DWORD *)addr;
        else CX_reg(context) = *(WORD *)addr;
        break;
    case 2:
        if (long_op) EDX_reg(context) = *(DWORD *)addr;
        else DX_reg(context) = *(WORD *)addr;
        break;
    case 3:
        if (long_op) EBX_reg(context) = *(DWORD *)addr;
        else BX_reg(context) = *(WORD *)addr;
        break;
    case 4:
        if (long_op) ESP_reg(context) = *(DWORD *)addr;
        else SP_reg(context) = *(WORD *)addr;
        break;
    case 5:
        if (long_op) EBP_reg(context) = *(DWORD *)addr;
        else BP_reg(context) = *(WORD *)addr;
        break;
    case 6:
        if (long_op) ESI_reg(context) = *(DWORD *)addr;
        else SI_reg(context) = *(WORD *)addr;
        break;
    case 7:
        if (long_op) EDI_reg(context) = *(DWORD *)addr;
        else DI_reg(context) = *(WORD *)addr;
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
#ifdef FS_reg
               case 0xb4: FS_reg(context) = seg; break;  /* lfs */
#endif
#ifdef GS_reg
               case 0xb5: GS_reg(context) = seg; break;  /* lgs */
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
BOOL INSTR_EmulateInstruction( SIGCONTEXT *context )
{
    int prefix, segprefix, prefixlen, len, repX, long_op, long_addr;
    BYTE *instr;

    long_op = long_addr = (GET_SEL_FLAGS(CS_reg(context)) & LDT_FLAGS_32BIT) != 0;
    instr = (BYTE *) PTR_SEG_OFF_TO_LIN( CS_reg(context), EIP_reg(context) );

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
#ifdef FS_reg
        case 0x64:
            segprefix = FS_reg(context);
            break;
#endif
#ifdef GS_reg
        case 0x65:
            segprefix = GS_reg(context);
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
                    case 0x07: ES_reg(context) = seg; break;
                    case 0x17: SS_reg(context) = seg; break;
                    case 0x1f: DS_reg(context) = seg; break;
                    }
                    STACK_reg(context) += long_op ? 4 : 2;
                    EIP_reg(context) += prefixlen + 1;
                    return TRUE;
                }
            }
            break;  /* Unable to emulate it */

        case 0x0f: /* extended instruction */
            switch(instr[1])
            {
#ifdef FS_reg
            case 0xa1: /* pop fs */
                {
                    WORD seg = *(WORD *)STACK_PTR( context );
                    if ((seg = INSTR_ReplaceSelector( context, seg )) != 0)
                    {
                        FS_reg(context) = seg;
                        STACK_reg(context) += long_op ? 4 : 2;
                        EIP_reg(context) += prefixlen + 2;
                        return TRUE;
                    }
                }
                break;
#endif  /* FS_reg */

#ifdef GS_reg
            case 0xa9: /* pop gs */
                {
                    WORD seg = *(WORD *)STACK_PTR( context );
                    if ((seg = INSTR_ReplaceSelector( context, seg )) != 0)
                    {
                        GS_reg(context) = seg;
                        STACK_reg(context) += long_op ? 4 : 2;
                        EIP_reg(context) += prefixlen + 2;
                        return TRUE;
                    }
                }
                break;
#endif  /* GS_reg */

            case 0xb2: /* lss addr,reg */
#ifdef FS_reg
            case 0xb4: /* lfs addr,reg */
#endif
#ifdef GS_reg
            case 0xb5: /* lgs addr,reg */
#endif
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
                          (long_addr ? ECX_reg(context) : CX_reg(context)) : 1;
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
		if (long_addr)
		  ECX_reg(context) = 0;
		else
		  CX_reg(context) = 0;

	      while (count-- > 0)
		{
		  void *data;
		  if (outp)
                  {
		      data = PTR_SEG_OFF_TO_LIN (seg,
                               long_addr ? ESI_reg(context) : SI_reg(context));
		      if (long_addr) ESI_reg(context) += step;
		      else SI_reg(context) += step;
                  }
		  else
                  {
		      data = PTR_SEG_OFF_TO_LIN (seg,
                               long_addr ? EDI_reg(context) : DI_reg(context));
		      if (long_addr) EDI_reg(context) += step;
		      else DI_reg(context) += step;
                  }
                  
		  switch (typ)
                  {
		    case 0x6c:
		      *((BYTE *)data) = inport( DX_reg(context), 1);
		      break;
		    case 0x6d:
		      if (long_op)
			*((DWORD *)data) = inport( DX_reg(context), 4);
		      else
			*((WORD *)data) = inport( DX_reg(context), 2);
		      break;
		    case 0x6e:
		      outport( DX_reg(context), 1, *((BYTE *)data));
		      break;
		    case 0x6f:
		      if (long_op)
			outport( DX_reg(context), 4, *((DWORD *)data));
		      else
			outport( DX_reg(context), 2, *((WORD *)data));
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
                if (!(seg = INSTR_ReplaceSelector( context, seg )))
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
#ifdef FS_reg
                    FS_reg(context) = seg;
                    EIP_reg(context) += prefixlen + len + 1;
                    return TRUE;
#endif
                case 5:
#ifdef GS_reg
                    GS_reg(context) = seg;
                    EIP_reg(context) += prefixlen + len + 1;
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
                EIP_reg(context) += prefixlen + len;
                return TRUE;
            }
            break;  /* Unable to emulate it */
            
        case 0xcd: /* int <XX> */
            if (long_op)
            {
                fprintf(stderr, "int xx from 32-bit code is not supported.\n");
                break;  /* Unable to emulate it */
            }
            else
            {
                SEGPTR addr = INT_GetHandler( instr[1] );
                WORD *stack = (WORD *)STACK_PTR( context );
                /* Push the flags and return address on the stack */
                *(--stack) = FL_reg(context);
                *(--stack) = CS_reg(context);
                *(--stack) = IP_reg(context) + prefixlen + 2;
                STACK_reg(context) -= 3 * sizeof(WORD);
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
                STACK_reg(context) += 3*sizeof(DWORD);  /* Pop the return address and flags */
            }
            else
            {
                WORD *stack = (WORD *)STACK_PTR( context );
                EIP_reg(context) = *stack++;
                CS_reg(context)  = *stack++;
                FL_reg(context)  = *stack;
                STACK_reg(context) += 3*sizeof(WORD);  /* Pop the return address and flags */
            }
            return TRUE;

        case 0xe4: /* inb al,XX */
            AL_reg(context) = inport( instr[1], 1 );
	    EIP_reg(context) += prefixlen + 2;
            return TRUE;

        case 0xe5: /* in (e)ax,XX */
            if (long_op) EAX_reg(context) = inport( instr[1], 4 );
            else AX_reg(context) = inport( instr[1], 2 );
	    EIP_reg(context) += prefixlen + 2;
            return TRUE;

        case 0xe6: /* outb XX,al */
            outport( instr[1], 1, AL_reg(context) );
	    EIP_reg(context) += prefixlen + 2;
            return TRUE;

        case 0xe7: /* out XX,(e)ax */
            if (long_op) outport( instr[1], 4, EAX_reg(context) );
            else outport( instr[1], 2, AX_reg(context) );
  	    EIP_reg(context) += prefixlen + 2;
            return TRUE;

        case 0xec: /* inb al,dx */
            AL_reg(context) = inport( DX_reg(context), 1 );
  	    EIP_reg(context) += prefixlen + 1;
            return TRUE;

        case 0xed: /* in (e)ax,dx */
            if (long_op) EAX_reg(context) = inport( DX_reg(context), 4 );
            else AX_reg(context) = inport( DX_reg(context), 2 );
  	    EIP_reg(context) += prefixlen + 1;
            return TRUE;

        case 0xee: /* outb dx,al */
            outport( DX_reg(context), 1, AL_reg(context) );
  	    EIP_reg(context) += prefixlen + 1;
            return TRUE;
      
        case 0xef: /* out dx,(e)ax */
            if (long_op) outport( DX_reg(context), 4, EAX_reg(context) );
            else outport( DX_reg(context), 2, AX_reg(context) );
  	    EIP_reg(context) += prefixlen + 1;
            return TRUE;

        case 0xfa: /* cli, ignored */
  	    EIP_reg(context) += prefixlen + 1;
            return TRUE;

        case 0xfb: /* sti, ignored */
  	    EIP_reg(context) += prefixlen + 1;
            return TRUE;
    }
    fprintf(stderr, "Unexpected Windows program segfault"
                    " - opcode = %x\n", *instr);
    return FALSE;  /* Unable to emulate it */
}
