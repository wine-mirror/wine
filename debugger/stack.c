/*
 * Debugger stack handling
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include "windows.h"
#include "debugger.h"


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
        fprintf(stderr,"%d ",frameno++);
        addr.off = EIP_reg(DEBUG_context);
        DEBUG_PrintAddress( &addr, 32 );
        fprintf( stderr, "\n" );
        addr.off = EBP_reg(DEBUG_context);
        while (addr.off)
        {
            FRAME32 *frame = (FRAME32 *)addr.off;
            if (!DBG_CHECK_READ_PTR( &addr, sizeof(FRAME32) )) return;
            if (!frame->ip) break;
            fprintf(stderr,"%d ",frameno++);
            addr.off = frame->ip;
            DEBUG_PrintAddress( &addr, 32 );
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
      DEBUG_PrintAddress( &addr, 16 );
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
          DEBUG_PrintAddress( &addr, 16 );
          fprintf( stderr, "\n" );
          addr.seg = ss;
          addr.off = frame->bp & ~1;
      }
  }
  fprintf( stderr, "\n" );
}
