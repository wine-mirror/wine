/*
 * UrlMon URL tests
 *
 * Copyright 2004 Kevin Koltzau
 * Copyright 2004 Jacek Caban
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "urlmon.h"

#include "wine/test.h"

static const WCHAR TEST_URL_1[] = {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','\0'};
static const WCHAR TEST_PART_URL_1[] = {'/','t','e','s','t','/','\0'};

static void test_CreateURLMoniker(LPCWSTR url1, LPCWSTR url2)
{
    HRESULT hr;
    IMoniker *mon1 = NULL;
    IMoniker *mon2 = NULL;

    hr = CreateURLMoniker(NULL, url1, &mon1);
    ok(SUCCEEDED(hr), "failed to create moniker: 0x%08lx\n", hr);
    if(SUCCEEDED(hr)) {
        hr = CreateURLMoniker(mon1, url2, &mon2);
        ok(SUCCEEDED(hr), "failed to create moniker: 0x%08lx\n", hr);
    }
    if(mon1) IMoniker_Release(mon1);
    if(mon2) IMoniker_Release(mon2);
}

static void test_create()
{
    test_CreateURLMoniker(TEST_URL_1, TEST_PART_URL_1);
}

typedef struct {
    IBindStatusCallbackVtbl *lpVtbl;
    ULONG ref;
} statusclb;

static HRESULT WINAPI statusclb_QueryInterface(IBindStatusCallback *iface, REFIID riid, void **ppvObject)
{
    return E_NOINTERFACE;
}

static ULONG WINAPI statusclb_AddRef(IBindStatusCallback *iface)
{
    return InterlockedIncrement(&((statusclb*)iface)->ref);
}

static ULONG WINAPI statusclb_Release(IBindStatusCallback *iface)
{
    statusclb *This = (statusclb*)iface;
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    if(!ref)
        HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

static HRESULT WINAPI statusclb_OnStartBinding(IBindStatusCallback *iface,DWORD dwReserved, IBinding * pib)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI statusclb_GetPriority(IBindStatusCallback *iface, LONG * pnPriority)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI statusclb_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI statusclb_OnProgress(IBindStatusCallback *iface, ULONG ulProgress, ULONG ulProgressMax,
                           ULONG ulStatusCode, LPCWSTR szStatusText)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI statusclb_OnStopBinding(IBindStatusCallback *iface, HRESULT hresult, LPCWSTR szError)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI statusclb_GetBindInfo(IBindStatusCallback *iface, DWORD *grfBINDF, BINDINFO * pbindinfo)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI statusclb_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF, DWORD dwSize,
                           FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI statusclb_OnObjectAvailable(IBindStatusCallback *iface, REFIID riid, IUnknown* punk)
{
    return E_NOTIMPL;
}

static IBindStatusCallbackVtbl statusclbVtbl = {
    statusclb_QueryInterface,
    statusclb_AddRef,
    statusclb_Release,
    statusclb_OnStartBinding,
    statusclb_GetPriority,
    statusclb_OnLowResource,
    statusclb_OnProgress,
    statusclb_OnStopBinding,
    statusclb_GetBindInfo,
    statusclb_OnDataAvailable,
    statusclb_OnObjectAvailable
};

static IBindStatusCallback* statusclb_create()
{
    statusclb *ret = HeapAlloc(GetProcessHeap(), 0, sizeof(statusclb));
    ret->lpVtbl = &statusclbVtbl;
    ret->ref = 1;
    return (IBindStatusCallback*)ret;
}

static void test_CreateAsyncBindCtx()
{
    IBindCtx *bctx = (IBindCtx*)0x0ff00ff0;
    HRESULT hres;
    ULONG ref;
    BIND_OPTS bindopts;
    IBindStatusCallback *bsc = statusclb_create();

    hres = CreateAsyncBindCtx(0, NULL, NULL, &bctx);
    ok(hres == E_INVALIDARG, "CreateAsyncBindCtx failed. expected: E_INVALIDARG, got: %08lx\n", hres);
    ok(bctx == (IBindCtx*)0x0ff00ff0, "bctx should not be changed\n");

    hres = CreateAsyncBindCtx(0, NULL, NULL, NULL);
    ok(hres == E_INVALIDARG, "CreateAsyncBindCtx failed. expected: E_INVALIDARG, got: %08lx\n", hres);

    hres = CreateAsyncBindCtx(0, bsc, NULL, &bctx);
    ok(SUCCEEDED(hres), "CreateAsyncBindCtx failed: %08lx\n", hres);
    if(FAILED(hres)) {
        IBindStatusCallback_Release(bsc);
        return;
    }

    bindopts.cbStruct = 16;
    hres = IBindCtx_GetBindOptions(bctx, &bindopts);
    ok(SUCCEEDED(hres), "IBindCtx_GetBindOptions failed: %08lx\n", hres);
    ok(bindopts.grfFlags == BIND_MAYBOTHERUSER,
                "bindopts.grfFlags = %08lx, expected: BIND_MAYBOTHERUSER\n", bindopts.grfFlags);
    ok(bindopts.grfMode = STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                "bindopts.grfMode = %08lx, expected: STGM_READWRITE | STGM_SHARE_EXCLUSIVE\n", bindopts.grfMode);
    ok(bindopts.dwTickCountDeadline == 0,
                "bindopts.dwTickCountDeadline = %08lx, expected: 0\n", bindopts.dwTickCountDeadline);

    ref = IBindCtx_Release(bctx);
    ok(ref == 0, "bctx should be destroyed here\n");
    ref = IBindStatusCallback_Release(bsc);
    ok(ref == 0, "bsc should be destroyed here\n");
}

START_TEST(url)
{
    test_create();
    test_CreateAsyncBindCtx();
}
