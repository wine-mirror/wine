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

    long_op = long_addr = (GET_SEL_FLAGS(CS) & LDT_FLAGS_32BIT) != 0;
    instr = (BYTE *) PTR_SEG_OFF_TO_LIN( CS, long_op ? EIP : IP );

    /* First handle any possible prefix */

    segprefix = -1;  /* no prefix */
    prefix = 1;
    repX = 0;
    while(prefix)
    {
        switch(*instr)
        {
        case 0x2e:
            segprefix = CS;
            break;
        case 0x36:
            segprefix = SS;
            break;
        case 0x3e:
            segprefix = DS;
            break;
        case 0x26:
            segprefix = ES;
            break;
        case 0x64:
            segprefix = FS;
            break;
        case 0x65:
            segprefix = GS;
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
            EIP++;
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
                WORD *stack = (WORD *)PTR_SEG_OFF_TO_LIN( SS, SP );
                /* Push the flags and return address on the stack */
                *(--stack) = FL;
                *(--stack) = CS;
                *(--stack) = IP + 2;
                SP -= 3 * sizeof(WORD);
                /* Jump to the interrupt handler */
                CS  = HIWORD(addr);
                EIP = LOWORD(addr);
            }
            break;

        case 0xcf: /* iret */
            if (long_op)
            {
                /* FIXME: should check the stack 'big' bit */
                DWORD *stack = (DWORD *)PTR_SEG_OFF_TO_LIN( SS, SP );
                EIP = *stack++;
                CS  = *stack++;
                EFL = *stack;
                SP += 3*sizeof(DWORD);  /* Pop the return address and flags */
            }
            else
            {
                /* FIXME: should check the stack 'big' bit */
                WORD *stack = (WORD *)PTR_SEG_OFF_TO_LIN( SS, SP );
                EIP = *stack++;
                CS  = *stack++;
                FL  = *stack;
                SP += 3*sizeof(WORD);  /* Pop the return address and flags */
            }
            break;

        case 0xe4: /* inb al,XX */
            AL = inport( instr[1], 1 );
	    EIP += 2;
            break;

        case 0xe5: /* in (e)ax,XX */
            if (long_op) EAX = inport( instr[1], 4 );
            else AX = inport( instr[1], 2 );
	    EIP += 2;
            break;

        case 0xe6: /* outb XX,al */
            outport( instr[1], 1, AL );
	    EIP += 2;
            break;

        case 0xe7: /* out XX,(e)ax */
            if (long_op) outport( instr[1], 4, EAX );
            else outport( instr[1], 2, AX );
  	    EIP += 2;
            break;

        case 0xec: /* inb al,dx */
            AL = inport( DX, 1 );
	    EIP++;
            break;

        case 0xed: /* in (e)ax,dx */
            if (long_op) EAX = inport( DX, 4 );
            else AX = inport( DX, 2 );
	    EIP++;  
            break;

        case 0xee: /* outb dx,al */
            outport( DX, 1, AL );
	    EIP++;
            break;
      
        case 0xef: /* out dx,(e)ax */
            if (long_op) outport( DX, 4, EAX );
            else outport( DX, 2, AX );
	    EIP++;
            break;

        case 0xfa: /* cli, ignored */
	    EIP++;
            break;

        case 0xfb: /* sti, ignored */
	    EIP++;
            break;

        case 0x6c: /* insb     */
        case 0x6d: /* insw/d   */
        case 0x6e: /* outsb    */
        case 0x6f: /* outsw/d  */
	    {
	      int typ = *instr;  /* Just in case it's overwritten.  */
	      int outp = (typ >= 0x6e);
	      unsigned long count = repX ? (long_addr ? ECX : CX) : 1;
	      int opsize = (typ & 1) ? (long_op ? 4 : 2) : 1;
	      int step = (EFL & 0x400) ? -opsize : +opsize;
	      /* FIXME: Check this, please.  */
	      int seg = outp ? (segprefix >= 0 ? segprefix : DS) : ES;

	      if (outp)
		/* FIXME: Check segment readable.  */
		;
	      else
		/* FIXME: Check segment writeable.  */
		;

	      if (repX)
		if (long_addr)
		  ECX = 0;
		else
		  CX = 0;

	      while (count-- > 0)
		{
		  void *data;
		  if (outp)
		    {
		      data = PTR_SEG_OFF_TO_LIN (seg, long_addr ? ESI : SI);
		      if (long_addr)
			ESI += step;
		      else
			SI += step;
		    }
		  else
		    {
		      data = PTR_SEG_OFF_TO_LIN (seg, long_addr ? EDI : DI);
		      if (long_addr)
			EDI += step;
		      else
			DI += step;
		    }

		  switch (typ)
		    {
		    case 0x6c:
		      *((BYTE *)data) = inport (DX, 1);
		      break;
		    case 0x6d:
		      if (long_op)
			*((DWORD *)data) = inport (DX, 4);
		      else
			*((WORD *)data) = inport (DX, 2);
		      break;
		    case 0x6e:
		      outport (DX, 1, *((BYTE *)data));
		      break;
		    case 0x6f:
		      if (long_op)
			outport (DX, 4, *((DWORD *)data));
		      else
			outport (DX, 2, *((WORD *)data));
		      break;
		    }
		}
	      EIP++;
	      break;
	    }

      default:
            fprintf(stderr, "Unexpected Windows program segfault"
                            " - opcode = %x\n", *instr);
            return FALSE;  /* Unable to emulate it */
    }
    return TRUE;
}
