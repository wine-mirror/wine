/*
 * Win32 advapi/wmi functions
 *
 * Copyright 2016 Austin English
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
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "wmistr.h"

#define _WMI_SOURCE_
#include "wmium.h"

#include "wine/debug.h"

#include "advapi32_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmi);

/******************************************************************************
 * WmiExecuteMethodA [ADVAPI32.@]
 */
ULONG WMIAPI WmiExecuteMethodA(WMIHANDLE handle, const char *name, ULONG method, ULONG inputsize,
                               void *inputbuffer, ULONG *outputsize, void *outputbuffer)
{
    FIXME(" %p %s %lu %lu %p %p %p: stub\n", handle, debugstr_a(name), method, inputsize, inputbuffer,
                                           outputsize, outputbuffer);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * WmiExecuteMethodW [ADVAPI32.@]
 */
ULONG WMIAPI WmiExecuteMethodW(WMIHANDLE handle, const WCHAR *name, ULONG method, ULONG inputsize,
                               void *inputbuffer, ULONG *outputsize, void *outputbuffer)
{
    FIXME("%p %s %lu %lu %p %p %p: stub\n", handle, debugstr_w(name), method, inputsize, inputbuffer,
                                          outputsize, outputbuffer);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * WmiFreeBuffer [ADVAPI32.@]
 */
void WMIAPI WmiFreeBuffer(void *buffer)
{
    FIXME("%p: stub\n", buffer);
}

/******************************************************************************
 * WmiMofEnumerateResourcesA [ADVAPI32.@]
 */
ULONG WMIAPI WmiMofEnumerateResourcesA(MOFHANDLE handle, ULONG *count, MOFRESOURCEINFOA **resource)
{
    FIXME("%p %p %p: stub\n", handle, count, resource);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * WmiMofEnumerateResourcesW [ADVAPI32.@]
 */
ULONG WMIAPI WmiMofEnumerateResourcesW(MOFHANDLE handle, ULONG *count, MOFRESOURCEINFOW **resource)
{
    FIXME("%p %p %p: stub\n", handle, count, resource);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * WmiNotificationRegistrationA [ADVAPI32.@]
 */
ULONG WMIAPI WmiNotificationRegistrationA(GUID *guid, BOOLEAN enable, void *info,
                                          ULONG_PTR context, ULONG flags)
{
    FIXME("%s %u %p 0x%Ix 0x%08lx: stub\n", debugstr_guid(guid), enable, info, context, flags);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * WmiNotificationRegistrationW [ADVAPI32.@]
 */
ULONG WMIAPI WmiNotificationRegistrationW(GUID *guid, BOOLEAN enable, void *info,
                                          ULONG_PTR context, ULONG flags)
{
    FIXME("%s %u %p 0x%Ix 0x%08lx: stub\n", debugstr_guid(guid), enable, info, context, flags);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * WmiOpenBlock [ADVAPI32.@]
 */
ULONG WINAPI WmiOpenBlock(GUID *guid, ULONG access, WMIHANDLE *handle)
{
    FIXME("%s %lu %p: stub\n", debugstr_guid(guid), access, handle);
    return ERROR_SUCCESS;
}

/******************************************************************************
 * WmiQueryAllDataA [ADVAPI32.@]
 */
ULONG WMIAPI WmiQueryAllDataA(WMIHANDLE handle, ULONG *size, void *buffer)
{
    FIXME("%p %p %p: stub\n", handle, size, buffer);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * WmiQueryAllDataW [ADVAPI32.@]
 */
ULONG WMIAPI WmiQueryAllDataW(WMIHANDLE handle, ULONG *size, void *buffer)
{
    FIXME("%p %p %p: stub\n", handle, size, buffer);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * WmiQueryGuidInformation [ADVAPI32.@]
 */
ULONG WMIAPI WmiQueryGuidInformation(WMIHANDLE handle, WMIGUIDINFORMATION *info)
{
    FIXME("%p %p: stub\n", handle, info);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * WmiSetSingleInstanceA [ADVAPI32.@]
 */
ULONG WMIAPI WmiSetSingleInstanceA(WMIHANDLE handle, const char *name, ULONG reserved,
                                   ULONG size, void *buffer)
{
    FIXME("%p %s %lu %lu %p: stub\n", handle, debugstr_a(name), reserved, size, buffer);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * WmiSetSingleInstanceW [ADVAPI32.@]
 */
ULONG WMIAPI WmiSetSingleInstanceW(WMIHANDLE handle, const WCHAR *name, ULONG reserved,
                                   ULONG size, void *buffer)
{
    FIXME("%p %s %lu %lu %p: stub\n", handle, debugstr_w(name), reserved, size, buffer);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * WmiSetSingleItemA [ADVAPI32.@]
 */
ULONG WMIAPI WmiSetSingleItemA(WMIHANDLE handle, const char *name, ULONG id, ULONG reserved,
                               ULONG size, void *buffer)
{
    FIXME("%p %s %lu %lu %lu %p: stub\n", handle, debugstr_a(name), id, reserved, size, buffer);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * WmiSetSingleItemW [ADVAPI32.@]
 */
ULONG WMIAPI WmiSetSingleItemW(WMIHANDLE handle, const WCHAR *name, ULONG id, ULONG reserved,
                               ULONG size, void *buffer)
{
    FIXME("%p %s %lu %lu %lu %p: stub\n", handle, debugstr_w(name), id, reserved, size, buffer);
    return ERROR_CALL_NOT_IMPLEMENTED;
}
