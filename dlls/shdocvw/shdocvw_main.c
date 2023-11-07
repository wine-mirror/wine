/*
 * SHDOCVW - Internet Explorer Web Control
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
 * Copyright 2004 Mike McCormack (for CodeWeavers)
 * Copyright 2008 Detlef Riekenberg
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
#include <stdio.h>

#include "wine/debug.h"

#include "shdocvw.h"

#include "winreg.h"
#include "shlwapi.h"
#include "wininet.h"
#include "isguids.h"

#include "initguid.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

LONG SHDOCVW_refCount = 0;

static HMODULE SHDOCVW_hshell32 = 0;
static HINSTANCE ieframe_instance;

static HINSTANCE get_ieframe_instance(void)
{
    if(!ieframe_instance)
        ieframe_instance = LoadLibraryW(L"ieframe.dll");

    return ieframe_instance;
}

static HRESULT get_ieframe_object(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HINSTANCE ieframe_instance;

    static HRESULT (WINAPI *ieframe_DllGetClassObject)(REFCLSID,REFIID,void**);

    if(!ieframe_DllGetClassObject) {
        ieframe_instance = get_ieframe_instance();
        if(!ieframe_instance)
            return CLASS_E_CLASSNOTAVAILABLE;

        ieframe_DllGetClassObject = (void*)GetProcAddress(ieframe_instance, "DllGetClassObject");
        if(!ieframe_DllGetClassObject)
            return CLASS_E_CLASSNOTAVAILABLE;
    }

    return ieframe_DllGetClassObject(rclsid, riid, ppv);
}

/*************************************************************************
 *              DllGetClassObject (SHDOCVW.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    TRACE("(%s %s %p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if(IsEqualGUID(&CLSID_WebBrowser, rclsid)
       || IsEqualGUID(&CLSID_WebBrowser_V1, rclsid)
       || IsEqualGUID(&CLSID_InternetShortcut, rclsid)
       || IsEqualGUID(&CLSID_CUrlHistory, rclsid)
       || IsEqualGUID(&CLSID_TaskbarList, rclsid))
        return get_ieframe_object(rclsid, riid, ppv);

    /* As a last resort, figure if the CLSID belongs to a 'Shell Instance Object' */
    return SHDOCVW_GetShellInstanceObjectClassObject(rclsid, riid, ppv);
}

/******************************************************************
 *             IEWinMain            (SHDOCVW.101)
 *
 * Only returns on error.
 */
DWORD WINAPI IEWinMain(LPSTR szCommandLine, int nShowWindow)
{
    DWORD (WINAPI *pIEWinMain)(const WCHAR*,int);
    WCHAR *cmdline;
    DWORD ret, len;

    TRACE("%s %d\n", debugstr_a(szCommandLine), nShowWindow);

    pIEWinMain = (void*)GetProcAddress(get_ieframe_instance(), MAKEINTRESOURCEA(101));
    if(!pIEWinMain)
        ExitProcess(1);

    len = MultiByteToWideChar(CP_ACP, 0, szCommandLine, -1, NULL, 0);
    cmdline = malloc(len * sizeof(WCHAR));
    if(!cmdline)
        ExitProcess(1);
    MultiByteToWideChar(CP_ACP, 0, szCommandLine, -1, cmdline, len);

    ret = pIEWinMain(cmdline, nShowWindow);

    free(cmdline);
    return ret;
}

/*************************************************************************
 * SHDOCVW DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%lx %p\n", hinst, fdwReason, fImpLoad);
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinst);
        break;
    case DLL_PROCESS_DETACH:
        if (fImpLoad) break;
        if (SHDOCVW_hshell32) FreeLibrary(SHDOCVW_hshell32);
        if (ieframe_instance) FreeLibrary(ieframe_instance);
        break;
    }
    return TRUE;
}

/*************************************************************************
 *              DllCanUnloadNow (SHDOCVW.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return SHDOCVW_refCount ? S_FALSE : S_OK;
}

/***********************************************************************
 *              DllGetVersion (SHDOCVW.@)
 */
HRESULT WINAPI DllGetVersion(DLLVERSIONINFO *info)
{
    if (info->cbSize != sizeof(DLLVERSIONINFO)) FIXME("support DLLVERSIONINFO2\n");

    /* this is what IE6 on Windows 98 reports */
    info->dwMajorVersion = 6;
    info->dwMinorVersion = 0;
    info->dwBuildNumber = 2600;
    info->dwPlatformID = DLLVER_PLATFORM_WINDOWS;

    return NOERROR;
}

