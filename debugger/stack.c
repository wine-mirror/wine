/*
 * Debugger stack handling
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Eric Youngdale
 * Copyright 1999 Ove Kåven
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "debugger.h"
#include "stackframe.h"


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



/***********************************************************************
 *           DEBUG_InfoStack
 *
 * Dump the top of the stack
 */
void DEBUG_InfoStack(void)
{
    DBG_ADDR addr;

    addr.type = NULL;
    addr.seg = SS_reg(&DEBUG_context);
    addr.off = ESP_reg(&DEBUG_context);

    fprintf(stderr,"Stack dump:\n");
    if (IS_SELECTOR_32BIT(addr.seg))
    {  /* 32-bit mode */
        DEBUG_ExamineMemory( &addr, 24, 'x' );
    }
    else  /* 16-bit mode */
    {
        addr.off &= 0xffff;
        DEBUG_ExamineMemory( &addr, 24, 'w' );
    }
    fprintf(stderr,"\n");
}


static void DEBUG_ForceFrame(DBG_ADDR *stack, DBG_ADDR *code, int frameno, int bits, int noisy)
{
    int theframe = nframe++;
    frames = (struct bt_info *)DBG_realloc(frames,
                                        nframe*sizeof(struct bt_info));
    if (noisy)
      fprintf(stderr,"%s%d ", (theframe == curr_frame ? "=>" : "  "),
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
      fprintf( stderr, (bits == 16) ? " (bp=%04lx)\n" : " (ebp=%08lx)\n", stack->off );
    }
}

static BOOL DEBUG_Frame16(DBG_ADDR *addr, unsigned int *cs, int frameno, int noisy)
{
    unsigned int ss = addr->seg, possible_cs = 0;
    FRAME16 *frame = (FRAME16 *)DBG_ADDR_TO_LIN(addr);
    int theframe = nframe;

    if (!DBG_CHECK_READ_PTR( addr, sizeof(FRAME16) )) return FALSE;
    if (!frame->bp) return FALSE;
    nframe++;
    frames = (struct bt_info *)DBG_realloc(frames,
                                        nframe*sizeof(struct bt_info));
    if (noisy)
      fprintf(stderr,"%s%d ", (theframe == curr_frame ? "=>" : "  "),
              frameno);
    if (frame->bp & 1) *cs = frame->cs;
    else {
      /* not explicitly marked as far call,
       * but check whether it could be anyway */
      if (((frame->cs&7)==7) && (frame->cs != *cs) && !IS_SELECTOR_FREE(frame->cs)) {
        ldt_entry tcs;
        LDT_GetEntry( SELECTOR_TO_ENTRY(frame->cs), &tcs );
        if ( tcs.type == SEGMENT_CODE ) {
          /* it is very uncommon to push a code segment cs as
           * a parameter, so this should work in most cases */
          *cs = possible_cs = frame->cs;
        }
      }
    }
    frames[theframe].cs = addr->seg = *cs;
    frames[theframe].eip = addr->off = frame->ip;
    if (noisy)
      frames[theframe].frame = DEBUG_PrintAddressAndArgs( addr, 16, 
						frame->bp, TRUE );
    else
      DEBUG_FindNearestSymbol( addr, TRUE, 
			       &frames[theframe].frame.sym, frame->bp, 
			       &frames[theframe].frame.list);
    frames[theframe].ss = addr->seg = ss;
    frames[theframe].ebp = addr->off = frame->bp & ~1;
    if (noisy) {
      fprintf( stderr, " (bp=%04lx", addr->off );
      if (possible_cs) {
        fprintf( stderr, ", far call assumed" );
      }
      fprintf( stderr, ")\n" );
    }
    return TRUE;
}

