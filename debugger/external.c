/* 
 *  File    : external.c
 *  Author  : Kevin Holbrook
 *  Created : July 18, 1999
 *
 *  Convenience functions to handle use of external debugger.
 *
 */


#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



#define DBG_BUFF_SIZE  12

#define DBG_EXTERNAL_DEFAULT   "gdb"
#define DBG_LOCATION_DEFAULT   "/usr/local/bin/wine"
#define DBG_SLEEPTIME_DEFAULT  120



/* DEBUG_ExternalDebugger
 *
 * This function invokes an external debugger on the current
 * wine process. The form of the command executed is:
 *  <debugger image> <wine image> <attach process id>
 *
 * The debugger command is normally invoked by a newly created xterm.
 *
 * The current calling process is temporarily put to sleep
 * so that the invoked debugger has time to come up and attach.
 *
 * The following environment variables may be used:
 *
 *   Name                Use                                      Default
 *  -------------------------------------------------------------------------------------
 *   WINE_DBG_EXTERNAL   debugger command to invoke               ("gdb")
 *   WINE_DBG_LOCATION   fully qualified location of wine image   ("/usr/local/bin/wine")
 *   WINE_DBG_NO_XTERM   if set do not invoke xterm with command  (not set)
 *   WINE_DBG_SLEEPTIME  number of seconds to make process sleep  (120)
 *
 *
 * Usage:
 *
 *   #include "debugtools.h"
 *
 *   DEBUG_ExternalDebugger();
 *
 *
 * Environment Example:
 *
 *   export WINE_DBG_EXTERNAL="ddd"
 *   export WINE_DBG_NO_XTERM=1
 *   export WINE_DBG_SLEEPTIME=60
 *
 */

void DEBUG_ExternalDebugger(void)
{
  pid_t attach_pid;
  pid_t child_pid;
  int   dbg_sleep_secs = DBG_SLEEPTIME_DEFAULT;
  char *dbg_sleeptime;


  dbg_sleeptime = getenv("WINE_DBG_SLEEPTIME");

  /* convert sleep time string to integer seconds */
  if (dbg_sleeptime)
  {
    dbg_sleep_secs = atoi(dbg_sleeptime);

    /* check for conversion error */
    if (dbg_sleep_secs == 0)
      dbg_sleep_secs = DBG_SLEEPTIME_DEFAULT;
  }

  /* get the curent process id */
  attach_pid = getpid();

  /* create new process */
  child_pid = fork();

  /* check if we are the child process */
  if (child_pid == 0)
  {
    int  status;
    char *dbg_external;
    char *dbg_wine_location;
    char *dbg_no_xterm;
    char pid_string[DBG_BUFF_SIZE];    


    /* check settings in environment for debugger to use */
    dbg_external      = getenv("WINE_DBG_EXTERNAL");
    dbg_wine_location = getenv("WINE_DBG_LOCATION");
    dbg_no_xterm      = getenv("WINE_DBG_NO_XTERM");

    /* if not set in environment, use default */
    if (!dbg_external)
      dbg_external = "gdb";

    /* if not set in environment, use default */
    if (!dbg_wine_location)
      dbg_wine_location = "/usr/local/bin/wine";

    /* check for empty string in WINE_DBG_NO_XTERM */
    if (dbg_no_xterm && (strlen(dbg_no_xterm) < 1))
      dbg_no_xterm = NULL;

    /* clear the buffer */
    memset(pid_string, 0, DBG_BUFF_SIZE);

    /* make pid into string */
    sprintf(pid_string, "%d", attach_pid);

    /* now exec the debugger to get it's own clean memory space */
    if (dbg_no_xterm)
      status = execlp(dbg_external, dbg_external, dbg_wine_location, pid_string, NULL);
    else
      status = execlp("xterm", "xterm", "-e", dbg_external, dbg_wine_location, pid_string, NULL); 

    if (status == -1)
    {
      if (dbg_no_xterm)
        fprintf(stderr, "DEBUG_ExternalDebugger failed to execute \"%s %s %s\", errno = %d\n", 
                dbg_external, dbg_wine_location, pid_string, errno);
      else
        fprintf(stderr, "DEBUG_ExternalDebugger failed to execute \"xterm -e %s %s %s\", errno = %d\n", 
                dbg_external, dbg_wine_location, pid_string, errno);
    }

  }
  else if (child_pid != -1)
  {
    /* make the parent/caller sleep so the child/debugger can catch it */
    sleep(dbg_sleep_secs);
  }
  else
    fprintf(stderr, "DEBUG_ExternalDebugger failed.\n");
  
}


