/*
 * Credentials User Interface
 *
 * Copyright 2006 Robert Shearman (for CodeWeavers)
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
#include "winnt.h"
#include "winuser.h"
#include "wincred.h"
#include "commctrl.h"

#include "credui_resources.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(credui);

struct pending_credentials
{
    struct list entry;
    PWSTR pszTargetName;
    PWSTR pszUsername;
    PWSTR pszPassword;
};

static HINSTANCE hinstCredUI;

static struct list pending_credentials_list = LIST_INIT(pending_credentials_list);

static CRITICAL_SECTION csPendingCredentials;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &csPendingCredentials,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": csPendingCredentials") }
};
static CRITICAL_SECTION csPendingCredentials = { &critsect_debug, -1, 0, 0, 0, 0 };


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n",hinstDLL,fdwReason,lpvReserved);

    if (fdwReason == DLL_WINE_PREATTACH) return FALSE;	/* prefer native version */

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDLL);
        hinstCredUI = hinstDLL;
        InitCommonControls();
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        struct pending_credentials *entry, *cursor2;
        LIST_FOR_EACH_ENTRY_SAFE(entry, cursor2, &pending_credentials_list, struct pending_credentials, entry)
        {
            list_remove(&entry->entry);

            HeapFree(GetProcessHeap(), 0, entry->pszTargetName);
            HeapFree(GetProcessHeap(), 0, entry->pszUsername);
            HeapFree(GetProcessHeap(), 0, entry->pszPassword);
            HeapFree(GetProcessHeap(), 0, entry);
        }
    }

    return TRUE;
}

static DWORD save_credentials(PCWSTR pszTargetName, PCWSTR pszUsername,
                              PCWSTR pszPassword)
{
    FIXME("save servername %s with username %s\n", debugstr_w(pszTargetName), debugstr_w(pszUsername));
    return ERROR_SUCCESS;
}

struct cred_dialog_params
{
    PCWSTR pszTargetName;
    PCWSTR pszMessageText;
    PCWSTR pszCaptionText;
    HBITMAP hbmBanner;
    PWSTR pszUsername;
    ULONG ulUsernameMaxChars;
    PWSTR pszPassword;
    ULONG ulPasswordMaxChars;
    BOOL fSave;
    DWORD dwFlags;
};