static BOOL DEBUG_Frame32(DBG_ADDR *addr, unsigned int *cs, int frameno, int noisy)
{
    unsigned int ss = addr->seg;
    FRAME32 *frame = (FRAME32 *)DBG_ADDR_TO_LIN(addr);
    int theframe = nframe;

    if (!DBG_CHECK_READ_PTR( addr, sizeof(FRAME32) )) return FALSE;
    if (!frame->ip) return FALSE;
    nframe++;
    frames = (struct bt_info *)DBG_realloc(frames,
                                        nframe*sizeof(struct bt_info));
    if (noisy)
      fprintf(stderr,"%s%d ", (theframe == curr_frame ? "=>" : "  "),
              frameno);
    frames[theframe].cs = addr->seg = *cs;
    frames[theframe].eip = addr->off = frame->ip;
    if (noisy)
      frames[theframe].frame = DEBUG_PrintAddressAndArgs( addr, 32, 
						frame->bp, TRUE );
    else
      DEBUG_FindNearestSymbol( addr, TRUE, 
			       &frames[theframe].frame.sym, frame->bp, 
			       &frames[theframe].frame.list);
    if (noisy) fprintf( stderr, " (ebp=%08lx)\n", frame->bp );
    frames[theframe].ss = addr->seg = ss;
    frames[theframe].ebp = frame->bp;
    if (addr->off == frame->bp) return FALSE;
    addr->off = frame->bp;
    return TRUE;
}

