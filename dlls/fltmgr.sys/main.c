/*
 * fltmgr.sys
 *
 * Copyright 2015 Austin English
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "ddk/fltkernel.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(fltmgr);

NTSTATUS WINAPI DriverEntry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    TRACE( "(%p, %s)\n", driver, debugstr_w(path->Buffer) );

    return STATUS_SUCCESS;
}

void WINAPI FltInitializePushLock( EX_PUSH_LOCK *lock )
{
    FIXME( "(%p): stub\n", lock );
}

void WINAPI FltDeletePushLock( EX_PUSH_LOCK *lock )
{
    FIXME( "(%p): stub\n", lock );
}

void WINAPI FltAcquirePushLockExclusive( EX_PUSH_LOCK *lock )
{
    FIXME( "(%p): stub\n", lock );
}

void WINAPI FltReleasePushLock( EX_PUSH_LOCK *lock )
{
    FIXME( "(%p): stub\n", lock );
}

NTSTATUS WINAPI FltRegisterFilter( PDRIVER_OBJECT driver, const FLT_REGISTRATION *reg, PFLT_FILTER *filter )
{
    FIXME( "(%p,%p,%p): stub\n", driver, reg, filter );

    if(filter)
        *filter = UlongToHandle(0xdeadbeaf);

    return STATUS_SUCCESS;
}

NTSTATUS WINAPI FltStartFiltering( PFLT_FILTER filter )
{
    FIXME( "(%p): stub\n", filter );

    return STATUS_SUCCESS;
}

void WINAPI FltUnregisterFilter( PFLT_FILTER filter )
{
    FIXME( "(%p): stub\n", filter );
}

void* WINAPI FltGetRoutineAddress(LPCSTR name)
{
    HMODULE mod = GetModuleHandleW(L"fltmgr.sys");
    void *func;

    func = GetProcAddress(mod, name);
    if (func)
        TRACE( "%s -> %p\n", debugstr_a(name), func );
    else
        FIXME( "%s not found\n", debugstr_a(name) );

    return func;
}

NTSTATUS WINAPI FltBuildDefaultSecurityDescriptor(PSECURITY_DESCRIPTOR *descriptor, ACCESS_MASK access)
{
    PACL dacl;
    NTSTATUS ret = STATUS_INSUFFICIENT_RESOURCES;
    DWORD sid_len;
    SID *sid;
    SID *sid_system = NULL;
    PSECURITY_DESCRIPTOR sec_desc = NULL;
    SID_IDENTIFIER_AUTHORITY auth = { SECURITY_NULL_SID_AUTHORITY };

    *descriptor = NULL;

    sid_len = RtlLengthRequiredSid(2);
    sid = ExAllocatePool(PagedPool, sid_len);
    if (!sid)
        goto done;
    RtlInitializeSid(sid, &auth, 2);
    sid->SubAuthority[1] = DOMAIN_GROUP_RID_ADMINS;
    sid->SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;

    sid_len = RtlLengthRequiredSid(1);
    sid_system = ExAllocatePool(PagedPool, sid_len);
    if (!sid_system)
        goto done;
    RtlInitializeSid(sid_system, &auth, 1);
    sid_system->SubAuthority[0] = SECURITY_LOCAL_SYSTEM_RID;

    sid_len = SECURITY_DESCRIPTOR_MIN_LENGTH + sizeof(ACL) +
            sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(sid) +
            sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(sid_system);

    sec_desc = ExAllocatePool(PagedPool, sid_len);
    if (!sec_desc)
    {
        ret = STATUS_NO_MEMORY;
        goto done;
    }

    RtlCreateSecurityDescriptor(sec_desc, SECURITY_DESCRIPTOR_REVISION);
    dacl = (PACL)((char*)sec_desc + SECURITY_DESCRIPTOR_MIN_LENGTH);
    RtlCreateAcl(dacl, sid_len - SECURITY_DESCRIPTOR_MIN_LENGTH, ACL_REVISION);
    RtlAddAccessAllowedAce(dacl, ACL_REVISION, access, sid);
    RtlAddAccessAllowedAce(dacl, ACL_REVISION, access, sid_system);
    RtlSetDaclSecurityDescriptor(sec_desc, 1, dacl, 0);
    *descriptor = sec_desc;
    ret = STATUS_SUCCESS;

done:
    ExFreePool(sid);
    ExFreePool(sid_system);
    return ret;
}

void WINAPI FltFreeSecurityDescriptor(PSECURITY_DESCRIPTOR descriptor)
{
    ExFreePool(descriptor);
}
