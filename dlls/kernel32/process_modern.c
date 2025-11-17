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
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(kernel32);

/* Process mitigation policies */
typedef enum _PROCESS_MITIGATION_POLICY {
    ProcessDEPPolicy,
    ProcessASLRPolicy,
    ProcessDynamicCodePolicy,
    ProcessStrictHandleCheckPolicy,
    ProcessSystemCallDisablePolicy,
    ProcessMitigationOptionsMask,
    ProcessExtensionPointDisablePolicy,
    ProcessControlFlowGuardPolicy,
    ProcessSignaturePolicy,
    ProcessFontDisablePolicy,
    ProcessImageLoadPolicy,
    ProcessSystemCallFilterPolicy,
    ProcessPayloadRestrictionPolicy,
    ProcessChildProcessPolicy,
    ProcessSideChannelIsolationPolicy,
    ProcessUserShadowStackPolicy,
    ProcessRedirectionTrustPolicy,
    MaxProcessMitigationPolicy
} PROCESS_MITIGATION_POLICY, *PPROCESS_MITIGATION_POLICY;

typedef struct _PROCESS_MITIGATION_DEP_POLICY {
    DWORD Enable : 1;
    DWORD DisableAtlThunkEmulation : 1;
    DWORD ReservedFlags : 30;
    BOOL Permanent;
} PROCESS_MITIGATION_DEP_POLICY, *PPROCESS_MITIGATION_DEP_POLICY;

typedef struct _PROCESS_MITIGATION_ASLR_POLICY {
    DWORD EnableBottomUpRandomization : 1;
    DWORD EnableForceRelocateImages : 1;
    DWORD EnableHighEntropy : 1;
    DWORD DisallowStrippedImages : 1;
    DWORD ReservedFlags : 28;
} PROCESS_MITIGATION_ASLR_POLICY, *PPROCESS_MITIGATION_ASLR_POLICY;

typedef struct _PROCESS_MITIGATION_DYNAMIC_CODE_POLICY {
    DWORD ProhibitDynamicCode : 1;
    DWORD AllowThreadOptOut : 1;
    DWORD AllowRemoteDowngrade : 1;
    DWORD AuditProhibitDynamicCode : 1;
    DWORD ReservedFlags : 28;
} PROCESS_MITIGATION_DYNAMIC_CODE_POLICY, *PPROCESS_MITIGATION_DYNAMIC_CODE_POLICY;

typedef struct _PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY {
    DWORD RaiseExceptionOnInvalidHandleReference : 1;
    DWORD HandleExceptionsPermanentlyEnabled : 1;
    DWORD ReservedFlags : 30;
} PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY, *PPROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY;

typedef struct _PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY {
    DWORD EnableControlFlowGuard : 1;
    DWORD EnableExportSuppression : 1;
    DWORD StrictMode : 1;
    DWORD ReservedFlags : 29;
} PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY, *PPROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY;

typedef struct _PROCESS_MITIGATION_IMAGE_LOAD_POLICY {
    DWORD NoRemoteImages : 1;
    DWORD NoLowMandatoryLabelImages : 1;
    DWORD PreferSystem32Images : 1;
    DWORD AuditNoRemoteImages : 1;
    DWORD AuditNoLowMandatoryLabelImages : 1;
    DWORD ReservedFlags : 27;
} PROCESS_MITIGATION_IMAGE_LOAD_POLICY, *PPROCESS_MITIGATION_IMAGE_LOAD_POLICY;

typedef struct _PROCESS_MITIGATION_CHILD_PROCESS_POLICY {
    DWORD NoChildProcessCreation : 1;
    DWORD AuditNoChildProcessCreation : 1;
    DWORD AllowSecureProcessCreation : 1;
    DWORD ReservedFlags : 29;
} PROCESS_MITIGATION_CHILD_PROCESS_POLICY, *PPROCESS_MITIGATION_CHILD_PROCESS_POLICY;

