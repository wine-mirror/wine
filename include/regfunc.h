/* $Id$
 */

#ifndef REGFUNC_H
#define REGFUNC_H

#include "wine.h"
#include "stackframe.h"

#define _CONTEXT ((struct sigcontext_struct *) CURRENT_STACK16->args)
#define _AX	(_CONTEXT->sc_eax)
#define _BX	(_CONTEXT->sc_ebx)
#define _CX	(_CONTEXT->sc_ecx)
#define _DX	(_CONTEXT->sc_edx)
#define _SI	(_CONTEXT->sc_esi)
#define _DI	(_CONTEXT->sc_edi)
#define _DS	(_CONTEXT->sc_ds)
#define _ES	(_CONTEXT->sc_es)

extern void ReturnFromRegisterFunc(void);

#endif /* REGFUNC_H */
