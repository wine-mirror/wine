/*
 * Debugger stack handling
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Eric Youngdale
 * Copyright 1999 Ove Kåven
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

    DEBUG_Printf("Stack dump:\n");
    switch (DEBUG_GetSelectorType(value.addr.seg))
    {
    case MODE_32: /* 32-bit mode */
       DEBUG_ExamineMemory( &value, 24, 'x' );
       break;
    case MODE_16:  /* 16-bit mode */
    case MODE_VM86:
        value.addr.off &= 0xffff;
        DEBUG_ExamineMemory( &value, 24, 'w' );
	break;
    default:
       DEBUG_Printf("Bad segment (%ld)\n", value.addr.seg);
    }
    DEBUG_Printf("\n");
#endif
}

#ifdef __i386__
static void DEBUG_ForceFrame(DBG_ADDR *stack, DBG_ADDR *code, int frameno, enum dbg_mode mode,
                             int noisy, const char *caveat)
{
    int theframe = nframe++;
    frames = (struct bt_info *)DBG_realloc(frames,
					   nframe*sizeof(struct bt_info));
    if (noisy)
      DEBUG_Printf("%s%d ", (theframe == curr_frame ? "=>" : "  "),
              frameno);
    frames[theframe].cs = code->seg;
    frames[theframe].eip = code->off;
    if (noisy)
        frames[theframe].frame = DEBUG_PrintAddressAndArgs( code, mode, stack->off, TRUE );
    else
      DEBUG_FindNearestSymbol( code, TRUE,
			       &frames[theframe].frame.sym, stack->off,
			       &frames[theframe].frame.list);
    frames[theframe].ss = stack->seg;
    frames[theframe].ebp = stack->off;
    if (noisy) {
      DEBUG_Printf((mode != MODE_32) ? " (bp=%04lx%s)\n" : " (ebp=%08lx%s)\n",
                   stack->off, caveat ? caveat : "");
    }
}

static BOOL DEBUG_Frame16(DBG_THREAD* thread, DBG_ADDR *addr, unsigned int *cs, int frameno, int noisy)
{
    unsigned int	possible_cs = 0;
    FRAME16 		frame;
    void*		p = (void*)DEBUG_ToLinear(addr);
    DBG_ADDR		code;

    if (!p) return FALSE;

    if (!DEBUG_READ_MEM(p, &frame, sizeof(frame))) {
        if (noisy) DEBUG_InvalAddr(addr);
	return FALSE;
    }
    if (!frame.bp) return FALSE;

    if (frame.bp & 1) *cs = frame.cs;
    else {
        /* not explicitly marked as far call,
	 * but check whether it could be anyway */
        if (((frame.cs&7)==7) && (frame.cs != *cs)) {
	    LDT_ENTRY	le;

	    if (GetThreadSelectorEntry( thread->handle, frame.cs, &le) &&
		(le.HighWord.Bits.Type & 0x08)) { /* code segment */
	        /* it is very uncommon to push a code segment cs as
		 * a parameter, so this should work in most cases */
	        *cs = possible_cs = frame.cs;
	    }
	}
    }
    code.seg = *cs;
    code.off = frame.ip;
    addr->off = frame.bp & ~1;
    DEBUG_ForceFrame(addr, &code, frameno, MODE_16, noisy,
                     possible_cs ? ", far call assumed" : NULL );
    return TRUE;
}

static BOOL DEBUG_Frame32(DBG_ADDR *addr, unsigned int *cs, int frameno, int noisy)
{
    FRAME32 		frame;
    void*		p = (void*)DEBUG_ToLinear(addr);
    DBG_ADDR		code;
    DWORD		old_bp = addr->off;

    if (!p) return FALSE;

    if (!DEBUG_READ_MEM(p, &frame, sizeof(frame))) {
       if (noisy) DEBUG_InvalAddr(addr);
       return FALSE;
    }
    if (!frame.ip) return FALSE;

    code.seg = *cs;
    code.off = frame.ip;
    addr->off = frame.bp;
    DEBUG_ForceFrame(addr, &code, frameno, MODE_32, noisy, NULL);
    if (addr->off == old_bp) return FALSE;
    return TRUE;
}
#endif


/***********************************************************************
 *           DEBUG_BackTrace
 *
 * Display a stack back-trace.
 */
