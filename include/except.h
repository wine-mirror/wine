/*
 * except.h
 * Copyright (c) 1996, Onno Hovers (onno@stack.urc.tue.nl)
 */

#ifndef __WINE_EXCEPT_H
#define __WINE_EXCEPT_H

#include"windows.h"

/* 
 * general definitions
 */
 
#ifndef PVOID
#define PVOID void *
#endif

/*
 * exception codes
 */
 
#define STATUS_WAIT_0                    0x00000000
#define STATUS_ABANDONED_WAIT_0          0x00000080
#define STATUS_USER_APC                  0x000000C0
#define STATUS_TIMEOUT                   0x00000102
#define STATUS_PENDING                   0x00000103
#define STATUS_DATATYPE_MISALIGNMENT     0x80000002
#define STATUS_BREAKPOINT                0x80000003
#define STATUS_SINGLE_STEP               0x80000004
#define STATUS_ACCESS_VIOLATION          0xC0000005
#define STATUS_IN_PAGE_ERROR             0xC0000006
#define STATUS_NO_MEMORY                 0xC0000017
#define STATUS_ILLEGAL_INSTRUCTION       0xC000001D
#define STATUS_NONCONTINUABLE_EXCEPTION  0xC0000025
#define STATUS_INVALID_DISPOSITION       0xC0000026
#define STATUS_ARRAY_BOUNDS_EXCEEDED     0xC000008C
#define STATUS_FLOAT_DENORMAL_OPERAND    0xC000008D
#define STATUS_FLOAT_DIVIDE_BY_ZERO      0xC000008E
#define STATUS_FLOAT_INEXACT_RESULT      0xC000008F
#define STATUS_FLOAT_INVALID_OPERATION   0xC0000090
#define STATUS_FLOAT_OVERFLOW            0xC0000091
#define STATUS_FLOAT_STACK_CHECK         0xC0000092
#define STATUS_FLOAT_UNDERFLOW           0xC0000093
#define STATUS_INTEGER_DIVIDE_BY_ZERO    0xC0000094
#define STATUS_INTEGER_OVERFLOW          0xC0000095
#define STATUS_PRIVILEGED_INSTRUCTION    0xC0000096
#define STATUS_STACK_OVERFLOW            0xC00000FD
#define STATUS_CONTROL_C_EXIT            0xC000013A

#define EXCEPTION_ACCESS_VIOLATION      STATUS_ACCESS_VIOLATION
#define EXCEPTION_DATATYPE_MISALIGNMENT STATUS_DATATYPE_MISALIGNMENT
#define EXCEPTION_BREAKPOINT            STATUS_BREAKPOINT
#define EXCEPTION_SINGLE_STEP           STATUS_SINGLE_STEP
#define EXCEPTION_ARRAY_BOUNDS_EXCEEDED STATUS_ARRAY_BOUNDS_EXCEEDED
#define EXCEPTION_FLT_DENORMAL_OPERAND  STATUS_FLOAT_DENORMAL_OPERAND
#define EXCEPTION_FLT_DIVIDE_BY_ZERO    STATUS_FLOAT_DIVIDE_BY_ZERO
#define EXCEPTION_FLT_INEXACT_RESULT    STATUS_FLOAT_INEXACT_RESULT
#define EXCEPTION_FLT_INVALID_OPERATION STATUS_FLOAT_INVALID_OPERATION
#define EXCEPTION_FLT_OVERFLOW          STATUS_FLOAT_OVERFLOW
#define EXCEPTION_FLT_STACK_CHECK       STATUS_FLOAT_STACK_CHECK
#define EXCEPTION_FLT_UNDERFLOW         STATUS_FLOAT_UNDERFLOW
#define EXCEPTION_INT_DIVIDE_BY_ZERO    STATUS_INTEGER_DIVIDE_BY_ZERO
#define EXCEPTION_INT_OVERFLOW          STATUS_INTEGER_OVERFLOW
#define EXCEPTION_PRIV_INSTRUCTION      STATUS_PRIVILEGED_INSTRUCTION
#define EXCEPTION_IN_PAGE_ERROR         STATUS_IN_PAGE_ERROR

/*
 * return values from the actual exception handlers
 */

#define ExceptionContinueExecution 0
#define ExceptionContinueSearch    1
#define ExceptionNestedException   2
#define ExceptionCollidedUnwind    3
 
/*
 * return values from filters in except() and from UnhandledExceptionFilter
 */
 
#define EXCEPTION_EXECUTE_HANDLER        1
#define EXCEPTION_CONTINUE_SEARCH        0
#define EXCEPTION_CONTINUE_EXECUTION    -1

/*
 * from OS/2 2.0 exception handling
 * Win32 seems to use the same flags as ExceptionFlags in an EXCEPTION_RECORD
 */

#define EH_NONCONTINUABLE   0x01
#define EH_UNWINDING        0x02
#define EH_EXIT_UNWIND      0x04
#define EH_STACK_INVALID    0x08
#define EH_NESTED_CALL      0x10

#define EXCEPTION_CONTINUABLE        0
#define EXCEPTION_NONCONTINUABLE     EH_NONCONTINUABLE
 
/* 
 * data types
 */

/*
 * The i386 context used by Win32 for almost everything. 
 */
  
#define SIZE_OF_80387_REGISTERS      80

