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

#include "credui_resources.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(credui);

static HINSTANCE hinstCredUI;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	TRACE("(0x%p, %d, %p)\n",hinstDLL,fdwReason,lpvReserved);

	if (fdwReason == DLL_WINE_PREATTACH) return FALSE;	/* prefer native version */

	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hinstDLL);
		hinstCredUI = hinstDLL;
	}

	return TRUE;
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
};

static INT_PTR CALLBACK CredDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
                                       LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            struct cred_dialog_params *params = (struct cred_dialog_params *)lParam;
            DWORD ret;
            WCHAR user[256];
            WCHAR domain[256];

            SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)params);
            ret = CredUIParseUserNameW(params->pszUsername, user, 256, domain, 256);
            if (ret == ERROR_SUCCESS)
            {
                SetDlgItemTextW(hwndDlg, IDC_USERNAME, user);
                SetDlgItemTextW(hwndDlg, IDC_DOMAIN, domain);
            }
            SetDlgItemTextW(hwndDlg, IDC_PASSWORD, params->pszPassword);

            if (ret == ERROR_SUCCESS && user[0])
                SetFocus(GetDlgItem(hwndDlg, IDC_PASSWORD));
            else
                SetFocus(GetDlgItem(hwndDlg, IDC_USERNAME));

            if (params->pszCaptionText)
                SetWindowTextW(hwndDlg, params->pszCaptionText);
            return FALSE;
        }
        case WM_COMMAND:
            switch (wParam)
            {
                case MAKELONG(IDOK, BN_CLICKED):
                {
                    ULONG domainlen;
                    struct cred_dialog_params *params =
                        (struct cred_dialog_params *)GetWindowLongPtrW(hwndDlg, DWLP_USER);

                    domainlen = GetDlgItemTextW(hwndDlg, IDC_DOMAIN,
                                                params->pszUsername,
                                                params->ulUsernameMaxChars);
                    if (domainlen && (domainlen < params->ulUsernameMaxChars))
                    {
                        params->pszUsername[domainlen++] = '\\';
                        params->pszUsername[domainlen] = '\0';
                    }
                    if (domainlen < params->ulUsernameMaxChars)
                        GetDlgItemTextW(hwndDlg, IDC_USERNAME,
                                        params->pszUsername + domainlen,
                                        params->ulUsernameMaxChars - domainlen);
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

    TRACE("(%p, %s, %p, %d, %s, %d, %p, %d, %p, 0x%08x)\n", pUIInfo,
          debugstr_w(pszTargetName), Reserved, dwAuthError, debugstr_w(pszUsername),
          ulUsernameMaxChars, pszPassword, ulPasswordMaxChars, pfSave, dwFlags);

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

    ret = DialogBoxParamW(hinstCredUI, MAKEINTRESOURCEW(IDD_CREDDIALOG),
                          pUIInfo->hwndParent, CredDialogProc, (LPARAM)&params);
    if (ret <= 0)
        return GetLastError();

    if (ret == IDCANCEL)
    {
        TRACE("dialog cancelled\n");
        return ERROR_CANCELLED;
    }

    if (pfSave)
        *pfSave = params.fSave;

    return ERROR_SUCCESS;
}

/******************************************************************************
 *           CredUIConfirmCredentialsW [CREDUI.@]
 */
DWORD WINAPI CredUIConfirmCredentialsW(PCWSTR pszTargetName, BOOL bConfirm)
{
    FIXME("(%s, %s): stub\n", debugstr_w(pszTargetName),
          bConfirm ? "TRUE" : "FALSE");
    return ERROR_SUCCESS;
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