/*************************************************************************
 *              DllInstall (SHDOCVW.@)
 */
HRESULT WINAPI DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
   FIXME("(%s, %s): stub!\n", bInstall ? "TRUE":"FALSE", debugstr_w(cmdline));

   return S_OK;
}

/*************************************************************************
 * SHDOCVW_LoadShell32
 *
 * makes sure the handle to shell32 is valid
 */
static BOOL SHDOCVW_LoadShell32(void)
{
     if (SHDOCVW_hshell32)
       return TRUE;
     return ((SHDOCVW_hshell32 = LoadLibraryA("shell32.dll")) != NULL);
}

/***********************************************************************
 *		@ (SHDOCVW.110)
 *
 * Called by Win98 explorer.exe main binary, definitely has 0
 * parameters.
 */
DWORD WINAPI WinList_Init(void)
{
    FIXME("(), stub!\n");
    return 0x0deadfeed;
}

/***********************************************************************
 *		@ (SHDOCVW.118)
 *
 * Called by Win98 explorer.exe main binary, definitely has only one
 * parameter.
 */
static BOOL (WINAPI *pShellDDEInit)(BOOL start) = NULL;

BOOL WINAPI ShellDDEInit(BOOL start)
{
    TRACE("(%d)\n", start);

    if (!pShellDDEInit)
    {
      if (!SHDOCVW_LoadShell32())
        return FALSE;
      pShellDDEInit = (void *)GetProcAddress(SHDOCVW_hshell32, (LPCSTR)188);
    }

    if (pShellDDEInit)
      return pShellDDEInit(start);
    else
      return FALSE;
}

/***********************************************************************
 *		@ (SHDOCVW.125)
 *
 * Called by Win98 explorer.exe main binary, definitely has 0
 * parameters.
 */
DWORD WINAPI RunInstallUninstallStubs(void)
{
    FIXME("(), stub!\n");
    return 0x0deadbee;
}

/***********************************************************************
 *              @ (SHDOCVW.130)
 *
 * Called by Emerge Desktop (alternative Windows Shell).
 */
DWORD WINAPI RunInstallUninstallStubs2(int arg)
{
    FIXME("(%d), stub!\n", arg);
    return 0x0deadbee;
}

/***********************************************************************
 *              SetQueryNetSessionCount (SHDOCVW.@)
 */
DWORD WINAPI SetQueryNetSessionCount(DWORD arg)
{
    FIXME("(%lu), stub!\n", arg);
    return 0;
}

/**********************************************************************
 * Some forwards (by ordinal) to SHLWAPI
 */

static void* fetch_shlwapi_ordinal(UINT_PTR ord)
{
    static HANDLE h;

    if (!h && !(h = GetModuleHandleW(L"shlwapi.dll"))) return NULL;
    return (void*)GetProcAddress(h, (const char*)ord);
}

/******************************************************************
 *		StopWatchModeFORWARD            (SHDOCVW.@)
 */
void WINAPI StopWatchModeFORWARD(void)
{
    static void (WINAPI *p)(void);

    if (p || (p = fetch_shlwapi_ordinal(241))) p();
}

/******************************************************************
 *		StopWatchFlushFORWARD            (SHDOCVW.@)
 */
void WINAPI StopWatchFlushFORWARD(void)
{
    static void (WINAPI *p)(void);

    if (p || (p = fetch_shlwapi_ordinal(242))) p();
}

/******************************************************************
 *		StopWatchAFORWARD            (SHDOCVW.@)
 */
