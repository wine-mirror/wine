/*
 * msvcrt.dll thread functions
 *
 * Copyright 2000 Jon Griffiths
 */
#include "msvcrt.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);


/*********************************************************************
 *		_beginthreadex (MSVCRT.@)
 */
unsigned long __cdecl MSVCRT__beginthreadex(void *sec,
                                            unsigned int stack,
                                            LPTHREAD_START_ROUTINE start,
                                            void *arg, unsigned int flag,
                                            unsigned int*addr)
{
  TRACE("(%p,%d,%p,%p,%d,%p)\n",sec, stack,start, arg,flag,addr);
  /* FIXME */
  return CreateThread( sec, stack, (LPTHREAD_START_ROUTINE)start, arg,flag,(LPDWORD)addr);
}

/*********************************************************************
 *		_endthreadex (MSVCRT.@)
 */
void __cdecl MSVCRT__endthreadex(unsigned int retval)
{
  TRACE("(%d)\n",retval);
  /* FIXME */
  ExitThread(retval);
}

