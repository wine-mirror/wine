/*
 * Win32 exception assembly functions
 *
 * Copyright (c) 1996 Onno Hovers, (onno@stack.urc.tue.nl)
 *
 */
#include "config.h"

#ifdef NEED_UNDERSCORE_PREFIX

     .globl _EXC_CallUnhandledExceptionFilter
 _EXC_CallUnhandledExceptionFilter:

#else

     .globl EXC_CallUnhandledExceptionFilter
 EXC_CallUnhandledExceptionFilter:

#endif  /* NEED_UNDERSCORE_PREFIX */
     leal   4(%esp),%eax
     pushl  %eax
#ifdef NEED_UNDERSCORE_PREFIX
     call   *_pTopExcHandler
#else
     call   *pTopExcHandler
#endif  /* NEED_UNDERSCORE_PREFIX */
     movl   %ebp,%esp
     ret



/*******************************************************************
 *
 *        RaiseException           (KERNEL32. 418 )
 *        RtlUnwind                (KERNEL32. 443 )
 *
 * we need to save our context before a call to
 *
 * -RaiseException
 * -RtlUnwind
 *
 * after these functions we need to restore that context structure as
 * the actual context so changes made to the context structure in an
 * exception-handler will be reflected in the context after these
 * functions return. Fortunately both functions have 4 DWORD params.
 * we pass the function to be called as a fifth parameter to ContextCall
 *
 */
#define CONTEXT_SegSs        -4
#define CONTEXT_Esp          -8
#define CONTEXT_EFlags       -12
#define CONTEXT_SegCs        -16
#define CONTEXT_Eip          -20
#define CONTEXT_Ebp          -24
#define CONTEXT_Eax          -28
#define CONTEXT_Ecx          -32
#define CONTEXT_Edx          -36
#define CONTEXT_Ebx          -40
#define CONTEXT_Esi          -44
#define CONTEXT_Edi          -48
#define CONTEXT_SegDs        -52
#define CONTEXT_SegEs        -56
#define CONTEXT_SegFs        -60
#define CONTEXT_SegGs        -64
#define FLOAT_Cr0NpxState    -68
#define FLOAT_RegisterArea   -148
#define FLOAT_DataSelector   -152
#define FLOAT_DataOffset     -156
#define FLOAT_ErrorSelector  -160
#define FLOAT_ErrorOffset    -164
#define FLOAT_TagWord        -168
#define FLOAT_StatusWord     -172
#define FLOAT_ControlWord    -176
#define CONTEXT_FloatSave    -176
#define CONTEXT_Dr7          -180
#define CONTEXT_Dr6          -184
#define CONTEXT_Dr3          -188
#define CONTEXT_Dr2          -192
#define CONTEXT_Dr1          -196
#define CONTEXT_Dr0          -200
#define CONTEXT_ContextFlags -204
#define CONTEXT              -204
#define CONTEXTSIZE           204
#define CONTEXTFLAGS          0x10007

#define ORIG_ESP              16         /** cdecl !!! **/
#define PARM_ARG4             28
#define PARM_ARG3             24
#define PARM_ARG2             20
#define PARM_ARG1             16
#define PARM_RETURN           12
#define PARM_CALLFUNC         8
#define PARM_EBP              4
#define PARM_EFLAGS           0

#ifdef NEED_UNDERSCORE_PREFIX
	.globl _RaiseException

 _RaiseException:
       push   $_EXC_RaiseException
       jmp    ContextCall

       .globl _RtlUnwind

 _RtlUnwind:
       push   $_EXC_RtlUnwind

#else  /* NEED_UNDERSCORE_PREFIX */
       .globl RaiseException

 RaiseException:
       push   $EXC_RaiseException
       jmp    ContextCall

       .globl RtlUnwind

 RtlUnwind:
       push   $EXC_RtlUnwind
#endif  /* NEED_UNDERSCORE_PREFIX */

 ContextCall:
       pushl  %ebp
       pushfl
       movl   %esp, %ebp
       subl   $CONTEXTSIZE,      %esp
       movl   %eax, CONTEXT_Eax(%ebp)
       leal   ORIG_ESP(%ebp), %eax
       movl   %eax, CONTEXT_Esp(%ebp)
       movl   PARM_EFLAGS(%ebp), %eax
       movl   %eax, CONTEXT_EFlags(%ebp)
       movl   PARM_EBP(%ebp), %eax
       movl   %eax, CONTEXT_Ebp(%ebp)
       movl   PARM_RETURN(%ebp), %eax
       movl   %eax, CONTEXT_Eip(%ebp)
       movl   %edi, CONTEXT_Edi(%ebp)
       movl   %esi, CONTEXT_Esi(%ebp)
       movl   %ebx, CONTEXT_Ebx(%ebp)
       movl   %edx, CONTEXT_Edx(%ebp)
       movl   %ecx, CONTEXT_Ecx(%ebp)
       xorl   %eax, %eax
       movw   %ss, %ax
       movl   %eax, CONTEXT_SegSs(%ebp)
       movw   %cs, %ax
       movl   %eax, CONTEXT_SegCs(%ebp)
       movw   %gs, %ax
       movl   %eax, CONTEXT_SegGs(%ebp)
       movw   %fs, %ax
       movl   %eax, CONTEXT_SegFs(%ebp)
       movw   %es, %ax
       movl   %eax, CONTEXT_SegEs(%ebp)
       movw   %ds, %ax
       movl   %eax, CONTEXT_SegDs(%ebp)
       fsave  CONTEXT_FloatSave(%ebp)
       movl   $CONTEXTFLAGS, %eax
       movl   %eax, CONTEXT_ContextFlags(%ebp)
       pushl  %ebp
       leal   CONTEXT(%ebp), %eax
       pushl  %eax
       pushl  PARM_ARG4(%ebp)
       pushl  PARM_ARG3(%ebp)
       pushl  PARM_ARG2(%ebp)
       pushl  PARM_ARG1(%ebp)
       call   *PARM_CALLFUNC(%ebp)
       addl   $20,%esp
       popl   %ebp
       lds    CONTEXT_Esp(%ebp),%edi
       movl   CONTEXT_Eip(%ebp),%eax
       movl   %eax,-4(%edi)
       movl   CONTEXT_EFlags(%ebp),%eax
       movl   %eax,-8(%edi)
       movl   CONTEXT_Edi(%ebp),%eax
       movl   %eax,-12(%edi)
       movl   CONTEXT_SegDs(%ebp),%eax
       movw   %ax,%ds
       movl   CONTEXT_SegEs(%ebp),%eax
       movw   %ax,%es
       movl   CONTEXT_SegFs(%ebp),%eax
       movw   %ax,%fs
       movl   CONTEXT_SegGs(%ebp),%eax
       movw   %ax,%gs
       frstor CONTEXT_FloatSave(%ebp)
       movl   CONTEXT_Ecx(%ebp),%ecx
       movl   CONTEXT_Edx(%ebp),%edx
       movl   CONTEXT_Ebx(%ebp),%ebx
       movl   CONTEXT_Esi(%ebp),%esi
       movl   CONTEXT_Eax(%ebp),%eax
       movl   CONTEXT_Ebp(%ebp),%ebp
       lea    -12(%edi),%esp
       popl   %edi
       popfl
       ret
