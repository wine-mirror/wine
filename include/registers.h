#ifndef __WINE_REGISTERS_H
#define __WINE_REGISTERS_H

#include <windows.h>
#include "autoconf.h"

#ifndef PROCEMU

#define EAX context->sc_eax
#define EBX context->sc_ebx
#define ECX context->sc_ecx
#define EDX context->sc_edx

#define AX *(WORD*)&context->sc_eax
#define BX *(WORD*)&context->sc_ebx
#define CX *(WORD*)&context->sc_ecx
#define DX *(WORD*)&context->sc_edx

#define AL *(BYTE*)&context->sc_eax
#define AH *(((BYTE*)&context->sc_eax)+1)
#define BL *(BYTE*)&context->sc_ebx
#define BH *(((BYTE*)&context->sc_ebx)+1)
#define CL *(BYTE*)&context->sc_ecx
#define CH *(((BYTE*)&context->sc_ecx)+1)
#define DL *(BYTE*)&context->sc_edx
#define DH *(((BYTE*)&context->sc_edx)+1)

#define CS context->sc_cs
#define DS context->sc_ds
#define ES context->sc_es
#define SS context->sc_ss

#define DI context->sc_edi
#define SI context->sc_esi
#define SP context->sc_esp
#define EFL context->sc_efl
#define EIP context->sc_eip

#define SetCflag	(EFL |= 0x00000001)
#define ResetCflag	(EFL &= 0xfffffffe)

#else

#include "bx_bochs.h"

#define SetCflag	bx_STC()
#define ResetCflag	bx_CLC()

#endif /* PROCEMU */
#endif /* __WINE_REGISTERS_H */
