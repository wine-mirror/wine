/*
 * File display.c - display handling for Wine internal debugger.
 *
 * Copyright (C) 1997, Eric Youngdale.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <neexe.h>
#include "module.h"
#include "selectors.h"
#include "debugger.h"
#include "toolhelp.h"
#include "xmalloc.h"

#include <stdarg.h>

#define MAX_DISPLAY 25
static struct expr * displaypoints[MAX_DISPLAY];

int
DEBUG_AddDisplay(struct expr * exp)
{
  int i;

  /*
   * First find a slot where we can store this display.
   */
  for(i=0; i < MAX_DISPLAY; i++ )
    {
      if( displaypoints[i] == NULL )
	{
	  displaypoints[i] = DEBUG_CloneExpr(exp);
	  break;
	}
    }

  return TRUE;
}

int
DEBUG_InfoDisplay()
{
  int i;

  /*
   * First find a slot where we can store this display.
   */
  for(i=0; i < MAX_DISPLAY; i++ )
    {
      if( displaypoints[i] != NULL )
	{
	  fprintf(stderr, "%d : ", i+1);
	  DEBUG_DisplayExpr(displaypoints[i]);
	  fprintf(stderr, "\n");
	}
    }

  return TRUE;
}

int
DEBUG_DoDisplay()
{
  DBG_ADDR	addr;
  int i;

  /*
   * First find a slot where we can store this display.
   */
  for(i=0; i < MAX_DISPLAY; i++ )
    {
      if( displaypoints[i] != NULL )
	{
	  addr = DEBUG_EvalExpr(displaypoints[i]);
	  if( addr.type == NULL )
	    {
	      fprintf(stderr, "Unable to evaluate expression ");
	      DEBUG_DisplayExpr(displaypoints[i]);
	      fprintf(stderr, "\nDisabling...\n");
	      DEBUG_DelDisplay(i);
	    }
	  else
	    {
	      fprintf(stderr, "%d  : ", i + 1);
	      DEBUG_DisplayExpr(displaypoints[i]);
	      fprintf(stderr, " = ");
	      DEBUG_Print( &addr, 1, 0, 0);
	    }
	}
    }

  return TRUE;
}

int
DEBUG_DelDisplay(int displaynum)
{
  int i;
  
  if( displaynum >= MAX_DISPLAY || displaynum == 0 || displaynum < -1 )
    {
      fprintf(stderr, "Invalid display number\n");
      return TRUE;
    }
  if( displaynum == -1 )
    {
      for(i=0; i < MAX_DISPLAY; i++ )
	{
	  if( displaypoints[i] != NULL )
	    {
	      DEBUG_FreeExpr(displaypoints[i]);
	      displaypoints[i] = NULL;
	    }
	}
    }
  else if( displaypoints[displaynum - 1] != NULL )
    {
      DEBUG_FreeExpr(displaypoints[displaynum - 1]);
      displaypoints[displaynum - 1] = NULL;
    }
  return TRUE;
}
