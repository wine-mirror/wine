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
#include "windows.h"
#include "debugger.h"

#define INT3          0xcc   /* int 3 opcode */

#define MAX_BREAKPOINTS 25

typedef struct
{
    DBG_ADDR    addr;
    BYTE        addrlen;
    BYTE        opcode;
    BOOL16      enabled;
    BOOL16      in_use;
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
    BYTE *ptr;

    if (addr->seg) ptr = (BYTE *)PTR_SEG_OFF_TO_LIN( addr->seg, addr->off );
    else ptr = (BYTE *)addr->off;

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
static BOOL32 DEBUG_IsStepOverInstr( SIGCONTEXT *context )
{
    BYTE *instr = (BYTE *)PTR_SEG_OFF_TO_LIN(CS_reg(context),EIP_reg(context));

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
    BYTE *p;

    DBG_FIX_ADDR_SEG( &addr, CS_reg(DEBUG_context) );

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
    if (!DBG_CHECK_READ_PTR( &addr, 1 )) return;
    p = DBG_ADDR_TO_LIN( &addr );
    breakpoints[num].addr    = addr;
    breakpoints[num].addrlen = !addr.seg ? 32 :
                         (GET_SEL_FLAGS(addr.seg) & LDT_FLAGS_32BIT) ? 32 : 16;
    breakpoints[num].opcode  = *p;
    breakpoints[num].enabled = TRUE;
    breakpoints[num].in_use  = TRUE;
    fprintf( stderr, "Breakpoint %d at ", num );
    DEBUG_PrintAddress( &breakpoints[num].addr, breakpoints[num].addrlen );
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
    breakpoints[num].enabled = FALSE;
    breakpoints[num].in_use  = FALSE;
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
            DEBUG_PrintAddress( &breakpoints[i].addr, breakpoints[i].addrlen );
            fprintf( stderr, "\n" );
        }
    }
}


/***********************************************************************
 *           DEBUG_ShouldContinue
 *
 * Determine if we should continue execution after a SIGTRAP signal when
 * executing in the given mode.
 */
BOOL32 DEBUG_ShouldContinue( SIGCONTEXT *context, enum exec_mode mode )
{
    DBG_ADDR addr;
    int bpnum;

      /* If not single-stepping, back up over the int3 instruction */
    if (!(EFL_reg(DEBUG_context) & STEP_FLAG)) EIP_reg(DEBUG_context)--;

    addr.seg = (CS_reg(DEBUG_context) == WINE_CODE_SELECTOR) ? 
                0 : CS_reg(DEBUG_context);
    addr.off = EIP_reg(DEBUG_context);
        
    bpnum = DEBUG_FindBreakpoint( &addr );
    breakpoints[0].enabled = 0;  /* disable the step-over breakpoint */

    if ((bpnum != 0) && (bpnum != -1))
    {
        fprintf( stderr, "Stopped on breakpoint %d at ", bpnum );
        DEBUG_PrintAddress( &breakpoints[bpnum].addr,
                            breakpoints[bpnum].addrlen );
        fprintf( stderr, "\n" );
        return FALSE;
    }

    /* If there's no breakpoint and we are not single-stepping, then we     */
    /* must have encountered an int3 in the Windows program; let's skip it. */
    if ((bpnum == -1) && !(EFL_reg(DEBUG_context) & STEP_FLAG))
        EIP_reg(DEBUG_context)++;

      /* no breakpoint, continue if in continuous mode */
    return (mode == EXEC_CONT);
}


/***********************************************************************
 *           DEBUG_RestartExecution
 *
 * Set the breakpoints to the correct state to restart execution
 * in the given mode.
 */
void DEBUG_RestartExecution( SIGCONTEXT *context, enum exec_mode mode,
                             int instr_len )
{
    DBG_ADDR addr;

    addr.seg = (CS_reg(DEBUG_context) == WINE_CODE_SELECTOR) ?
                0 : CS_reg(DEBUG_context);
    addr.off = EIP_reg(DEBUG_context);

    if (DEBUG_FindBreakpoint( &addr ) != -1)
        mode = EXEC_STEP_INSTR;  /* If there's a breakpoint, skip it */

    switch(mode)
    {
    case EXEC_CONT: /* Continuous execution */
        EFL_reg(DEBUG_context) &= ~STEP_FLAG;
        DEBUG_SetBreakpoints( TRUE );
        break;

    case EXEC_STEP_OVER:  /* Stepping over a call */
        if (DEBUG_IsStepOverInstr(DEBUG_context))
        {
            EFL_reg(DEBUG_context) &= ~STEP_FLAG;
            addr.off += instr_len;
            breakpoints[0].addr    = addr;
            breakpoints[0].enabled = TRUE;
            breakpoints[0].in_use  = TRUE;
            breakpoints[0].opcode  = *(BYTE *)DBG_ADDR_TO_LIN( &addr );
            DEBUG_SetBreakpoints( TRUE );
            break;
        }
        /* else fall through to single-stepping */

    case EXEC_STEP_INSTR: /* Single-stepping an instruction */
        EFL_reg(DEBUG_context) |= STEP_FLAG;
        break;
    }
}
