/*
 * Debugger break-points handling
 *
 * Copyright 1994 Martin von Loewis
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#ifdef linux
#include <sys/utsname.h>
#endif
#include "windows.h"
#include "debugger.h"

#define INT3          0xcc   /* int 3 opcode */
#define STEP_FLAG     0x100  /* single-step flag */

#define MAX_BREAKPOINTS 25

typedef struct
{
    unsigned int  segment;
    unsigned int  addr;
    BYTE          addrlen;
    BYTE          opcode;
    BOOL          enabled;
    BOOL          in_use;
} BREAKPOINT;

static BREAKPOINT breakpoints[MAX_BREAKPOINTS] = { { 0, }, };

static int next_bp = 1;  /* breakpoint 0 is reserved for step-over */


/***********************************************************************
 *           DEBUG_ChangeOpcode
 *
 * Change the opcode at segment:addr.
 */
static void DEBUG_SetOpcode( unsigned int segment, unsigned int addr, BYTE op )
{
    if (segment)
    {
        *(BYTE *)PTR_SEG_OFF_TO_LIN( segment, addr ) = op;
    }
    else  /* 32-bit code, so we have to change the protection first */
    {
        /* There are a couple of problems with this. On Linux prior to
           1.1.62, this call fails (ENOACCESS) due to a bug in fs/exec.c.
           This code is currently not tested at all on BSD.
           How do I determine the page size in a more symbolic manner?
           And why does mprotect need that start address of the page
           in the first place?
           Not that portability matters, this code is i386 only anyways...
           How do I get the old protection in order to restore it later on?
        */
        if (mprotect((caddr_t)(addr & (~4095)), 4096,
                     PROT_READ|PROT_WRITE|PROT_EXEC) == -1)
        {
            perror( "Can't set break point" );
            return;
	}
        *(BYTE *)addr = op;
	mprotect((caddr_t)(addr & ~4095), 4096, PROT_READ|PROT_EXEC);
    }
}


/***********************************************************************
 *           DEBUG_SetBreakpoints
 *
 * Set or remove all the breakpoints.
 */
void DEBUG_SetBreakpoints( BOOL set )
{
    int i;

    for (i = 0; i < MAX_BREAKPOINTS; i++)
    {
        if (breakpoints[i].in_use && breakpoints[i].enabled)
            DEBUG_SetOpcode( breakpoints[i].segment, breakpoints[i].addr,
                             set ? INT3 : breakpoints[i].opcode );
    }
}


/***********************************************************************
 *           DEBUG_FindBreakpoint
 *
 * Find the breakpoint for a given address. Return the breakpoint
 * number or -1 if none.
 */
int DEBUG_FindBreakpoint( unsigned int segment, unsigned int addr )
{
    int i;

    for (i = 0; i < MAX_BREAKPOINTS; i++)
    {
        if (breakpoints[i].in_use && breakpoints[i].enabled &&
            breakpoints[i].segment == segment && breakpoints[i].addr == addr)
            return i;
    }
    return -1;
}


/***********************************************************************
 *           DEBUG_AddBreakpoint
 *
 * Add a breakpoint.
 */
void DEBUG_AddBreakpoint( unsigned int segment, unsigned int addr )
{
    int num;
    BYTE *p;

    if (segment == 0xffffffff) segment = CS;
    if (segment == WINE_CODE_SELECTOR) segment = 0;

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
    p = segment ? (BYTE *)PTR_SEG_OFF_TO_LIN( segment, addr ) : (BYTE *)addr;
    breakpoints[num].segment = segment;
    breakpoints[num].addr    = addr;
    breakpoints[num].addrlen = !segment ? 32 :
                          (GET_SEL_FLAGS(segment) & LDT_FLAGS_32BIT) ? 32 : 16;
    breakpoints[num].opcode  = *p;
    breakpoints[num].enabled = TRUE;
    breakpoints[num].in_use  = TRUE;
    fprintf( stderr, "Breakpoint %d at ", num );
    print_address( segment, addr, breakpoints[num].addrlen );
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
void DEBUG_EnableBreakpoint( int num, BOOL enable )
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
            print_address( breakpoints[i].segment, breakpoints[i].addr,
                           breakpoints[i].addrlen );
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
BOOL DEBUG_ShouldContinue( struct sigcontext_struct *context,
                           enum exec_mode mode )
{
    unsigned int segment, addr;
    int bpnum;

      /* If not single-stepping, back up over the int3 instruction */
    if (!(EFL & STEP_FLAG)) EIP--;

    segment = (CS == WINE_CODE_SELECTOR) ? 0 : CS;
    addr    = EIP;
    bpnum  = DEBUG_FindBreakpoint( segment, addr );
    breakpoints[0].enabled = 0;  /* disable the step-over breakpoint */

    if ((bpnum != 0) && (bpnum != -1))
    {
        fprintf( stderr, "Stopped on breakpoint %d at ", bpnum );
        print_address( breakpoints[bpnum].segment, breakpoints[bpnum].addr,
                       breakpoints[bpnum].addrlen );
        fprintf( stderr, "\n" );
        return FALSE;
    }
      /* no breakpoint, continue if in continuous mode */
    return (mode == EXEC_CONT);
}


/***********************************************************************
 *           DEBUG_RestartExecution
 *
 * Set the breakpoints to the correct state to restart execution
 * in the given mode.
 */
void DEBUG_RestartExecution( struct sigcontext_struct *context,
                             enum exec_mode mode, int instr_len )
{
    unsigned int segment, addr;

    segment = (CS == WINE_CODE_SELECTOR) ? 0 : CS;
    addr    = EIP;

    if (DEBUG_FindBreakpoint( segment, addr ) != -1)
        mode = EXEC_STEP_INSTR;  /* If there's a breakpoint, skip it */

    switch(mode)
    {
    case EXEC_CONT: /* Continuous execution */
        EFL &= ~STEP_FLAG;
        DEBUG_SetBreakpoints( TRUE );
        break;

    case EXEC_STEP_OVER:  /* Stepping over a call */
        EFL &= ~STEP_FLAG;
        breakpoints[0].segment = segment;
        breakpoints[0].addr    = addr + instr_len;
        breakpoints[0].enabled = TRUE;
        breakpoints[0].in_use  = TRUE;
        breakpoints[0].opcode  = segment ?
           *(BYTE *)PTR_SEG_OFF_TO_LIN(segment,addr+instr_len) : *(BYTE *)addr;
        DEBUG_SetBreakpoints( TRUE );
        break;

    case EXEC_STEP_INSTR: /* Single-stepping an instruction */
        EFL |= STEP_FLAG;
        break;
    }
}
