/*
 * Debugger stack handling
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Eric Youngdale
 * Copyright 1999 Ove Kåven
 */

#include "config.h"

#include <stdlib.h>

#include "debugger.h"
#include "stackframe.h"
#include "winbase.h"

#ifdef __i386__
/*
 * We keep this info for each frame, so that we can
 * find local variable information correctly.
 */
struct bt_info
{
  unsigned int	     cs;
  unsigned int	     eip;
  unsigned int	     ss;
  unsigned int	     ebp;
  struct symbol_info frame;
};

static int nframe;
static struct bt_info * frames = NULL;
int curr_frame;

typedef struct
{
    WORD bp;
    WORD ip;
    WORD cs;
} FRAME16;

typedef struct
{
    DWORD bp;
    DWORD ip;
    WORD cs;
} FRAME32;
#endif


/***********************************************************************
 *           DEBUG_InfoStack
 *
 * Dump the top of the stack
 */
void DEBUG_InfoStack(void)
{
#ifdef __i386__
    DBG_VALUE	value;
    
    value.type = NULL;
    value.cookie = DV_TARGET;
    value.addr.seg = DEBUG_context.SegSs;
    value.addr.off = DEBUG_context.Esp;

    DEBUG_Printf(DBG_CHN_MESG,"Stack dump:\n");
    switch (DEBUG_GetSelectorType(value.addr.seg)) {
    case 32: /* 32-bit mode */
       DEBUG_ExamineMemory( &value, 24, 'x' );
       break;
    case 16:  /* 16-bit mode */
        value.addr.off &= 0xffff;
        DEBUG_ExamineMemory( &value, 24, 'w' );
	break;
    default:
       DEBUG_Printf(DBG_CHN_MESG, "Bad segment (%ld)\n", value.addr.seg);
    }
    DEBUG_Printf(DBG_CHN_MESG,"\n");
#endif
}

#ifdef __i386__
static void DEBUG_ForceFrame(DBG_ADDR *stack, DBG_ADDR *code, int frameno, int bits, int noisy)
{
    int theframe = nframe++;
    frames = (struct bt_info *)DBG_realloc(frames,
					   nframe*sizeof(struct bt_info));
    if (noisy)
      DEBUG_Printf(DBG_CHN_MESG,"%s%d ", (theframe == curr_frame ? "=>" : "  "),
              frameno);
    frames[theframe].cs = code->seg;
    frames[theframe].eip = code->off;
    if (noisy)
      frames[theframe].frame = DEBUG_PrintAddressAndArgs( code, bits, 
							  stack->off, TRUE );
    else
      DEBUG_FindNearestSymbol( code, TRUE, 
			       &frames[theframe].frame.sym, stack->off, 
			       &frames[theframe].frame.list);
    frames[theframe].ss = stack->seg;
    frames[theframe].ebp = stack->off;
    if (noisy) {
      DEBUG_Printf( DBG_CHN_MESG, (bits == 16) ? " (bp=%04lx)\n" : " (ebp=%08lx)\n", stack->off );
    }
}

