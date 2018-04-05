/*
 * ATSvc RPC API
 *
 * Copyright 2018 Dmitry Timoshkov
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

#include <stdarg.h>

#include "windef.h"
#include "atsvc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(schedsvc);

DWORD __cdecl NetrJobAdd(ATSVC_HANDLE server_name, AT_INFO *info, DWORD *jobid)
{
    FIXME("%s,%p,%p: stub\n", debugstr_w(server_name), info, jobid);
    return ERROR_NOT_SUPPORTED;
}

DWORD __cdecl NetrJobDel(ATSVC_HANDLE server_name, DWORD min_jobid, DWORD max_jobid)
{
    FIXME("%s,%u,%u: stub\n", debugstr_w(server_name), min_jobid, max_jobid);
    return ERROR_NOT_SUPPORTED;
}

DWORD __cdecl NetrJobEnum(ATSVC_HANDLE server_name, AT_ENUM_CONTAINER *container,
                          DWORD max_length, DWORD *total, DWORD *resume)
{
    FIXME("%s,%p,%u,%p,%p: stub\n", debugstr_w(server_name), container, max_length, total, resume);
    return ERROR_NOT_SUPPORTED;
}

DWORD __cdecl NetrJobGetInfo(ATSVC_HANDLE server_name, DWORD jobid, AT_INFO **info)
{
    FIXME("%s,%u,%p: stub\n", debugstr_w(server_name), jobid, info);
    return ERROR_NOT_SUPPORTED;
}
