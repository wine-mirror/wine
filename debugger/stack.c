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
    fprintf(stderr,"Stack dump:\n");
    if ((SS == WINE_DATA_SELECTOR) ||
        (GET_SEL_FLAGS(SS) & LDT_FLAGS_32BIT))  /* 32-bit mode */
    {
        examine_memory( 0, ESP, 10, 'x' );
    }
    else  /* 16-bit mode */
    {
        examine_memory( SS, SP, 10, 'w' );
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
  int frameno = 0;

  fprintf(stderr,"Backtrace:\n");
  if (SS == WINE_DATA_SELECTOR)  /* 32-bit mode */
  {
      FRAME32 *frame = (FRAME32 *)EBP;
      while (frame->ip)
      {
          fprintf(stderr,"%d ",frameno++);
          print_address( 0, frame->ip, 32 );
          fprintf( stderr, "\n" );
          frame = (FRAME32 *)frame->bp;
      }
  }
  else  /* 16-bit mode */
  {
      FRAME16 *frame = (FRAME16 *)PTR_SEG_OFF_TO_LIN( SS, BP & ~1 );
      WORD cs = CS;
      if (GET_SEL_FLAGS(SS) & LDT_FLAGS_32BIT)
      {
          fprintf( stderr, "Not implemented: 32-bit backtrace on a different stack segment.\n" );
          return;
      }
      while (frame->bp)
      {
          if (frame->bp & 1) cs = frame->cs;
          fprintf( stderr,"%d ", frameno++ );
          print_address( cs, frame->ip, 16 );
          fprintf( stderr, "\n" );
          frame = (FRAME16 *)PTR_SEG_OFF_TO_LIN( SS, frame->bp & ~1 );
      }
  }
  fprintf( stderr, "\n" );
}
