/*
 * Debugger break-points handling
 *
 * Copyright 1994 Martin von Loewis
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "wine/winbase16.h"
#include "module.h"
#include "neexe.h"
#include "process.h"
#include "task.h"
#include "miscemu.h"
#include "toolhelp.h"
#include "debugger.h"
#include "dosexe.h"

#define INT3          0xcc   /* int 3 opcode */

#define MAX_BREAKPOINTS 100

typedef struct
{
    DBG_ADDR      addr;
    BYTE          addrlen;
    BYTE          opcode;
    BOOL16        enabled;
    WORD	  skipcount;
    BOOL16        in_use;
    struct expr * condition;
} BREAKPOINT;

static BREAKPOINT breakpoints[MAX_BREAKPOINTS];

static int next_bp = 1;  /* breakpoint 0 is reserved for step-over */


/***********************************************************************
 *           DEBUG_ChangeOpcode
 *
 * Change the opcode at segment:addr.
 */
static void DEBUG_SetOpcode( const DBG_ADDR *addr, BYTE op )
{
    BYTE *ptr = DBG_ADDR_TO_LIN(addr);

    /* There are a couple of problems with this. On Linux prior to
       1.1.62, this call fails (ENOACCESS) due to a bug in fs/exec.c.
       This code is currently not tested at all on BSD.
       How do I get the old protection in order to restore it later on?
       */
    if (mprotect((caddr_t)((int)ptr & (~4095)), 4096,
                 PROT_READ | PROT_WRITE | PROT_EXEC) == -1)
    {
        perror( "Can't set break point" );
        return;
    }
    *ptr = op;
    /* mprotect((caddr_t)(addr->off & ~4095), 4096,
       PROT_READ | PROT_EXEC ); */
}


/***********************************************************************
 *           DEBUG_IsStepOverInstr
 *
 * Determine if the instruction at CS:EIP is an instruction that
 * we need to step over (like a call or a repetitive string move).
 */
static BOOL32 DEBUG_IsStepOverInstr()
{
    BYTE *instr = (BYTE *)CTX_SEG_OFF_TO_LIN( &DEBUG_context,
                                              CS_reg(&DEBUG_context),
                                              EIP_reg(&DEBUG_context) );

    for (;;)
    {
        switch(*instr)
        {
          /* Skip all prefixes */

        case 0x2e:  /* cs: */
        case 0x36:  /* ss: */
        case 0x3e:  /* ds: */
        case 0x26:  /* es: */
        case 0x64:  /* fs: */
        case 0x65:  /* gs: */
        case 0x66:  /* opcode size prefix */
        case 0x67:  /* addr size prefix */
        case 0xf0:  /* lock */
        case 0xf2:  /* repne */
        case 0xf3:  /* repe */
            instr++;
            continue;

          /* Handle call instructions */

        case 0xcd:  /* int <intno> */
        case 0xe8:  /* call <offset> */
        case 0x9a:  /* lcall <seg>:<off> */
            return TRUE;

        case 0xff:  /* call <regmodrm> */
            return (((instr[1] & 0x38) == 0x10) ||
                    ((instr[1] & 0x38) == 0x18));

          /* Handle string instructions */

        case 0x6c:  /* insb */
        case 0x6d:  /* insw */
        case 0x6e:  /* outsb */
        case 0x6f:  /* outsw */
        case 0xa4:  /* movsb */
        case 0xa5:  /* movsw */
        case 0xa6:  /* cmpsb */
        case 0xa7:  /* cmpsw */
        case 0xaa:  /* stosb */
        case 0xab:  /* stosw */
        case 0xac:  /* lodsb */
        case 0xad:  /* lodsw */
        case 0xae:  /* scasb */
        case 0xaf:  /* scasw */
            return TRUE;

        default:
            return FALSE;
        }
    }
}


/***********************************************************************
 *           DEBUG_IsFctReturn
 *
 * Determine if the instruction at CS:EIP is an instruction that
 * is a function return.
 */
BOOL32 DEBUG_IsFctReturn(void)
{
    BYTE *instr = (BYTE *)CTX_SEG_OFF_TO_LIN( &DEBUG_context,
                                              CS_reg(&DEBUG_context),
                                              EIP_reg(&DEBUG_context) );

    for (;;)
    {
        switch(*instr)
        {
	case 0xc2:
	case 0xc3:
	  return TRUE;
        default:
            return FALSE;
        }
    }
}


