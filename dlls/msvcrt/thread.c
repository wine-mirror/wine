/*
 * msvcrt.dll thread functions
 *
 * Copyright 2000 Jon Griffiths
 */
#include "msvcrt.h"

#include "msvcrt/process.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);


/*********************************************************************
 *		_beginthreadex (MSVCRT.@)
 */
unsigned long _beginthreadex(void* sec,
                             unsigned int stack,
                             unsigned __stdcall (*start)(void*),
                             void* arg, unsigned int flag,
                             unsigned int* addr)
{
  TRACE("(%p,%d,%p,%p,%d,%p)\n",sec, stack,start, arg,flag,addr);
  /* FIXME */
  return CreateThread( sec, stack, (LPTHREAD_START_ROUTINE)start, arg,flag,(LPDWORD)addr);
}

/*********************************************************************
 *		_endthreadex (MSVCRT.@)
 */
void _endthreadex(unsigned int retval)
{
  TRACE("(%d)\n",retval);
  /* FIXME */
  ExitThread(retval);
}