static BOOL DEBUG_Frame16(DBG_ADDR *addr, unsigned int *cs, int frameno, int noisy)
{
    unsigned int	ss = addr->seg, possible_cs = 0;
    FRAME16 		frame;
    int 		theframe = nframe;
    void*		p = (void*)DEBUG_ToLinear(addr);
    
    if (!p) return FALSE;
    
    if (!DEBUG_READ_MEM(p, &frame, sizeof(frame))) {
        if (noisy) DEBUG_InvalAddr(addr);
	return FALSE;
    }
    if (!frame.bp) return FALSE;
    nframe++;
    frames = (struct bt_info *)DBG_realloc(frames,
					   nframe*sizeof(struct bt_info));
    if (noisy)
        DEBUG_Printf(DBG_CHN_MESG,"%s%d ", (theframe == curr_frame ? "=>" : "  "),
		     frameno);
    if (frame.bp & 1) *cs = frame.cs;
    else {
        /* not explicitly marked as far call,
	 * but check whether it could be anyway */
        if (((frame.cs&7)==7) && (frame.cs != *cs)) {
	    LDT_ENTRY	le;
	 
	    if (GetThreadSelectorEntry( DEBUG_CurrThread->handle, frame.cs, &le) &&
		(le.HighWord.Bits.Type & 0x08)) { /* code segment */
	        /* it is very uncommon to push a code segment cs as
		 * a parameter, so this should work in most cases */
	        *cs = possible_cs = frame.cs;
	    }
	}
    }
    frames[theframe].cs = addr->seg = *cs;
    frames[theframe].eip = addr->off = frame.ip;
    if (noisy)
        frames[theframe].frame = DEBUG_PrintAddressAndArgs( addr, 16, 
							    frame.bp, TRUE );
    else
        DEBUG_FindNearestSymbol( addr, TRUE, 
				 &frames[theframe].frame.sym, frame.bp, 
				 &frames[theframe].frame.list);
    frames[theframe].ss = addr->seg = ss;
    frames[theframe].ebp = addr->off = frame.bp & ~1;
    if (noisy) {
        DEBUG_Printf( DBG_CHN_MESG, " (bp=%04lx", addr->off );
        if (possible_cs) {
	    DEBUG_Printf( DBG_CHN_MESG, ", far call assumed" );
	}
	DEBUG_Printf( DBG_CHN_MESG, ")\n" );
    }
    return TRUE;
}

static BOOL DEBUG_Frame32(DBG_ADDR *addr, unsigned int *cs, int frameno, int noisy)
{
    unsigned int 	ss = addr->seg;
    FRAME32 		frame;
    int 		theframe = nframe;
    void*		p = (void*)DEBUG_ToLinear(addr);
    
    if (!p) return FALSE;
    
    if (!DEBUG_READ_MEM(p, &frame, sizeof(frame))) {
       if (noisy) DEBUG_InvalAddr(addr);
       return FALSE;
    }
    if (!frame.ip) return FALSE;
    
    nframe++;
    frames = (struct bt_info *)DBG_realloc(frames,
					   nframe*sizeof(struct bt_info));
    if (noisy)
       DEBUG_Printf(DBG_CHN_MESG,"%s%d ", (theframe == curr_frame ? "=>" : "  "),
		    frameno);
    frames[theframe].cs = addr->seg = *cs;
    frames[theframe].eip = addr->off = frame.ip;
    if (noisy)
        frames[theframe].frame = DEBUG_PrintAddressAndArgs( addr, 32, 
							    frame.bp, TRUE );
    else
        DEBUG_FindNearestSymbol( addr, TRUE, 
				 &frames[theframe].frame.sym, frame.bp, 
				 &frames[theframe].frame.list);
    if (noisy) DEBUG_Printf( DBG_CHN_MESG, " (ebp=%08lx)\n", frame.bp );
    frames[theframe].ss = addr->seg = ss;
    frames[theframe].ebp = frame.bp;
    if (addr->off == frame.bp) return FALSE;
    addr->off = frame.bp;
    return TRUE;
}
#endif


/***********************************************************************
 *           DEBUG_BackTrace
 *
 * Display a stack back-trace.
 */