DWORD WINAPI StopWatchAFORWARD(DWORD dwClass, LPCSTR lpszStr, DWORD dwUnknown,
                               DWORD dwMode, DWORD dwTimeStamp)
{
    static DWORD (WINAPI *p)(DWORD, LPCSTR, DWORD, DWORD, DWORD);

    if (p || (p = fetch_shlwapi_ordinal(243)))
        return p(dwClass, lpszStr, dwUnknown, dwMode, dwTimeStamp);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************
 *		StopWatchWFORWARD            (SHDOCVW.@)
 */
DWORD WINAPI StopWatchWFORWARD(DWORD dwClass, LPCWSTR lpszStr, DWORD dwUnknown,
                               DWORD dwMode, DWORD dwTimeStamp)
{
    static DWORD (WINAPI *p)(DWORD, LPCWSTR, DWORD, DWORD, DWORD);

    if (p || (p = fetch_shlwapi_ordinal(244)))
        return p(dwClass, lpszStr, dwUnknown, dwMode, dwTimeStamp);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************
 *  URLSubRegQueryA (SHDOCVW.151)
 */
HRESULT WINAPI URLSubRegQueryA(LPCSTR regpath, LPCSTR name, DWORD type,
                               LPSTR out, DWORD outlen, DWORD unknown)
{
    CHAR buffer[INTERNET_MAX_URL_LENGTH];
    DWORD len;
    LONG res;

    TRACE("(%s, %s, %ld, %p, %ld, %ld)\n", debugstr_a(regpath), debugstr_a(name),
            type, out, outlen, unknown);

    if (!out) return S_OK;

    len = sizeof(buffer);
    res = SHRegGetUSValueA(regpath, name, NULL, buffer,  &len, FALSE, NULL, 0);
    if (!res) {
        lstrcpynA(out, buffer, outlen);
        return S_OK;
    }

    return E_FAIL;
}

/******************************************************************
 *  ParseURLFromOutsideSourceW (SHDOCVW.170)
 */
DWORD WINAPI ParseURLFromOutsideSourceW(LPCWSTR url, LPWSTR out, LPDWORD plen, LPDWORD unknown)
{
    WCHAR buffer_in[INTERNET_MAX_URL_LENGTH];
    WCHAR buffer_out[INTERNET_MAX_URL_LENGTH];
    LPCWSTR ptr = url;
    HRESULT hr;
    DWORD needed;
    DWORD len;
    DWORD res = 0;


    TRACE("(%s, %p, %p, %p) len: %ld, unknown: 0x%lx\n", debugstr_w(url), out, plen, unknown,
            plen ? *plen : 0, unknown ? *unknown : 0);

    if (!PathIsURLW(ptr)) {
        len = ARRAY_SIZE(buffer_in);
        buffer_in[0] = 0;
        hr = UrlApplySchemeW(ptr, buffer_in, &len, URL_APPLY_GUESSSCHEME | URL_APPLY_DEFAULT);
        TRACE("got 0x%lx with %s\n", hr, debugstr_w(buffer_in));
        if (hr == S_OK) {
            /* we parsed the url to buffer_in */
            ptr = buffer_in;
        }
        else
        {
            FIXME("call search hook for %s\n", debugstr_w(ptr));
        }
    }

    len = ARRAY_SIZE(buffer_out);
    buffer_out[0] = '\0';
    hr = UrlCanonicalizeW(ptr, buffer_out, &len, URL_ESCAPE_SPACES_ONLY);
    needed = lstrlenW(buffer_out)+1;
    TRACE("got 0x%lx with %s (need %ld)\n", hr, debugstr_w(buffer_out), needed);

    if (*plen >= needed) {
        if (out != NULL) {
            lstrcpyW(out, buffer_out);
            res++;
        }
        needed--;
    }

    *plen = needed;

    TRACE("=> %ld\n", res);
    return res;
}

/******************************************************************
 *  ParseURLFromOutsideSourceA (SHDOCVW.169)
 *
 * See ParseURLFromOutsideSourceW
 */
DWORD WINAPI ParseURLFromOutsideSourceA(LPCSTR url, LPSTR out, LPDWORD plen, LPDWORD unknown)
{
    WCHAR buffer[INTERNET_MAX_URL_LENGTH];
    LPWSTR urlW = NULL;
    DWORD needed;
    DWORD res;
    DWORD len;

    TRACE("(%s, %p, %p, %p) len: %ld, unknown: 0x%lx\n", debugstr_a(url), out, plen, unknown,
            plen ? *plen : 0, unknown ? *unknown : 0);

    if (url) {
        len = MultiByteToWideChar(CP_ACP, 0, url, -1, NULL, 0);
        urlW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, url, -1, urlW, len);
    }

    len = ARRAY_SIZE(buffer);
    ParseURLFromOutsideSourceW(urlW, buffer, &len, unknown);
    free(urlW);

    needed = WideCharToMultiByte(CP_ACP, 0, buffer, -1, NULL, 0, NULL, NULL);

    res = 0;
    if (*plen >= needed) {
        if (out != NULL) {
            WideCharToMultiByte(CP_ACP, 0, buffer, -1, out, *plen, NULL, NULL);
            res = needed;
        }
        needed--;
    }

    *plen = needed;

    TRACE("=> %ld\n", res);
    return res;
}

/******************************************************************
 *  IEParseDisplayNameWithBCW (SHDOCVW.218)
 */
HRESULT WINAPI IEParseDisplayNameWithBCW(DWORD codepage, LPCWSTR lpszDisplayName, LPBC pbc, LPITEMIDLIST *ppidl)
{
    /* Guessing at parameter 3 based on IShellFolder's  ParseDisplayName */
    FIXME("stub: 0x%lx %s %p %p\n",codepage,debugstr_w(lpszDisplayName),pbc,ppidl);
    return E_FAIL;
}

/******************************************************************
 *  SHRestricted2W (SHDOCVW.159)
 */
DWORD WINAPI SHRestricted2W(DWORD res, LPCWSTR url, DWORD reserved)
{
    FIXME("(%ld %s %ld) stub\n", res, debugstr_w(url), reserved);
    return 0;
}

/******************************************************************
 * SHRestricted2A (SHDOCVW.158)
 *
 * See SHRestricted2W
 */
DWORD WINAPI SHRestricted2A(DWORD restriction, LPCSTR url, DWORD reserved)
{
    LPWSTR urlW = NULL;
    DWORD res;

    TRACE("(%ld, %s, %ld)\n", restriction, debugstr_a(url), reserved);
    if (url) {
        DWORD len = MultiByteToWideChar(CP_ACP, 0, url, -1, NULL, 0);
        urlW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, url, -1, urlW, len);
    }
    res = SHRestricted2W(restriction, urlW, reserved);
    free(urlW);
    return res;
}

