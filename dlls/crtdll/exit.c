/*
 * CRTDLL exit/abort/atexit functions
 * 
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 *
 * exit functions differ in whether they perform cleanup
 * and whether they return to the caller (really!).
 *            return      do
 *  Name      to caller?  cleanup? 
 *  _c_exit     Y           N
 *  _cexit      Y           Y
 *  _exit       N           N
 *  exit        N           Y
 *
 * Implementation Notes:
 * Not MT Safe - Adding/Executing exit() functions should be locked
 * for MT safety.
 *
 * FIXME:
 * Need a better way of printing errors for GUI programs(MsgBox?).
 * Is there really a difference between onexit/atexit?
 */
#include "crtdll.h"
#include <errno.h>


DEFAULT_DEBUG_CHANNEL(crtdll);

/* INTERNAL: Table of registered atexit() functions */
/* FIXME: This should be dynamically allocated */
#define CRTDLL_ATEXIT_TABLE_SIZE 16

static atexit_function atexit_table[CRTDLL_ATEXIT_TABLE_SIZE];
static int atexit_registered = 0; /* Points to free slot */


/* INTERNAL: call atexit functions */
void __CRTDLL__call_atexit(VOID);
void __CRTDLL__call_atexit(VOID)
{
  /* Last registered gets executed first */
  while (atexit_registered > 0)
  {
    atexit_registered--;
    TRACE(":call function (%p)\n",atexit_table[atexit_registered]);
    (*atexit_table[atexit_registered])();
  }
}


/*********************************************************************
 *                  __dllonexit           (CRTDLL.25)
 */
VOID __cdecl CRTDLL___dllonexit ()
{
    FIXME("stub\n");
}


/*********************************************************************
 *                  _abnormal_termination          (CRTDLL.36)
 *
 * Check if execution is processing during an exception.
 */
INT __cdecl CRTDLL__abnormal_termination(VOID)
{
  TRACE("(void)\n");
  return 0; /* FIME: Can we determine if we are in an exception? */
}


/*********************************************************************
 *                  _amsg_exit     (CRTDLL.040)
 *
 * Print an error message and terminate execution. Returns 255 to the
 * host OS.
 */
VOID __cdecl CRTDLL__amsg_exit(INT err)
{
  /* FIXME: Should be a popup for msvcrt gui executables, and should have
   * text for the error number.
   */
  CRTDLL_fprintf(CRTDLL_stderr,"\nruntime error R60%d\n",err);
  CRTDLL__exit(255);
}


/*********************************************************************
 *                  _assert     (CRTDLL.041)
 *
 * Print an assertion message and call abort(). Really only present 
 * for win binaries. Winelib programs would typically use libc's
 * version.
 */
VOID __cdecl CRTDLL__assert(LPVOID str, LPVOID file, UINT line)
{
  CRTDLL_fprintf(CRTDLL_stderr,"Assertion failed: %s, file %s, line %d\n\n",
                 (char*)str,(char*)file, line);
  CRTDLL_abort();
}


/*********************************************************************
 *                  _c_exit           (CRTDLL.047)
 */
VOID __cdecl CRTDLL__c_exit(VOID)
{
  /* All cleanup is done on DLL detach; Return to caller */
}


/*********************************************************************
 *                  _cexit           (CRTDLL.049)
 */
VOID __cdecl CRTDLL__cexit(VOID)
{
  /* All cleanup is done on DLL detach; Return to caller */
}


/*********************************************************************
 *                  _exit           (CRTDLL.087)
 */
VOID __cdecl CRTDLL__exit(LONG ret)
{
  TRACE(":exit code %ld\n",ret);
  CRTDLL__c_exit();
  ExitProcess(ret);
}

/*********************************************************************
 *                  _onexit           (CRTDLL.236)
 *
 * Register a function to be called when the process terminates.
 */
atexit_function __cdecl CRTDLL__onexit( atexit_function func)
{
  TRACE("registering function (%p)\n",func);
  if (func && atexit_registered <= CRTDLL_ATEXIT_TABLE_SIZE - 1)
  {
    atexit_table[atexit_registered] = (atexit_function)func;
    atexit_registered++;
    return func; /* successful */
  }
  ERR(":Too many onexit() functions, or NULL function - not registered!\n");
  return NULL;
}


/*********************************************************************
 *                  exit          (CRTDLL.359)
 */
void __cdecl CRTDLL_exit(DWORD ret)
{
  TRACE(":exit code %ld\n",ret);
  __CRTDLL__call_atexit();
  CRTDLL__cexit();
  ExitProcess(ret);
}


/*********************************************************************
 *                   abort     (CRTDLL.335)
 *
 * Terminate the progam with an abnormal termination message. Returns
 * 3 to the host OS.
 */
VOID __cdecl CRTDLL_abort()
{
  CRTDLL_fprintf(CRTDLL_stderr,"\nabnormal program termination\n");
  CRTDLL__exit(3);
}


/*********************************************************************
 *                  atexit           (CRTDLL.345)
 *
 * Register a function to be called when the process terminates.
 */
INT __cdecl CRTDLL_atexit( atexit_function func)
{
  return CRTDLL__onexit(func) == func ? 0 : -1;
}