void DEBUG_BackTrace(DWORD tid, BOOL noisy)
{
#ifdef __i386
    DBG_ADDR 		addr, sw_addr, code, tmp;
    unsigned int 	ss, cs;
    int 		frameno = 0, is16, ok;
    DWORD 		next_switch, cur_switch, p;
    STACK16FRAME       	frame16;
    STACK32FRAME       	frame32;
    char		ch;
    CONTEXT		ctx;
    DBG_THREAD*		thread;

    int 		copy_nframe = 0;
    int			copy_curr_frame = 0;
    struct bt_info* 	copy_frames = NULL;

    if (noisy) DEBUG_Printf("Backtrace:\n");

    if (tid == DEBUG_CurrTid)
    {
	 ctx = DEBUG_context;
	 thread = DEBUG_CurrThread;

	 if (frames) DBG_free( frames );
	 /* frames = (struct bt_info *) DBG_alloc( sizeof(struct bt_info) ); */
    }
    else
    {
	 thread = DEBUG_GetThread(DEBUG_CurrProcess, tid);

	 if (!thread)
	 {
	      DEBUG_Printf("Unknown thread id (0x%08lx) in current process\n", tid);
	      return;
	 }
	 memset(&ctx, 0, sizeof(ctx));
	 ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_SEGMENTS;

	 if ( SuspendThread( thread->handle ) == -1 ||
	      !GetThreadContext( thread->handle, &ctx ))
	 {
	      DEBUG_Printf("Can't get context for thread id (0x%08lx) in current process\n", tid);
	      return;
	 }
	 /* need to avoid trashing stack frame for current thread */
	 copy_nframe = nframe;
	 copy_frames = frames;
	 copy_curr_frame = curr_frame;
	 curr_frame = 0;
    }

    nframe = 0;
    frames = NULL;

    cs = ctx.SegCs;
    ss = ctx.SegSs;

    if (DEBUG_IsSelectorSystem(ss)) ss = 0;
    if (DEBUG_IsSelectorSystem(cs)) cs = 0;

    /* first stack frame from registers */
    switch (DEBUG_GetSelectorType(ss))
    {
    case MODE_32:
        code.seg = cs;
        code.off = ctx.Eip;
        addr.seg = ss;
	addr.off = ctx.Ebp;
        DEBUG_ForceFrame( &addr, &code, frameno, MODE_32, noisy, NULL );
        if (!(code.seg || code.off)) {
            /* trying to execute a null pointer... yuck...
             * if it was a call to null, the return EIP should be
             * available at SS:ESP, so let's try to retrieve it */
            tmp.seg = ss;
            tmp.off = ctx.Esp;
            if (DEBUG_READ_MEM((void *)DEBUG_ToLinear(&tmp), &code.off, sizeof(code.off))) {
                DEBUG_ForceFrame( &addr, &code, ++frameno, MODE_32, noisy, ", null call assumed" );
            }
        }
        is16 = FALSE;
	break;
    case MODE_16:
    case MODE_VM86:
        code.seg = cs;
        code.off = LOWORD(ctx.Eip);
        addr.seg = ss;
	addr.off = LOWORD(ctx.Ebp);
        DEBUG_ForceFrame( &addr, &code, frameno, MODE_16, noisy, NULL );
        is16 = TRUE;
	break;
    default:
        if (noisy) DEBUG_Printf("Bad segment '%x'\n", ss);
	return;
    }

    /* cur_switch holds address of curr_stack's field in TEB in debuggee
     * address space
     */
    cur_switch = (DWORD)thread->teb + OFFSET_OF(TEB, cur_stack);
    if (!DEBUG_READ_MEM((void*)cur_switch, &next_switch, sizeof(next_switch))) {
        if (noisy) DEBUG_Printf("Can't read TEB:cur_stack\n");
	return;
    }

    if (is16) {
        if (!DEBUG_READ_MEM((void*)next_switch, &frame32, sizeof(STACK32FRAME))) {
	    if (noisy) DEBUG_Printf("Bad stack frame 0x%08lx\n",
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
	    if (noisy) DEBUG_Printf("Bad stack frame 0x%08lx\n",
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
            sw_addr.off && (frames[frameno].ebp >= sw_addr.off))
        {
	   /* 16<->32 switch...
	    * yes, I know this is confusing, it gave me a headache too */
	   if (is16) {

	       if (!DEBUG_READ_MEM((void*)next_switch, &frame32, sizeof(STACK32FRAME))) {
		  if (noisy) DEBUG_Printf("Bad stack frame 0x%08lx\n",
                                          (unsigned long)(STACK32FRAME*)next_switch );
		  return;
	       }

	       code.seg  = 0;
	       code.off  = frame32.retaddr;

	       cs = 0;
	       addr.seg = 0;
	       addr.off = frame32.ebp;
	       DEBUG_ForceFrame( &addr, &code, ++frameno, MODE_32, noisy, NULL );

	       next_switch = cur_switch;
	       tmp.seg = SELECTOROF(next_switch);
	       tmp.off = OFFSETOF(next_switch);
	       p = DEBUG_ToLinear(&tmp);

	       if (!DEBUG_READ_MEM((void*)p, &frame16, sizeof(STACK16FRAME))) {
		   if (noisy) DEBUG_Printf("Bad stack frame 0x%08lx\n",
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
		  if (noisy) DEBUG_Printf("Bad stack frame 0x%08lx\n",
                                          (unsigned long)(STACK16FRAME*)p );
		  return;
	      }

	      code.seg  = frame16.cs;
	      code.off  = frame16.ip;

	      cs = frame16.cs;
	      addr.seg = SELECTOROF(next_switch);
	      addr.off = frame16.bp;
	      DEBUG_ForceFrame( &addr, &code, ++frameno, MODE_16, noisy, NULL );

	      next_switch = cur_switch;
	      if (!DEBUG_READ_MEM((void*)next_switch, &frame32, sizeof(STACK32FRAME))) {
		 if (noisy) DEBUG_Printf("Bad stack frame 0x%08lx\n",
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
	   ok = is16 ? DEBUG_Frame16( thread, &addr, &cs, ++frameno, noisy)
	      : DEBUG_Frame32( &addr, &cs, ++frameno, noisy);
	}
    }
    if (noisy) DEBUG_Printf("\n");

    if (tid != DEBUG_CurrTid)
    {
	 ResumeThread( thread->handle );
	 /* restore stack frame for current thread */
	 if (frames) DBG_free( frames );
	 frames = copy_frames;
	 nframe = copy_nframe;
	 curr_frame = copy_curr_frame;
    }
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
