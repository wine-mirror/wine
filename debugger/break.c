/*
 * Debugger break-points handling
 *
 * Copyright 1994 Martin von Loewis
 * Copyright 1995 Alexandre Julliard
 * Copyright 1999,2000 Eric Pouech
 */

#include "config.h"
#include "debugger.h"

#ifdef __i386__
#define DR7_CONTROL_SHIFT	16
#define DR7_CONTROL_SIZE 	4

#define DR7_RW_EXECUTE 		(0x0)
#define DR7_RW_WRITE		(0x1)
#define DR7_RW_READ		(0x3)

#define DR7_LEN_1		(0x0)
#define DR7_LEN_2		(0x4)
#define DR7_LEN_4		(0xC)

#define DR7_LOCAL_ENABLE_SHIFT	0
#define DR7_GLOBAL_ENABLE_SHIFT 1
#define DR7_ENABLE_SIZE 	2

#define DR7_LOCAL_ENABLE_MASK	(0x55)
#define DR7_GLOBAL_ENABLE_MASK	(0xAA)

#define DR7_CONTROL_RESERVED	(0xFC00)
#define DR7_LOCAL_SLOWDOWN	(0x100)
#define DR7_GLOBAL_SLOWDOWN	(0x200)

#define	DR7_ENABLE_MASK(dr)	(1<<(DR7_LOCAL_ENABLE_SHIFT+DR7_ENABLE_SIZE*(dr)))
#define	IS_DR7_SET(ctrl,dr) 	((ctrl)&DR7_ENABLE_MASK(dr))
#define INT3          0xcc   /* int 3 opcode */
#endif

#define MAX_BREAKPOINTS 100

static DBG_BREAKPOINT breakpoints[MAX_BREAKPOINTS];

static int next_bp = 1;  /* breakpoint 0 is reserved for step-over */

/***********************************************************************
 *           DEBUG_IsStepOverInstr
 *
 * Determine if the instruction at CS:EIP is an instruction that
 * we need to step over (like a call or a repetitive string move).
 */
