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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

   /* break handling */
INTERNAL_VAR(BreakAllThreadsStartup,    FALSE,  NULL,           dbg_itype_unsigned_int32)
INTERNAL_VAR(BreakOnCritSectTimeOut,    FALSE,  NULL,           dbg_itype_unsigned_int32)
INTERNAL_VAR(BreakOnAttach,             FALSE,  NULL,           dbg_itype_unsigned_int32)
INTERNAL_VAR(BreakOnFirstChance,        FALSE,  NULL,           dbg_itype_unsigned_int32)
INTERNAL_VAR(BreakOnDllLoad,            FALSE,  NULL,           dbg_itype_unsigned_int32)
INTERNAL_VAR(CanDeferOnBPByAddr,        FALSE,  NULL,           dbg_itype_unsigned_int32)

   /* current process/thread */
INTERNAL_VAR(ThreadId,                  FALSE,  &dbg_curr_tid,  dbg_itype_unsigned_int32)
INTERNAL_VAR(ProcessId,                 FALSE,  &dbg_curr_pid,  dbg_itype_unsigned_int32)

   /* symbol manipulation */
INTERNAL_VAR(AlwaysShowThunks,          FALSE,  NULL,           dbg_itype_unsigned_int32)

   /* process manipulation */
INTERNAL_VAR(AlsoDebugProcChild,        FALSE,  NULL,           dbg_itype_unsigned_int32)

   /* automatic invocation on failure */
INTERNAL_VAR(ShowCrashDialog,           TRUE,   NULL,           dbg_itype_unsigned_int32)