/***********************************************************************
 *           DEBUG_SetBreakpoints
 *
 * Set or remove all the breakpoints.
 */
void DEBUG_SetBreakpoints( BOOL32 set )
{
    int i;

    for (i = 0; i < MAX_BREAKPOINTS; i++)
    {
        if (breakpoints[i].in_use && breakpoints[i].enabled)
        {
            /* Note: we check for read here, because if reading is allowed */
            /*       writing permission will be forced in DEBUG_SetOpcode. */
            if (DEBUG_IsBadReadPtr( &breakpoints[i].addr, 1 ))
            {
                fprintf( stderr, "Invalid address for breakpoint %d, disabling it\n", i );
                breakpoints[i].enabled = FALSE;
            }
            else DEBUG_SetOpcode( &breakpoints[i].addr,
                                  set ? INT3 : breakpoints[i].opcode );
        }
    }
}


/***********************************************************************
 *           DEBUG_FindBreakpoint
 *
 * Find the breakpoint for a given address. Return the breakpoint
 * number or -1 if none.
 */
int DEBUG_FindBreakpoint( const DBG_ADDR *addr )
{
    int i;

    for (i = 0; i < MAX_BREAKPOINTS; i++)
    {
        if (breakpoints[i].in_use && breakpoints[i].enabled &&
            breakpoints[i].addr.seg == addr->seg &&
            breakpoints[i].addr.off == addr->off) return i;
    }
    return -1;
}


/***********************************************************************
 *           DEBUG_AddBreakpoint
 *
 * Add a breakpoint.
 */
void DEBUG_AddBreakpoint( const DBG_ADDR *address )
{
    DBG_ADDR addr = *address;
    int num;
    unsigned int seg2;
    BYTE *p;

    DBG_FIX_ADDR_SEG( &addr, CS_reg(&DEBUG_context) );

    if( addr.type != NULL && addr.type == DEBUG_TypeIntConst )
      {
	/*
	 * We know that we have the actual offset stored somewhere
	 * else in 32-bit space.  Grab it, and we
	 * should be all set.
	 */
	seg2 = addr.seg;
	addr.seg = 0;
	addr.off = DEBUG_GetExprValue(&addr, NULL);
	addr.seg = seg2;
      }
    if (!DBG_CHECK_READ_PTR( &addr, 1 )) return;

    if (next_bp < MAX_BREAKPOINTS)
        num = next_bp++;
    else  /* try to find an empty slot */  
    {
        for (num = 1; num < MAX_BREAKPOINTS; num++)
            if (!breakpoints[num].in_use) break;
        if (num >= MAX_BREAKPOINTS)
        {
            fprintf( stderr, "Too many breakpoints. Please delete some.\n" );
            return;
        }
    }
    p = DBG_ADDR_TO_LIN( &addr );
    breakpoints[num].addr    = addr;
    breakpoints[num].addrlen = !addr.seg ? 32 :
                         (GET_SEL_FLAGS(addr.seg) & LDT_FLAGS_32BIT) ? 32 : 16;
    breakpoints[num].opcode  = *p;
    breakpoints[num].enabled = TRUE;
    breakpoints[num].in_use  = TRUE;
    breakpoints[num].skipcount = 0;
    fprintf( stderr, "Breakpoint %d at ", num );
    DEBUG_PrintAddress( &breakpoints[num].addr, breakpoints[num].addrlen,
			TRUE );
    fprintf( stderr, "\n" );
}


/***********************************************************************
 *           DEBUG_DelBreakpoint
 *
 * Delete a breakpoint.
 */
void DEBUG_DelBreakpoint( int num )
{
    if ((num <= 0) || (num >= next_bp) || !breakpoints[num].in_use)
    {
        fprintf( stderr, "Invalid breakpoint number %d\n", num );
        return;
    }

    if( breakpoints[num].condition != NULL )
      {
	DEBUG_FreeExpr(breakpoints[num].condition);
	breakpoints[num].condition = NULL;
      }

    breakpoints[num].enabled = FALSE;
    breakpoints[num].in_use  = FALSE;
    breakpoints[num].skipcount = 0;
}


/***********************************************************************
 *           DEBUG_EnableBreakpoint
 *
 * Enable or disable a break point.
 */
void DEBUG_EnableBreakpoint( int num, BOOL32 enable )
{
    if ((num <= 0) || (num >= next_bp) || !breakpoints[num].in_use)
    {
        fprintf( stderr, "Invalid breakpoint number %d\n", num );
        return;
    }
    breakpoints[num].enabled = enable;
    breakpoints[num].skipcount = 0;
}


