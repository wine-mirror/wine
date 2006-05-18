/*
 * Implementation of the ODBC driver installer
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
 * Copyright 2005 Hans Leidekker
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

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "wine/debug.h"

#include "odbcinst.h"

WINE_DEFAULT_DEBUG_CHANNEL(odbc);

BOOL WINAPI ODBCCPlApplet( LONG i, LONG j, LONG * p1, LONG * p2)
{
    FIXME( "( %ld %ld %p %p) : stub!\n", i, j, p1, p2);
    return FALSE;
}

static LPWSTR SQLInstall_strdup_multi(LPCSTR str)
{
    LPCSTR p;
    LPWSTR ret = NULL;
    DWORD len;

    if (!str)
        return ret;

    for (p = str; *p; p += lstrlenA(p) + 1)
        ;

    len = MultiByteToWideChar(CP_ACP, 0, str, p - str, NULL, 0 );
    ret = HeapAlloc(GetProcessHeap(), 0, (len+1)*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, p - str, ret, len );
    ret[len] = 0;

    return ret;
}

static LPWSTR SQLInstall_strdup(LPCSTR str)
{
    DWORD len;
    LPWSTR ret = NULL;

    if (!str)
        return ret;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0 );
    ret = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len );

    return ret;
}

BOOL WINAPI SQLConfigDataSourceW(HWND hwndParent, WORD fRequest,
               LPCWSTR lpszDriver, LPCWSTR lpszAttributes)
{
    LPCWSTR p;

    FIXME("%p %d %s %s\n", hwndParent, fRequest, debugstr_w(lpszDriver),
          debugstr_w(lpszAttributes));

    for (p = lpszAttributes; *p; p += lstrlenW(p) + 1)
        FIXME("%s\n", debugstr_w(p));

    return TRUE;
}

BOOL WINAPI SQLConfigDataSource(HWND hwndParent, WORD fRequest,
               LPCSTR lpszDriver, LPCSTR lpszAttributes)
{
    FIXME("%p %d %s %s\n", hwndParent, fRequest, debugstr_a(lpszDriver),
          debugstr_a(lpszAttributes));
    return TRUE;
}

BOOL WINAPI SQLConfigDriverW(HWND hwndParent, WORD fRequest, LPCWSTR lpszDriver,
               LPCWSTR lpszArgs, LPWSTR lpszMsg, WORD cbMsgMax, WORD *pcbMsgOut)
{
    FIXME("(%p %d %s %s %p %d %p)\n", hwndParent, fRequest, debugstr_w(lpszDriver),
          debugstr_w(lpszArgs), lpszMsg, cbMsgMax, pcbMsgOut);
    return TRUE;
}

BOOL WINAPI SQLConfigDriver(HWND hwndParent, WORD fRequest, LPCSTR lpszDriver,
               LPCSTR lpszArgs, LPSTR lpszMsg, WORD cbMsgMax, WORD *pcbMsgOut)
{
    FIXME("(%p %d %s %s %p %d %p)\n", hwndParent, fRequest, debugstr_a(lpszDriver),
          debugstr_a(lpszArgs), lpszMsg, cbMsgMax, pcbMsgOut);
    return TRUE;
}

BOOL WINAPI SQLCreateDataSourceW(HWND hwnd, LPCWSTR lpszDS)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLCreateDataSource(HWND hwnd, LPCSTR lpszDS)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetAvailableDriversW(LPCWSTR lpszInfFile, LPWSTR lpszBuf,
               WORD cbBufMax, WORD *pcbBufOut)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetAvailableDrivers(LPCSTR lpszInfFile, LPSTR lpszBuf,
               WORD cbBufMax, WORD *pcbBufOut)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetConfigMode(UWORD *pwConfigMode)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetInstalledDriversW(LPWSTR lpszBuf, WORD cbBufMax,
               WORD *pcbBufOut)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetInstalledDrivers(LPSTR lpszBuf, WORD cbBufMax,
               WORD *pcbBufOut)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

int WINAPI SQLGetPrivateProfileStringW(LPCWSTR lpszSection, LPCWSTR lpszEntry,
               LPCWSTR lpszDefault, LPCWSTR RetBuffer, int cbRetBuffer,
               LPCWSTR lpszFilename)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

int WINAPI SQLGetPrivateProfileString(LPCSTR lpszSection, LPCSTR lpszEntry,
               LPCSTR lpszDefault, LPCSTR RetBuffer, int cbRetBuffer,
               LPCSTR lpszFilename)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetTranslatorW(HWND hwndParent, LPWSTR lpszName, WORD cbNameMax,
               WORD *pcbNameOut, LPWSTR lpszPath, WORD cbPathMax,
               WORD *pcbPathOut, DWORD *pvOption)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetTranslator(HWND hwndParent, LPSTR lpszName, WORD cbNameMax,
               WORD *pcbNameOut, LPSTR lpszPath, WORD cbPathMax,
               WORD *pcbPathOut, DWORD *pvOption)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLInstallDriverW(LPCWSTR lpszInfFile, LPCWSTR lpszDriver,
               LPWSTR lpszPath, WORD cbPathMax, WORD * pcbPathOut)
{
    DWORD usage;

    TRACE("%s %s %p %d %p\n", debugstr_w(lpszInfFile),
          debugstr_w(lpszDriver), lpszPath, cbPathMax, pcbPathOut);

    if (lpszInfFile)
        return FALSE;

    return SQLInstallDriverExW(lpszDriver, NULL, lpszPath, cbPathMax,
                               pcbPathOut, ODBC_INSTALL_COMPLETE, &usage);
}

BOOL WINAPI SQLInstallDriver(LPCSTR lpszInfFile, LPCSTR lpszDriver,
               LPSTR lpszPath, WORD cbPathMax, WORD * pcbPathOut)
{
    DWORD usage;

    TRACE("%s %s %p %d %p\n", debugstr_a(lpszInfFile),
          debugstr_a(lpszDriver), lpszPath, cbPathMax, pcbPathOut);

    if (lpszInfFile)
        return FALSE;
   
    return SQLInstallDriverEx(lpszDriver, NULL, lpszPath, cbPathMax,
                              pcbPathOut, ODBC_INSTALL_COMPLETE, &usage);
}

BOOL WINAPI SQLInstallDriverExW(LPCWSTR lpszDriver, LPCWSTR lpszPathIn,
               LPWSTR lpszPathOut, WORD cbPathOutMax, WORD *pcbPathOut,
               WORD fRequest, LPDWORD lpdwUsageCount)
{
    UINT len;
    LPCWSTR p;
    WCHAR path[MAX_PATH];

    TRACE("%s %s %p %d %p %d %p\n", debugstr_w(lpszDriver),
          debugstr_w(lpszPathIn), lpszPathOut, cbPathOutMax, pcbPathOut,
          fRequest, lpdwUsageCount);

    for (p = lpszDriver; *p; p += lstrlenW(p) + 1)
        TRACE("%s\n", debugstr_w(p));

    len = GetSystemDirectoryW(path, MAX_PATH);

    if (pcbPathOut)
        *pcbPathOut = len;

    len = GetSystemDirectoryW(path, MAX_PATH);

    if (lpszPathOut && cbPathOutMax > len)
    {
        lstrcpyW(lpszPathOut, path);
        return TRUE;
    }
    return FALSE;
}

BOOL WINAPI SQLInstallDriverEx(LPCSTR lpszDriver, LPCSTR lpszPathIn,
               LPSTR lpszPathOut, WORD cbPathOutMax, WORD *pcbPathOut,
               WORD fRequest, LPDWORD lpdwUsageCount)
{
    LPCSTR p;
    LPWSTR driver, pathin;
    WCHAR pathout[MAX_PATH];
    BOOL ret;
    WORD cbOut = 0;

    TRACE("%s %s %p %d %p %d %p\n", debugstr_a(lpszDriver),
          debugstr_a(lpszPathIn), lpszPathOut, cbPathOutMax, pcbPathOut,
          fRequest, lpdwUsageCount);

    for (p = lpszDriver; *p; p += lstrlenA(p) + 1)
        TRACE("%s\n", debugstr_a(p));

    driver = SQLInstall_strdup_multi(lpszDriver);
    pathin = SQLInstall_strdup(lpszPathIn);

    ret = SQLInstallDriverExW(driver, pathin, pathout, MAX_PATH, &cbOut,
                              fRequest, lpdwUsageCount);
    if (ret)
    {
        int len =  WideCharToMultiByte(CP_ACP, 0, pathout, -1, lpszPathOut,
                                       0, NULL, NULL);
        if (len)
        {
            if (pcbPathOut)
                *pcbPathOut = len - 1;

            if (!lpszPathOut || cbPathOutMax < len)
            {
                ret = FALSE;
                goto out;
            }
            len =  WideCharToMultiByte(CP_ACP, 0, pathout, -1, lpszPathOut,
                                       cbPathOutMax, NULL, NULL);
        }
    }

out:
    HeapFree(GetProcessHeap(), 0, driver);
    HeapFree(GetProcessHeap(), 0, pathin);
    return ret;
}

BOOL WINAPI SQLInstallDriverManagerW(LPWSTR lpszPath, WORD cbPathMax,
               WORD *pcbPathOut)
{
    UINT len;
    WCHAR path[MAX_PATH];

    TRACE("(%p %d %d)\n", lpszPath, cbPathMax, *pcbPathOut);

    len = GetSystemDirectoryW(path, MAX_PATH);

    if (pcbPathOut)
        *pcbPathOut = len;

    if (lpszPath && cbPathMax > len)
    {
    	lstrcpyW(lpszPath, path);
    	return TRUE;
    }
    return FALSE;
}

BOOL WINAPI SQLInstallDriverManager(LPSTR lpszPath, WORD cbPathMax,
               WORD *pcbPathOut)
{
    BOOL ret;
    WORD len, cbOut = 0;
    WCHAR path[MAX_PATH];

    TRACE("(%p %d %d)\n", lpszPath, cbPathMax, *pcbPathOut);

    ret = SQLInstallDriverManagerW(path, MAX_PATH, &cbOut);
    if (ret)
    {
        len =  WideCharToMultiByte(CP_ACP, 0, path, -1, lpszPath, 0,
                                   NULL, NULL);
        if (len)
        {
            if (pcbPathOut)
                *pcbPathOut = len - 1;

            if (!lpszPath || cbPathMax < len)
                return FALSE;

            len =  WideCharToMultiByte(CP_ACP, 0, path, -1, lpszPath,
                                       cbPathMax, NULL, NULL);
        }
    }
    return ret;
}

BOOL WINAPI SQLInstallODBCW(HWND hwndParent, LPCWSTR lpszInfFile,
               LPCWSTR lpszSrcPath, LPCWSTR lpszDrivers)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLInstallODBC(HWND hwndParent, LPCSTR lpszInfFile,
               LPCSTR lpszSrcPath, LPCSTR lpszDrivers)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

SQLRETURN WINAPI SQLInstallerErrorW(WORD iError, DWORD *pfErrorCode,
               LPWSTR lpszErrorMsg, WORD cbErrorMsgMax, WORD *pcbErrorMsg)
{
    TRACE("%d %p %p %d %p\n", iError, pfErrorCode, lpszErrorMsg,
          cbErrorMsgMax, pcbErrorMsg);

    if (pcbErrorMsg)
        *pcbErrorMsg = 0;

    if (lpszErrorMsg && cbErrorMsgMax > 0)
        *lpszErrorMsg = '\0';

    return SQL_NO_DATA;
}

SQLRETURN WINAPI SQLInstallerError(WORD iError, DWORD *pfErrorCode,
               LPSTR lpszErrorMsg, WORD cbErrorMsgMax, WORD *pcbErrorMsg)
{
    TRACE("%d %p %p %d %p\n", iError, pfErrorCode, lpszErrorMsg,
          cbErrorMsgMax, pcbErrorMsg);

    if (pcbErrorMsg)
        *pcbErrorMsg = 0;

    if (lpszErrorMsg && cbErrorMsgMax > 0)
        *lpszErrorMsg = '\0';

    return SQL_NO_DATA;
}

BOOL WINAPI SQLInstallTranslatorExW(LPCWSTR lpszTranslator, LPCWSTR lpszPathIn,
               LPWSTR lpszPathOut, WORD cbPathOutMax, WORD *pcbPathOut,
               WORD fRequest, LPDWORD lpdwUsageCount)
{
    UINT len;
    LPCWSTR p;
    WCHAR path[MAX_PATH];

    TRACE("%s %s %p %d %p %d %p\n", debugstr_w(lpszTranslator),
          debugstr_w(lpszPathIn), lpszPathOut, cbPathOutMax, pcbPathOut,
          fRequest, lpdwUsageCount);

    for (p = lpszTranslator; *p; p += lstrlenW(p) + 1)
        TRACE("%s\n", debugstr_w(p));

    len = GetSystemDirectoryW(path, MAX_PATH);

    if (pcbPathOut)
        *pcbPathOut = len;

    if (lpszPathOut && cbPathOutMax > len)
    {
        lstrcpyW(lpszPathOut, path);
        return TRUE;
    }
    return FALSE;
}

BOOL WINAPI SQLInstallTranslatorEx(LPCSTR lpszTranslator, LPCSTR lpszPathIn,
               LPSTR lpszPathOut, WORD cbPathOutMax, WORD *pcbPathOut,
               WORD fRequest, LPDWORD lpdwUsageCount)
{
    LPCSTR p;
    LPWSTR translator, pathin;
    WCHAR pathout[MAX_PATH];
    BOOL ret;
    WORD cbOut = 0;

    TRACE("%s %s %p %d %p %d %p\n", debugstr_a(lpszTranslator),
          debugstr_a(lpszPathIn), lpszPathOut, cbPathOutMax, pcbPathOut,
          fRequest, lpdwUsageCount);

    for (p = lpszTranslator; *p; p += lstrlenA(p) + 1)
        TRACE("%s\n", debugstr_a(p));

    translator = SQLInstall_strdup_multi(lpszTranslator);
    pathin = SQLInstall_strdup(lpszPathIn);

    ret = SQLInstallTranslatorExW(translator, pathin, pathout, MAX_PATH,
                                  &cbOut, fRequest, lpdwUsageCount);
    if (ret)
    {
        int len =  WideCharToMultiByte(CP_ACP, 0, pathout, -1, lpszPathOut,
                                       0, NULL, NULL);
        if (len)
        {
            if (pcbPathOut)
                *pcbPathOut = len - 1;

            if (!lpszPathOut || cbPathOutMax < len)
            {
                ret = FALSE;
                goto out;
            }
            len =  WideCharToMultiByte(CP_ACP, 0, pathout, -1, lpszPathOut,
                                       cbPathOutMax, NULL, NULL);
        }
    }

out:
    HeapFree(GetProcessHeap(), 0, translator);
    HeapFree(GetProcessHeap(), 0, pathin);
    return ret;
}

BOOL WINAPI SQLInstallTranslator(LPCSTR lpszInfFile, LPCSTR lpszTranslator,
               LPCSTR lpszPathIn, LPSTR lpszPathOut, WORD cbPathOutMax,
               WORD *pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount)
{
    TRACE("%s %s %s %p %d %p %d %p\n", debugstr_a(lpszInfFile),
          debugstr_a(lpszTranslator), debugstr_a(lpszPathIn), lpszPathOut,
          cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);

    if (lpszInfFile)
        return FALSE;

    return SQLInstallTranslatorEx(lpszTranslator, lpszPathIn, lpszPathOut,
                       cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
}

BOOL WINAPI SQLInstallTranslatorW(LPCWSTR lpszInfFile, LPCWSTR lpszTranslator,
              LPCWSTR lpszPathIn, LPWSTR lpszPathOut, WORD cbPathOutMax,
              WORD *pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount)
{
    TRACE("%s %s %s %p %d %p %d %p\n", debugstr_w(lpszInfFile),
          debugstr_w(lpszTranslator), debugstr_w(lpszPathIn), lpszPathOut,
          cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);

    if (lpszInfFile)
        return FALSE;

    return SQLInstallTranslatorExW(lpszTranslator, lpszPathIn, lpszPathOut,
                        cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
}

BOOL WINAPI SQLManageDataSources(HWND hwnd)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

SQLRETURN WINAPI SQLPostInstallerErrorW(DWORD fErrorCode, LPCWSTR szErrorMsg)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

SQLRETURN WINAPI SQLPostInstallerError(DWORD fErrorCode, LPCSTR szErrorMsg)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLReadFileDSNW(LPCWSTR lpszFileName, LPCWSTR lpszAppName,
               LPCWSTR lpszKeyName, LPWSTR lpszString, WORD cbString,
               WORD *pcbString)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLReadFileDSN(LPCSTR lpszFileName, LPCSTR lpszAppName,
               LPCSTR lpszKeyName, LPSTR lpszString, WORD cbString,
               WORD *pcbString)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveDefaultDataSource(void)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveDriverW(LPCWSTR lpszDriver, BOOL fRemoveDSN,
               LPDWORD lpdwUsageCount)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveDriver(LPCSTR lpszDriver, BOOL fRemoveDSN,
               LPDWORD lpdwUsageCount)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveDriverManager(LPDWORD pdwUsageCount)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveDSNFromIniW(LPCWSTR lpszDSN)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveDSNFromIni(LPCSTR lpszDSN)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveTranslatorW(LPCWSTR lpszTranslator, LPDWORD lpdwUsageCount)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveTranslator(LPCSTR lpszTranslator, LPDWORD lpdwUsageCount)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLSetConfigMode(UWORD wConfigMode)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLValidDSNW(LPCWSTR lpszDSN)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLValidDSN(LPCSTR lpszDSN)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLWriteDSNToIniW(LPCWSTR lpszDSN, LPCWSTR lpszDriver)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLWriteDSNToIni(LPCSTR lpszDSN, LPCSTR lpszDriver)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLWriteFileDSNW(LPCWSTR lpszFileName, LPCWSTR lpszAppName,
               LPCWSTR lpszKeyName, LPCWSTR lpszString)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLWriteFileDSN(LPCSTR lpszFileName, LPCSTR lpszAppName,
               LPCSTR lpszKeyName, LPCSTR lpszString)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLWritePrivateProfileStringW(LPCWSTR lpszSection, LPCWSTR lpszEntry,
               LPCWSTR lpszString, LPCWSTR lpszFilename)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLWritePrivateProfileString(LPCSTR lpszSection, LPCSTR lpszEntry,
               LPCSTR lpszString, LPCSTR lpszFilename)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
