/*
 * Win32 exception assembly functions
 *
 * Copyright (c) 1996 Onno Hovers, (onno@stack.urc.tue.nl)
 *
 */

#ifndef __ELF__
     .globl _EXC_CallUnhandledExceptionFilter
     .extern _pTopExcHandler

 _EXC_CallUnhandledExceptionFilter:
#else  /* __ELF__ */
     .globl EXC_CallUnhandledExceptionFilter
     .extern pTopExcHandler

 EXC_CallUnhandledExceptionFilter:
#endif  /* __ELF__ */
     leal   4(%esp),%eax
     pushl  %eax
#ifndef __ELF__
     call   *_pTopExcHandler
#else  /* __ELF__ */
     call   *pTopExcHandler
#endif  /* __ELF__ */
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
       .equ CONTEXT_SegSs,        -4
       .equ CONTEXT_Esp,          -8
       .equ CONTEXT_EFlags,       -12
       .equ CONTEXT_SegCs,        -16
       .equ CONTEXT_Eip,          -20
       .equ CONTEXT_Ebp,          -24
       .equ CONTEXT_Eax,          -28
       .equ CONTEXT_Ecx,          -32
       .equ CONTEXT_Edx,          -36
       .equ CONTEXT_Ebx,          -40
       .equ CONTEXT_Esi,          -44
       .equ CONTEXT_Edi,          -48
       .equ CONTEXT_SegDs,        -52
       .equ CONTEXT_SegEs,        -56
       .equ CONTEXT_SegFs,        -60
       .equ CONTEXT_SegGs,        -64
       .equ FLOAT_Cr0NpxState,    -68
       .equ FLOAT_RegisterArea,   -148
       .equ FLOAT_DataSelector,   -152
       .equ FLOAT_DataOffset,     -156
       .equ FLOAT_ErrorSelector,  -160
       .equ FLOAT_ErrorOffset,    -164
       .equ FLOAT_TagWord,        -168
       .equ FLOAT_StatusWord,     -172
       .equ FLOAT_ControlWord,    -176
       .equ CONTEXT_FloatSave,    -176
       .equ CONTEXT_Dr7,          -180
       .equ CONTEXT_Dr6,          -184
       .equ CONTEXT_Dr3,          -188
       .equ CONTEXT_Dr2,          -192
       .equ CONTEXT_Dr1,          -196
       .equ CONTEXT_Dr0,          -200
       .equ CONTEXT_ContextFlags, -204
       .equ CONTEXT,              -204
       .equ CONTEXTSIZE,           204
       .equ CONTEXTFLAGS,          0x10007

       .equ ORIG_ESP,              16         /** cdecl !!! **/
       .equ PARM_ARG4,             28
       .equ PARM_ARG3,             24
       .equ PARM_ARG2,             20
       .equ PARM_ARG1,             16
       .equ PARM_RETURN,           12
       .equ PARM_CALLFUNC,         8
       .equ PARM_EBP,              4
       .equ PARM_EFLAGS,           0

#ifndef __ELF__
	.globl _RaiseException
        .extern _EXC_RaiseException

 _RaiseException:
       push   $_EXC_RaiseException
       jmp    ContextCall

       .globl _RtlUnwind

 _RtlUnwind:
       push   $_EXC_RtlUnwind

#else  /* __ELF__ */
       .globl RaiseException
       .extern EXC_RaiseException

 RaiseException:
       push   $EXC_RaiseException
       jmp    ContextCall

       .globl RtlUnwind

 RtlUnwind:
       push   $EXC_RtlUnwind
#endif  /* __ELF__ */

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
