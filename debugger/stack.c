/*
 * Debugger stack handling
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Eric Youngdale
 */

#include <stdio.h>
#include <malloc.h>
#include "xmalloc.h"
#include "windows.h"
#include "debugger.h"


/*
 * We keep this info for each frame, so that we can
 * find local variable information correctly.
 */
struct bt_info
{
  unsigned int	     eip;
  unsigned int	     ebp;
  struct name_hash * frame;
};

static int nframe;
static struct bt_info * frames = NULL;
int curr_frame;
static char * reg_name[] =
{
  "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
};

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

    fprintf(stderr,"Stack dump:\n");
    if ((SS_reg(DEBUG_context) == WINE_DATA_SELECTOR) ||
        (GET_SEL_FLAGS(SS_reg(DEBUG_context)) & LDT_FLAGS_32BIT))
    {  /* 32-bit mode */
        addr.seg = 0;
        addr.off = ESP_reg(DEBUG_context);
        DEBUG_ExamineMemory( &addr, 24, 'x' );
    }
    else  /* 16-bit mode */
    {
        addr.seg = SS_reg(DEBUG_context);
        addr.off = SP_reg(DEBUG_context);
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
    if (SS_reg(DEBUG_context) == WINE_DATA_SELECTOR)  /* 32-bit mode */
    {
        addr.seg = 0;
        addr.off = EBP_reg(DEBUG_context);
	nframe = 1;
        while (addr.off)
        {
            FRAME32 *frame = (FRAME32 *)addr.off;
            if (!DBG_CHECK_READ_PTR( &addr, sizeof(FRAME32) )) return;
            if (!frame->ip) break;
            addr.off = frame->bp;
	    nframe++;
        }
	if( frames != NULL )
	  {
	    free(frames);
	  }

	frames = (struct bt_info *) xmalloc(nframe 
					      * sizeof(struct bt_info) );
        fprintf(stderr,"%s%d ",(curr_frame == 0 ? "=>" : "  "), frameno++);
        addr.off = EIP_reg(DEBUG_context);
	frames[0].eip = addr.off;
        frames[0].frame = DEBUG_PrintAddress( &addr, 32, TRUE );
        fprintf( stderr, "\n" );
        addr.off = EBP_reg(DEBUG_context);

	frames[0].ebp = addr.off;

        while (addr.off)
        {
            FRAME32 *frame = (FRAME32 *)addr.off;
            if (!DBG_CHECK_READ_PTR( &addr, sizeof(FRAME32) )) return;
            if (!frame->ip) break;
            fprintf(stderr,"%s%d ", (frameno == curr_frame ? "=>" : "  "),
		    frameno);
            addr.off = frame->ip;
	    frames[frameno].eip = addr.off;
	    frames[frameno].ebp = frame->bp;
            frames[frameno].frame = DEBUG_PrintAddressAndArgs( &addr, 32, 
							frame->bp, TRUE );
	    frameno++;
            fprintf( stderr, "\n" );
            addr.off = frame->bp;
        }
    }
    else  /* 16-bit mode */
    {
      WORD ss = SS_reg(DEBUG_context), cs = CS_reg(DEBUG_context);
      if (GET_SEL_FLAGS(ss) & LDT_FLAGS_32BIT)
      {
          fprintf( stderr, "Not implemented: 32-bit backtrace on a different stack segment.\n" );
          return;
      }
      fprintf( stderr,"%d ", frameno++ );
      addr.seg = cs;
      addr.off = IP_reg(DEBUG_context);
      DEBUG_PrintAddress( &addr, 16, TRUE );
      fprintf( stderr, "\n" );
      addr.seg = ss;
      addr.off = BP_reg(DEBUG_context) & ~1;
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
 *           DEBUG_GetSymbolValue
 *
 * Get the address of a named symbol from the current stack frame.
 */
BOOL32 DEBUG_GetStackSymbolValue( const char * name, DBG_ADDR *addr )
{
  struct name_hash * curr_func;
  int		     i;

  /*
   * If we don't have a valid backtrace, then just return.
   */
  if( frames == NULL )
    {
      return FALSE;
    }

  curr_func = frames[curr_frame].frame;

  /*
   * If we don't know what the current function is, then we also have
   * nothing to report here.
   */
  if( curr_func == NULL )
    {
      return FALSE;
    }

  for(i=0; i < curr_func->n_locals; i++ )
    {
      /*
       * Test the range of validity of the local variable.  This
       * comes up with RBRAC/LBRAC stabs in particular.
       */
      if(    (curr_func->local_vars[i].pc_start != 0)
	  && ((frames[curr_frame].eip - curr_func->addr.off) 
	      < curr_func->local_vars[i].pc_start) )
	{
	  continue;
	}

      if(    (curr_func->local_vars[i].pc_end != 0)
	  && ((frames[curr_frame].eip - curr_func->addr.off) 
	      > curr_func->local_vars[i].pc_end) )
	{
	  continue;
	}

      if( strcmp(name, curr_func->local_vars[i].name) == 0 )
	{
	  /*
	   * OK, we found it.  Now figure out what to do with this.
	   */
	  if( curr_func->local_vars[i].regno != 0 )
	    {
	      /*
	       * Register variable.  We don't know how to treat
	       * this yet.
	       */
	      return FALSE;
	    }

	  addr->seg = 0;
	  addr->off = frames[curr_frame].ebp + curr_func->local_vars[i].offset;

	  return TRUE;
	}
    }
  return FALSE;
}

int
DEBUG_SetFrame(int newframe)
{
  int		rtn = FALSE;
  /*
   * Nothing for now.  Add support later.
   */

  curr_frame = newframe;
  if( curr_frame < 0 )
    {
      curr_frame = 0;
    }

  if( curr_frame >= nframe )
    {
      curr_frame = nframe - 1;
    }

  rtn = TRUE;
  return (rtn);
}

int
DEBUG_InfoLocals()
{
  struct name_hash * curr_func;
  int		     i;
  int		rtn = FALSE;
  unsigned	int * ptr;

  /*
   * If we don't have a valid backtrace, then just return.
   */
  if( frames == NULL )
    {
      return FALSE;
    }

  curr_func = frames[curr_frame].frame;

  /*
   * If we don't know what the current function is, then we also have
   * nothing to report here.
   */
  if( curr_func == NULL )
    {
      return FALSE;
    }

  for(i=0; i < curr_func->n_locals; i++ )
    {
      /*
       * Test the range of validity of the local variable.  This
       * comes up with RBRAC/LBRAC stabs in particular.
       */
      if(    (curr_func->local_vars[i].pc_start != 0)
	  && ((frames[curr_frame].eip - curr_func->addr.off) 
	      < curr_func->local_vars[i].pc_start) )
	{
	  continue;
	}

      if(    (curr_func->local_vars[i].pc_end != 0)
	  && ((frames[curr_frame].eip - curr_func->addr.off) 
	      > curr_func->local_vars[i].pc_end) )
	{
	  continue;
	}
      
      if( curr_func->local_vars[i].offset == 0 )
	{
	  fprintf(stderr, "%s:%s optimized into register $%s \n",
		  curr_func->name, curr_func->local_vars[i].name,
		  reg_name[curr_func->local_vars[i].regno]);
	}
      else
	{
	  ptr = (unsigned int *) (frames[curr_frame].ebp 
				   + curr_func->local_vars[i].offset);
	  fprintf(stderr, "%s:%s == 0x%8.8x\n",
		  curr_func->name, curr_func->local_vars[i].name,
		  *ptr);
	}
    }

  rtn = TRUE;

  return (rtn);
}