static void DEBUG_DoBackTrace(int noisy)
{
    DBG_ADDR addr, sw_addr;
    unsigned int ss = SS_reg(&DEBUG_context), cs = CS_reg(&DEBUG_context);
    int frameno = 0, is16, ok;
    DWORD next_switch, cur_switch;

    if (noisy) fprintf( stderr, "Backtrace:\n" );

    nframe = 1;
    if (frames) DBG_free( frames );
    frames = (struct bt_info *) DBG_alloc( sizeof(struct bt_info) );
    if (noisy)
      fprintf(stderr,"%s%d ",(curr_frame == 0 ? "=>" : "  "), frameno);

    if (IS_SELECTOR_SYSTEM(ss)) ss = 0;
    if (IS_SELECTOR_SYSTEM(cs)) cs = 0;

    /* first stack frame from registers */
    if (IS_SELECTOR_32BIT(ss))
    {
        frames[0].cs = addr.seg = cs;
        frames[0].eip = addr.off = EIP_reg(&DEBUG_context);
        if (noisy)
          frames[0].frame = DEBUG_PrintAddress( &addr, 32, TRUE );
        else
	  DEBUG_FindNearestSymbol( &addr, TRUE, &frames[0].frame.sym, 0, 
						&frames[0].frame.list);
        frames[0].ss = addr.seg = ss;
	frames[0].ebp = addr.off = EBP_reg(&DEBUG_context);
        if (noisy) fprintf( stderr, " (ebp=%08x)\n", frames[0].ebp );
        is16 = FALSE;
    } else {
        frames[0].cs = addr.seg = cs;
        frames[0].eip = addr.off = IP_reg(&DEBUG_context);
        if (noisy)
          frames[0].frame = DEBUG_PrintAddress( &addr, 16, TRUE );
        else
	  DEBUG_FindNearestSymbol( &addr, TRUE, &frames[0].frame.sym, 0, 
						&frames[0].frame.list);
        frames[0].ss = addr.seg = ss;
	frames[0].ebp = addr.off = BP_reg(&DEBUG_context);
        if (noisy) fprintf( stderr, " (bp=%04x)\n", frames[0].ebp );
        is16 = TRUE;
    }

    next_switch = THREAD_Current()->cur_stack;
    if (is16) {
      if (IsBadReadPtr((STACK32FRAME*)next_switch, sizeof(STACK32FRAME))) {
	 fprintf( stderr, "Bad stack frame %p\n", (STACK32FRAME*)next_switch );
	 return;
      }
      cur_switch = (DWORD)((STACK32FRAME*)next_switch)->frame16;
      sw_addr.seg = SELECTOROF(cur_switch);
      sw_addr.off = OFFSETOF(cur_switch);
    } else {
      if (IsBadReadPtr((STACK16FRAME*)PTR_SEG_TO_LIN(next_switch), sizeof(STACK16FRAME))) {
	 fprintf( stderr, "Bad stack frame %p\n", (STACK16FRAME*)PTR_SEG_TO_LIN(next_switch) );
	 return;
      }
      cur_switch = (DWORD)((STACK16FRAME*)PTR_SEG_TO_LIN(next_switch))->frame32;
      sw_addr.seg = ss;
      sw_addr.off = cur_switch;
    }
    if (DEBUG_IsBadReadPtr(&sw_addr,1)) {
      sw_addr.seg = (DWORD)-1;
      sw_addr.off = (DWORD)-1;
    }

    for (ok = TRUE; ok;) {
      if ((frames[frameno].ss == sw_addr.seg) &&
          (frames[frameno].ebp >= sw_addr.off)) {
        /* 16<->32 switch...
         * yes, I know this is confusing, it gave me a headache too */
        if (is16) {
          STACK32FRAME *frame = (STACK32FRAME*)next_switch;
          DBG_ADDR code;

	  if (IsBadReadPtr((STACK32FRAME*)next_switch, sizeof(STACK32FRAME))) {
	    fprintf( stderr, "Bad stack frame %p\n", (STACK32FRAME*)next_switch );
	    return;
	  }
	  code.type = NULL;
	  code.seg  = 0;
	  code.off  = frame->retaddr;

          cs = 0;
          addr.seg = 0;
          addr.off = frame->ebp;
          DEBUG_ForceFrame( &addr, &code, ++frameno, 32, noisy );

          next_switch = cur_switch;
	  if (IsBadReadPtr((STACK16FRAME*)PTR_SEG_TO_LIN(next_switch), sizeof(STACK16FRAME))) {
	    fprintf( stderr, "Bad stack frame %p\n", (STACK16FRAME*)PTR_SEG_TO_LIN(next_switch) );
	    return;
	  }
          cur_switch = (DWORD)((STACK16FRAME*)PTR_SEG_TO_LIN(next_switch))->frame32;
          sw_addr.seg = 0;
          sw_addr.off = cur_switch;

          is16 = FALSE;
        } else {
          STACK16FRAME *frame = (STACK16FRAME*)PTR_SEG_TO_LIN(next_switch);
          DBG_ADDR code;

	  if (IsBadReadPtr((STACK16FRAME*)PTR_SEG_TO_LIN(next_switch), sizeof(STACK16FRAME))) {
	    fprintf( stderr, "Bad stack frame %p\n", (STACK16FRAME*)PTR_SEG_TO_LIN(next_switch) );
	    return;
	  }

	  code.type = NULL;
	  code.seg  = frame->cs;
	  code.off  = frame->ip;

          cs = frame->cs;
          addr.seg = SELECTOROF(next_switch);
          addr.off = frame->bp;
          DEBUG_ForceFrame( &addr, &code, ++frameno, 16, noisy );

          next_switch = cur_switch;
	  if (IsBadReadPtr((STACK32FRAME*)next_switch, sizeof(STACK32FRAME))) {
	    fprintf( stderr, "Bad stack frame %p\n", (STACK32FRAME*)next_switch );
	    return;
	  }
          cur_switch = (DWORD)((STACK32FRAME*)next_switch)->frame16;
          sw_addr.seg = SELECTOROF(cur_switch);
          sw_addr.off = OFFSETOF(cur_switch);

          is16 = TRUE;
        }
        if (DEBUG_IsBadReadPtr(&sw_addr,1)) {
          sw_addr.seg = (DWORD)-1;
          sw_addr.off = (DWORD)-1;
        }
      } else {
        /* ordinary stack frame */
        ok = is16 ? DEBUG_Frame16( &addr, &cs, ++frameno, noisy)
                  : DEBUG_Frame32( &addr, &cs, ++frameno, noisy);
      }
    }
    if (noisy) fprintf( stderr, "\n" );
}

/***********************************************************************
 *           DEBUG_BackTrace
 *
 * Display a stack back-trace.
 */
void DEBUG_BackTrace(void)
{
    DEBUG_DoBackTrace( TRUE );
}

/***********************************************************************
 *           DEBUG_SilentBackTrace
 *
 * Display a stack back-trace.
 */
void DEBUG_SilentBackTrace(void)
{
    DEBUG_DoBackTrace( FALSE );
}

int
DEBUG_SetFrame(int newframe)
{
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
}

int
DEBUG_GetCurrentFrame(struct name_hash ** name, unsigned int * eip,
		      unsigned int * ebp)
{
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
}

