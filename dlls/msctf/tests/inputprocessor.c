/*
 * Unit tests for ITfInputProcessor
 *
 * Copyright 2009 Aric Stewart, CodeWeavers
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

#include <stdio.h>

#define COBJMACROS
#include "wine/test.h"
#include "winuser.h"
#include "initguid.h"
#include "shlwapi.h"
#include "shlguid.h"
#include "comcat.h"
#include "msctf.h"

static ITfInputProcessorProfiles* g_ipp;
static LANGID gLangid;
static ITfCategoryMgr * g_cm = NULL;
static ITfThreadMgr* g_tm = NULL;

static DWORD tmSinkCookie;
static DWORD tmSinkRefCount;

HRESULT RegisterTextService(REFCLSID rclsid);
HRESULT UnregisterTextService();
HRESULT ThreadMgrEventSink_Constructor(IUnknown **ppOut);

DEFINE_GUID(CLSID_FakeService, 0xEDE1A7AD,0x66DE,0x47E0,0xB6,0x20,0x3E,0x92,0xF8,0x24,0x6B,0xF3);
DEFINE_GUID(CLSID_TF_InputProcessorProfiles, 0x33c53a50,0xf456,0x4884,0xb0,0x49,0x85,0xfd,0x64,0x3e,0xcf,0xed);
DEFINE_GUID(CLSID_TF_CategoryMgr,         0xA4B544A1,0x438D,0x4B41,0x93,0x25,0x86,0x95,0x23,0xE2,0xD6,0xC7);
DEFINE_GUID(GUID_TFCAT_TIP_KEYBOARD,     0x34745c63,0xb2f0,0x4784,0x8b,0x67,0x5e,0x12,0xc8,0x70,0x1a,0x31);
DEFINE_GUID(GUID_TFCAT_TIP_SPEECH,       0xB5A73CD1,0x8355,0x426B,0xA1,0x61,0x25,0x98,0x08,0xF2,0x6B,0x14);
DEFINE_GUID(GUID_TFCAT_TIP_HANDWRITING,  0x246ecb87,0xc2f2,0x4abe,0x90,0x5b,0xc8,0xb3,0x8a,0xdd,0x2c,0x43);
DEFINE_GUID (GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,  0x046B8C80,0x1647,0x40F7,0x9B,0x21,0xB9,0x3B,0x81,0xAA,0xBC,0x1B);
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(CLSID_TF_ThreadMgr,           0x529a9e6b,0x6587,0x4f23,0xab,0x9e,0x9c,0x7d,0x68,0x3e,0x3c,0x50);


static HRESULT initialize(void)
{
    HRESULT hr;
    CoInitialize(NULL);
    hr = CoCreateInstance (&CLSID_TF_InputProcessorProfiles, NULL,
          CLSCTX_INPROC_SERVER, &IID_ITfInputProcessorProfiles, (void**)&g_ipp);
    if (SUCCEEDED(hr))
        hr = CoCreateInstance (&CLSID_TF_CategoryMgr, NULL,
          CLSCTX_INPROC_SERVER, &IID_ITfCategoryMgr, (void**)&g_cm);
    if (SUCCEEDED(hr))
        hr = CoCreateInstance (&CLSID_TF_ThreadMgr, NULL,
          CLSCTX_INPROC_SERVER, &IID_ITfThreadMgr, (void**)&g_tm);
    return hr;
}

static void cleanup(void)
{
    if (g_ipp)
        ITfInputProcessorProfiles_Release(g_ipp);
    if (g_cm)
        ITfCategoryMgr_Release(g_cm);
    if (g_tm)
        ITfThreadMgr_Release(g_tm);
    CoUninitialize();
}

static void test_Register(void)
{
    HRESULT hr;

    static const WCHAR szDesc[] = {'F','a','k','e',' ','W','i','n','e',' ','S','e','r','v','i','c','e',0};
    static const WCHAR szFile[] = {'F','a','k','e',' ','W','i','n','e',' ','S','e','r','v','i','c','e',' ','F','i','l','e',0};

    hr = RegisterTextService(&CLSID_FakeService);
    ok(SUCCEEDED(hr),"Unable to register COM for TextService\n");
    hr = ITfInputProcessorProfiles_Register(g_ipp, &CLSID_FakeService);
    ok(SUCCEEDED(hr),"Unable to register text service(%x)\n",hr);
    hr = ITfInputProcessorProfiles_AddLanguageProfile(g_ipp, &CLSID_FakeService, gLangid, &CLSID_FakeService, szDesc, sizeof(szDesc)/sizeof(WCHAR), szFile, sizeof(szFile)/sizeof(WCHAR), 1);
    ok(SUCCEEDED(hr),"Unable to add Language Profile (%x)\n",hr);
}

static void test_Unregister(void)
{
    HRESULT hr;
    hr = ITfInputProcessorProfiles_Unregister(g_ipp, &CLSID_FakeService);
    ok(SUCCEEDED(hr),"Unable to unregister text service(%x)\n",hr);
    UnregisterTextService();
}

static void test_EnumInputProcessorInfo(void)
{
    IEnumGUID *ppEnum;
    BOOL found = FALSE;

    if (SUCCEEDED(ITfInputProcessorProfiles_EnumInputProcessorInfo(g_ipp, &ppEnum)))
    {
        ULONG fetched;
        GUID g;
        while (IEnumGUID_Next(ppEnum, 1, &g, &fetched) == S_OK)
        {
            if(IsEqualGUID(&g,&CLSID_FakeService))
                found = TRUE;
        }
    }
    ok(found,"Did not find registered text service\n");
}

static void test_EnumLanguageProfiles(void)
{
    BOOL found = FALSE;
    IEnumTfLanguageProfiles *ppEnum;
    if (SUCCEEDED(ITfInputProcessorProfiles_EnumLanguageProfiles(g_ipp,gLangid,&ppEnum)))
    {
        TF_LANGUAGEPROFILE profile;
        while (IEnumTfLanguageProfiles_Next(ppEnum,1,&profile,NULL)==S_OK)
        {
            if (IsEqualGUID(&profile.clsid,&CLSID_FakeService))
            {
                found = TRUE;
                ok(profile.langid == gLangid, "LangId Incorrect\n");
                ok(IsEqualGUID(&profile.catid,&GUID_TFCAT_TIP_KEYBOARD), "CatId Incorrect\n");
                ok(IsEqualGUID(&profile.guidProfile,&CLSID_FakeService), "guidProfile Incorrect\n");
            }
        }
    }
    ok(found,"Registered text service not found\n");
}

static void test_RegisterCategory(void)
{
    HRESULT hr;
    hr = ITfCategoryMgr_RegisterCategory(g_cm, &CLSID_FakeService, &GUID_TFCAT_TIP_KEYBOARD, &CLSID_FakeService);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_RegisterCategory failed\n");
    hr = ITfCategoryMgr_RegisterCategory(g_cm, &CLSID_FakeService, &GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER, &CLSID_FakeService);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_RegisterCategory failed\n");
}

static void test_UnregisterCategory(void)
{
    HRESULT hr;
    hr = ITfCategoryMgr_UnregisterCategory(g_cm, &CLSID_FakeService, &GUID_TFCAT_TIP_KEYBOARD, &CLSID_FakeService);
    todo_wine ok(SUCCEEDED(hr),"ITfCategoryMgr_UnregisterCategory failed\n");
    hr = ITfCategoryMgr_UnregisterCategory(g_cm, &CLSID_FakeService, &GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER, &CLSID_FakeService);
    todo_wine ok(SUCCEEDED(hr),"ITfCategoryMgr_UnregisterCategory failed\n");
}

static void test_FindClosestCategory(void)
{
    GUID output;
    HRESULT hr;
    const GUID *list[3] = {&GUID_TFCAT_TIP_SPEECH, &GUID_TFCAT_TIP_KEYBOARD, &GUID_TFCAT_TIP_HANDWRITING};

    hr = ITfCategoryMgr_FindClosestCategory(g_cm, &CLSID_FakeService, &output, NULL, 0);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_FindClosestCategory failed (%x)\n",hr);
    ok(IsEqualGUID(&output,&GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER),"Wrong GUID\n");

    hr = ITfCategoryMgr_FindClosestCategory(g_cm, &CLSID_FakeService, &output, list, 1);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_FindClosestCategory failed (%x)\n",hr);
    ok(IsEqualGUID(&output,&GUID_NULL),"Wrong GUID\n");

    hr = ITfCategoryMgr_FindClosestCategory(g_cm, &CLSID_FakeService, &output, list, 3);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_FindClosestCategory failed (%x)\n",hr);
    ok(IsEqualGUID(&output,&GUID_TFCAT_TIP_KEYBOARD),"Wrong GUID\n");
}

static void test_Enable(void)
{
    HRESULT hr;
    BOOL enabled = FALSE;

    hr = ITfInputProcessorProfiles_EnableLanguageProfile(g_ipp,&CLSID_FakeService, gLangid, &CLSID_FakeService, TRUE);
    ok(SUCCEEDED(hr),"Failed to enable text service\n");
    hr = ITfInputProcessorProfiles_IsEnabledLanguageProfile(g_ipp,&CLSID_FakeService, gLangid, &CLSID_FakeService, &enabled);
    ok(SUCCEEDED(hr),"Failed to get enabled state\n");
    ok(enabled == TRUE,"enabled state incorrect\n");
}

static void test_Disable(void)
{
    HRESULT hr;

    trace("Disabling\n");
    hr = ITfInputProcessorProfiles_EnableLanguageProfile(g_ipp,&CLSID_FakeService, gLangid, &CLSID_FakeService, FALSE);
    ok(SUCCEEDED(hr),"Failed to disable text service\n");
}

static void test_ThreadMgrAdviseSinks(void)
{
    ITfSource *source = NULL;
    HRESULT hr;
    IUnknown *sink;

    hr = ITfThreadMgr_QueryInterface(g_tm, &IID_ITfSource, (LPVOID*)&source);
    ok(SUCCEEDED(hr),"Failed to get IID_ITfSource for ThreadMgr\n");
    if (!source)
        return;

    ThreadMgrEventSink_Constructor(&sink);

    tmSinkRefCount = 1;
    tmSinkCookie = 0;
    hr = ITfSource_AdviseSink(source,&IID_ITfThreadMgrEventSink, sink, &tmSinkCookie);
    ok(SUCCEEDED(hr),"Failed to Advise Sink\n");
    ok(tmSinkCookie!=0,"Failed to get sink cookie\n");

    /* Advising the sink adds a ref, Relesing here lets the object be deleted
       when unadvised */
    tmSinkRefCount = 2;
    IUnknown_Release(sink);
    ITfSource_Release(source);
}

