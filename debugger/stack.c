/*
 * Debugger stack handling
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Eric Youngdale
 */

#include <stdio.h>
#include <stdlib.h>
#include "xmalloc.h"
#include "debugger.h"


/*
 * We keep this info for each frame, so that we can
 * find local variable information correctly.
 */
struct bt_info
{
  unsigned int	     eip;
  unsigned int	     ess;
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
    DBG_ADDR addr = { NULL, SS_reg(&DEBUG_context), ESP_reg(&DEBUG_context) };

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


/***********************************************************************
 *           DEBUG_BackTrace
 *
 * Display a stack back-trace.
 */
void DEBUG_BackTrace(void)
{
    DBG_ADDR addr;
    int frameno = 0;

    fprintf(stderr,"Backtrace:\n");
    if (IS_SELECTOR_SYSTEM(SS_reg(&DEBUG_context)))  /* system stack */
    {
        nframe = 1;
        if (frames) free( frames );
	frames = (struct bt_info *) xmalloc( sizeof(struct bt_info) );
        fprintf(stderr,"%s%d ",(curr_frame == 0 ? "=>" : "  "), frameno++);

        addr.seg = 0;
        addr.off = EIP_reg(&DEBUG_context);
	frames[0].eip = addr.off;
        frames[0].frame = DEBUG_PrintAddress( &addr, 32, TRUE );
        fprintf( stderr, "\n" );
	frames[0].ebp = addr.off = EBP_reg(&DEBUG_context);

        while (addr.off)
        {
            FRAME32 *frame = (FRAME32 *)addr.off;
            if (!DBG_CHECK_READ_PTR( &addr, sizeof(FRAME32) )) return;
            if (!frame->ip) break;
            nframe++;
            frames = (struct bt_info *)xrealloc(frames,
                                                nframe*sizeof(struct bt_info));
            fprintf(stderr,"%s%d ", (frameno == curr_frame ? "=>" : "  "),
		    frameno);
            addr.off = frame->ip;
	    frames[frameno].eip = addr.off;
	    frames[frameno].ebp = frame->bp;
            frames[frameno].frame = DEBUG_PrintAddressAndArgs( &addr, 32, 
							frame->bp, TRUE );
	    frameno++;
            fprintf( stderr, "\n" );
            if (addr.off == frame->bp) break;
            addr.off = frame->bp;
        }
    }
    else  /* 16-bit mode */
    {
      WORD ss = SS_reg(&DEBUG_context), cs = CS_reg(&DEBUG_context);
      if (GET_SEL_FLAGS(ss) & LDT_FLAGS_32BIT)
      {
          fprintf( stderr, "Not implemented: 32-bit backtrace on a different stack segment.\n" );
          return;
      }
      fprintf( stderr,"%d ", frameno++ );
      addr.seg = cs;
      addr.off = IP_reg(&DEBUG_context);
      DEBUG_PrintAddress( &addr, 16, TRUE );
      fprintf( stderr, "\n" );
      addr.seg = ss;
      addr.off = BP_reg(&DEBUG_context) & ~1;
      for (;;)
      {
          FRAME16 *frame = (FRAME16 *)DBG_ADDR_TO_LIN(&addr);
          if (!DBG_CHECK_READ_PTR( &addr, sizeof(FRAME16) )) return;
          if (!frame->bp) break;
          if (frame->bp & 1) cs = frame->cs;
          fprintf( stderr,"%d ", frameno++ );
          addr.seg = cs;
          addr.off = frame->ip;
          DEBUG_PrintAddress( &addr, 16, TRUE );
          fprintf( stderr, "\n" );
          addr.seg = ss;
          addr.off = frame->bp & ~1;
      }
  }
  fprintf( stderr, "\n" );
}

/***********************************************************************
 *           DEBUG_SilentBackTrace
 *
 * Display a stack back-trace.
 */
void DEBUG_SilentBackTrace(void)
{
    DBG_ADDR addr;
    int frameno = 0;

    nframe = 1;
    if (frames) free( frames );
    frames = (struct bt_info *) xmalloc( sizeof(struct bt_info) );
    if (IS_SELECTOR_SYSTEM(SS_reg(&DEBUG_context)))  /* system stack */
    {
        addr.seg = 0;
        addr.off = EIP_reg(&DEBUG_context);
	frames[0].eip = addr.off;
	DEBUG_FindNearestSymbol( &addr, TRUE, &frames[0].frame.sym, 0, 
						&frames[0].frame.list);
	frames[0].ebp = addr.off = EBP_reg(&DEBUG_context);
	frameno++;

        while (addr.off)
        {
            FRAME32 *frame = (FRAME32 *)addr.off;
            if (!DBG_CHECK_READ_PTR( &addr, sizeof(FRAME32) )) return;
            if (!frame->ip) break;
            nframe++;
            frames = (struct bt_info *)xrealloc(frames,
                                                nframe*sizeof(struct bt_info));
            addr.off = frame->ip;
	    frames[frameno].eip = addr.off;
	    frames[frameno].ebp = frame->bp;
	    DEBUG_FindNearestSymbol( &addr, TRUE, 
				     &frames[frameno].frame.sym, frame->bp, 
				     &frames[frameno].frame.list);
	    frameno++;
            addr.off = frame->bp;
        }
    }
    else  /* 16-bit mode */
    {
      /*
       * Not implemented here.  I am not entirely sure how best to handle
       * this stuff.
       */
    }
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

   if( frames[curr_frame].frame.list.sourcefile != NULL )
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