typedef struct _FLOATING_SAVE_AREA
{
    DWORD   ControlWord;
    DWORD   StatusWord;
    DWORD   TagWord;    
    DWORD   ErrorOffset;
    DWORD   ErrorSelector;
    DWORD   DataOffset;
    DWORD   DataSelector;    
    BYTE    RegisterArea[SIZE_OF_80387_REGISTERS];
    DWORD   Cr0NpxState;
} FLOATING_SAVE_AREA;

typedef struct __CONTEXT
{
    DWORD   ContextFlags;
    DWORD   Dr0;
    DWORD   Dr1;    
    DWORD   Dr2;    
    DWORD   Dr3;
    DWORD   Dr6;
    DWORD   Dr7;    
    FLOATING_SAVE_AREA FloatSave;
    DWORD   SegGs;
    DWORD   SegFs;
    DWORD   SegEs;
    DWORD   SegDs;    
    DWORD   Edi;
    DWORD   Esi;
    DWORD   Ebx;
    DWORD   Edx;    
    DWORD   Ecx;
    DWORD   Eax;
    DWORD   Ebp;    
    DWORD   Eip;
    DWORD   SegCs;
    DWORD   EFlags;
    DWORD   Esp;
    DWORD   SegSs;
} CONTEXT;

typedef struct __CONTEXT *PCONTEXT;

/*
 * The exception record used by Win32 to give additional information 
 * about exception to exception handlers.
 */

#define EXCEPTION_MAXIMUM_PARAMETERS 15

typedef struct __EXCEPTION_RECORD
{
    DWORD    ExceptionCode;
    DWORD    ExceptionFlags;
    struct __EXCEPTION_RECORD *ExceptionRecord;

    PVOID    ExceptionAddress;
    DWORD    NumberParameters;
    DWORD    ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD;

typedef struct __EXCEPTION_RECORD_MIN
{
    DWORD    ExceptionCode;
    DWORD    ExceptionFlags;
    struct __EXCEPTION_RECORD *ExceptionRecord;

    PVOID    ExceptionAddress;
    DWORD    NumberParameters;
    DWORD    ExceptionInformation[0];
} EXCEPTION_RECORD_MIN;

typedef struct __EXCEPTION_RECORD *PEXCEPTION_RECORD;

/*
 * The exception pointers structure passed to exception filters
 * in except() and the UnhandledExceptionFilter().
 */
 
typedef struct __EXCEPTION_POINTERS 
{
  PEXCEPTION_RECORD  ExceptionRecord;
  PCONTEXT           ContextRecord;
} EXCEPTION_POINTERS;

typedef struct __EXCEPTION_POINTERS *PEXCEPTION_POINTERS;

/*
 * the function pointer to a exception handler
 */

/* forward definition */
struct __EXCEPTION_FRAME;

typedef DWORD ( *PEXCEPTION_HANDLER)( PEXCEPTION_RECORD          pexcrec,
                                      struct __EXCEPTION_FRAME  *pestframe,
                                      PCONTEXT                   pcontext,
                                      PVOID                      pdispatcher);

/*
 * function pointer to a UnhandledExceptionFilter();
 */

typedef long (WINAPI *__PTOP_EXCFILTER)
                   (PEXCEPTION_POINTERS ExceptionInfo);

typedef __PTOP_EXCFILTER PTOP_LEVER_EXCEPTION_FILTER;
typedef __PTOP_EXCFILTER LPTOP_LEVEL_EXCEPTION_FILTER;
                                 
/*
 * The exception frame, used for registering exception handlers 
 * Win32 cares only about this, but compilers generally emit 
 * larger exception frames for their own use.
 */

typedef struct __EXCEPTION_FRAME
{
  struct __EXCEPTION_FRAME *Prev;
  PEXCEPTION_HANDLER       Handler;
} EXCEPTION_FRAME;

typedef struct __EXCEPTION_FRAME *PEXCEPTION_FRAME;

extern PEXCEPTION_FRAME  TebExceptionFrame asm("%fs:0");

#define EXC_GetFrame()   TebExceptionFrame
#define EXC_SetFrame(a) (TebExceptionFrame=(a))

/*
 * Function definitions  
 */
 
void EXC_RaiseException(DWORD exccode, DWORD excflags, 
                        DWORD nargs, const LPDWORD pargs,
                        PCONTEXT pcontext); 
                        
void EXC_RtlUnwind( PEXCEPTION_FRAME pestframe,
                    LPVOID unusedEIP,
                    PEXCEPTION_RECORD pexcrec,
                    DWORD contextEAX,
                    PCONTEXT pcontext );
                    
DWORD EXC_CallUnhandledExceptionFilter( PEXCEPTION_RECORD precord,
                                        PCONTEXT          pcontext);

void EXC_Init(void);

BOOL WINAPI RaiseException(DWORD exccode, DWORD excflags,
                           DWORD nargs, const LPDWORD pargs);

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

BOOL  WINAPI RtlUnwind( PEXCEPTION_FRAME pestframe,
                        LPVOID unusedEIP,
                        PEXCEPTION_RECORD pexcrec,
                        DWORD contextEAX );

DWORD WINAPI UnhandledExceptionFilter(PEXCEPTION_POINTERS epointers);

__PTOP_EXCFILTER WINAPI SetUnhandledExceptionFilter(
      LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
  
#endif  /* __WINE_EXCEPT_H */