static INT_PTR CALLBACK CredDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
                                       LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            struct cred_dialog_params *params = (struct cred_dialog_params *)lParam;

            SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)params);
            if (params->pszMessageText)
                SetDlgItemTextW(hwndDlg, IDC_MESSAGE, params->pszMessageText);
            else
            {
                WCHAR format[256];
                WCHAR message[256];
                LoadStringW(hinstCredUI, IDS_MESSAGEFORMAT, format, sizeof(format)/sizeof(format[0]));
                snprintfW(message, sizeof(message)/sizeof(message[0]), format, params->pszTargetName);
                SetDlgItemTextW(hwndDlg, IDC_MESSAGE, message);
            }
            SetDlgItemTextW(hwndDlg, IDC_USERNAME, params->pszUsername);
            SetDlgItemTextW(hwndDlg, IDC_PASSWORD, params->pszPassword);

            if (params->pszUsername[0])
                SetFocus(GetDlgItem(hwndDlg, IDC_PASSWORD));
            else
                SetFocus(GetDlgItem(hwndDlg, IDC_USERNAME));

            if (params->pszCaptionText)
                SetWindowTextW(hwndDlg, params->pszCaptionText);
            else
            {
                WCHAR format[256];
                WCHAR title[256];
                LoadStringW(hinstCredUI, IDS_TITLEFORMAT, format, sizeof(format)/sizeof(format[0]));
                snprintfW(title, sizeof(title)/sizeof(title[0]), format, params->pszTargetName);
                SetWindowTextW(hwndDlg, title);
            }

            if (params->dwFlags & (CREDUI_FLAGS_DO_NOT_PERSIST|CREDUI_FLAGS_PERSIST))
                ShowWindow(GetDlgItem(hwndDlg, IDC_SAVE), SW_HIDE);
            else if (params->fSave)
                CheckDlgButton(hwndDlg, IDC_SAVE, BST_CHECKED);

            return FALSE;
        }
        case WM_COMMAND:
            switch (wParam)
            {
                case MAKELONG(IDOK, BN_CLICKED):
                {
                    struct cred_dialog_params *params =
                        (struct cred_dialog_params *)GetWindowLongPtrW(hwndDlg, DWLP_USER);
                    HWND hwndUsername = GetDlgItem(hwndDlg, IDC_USERNAME);
                    LPWSTR user;
                    INT len;
                    INT len2;

                    len = GetWindowTextLengthW(hwndUsername);
                    user = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
                    GetWindowTextW(hwndUsername, user, len + 1);

                    if (!user[0])
                    {
                        HeapFree(GetProcessHeap(), 0, user);
                        return TRUE;
                    }

                    if (!strchrW(user, '\\') && !strchrW(user, '@'))
                    {
                        INT len_target = strlenW(params->pszTargetName);
                        memcpy(params->pszUsername, params->pszTargetName,
                               min(len_target, params->ulUsernameMaxChars) * sizeof(WCHAR));
                        if (len_target + 1 < params->ulUsernameMaxChars)
                            params->pszUsername[len_target] = '\\';
                        if (len_target + 2 < params->ulUsernameMaxChars)
                            params->pszUsername[len_target + 1] = '\0';
                    }
                    else if (params->ulUsernameMaxChars > 0)
                        params->pszUsername[0] = '\0';

                    len2 = strlenW(params->pszUsername);
                    memcpy(params->pszUsername + len2, user, min(len, params->ulUsernameMaxChars - len2) * sizeof(WCHAR));
                    if (params->ulUsernameMaxChars)
                        params->pszUsername[len2 + min(len, params->ulUsernameMaxChars - len2 - 1)] = '\0';

                    HeapFree(GetProcessHeap(), 0, user);

                    GetDlgItemTextW(hwndDlg, IDC_PASSWORD, params->pszPassword,
                                    params->ulPasswordMaxChars);

                    EndDialog(hwndDlg, IDOK);
                    return TRUE;
                }
                case MAKELONG(IDCANCEL, BN_CLICKED):
                    EndDialog(hwndDlg, IDCANCEL);
                    return TRUE;
            }
            /* fall through */
        default:
            return FALSE;
    }
}

/******************************************************************************
 *           CredUIPromptForCredentialsW [CREDUI.@]
 */
