/*
 * Win32 definitions for Windows NT
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_WINNT_H
#define __WINE_WINNT_H

#include "wintypes.h"

/* Heap flags */

#define HEAP_NO_SERIALIZE               0x00000001
#define HEAP_GROWABLE                   0x00000002
#define HEAP_GENERATE_EXCEPTIONS        0x00000004
#define HEAP_ZERO_MEMORY                0x00000008
#define HEAP_REALLOC_IN_PLACE_ONLY      0x00000010
#define HEAP_TAIL_CHECKING_ENABLED      0x00000020
#define HEAP_FREE_CHECKING_ENABLED      0x00000040
#define HEAP_DISABLE_COALESCE_ON_FREE   0x00000080
#define HEAP_CREATE_ALIGN_16            0x00010000
#define HEAP_CREATE_ENABLE_TRACING      0x00020000
#define HEAP_WINE_SEGPTR                0x01000000  /* Not a Win32 flag */
#define HEAP_WINE_CODESEG               0x02000000  /* Not a Win32 flag */

/* Processor feature flags.  */
#define PF_FLOATING_POINT_PRECISION_ERRATA	0
#define PF_FLOATING_POINT_EMULATED		1
#define PF_COMPARE_EXCHANGE_DOUBLE		2
#define PF_MMX_INSTRUCTIONS_AVAILABLE		3
#define PF_PPC_MOVEMEM_64BIT_OK			4
#define PF_ALPHA_BYTE_INSTRUCTIONS		5


/* The Win32 register context */

#define CONTEXT_i386      0x00010000
#define CONTEXT_i486      CONTEXT_i386
#define CONTEXT_CONTROL   (CONTEXT_i386 | 0x0001) /* SS:SP, CS:IP, FLAGS, BP */
#define CONTEXT_INTEGER   (CONTEXT_i386 | 0x0002) /* AX, BX, CX, DX, SI, DI */
#define CONTEXT_SEGMENTS  (CONTEXT_i386 | 0x0004) /* DS, ES, FS, GS */
#define CONTEXT_FLOATING_POINT  (CONTEXT_i386 | 0x0008L) /* 387 state */
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_i386 | 0x0010L) /* DB 0-3,6,7 */
#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS)

#define SIZE_OF_80387_REGISTERS      80

typedef struct
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

typedef struct
{
    DWORD   ContextFlags;

    /* These are selected by CONTEXT_DEBUG_REGISTERS */
    DWORD   Dr0;
    DWORD   Dr1;
    DWORD   Dr2;
    DWORD   Dr3;
    DWORD   Dr6;
    DWORD   Dr7;

    /* These are selected by CONTEXT_FLOATING_POINT */
    FLOATING_SAVE_AREA FloatSave;

    /* These are selected by CONTEXT_SEGMENTS */
    DWORD   SegGs;
    DWORD   SegFs;
    DWORD   SegEs;
    DWORD   SegDs;    

    /* These are selected by CONTEXT_INTEGER */
    DWORD   Edi;
    DWORD   Esi;
    DWORD   Ebx;
    DWORD   Edx;    
    DWORD   Ecx;
    DWORD   Eax;

    /* These are selected by CONTEXT_CONTROL */
    DWORD   Ebp;    
    DWORD   Eip;
    DWORD   SegCs;
    DWORD   EFlags;
    DWORD   Esp;
    DWORD   SegSs;
} CONTEXT, *PCONTEXT;


#ifdef __WINE__

/* Macros for easier access to context registers */

#define EAX_reg(context)     ((context)->Eax)
#define EBX_reg(context)     ((context)->Ebx)
#define ECX_reg(context)     ((context)->Ecx)
#define EDX_reg(context)     ((context)->Edx)
#define ESI_reg(context)     ((context)->Esi)
#define EDI_reg(context)     ((context)->Edi)
#define EBP_reg(context)     ((context)->Ebp)

#define CS_reg(context)      ((context)->SegCs)
#define DS_reg(context)      ((context)->SegDs)
#define ES_reg(context)      ((context)->SegEs)
#define FS_reg(context)      ((context)->SegFs)
#define GS_reg(context)      ((context)->SegGs)
#define SS_reg(context)      ((context)->SegSs)

#define EFL_reg(context)     ((context)->EFlags)
#define EIP_reg(context)     ((context)->Eip)
#define ESP_reg(context)     ((context)->Esp)

#define AX_reg(context)      (*(WORD*)&EAX_reg(context))
#define BX_reg(context)      (*(WORD*)&EBX_reg(context))
#define CX_reg(context)      (*(WORD*)&ECX_reg(context))
#define DX_reg(context)      (*(WORD*)&EDX_reg(context))
#define SI_reg(context)      (*(WORD*)&ESI_reg(context))
#define DI_reg(context)      (*(WORD*)&EDI_reg(context))
#define BP_reg(context)      (*(WORD*)&EBP_reg(context))

