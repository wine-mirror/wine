/*
 * Register definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_REGISTERS_H
#define __WINE_REGISTERS_H

#include "wintypes.h"

#ifndef WINELIB

#ifdef i386
extern int runtime_cpu (void);
#else
static inline int runtime_cpu(void) { return 3; }
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
#define WINE_DATA_SELECTOR 0x2b
#define WINE_CODE_SELECTOR 0x23
#endif  /* linux */

#ifdef __NetBSD__
#include <signal.h>
typedef struct sigcontext SIGCONTEXT;
#define WINE_DATA_SELECTOR 0x1f
#define WINE_CODE_SELECTOR 0x17
#endif  /* NetBSD */

#if defined(__svr4__) || defined(_SCO_DS)
#include <signal.h>
#ifdef _SCO_DS
#include <sys/regset.h>
#endif
#include <sys/ucontext.h>
typedef struct ucontext SIGCONTEXT;
#define WINE_DATA_SELECTOR 0x1f
#define WINE_CODE_SELECTOR 0x17
#endif  /* svr4 || SCO_DS */

#ifdef __FreeBSD__
#include <signal.h>
typedef struct sigcontext SIGCONTEXT;
#define WINE_DATA_SELECTOR 0x27
#define WINE_CODE_SELECTOR 0x1f
#endif  /* FreeBSD */

#if !defined(__svr4__) && !defined(_SCO_DS)

#define EAX_reg(context)     ((context)->sc_eax)
#define EBX_reg(context)     ((context)->sc_ebx)
#define ECX_reg(context)     ((context)->sc_ecx)
#define EDX_reg(context)     ((context)->sc_edx)
#define ESI_reg(context)     ((context)->sc_esi)
#define EDI_reg(context)     ((context)->sc_edi)
#define EBP_reg(context)     ((context)->sc_ebp)
                            
#define AX_reg(context)      (*(WORD*)&((context)->sc_eax))
#define BX_reg(context)      (*(WORD*)&((context)->sc_ebx))
#define CX_reg(context)      (*(WORD*)&((context)->sc_ecx))
#define DX_reg(context)      (*(WORD*)&((context)->sc_edx))
#define SI_reg(context)      (*(WORD*)&((context)->sc_esi))
#define DI_reg(context)      (*(WORD*)&((context)->sc_edi))
#define BP_reg(context)      (*(WORD*)&((context)->sc_ebp))
                            
#define AL_reg(context)      (*(BYTE*)(&(context)->sc_eax))
#define AH_reg(context)      (*(((BYTE*)(&(context)->sc_eax)+1)))
#define BL_reg(context)      (*(BYTE*)(&(context)->sc_ebx))
#define BH_reg(context)      (*(((BYTE*)(&(context)->sc_ebx)+1)))
#define CL_reg(context)      (*(BYTE*)(&(context)->sc_ecx))
#define CH_reg(context)      (*(((BYTE*)(&(context)->sc_ecx)+1)))
#define DL_reg(context)      (*(BYTE*)(&(context)->sc_edx))
#define DH_reg(context)      (*(((BYTE*)(&(context)->sc_edx)+1)))
                            
#define CS_reg(context)      ((context)->sc_cs)
#define DS_reg(context)      ((context)->sc_ds)
#define ES_reg(context)      ((context)->sc_es)
#define SS_reg(context)      ((context)->sc_ss)
                            
#ifdef linux
/* fs and gs are not supported on *BSD. Hopefully we won't need them. */
#define FS_reg(context)      ((context)->sc_fs)
#define GS_reg(context)      ((context)->sc_gs)
#endif
                            
#ifndef __FreeBSD__         
#define EFL_reg(context)     ((context)->sc_eflags)
#define FL_reg(context)      (*(WORD*)(&(context)->sc_eflags))
#else                       
#define EFL_reg(context)     ((context)->sc_efl)
#define FL_reg(context)      (*(WORD*)(&(context)->sc_efl))
#endif                      
                            
#define EIP_reg(context)     ((context)->sc_eip)
#define ESP_reg(context)     ((context)->sc_esp)
                            
#define IP_reg(context)      (*(WORD*)(&(context)->sc_eip))
#define SP_reg(context)      (*(WORD*)(&(context)->sc_esp))
                            
