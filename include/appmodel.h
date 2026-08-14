/*
 * Copyright (C) 2017 Alistair Leslie-Hughes
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
#ifndef _APPMODEL_H_
#define _APPMODEL_H_

/* Application User Model ID (space included for NULL terminator) */
#define APPLICATION_USER_MODEL_ID_MIN_LENGTH 21
#define APPLICATION_USER_MODEL_ID_MAX_LENGTH 130

/* Package architecture (no space for NULL terminator) */
#define PACKAGE_ARCHITECTURE_MIN_LENGTH 3
#define PACKAGE_ARCHITECTURE_MAX_LENGTH 7

/* Package family name (no space for NULL terminator) */
#define PACKAGE_FAMILY_NAME_MIN_LENGTH 17
#define PACKAGE_FAMILY_NAME_MAX_LENGTH 64

/* Package full name (no space for NULL terminator) */
#define PACKAGE_FULL_NAME_MIN_LENGTH 30
#define PACKAGE_FULL_NAME_MAX_LENGTH 127

/* Package name (no space for NULL terminator) */
#define PACKAGE_NAME_MIN_LENGTH 3
#define PACKAGE_NAME_MAX_LENGTH 50

/* Package publisher ID (no space for NULL terminator) */
#define PACKAGE_PUBLISHERID_MIN_LENGTH 13
#define PACKAGE_PUBLISHERID_MAX_LENGTH 13

/* Package publisher (no space for NULL terminator) */
#define PACKAGE_PUBLISHER_MIN_LENGTH 4
#define PACKAGE_PUBLISHER_MAX_LENGTH 8192

/* Package relative application ID (space included for NULL terminator) */
#define PACKAGE_RELATIVE_APPLICATION_ID_MIN_LENGTH 2
#define PACKAGE_RELATIVE_APPLICATION_ID_MAX_LENGTH 65

/* Package resource ID (no space for NULL terminator) */
#define PACKAGE_RESOURCEID_MIN_LENGTH 0
#define PACKAGE_RESOURCEID_MAX_LENGTH 30

/* Package version (no space for NULL terminator) */
#define PACKAGE_VERSION_MIN_LENGTH 7
#define PACKAGE_VERSION_MAX_LENGTH 23

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum AppPolicyMediaFoundationCodecLoading
{
    AppPolicyMediaFoundationCodecLoading_All       = 0,
    AppPolicyMediaFoundationCodecLoading_InboxOnly = 1,
} AppPolicyMediaFoundationCodecLoading;

typedef enum AppPolicyProcessTerminationMethod
{
    AppPolicyProcessTerminationMethod_ExitProcess      = 0,
    AppPolicyProcessTerminationMethod_TerminateProcess = 1,
} AppPolicyProcessTerminationMethod;

typedef enum AppPolicyThreadInitializationType
{
    AppPolicyThreadInitializationType_None            = 0,
    AppPolicyThreadInitializationType_InitializeWinRT = 1,
} AppPolicyThreadInitializationType;

typedef enum AppPolicyShowDeveloperDiagnostic
{
    AppPolicyShowDeveloperDiagnostic_None   = 0,
    AppPolicyShowDeveloperDiagnostic_ShowUI = 1,
} AppPolicyShowDeveloperDiagnostic;

typedef enum AppPolicyWindowingModel
{
    AppPolicyWindowingModel_None           = 0,
    AppPolicyWindowingModel_Universal      = 1,
    AppPolicyWindowingModel_ClassicDesktop = 2,
    AppPolicyWindowingModel_ClassicPhone   = 3
} AppPolicyWindowingModel;

typedef struct PACKAGE_VERSION
{
    union
    {
        UINT64 Version;
        struct
        {
            USHORT Revision;
            USHORT Build;
            USHORT Minor;
            USHORT Major;
        }
        DUMMYSTRUCTNAME;
    }
    DUMMYUNIONNAME;
}
PACKAGE_VERSION;

typedef struct PACKAGE_ID
{
    UINT32 reserved;
    UINT32 processorArchitecture;
    PACKAGE_VERSION version;
    WCHAR *name;
    WCHAR *publisher;
    WCHAR *resourceId;
    WCHAR *publisherId;
}
PACKAGE_ID;

LONG WINAPI AppPolicyGetMediaFoundationCodecLoading(HANDLE token, AppPolicyMediaFoundationCodecLoading *policy);
LONG WINAPI AppPolicyGetProcessTerminationMethod(HANDLE token, AppPolicyProcessTerminationMethod *policy);
LONG WINAPI AppPolicyGetShowDeveloperDiagnostic(HANDLE token, AppPolicyShowDeveloperDiagnostic *policy);
LONG WINAPI AppPolicyGetThreadInitializationType(HANDLE token, AppPolicyThreadInitializationType *policy);
LONG WINAPI AppPolicyGetWindowingModel(HANDLE processToken, AppPolicyWindowingModel *policy);
LONG WINAPI PackageIdFromFullName(const WCHAR *full_name, UINT32 flags, UINT32 *buffer_length, BYTE *buffer);

#if defined(__cplusplus)
}
#endif

#endif /* _APPMODEL_H_ */
