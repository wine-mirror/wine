/*
 * msvcrt.dll exit functions
 *
 * Copyright 2000 Jon Griffiths
 */
#include "msvcrt.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);

/* MT */
extern CRITICAL_SECTION MSVCRT_exit_cs;
#define LOCK_EXIT      EnterCriticalSection(&MSVCRT_exit_cs)
#define UNLOCK_EXIT    LeaveCriticalSection(&MSVCRT_exit_cs)

typedef void (__cdecl *MSVCRT_atexit_func)(void);

static MSVCRT_atexit_func *MSVCRT_atexit_table = NULL;
static int MSVCRT_atexit_table_size = 0;
static int MSVCRT_atexit_registered = 0; /* Points to free slot */

extern int MSVCRT_app_type;

/* INTERNAL: call atexit functions */
void __MSVCRT__call_atexit(void)
{
  /* Note: should only be called with the exit lock held */
  TRACE("%d atext functions to call\n", MSVCRT_atexit_registered);
  /* Last registered gets executed first */
  while (MSVCRT_atexit_registered > 0)
  {
    MSVCRT_atexit_registered--;
    TRACE("next is %p\n",MSVCRT_atexit_table[MSVCRT_atexit_registered]);
    if (MSVCRT_atexit_table[MSVCRT_atexit_registered])
      (*MSVCRT_atexit_table[MSVCRT_atexit_registered])();
    TRACE("returned\n");
  }
}

/*********************************************************************
 *		__dllonexit (MSVCRT.@)
 */
void __cdecl MSVCRT___dllonexit (MSVCRT_atexit_func func, MSVCRT_atexit_func **start,
                                 MSVCRT_atexit_func **end)
{
  FIXME("(%p,%p,%p)stub\n", func, start,end);
  /* FIXME: How do we know when to increase the table? */
}

/*********************************************************************
 *		_exit (MSVCRT.@)
 */
void __cdecl MSVCRT__exit(int exitcode)
{
  TRACE("(%d)\n", exitcode);
  ExitProcess(exitcode);
}

/*********************************************************************
 *		_amsg_exit (MSVCRT.@)
 */
void __cdecl MSVCRT__amsg_exit(int errnum)
{
  TRACE("(%d)\n", errnum);
  /* FIXME: text for the error number. */
  if (MSVCRT_app_type == 2)
  {
    /* FIXME: MsgBox */
  }
  MSVCRT__cprintf("\nruntime error R60%d\n",errnum);
  MSVCRT__exit(255);
}

/*********************************************************************
 *		abort (MSVCRT.@)
 */
void __cdecl MSVCRT_abort(void)
{
  TRACE("(void)\n");
  if (MSVCRT_app_type == 2)
  {
    /* FIXME: MsgBox */
  }
  MSVCRT__cputs("\nabnormal program termination\n");
  MSVCRT__exit(3);
}

/*********************************************************************
 *		_assert (MSVCRT.@)
 */
void __cdecl MSVCRT__assert(const char* str, const char* file, unsigned int line)
{
  TRACE("(%s,%s,%d)\n",str,file,line);
  if (MSVCRT_app_type == 2)
  {
    /* FIXME: MsgBox */
  }
  MSVCRT__cprintf("Assertion failed: %s, file %s, line %d\n\n",str,file, line);
  MSVCRT_abort();
}

/*********************************************************************
 *		_c_exit (MSVCRT.@)
 */
void __cdecl MSVCRT__c_exit(void)
{
  TRACE("(void)\n");
  /* All cleanup is done on DLL detach; Return to caller */
}

/*********************************************************************
 *		_cexit (MSVCRT.@)
 */
void __cdecl MSVCRT__cexit(void)
{
  TRACE("(void)\n");
  /* All cleanup is done on DLL detach; Return to caller */
}

/*********************************************************************
 *		_onexit (MSVCRT.@)
 */
 MSVCRT_atexit_func __cdecl MSVCRT__onexit(MSVCRT_atexit_func func)
{
  TRACE("(%p)\n",func);

  if (!func)
    return NULL;

  LOCK_EXIT;
  if (MSVCRT_atexit_registered > MSVCRT_atexit_table_size - 1)
  {
    MSVCRT_atexit_func *newtable;
    TRACE("expanding table\n");
    newtable = MSVCRT_calloc(sizeof(void *),MSVCRT_atexit_table_size + 32);
    if (!newtable)
    {
      TRACE("failed!\n");
      UNLOCK_EXIT;
      return NULL;
    }
    memcpy (newtable, MSVCRT_atexit_table, MSVCRT_atexit_table_size);
    MSVCRT_atexit_table_size += 32;
    if (MSVCRT_atexit_table)
      MSVCRT_free (MSVCRT_atexit_table);
    MSVCRT_atexit_table = newtable;
  }
  MSVCRT_atexit_table[MSVCRT_atexit_registered] = func;
  MSVCRT_atexit_registered++;
  UNLOCK_EXIT;
  return func;
}

/*********************************************************************
 *		exit (MSVCRT.@)
 */
void __cdecl MSVCRT_exit(int exitcode)
{
  TRACE("(%d)\n",exitcode);
  LOCK_EXIT;
  __MSVCRT__call_atexit();
  UNLOCK_EXIT;
  ExitProcess(exitcode);
}

/*********************************************************************
 *		atexit (MSVCRT.@)
 */
int __cdecl MSVCRT_atexit(MSVCRT_atexit_func func)
{
  TRACE("(%p)\n", func);
  return MSVCRT__onexit(func) == func ? 0 : -1;
}

/*********************************************************************
 *		_purecall (MSVCRT.@)
 */
void __cdecl MSVCRT__purecall(void)
{
  TRACE("(void)\n");
  MSVCRT__amsg_exit( 25 );
}