void DEBUG_BackTrace(BOOL noisy)
{
#ifdef __i386
    DBG_ADDR 		addr, sw_addr, code, tmp;
    unsigned int 	ss = DEBUG_context.SegSs;
    unsigned int	cs = DEBUG_context.SegCs;
    int 		frameno = 0, is16, ok;
    DWORD 		next_switch, cur_switch, p;
    STACK16FRAME       	frame16;
    STACK32FRAME       	frame32;
    char		ch;

    if (noisy) DEBUG_Printf( DBG_CHN_MESG, "Backtrace:\n" );

    nframe = 1;
    if (frames) DBG_free( frames );
    frames = (struct bt_info *) DBG_alloc( sizeof(struct bt_info) );
    if (noisy)
        DEBUG_Printf(DBG_CHN_MESG,"%s%d ",(curr_frame == 0 ? "=>" : "  "), frameno);

    if (DEBUG_IsSelectorSystem(ss)) ss = 0;
    if (DEBUG_IsSelectorSystem(cs)) cs = 0;

    /* first stack frame from registers */
    switch (DEBUG_GetSelectorType(ss))
    {
    case 32:
        frames[0].cs = addr.seg = cs;
        frames[0].eip = addr.off = DEBUG_context.Eip;
        if (noisy)
            frames[0].frame = DEBUG_PrintAddress( &addr, 32, TRUE );
        else
	    DEBUG_FindNearestSymbol( &addr, TRUE, &frames[0].frame.sym, 0, 
				     &frames[0].frame.list);
        frames[0].ss = addr.seg = ss;
	frames[0].ebp = addr.off = DEBUG_context.Ebp;
        if (noisy) DEBUG_Printf( DBG_CHN_MESG, " (ebp=%08x)\n", frames[0].ebp );
        is16 = FALSE;
	break;
    case 16:
        frames[0].cs = addr.seg = cs;
        frames[0].eip = addr.off = LOWORD(DEBUG_context.Eip);
        if (noisy)
	    frames[0].frame = DEBUG_PrintAddress( &addr, 16, TRUE );
        else
	    DEBUG_FindNearestSymbol( &addr, TRUE, &frames[0].frame.sym, 0, 
				     &frames[0].frame.list);
        frames[0].ss = addr.seg = ss;
	frames[0].ebp = addr.off = LOWORD(DEBUG_context.Ebp);
        if (noisy) DEBUG_Printf( DBG_CHN_MESG, " (bp=%04x)\n", frames[0].ebp );
        is16 = TRUE;
	break;
    default:
        if (noisy) DEBUG_Printf( DBG_CHN_MESG, "Bad segment '%u'\n", ss);
	return;
    }

    /* cur_switch holds address of curr_stack's field in TEB in debuggee 
     * address space 
     */
    cur_switch = (DWORD)DEBUG_CurrThread->teb + OFFSET_OF(TEB, cur_stack);
    if (!DEBUG_READ_MEM((void*)cur_switch, &next_switch, sizeof(next_switch))) {
        if (noisy) DEBUG_Printf( DBG_CHN_MESG, "Can't read TEB:cur_stack\n");
	return;
    }

    if (is16) {
        if (!DEBUG_READ_MEM((void*)next_switch, &frame32, sizeof(STACK32FRAME))) {
	    if (noisy) DEBUG_Printf( DBG_CHN_MESG, "Bad stack frame 0x%08lx\n", 
				     (unsigned long)(STACK32FRAME*)next_switch );
	    return;
	}
	cur_switch = (DWORD)frame32.frame16;
	sw_addr.seg = SELECTOROF(cur_switch);
	sw_addr.off = OFFSETOF(cur_switch);
    } else {
        tmp.seg = SELECTOROF(next_switch);
	tmp.off = OFFSETOF(next_switch);
	p = DEBUG_ToLinear(&tmp);
	
	if (!DEBUG_READ_MEM((void*)p, &frame16, sizeof(STACK16FRAME))) {
	    if (noisy) DEBUG_Printf( DBG_CHN_MESG, "Bad stack frame 0x%08lx\n", 
				     (unsigned long)(STACK16FRAME*)p );
	    return;
	}
	cur_switch = (DWORD)frame16.frame32;
	sw_addr.seg = ss;
	sw_addr.off = cur_switch;
    }
    if (!DEBUG_READ_MEM((void*)DEBUG_ToLinear(&sw_addr), &ch, sizeof(ch))) {
        sw_addr.seg = (DWORD)-1;
	sw_addr.off = (DWORD)-1;
    }
    
    for (ok = TRUE; ok;) {
        if ((frames[frameno].ss == sw_addr.seg) &&
	    (frames[frameno].ebp >= sw_addr.off)) {
	   /* 16<->32 switch...
	    * yes, I know this is confusing, it gave me a headache too */
	   if (is16) {
	      
	       if (!DEBUG_READ_MEM((void*)next_switch, &frame32, sizeof(STACK32FRAME))) {
		  if (noisy) DEBUG_Printf( DBG_CHN_MESG, "Bad stack frame 0x%08lx\n", 
					   (unsigned long)(STACK32FRAME*)next_switch );
		  return;
	       }

	       code.seg  = 0;
	       code.off  = frame32.retaddr;
	       
	       cs = 0;
	       addr.seg = 0;
	       addr.off = frame32.ebp;
	       DEBUG_ForceFrame( &addr, &code, ++frameno, 32, noisy );
	       
	       next_switch = cur_switch;
	       tmp.seg = SELECTOROF(next_switch);
	       tmp.off = OFFSETOF(next_switch);
	       p = DEBUG_ToLinear(&tmp);
	       
	       if (!DEBUG_READ_MEM((void*)p, &frame16, sizeof(STACK16FRAME))) {
		   if (noisy) DEBUG_Printf( DBG_CHN_MESG, "Bad stack frame 0x%08lx\n", 
					    (unsigned long)(STACK16FRAME*)p );
		   return;
	       }
	       cur_switch = (DWORD)frame16.frame32;
	       sw_addr.seg = 0;
	       sw_addr.off = cur_switch;
	       
	       is16 = FALSE;
	   } else {
	      tmp.seg = SELECTOROF(next_switch);
	      tmp.off = OFFSETOF(next_switch);
	      p = DEBUG_ToLinear(&tmp);
	      
	      if (!DEBUG_READ_MEM((void*)p, &frame16, sizeof(STACK16FRAME))) {
		  if (noisy) DEBUG_Printf( DBG_CHN_MESG, "Bad stack frame 0x%08lx\n",
					   (unsigned long)(STACK16FRAME*)p );
		  return;
	      }
	      
	      code.seg  = frame16.cs;
	      code.off  = frame16.ip;
	      
	      cs = frame16.cs;
	      addr.seg = SELECTOROF(next_switch);
	      addr.off = frame16.bp;
	      DEBUG_ForceFrame( &addr, &code, ++frameno, 16, noisy );
	      
	      next_switch = cur_switch;
	      if (!DEBUG_READ_MEM((void*)next_switch, &frame32, sizeof(STACK32FRAME))) {
		 if (noisy) DEBUG_Printf( DBG_CHN_MESG, "Bad stack frame 0x%08lx\n", 
					  (unsigned long)(STACK32FRAME*)next_switch );
		 return;
	      }
	      cur_switch = (DWORD)frame32.frame16;
	      sw_addr.seg = SELECTOROF(cur_switch);
	      sw_addr.off = OFFSETOF(cur_switch);
	      
	      is16 = TRUE;
	   }
	   if (!DEBUG_READ_MEM((void*)DEBUG_ToLinear(&sw_addr), &ch, sizeof(ch))) {
	      sw_addr.seg = (DWORD)-1;
	      sw_addr.off = (DWORD)-1;
	   }
	} else {
	    /* ordinary stack frame */
	   ok = is16 ? DEBUG_Frame16( &addr, &cs, ++frameno, noisy)
	      : DEBUG_Frame32( &addr, &cs, ++frameno, noisy);
	}
    }
    if (noisy) DEBUG_Printf( DBG_CHN_MESG, "\n" );
#endif
}

