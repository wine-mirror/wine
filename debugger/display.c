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

#include "neexe.h"
#include "module.h"
#include "selectors.h"
#include "debugger.h"

#include <stdarg.h>

#define MAX_DISPLAY 25

struct display
{
  struct expr *	exp;
  int		count;
  char		format;
};

static struct display displaypoints[MAX_DISPLAY];

int
DEBUG_AddDisplay(struct expr * exp, int count, char format)
{
  int i;

  /*
   * First find a slot where we can store this display.
   */
  for(i=0; i < MAX_DISPLAY; i++ )
    {
      if( displaypoints[i].exp == NULL )
	{
	  displaypoints[i].exp = DEBUG_CloneExpr(exp);
	  displaypoints[i].count  = count;
	  displaypoints[i].format = format;
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
      if( displaypoints[i].exp != NULL )
	{
	  fprintf(stderr, "%d : ", i+1);
	  DEBUG_DisplayExpr(displaypoints[i].exp);
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
      if( displaypoints[i].exp != NULL )
	{
	  addr = DEBUG_EvalExpr(displaypoints[i].exp);
	  if( addr.type == NULL )
	    {
	      fprintf(stderr, "Unable to evaluate expression ");
	      DEBUG_DisplayExpr(displaypoints[i].exp);
	      fprintf(stderr, "\nDisabling...\n");
	      DEBUG_DelDisplay(i);
	    }
	  else
	    {
	      fprintf(stderr, "%d  : ", i + 1);
	      DEBUG_DisplayExpr(displaypoints[i].exp);
	      fprintf(stderr, " = ");
	      if( displaypoints[i].format == 'i' )
		{
		  DEBUG_ExamineMemory( &addr, 
			       displaypoints[i].count, 
			       displaypoints[i].format);
		}
	      else
		{
		  DEBUG_Print( &addr, 
			       displaypoints[i].count, 
			       displaypoints[i].format, 0);
		}
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
	  if( displaypoints[i].exp != NULL )
	    {
	      DEBUG_FreeExpr(displaypoints[i].exp);
	      displaypoints[i].exp = NULL;
	    }
	}
    }
  else if( displaypoints[displaynum - 1].exp != NULL )
    {
      DEBUG_FreeExpr(displaypoints[displaynum - 1].exp);
      displaypoints[displaynum - 1].exp = NULL;
    }
  return TRUE;
}