/******************************************************************
 * ImportPrivacySettings (SHDOCVW.@)
 *
 * Import global and/or per site privacy preferences from an xml file
 *
 * PARAMS
 *  filename      [I] XML file to use
 *  pGlobalPrefs  [IO] PTR to a usage flag for the global privacy preferences
 *  pPerSitePrefs [IO] PTR to a usage flag for the per site privacy preferences
 *
 * RETURNS
 *  Success: TRUE  (the privacy preferences where updated)
 *  Failure: FALSE (the privacy preferences are unchanged)
 *
 * NOTES
 *  Set the flag to TRUE, when the related privacy preferences in the xml file
 *  should be used (parsed and overwrite the current settings).
 *  On return, the flag is TRUE, when the related privacy settings where used
 *
 */
BOOL WINAPI ImportPrivacySettings(LPCWSTR filename, BOOL *pGlobalPrefs, BOOL * pPerSitePrefs)
{
    FIXME("(%s, %p->%d, %p->%d): stub\n", debugstr_w(filename),
        pGlobalPrefs, pGlobalPrefs ? *pGlobalPrefs : 0,
        pPerSitePrefs, pPerSitePrefs ? *pPerSitePrefs : 0);

    if (pGlobalPrefs) *pGlobalPrefs = FALSE;
    if (pPerSitePrefs) *pPerSitePrefs = FALSE;

    return TRUE;
}

/******************************************************************
 * ResetProfileSharing (SHDOCVW.164)
 */
HRESULT WINAPI ResetProfileSharing(HWND hwnd)
{
    FIXME("(%p) stub\n", hwnd);
    return E_NOTIMPL;
}

/******************************************************************
 * InstallReg_RunDLL (SHDOCVW.@)
 */
void WINAPI InstallReg_RunDLL(HWND hwnd, HINSTANCE handle, LPCSTR cmdline, INT show)
{
    FIXME("(%p %p %s %x)\n", hwnd, handle, debugstr_a(cmdline), show);
}

/******************************************************************
 * DoFileDownload (SHDOCVW.@)
 */
BOOL WINAPI DoFileDownload(LPWSTR filename)
{
    FIXME("(%s) stub\n", debugstr_w(filename));
    return FALSE;
}

/******************************************************************
 * DoOrganizeFavDlgW (SHDOCVW.@)
 */
BOOL WINAPI DoOrganizeFavDlgW(HWND hwnd, LPCWSTR initDir)
{
    FIXME("(%p %s) stub\n", hwnd, debugstr_w(initDir));
    return FALSE;
}

/******************************************************************
 * DoOrganizeFavDlg (SHDOCVW.@)
 */
BOOL WINAPI DoOrganizeFavDlg(HWND hwnd, LPCSTR initDir)
{
    LPWSTR initDirW = NULL;
    BOOL res;

    TRACE("(%p %s)\n", hwnd, debugstr_a(initDir));

    if (initDir) {
        DWORD len = MultiByteToWideChar(CP_ACP, 0, initDir, -1, NULL, 0);
        initDirW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, initDir, -1, initDirW, len);
    }
    res = DoOrganizeFavDlgW(hwnd, initDirW);
    free(initDirW);
    return res;
}