int
DEBUG_SetFrame(int newframe)
{
#ifdef __i386__
  int		rtn = FALSE;

  curr_frame = newframe;

  if( curr_frame >= nframe )
    {
      curr_frame = nframe - 1;
    }

  if( curr_frame < 0 )
    {
      curr_frame = 0;
    }

   if( frames && frames[curr_frame].frame.list.sourcefile != NULL )
    {
      DEBUG_List(&frames[curr_frame].frame.list, NULL, 0);
    }

  rtn = TRUE;
  return (rtn);
#else /* __i386__ */
  return FALSE;
#endif /* __i386__ */
}

int
DEBUG_GetCurrentFrame(struct name_hash ** name, unsigned int * eip,
		      unsigned int * ebp)
{
#ifdef __i386__
  /*
   * If we don't have a valid backtrace, then just return.
   */
  if( frames == NULL )
    {
      return FALSE;
    }

  /*
   * If we don't know what the current function is, then we also have
   * nothing to report here.
   */
  if( frames[curr_frame].frame.sym == NULL )
    {
      return FALSE;
    }

  *name = frames[curr_frame].frame.sym;
  *eip = frames[curr_frame].eip;
  *ebp = frames[curr_frame].ebp;

  return TRUE;
#else /* __i386__ */
  return FALSE;
#endif /* __i386__ */
}

