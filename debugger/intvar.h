/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/* Wine internal debugger
 * Definitionz for internal variables
 * Eric Pouech (c) 2000
 */

   /* break handling */
INTERNAL_VAR(BreakAllThreadsStartup,	FALSE,		NULL,  			DEBUG_TypeIntConst)
INTERNAL_VAR(BreakOnCritSectTimeOut,    FALSE,          NULL,  			DEBUG_TypeIntConst)

   /* output handling */
INTERNAL_VAR(ConChannelMask,		DBG_CHN_MESG,	NULL,  			DEBUG_TypeIntConst)
INTERNAL_VAR(StdChannelMask,		0,		NULL,  			DEBUG_TypeIntConst)
INTERNAL_VAR(UseXTerm,			TRUE,		NULL,  			DEBUG_TypeIntConst)

   /* debugging debugger */
INTERNAL_VAR(ExtDbgOnInvalidAddress,	FALSE,		NULL,  			DEBUG_TypeIntConst)

   /* context manipulation */
#ifdef __i386__
/* FIXME: 16 bit registers use imply that CPU is little endian, which is
 * the case when running natively i386 code
 */
INTERNAL_VAR(eip,			0,		&DEBUG_context.Eip,	DEBUG_TypeIntConst)
INTERNAL_VAR(ip,			0,		&DEBUG_context.Eip,  	DEBUG_TypeShortUInt)
INTERNAL_VAR(pc,			0,		&DEBUG_context.Eip,  	DEBUG_TypeIntConst)
INTERNAL_VAR(flags,			0,		&DEBUG_context.EFlags,  DEBUG_TypeIntConst)
INTERNAL_VAR(esp,			0,		&DEBUG_context.Esp,  	DEBUG_TypeIntConst)
INTERNAL_VAR(sp,			0,		&DEBUG_context.Esp,  	DEBUG_TypeShortUInt)
INTERNAL_VAR(eax,			0,		&DEBUG_context.Eax,  	DEBUG_TypeIntConst)
INTERNAL_VAR(ax,			0,		&DEBUG_context.Eax,  	DEBUG_TypeShortUInt)
INTERNAL_VAR(ebx,			0,		&DEBUG_context.Ebx,  	DEBUG_TypeIntConst)
INTERNAL_VAR(bx,			0,		&DEBUG_context.Ebx,  	DEBUG_TypeShortUInt)
INTERNAL_VAR(ecx,			0,		&DEBUG_context.Ecx,  	DEBUG_TypeIntConst)
INTERNAL_VAR(cx,			0,		&DEBUG_context.Ecx,  	DEBUG_TypeShortUInt)
INTERNAL_VAR(edx,			0,		&DEBUG_context.Edx,  	DEBUG_TypeIntConst)
INTERNAL_VAR(dx,			0,		&DEBUG_context.Edx,  	DEBUG_TypeShortUInt)
INTERNAL_VAR(esi,			0,		&DEBUG_context.Esi,  	DEBUG_TypeIntConst)
INTERNAL_VAR(si,			0,		&DEBUG_context.Esi,  	DEBUG_TypeShortUInt)
INTERNAL_VAR(edi,			0,		&DEBUG_context.Edi,  	DEBUG_TypeIntConst)
INTERNAL_VAR(di,			0,		&DEBUG_context.Edi,  	DEBUG_TypeShortUInt)
INTERNAL_VAR(ebp,			0,		&DEBUG_context.Ebp,  	DEBUG_TypeIntConst)
INTERNAL_VAR(bp,			0,		&DEBUG_context.Ebp,  	DEBUG_TypeShortUInt)
INTERNAL_VAR(es,			0,		&DEBUG_context.SegEs,  	DEBUG_TypeIntConst)
INTERNAL_VAR(ds,			0,		&DEBUG_context.SegDs,  	DEBUG_TypeIntConst)
INTERNAL_VAR(cs,			0,		&DEBUG_context.SegCs,	DEBUG_TypeIntConst)
INTERNAL_VAR(ss,			0,		&DEBUG_context.SegSs,  	DEBUG_TypeIntConst)
INTERNAL_VAR(fs,			0,		&DEBUG_context.SegFs,  	DEBUG_TypeIntConst)
INTERNAL_VAR(gs,			0,		&DEBUG_context.SegGs,  	DEBUG_TypeIntConst)
#endif
