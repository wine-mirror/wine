/*
 * Register definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_REGISTERS_H
#define __WINE_REGISTERS_H

#include <windows.h>
#include "wine.h"

#ifndef __svr4__

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
                            
#define SET_CFLAG(context)   (EFL_reg(context) |= 0x0001)
#define RESET_CFLAG(context) (EFL_reg(context) &= 0xfffffffe)

#else  /* __svr4__ */

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
#define ESP_reg(context)     ((context)->uc_mcontext.gregs[R_ESP])
                            
#define IP_reg(context)      (*(WORD*)(&(context)->uc_mcontext.gregs[EIP]))
#define SP_reg(context)      (*(WORD*)(&(context)->uc_mcontext.gregs[R_ESP]))
                            
#define SET_CFLAG(context)   (EFL_reg(context) |= 0x0001)
#define RESET_CFLAG(context) (EFL_reg(context) &= 0xfffffffe)

#endif  /* __svr4__ */

#endif /* __WINE_REGISTERS_H */
