/*
 * Main function.
 *
 * Copyright 1994 Alexandre Julliard
 */

#include "config.h"

#include <locale.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifdef MALLOC_DEBUGGING
# include <malloc.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "ntddk.h"
#include "winnls.h"
#include "winerror.h"

#include "winsock.h"
#include "heap.h"
#include "msdos.h"
#include "options.h"
#include "debugtools.h"
#include "module.h"
#include "tweak.h"

DECLARE_DEBUG_CHANNEL(file);


#if 0
/***********************************************************************
 *          MAIN_ParseDebugOptions
 *
 *  Turns specific debug messages on or off, according to "options".
 */
void MAIN_ParseDebugOptions( const char *arg )
{
  /* defined in relay32/relay386.c */
  extern char **debug_relay_includelist;
  extern char **debug_relay_excludelist;
  /* defined in relay32/snoop.c */
  extern char **debug_snoop_includelist;
  extern char **debug_snoop_excludelist;

  int i;
  int l, cls;

  char *options = strdup(arg);

  l = strlen(options);
  if (l<2) goto error;

  if (options[l-1]=='\n') options[l-1]='\0';
  do
  {
    if ((*options!='+')&&(*options!='-')){
      int j;

      for(j=0; j<DEBUG_CLASS_COUNT; j++)
	if(!strncasecmp(options, debug_cl_name[j], strlen(debug_cl_name[j])))
	  break;
      if(j==DEBUG_CLASS_COUNT)
	goto error;
      options += strlen(debug_cl_name[j]);
      if ((*options!='+')&&(*options!='-'))
	goto error;
      cls = j;
    }
    else
      cls = -1; /* all classes */

    if (strchr(options,','))
      l=strchr(options,',')-options;
    else
      l=strlen(options);

    if (!strncasecmp(options+1,"all",l-1))
      {
	int i, j;
	for (i=0; i<DEBUG_CHANNEL_COUNT; i++)
	  for(j=0; j<DEBUG_CLASS_COUNT; j++)
	    if(cls == -1 || cls == j)
                __SET_DEBUGGING( j, debug_channels[i], (*options=='+') );
      }
    else if (!strncasecmp(options+1, "relay=", 6) ||
	     !strncasecmp(options+1, "snoop=", 6))
      {
	int i, j;
	char *s, *s2, ***output, c;

	for (i=0; i<DEBUG_CHANNEL_COUNT; i++)
	  if (!strncasecmp( debug_channels[i] + 1, options + 1, 5))
          {
	    for(j=0; j<DEBUG_CLASS_COUNT; j++)
	      if(cls == -1 || cls == j)
                  __SET_DEBUGGING( j, debug_channels[i], 1 );
	    break;
	  }
	/* should never happen, maybe assert(i!=DEBUG_CHANNEL_COUNT)? */
	if (i==DEBUG_CHANNEL_COUNT)
	  goto error;
	output = (*options == '+') ?
			((*(options+1) == 'r') ?
				&debug_relay_includelist :
				&debug_snoop_includelist) :
			((*(options+1) == 'r') ?
				&debug_relay_excludelist :
				&debug_snoop_excludelist);
	s = options + 7;
	/* if there are n ':', there are n+1 modules, and we need n+2 slots
	 * last one being for the sentinel (NULL) */
	i = 2;	
	while((s = strchr(s, ':'))) i++, s++;
	*output = malloc(sizeof(char **) * i);
	i = 0;
	s = options + 7;
	while((s2 = strchr(s, ':'))) {
          c = *s2;
          *s2 = '\0';
	  *((*output)+i) = _strupr(strdup(s));
          *s2 = c;
	  s = s2 + 1;
	  i++;
	}
	c = *(options + l);
	*(options + l) = '\0';
	*((*output)+i) = _strupr(strdup(s));
	*(options + l) = c;
	*((*output)+i+1) = NULL;
      }
    else
      {
	int i, j;
	for (i=0; i<DEBUG_CHANNEL_COUNT; i++)
          if (!strncasecmp( debug_channels[i] + 1, options + 1, l - 1) && !debug_channels[i][l])
          {
	    for(j=0; j<DEBUG_CLASS_COUNT; j++)
	      if(cls == -1 || cls == j)
                  __SET_DEBUGGING( j, debug_channels[i], (*options=='+') );
	    break;
	  }
	if (i==DEBUG_CHANNEL_COUNT)
	  goto error;
      }
    options+=l;
  }
  while((*options==',')&&(*(++options)));

  if (!*options) return;

 error:  
  MESSAGE("%s: Syntax: --debugmsg [class]+xxx,...  or "
      "-debugmsg [class]-xxx,...\n",argv0);
  MESSAGE("Example: --debugmsg +all,warn-heap\n"
      "  turn on all messages except warning heap messages\n");
  MESSAGE("Special case: --debugmsg +relay=DLL:DLL.###:FuncName\n"
      "  turn on -debugmsg +relay only as specified\n"
      "Special case: --debugmsg -relay=DLL:DLL.###:FuncName\n"
      "  turn on --debugmsg +relay except as specified\n"
      "Also permitted, +snoop=..., -snoop=... as with relay.\n\n");
  
  MESSAGE("Available message classes:\n");
  for(i=0;i<DEBUG_CLASS_COUNT;i++)
    MESSAGE( "%-9s", debug_cl_name[i]);
  MESSAGE("\n\n");
  
  MESSAGE("Available message types:\n");
  MESSAGE("%-9s ","all");
  for(i=0;i<DEBUG_CHANNEL_COUNT;i++)
      MESSAGE("%-9s%c",debug_channels[i] + 1,
	  (((i+2)%8==0)?'\n':' '));
  MESSAGE("\n\n");
  ExitProcess(1);
}
#endif

/***********************************************************************
 *           MAIN_WineInit
 *
 * Wine initialisation
 */
void MAIN_WineInit(void)
{
#ifdef MALLOC_DEBUGGING
    char *trace;

    mcheck(NULL);
    if (!(trace = getenv("MALLOC_TRACE")))
    {       
        MESSAGE( "MALLOC_TRACE not set. No trace generated\n" );
    }
    else
    {
        MESSAGE( "malloc trace goes to %s\n", trace );
        mtrace();
    }
#endif

    setbuf(stdout,NULL);
    setbuf(stderr,NULL);
    setlocale(LC_CTYPE,"");
}

/***********************************************************************
 *           Beep   (KERNEL32.11)
 */
BOOL WINAPI Beep( DWORD dwFreq, DWORD dwDur )
{
    static char beep = '\a';
    /* dwFreq and dwDur are ignored by Win95 */
    if (isatty(2)) write( 2, &beep, 1 );
    return TRUE;
}


/***********************************************************************
*	FileCDR (KERNEL.130)
*/
FARPROC16 WINAPI FileCDR16(FARPROC16 x)
{
	FIXME_(file)("(0x%8x): stub\n", (int) x);
	return (FARPROC16)TRUE;
}

/***********************************************************************
 *           GetTickCount   (USER.13) (KERNEL32.299)
 *
 * Returns the number of milliseconds, modulo 2^32, since the start
 * of the wineserver.
 */
DWORD WINAPI GetTickCount(void)
{
    struct timeval t;
    gettimeofday( &t, NULL );
    return ((t.tv_sec * 1000) + (t.tv_usec / 1000)) - server_startticks;
}