static void test_ThreadMgrUnadviseSinks(void)
{
    ITfSource *source = NULL;
    HRESULT hr;

    hr = ITfThreadMgr_QueryInterface(g_tm, &IID_ITfSource, (LPVOID*)&source);
    ok(SUCCEEDED(hr),"Failed to get IID_ITfSource for ThreadMgr\n");
    if (!source)
        return;

    tmSinkRefCount = 1;
    hr = ITfSource_UnadviseSink(source, tmSinkCookie);
    ok(SUCCEEDED(hr),"Failed to unadvise Sink\n");
    ITfSource_Release(source);
}

START_TEST(inputprocessor)
{
    if (SUCCEEDED(initialize()))
    {
        gLangid = GetUserDefaultLCID();
        test_Register();
        test_RegisterCategory();
        test_EnumInputProcessorInfo();
        test_Enable();
        test_ThreadMgrAdviseSinks();
        test_EnumLanguageProfiles();
        test_FindClosestCategory();
        test_Disable();
        test_ThreadMgrUnadviseSinks();
        test_UnregisterCategory();
        test_Unregister();
    }
    else
        skip("Unable to create InputProcessor\n");
    cleanup();
}

/**********************************************************************
 * ITfThreadMgrEventSink
 **********************************************************************/
typedef struct tagThreadMgrEventSink
{
    const ITfThreadMgrEventSinkVtbl *ThreadMgrEventSinkVtbl;
    LONG refCount;
} ThreadMgrEventSink;

