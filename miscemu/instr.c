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


/***********************************************************************
 *           INSTR_EmulateInstruction
 *
 * Emulate a priviledged instruction. Returns TRUE if emulation successful.
 */
BOOL INSTR_EmulateInstruction( struct sigcontext_struct *context )
{
    int prefix, segprefix, repX, long_op, long_addr;
    BYTE *instr;

    long_op = long_addr = (GET_SEL_FLAGS(CS_reg(context)) & LDT_FLAGS_32BIT) != 0;
    instr = (BYTE *) PTR_SEG_OFF_TO_LIN( CS_reg(context), EIP_reg(context) );

    /* First handle any possible prefix */

    segprefix = -1;  /* no prefix */
    prefix = 1;
    repX = 0;
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
            EIP_reg(context)++;
        }
    }

    /* Now look at the actual instruction */

    switch(*instr)
    {
        case 0xcd: /* int <XX> */
            if (long_op)
            {
                fprintf(stderr, "int xx from 32-bit code is not supported.\n");
                return FALSE;  /* Unable to emulate it */
            }
            else
            {
                SEGPTR addr = INT_GetHandler( instr[1] );
                /* FIXME: should check the stack 'big' bit */
                WORD *stack = (WORD *)PTR_SEG_OFF_TO_LIN( SS_reg(context),
                                                          SP_reg(context) );
                /* Push the flags and return address on the stack */
                *(--stack) = FL_reg(context);
                *(--stack) = CS_reg(context);
                *(--stack) = IP_reg(context) + 2;
                SP_reg(context) -= 3 * sizeof(WORD);
                /* Jump to the interrupt handler */
                CS_reg(context)  = HIWORD(addr);
                EIP_reg(context) = LOWORD(addr);
            }
            break;

        case 0xcf: /* iret */
            if (long_op)
            {
                /* FIXME: should check the stack 'big' bit */
                DWORD *stack = (DWORD *)PTR_SEG_OFF_TO_LIN( SS_reg(context),
                                                            SP_reg(context) );
                EIP_reg(context) = *stack++;
                CS_reg(context)  = *stack++;
                EFL_reg(context) = *stack;
                SP_reg(context) += 3*sizeof(DWORD);  /* Pop the return address and flags */
            }
            else
            {
                /* FIXME: should check the stack 'big' bit */
                WORD *stack = (WORD *)PTR_SEG_OFF_TO_LIN( SS_reg(context),
                                                          SP_reg(context) );
                EIP_reg(context) = *stack++;
                CS_reg(context)  = *stack++;
                FL_reg(context)  = *stack;
                SP_reg(context) += 3*sizeof(WORD);  /* Pop the return address and flags */
            }
            break;

        case 0xe4: /* inb al,XX */
            AL_reg(context) = inport( instr[1], 1 );
	    EIP_reg(context) += 2;
            break;

        case 0xe5: /* in (e)ax,XX */
            if (long_op) EAX_reg(context) = inport( instr[1], 4 );
            else AX_reg(context) = inport( instr[1], 2 );
	    EIP_reg(context) += 2;
            break;

        case 0xe6: /* outb XX,al */
            outport( instr[1], 1, AL_reg(context) );
	    EIP_reg(context) += 2;
            break;

        case 0xe7: /* out XX,(e)ax */
            if (long_op) outport( instr[1], 4, EAX_reg(context) );
            else outport( instr[1], 2, AX_reg(context) );
  	    EIP_reg(context) += 2;
            break;

        case 0xec: /* inb al,dx */
            AL_reg(context) = inport( DX_reg(context), 1 );
	    EIP_reg(context)++;
            break;

        case 0xed: /* in (e)ax,dx */
            if (long_op) EAX_reg(context) = inport( DX_reg(context), 4 );
            else AX_reg(context) = inport( DX_reg(context), 2 );
	    EIP_reg(context)++;  
            break;

        case 0xee: /* outb dx,al */
            outport( DX_reg(context), 1, AL_reg(context) );
	    EIP_reg(context)++;
            break;
      
        case 0xef: /* out dx,(e)ax */
            if (long_op) outport( DX_reg(context), 4, EAX_reg(context) );
            else outport( DX_reg(context), 2, AX_reg(context) );
	    EIP_reg(context)++;
            break;

        case 0xfa: /* cli, ignored */
	    EIP_reg(context)++;
            break;

        case 0xfb: /* sti, ignored */
	    EIP_reg(context)++;
            break;

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
	      int seg = outp ? (segprefix >= 0 ? segprefix : DS_reg(context))
                             : ES_reg(context);

	      if (outp)
		/* FIXME: Check segment readable.  */
		;
	      else
		/* FIXME: Check segment writeable.  */
		;

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
	      EIP_reg(context)++;
	      break;
	    }

      default:
            fprintf(stderr, "Unexpected Windows program segfault"
                            " - opcode = %x\n", *instr);
            return FALSE;  /* Unable to emulate it */
    }
    return TRUE;
}
