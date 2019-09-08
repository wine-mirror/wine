/*
 * Win32 debugger functions
 *
 * Copyright (C) 1999 Alexandre Julliard
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

#include "config.h"
#include <stdio.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winerror.h"
#include "wine/server.h"
#include "kernel_private.h"
#include "wine/asm.h"
#include "wine/debug.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(debugstr);

/***********************************************************************
 *           DebugBreakProcess   (KERNEL32.@)
 *
 *  Raises an exception so that a debugger (if attached)
 *  can take some action. Same as DebugBreak, but applies to any process.
 *
 * PARAMS
 *  hProc [I] Process to break into.
 *
 * RETURNS
 *
 *  True if successful.
 */
BOOL WINAPI DebugBreakProcess(HANDLE process)
{
    NTSTATUS status;

    TRACE("(%p)\n", process);

    status = DbgUiIssueRemoteBreakin(process);
    if (status) SetLastError(RtlNtStatusToDosError(status));
    return !status;
}


/***********************************************************************
 *           DebugSetProcessKillOnExit                    (KERNEL32.@)
 *
 * Let a debugger decide whether a debuggee will be killed upon debugger
 * termination.
 *
 * PARAMS
 *  kill [I] If set to true then kill the process on exit.
 *
 * RETURNS
 *  True if successful, false otherwise.
 */
BOOL WINAPI DebugSetProcessKillOnExit(BOOL kill)
{
    BOOL ret = FALSE;

    SERVER_START_REQ( set_debugger_kill_on_exit )
    {
        req->kill_on_exit = kill;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}
