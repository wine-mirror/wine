/*
 * Signal context definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_SIGCONTEXT_H
#define __WINE_SIGCONTEXT_H

#ifdef WINELIB
#error This file must not be used in Winelib
#endif

#ifdef linux
typedef struct
{
    unsigned short sc_gs, __gsh;
    unsigned short sc_fs, __fsh;
    unsigned short sc_es, __esh;
    unsigned short sc_ds, __dsh;
    unsigned long sc_edi;
    unsigned long sc_esi;
    unsigned long sc_ebp;
    unsigned long sc_esp;
    unsigned long sc_ebx;
    unsigned long sc_edx;
    unsigned long sc_ecx;
    unsigned long sc_eax;
    unsigned long sc_trapno;
    unsigned long sc_err;
    unsigned long sc_eip;
    unsigned short sc_cs, __csh;
    unsigned long sc_eflags;
    unsigned long esp_at_signal;
    unsigned short sc_ss, __ssh;
    unsigned long i387;
    unsigned long oldmask;
    unsigned long cr2;
} SIGCONTEXT;
#endif  /* linux */

#ifdef __NetBSD__
#include <signal.h>
typedef struct sigcontext SIGCONTEXT;
#endif  /* NetBSD */

#if defined(__svr4__) || defined(_SCO_DS)
#include <signal.h>
#ifdef _SCO_DS
#include <sys/regset.h>
#endif
#include <sys/ucontext.h>
typedef struct ucontext SIGCONTEXT;
#endif  /* svr4 || SCO_DS */

#ifdef __FreeBSD__
#include <signal.h>
typedef struct sigcontext SIGCONTEXT;
#endif  /* FreeBSD */

#if !defined(__svr4__) && !defined(_SCO_DS)

#define EAX_sig(context)     ((context)->sc_eax)
#define EBX_sig(context)     ((context)->sc_ebx)
#define ECX_sig(context)     ((context)->sc_ecx)
#define EDX_sig(context)     ((context)->sc_edx)
#define ESI_sig(context)     ((context)->sc_esi)
#define EDI_sig(context)     ((context)->sc_edi)
#define EBP_sig(context)     ((context)->sc_ebp)
                            
#define CS_sig(context)      ((context)->sc_cs)
#define DS_sig(context)      ((context)->sc_ds)
#define ES_sig(context)      ((context)->sc_es)
#define SS_sig(context)      ((context)->sc_ss)
                            
#ifdef linux
/* fs and gs are not supported on *BSD. Hopefully we won't need them. */
#define FS_sig(context)      ((context)->sc_fs)
#define GS_sig(context)      ((context)->sc_gs)
#endif
                            
#ifndef __FreeBSD__         
#define EFL_sig(context)     ((context)->sc_eflags)
#else                       
#define EFL_sig(context)     ((context)->sc_efl)
#endif                      
                            
#define EIP_sig(context)     ((context)->sc_eip)
#define ESP_sig(context)     ((context)->sc_esp)

#else  /* __svr4__ || _SCO_DS */

#ifdef _SCO_DS
#define gregs regs
#endif

#define EAX_sig(context)     ((context)->uc_mcontext.gregs[EAX])
#define EBX_sig(context)     ((context)->uc_mcontext.gregs[EBX])
#define ECX_sig(context)     ((context)->uc_mcontext.gregs[ECX])
#define EDX_sig(context)     ((context)->uc_mcontext.gregs[EDX])
#define ESI_sig(context)     ((context)->uc_mcontext.gregs[ESI])
#define EDI_sig(context)     ((context)->uc_mcontext.gregs[EDI])
#define EBP_sig(context)     ((context)->uc_mcontext.gregs[EBP])
                            
#define CS_sig(context)      ((context)->uc_mcontext.gregs[CS])
#define DS_sig(context)      ((context)->uc_mcontext.gregs[DS])
#define ES_sig(context)      ((context)->uc_mcontext.gregs[ES])
#define SS_sig(context)      ((context)->uc_mcontext.gregs[SS])
                            
#define FS_sig(context)      ((context)->uc_mcontext.gregs[FS])
#define GS_sig(context)      ((context)->uc_mcontext.gregs[GS])

#define EFL_sig(context)     ((context)->uc_mcontext.gregs[EFL])
                            
#define EIP_sig(context)     ((context)->uc_mcontext.gregs[EIP])
#ifdef R_ESP
#define ESP_sig(context)     ((context)->uc_mcontext.gregs[R_ESP])
#else
#define ESP_sig(context)     ((context)->uc_mcontext.gregs[ESP])
#endif
                            
#endif  /* __svr4__ || _SCO_DS */

#define AX_sig(context)      (*(WORD*)&EAX_sig(context))
#define BX_sig(context)      (*(WORD*)&EBX_sig(context))
#define CX_sig(context)      (*(WORD*)&ECX_sig(context))
#define DX_sig(context)      (*(WORD*)&EDX_sig(context))
#define SI_sig(context)      (*(WORD*)&ESI_sig(context))
#define DI_sig(context)      (*(WORD*)&EDI_sig(context))
#define BP_sig(context)      (*(WORD*)&EBP_sig(context))

#define AL_sig(context)      (*(BYTE*)&EAX_sig(context))
#define AH_sig(context)      (*((BYTE*)&EAX_sig(context)+1))
#define BL_sig(context)      (*(BYTE*)&EBX_sig(context))
#define BH_sig(context)      (*((BYTE*)&EBX_sig(context)+1))
#define CL_sig(context)      (*(BYTE*)&ECX_sig(context))
#define CH_sig(context)      (*((BYTE*)&ECX_sig(context)+1))
#define DL_sig(context)      (*(BYTE*)&EDX_sig(context))
#define DH_sig(context)      (*((BYTE*)&EDX_sig(context)+1))
                            
#define IP_sig(context)      (*(WORD*)&EIP_sig(context))
#define SP_sig(context)      (*(WORD*)&ESP_sig(context))
                            
#define FL_sig(context)      (*(WORD*)&EFL_sig(context))

#endif /* __WINE_SIGCONTEXT_H */