#else  /* __svr4__ || _SCO_DS */

#ifdef _SCO_DS
#define gregs regs
#endif

#define EAX_reg(context)      ((context)->uc_mcontext.gregs[EAX])
#define EBX_reg(context)      ((context)->uc_mcontext.gregs[EBX])
#define ECX_reg(context)      ((context)->uc_mcontext.gregs[ECX])
#define EDX_reg(context)      ((context)->uc_mcontext.gregs[EDX])
#define ESI_reg(context)      ((context)->uc_mcontext.gregs[ESI])
#define EDI_reg(context)      ((context)->uc_mcontext.gregs[EDI])
#define EBP_reg(context)      ((context)->uc_mcontext.gregs[EBP])
                            
#define AX_reg(context)      (*(WORD*)&((context)->uc_mcontext.gregs[EAX]))
#define BX_reg(context)      (*(WORD*)&((context)->uc_mcontext.gregs[EBX]))
#define CX_reg(context)      (*(WORD*)&((context)->uc_mcontext.gregs[ECX]))
#define DX_reg(context)      (*(WORD*)&((context)->uc_mcontext.gregs[EDX]))
#define SI_reg(context)      (*(WORD*)&((context)->uc_mcontext.gregs[ESI]))
#define DI_reg(context)      (*(WORD*)&((context)->uc_mcontext.gregs[EDI]))
#define BP_reg(context)      (*(WORD*)&((context)->uc_mcontext.gregs[EBP]))
                            
#define AL_reg(context)      (*(BYTE*)(&(context)->uc_mcontext.gregs[EAX]))
#define AH_reg(context)      (*(((BYTE*)(&(context)->uc_mcontext.gregs[EAX])+1)))
#define BL_reg(context)      (*(BYTE*)(&(context)->uc_mcontext.gregs[EBX]))
#define BH_reg(context)      (*(((BYTE*)(&(context)->uc_mcontext.gregs[EBX])+1)))
#define CL_reg(context)      (*(BYTE*)(&(context)->uc_mcontext.gregs[ECX]))
#define CH_reg(context)      (*(((BYTE*)(&(context)->uc_mcontext.gregs[ECX])+1)))
#define DL_reg(context)      (*(BYTE*)(&(context)->uc_mcontext.gregs[EDX]))
#define DH_reg(context)      (*(((BYTE*)(&(context)->uc_mcontext.gregs[EDX])+1)))
                            
#define CS_reg(context)      ((context)->uc_mcontext.gregs[CS])
#define DS_reg(context)      ((context)->uc_mcontext.gregs[DS])
#define ES_reg(context)      ((context)->uc_mcontext.gregs[ES])
#define SS_reg(context)      ((context)->uc_mcontext.gregs[SS])
                            
#define FS_reg(context)      ((context)->uc_mcontext.gregs[FS])
#define GS_reg(context)      ((context)->uc_mcontext.gregs[GS])

                            
#define EFL_reg(context)     ((context)->uc_mcontext.gregs[EFL])
#define FL_reg(context)      (*(WORD*)(&(context)->uc_mcontext.gregs[EFL]))

                            
#define EIP_reg(context)      ((context)->uc_mcontext.gregs[EIP])
#ifdef R_ESP
#define ESP_reg(context)     ((context)->uc_mcontext.gregs[R_ESP])
#else
#define ESP_reg(context)     ((context)->uc_mcontext.gregs[ESP])
#endif
                            
#define IP_reg(context)      (*(WORD*)(&(context)->uc_mcontext.gregs[EIP]))
#ifdef R_ESP
#define SP_reg(context)      (*(WORD*)(&(context)->uc_mcontext.gregs[R_ESP]))
#else
#define SP_reg(context)      (*(WORD*)(&(context)->uc_mcontext.gregs[ESP]))
#endif
                            
#endif  /* __svr4__ || _SCO_DS */

#define SET_CFLAG(context)   (EFL_reg(context) |= 0x0001)
#define RESET_CFLAG(context) (EFL_reg(context) &= 0xfffffffe)

#else  /* ifndef WINELIB */

typedef void SIGCONTEXT;
#define WINE_DATA_SELECTOR 0x00
#define WINE_CODE_SELECTOR 0x00

#endif  /* ifndef WINELIB */

#endif /* __WINE_REGISTERS_H */
