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

static const WCHAR WINE_ABOUT_URL[] = {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.',
                                       'o','r','g','/','s','i','t','e','/','a','b','o','u','t',0};
static BOOL stopped_binding = FALSE;

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

static void test_create(void)
{
    test_CreateURLMoniker(TEST_URL_1, TEST_PART_URL_1);
}

typedef struct {
    const IBindStatusCallbackVtbl *lpVtbl;
    ULONG ref;
    IBinding *pbind;
    IStream *pstr;
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

static HRESULT WINAPI statusclb_OnStartBinding(IBindStatusCallback *iface, DWORD dwReserved, IBinding *pib)
{
    statusclb *This = (statusclb*)iface;
    HRESULT hres;
    IMoniker *mon;

    This->pbind = pib;
    ok(pib != NULL, "pib should not be NULL\n");
    if(pib)
        IBinding_AddRef(pib);

    hres = IBinding_QueryInterface(pib, &IID_IMoniker, (void**)&mon);
    ok(hres == E_NOINTERFACE, "IBinding should not have IMoniker interface\n");
    if(SUCCEEDED(hres))
        IMoniker_Release(mon);

    return S_OK;
}

static HRESULT WINAPI statusclb_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
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
    return S_OK;
}

static HRESULT WINAPI statusclb_OnStopBinding(IBindStatusCallback *iface, HRESULT hresult, LPCWSTR szError)
{
    statusclb *This = (statusclb*)iface;

    ok(SUCCEEDED(hresult), "Download failed: %08lx\n", hresult);
    ok(szError == NULL, "szError should be NULL\n");
    stopped_binding = TRUE;
    IBinding_Release(This->pbind);
    ok(This->pstr != NULL, "pstr should not be NULL here\n");
    if(This->pstr)
        IStream_Release(This->pstr);

    return S_OK;
}

static HRESULT WINAPI statusclb_GetBindInfo(IBindStatusCallback *iface, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    DWORD cbSize;

    *grfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
    cbSize = pbindinfo->cbSize;
    memset(pbindinfo, 0, cbSize);
    pbindinfo->cbSize = cbSize;

    return S_OK;
}

static HRESULT WINAPI statusclb_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF, DWORD dwSize,
                                                FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    statusclb *This = (statusclb*)iface;
    HRESULT hres;
    DWORD readed;
    BYTE buf[512];
    if(!This->pstr) {
        ok(grfBSCF & BSCF_FIRSTDATANOTIFICATION, "pstr should be set when BSCF_FIRSTDATANOTIFICATION\n");
        This->pstr = U(*pstgmed).pstm;
        IStream_AddRef(This->pstr);
        ok(This->pstr != NULL, "pstr should not be NULL here\n");
    }

    do hres = IStream_Read(This->pstr, buf, 512, &readed);
    while(hres == S_OK);
    ok(hres == S_FALSE || hres == E_PENDING, "IStream_Read returned %08lx\n", hres);

    return S_OK;
}

static HRESULT WINAPI statusclb_OnObjectAvailable(IBindStatusCallback *iface, REFIID riid, IUnknown *punk)
{
    return E_NOTIMPL;
}

static const IBindStatusCallbackVtbl statusclbVtbl = {
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

static IBindStatusCallback* statusclb_create(void)
{
    statusclb *ret = HeapAlloc(GetProcessHeap(), 0, sizeof(statusclb));
    ret->lpVtbl = &statusclbVtbl;
    ret->ref = 1;
    ret->pbind = NULL;
    ret->pstr = NULL;
    return (IBindStatusCallback*)ret;
}

static void test_CreateAsyncBindCtx(void)
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

static void test_BindToStorage(void)
{
    IMoniker *mon;
    HRESULT hres;
    LPOLESTR display_name;
    IBindCtx *bctx;
    MSG msg;
    IBindStatusCallback *previousclb, *sclb = statusclb_create();
    IUnknown *unk = (IUnknown*)0x00ff00ff;
    IBinding *bind;

    hres = CreateAsyncBindCtx(0, sclb, NULL, &bctx);
    ok(SUCCEEDED(hres), "CreateAsyncBindCtx failed: %08lx\n\n", hres);
    if(FAILED(hres)) {
        IBindStatusCallback_Release(sclb);
        return;
    }

    hres = RegisterBindStatusCallback(bctx, sclb, &previousclb, 0);
    ok(SUCCEEDED(hres), "RegisterBindStatusCallback failed: %08lx\n", hres);
    ok(previousclb == sclb, "previousclb(%p) != sclb(%p)\n", previousclb, sclb);
    if(previousclb)
        IBindStatusCallback_Release(previousclb);

    hres = CreateURLMoniker(NULL, WINE_ABOUT_URL, &mon);
    ok(SUCCEEDED(hres), "failed to create moniker: %08lx\n", hres);
    if(FAILED(hres)) {
        IBindStatusCallback_Release(sclb);
        IBindCtx_Release(bctx);
        return;
    }

    hres = IMoniker_QueryInterface(mon, &IID_IBinding, (void**)&bind);
    ok(hres == E_NOINTERFACE, "IMoniker should not have IBinding interface\n");
    if(SUCCEEDED(hres))
        IBinding_Release(bind);

    hres = IMoniker_GetDisplayName(mon, bctx, NULL, &display_name);
    ok(SUCCEEDED(hres), "GetDisplayName failed %08lx\n", hres);
    ok(!lstrcmpW(display_name, WINE_ABOUT_URL), "GetDisplayName got wrong name\n");

    hres = IMoniker_BindToStorage(mon, bctx, NULL, &IID_IStream, (void**)&unk);
    ok(SUCCEEDED(hres), "IMoniker_BindToStorage failed: %08lx\n", hres);
    todo_wine {
        ok(unk == NULL, "istr should be NULL\n");
    }
    if(FAILED(hres)) {
        IBindStatusCallback_Release(sclb);
        IMoniker_Release(mon);
        return;
    }
    if(unk)
        IUnknown_Release(unk);

    while(!stopped_binding && GetMessage(&msg,NULL,0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ok(IMoniker_Release(mon) == 0, "mon should be destroyed here\n");
    ok(IBindCtx_Release(bctx) == 0, "bctx should be destroyed here\n");
    ok(IBindStatusCallback_Release(sclb) == 0, "scbl should be destroyed here\n");
}

START_TEST(url)
{
    test_create();
    test_CreateAsyncBindCtx();
    test_BindToStorage();
}
