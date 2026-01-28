/*
 * Kernel32 Modern Process APIs
 * Support for Windows 10/11 process security and mitigation policies
 * Required by modern applications including Electron-based apps
 *
 * Copyright 2025 Wine Project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "winbase.h"
#include "kernel_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(kernel32);

/* Note: Process mitigation policy types are defined in winnt.h */

/***********************************************************************
 *          GetProcessMitigationPolicy
 */
BOOL WINAPI GetProcessMitigationPolicy( HANDLE process, PROCESS_MITIGATION_POLICY policy,
                                         void *buffer, SIZE_T length )
{
    TRACE( "process %p, policy %d, buffer %p, length %Iu\n", process, policy, buffer, length );

    if (!buffer)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* For now, return default/disabled policies for all types */
    switch (policy)
    {
    case ProcessDEPPolicy:
    case ProcessASLRPolicy:
    case ProcessDynamicCodePolicy:
    case ProcessStrictHandleCheckPolicy:
    case ProcessSystemCallDisablePolicy:
    case ProcessExtensionPointDisablePolicy:
    case ProcessControlFlowGuardPolicy:
    case ProcessSignaturePolicy:
    case ProcessFontDisablePolicy:
    case ProcessImageLoadPolicy:
    case ProcessSystemCallFilterPolicy:
    case ProcessPayloadRestrictionPolicy:
    case ProcessChildProcessPolicy:
    case ProcessSideChannelIsolationPolicy:
        TRACE( "Returning default policy for type %d\n", policy );
        /* Return zeroed policy structure (disabled/not enforced) */
        memset( buffer, 0, length );
        return TRUE;

    default:
        WARN( "Invalid policy %d\n", policy );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
}

/***********************************************************************
 *          SetProcessMitigationPolicy
 */
BOOL WINAPI SetProcessMitigationPolicy( PROCESS_MITIGATION_POLICY policy, void *buffer, SIZE_T length )
{
    TRACE( "policy %d, buffer %p, length %Iu\n", policy, buffer, length );

    if (!buffer)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* Accept all policy types but don't enforce them */
    switch (policy)
    {
    case ProcessDEPPolicy:
    case ProcessASLRPolicy:
    case ProcessDynamicCodePolicy:
    case ProcessStrictHandleCheckPolicy:
    case ProcessSystemCallDisablePolicy:
    case ProcessExtensionPointDisablePolicy:
    case ProcessControlFlowGuardPolicy:
    case ProcessSignaturePolicy:
    case ProcessFontDisablePolicy:
    case ProcessImageLoadPolicy:
    case ProcessSystemCallFilterPolicy:
    case ProcessPayloadRestrictionPolicy:
    case ProcessChildProcessPolicy:
    case ProcessSideChannelIsolationPolicy:
        TRACE( "Accepted policy %d (not enforced)\n", policy );
        return TRUE;

    default:
        FIXME( "Unknown policy %d, accepting anyway\n", policy );
        return TRUE;
    }
}

/***********************************************************************
 *          GetProcessInformation
 */
BOOL WINAPI GetProcessInformation( HANDLE process, PROCESS_INFORMATION_CLASS info_class, void *info, DWORD size )
{
    FIXME( "process %p, info_class %#x, info %p, size %lu: stub\n", process, info_class, info, size );

    /* Return success with zeroed data for now */
    if (info && size > 0)
        memset( info, 0, size );

    return TRUE;
}

/***********************************************************************
 *          SetProcessInformation
 */
BOOL WINAPI SetProcessInformation( HANDLE process, PROCESS_INFORMATION_CLASS info_class, void *info, DWORD size )
{
    FIXME( "process %p, info_class %#x, info %p, size %lu: stub\n", process, info_class, info, size );

    /* Accept the call for compatibility */
    return TRUE;
}