DWORD WINAPI CredUIPromptForCredentialsW(PCREDUI_INFOW pUIInfo,
                                         PCWSTR pszTargetName,
                                         PCtxtHandle Reserved,
                                         DWORD dwAuthError,
                                         PWSTR pszUsername,
                                         ULONG ulUsernameMaxChars,
                                         PWSTR pszPassword,
                                         ULONG ulPasswordMaxChars, PBOOL pfSave,
                                         DWORD dwFlags)
{
    INT_PTR ret;
    struct cred_dialog_params params;
    DWORD result = ERROR_SUCCESS;

    TRACE("(%p, %s, %p, %d, %s, %d, %p, %d, %p, 0x%08x)\n", pUIInfo,
          debugstr_w(pszTargetName), Reserved, dwAuthError, debugstr_w(pszUsername),
          ulUsernameMaxChars, pszPassword, ulPasswordMaxChars, pfSave, dwFlags);

    if ((dwFlags & (CREDUI_FLAGS_ALWAYS_SHOW_UI|CREDUI_FLAGS_GENERIC_CREDENTIALS)) == CREDUI_FLAGS_ALWAYS_SHOW_UI)
        return ERROR_INVALID_FLAGS;

    if (!pszTargetName)
        return ERROR_INVALID_PARAMETER;

    if ((dwFlags & CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX) && !pfSave)
        return ERROR_INVALID_PARAMETER;

    params.pszTargetName = pszTargetName;
    if (pUIInfo)
    {
        params.pszMessageText = pUIInfo->pszMessageText;
        params.pszCaptionText = pUIInfo->pszCaptionText;
        params.hbmBanner  = pUIInfo->hbmBanner;
    }
    else
    {
        params.pszMessageText = NULL;
        params.pszCaptionText = NULL;
        params.hbmBanner = NULL;
    }
    params.pszUsername = pszUsername;
    params.ulUsernameMaxChars = ulUsernameMaxChars;
    params.pszPassword = pszPassword;
    params.ulPasswordMaxChars = ulPasswordMaxChars;
    params.fSave = pfSave ? *pfSave : FALSE;
    params.dwFlags = dwFlags;

    ret = DialogBoxParamW(hinstCredUI, MAKEINTRESOURCEW(IDD_CREDDIALOG),
                          pUIInfo ? pUIInfo->hwndParent : NULL,
                          CredDialogProc, (LPARAM)&params);
    if (ret <= 0)
        return GetLastError();

    if (ret == IDCANCEL)
    {
        TRACE("dialog cancelled\n");
        return ERROR_CANCELLED;
    }

    if (pfSave)
        *pfSave = params.fSave;

    if (params.fSave)
    {
        if (dwFlags & CREDUI_FLAGS_EXPECT_CONFIRMATION)
        {
            BOOL found = FALSE;
            struct pending_credentials *entry;
            int len;

            EnterCriticalSection(&csPendingCredentials);

            /* find existing pending credentials for the same target and overwrite */
            /* FIXME: is this correct? */
            LIST_FOR_EACH_ENTRY(entry, &pending_credentials_list, struct pending_credentials, entry)
                if (!strcmpW(pszTargetName, entry->pszTargetName))
                {
                    found = TRUE;
                    HeapFree(GetProcessHeap(), 0, entry->pszUsername);
                    HeapFree(GetProcessHeap(), 0, entry->pszPassword);
                }

            if (!found)
            {
                entry = HeapAlloc(GetProcessHeap(), 0, sizeof(*entry));
                list_init(&entry->entry);
                len = strlenW(pszTargetName);
                entry->pszTargetName = HeapAlloc(GetProcessHeap(), 0, (len + 1)*sizeof(WCHAR));
                memcpy(entry->pszTargetName, pszTargetName, (len + 1)*sizeof(WCHAR));
                list_add_tail(&entry->entry, &pending_credentials_list);
            }

            len = strlenW(params.pszUsername);
            entry->pszUsername = HeapAlloc(GetProcessHeap(), 0, (len + 1)*sizeof(WCHAR));
            memcpy(entry->pszUsername, params.pszUsername, (len + 1)*sizeof(WCHAR));
            len = strlenW(params.pszPassword);
            entry->pszPassword = HeapAlloc(GetProcessHeap(), 0, (len + 1)*sizeof(WCHAR));
            memcpy(entry->pszPassword, params.pszPassword, (len + 1)*sizeof(WCHAR));

            LeaveCriticalSection(&csPendingCredentials);
        }
        else
            result = save_credentials(pszTargetName, pszUsername, pszPassword);
    }

    return result;
}

/******************************************************************************
 *           CredUIConfirmCredentialsW [CREDUI.@]
 */
DWORD WINAPI CredUIConfirmCredentialsW(PCWSTR pszTargetName, BOOL bConfirm)
{
    struct pending_credentials *entry;
    DWORD result = ERROR_NOT_FOUND;

    TRACE("(%s, %s)\n", debugstr_w(pszTargetName), bConfirm ? "TRUE" : "FALSE");

    if (!pszTargetName)
        return ERROR_INVALID_PARAMETER;

    EnterCriticalSection(&csPendingCredentials);

    LIST_FOR_EACH_ENTRY(entry, &pending_credentials_list, struct pending_credentials, entry)
    {
        if (!strcmpW(pszTargetName, entry->pszTargetName))
        {
            if (bConfirm)
                result = save_credentials(entry->pszTargetName, entry->pszUsername, entry->pszPassword);
            else
                result = ERROR_SUCCESS;

            list_remove(&entry->entry);

            HeapFree(GetProcessHeap(), 0, entry->pszTargetName);
            HeapFree(GetProcessHeap(), 0, entry->pszUsername);
            HeapFree(GetProcessHeap(), 0, entry->pszPassword);
            HeapFree(GetProcessHeap(), 0, entry);

            break;
        }
    }

    LeaveCriticalSection(&csPendingCredentials);

    return result;
}

