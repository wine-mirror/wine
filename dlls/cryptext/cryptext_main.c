/*
 * Crypto Shell Extensions
 *
 * Copyright 2014 Austin English
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
#include "winnls.h"
#include "wincrypt.h"
#include "winuser.h"
#include "cryptuiapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptext);

static inline WCHAR *strdupAW(const char *src)
{
    WCHAR *dst = NULL;
    if (src)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
        dst = malloc(len * sizeof(WCHAR));
        if (dst) MultiByteToWideChar(CP_ACP, 0, src, -1, dst, len);
    }
    return dst;
}

/***********************************************************************
 * CryptExtAddPFX (CRYPTEXT.@)
 */
HRESULT WINAPI CryptExtAddPFX(LPCSTR filename)
{
    FIXME("stub: %s\n", debugstr_a(filename));
    return E_NOTIMPL;
}

/***********************************************************************
 * CryptExtAddPFXW (CRYPTEXT.@)
 */
HRESULT WINAPI CryptExtAddPFXW(LPCWSTR filename)
{
    FIXME("stub: %s\n", debugstr_w(filename));
    return E_NOTIMPL;
}

/***********************************************************************
 * CryptExtOpenCERW (CRYPTEXT.@)
 */
HRESULT WINAPI CryptExtOpenCERW(HWND hwnd, HINSTANCE hinst, LPCWSTR filename, DWORD showcmd)
{
    PCCERT_CONTEXT ctx;
    CRYPTUI_VIEWCERTIFICATE_STRUCTW info;

    TRACE("(%p, %p, %s, %lu)\n", hwnd, hinst, debugstr_w(filename), showcmd);

    if (!CryptQueryObject(CERT_QUERY_OBJECT_FILE, filename, CERT_QUERY_CONTENT_FLAG_CERT,
                          CERT_QUERY_FORMAT_FLAG_ALL, 0, NULL, NULL, NULL, NULL, NULL,
                          (const void **)&ctx))
    {
        /* FIXME: move to the resources */
        MessageBoxW(NULL, L"This is not a valid certificate", filename, MB_OK | MB_ICONERROR);
        return S_OK; /* according to the tests */
    }

    memset(&info, 0, sizeof(info));
    info.dwSize = sizeof(info);
    info.hwndParent = hwnd;
    info.pCertContext = ctx;
    CryptUIDlgViewCertificateW(&info, NULL);
    CertFreeCertificateContext(ctx);

    return S_OK;
}

/***********************************************************************
 * CryptExtOpenCER (CRYPTEXT.@)
 */
HRESULT WINAPI CryptExtOpenCER(HWND hwnd, HINSTANCE hinst, LPCSTR filename, DWORD showcmd)
{
    HRESULT hr;
    LPWSTR filenameW;

    TRACE("(%p, %p, %s, %lu)\n", hwnd, hinst, debugstr_a(filename), showcmd);

    filenameW = strdupAW(filename);
    hr = CryptExtOpenCERW(hwnd, hinst, filenameW, showcmd);
    free(filenameW);
    return hr;
}
