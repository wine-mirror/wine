/*
 * What processor?
 *
 * Copyright 1995 Morten Welinder
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "windows.h"

int runtime_cpu (void)
{
  static int cache = 0;

#ifdef linux
  if (!cache)
    {
      FILE *f = fopen ("/proc/cpuinfo", "r");

      cache = 3;  /* Default.  */

      if (f)
	{
	  char info[5], value[5];
	  while (fscanf (f, " %4s%*s : %4s%*s", info, value) == 2)
	    if (!lstrcmpi (info, "cpu"))
	      {
		if (isdigit (value[0]) && value[1] == '8'
		    && value[2] == '6' && value[3] == 0)
		  {
		    cache = value[0] - '0';
		    break;
		  }
	      }
	  fclose (f);
	}
    }
  return cache;
#else
  /* FIXME: how do we do this on other systems? */
  return 3;
#endif
}