/******************************************************************************
 *           CredUIParseUserNameW [CREDUI.@]
 */
DWORD WINAPI CredUIParseUserNameW(PCWSTR pszUserName, PWSTR pszUser,
                                  ULONG ulMaxUserChars, PWSTR pszDomain,
                                  ULONG ulMaxDomainChars)
{
    PWSTR p;

    TRACE("(%s, %p, %d, %p, %d)\n", debugstr_w(pszUserName), pszUser,
          ulMaxUserChars, pszDomain, ulMaxDomainChars);

    if (!pszUserName || !pszUser || !ulMaxUserChars || !pszDomain ||
        !ulMaxDomainChars)
        return ERROR_INVALID_PARAMETER;

    /* FIXME: handle marshaled credentials */

    p = strchrW(pszUserName, '\\');
    if (p)
    {
        if (p - pszUserName > ulMaxDomainChars - 1)
            return ERROR_INSUFFICIENT_BUFFER;
        if (strlenW(p + 1) > ulMaxUserChars - 1)
            return ERROR_INSUFFICIENT_BUFFER;
        strcpyW(pszUser, p + 1);
        memcpy(pszDomain, pszUserName, (p - pszUserName)*sizeof(WCHAR));
        pszDomain[p - pszUserName] = '\0';

        return ERROR_SUCCESS;
    }

    p = strrchrW(pszUserName, '@');
    if (p)
    {
        if (p + 1 - pszUserName > ulMaxUserChars - 1)
            return ERROR_INSUFFICIENT_BUFFER;
        if (strlenW(p + 1) > ulMaxDomainChars - 1)
            return ERROR_INSUFFICIENT_BUFFER;
        strcpyW(pszDomain, p + 1);
        memcpy(pszUser, pszUserName, (p - pszUserName)*sizeof(WCHAR));
        pszUser[p - pszUserName] = '\0';

        return ERROR_SUCCESS;
    }

    if (strlenW(pszUserName) > ulMaxUserChars - 1)
        return ERROR_INSUFFICIENT_BUFFER;
    strcpyW(pszUser, pszUserName);
    pszDomain[0] = '\0';

    return ERROR_SUCCESS;
}

/******************************************************************************
 *           CredUIStoreSSOCredA [CREDUI.@]
 */
DWORD WINAPI CredUIStoreSSOCredA(PCSTR pszRealm, PCSTR pszUsername,
                                 PCSTR pszPassword, BOOL bPersist)
{
    FIXME("(%s, %s, %p, %d)\n", debugstr_a(pszRealm), debugstr_a(pszUsername),
          pszPassword, bPersist);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *           CredUIStoreSSOCredW [CREDUI.@]
 */
DWORD WINAPI CredUIStoreSSOCredW(PCWSTR pszRealm, PCWSTR pszUsername,
                                 PCWSTR pszPassword, BOOL bPersist)
{
    FIXME("(%s, %s, %p, %d)\n", debugstr_w(pszRealm), debugstr_w(pszUsername),
          pszPassword, bPersist);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *           CredUIReadSSOCredA [CREDUI.@]
 */
DWORD WINAPI CredUIReadSSOCredA(PCSTR pszRealm, PSTR *ppszUsername)
{
    FIXME("(%s, %p)\n", debugstr_a(pszRealm), ppszUsername);
    if (ppszUsername)
        *ppszUsername = NULL;
    return ERROR_NOT_FOUND;
}

/******************************************************************************
 *           CredUIReadSSOCredW [CREDUI.@]
 */
DWORD WINAPI CredUIReadSSOCredW(PCWSTR pszRealm, PWSTR *ppszUsername)
{
    FIXME("(%s, %p)\n", debugstr_w(pszRealm), ppszUsername);
    if (ppszUsername)
        *ppszUsername = NULL;
    return ERROR_NOT_FOUND;
}