/* Static storage for process policies - in real implementation would be per-process */
static PROCESS_MITIGATION_DEP_POLICY dep_policy = { 1, 0, 0, FALSE };
static PROCESS_MITIGATION_ASLR_POLICY aslr_policy = { 1, 0, 0, 0, 0 };
static PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamic_code_policy = { 0, 0, 0, 0, 0 };
static PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY cfg_policy = { 0, 0, 0, 0 };

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

    switch (policy)
    {
    case ProcessDEPPolicy:
        if (length < sizeof(PROCESS_MITIGATION_DEP_POLICY))
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        memcpy( buffer, &dep_policy, sizeof(dep_policy) );
        TRACE( "Returning DEP policy: Enable=%d\n", dep_policy.Enable );
        return TRUE;

    case ProcessASLRPolicy:
        if (length < sizeof(PROCESS_MITIGATION_ASLR_POLICY))
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        memcpy( buffer, &aslr_policy, sizeof(aslr_policy) );
        TRACE( "Returning ASLR policy: BottomUp=%d\n", aslr_policy.EnableBottomUpRandomization );
        return TRUE;

    case ProcessDynamicCodePolicy:
        if (length < sizeof(PROCESS_MITIGATION_DYNAMIC_CODE_POLICY))
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        memcpy( buffer, &dynamic_code_policy, sizeof(dynamic_code_policy) );
        TRACE( "Returning Dynamic Code policy: Prohibit=%d\n", dynamic_code_policy.ProhibitDynamicCode );
        return TRUE;

    case ProcessControlFlowGuardPolicy:
        if (length < sizeof(PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY))
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        memcpy( buffer, &cfg_policy, sizeof(cfg_policy) );
        TRACE( "Returning CFG policy: Enable=%d\n", cfg_policy.EnableControlFlowGuard );
        return TRUE;

    case ProcessStrictHandleCheckPolicy:
    case ProcessSystemCallDisablePolicy:
    case ProcessExtensionPointDisablePolicy:
    case ProcessSignaturePolicy:
    case ProcessFontDisablePolicy:
    case ProcessImageLoadPolicy:
    case ProcessSystemCallFilterPolicy:
    case ProcessPayloadRestrictionPolicy:
    case ProcessChildProcessPolicy:
    case ProcessSideChannelIsolationPolicy:
    case ProcessUserShadowStackPolicy:
    case ProcessRedirectionTrustPolicy:
        FIXME( "Policy %d not implemented, returning default (disabled)\n", policy );
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

    switch (policy)
    {
    case ProcessDEPPolicy:
        if (length < sizeof(PROCESS_MITIGATION_DEP_POLICY))
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        memcpy( &dep_policy, buffer, sizeof(dep_policy) );
        TRACE( "Set DEP policy: Enable=%d\n", dep_policy.Enable );
        return TRUE;

    case ProcessASLRPolicy:
        if (length < sizeof(PROCESS_MITIGATION_ASLR_POLICY))
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        memcpy( &aslr_policy, buffer, sizeof(aslr_policy) );
        TRACE( "Set ASLR policy: BottomUp=%d\n", aslr_policy.EnableBottomUpRandomization );
        return TRUE;

    case ProcessDynamicCodePolicy:
        if (length < sizeof(PROCESS_MITIGATION_DYNAMIC_CODE_POLICY))
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        memcpy( &dynamic_code_policy, buffer, sizeof(dynamic_code_policy) );
        TRACE( "Set Dynamic Code policy: Prohibit=%d\n", dynamic_code_policy.ProhibitDynamicCode );
        return TRUE;

    case ProcessControlFlowGuardPolicy:
        if (length < sizeof(PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY))
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        memcpy( &cfg_policy, buffer, sizeof(cfg_policy) );
        TRACE( "Set CFG policy: Enable=%d\n", cfg_policy.EnableControlFlowGuard );
        return TRUE;

    default:
        FIXME( "Policy %d not fully implemented, accepting anyway\n", policy );
        return TRUE;
    }
}

/***********************************************************************
 *          GetProcessInformation
 */
BOOL WINAPI GetProcessInformation( HANDLE process, DWORD info_class, void *info, DWORD size )
{
    FIXME( "process %p, info_class %#lx, info %p, size %lu: stub\n", process, info_class, info, size );

    /* Return success with zeroed data for now */
    if (info && size > 0)
        memset( info, 0, size );

    return TRUE;
}

/***********************************************************************
 *          SetProcessInformation
 */
BOOL WINAPI SetProcessInformation( HANDLE process, DWORD info_class, void *info, DWORD size )
{
    FIXME( "process %p, info_class %#lx, info %p, size %lu: stub\n", process, info_class, info, size );

    /* Accept the call for compatibility */
    return TRUE;
}
