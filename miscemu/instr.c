/*
 * Emulation of priviledged instructions
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include "windows.h"
#include "dos_fs.h"
#include "ldt.h"
#include "miscemu.h"
#include "registers.h"


static int do_int(int intnum, struct sigcontext_struct *context)
{
	switch(intnum)
	{
	      case 0x10: return do_int10(context);

	      case 0x11:  
		AX = DOS_GetEquipment();
		return 1;

	      case 0x12:               
		AX = 640;
		return 1;	/* get base mem size */                

              case 0x13: return do_int13(context);
	      case 0x15: return do_int15(context);
	      case 0x16: return do_int16(context);
	      case 0x1a: return do_int1a(context);
	      case 0x21: return do_int21(context);

	      case 0x22:
		AX = 0x1234;
		BX = 0x5678;
		CX = 0x9abc;
		DX = 0xdef0;
		return 1;

              case 0x25: return do_int25(context);
              case 0x26: return do_int26(context);
              case 0x2a: return do_int2a(context);
	      case 0x2f: return do_int2f(context);
	      case 0x31: return do_int31(context);
              case 0x3d: return 1;
	      case 0x5c: return do_int5c(context);

              default:
                fprintf(stderr,"int%02x: Unimplemented!\n", intnum);
                break;
	}
	return 0;
}


/***********************************************************************
 *           INSTR_EmulateInstruction
 *
 * Emulate a priviledged instruction. Returns TRUE if emulation successful.
 */
BOOL INSTR_EmulateInstruction( struct sigcontext_struct *context )
{
    int prefix, segprefix, long_op, long_addr;
    BYTE *instr = (BYTE *) PTR_SEG_OFF_TO_LIN( CS, IP );

    /* First handle any possible prefix */

    long_op = long_addr = (GET_SEL_FLAGS(CS) & LDT_FLAGS_32BIT) != 0;
    segprefix = 0;  /* no prefix */
    prefix = 1;
    while(prefix)
    {
        switch(*instr)
        {
        case 0x2e:
            segprefix = 1;  /* CS */
            break;
        case 0x36:
            segprefix = 2;  /* SS */
            break;
        case 0x3e:
            segprefix = 3;  /* DS */
            break;
        case 0x26:
            segprefix = 4;  /* ES */
            break;
        case 0x64:
            segprefix = 5;  /* FS */
            break;
        case 0x65:
            segprefix = 6;  /* GS */
            break;
        case 0x66:
            long_op = !long_op;  /* opcode size prefix */
            break;
        case 0x67:
            long_addr = !long_addr;  /* addr size prefix */
            break;
        case 0xf0:  /* lock */
        case 0xf2:  /* repne */
        case 0xf3:  /* repe */
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
            instr++;
            /* FIXME: should check if handler has been changed */
	    if (!do_int(*instr, context))
            {
		fprintf(stderr,"Unexpected Windows interrupt %x\n", *instr);
                return FALSE;
	    }
	    EIP += 2;  /* Bypass the int instruction */
            break;

      case 0xcf: /* iret */
            if (long_op)
            {
                /* FIXME: should check the stack 'big' bit */
                DWORD *stack = (WORD *)PTR_SEG_OFF_TO_LIN( SS, SP );
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
                EFL = (EFL & 0xffff0000) | *stack;
                SP += 3*sizeof(WORD);  /* Pop the return address and flags */
            }
            break;

      case 0xe4: /* inb al,XX */
            inportb_abs(context);
	    EIP += 2;
            break;

      case 0xe5: /* in ax,XX */
            inport_abs( context, long_op );
	    EIP += 2;
            break;

      case 0xe6: /* outb XX,al */
            outportb_abs(context);
	    EIP += 2;
            break;

      case 0xe7: /* out XX,ax */
            outport_abs( context, long_op );
	    EIP += 2;
            break;

      case 0xec: /* inb al,dx */
            inportb(context);
	    EIP++;
            break;

      case 0xed: /* in ax,dx */
            inport( context, long_op );
	    EIP++;  
            break;

      case 0xee: /* outb dx,al */
            outportb(context);
	    EIP++;
            break;
      
      case 0xef: /* out dx,ax */
            outport( context, long_op );
	    EIP++;
            break;

      case 0xfa: /* cli, ignored */
	    EIP++;
            break;

      case 0xfb: /* sti, ignored */
	    EIP++;
            break;

      default:
            fprintf(stderr, "Unexpected Windows program segfault"
                            " - opcode = %x\n", *instr);
            return FALSE;  /* Unable to emulate it */
    }
    return TRUE;
}
