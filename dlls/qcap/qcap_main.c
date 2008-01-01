/*
 * Qcap implementation, dllentry points
 *
 * Copyright (C) 2003 Dominik Strasser
 * Copyright (C) 2005 Rolf Kalbermatter
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
#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "uuids.h"
#include "strmif.h"

#include "dllsetup.h"
#include "qcap_main.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

static LONG objects_ref = 0;
static LONG server_locks = 0;

static const WCHAR wAudioCaptFilter[] =
{'A','u','d','i','o',' ','C','a','p','t','u','r','e',' ','F','i','l','t','e','r',0};
static const WCHAR wAVICompressor[] =
{'A','V','I',' ','C','o','m','p','r','e','s','s','o','r',0};
static const WCHAR wVFWCaptFilter[] =
{'V','F','W',' ','C','a','p','t','u','r','e',' ','F','i','l','t','e','r',0};
static const WCHAR wVFWCaptFilterProp[] =
{'V','F','W',' ','C','a','p','t','u','r','e',' ','F','i','l','t','e','r',' ',
 'P','r','o','p','e','r','t','y',' ','P','a','g','e',0};
static const WCHAR wAVIMux[] =
{'A','V','I',' ','m','u','x',0};
static const WCHAR wAVIMuxPropPage[] =
{'A','V','I',' ','m','u','x',' ','P','r','o','p','e','r','t','y',' ','P','a','g','e',0};
static const WCHAR wAVIMuxPropPage1[] =
{'A','V','I',' ','m','u','x',' ','P','r','o','p','e','r','t','y',' ','P','a','g','e','1',0};
static const WCHAR wFileWriter[] =
{'F','i','l','e',' ','W','r','i','t','e','r',0};
static const WCHAR wCaptGraphBuilder[] =
{'C','a','p','t','u','r','e',' ','G','r','a','p','h',' ','B','u','i','l','d','e','r',0};
static const WCHAR wCaptGraphBuilder2[] =
{'C','a','p','t','u','r','e',' ','G','r','a','p','h',' ','B','u','i','l','d','e','r','2',0};
static const WCHAR wInfPinTeeFilter[] =
{'I','n','f','i','n','i','t','e',' ','P','i','n',' ','T','e','e',' ','F','i',
 'l','t','e','r',0};
static const WCHAR wSmartTeeFilter[] =
{'S','m','a','r','t',' ','T','e','e',' ','F','i','l','t','e','r',0};
static const WCHAR wAudioInMixerProp[] =
{'A','u','d','i','o','I','n','p','u','t','M','i','x','e','r',' ','P','r','o',
 'p','e','r','t','y',' ','P','a','g','e',0};
 
static CFactoryTemplate const g_cTemplates[] = {
/*
    {
        wAudioCaptureFilter, 
        &CLSID_AudioCaptureFilter,
        QCAP_createAudioCaptureFilter,
        NULL,
        NULL
    },{
        wAVICompressor, 
        &CLSID_AVICompressor, 
        QCAP_createAVICompressor,
        NULL,
        NULL
    },*/{
        wVFWCaptFilter,
        &CLSID_VfwCapture,
        QCAP_createVFWCaptureFilter,
        NULL,
        NULL
    },/*{
        wVFWCaptFilterProp,
        &CLSID_VFWCaptureFilterPropertyPage,
        QCAP_createVFWCaptureFilterPropertyPage,
        NULL,
        NULL
    },{
        wAVIMux,
        &CLSID_AVImux,
        QCAP_createAVImux,
        NULL,
        NULL
    },{
        wAVIMuxPropPage,
        &CLSID_AVImuxPropertyPage,
        QCAP_createAVImuxPropertyPage,
        NULL,
        NULL
    },{
        wAVIMuxPropPage1,
        &CLSID_AVImuxPropertyPage1,
        QCAP_createAVImuxPropertyPage1,
        NULL,
        NULL
    },{
        wFileWriter,
        &CLSID_FileWriter,
        QCAP_createFileWriter,
        NULL,
        NULL
    },*/{
        wCaptGraphBuilder,
        &CLSID_CaptureGraphBuilder,
        QCAP_createCaptureGraphBuilder2,
        NULL,
        NULL
    },{
        wCaptGraphBuilder2,
        &CLSID_CaptureGraphBuilder2,
        QCAP_createCaptureGraphBuilder2,
        NULL,
        NULL
    }/*,{
        wInfPinTeeFilter, 
        &CLSID_InfinitePinTeeFilter, 
        QCAP_createInfinitePinTeeFilter,
        NULL,
        NULL
    },{
        wSmartTeeFilter,
        &CLSID_SmartTeeFilter,
        QCAP_createSmartTeeFilter,
        NULL,
        NULL
    },{
        wAudioInMixerProp,
        &CLSID_AudioInputMixerPropertyPage,
        QCAP_createAudioInputMixerPropertyPage,
        NULL,
        NULL
    }*/
};