#define AL_reg(context)      (*(BYTE*)&EAX_reg(context))
#define AH_reg(context)      (*((BYTE*)&EAX_reg(context)+1))
#define BL_reg(context)      (*(BYTE*)&EBX_reg(context))
#define BH_reg(context)      (*((BYTE*)&EBX_reg(context)+1))
#define CL_reg(context)      (*(BYTE*)&ECX_reg(context))
#define CH_reg(context)      (*((BYTE*)&ECX_reg(context)+1))
#define DL_reg(context)      (*(BYTE*)&EDX_reg(context))
#define DH_reg(context)      (*((BYTE*)&EDX_reg(context)+1))
                            
#define IP_reg(context)      (*(WORD*)&EIP_reg(context))
#define SP_reg(context)      (*(WORD*)&ESP_reg(context))
                            
#define FL_reg(context)      (*(WORD*)&EFL_reg(context))

#define SET_CFLAG(context)   (EFL_reg(context) |= 0x0001)
#define RESET_CFLAG(context) (EFL_reg(context) &= 0xfffffffe)

#endif  /* __WINE__ */

/*
 * Exception codes
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

#define MAXIMUM_WAIT_OBJECTS 64
#define MAXIMUM_SUSPEND_COUNT 127


/*
 * Return values from the actual exception handlers
 */

#define ExceptionContinueExecution 0
#define ExceptionContinueSearch    1
#define ExceptionNestedException   2
#define ExceptionCollidedUnwind    3
 
/*
 * Return values from filters in except() and from UnhandledExceptionFilter
 */
 
#define EXCEPTION_EXECUTE_HANDLER        1
#define EXCEPTION_CONTINUE_SEARCH        0
#define EXCEPTION_CONTINUE_EXECUTION    -1

/*
 * From OS/2 2.0 exception handling
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
 * The exception record used by Win32 to give additional information 
 * about exception to exception handlers.
 */

#define EXCEPTION_MAXIMUM_PARAMETERS 15

typedef struct __EXCEPTION_RECORD
{
    DWORD    ExceptionCode;
    DWORD    ExceptionFlags;
    struct __EXCEPTION_RECORD *ExceptionRecord;

    LPVOID   ExceptionAddress;
    DWORD    NumberParameters;
    DWORD    ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD;

/*
 * The exception pointers structure passed to exception filters
 * in except() and the UnhandledExceptionFilter().
 */
 
typedef struct _EXCEPTION_POINTERS 
{
  PEXCEPTION_RECORD  ExceptionRecord;
  PCONTEXT           ContextRecord;
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;

/*
 * function pointer to a exception filter
 */

typedef LONG (CALLBACK *PTOP_LEVEL_EXCEPTION_FILTER)(PEXCEPTION_POINTERS ExceptionInfo);
typedef PTOP_LEVEL_EXCEPTION_FILTER LPTOP_LEVEL_EXCEPTION_FILTER;

DWORD WINAPI UnhandledExceptionFilter( PEXCEPTION_POINTERS epointers );
LPTOP_LEVEL_EXCEPTION_FILTER
WINAPI SetUnhandledExceptionFilter( LPTOP_LEVEL_EXCEPTION_FILTER filter );

/* Language IDs */

#define LANG_NEUTRAL                     0x00
#define LANG_ARABIC                      0x01
#define LANG_AFRIKAANS                   0x36
#define LANG_ALBANIAN                    0x1c
#define LANG_BASQUE                      0x2d
#define LANG_BULGARIAN                   0x02
#define LANG_BYELORUSSIAN                0x23
#define LANG_CATALAN                     0x03
#define LANG_CHINESE                     0x04
#define LANG_CROATIAN                    0x1a
#define LANG_CZECH                       0x05
#define LANG_DANISH                      0x06
#define LANG_DUTCH                       0x13
#define LANG_ENGLISH                     0x09
#define LANG_ESTONIAN                    0x25
#define LANG_FAEROESE                    0x38
#define LANG_FARSI                       0x29
#define LANG_FINNISH                     0x0b
#define LANG_FRENCH                      0x0c
#define LANG_GERMAN                      0x07
#define LANG_GREEK                       0x08
#define LANG_HEBREW                      0x0D
#define LANG_HUNGARIAN                   0x0e
#define LANG_ICELANDIC                   0x0f
#define LANG_INDONESIAN                  0x21
#define LANG_ITALIAN                     0x10
#define LANG_JAPANESE                    0x11
#define LANG_KOREAN                      0x12
#define LANG_LATVIAN                     0x26
#define LANG_LITHUANIAN                  0x27
#define LANG_NORWEGIAN                   0x14
#define LANG_POLISH                      0x15
#define LANG_PORTUGUESE                  0x16
#define LANG_ROMANIAN                    0x18
#define LANG_RUSSIAN                     0x19
#define LANG_SLOVAK                      0x1b
#define LANG_SLOVENIAN                   0x24
#define LANG_SORBIAN                     0x2e
#define LANG_SPANISH                     0x0a
#define LANG_SWEDISH                     0x1d
#define LANG_THAI                        0x1e
#define LANG_TURKISH                     0x1f
#define LANG_UKRAINIAN                   0x22

#endif  /* __WINE_WINNT_H */
