/*
 * except.h
 * Copyright (c) 1996, Onno Hovers (onno@stack.urc.tue.nl)
 */

#ifndef __WINE_EXCEPT_H
#define __WINE_EXCEPT_H

#include "winnt.h"

/*
 * the function pointer to a exception handler
 */

/* forward definition */
struct __EXCEPTION_FRAME;

typedef DWORD ( *PEXCEPTION_HANDLER)( PEXCEPTION_RECORD          pexcrec,
                                      struct __EXCEPTION_FRAME  *pestframe,
                                      PCONTEXT                   pcontext,
                                      LPVOID                     pdispatcher);

/*
 * The exception frame, used for registering exception handlers 
 * Win32 cares only about this, but compilers generally emit 
 * larger exception frames for their own use.
 */

typedef struct __EXCEPTION_FRAME
{
  struct __EXCEPTION_FRAME *Prev;
  PEXCEPTION_HANDLER       Handler;
} EXCEPTION_FRAME, *PEXCEPTION_FRAME;

/*
 * Function definitions  
 */
 
void RaiseException(DWORD exccode, DWORD excflags, 
                    DWORD nargs, const LPDWORD pargs,
                    PCONTEXT pcontext /* Wine additional parameter */ ); 
                        
/*
 *  this undocumented function is called when an exception
 *  handler wants all the frames to be unwound. RtlUnwind
 *  calls all exception handlers with the EH_UNWIND or
 *  EH_EXIT_UNWIND flags set in the exception record
 *
 *  This prototype assumes RtlUnwind takes the same
 *  parameters as OS/2 2.0 DosUnwindException
 *  Disassembling RtlUnwind shows this is true, except for
 *  the TargetEIP parameter, which is unused. There is 
 *  a fourth parameter, that is used as the eax in the 
 *  context.   
 */

void RtlUnwind( PEXCEPTION_FRAME pestframe,
                LPVOID unusedEIP,
                PEXCEPTION_RECORD pexcrec,
                DWORD contextEAX,
                PCONTEXT pcontext /* Wine additional parameter */ );

#endif  /* __WINE_EXCEPT_H */