static int g_numTemplates = sizeof(g_cTemplates) / sizeof(g_cTemplates[0]);

/***********************************************************************
 *    Dll EntryPoint (QCAP.@)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            SetupInitializeServers(g_cTemplates, g_numTemplates, TRUE);
            break;
        case DLL_PROCESS_DETACH:
            SetupInitializeServers(g_cTemplates, g_numTemplates, FALSE);
            break;
    }
    return TRUE;
}

/***********************************************************************
 *    DllRegisterServer (QCAP.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    TRACE("()\n");

    return SetupRegisterServers(g_cTemplates, g_numTemplates, TRUE);
}

/***********************************************************************
 *    DllUnregisterServer (QCAP.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    TRACE("\n");

    return SetupRegisterServers(g_cTemplates, g_numTemplates, FALSE);
}

/***********************************************************************
 *    DllCanUnloadNow (QCAP.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("\n");

    if (objects_ref == 0 && server_locks == 0)
        return S_OK;
    return S_FALSE;	
}

/******************************************************************************
 * DLL ClassFactory
 */
typedef struct {
    IClassFactory ITF_IClassFactory;

    LONG ref;
    LPFNNewCOMObject pfnCreateInstance;
} IClassFactoryImpl;

static HRESULT WINAPI
DSCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p), not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI DSCF_AddRef(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI DSCF_Release(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI DSCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter,
                                          REFIID riid, LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    HRESULT hres = ERROR_SUCCESS;
    LPUNKNOWN punk;

    TRACE("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);

    if (!ppobj)
        return E_POINTER;

    /* Enforce the normal OLE rules regarding interfaces and delegation */
    if (pOuter && !IsEqualGUID(riid, &IID_IUnknown))
        return E_NOINTERFACE;

    *ppobj = NULL;
    punk = This->pfnCreateInstance(pOuter, &hres);
    if (!punk)
    {
        /* No object created, update error if it isn't done already and return */
        if (!FAILED(hres))
            hres = E_OUTOFMEMORY;
    return hres;
    }

    if (SUCCEEDED(hres))
    {
        hres = IUnknown_QueryInterface(punk, riid, ppobj);
    }
    /* Releasing the object. If everything was successful, QueryInterface
       should have incremented the refcount once more, otherwise this will
       purge the object. */
    IUnknown_Release(punk);
    return hres;
}

static HRESULT WINAPI DSCF_LockServer(LPCLASSFACTORY iface, BOOL dolock)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    TRACE("(%p)->(%d)\n",This, dolock);

    if (dolock)
        InterlockedIncrement(&server_locks);
    else
        InterlockedDecrement(&server_locks);
    return S_OK;
}

static const IClassFactoryVtbl DSCF_Vtbl =
{
    DSCF_QueryInterface,
    DSCF_AddRef,
    DSCF_Release,
    DSCF_CreateInstance,
    DSCF_LockServer
};

/***********************************************************************
 *    DllGetClassObject (QCAP.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    const CFactoryTemplate *pList = g_cTemplates;
    IClassFactoryImpl *factory;
    int i;

    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    if (!IsEqualGUID(&IID_IClassFactory, riid) &&
        !IsEqualGUID(&IID_IUnknown, riid))
        return E_NOINTERFACE;

    for (i = 0; i < g_numTemplates; i++, pList++)
    {
        if (IsEqualGUID(pList->m_ClsID, rclsid))
            break;
    }

    if (i == g_numTemplates)
    {
        FIXME("%s: no class found.\n", debugstr_guid(rclsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    factory = HeapAlloc(GetProcessHeap(), 0, sizeof(IClassFactoryImpl));
    if (!factory)
        return E_OUTOFMEMORY;

    factory->ITF_IClassFactory.lpVtbl = &DSCF_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = pList->m_lpfnNew;

    *ppv = &(factory->ITF_IClassFactory);
    return S_OK;
}

DWORD ObjectRefCount(BOOL increment)
{
    if (increment)
        return InterlockedIncrement(&objects_ref);
    return InterlockedDecrement(&objects_ref);
}