static void ThreadMgrEventSink_Destructor(ThreadMgrEventSink *This)
{
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI ThreadMgrEventSink_QueryInterface(ITfThreadMgrEventSink *iface, REFIID iid, LPVOID *ppvOut)
{
    ThreadMgrEventSink *This = (ThreadMgrEventSink *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfThreadMgrEventSink))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI ThreadMgrEventSink_AddRef(ITfThreadMgrEventSink *iface)
{
    ThreadMgrEventSink *This = (ThreadMgrEventSink *)iface;
    ok (tmSinkRefCount == This->refCount,"ThreadMgrEventSink refcount off %i vs %i\n",This->refCount,tmSinkRefCount);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI ThreadMgrEventSink_Release(ITfThreadMgrEventSink *iface)
{
    ThreadMgrEventSink *This = (ThreadMgrEventSink *)iface;
    ULONG ret;

    ok (tmSinkRefCount == This->refCount,"ThreadMgrEventSink refcount off %i vs %i\n",This->refCount,tmSinkRefCount);
    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        ThreadMgrEventSink_Destructor(This);
    return ret;
}

static HRESULT WINAPI ThreadMgrEventSink_OnInitDocumentMgr(ITfThreadMgrEventSink *iface,
ITfDocumentMgr *pdim)
{
    trace("\n");
    return S_OK;
}

static HRESULT WINAPI ThreadMgrEventSink_OnUninitDocumentMgr(ITfThreadMgrEventSink *iface,
ITfDocumentMgr *pdim)
{
    trace("\n");
    return S_OK;
}

static HRESULT WINAPI ThreadMgrEventSink_OnSetFocus(ITfThreadMgrEventSink *iface,
ITfDocumentMgr *pdimFocus, ITfDocumentMgr *pdimPrevFocus)
{
    trace("\n");
    return S_OK;
}

static HRESULT WINAPI ThreadMgrEventSink_OnPushContext(ITfThreadMgrEventSink *iface,
ITfContext *pic)
{
    trace("\n");
    return S_OK;
}

static HRESULT WINAPI ThreadMgrEventSink_OnPopContext(ITfThreadMgrEventSink *iface,
ITfContext *pic)
{
    trace("\n");
    return S_OK;
}

static const ITfThreadMgrEventSinkVtbl ThreadMgrEventSink_ThreadMgrEventSinkVtbl =
{
    ThreadMgrEventSink_QueryInterface,
    ThreadMgrEventSink_AddRef,
    ThreadMgrEventSink_Release,

    ThreadMgrEventSink_OnInitDocumentMgr,
    ThreadMgrEventSink_OnUninitDocumentMgr,
    ThreadMgrEventSink_OnSetFocus,
    ThreadMgrEventSink_OnPushContext,
    ThreadMgrEventSink_OnPopContext
};

HRESULT ThreadMgrEventSink_Constructor(IUnknown **ppOut)
{
    ThreadMgrEventSink *This;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ThreadMgrEventSink));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->ThreadMgrEventSinkVtbl = &ThreadMgrEventSink_ThreadMgrEventSinkVtbl;
    This->refCount = 1;

    *ppOut = (IUnknown *)This;
    return S_OK;
}