static BOOL DEBUG_IsStepOverInstr(void)
{
#ifdef __i386__
    BYTE*	instr;
    BYTE	ch;
    DBG_ADDR	addr;

    addr.seg = DEBUG_context.SegCs;
    addr.off = DEBUG_context.Eip;
    /* FIXME: old code was using V86BASE(DEBUG_context)
     * instead of passing thru DOSMEM_MemoryBase
     */
    instr = (BYTE*)DEBUG_ToLinear(&addr);

    for (;;)
    {
        if (!DEBUG_READ_MEM(instr, &ch, sizeof(ch)))
	    return FALSE;

        switch (ch)
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
	    if (!DEBUG_READ_MEM(instr + 1, &ch, sizeof(ch)))
	        return FALSE;
	    return (((ch & 0x38) == 0x10) || ((ch & 0x38) == 0x18));

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
#else
    return FALSE;
#endif
}


/***********************************************************************
 *           DEBUG_IsFctReturn
 *
 * Determine if the instruction at CS:EIP is an instruction that
 * is a function return.
 */
BOOL DEBUG_IsFctReturn(void)
{
#ifdef __i386__
    BYTE*	instr;
    BYTE  	ch;
    DBG_ADDR	addr;

    addr.seg = DEBUG_context.SegCs;
    addr.off = DEBUG_context.Eip;
    /* FIXME: old code was using V86BASE(DEBUG_context)
     * instead of passing thru DOSMEM_MemoryBase
     */
    instr = (BYTE*)DEBUG_ToLinear(&addr);

    if (!DEBUG_READ_MEM(instr, &ch, sizeof(ch)))
        return FALSE;

    return (ch == 0xc2) || (ch == 0xc3);
#else
    return FALSE;
#endif
}


/***********************************************************************
 *           DEBUG_SetBreakpoints
 *
 * Set or remove all the breakpoints.
 */
void DEBUG_SetBreakpoints( BOOL set )
{
   int		i;
   
#ifdef __i386__
   DEBUG_context.Dr7 &= ~DR7_LOCAL_ENABLE_MASK;
#endif
   
   for (i = 0; i < next_bp; i++)
   {
      if (!(breakpoints[i].refcount && breakpoints[i].enabled))
	 continue;
      
      switch (breakpoints[i].type) {
      case DBG_BREAK:
	 {
#ifdef __i386__
	    char ch = set ? INT3 : breakpoints[i].u.b.opcode;
	    
	    if (!DEBUG_WRITE_MEM( (void*)DEBUG_ToLinear(&breakpoints[i].addr), 
				  &ch, sizeof(ch) ))
	    {
	       DEBUG_Printf(DBG_CHN_MESG, "Invalid address for breakpoint %d, disabling it\n", i);
	       breakpoints[i].enabled = FALSE;
	    }
#endif
	 }
	 break;
      case DBG_WATCH:
	 if (set) 
	 {
#ifdef __i386__
	    DWORD	bits;
	    int		reg = breakpoints[i].u.w.reg;
	    LPDWORD	lpdr = NULL;

	    switch (reg) 
	    {
	       case 0: lpdr = &DEBUG_context.Dr0; break;
	       case 1: lpdr = &DEBUG_context.Dr1; break;
	       case 2: lpdr = &DEBUG_context.Dr2; break;
	       case 3: lpdr = &DEBUG_context.Dr3; break;
	       default: RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
	    }
	    
	    *lpdr = DEBUG_ToLinear(&breakpoints[i].addr);
	    bits = (breakpoints[i].u.w.rw) ? DR7_RW_WRITE : DR7_RW_READ;
	    switch (breakpoints[i].u.w.len + 1)
	    {
	       case 4: bits |= DR7_LEN_4;	break;
	       case 2: bits |= DR7_LEN_2;	break;
	       case 1: bits |= DR7_LEN_1;	break;
	       default: RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
	    }
	    
	    DEBUG_context.Dr7 &= ~(0x0F << (DR7_CONTROL_SHIFT + DR7_CONTROL_SIZE * reg));
	    DEBUG_context.Dr7 |= bits << (DR7_CONTROL_SHIFT + DR7_CONTROL_SIZE * reg);
	    DEBUG_context.Dr7 |= DR7_ENABLE_MASK(reg) | DR7_LOCAL_SLOWDOWN;
#endif
	 }
	 break;
      }
   }
}

/***********************************************************************
 *           DEBUG_FindBreakpoint
 *
 * Find the breakpoint for a given address. Return the breakpoint
 * number or -1 if none.
 * If type is DBG_BREAKPOINT, addr is a complete addr
 * If type is DBG_WATCHPOINT, only addr.off is meaningful and contains 
 * linear address
 */
static int DEBUG_FindBreakpoint( const DBG_ADDR *addr, int type )
{
   int i;
   
   for (i = 0; i < next_bp; i++)
   {
      if (breakpoints[i].refcount && breakpoints[i].enabled &&
	  breakpoints[i].type == type )
      {
	 if ((type == DBG_BREAK && 
	      breakpoints[i].addr.seg == addr->seg &&
	      breakpoints[i].addr.off == addr->off) ||
	     (type == DBG_WATCH && 
	      DEBUG_ToLinear(&breakpoints[i].addr) == addr->off))
	    return i;
      }
   }
   return -1;
}

/***********************************************************************
 *           DEBUG_InitXPoint
 *
 * Find an empty slot in BP table to add a new break/watch point
 */
static	int	DEBUG_InitXPoint(int type, DBG_ADDR* addr)
{
   int	num;
   
   for (num = (next_bp < MAX_BREAKPOINTS) ? next_bp++ : 1; 
	num < MAX_BREAKPOINTS; num++) 
   {
      if (breakpoints[num].refcount == 0) 
      {
	 breakpoints[num].refcount = 1;
	 breakpoints[num].enabled = TRUE;
	 breakpoints[num].type    = type;
	 breakpoints[num].skipcount = 0;
	 breakpoints[num].addr = *addr;
	 breakpoints[num].is32 = 1;
#ifdef __i386__
	 if (addr->seg) 
	 {
	    switch (DEBUG_GetSelectorType( addr->seg )) 
	    {
	       case 32: break;
	       case 16: breakpoints[num].is32 = 0; break;
	       default: RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
	    }
	 }
#endif
	 return num;
      }
   }
   
   DEBUG_Printf( DBG_CHN_MESG, "Too many breakpoints. Please delete some.\n" );
   return -1;
}

/***********************************************************************
 *           DEBUG_GetWatchedValue
 *
 * Returns the value watched by watch point 'num'.
 */
static	BOOL	DEBUG_GetWatchedValue( int num, LPDWORD val )
{
   BYTE		buf[4];
   
   if (!DEBUG_READ_MEM((void*)DEBUG_ToLinear(&breakpoints[num].addr), 
		       buf, breakpoints[num].u.w.len + 1))
      return FALSE;
   
   switch (breakpoints[num].u.w.len + 1) 
   {
      case 4:	*val = *(DWORD*)buf;	break;
      case 2:	*val = *(WORD*)buf;	break;
      case 1:	*val = *(BYTE*)buf;	break;
      default: RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
   }
   return TRUE;
}

/***********************************************************************
 *           DEBUG_AddBreakpoint
 *
 * Add a breakpoint.
 */
void DEBUG_AddBreakpoint( const DBG_VALUE *_value, BOOL (*func)(void) )
{
    DBG_VALUE value = *_value;
    int num;
    unsigned int seg2;
    BYTE ch;

    assert(_value->cookie == DV_TARGET || _value->cookie == DV_HOST);

#ifdef __i386__
    DEBUG_FixAddress( &value.addr, DEBUG_context.SegCs );
#endif

    if( value.type != NULL && value.type == DEBUG_TypeIntConst )
    {
       /*
	* We know that we have the actual offset stored somewhere
	* else in 32-bit space.  Grab it, and we
	* should be all set.
	*/
       seg2 = value.addr.seg;
       value.addr.seg = 0;
       value.addr.off = DEBUG_GetExprValue(&value, NULL);
       value.addr.seg = seg2;
    }

    if ((num = DEBUG_FindBreakpoint(&value.addr, DBG_BREAK)) >= 1) 
    {
       breakpoints[num].refcount++;
       return;
    }

    if (!DEBUG_READ_MEM_VERBOSE((void*)DEBUG_ToLinear( &value.addr ), &ch, sizeof(ch)))
	return;

    if ((num = DEBUG_InitXPoint(DBG_BREAK, &value.addr)) == -1)
       return;

    breakpoints[num].u.b.opcode = ch;
    breakpoints[num].u.b.func = func;

    DEBUG_Printf( DBG_CHN_MESG, "Breakpoint %d at ", num );
    DEBUG_PrintAddress( &breakpoints[num].addr, breakpoints[num].is32 ? 32 : 16,
			TRUE );
    DEBUG_Printf( DBG_CHN_MESG, "\n" );
}


 /***********************************************************************
 *           DEBUG_AddWatchpoint
 *
 * Add a watchpoint.
 */
void DEBUG_AddWatchpoint( const DBG_VALUE *_value, BOOL is_write )
{
   DBG_VALUE	value = *_value;
   int		num, reg = -1;
   unsigned	seg2;
   DWORD	mask = 0;
   
   assert(_value->cookie == DV_TARGET || _value->cookie == DV_HOST);

#ifdef __i386__
   DEBUG_FixAddress( &value.addr, DEBUG_context.SegCs );
#endif
   
   if ( value.type != NULL && value.type == DEBUG_TypeIntConst )
   {
      /*
       * We know that we have the actual offset stored somewhere
       * else in 32-bit space.  Grab it, and we
       * should be all set.
       */
      seg2 = value.addr.seg;
      value.addr.seg = 0;
      value.addr.off = DEBUG_GetExprValue(&value, NULL);
      value.addr.seg = seg2;
   }
   
   for (num = 1; num < next_bp; num++) 
   {
      if (breakpoints[num].refcount && breakpoints[num].enabled && 
	  breakpoints[num].type == DBG_WATCH) {
	 mask |= (1 << breakpoints[num].u.w.reg);
      }
   }
#ifdef __i386__
   for (reg = 0; reg < 4 && (mask & (1 << reg)); reg++);
   if (reg == 4)
   {
      DEBUG_Printf(DBG_CHN_MESG, "All i386 hardware watchpoints have been set. Delete some\n");
      return;
   }
#endif

   if ((num = DEBUG_InitXPoint(DBG_WATCH, &value.addr)) == -1)
      return;
   
   breakpoints[num].u.w.len = 4 - 1;
   if (_value->type && DEBUG_GetObjectSize(_value->type) < 4)
      breakpoints[num].u.w.len = 2 - 1;
   
   if (!DEBUG_GetWatchedValue( num, &breakpoints[num].u.w.oldval)) 
   {
      DEBUG_Printf(DBG_CHN_MESG, "Bad address. Watchpoint not set\n");
      breakpoints[num].refcount = 0;
   }
   
   breakpoints[num].u.w.rw = (is_write) ? TRUE : FALSE;
   breakpoints[reg].u.w.reg = reg;
   
   DEBUG_Printf( DBG_CHN_MESG, "Watchpoint %d at ", num );
   DEBUG_PrintAddress( &breakpoints[num].addr, breakpoints[num].is32 ? 32:16, TRUE );
   DEBUG_Printf( DBG_CHN_MESG, "\n" );
}

/***********************************************************************
 *           DEBUG_DelBreakpoint
 *
 * Delete a breakpoint.
 */
void DEBUG_DelBreakpoint( int num )
{
    if ((num <= 0) || (num >= next_bp) || breakpoints[num].refcount == 0)
    {
        DEBUG_Printf( DBG_CHN_MESG, "Invalid breakpoint number %d\n", num );
        return;
    }

    if (--breakpoints[num].refcount > 0)
       return;

    if( breakpoints[num].condition != NULL )
    {
       DEBUG_FreeExpr(breakpoints[num].condition);
       breakpoints[num].condition = NULL;
    }

    breakpoints[num].enabled = FALSE;
    breakpoints[num].refcount = 0;
    breakpoints[num].skipcount = 0;
}

/***********************************************************************
 *           DEBUG_EnableBreakpoint
 *
 * Enable or disable a break point.
 */
void DEBUG_EnableBreakpoint( int num, BOOL enable )
{
    if ((num <= 0) || (num >= next_bp) || breakpoints[num].refcount == 0)
    {
        DEBUG_Printf( DBG_CHN_MESG, "Invalid breakpoint number %d\n", num );
        return;
    }
    breakpoints[num].enabled = (enable) ? TRUE : FALSE;
    breakpoints[num].skipcount = 0;
}


/***********************************************************************
 *           DEBUG_FindTriggeredWatchpoint
 *
 * Lookup the watchpoints to see if one has been triggered
 * Return >= (watch point index) if one is found and *oldval is set to
 * 	the value watched before the TRAP
 * Return -1 if none found (*oldval is undetermined)
 *
 * Unfortunately, Linux does *NOT* (A REAL PITA) report with ptrace 
 * the DR6 register value, so we have to look with our own need the
 * cause of the TRAP.
 * -EP
 */
static int DEBUG_FindTriggeredWatchpoint(LPDWORD oldval)
{
   int 				i;
   int				found = -1;
   
   /* Method 1 => get triggered watchpoint from context (doesn't work on Linux
    * 2.2.x)
    */
   for (i = 0; i < next_bp; i++) 
   {
#ifdef __i386__
      DWORD val = 0;

      if (breakpoints[i].refcount && breakpoints[i].enabled && 
	  breakpoints[i].type == DBG_WATCH &&
	  (DEBUG_context.Dr6 & (1 << breakpoints[i].u.w.reg)))
      {
	 DEBUG_context.Dr6 &= ~(1 << breakpoints[i].u.w.reg);
	 
	 *oldval = breakpoints[i].u.w.oldval;
	 if (DEBUG_GetWatchedValue(i, &val)) {
	    breakpoints[i].u.w.oldval = val;
	    return i;
	 }
      }
#endif
   }
   
   /* Method 1 failed, trying method2 */
   
   /* Method 2 => check if value has changed among registered watchpoints
    * this really sucks, but this is how gdb 4.18 works on my linux box
    * -EP
    */
   for (i = 0; i < next_bp; i++) 
   {
#ifdef __i386__
      DWORD val = 0;

      if (breakpoints[i].refcount && breakpoints[i].enabled && 
	  breakpoints[i].type == DBG_WATCH && 
	  DEBUG_GetWatchedValue(i, &val)) 
      {
	 *oldval = breakpoints[i].u.w.oldval;
	 if (val != *oldval) 
	 {
	    DEBUG_context.Dr6 &= ~(1 << breakpoints[i].u.w.reg);
	    breakpoints[i].u.w.oldval = val;
	    found = i;
	    /* cannot break, because two watch points may have been triggered on
	     * the same acces
	     * only one will be reported to the user (FIXME ?)
	     */
	 }
      }
#endif
   }
   return found;
}

/***********************************************************************
 *           DEBUG_InfoBreakpoints
 *
 * Display break points information.
 */
void DEBUG_InfoBreakpoints(void)
{
    int i;

    DEBUG_Printf( DBG_CHN_MESG, "Breakpoints:\n" );
    for (i = 1; i < next_bp; i++)
    {
        if (breakpoints[i].refcount && breakpoints[i].type == DBG_BREAK)
        {
            DEBUG_Printf( DBG_CHN_MESG, "%d: %c ", i, breakpoints[i].enabled ? 'y' : 'n');
            DEBUG_PrintAddress( &breakpoints[i].addr, 
				breakpoints[i].is32 ? 32 : 16, TRUE);
            DEBUG_Printf( DBG_CHN_MESG, " (%u)\n", breakpoints[i].refcount );
	    if( breakpoints[i].condition != NULL )
	    {
	        DEBUG_Printf(DBG_CHN_MESG, "\t\tstop when  ");
 		DEBUG_DisplayExpr(breakpoints[i].condition);
		DEBUG_Printf(DBG_CHN_MESG, "\n");
	    }
        }
    }
    DEBUG_Printf( DBG_CHN_MESG, "Watchpoints:\n" );
    for (i = 1; i < next_bp; i++)
    {
        if (breakpoints[i].refcount && breakpoints[i].type == DBG_WATCH)
        {
            DEBUG_Printf( DBG_CHN_MESG, "%d: %c ", i, breakpoints[i].enabled ? 'y' : 'n');
            DEBUG_PrintAddress( &breakpoints[i].addr, 
				breakpoints[i].is32 ? 32 : 16, TRUE);
            DEBUG_Printf( DBG_CHN_MESG, " on %d byte%s (%c)\n", 
		     breakpoints[i].u.w.len + 1, 
		     breakpoints[i].u.w.len > 0 ? "s" : "",
		     breakpoints[i].u.w.rw ? 'W' : 'R');
	    if( breakpoints[i].condition != NULL )
	    {
	        DEBUG_Printf(DBG_CHN_MESG, "\t\tstop when  ");
 		DEBUG_DisplayExpr(breakpoints[i].condition);
		DEBUG_Printf(DBG_CHN_MESG, "\n");
	    }
        }
    }
}

/***********************************************************************
 *           DEBUG_ShallBreak
 *
 * Check whether or not the condition (bp / skipcount) of a break/watch
 * point are met.
 */
static	BOOL DEBUG_ShallBreak( int bpnum )
{
   if ( breakpoints[bpnum].condition != NULL )
   {
      DBG_VALUE value = DEBUG_EvalExpr(breakpoints[bpnum].condition);

      if ( value.type == NULL )
      {
	 /*
	  * Something wrong - unable to evaluate this expression.
	  */
	 DEBUG_Printf(DBG_CHN_MESG, "Unable to evaluate expression ");
	 DEBUG_DisplayExpr(breakpoints[bpnum].condition);
	 DEBUG_Printf(DBG_CHN_MESG, "\nTurning off condition\n");
	 DEBUG_AddBPCondition(bpnum, NULL);
      }
      else if( !DEBUG_GetExprValue( &value, NULL) )
      {
	 return FALSE;
      }
   }
   
   if ( breakpoints[bpnum].skipcount > 0 && --breakpoints[bpnum].skipcount > 0 )
      return FALSE;

   return (breakpoints[bpnum].u.b.func) ? (breakpoints[bpnum].u.b.func)() : TRUE;
}

/***********************************************************************
 *           DEBUG_ShouldContinue
 *
 * Determine if we should continue execution after a SIGTRAP signal when
 * executing in the given mode.
 */
BOOL DEBUG_ShouldContinue( DWORD code, enum exec_mode mode, int * count )
{
    DBG_ADDR 	addr;
    int 	bpnum;
    DWORD	oldval;
    int 	wpnum;
    int		addrlen = 32;
    struct symbol_info syminfo;

#ifdef __i386__
    /* If not single-stepping, back up over the int3 instruction */
    if (code == EXCEPTION_BREAKPOINT) 
       DEBUG_context.Eip--;
#endif

    DEBUG_GetCurrentAddress( &addr );
    bpnum = DEBUG_FindBreakpoint( &addr, DBG_BREAK );
    breakpoints[0].enabled = FALSE;  /* disable the step-over breakpoint */

    if ((bpnum != 0) && (bpnum != -1))
    {
        if (!DEBUG_ShallBreak(bpnum)) return TRUE;

        DEBUG_Printf( DBG_CHN_MESG, "Stopped on breakpoint %d at ", bpnum );
        syminfo = DEBUG_PrintAddress( &breakpoints[bpnum].addr,
				      breakpoints[bpnum].is32 ? 32 : 16, TRUE );
        DEBUG_Printf( DBG_CHN_MESG, "\n" );
	
	if( syminfo.list.sourcefile != NULL )
	    DEBUG_List(&syminfo.list, NULL, 0);
        return FALSE;
    }

    wpnum = DEBUG_FindTriggeredWatchpoint(&oldval);
    if ((wpnum != 0) && (wpnum != -1))
    {
       /* If not single-stepping, do not back up over the int3 instruction */
       if (code == EXCEPTION_BREAKPOINT) 
       {
#ifdef __i386__
	   DEBUG_context.Eip++;
	   addr.off++;
#endif
       }
       if (!DEBUG_ShallBreak(wpnum)) return TRUE;
       
#ifdef __i386__
       addrlen = !addr.seg? 32 : DEBUG_GetSelectorType( addr.seg );
#endif
       DEBUG_Printf(DBG_CHN_MESG, "Stopped on watchpoint %d at ", wpnum);
       syminfo = DEBUG_PrintAddress( &addr, addrlen, TRUE );
       
       DEBUG_Printf(DBG_CHN_MESG, " values: old=%lu new=%lu\n", 
	       oldval, breakpoints[wpnum].u.w.oldval);
       if (syminfo.list.sourcefile != NULL)
	  DEBUG_List(&syminfo.list, NULL, 0);
       return FALSE;
    }

    /*
     * If our mode indicates that we are stepping line numbers,
     * get the current function, and figure out if we are exactly
     * on a line number or not.
     */
    if( mode == EXEC_STEP_OVER || mode == EXEC_STEP_INSTR )
    {
	if( DEBUG_CheckLinenoStatus(&addr) == AT_LINENUMBER )
	{
	    (*count)--;
	}
    }
    else if( mode == EXEC_STEPI_OVER || mode == EXEC_STEPI_INSTR )
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
    if (mode != EXEC_CONT && mode != EXEC_PASS && mode != EXEC_FINISH)
    {
	DEBUG_FindNearestSymbol( &addr, TRUE, NULL, 0, &syminfo.list);
	if( syminfo.list.sourcefile != NULL )
	{
	    DEBUG_List(&syminfo.list, NULL, 0);
	}
    }

#ifdef __i386__
    /* If there's no breakpoint and we are not single-stepping, then we     */
    /* must have encountered an int3 in the Windows program; let's skip it. */
    if ((bpnum == -1) && code == EXCEPTION_BREAKPOINT)
        DEBUG_context.Eip++;
#endif

    /* no breakpoint, continue if in continuous mode */
    return (mode == EXEC_CONT || mode == EXEC_PASS || mode == EXEC_FINISH);
}

/***********************************************************************
 *           DEBUG_SuspendExecution
 *
 * Remove all breakpoints before entering the debug loop
 */
void	DEBUG_SuspendExecution( void )
{
   DEBUG_SetBreakpoints( FALSE );
   breakpoints[0] = DEBUG_CurrThread->stepOverBP;
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
    enum exec_mode ret_mode;
    DWORD instr;
    unsigned char ch;

    DEBUG_GetCurrentAddress( &addr );

    /*
     * This is the mode we will be running in after we finish.  We would like
     * to be able to modify this in certain cases.
     */
    ret_mode = mode;

    bp = DEBUG_FindBreakpoint( &addr, DBG_BREAK ); 
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
	    DEBUG_Printf(DBG_CHN_MESG, "Not stopped at any breakpoint; argument ignored.\n");
	  }
      }
    
    if( mode == EXEC_FINISH && DEBUG_IsFctReturn() )
      {
	mode = ret_mode = EXEC_STEPI_INSTR;
      }

    instr = DEBUG_ToLinear( &addr );
    DEBUG_READ_MEM((void*)instr, &ch, sizeof(ch));
    /*
     * See if the function we are stepping into has debug info
     * and line numbers.  If not, then we step over it instead.
     * FIXME - we need to check for things like thunks or trampolines,
     * as the actual function may in fact have debug info.
     */
    if( ch == 0xe8 )
      {
	DEBUG_READ_MEM((void*)(instr + 1), &delta, sizeof(delta));
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
	    DEBUG_Printf(DBG_CHN_MESG, "Not stepping into trampoline at %x (no lines)\n",
		    addr2.off);
#endif
	    mode = EXEC_STEP_OVER_TRAMPOLINE;
	  }
	
	if( mode == EXEC_STEP_INSTR && status == FUNC_HAS_NO_LINES )
	  {
#if 0
	    DEBUG_Printf(DBG_CHN_MESG, "Not stepping into function at %x (no lines)\n",
		    addr2.off);
#endif
	    mode = EXEC_STEP_OVER;
	  }
      }


    if( mode == EXEC_STEP_INSTR )
      {
	if( DEBUG_CheckLinenoStatus(&addr) == FUNC_HAS_NO_LINES )
	  {
	    DEBUG_Printf(DBG_CHN_MESG, "Single stepping until exit from function, \n");
	    DEBUG_Printf(DBG_CHN_MESG, "which has no line number information.\n");
	    
	    ret_mode = mode = EXEC_FINISH;
	  }
      }

    switch(mode)
    {
    case EXEC_CONT: /* Continuous execution */
    case EXEC_PASS: /* Continue, passing exception */
#ifdef __i386__
        DEBUG_context.EFlags &= ~STEP_FLAG;
#endif
        DEBUG_SetBreakpoints( TRUE );
        break;

    case EXEC_STEP_OVER_TRAMPOLINE:
      /*
       * This is the means by which we step over our conversion stubs
       * in callfrom*.s and callto*.s.  We dig the appropriate address
       * off the stack, and we set the breakpoint there instead of the
       * address just after the call.
       */
#ifdef __i386__
      DEBUG_READ_MEM((void*)(DEBUG_context.Esp + 
			     2 * sizeof(unsigned int)), 
		     &addr.off, sizeof(addr.off));
      DEBUG_context.EFlags &= ~STEP_FLAG;
#endif
      breakpoints[0].addr    = addr;
      breakpoints[0].enabled = TRUE;
      breakpoints[0].refcount = 1;
      breakpoints[0].skipcount = 0;
      DEBUG_READ_MEM((void*)DEBUG_ToLinear( &addr ), &breakpoints[0].u.b.opcode, 
		     sizeof(char));
      DEBUG_SetBreakpoints( TRUE );
      break;

    case EXEC_FINISH:
    case EXEC_STEPI_OVER:  /* Stepping over a call */
    case EXEC_STEP_OVER:  /* Stepping over a call */
        if (DEBUG_IsStepOverInstr())
        {
#ifdef __i386__
            DEBUG_context.EFlags &= ~STEP_FLAG;
#endif
            DEBUG_Disasm(&addr, FALSE);
            breakpoints[0].addr    = addr;
            breakpoints[0].enabled = TRUE;
            breakpoints[0].refcount = 1;
	    breakpoints[0].skipcount = 0;
	    DEBUG_READ_MEM((void*)DEBUG_ToLinear( &addr ), &breakpoints[0].u.b.opcode,
			   sizeof(char));
            DEBUG_SetBreakpoints( TRUE );
            break;
        }
        /* else fall through to single-stepping */

    case EXEC_STEP_INSTR: /* Single-stepping an instruction */
    case EXEC_STEPI_INSTR: /* Single-stepping an instruction */
#ifdef __i386__
        DEBUG_context.EFlags |= STEP_FLAG;
#endif
        break;
    case EXEC_KILL:
        break;
    default:
        RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
    }
    DEBUG_CurrThread->stepOverBP = breakpoints[0];
    return ret_mode;
}

int
DEBUG_AddBPCondition(int num, struct expr * exp)
{
    if ((num <= 0) || (num >= next_bp) || !breakpoints[num].refcount)
    {
        DEBUG_Printf( DBG_CHN_MESG, "Invalid breakpoint number %d\n", num );
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
