/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/* Wine internal debugger
 * Definitions for internal variables
 * Copyright 2000 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

   /* break handling */
INTERNAL_VAR(BreakAllThreadsStartup,	FALSE,		NULL,  			DT_BASIC_CONST_INT)
INTERNAL_VAR(BreakOnCritSectTimeOut,    FALSE,          NULL,  			DT_BASIC_CONST_INT)
INTERNAL_VAR(BreakOnAttach,		FALSE,		NULL,			DT_BASIC_CONST_INT)
INTERNAL_VAR(BreakOnFirstChance,	TRUE,		NULL,			DT_BASIC_CONST_INT)
INTERNAL_VAR(BreakOnDllLoad,		FALSE, 		NULL, 			DT_BASIC_CONST_INT)
INTERNAL_VAR(CanDeferOnBPByAddr,        FALSE,          NULL,                   DT_BASIC_CONST_INT)

   /* console handling */
INTERNAL_VAR(ConChannelMask,		DBG_CHN_MESG,	NULL,  			DT_BASIC_CONST_INT)
INTERNAL_VAR(StdChannelMask,		0,		NULL,  			DT_BASIC_CONST_INT)

   /* debugging debugger */
INTERNAL_VAR(ExtDbgOnInvalidAddress,	FALSE,		NULL,  			DT_BASIC_CONST_INT)
INTERNAL_VAR(ExtDbgOnInternalException, FALSE,          NULL,                   DT_BASIC_CONST_INT)

   /* current process/thread */
INTERNAL_VAR(ThreadId,			FALSE,		&DEBUG_CurrTid,		DT_BASIC_CONST_INT)
INTERNAL_VAR(ProcessId,			FALSE,		&DEBUG_CurrPid,		DT_BASIC_CONST_INT)

   /* context manipulation */
#ifdef __i386__
/* FIXME: 16 bit registers use imply that CPU is little endian, which is
 * the case when running natively i386 code
 */
INTERNAL_VAR(eip,			0,		&DEBUG_context.Eip,	DT_BASIC_CONST_INT)
INTERNAL_VAR(ip,			0,		&DEBUG_context.Eip,  	DT_BASIC_USHORTINT)
INTERNAL_VAR(pc,			0,		&DEBUG_context.Eip,  	DT_BASIC_CONST_INT)
INTERNAL_VAR(flags,			0,		&DEBUG_context.EFlags,  DT_BASIC_CONST_INT)
INTERNAL_VAR(esp,			0,		&DEBUG_context.Esp,  	DT_BASIC_CONST_INT)
INTERNAL_VAR(sp,			0,		&DEBUG_context.Esp,  	DT_BASIC_USHORTINT)
INTERNAL_VAR(eax,			0,		&DEBUG_context.Eax,  	DT_BASIC_CONST_INT)
INTERNAL_VAR(ax,			0,		&DEBUG_context.Eax,  	DT_BASIC_USHORTINT)
INTERNAL_VAR(ebx,			0,		&DEBUG_context.Ebx,  	DT_BASIC_CONST_INT)
INTERNAL_VAR(bx,			0,		&DEBUG_context.Ebx,  	DT_BASIC_USHORTINT)
INTERNAL_VAR(ecx,			0,		&DEBUG_context.Ecx,  	DT_BASIC_CONST_INT)
INTERNAL_VAR(cx,			0,		&DEBUG_context.Ecx,  	DT_BASIC_USHORTINT)
INTERNAL_VAR(edx,			0,		&DEBUG_context.Edx,  	DT_BASIC_CONST_INT)
INTERNAL_VAR(dx,			0,		&DEBUG_context.Edx,  	DT_BASIC_USHORTINT)
INTERNAL_VAR(esi,			0,		&DEBUG_context.Esi,  	DT_BASIC_CONST_INT)
INTERNAL_VAR(si,			0,		&DEBUG_context.Esi,  	DT_BASIC_USHORTINT)
INTERNAL_VAR(edi,			0,		&DEBUG_context.Edi,  	DT_BASIC_CONST_INT)
INTERNAL_VAR(di,			0,		&DEBUG_context.Edi,  	DT_BASIC_USHORTINT)
INTERNAL_VAR(ebp,			0,		&DEBUG_context.Ebp,  	DT_BASIC_CONST_INT)
INTERNAL_VAR(bp,			0,		&DEBUG_context.Ebp,  	DT_BASIC_USHORTINT)
INTERNAL_VAR(es,			0,		&DEBUG_context.SegEs,  	DT_BASIC_CONST_INT)
INTERNAL_VAR(ds,			0,		&DEBUG_context.SegDs,  	DT_BASIC_CONST_INT)
INTERNAL_VAR(cs,			0,		&DEBUG_context.SegCs,	DT_BASIC_CONST_INT)
INTERNAL_VAR(ss,			0,		&DEBUG_context.SegSs,  	DT_BASIC_CONST_INT)
INTERNAL_VAR(fs,			0,		&DEBUG_context.SegFs,  	DT_BASIC_CONST_INT)
INTERNAL_VAR(gs,			0,		&DEBUG_context.SegGs,  	DT_BASIC_CONST_INT)

INTERNAL_VAR(regs,                      0,              (DWORD*)&DEBUG_context, DT_BASIC_CONTEXT)
#endif
