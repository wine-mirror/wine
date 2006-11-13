/*
 * Copyright (C) 2006 Robert Shearman (for CodeWeavers)
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

#ifndef _WINCRED_H_
#define _WINCRED_H_

#ifndef __SECHANDLE_DEFINED__
#define __SECHANDLE_DEFINED__
typedef struct _SecHandle
{
    ULONG_PTR dwLower;
    ULONG_PTR dwUpper;
} SecHandle, *PSecHandle;
#endif

typedef SecHandle CtxtHandle;
typedef PSecHandle PCtxtHandle;

typedef struct _CREDUI_INFOA
{
    DWORD cbSize;
    HWND hwndParent;
    PCSTR pszMessageText;
    PCSTR pszCaptionText;
    HBITMAP hbmBanner;
} CREDUI_INFOA, *PCREDUI_INFOA;

typedef struct _CREDUI_INFOW
{
    DWORD cbSize;
    HWND hwndParent;
    PCWSTR pszMessageText;
    PCWSTR pszCaptionText;
    HBITMAP hbmBanner;
} CREDUI_INFOW, *PCREDUI_INFOW;

#define CRED_MAX_STRING_LENGTH              256
#define CRED_MAX_USERNAME_LENGTH            513
#define CRED_MAX_GENERIC_TARGET_NAME_LENGTH 32767
#define CRED_MAX_DOMAIN_TARGET_NAME_LENGTH  337
#define CRED_MAX_VALUE_SIZE                 256
#define CRED_MAX_ATTRIBUTES                 64

#define CRED_MAX_BLOB_SIZE                  512

#define CREDUI_MAX_MESSAGE_LENGTH 32767
#define CREDUI_MAX_CAPTION_LENGTH 128
#define CREDUI_MAX_GENERIC_TARGET_LENGTH CRED_MAX_GENERIC_TARGET_NAME_LENGTH
#define CREDUI_MAX_DOMAIN_TARGET_LENGTH CRED_MAX_DOMAIN_TARGET_LENGTH
#define CREDUI_MAX_USERNAME_LENGTH CRED_MAX_USERNAME_LENGTH
#define CREDUI_MAX_PASSWORD_LENGTH (CRED_MAX_CREDENTIAL_BLOB_SIZE / 2)

#define CREDUI_FLAGS_INCORRECT_PASSWORD             0x00000001
#define CREDUI_FLAGS_DO_NOT_PERSIST                 0x00000002
#define CREDUI_FLAGS_REQUEST_ADMINISTRATOR          0x00000004
#define CREDUI_FLAGS_EXCLUDE_CERTIFICATES           0x00000008
#define CREDUI_FLAGS_REQUIRE_CERTIFICATE            0x00000010
#define CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX            0x00000040
#define CREDUI_FLAGS_ALWAYS_SHOW_UI                 0x00000080
#define CREDUI_FLAGS_REQUIRE_SMARTCARD              0x00000100
#define CREDUI_FLAGS_PASSWORD_ONLY_OK               0x00000200
#define CREDUI_FLAGS_VALIDATE_USERNAME              0x00000400
#define CREDUI_FLAGS_COMPLETE_USERNAME              0x00000800
#define CREDUI_FLAGS_PERSIST                        0x00001000
#define CREDUI_FLAGS_SERVER_CREDENTIAL              0x00004000
#define CREDUI_FLAGS_EXPECT_CONFIRMATION            0x00020000
#define CREDUI_FLAGS_GENERIC_CREDENTIALS            0x00040000
#define CREDUI_FLAGS_USERNAME_TARGET_CREDENTIALS    0x00080000
#define CREDUI_FLAGS_KEEP_USERNAME                  0x00100000

DWORD WINAPI CredUICmdLinePromptForCredentialsW(PCWSTR,PCWSTR,PCtxtHandle,DWORD,PWSTR,ULONG,PWSTR,ULONG,BOOL*,DWORD);
DWORD WINAPI CredUICmdLinePromptForCredentialsA(PCSTR,PCSTR,PCtxtHandle,DWORD,PSTR,ULONG,PSTR,ULONG,BOOL*,DWORD);
#define      CredUICmdLinePromptForCredentials WINELIB_NAME_AW(CredUICmdLinePromptForCredentials)
DWORD WINAPI CredUIConfirmCredentialsW(PCWSTR,BOOL);
DWORD WINAPI CredUIConfirmCredentialsA(PCSTR,BOOL);
#define      CredUIConfirmCredentials WINELIB_NAME_AW(CredUIConfirmCredentials)
DWORD WINAPI CredUIParseUserNameW(PCWSTR,PWSTR,ULONG,PWSTR,ULONG);
DWORD WINAPI CredUIParseUserNameA(PCSTR,PSTR,ULONG,PSTR,ULONG);
#define      CredUIParseUserName WINELIB_NAME_AW(CredUIParseUserName)
DWORD WINAPI CredUIPromptForCredentialsW(PCREDUI_INFOW,PCWSTR,PCtxtHandle,DWORD,PWSTR,ULONG,PWSTR,ULONG,BOOL*,DWORD);
DWORD WINAPI CredUIPromptForCredentialsA(PCREDUI_INFOA,PCSTR,PCtxtHandle,DWORD,PSTR,ULONG,PSTR,ULONG,BOOL*,DWORD);
#define      CredUIPromptForCredentials WINELIB_NAME_AW(CredUIPromptForCredentials)
DWORD WINAPI CredUIStoreSSOCredW(PCWSTR,PCWSTR,PCWSTR,BOOL);
/* Note: no CredUIStoreSSOCredA in PSDK header */
DWORD WINAPI CredUIReadSSOCredW(PCWSTR,PWSTR*);
/* Note: no CredUIReadSSOCredA in PSDK header */

#endif /* _WINCRED_H_ */
