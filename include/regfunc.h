/* $Id$
 */

#ifndef REGFUNC_H
#define REGFUNC_H

#include "wine.h"

extern unsigned short *Stack16Frame;

#define _CONTEXT ((struct sigcontext_struct *) &Stack16Frame[12])
#define _AX	(_CONTEXT->sc_eax)
#define _BX	(_CONTEXT->sc_ebx)
#define _CX	(_CONTEXT->sc_ecx)
#define _DX	(_CONTEXT->sc_edx)
#define _SP	(_CONTEXT->sc_esp)
#define _BP	(_CONTEXT->sc_ebp)
#define _SI	(_CONTEXT->sc_esi)
#define _DI	(_CONTEXT->sc_edi)
#define _DS	(_CONTEXT->sc_ds)
#define _ES	(_CONTEXT->sc_es)

extern void ReturnFromRegisterFunc(void);

#endif /* REGFUNC_H */