/********************************************************************************************
 * Stub text service for testing
 ********************************************************************************************/

static LONG TS_refCount;
static IClassFactory *cf;
static DWORD regid;

typedef HRESULT (*LPFNCONSTRUCTOR)(IUnknown *pUnkOuter, IUnknown **ppvOut);

typedef struct tagClassFactory
{
    const IClassFactoryVtbl *vtbl;
    LONG   ref;
    LPFNCONSTRUCTOR ctor;
} ClassFactory;

typedef struct tagTextService
{
    const ITfTextInputProcessorVtbl *TextInputProcessorVtbl;
    LONG refCount;
} TextService;

static void ClassFactory_Destructor(ClassFactory *This)
{
    HeapFree(GetProcessHeap(),0,This);
    TS_refCount--;
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        IClassFactory_AddRef(iface);
        *ppvOut = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    ClassFactory *This = (ClassFactory *)iface;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    ClassFactory *This = (ClassFactory *)iface;
    ULONG ret = InterlockedDecrement(&This->ref);

    if (ret == 0)
        ClassFactory_Destructor(This);
    return ret;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *punkOuter, REFIID iid, LPVOID *ppvOut)
{
    ClassFactory *This = (ClassFactory *)iface;
    HRESULT ret;
    IUnknown *obj;

    ret = This->ctor(punkOuter, &obj);
    if (FAILED(ret))
        return ret;
    ret = IUnknown_QueryInterface(obj, iid, ppvOut);
    IUnknown_Release(obj);
    return ret;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    if(fLock)
        InterlockedIncrement(&TS_refCount);
    else
        InterlockedDecrement(&TS_refCount);

    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    /* IUnknown */
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,

    /* IClassFactory*/
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static HRESULT ClassFactory_Constructor(LPFNCONSTRUCTOR ctor, LPVOID *ppvOut)
{
    ClassFactory *This = HeapAlloc(GetProcessHeap(),0,sizeof(ClassFactory));
    This->vtbl = &ClassFactoryVtbl;
    This->ref = 1;
    This->ctor = ctor;
    *ppvOut = (LPVOID)This;
    TS_refCount++;
    return S_OK;
}

static void TextService_Destructor(TextService *This)
{
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI TextService_QueryInterface(ITfTextInputProcessor *iface, REFIID iid, LPVOID *ppvOut)
{
    TextService *This = (TextService *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfTextInputProcessor))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI TextService_AddRef(ITfTextInputProcessor *iface)
{
    TextService *This = (TextService *)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI TextService_Release(ITfTextInputProcessor *iface)
{
    TextService *This = (TextService *)iface;
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        TextService_Destructor(This);
    return ret;
}

static HRESULT WINAPI TextService_Activate(ITfTextInputProcessor *iface,
        ITfThreadMgr *ptim, TfClientId id)
{
    trace("TextService_Activate\n");
    return S_OK;
}

static HRESULT WINAPI TextService_Deactivate(ITfTextInputProcessor *iface)
{
    trace("TextService_Deactivate\n");
    return S_OK;
}

static const ITfTextInputProcessorVtbl TextService_TextInputProcessorVtbl=
{
    TextService_QueryInterface,
    TextService_AddRef,
    TextService_Release,

    TextService_Activate,
    TextService_Deactivate
};

HRESULT TextService_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    TextService *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(TextService));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->TextInputProcessorVtbl= &TextService_TextInputProcessorVtbl;
    This->refCount = 1;

    *ppOut = (IUnknown *)This;
    return S_OK;
}

HRESULT RegisterTextService(REFCLSID rclsid)
{
    ClassFactory_Constructor( TextService_Constructor ,(LPVOID*)&cf);
    return CoRegisterClassObject(rclsid, (IUnknown*) cf, CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &regid);
}

HRESULT UnregisterTextService()
{
    return CoRevokeClassObject(regid);
}