/***********************************************************************
 *           DEBUG_InfoBreakpoints
 *
 * Display break points information.
 */
void DEBUG_InfoBreakpoints(void)
{
    int i;

    fprintf( stderr, "Breakpoints:\n" );
    for (i = 1; i < next_bp; i++)
    {
        if (breakpoints[i].in_use)
        {
            fprintf( stderr, "%d: %c ", i, breakpoints[i].enabled ? 'y' : 'n');
            DEBUG_PrintAddress( &breakpoints[i].addr, breakpoints[i].addrlen,
				TRUE);
            fprintf( stderr, "\n" );
	    if( breakpoints[i].condition != NULL )
	      {
		fprintf(stderr, "\t\tstop when  ");
 		DEBUG_DisplayExpr(breakpoints[i].condition);
		fprintf(stderr, "\n");
	      }
        }
    }
}


/***********************************************************************
 *           DEBUG_AddTaskEntryBreakpoint
 *
 * Add a breakpoint at the entry point of the given task
 */
void DEBUG_AddTaskEntryBreakpoint( HTASK16 hTask )
{
    TDB *pTask = (TDB *)GlobalLock16( hTask );
    NE_MODULE *pModule;
    DBG_ADDR addr = { NULL, 0, 0 };

    if ( pTask )
    {
        if (!(pModule = NE_GetPtr( pTask->hModule ))) return;
        if (pModule->flags & NE_FFLAGS_LIBMODULE) return;  /* Library */

        if (pModule->lpDosTask) { /* DOS module */
            addr.seg = pModule->lpDosTask->init_cs | ((DWORD)pModule->self << 16);
            addr.off = pModule->lpDosTask->init_ip;
            fprintf( stderr, "DOS task '%s': ", NE_MODULE_NAME( pModule ) );
            DEBUG_AddBreakpoint( &addr );
        } else
        if (!(pModule->flags & NE_FFLAGS_WIN32))  /* NE module */
        {
            addr.seg =
		GlobalHandleToSel(NE_SEG_TABLE(pModule)[pModule->cs-1].hSeg);
            addr.off = pModule->ip;
            fprintf( stderr, "Win16 task '%s': ", NE_MODULE_NAME( pModule ) );
            DEBUG_AddBreakpoint( &addr );
	}
	else /* PE module */
	{
	    addr.seg = 0;
	    addr.off = (DWORD)RVA_PTR( pModule->module32,
			   OptionalHeader.AddressOfEntryPoint);
	    fprintf( stderr, "Win32 task '%s': ", NE_MODULE_NAME( pModule ) );
	    DEBUG_AddBreakpoint( &addr );
	}
    }

    DEBUG_SetBreakpoints( TRUE );  /* Setup breakpoints */
}


/***********************************************************************
 *           DEBUG_ShouldContinue
 *
 * Determine if we should continue execution after a SIGTRAP signal when
 * executing in the given mode.
 */
