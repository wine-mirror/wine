/*
 * AUTHZ Implementation
 *
 * Copyright 2009 Austin English
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
#include "wine/debug.h"

#include "authz.h"

WINE_DEFAULT_DEBUG_CHANNEL(authz);

/***********************************************************************
 *              AuthzInitializeResourceManager (AUTHZ.@)
 */
BOOL WINAPI AuthzInitializeResourceManager(DWORD flags, PFN_AUTHZ_DYNAMIC_ACCESS_CHECK access_checker,
        PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS compute_dyn_groups, PFN_AUTHZ_FREE_DYNAMIC_GROUPS free_dyn_groups,
        const WCHAR *managername, AUTHZ_RESOURCE_MANAGER_HANDLE *handle )
{
    FIXME("(0x%lx,%p,%p,%p,%s,%p): stub\n", flags, access_checker,
        compute_dyn_groups, free_dyn_groups,
        debugstr_w(managername), handle);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *              AuthzFreeResourceManager (AUTHZ.@)
 */
BOOL WINAPI AuthzFreeResourceManager(AUTHZ_RESOURCE_MANAGER_HANDLE handle)
{
    FIXME("%p\n", handle);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *              AuthzInstallSecurityEventSource (AUTHZ.@)
 */
BOOL WINAPI AuthzInstallSecurityEventSource(DWORD flags, AUTHZ_SOURCE_SCHEMA_REGISTRATION *registration)
{
    FIXME("(0x%lx,%p): stub\n", flags, registration);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *              AuthzAccessCheck (AUTHZ.@)
 */
BOOL WINAPI AuthzAccessCheck(DWORD flags, AUTHZ_CLIENT_CONTEXT_HANDLE client_context,
        AUTHZ_ACCESS_REQUEST *request, AUTHZ_AUDIT_EVENT_HANDLE audit_event,
        PSECURITY_DESCRIPTOR security, PSECURITY_DESCRIPTOR *optional_security,
        DWORD optional_security_count, AUTHZ_ACCESS_REPLY *reply,
        AUTHZ_ACCESS_CHECK_RESULTS_HANDLE *access_check_result)
{
    FIXME("(0x%lx,%p,%p,%p,%p,%p,0x%lx,%p,%p): stub\n", flags, client_context,
            request, audit_event, security, optional_security,
            optional_security_count, reply, access_check_result);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *              AuthzFreeContext (AUTHZ.@)
 */
BOOL WINAPI AuthzFreeContext(AUTHZ_CLIENT_CONTEXT_HANDLE client_context)
{
    FIXME("(%p): stub\n", client_context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *              AuthzInitializeContextFromSid (AUTHZ.@)
 */
BOOL WINAPI AuthzInitializeContextFromSid(DWORD flags, PSID sid,
        AUTHZ_RESOURCE_MANAGER_HANDLE resource_manager, LARGE_INTEGER *expire_time,
        LUID id, void *dynamic_group, AUTHZ_CLIENT_CONTEXT_HANDLE *client_context)
{
    FIXME("(0x%lx,%p,%p,%p,%08lx:%08lx,%p,%p): stub\n", flags, sid, resource_manager,
            expire_time, id.HighPart, id.LowPart, dynamic_group, client_context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *              AuthzInitializeContextFromToken (AUTHZ.@)
 */
BOOL WINAPI AuthzInitializeContextFromToken(DWORD flags, HANDLE token_handle,
        AUTHZ_RESOURCE_MANAGER_HANDLE resource_manager, LARGE_INTEGER *expire_time,
        LUID id, void *dynamic_group, AUTHZ_CLIENT_CONTEXT_HANDLE *client_context)
{
    FIXME("(0x%lx,%p,%p,%p,%08lx:%08lx,%p,%p): stub\n", flags, token_handle, resource_manager,
            expire_time, id.HighPart, id.LowPart, dynamic_group, client_context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