BOOL32 DEBUG_ShouldContinue( enum exec_mode mode, int * count )
{
    DBG_ADDR addr;
    DBG_ADDR cond_addr;
    int bpnum;
    struct list_id list;
    TDB *pTask = (TDB*)GlobalLock16( GetCurrentTask() );

      /* If not single-stepping, back up over the int3 instruction */
    if (!(EFL_reg(&DEBUG_context) & STEP_FLAG)) EIP_reg(&DEBUG_context)--;

    addr.seg = CS_reg(&DEBUG_context);
    addr.off = EIP_reg(&DEBUG_context);
    if (ISV86(&DEBUG_context)) addr.seg |= (DWORD)(pTask?(pTask->hModule):0)<<16; else
    if (IS_SELECTOR_SYSTEM(addr.seg)) addr.seg = 0;

    GlobalUnlock16( GetCurrentTask() );

    bpnum = DEBUG_FindBreakpoint( &addr );
    breakpoints[0].enabled = 0;  /* disable the step-over breakpoint */

    if ((bpnum != 0) && (bpnum != -1))
    {
        if( breakpoints[bpnum].condition != NULL )
	  {
	    cond_addr = DEBUG_EvalExpr(breakpoints[bpnum].condition);
	    if( cond_addr.type == NULL )
	      {
		/*
		 * Something wrong - unable to evaluate this expression.
		 */
		fprintf(stderr, "Unable to evaluate expression ");
 		DEBUG_DisplayExpr(breakpoints[bpnum].condition);
		fprintf(stderr, "\nTurning off condition\n");
		DEBUG_AddBPCondition(bpnum, NULL);
	      }
	    else if( ! DEBUG_GetExprValue( &cond_addr, NULL) )
	      {
		return TRUE;
	      }
	  }

        if( breakpoints[bpnum].skipcount > 0 )
	  {
	    breakpoints[bpnum].skipcount--;
	    if( breakpoints[bpnum].skipcount > 0 )
	      {
		return TRUE;
	      }
	  }
        fprintf( stderr, "Stopped on breakpoint %d at ", bpnum );
        DEBUG_PrintAddress( &breakpoints[bpnum].addr,
                            breakpoints[bpnum].addrlen, TRUE );
        fprintf( stderr, "\n" );
	
	/*
	 * See if there is a source file for this bp.  If so,
	 * then dig it out and display one line.
	 */
	DEBUG_FindNearestSymbol( &addr, TRUE, NULL, 0, &list);
	if( list.sourcefile != NULL )
	  {
	    DEBUG_List(&list, NULL, 0);
	  }
        return FALSE;
    }

    /*
     * If our mode indicates that we are stepping line numbers,
     * get the current function, and figure out if we are exactly
     * on a line number or not.
     */
    if( mode == EXEC_STEP_OVER 
	|| mode == EXEC_STEP_INSTR )
      {
	if( DEBUG_CheckLinenoStatus(&addr) == AT_LINENUMBER )
	  {
	    (*count)--;
	  }
      }
    else if( mode == EXEC_STEPI_OVER 
	|| mode == EXEC_STEPI_INSTR )

      {
	(*count)--;
      }

    if( *count > 0 || mode == EXEC_FINISH )
      {
	/*
	 * We still need to execute more instructions.
	 */
	return TRUE;
      }
    
    /*
     * If we are about to stop, then print out the source line if we
     * have it.
     */
    if( (mode != EXEC_CONT && mode != EXEC_FINISH) )
      {
	DEBUG_FindNearestSymbol( &addr, TRUE, NULL, 0, &list);
	if( list.sourcefile != NULL )
	  {
	    DEBUG_List(&list, NULL, 0);
	  }
      }

    /* If there's no breakpoint and we are not single-stepping, then we     */
    /* must have encountered an int3 in the Windows program; let's skip it. */
    if ((bpnum == -1) && !(EFL_reg(&DEBUG_context) & STEP_FLAG))
        EIP_reg(&DEBUG_context)++;

      /* no breakpoint, continue if in continuous mode */
    return (mode == EXEC_CONT || mode == EXEC_FINISH);
}


/***********************************************************************
 *           DEBUG_RestartExecution
 *
 * Set the breakpoints to the correct state to restart execution
 * in the given mode.
 */
enum exec_mode DEBUG_RestartExecution( enum exec_mode mode, int count )
{
    DBG_ADDR addr;
    DBG_ADDR addr2;
    int bp;
    int	delta;
    int status;
    unsigned int * value;
    enum exec_mode ret_mode;
    BYTE *instr;
    TDB *pTask = (TDB*)GlobalLock16( GetCurrentTask() );

    addr.seg = CS_reg(&DEBUG_context);
    addr.off = EIP_reg(&DEBUG_context);
    if (ISV86(&DEBUG_context)) addr.seg |= (DWORD)(pTask?(pTask->hModule):0)<<16; else
    if (IS_SELECTOR_SYSTEM(addr.seg)) addr.seg = 0;

    GlobalUnlock16( GetCurrentTask() );

    /*
     * This is the mode we will be running in after we finish.  We would like
     * to be able to modify this in certain cases.
     */
    ret_mode = mode;

    bp = DEBUG_FindBreakpoint( &addr ); 
    if ( bp != -1 && bp != 0)
      {
	/*
	 * If we have set a new value, then save it in the BP number.
	 */
	if( count != 0 && mode == EXEC_CONT )
	  {
	    breakpoints[bp].skipcount = count;
	  }
        mode = EXEC_STEPI_INSTR;  /* If there's a breakpoint, skip it */
      }
    else
      {
	if( mode == EXEC_CONT && count > 1 )
	  {
	    fprintf(stderr,"Not stopped at any breakpoint; argument ignored.\n");
	  }
      }
    
    if( mode == EXEC_FINISH && DEBUG_IsFctReturn() )
      {
	mode = ret_mode = EXEC_STEPI_INSTR;
      }

    instr = (BYTE *)CTX_SEG_OFF_TO_LIN( &DEBUG_context,
					CS_reg(&DEBUG_context),
					EIP_reg(&DEBUG_context) );
    /*
     * See if the function we are stepping into has debug info
     * and line numbers.  If not, then we step over it instead.
     * FIXME - we need to check for things like thunks or trampolines,
     * as the actual function may in fact have debug info.
     */
    if( *instr == 0xe8 )
      {
	delta = *(unsigned int*) (instr + 1);
	addr2 = addr;
	DEBUG_Disasm(&addr2, FALSE);
	addr2.off += delta;
	
	status = DEBUG_CheckLinenoStatus(&addr2);
	/*
	 * Anytime we have a trampoline, step over it.
	 */
	if( ((mode == EXEC_STEP_OVER) || (mode == EXEC_STEPI_OVER))
	    && status == FUNC_IS_TRAMPOLINE )
	  {
#if 0
	    fprintf(stderr, "Not stepping into trampoline at %x (no lines)\n",
		    addr2.off);
#endif
	    mode = EXEC_STEP_OVER_TRAMPOLINE;
	  }
	
	if( mode == EXEC_STEP_INSTR && status == FUNC_HAS_NO_LINES )
	  {
#if 0
	    fprintf(stderr, "Not stepping into function at %x (no lines)\n",
		    addr2.off);
#endif
	    mode = EXEC_STEP_OVER;
	  }
      }


    if( mode == EXEC_STEP_INSTR )
      {
	if( DEBUG_CheckLinenoStatus(&addr) == FUNC_HAS_NO_LINES )
	  {
	    fprintf(stderr, "Single stepping until exit from function, \n");
	    fprintf(stderr, "which has no line number information.\n");
	    
	    ret_mode = mode = EXEC_FINISH;
	  }
      }

    switch(mode)
    {
    case EXEC_CONT: /* Continuous execution */
        EFL_reg(&DEBUG_context) &= ~STEP_FLAG;
        DEBUG_SetBreakpoints( TRUE );
        break;

    case EXEC_STEP_OVER_TRAMPOLINE:
      /*
       * This is the means by which we step over our conversion stubs
       * in callfrom*.s and callto*.s.  We dig the appropriate address
       * off the stack, and we set the breakpoint there instead of the
       * address just after the call.
       */
      value = (unsigned int *) ESP_reg(&DEBUG_context) + 2;
      addr.off = *value;
      EFL_reg(&DEBUG_context) &= ~STEP_FLAG;
      breakpoints[0].addr    = addr;
      breakpoints[0].enabled = TRUE;
      breakpoints[0].in_use  = TRUE;
      breakpoints[0].skipcount = 0;
      breakpoints[0].opcode  = *(BYTE *)DBG_ADDR_TO_LIN( &addr );
      DEBUG_SetBreakpoints( TRUE );
      break;

    case EXEC_FINISH:
    case EXEC_STEPI_OVER:  /* Stepping over a call */
    case EXEC_STEP_OVER:  /* Stepping over a call */
        if (DEBUG_IsStepOverInstr())
        {
            EFL_reg(&DEBUG_context) &= ~STEP_FLAG;
            DEBUG_Disasm(&addr, FALSE);
            breakpoints[0].addr    = addr;
            breakpoints[0].enabled = TRUE;
            breakpoints[0].in_use  = TRUE;
	    breakpoints[0].skipcount = 0;
            breakpoints[0].opcode  = *(BYTE *)DBG_ADDR_TO_LIN( &addr );
            DEBUG_SetBreakpoints( TRUE );
            break;
        }
        /* else fall through to single-stepping */

    case EXEC_STEP_INSTR: /* Single-stepping an instruction */
    case EXEC_STEPI_INSTR: /* Single-stepping an instruction */
        EFL_reg(&DEBUG_context) |= STEP_FLAG;
        break;
    }
    return ret_mode;
}

int
DEBUG_AddBPCondition(int num, struct expr * exp)
{
    if ((num <= 0) || (num >= next_bp) || !breakpoints[num].in_use)
    {
        fprintf( stderr, "Invalid breakpoint number %d\n", num );
        return FALSE;
    }

    if( breakpoints[num].condition != NULL )
      {
	DEBUG_FreeExpr(breakpoints[num].condition);
	breakpoints[num].condition = NULL;
      }

    if( exp != NULL )
      {
	breakpoints[num].condition = DEBUG_CloneExpr(exp);
      }

   return TRUE;
}
