/*
 *	ICreateDevEnum implementation for DEVENUM.dll
 *
 * Copyright (C) 2002 Robert Shearman
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
 *
 * NOTES ON THIS FILE:
 * - Implements ICreateDevEnum interface which creates an IEnumMoniker
 *   implementation
 * - Also creates the special registry keys created at run-time
 */

#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include "devenum_private.h"

#include "wine/debug.h"
#include "mmddk.h"

WINE_DEFAULT_DEBUG_CHANNEL(devenum);

extern ICOM_VTABLE(IEnumMoniker) IEnumMoniker_Vtbl;

extern HINSTANCE DEVENUM_hInstance;

const WCHAR wszInstanceKeyName[] ={'I','n','s','t','a','n','c','e',0};

static const WCHAR wszRegSeperator[] =   {'\\', 0 };
static const WCHAR wszActiveMovieKey[] = {'S','o','f','t','w','a','r','e','\\',
                                   'M','i','c','r','o','s','o','f','t','\\',
                                   'A','c','t','i','v','e','M','o','v','i','e','\\',
                                   'd','e','v','e','n','u','m','\\',0};

static ULONG WINAPI DEVENUM_ICreateDevEnum_AddRef(ICreateDevEnum * iface);
static HRESULT DEVENUM_CreateSpecialCategories();

/**********************************************************************
 * DEVENUM_ICreateDevEnum_QueryInterface (also IUnknown)
 */
static HRESULT WINAPI DEVENUM_ICreateDevEnum_QueryInterface(
    ICreateDevEnum * iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    ICOM_THIS(CreateDevEnumImpl, iface);
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
	IsEqualGUID(riid, &IID_ICreateDevEnum))
    {
	*ppvObj = (LPVOID)iface;
	DEVENUM_ICreateDevEnum_AddRef(iface);
	return S_OK;
    }

    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

/**********************************************************************
 * DEVENUM_ICreateDevEnum_AddRef (also IUnknown)
 */
static ULONG WINAPI DEVENUM_ICreateDevEnum_AddRef(ICreateDevEnum * iface)
{
    ICOM_THIS(CreateDevEnumImpl, iface);
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    if (InterlockedIncrement(&This->ref) == 1) {
	InterlockedIncrement(&dll_ref);
    }
    return This->ref;
}

/**********************************************************************
 * DEVENUM_ICreateDevEnum_Release (also IUnknown)
 */
static ULONG WINAPI DEVENUM_ICreateDevEnum_Release(ICreateDevEnum * iface)
{
    ICOM_THIS(CreateDevEnumImpl, iface);
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    if (InterlockedDecrement(&This->ref) == 0) {
	InterlockedDecrement(&dll_ref);
    }
    return This->ref;
}

/**********************************************************************
 * DEVENUM_ICreateDevEnum_CreateClassEnumerator
 */
HRESULT WINAPI DEVENUM_ICreateDevEnum_CreateClassEnumerator(
    ICreateDevEnum * iface,
    REFCLSID clsidDeviceClass,
    IEnumMoniker **ppEnumMoniker,
    DWORD dwFlags)
{
    WCHAR wszRegKey[MAX_PATH];
    EnumMonikerImpl * pEnumMoniker;
    HKEY hkey;
    HKEY hbasekey;
    ICOM_THIS(CreateDevEnumImpl, iface);

    TRACE("(%p)->(%s, %p, %lx)\n\tDeviceClass:\t%s\n", This, debugstr_guid(clsidDeviceClass), ppEnumMoniker, dwFlags, debugstr_guid(clsidDeviceClass));

    if (!ppEnumMoniker)
        return E_POINTER;

    *ppEnumMoniker = NULL;

    if (IsEqualGUID(clsidDeviceClass, &CLSID_AudioRendererCategory) ||
        IsEqualGUID(clsidDeviceClass, &CLSID_AudioInputDeviceCategory) ||
        IsEqualGUID(clsidDeviceClass, &CLSID_MidiRendererCategory))
    {
        hbasekey = HKEY_CURRENT_USER;
        strcpyW(wszRegKey, wszActiveMovieKey);

        if (!StringFromGUID2(clsidDeviceClass, wszRegKey + strlenW(wszRegKey), MAX_PATH - strlenW(wszRegKey)))
            return E_OUTOFMEMORY;
    }
    else
    {
        hbasekey = HKEY_CLASSES_ROOT;
        strcpyW(wszRegKey, clsid_keyname);
        strcatW(wszRegKey, wszRegSeperator);

        if (!StringFromGUID2(clsidDeviceClass, wszRegKey + CLSID_STR_LEN, MAX_PATH - CLSID_STR_LEN))
            return E_OUTOFMEMORY;

        strcatW(wszRegKey, wszRegSeperator);
        strcatW(wszRegKey, wszInstanceKeyName);
    }

    if (RegOpenKeyW(hbasekey, wszRegKey, &hkey) != ERROR_SUCCESS)
    {
        if (IsEqualGUID(clsidDeviceClass, &CLSID_AudioRendererCategory) ||
            IsEqualGUID(clsidDeviceClass, &CLSID_AudioInputDeviceCategory) ||
            IsEqualGUID(clsidDeviceClass, &CLSID_MidiRendererCategory))
        {
             HRESULT hr = DEVENUM_CreateSpecialCategories();
             if (FAILED(hr))
                 return hr;
             if (RegOpenKeyW(hbasekey, wszRegKey, &hkey) != ERROR_SUCCESS)
             {
                 ERR("Couldn't open registry key for special device: %s\n",
                     debugstr_guid(clsidDeviceClass));
                 return S_FALSE;
             }
        }
        else
        {
            FIXME("Category %s not found\n", debugstr_guid(clsidDeviceClass));
            return S_FALSE;
        }
    }

    pEnumMoniker = (EnumMonikerImpl *)CoTaskMemAlloc(sizeof(EnumMonikerImpl));
    if (!pEnumMoniker)
        return E_OUTOFMEMORY;

    pEnumMoniker->lpVtbl = &IEnumMoniker_Vtbl;
    pEnumMoniker->ref = 1;
    pEnumMoniker->index = 0;
    pEnumMoniker->hkey = hkey;

    *ppEnumMoniker = (IEnumMoniker *)pEnumMoniker;

    return S_OK;
}

/**********************************************************************
 * ICreateDevEnum_Vtbl
 */
static ICOM_VTABLE(ICreateDevEnum) ICreateDevEnum_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    DEVENUM_ICreateDevEnum_QueryInterface,
    DEVENUM_ICreateDevEnum_AddRef,
    DEVENUM_ICreateDevEnum_Release,
    DEVENUM_ICreateDevEnum_CreateClassEnumerator,
};

/**********************************************************************
 * static CreateDevEnum instance
 */
CreateDevEnumImpl DEVENUM_CreateDevEnum = { &ICreateDevEnum_Vtbl, 0 };

/**********************************************************************
 * DEVENUM_CreateAMCategoryKey (INTERNAL)
 *
 * Creates a registry key for a category at HKEY_CURRENT_USER\Software\
 * Microsoft\ActiveMovie\devenum\{clsid}
 */
static HRESULT DEVENUM_CreateAMCategoryKey(const CLSID * clsidCategory)
{
    WCHAR wszRegKey[MAX_PATH];
    HRESULT res = S_OK;
    HKEY hkeyDummy = NULL;

    strcpyW(wszRegKey, wszActiveMovieKey);

    if (!StringFromGUID2(clsidCategory, wszRegKey + strlenW(wszRegKey), sizeof(wszRegKey)/sizeof(wszRegKey[0]) - strlenW(wszRegKey)))
        res = E_INVALIDARG;

    if (SUCCEEDED(res))
        res = HRESULT_FROM_WIN32(
            RegCreateKeyW(HKEY_CURRENT_USER, wszRegKey, &hkeyDummy));

    if (hkeyDummy)
        RegCloseKey(hkeyDummy);

    if (FAILED(res))
        ERR("Failed to create key HKEY_CURRENT_USER\\%s\n", debugstr_w(wszRegKey));

    return res;
}

/**********************************************************************
 * DEVENUM_CreateSpecialCategories (INTERNAL)
 *
 * Creates the keys in the registry for the dynamic categories
 */
static HRESULT DEVENUM_CreateSpecialCategories()
{
    HRESULT res;
    WCHAR szDSoundNameFormat[MAX_PATH + 1];
    WCHAR szDSoundName[MAX_PATH + 1];
    DWORD iDefaultDevice = -1;
    UINT numDevs;
    IFilterMapper2 * pMapper = NULL;
    REGFILTER2 rf2;
    REGFILTERPINS2 rfp2;

    rf2.dwVersion = 2;
    rf2.dwMerit = MERIT_PREFERRED;
    rf2.u.s1.cPins2 = 1;
    rf2.u.s1.rgPins2 = &rfp2;
    rfp2.dwFlags = REG_PINFLAG_B_RENDERER;
    rfp2.cInstances = 1;
    rfp2.nMediums = 0;
    rfp2.lpMedium = NULL;
    rfp2.clsPinCategory = &IID_NULL;

    if (!LoadStringW(DEVENUM_hInstance, IDS_DEVENUM_DS, szDSoundNameFormat, sizeof(szDSoundNameFormat)/sizeof(szDSoundNameFormat[0])-1))
    {
        ERR("Couldn't get string resource (GetLastError() is %ld)\n", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    res = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC,
                           &IID_IFilterMapper2, (void **) &pMapper);
    /*
     * Fill in info for devices
     */
    if (SUCCEEDED(res))
    {
        UINT i;
        WAVEOUTCAPSW wocaps;
	WAVEINCAPSW wicaps;
        MIDIOUTCAPSW mocaps;
        REGPINTYPES * pTypes;

	numDevs = waveOutGetNumDevs();

        res = DEVENUM_CreateAMCategoryKey(&CLSID_AudioRendererCategory);
        if (FAILED(res)) /* can't register any devices in this category */
            numDevs = 0;

	for (i = 0; i < numDevs; i++)
	{
	    if (waveOutGetDevCapsW(i, &wocaps, sizeof(WAVEOUTCAPSW))
	        == MMSYSERR_NOERROR)
	    {
                IMoniker * pMoniker = NULL;

                rfp2.nMediaTypes = 1;
                pTypes = CoTaskMemAlloc(rfp2.nMediaTypes * sizeof(REGPINTYPES));
                if (!pTypes)
                {
                    IFilterMapper2_Release(pMapper);
                    return E_OUTOFMEMORY;
                }
                /* FIXME: Native devenum seems to register a lot more types for
                 * DSound than we do. Not sure what purpose they serve */
                pTypes[0].clsMajorType = &MEDIATYPE_Audio;
                pTypes[0].clsMinorType = &MEDIASUBTYPE_PCM;

                rfp2.lpMediaType = pTypes;

                res = IFilterMapper2_RegisterFilter(pMapper,
		                              &CLSID_AudioRender,
					      wocaps.szPname,
					      &pMoniker,
					      &CLSID_AudioRendererCategory,
					      wocaps.szPname,
					      &rf2);

                /* FIXME: do additional stuff with IMoniker here, depending on what RegisterFilter does */

		if (pMoniker)
		    IMoniker_Release(pMoniker);

		wsprintfW(szDSoundName, szDSoundNameFormat, wocaps.szPname);
	        res = IFilterMapper2_RegisterFilter(pMapper,
		                              &CLSID_DSoundRender,
					      szDSoundName,
					      &pMoniker,
					      &CLSID_AudioRendererCategory,
					      szDSoundName,
					      &rf2);

                /* FIXME: do additional stuff with IMoniker here, depending on what RegisterFilter does */

		if (pMoniker)
		    IMoniker_Release(pMoniker);

		if (i == iDefaultDevice)
		{
		    FIXME("Default device\n");
		}

                CoTaskMemFree(pTypes);
	    }
	}

        numDevs = waveInGetNumDevs();

        res = DEVENUM_CreateAMCategoryKey(&CLSID_AudioInputDeviceCategory);
        if (FAILED(res)) /* can't register any devices in this category */
            numDevs = 0;

        for (i = 0; i < numDevs; i++)
        {
            if (waveInGetDevCapsW(i, &wicaps, sizeof(WAVEINCAPSW))
                == MMSYSERR_NOERROR)
            {
                IMoniker * pMoniker = NULL;

                rfp2.nMediaTypes = 1;
                pTypes = CoTaskMemAlloc(rfp2.nMediaTypes * sizeof(REGPINTYPES));
                if (!pTypes)
                {
                    IFilterMapper2_Release(pMapper);
                    return E_OUTOFMEMORY;
                }

                /* FIXME: Not sure if these are correct */
                pTypes[0].clsMajorType = &MEDIATYPE_Audio;
                pTypes[0].clsMinorType = &MEDIASUBTYPE_PCM;

                rfp2.lpMediaType = pTypes;

	        res = IFilterMapper2_RegisterFilter(pMapper,
		                              &CLSID_AudioRecord,
					      wicaps.szPname,
					      &pMoniker,
					      &CLSID_AudioInputDeviceCategory,
					      wicaps.szPname,
					      &rf2);

                /* FIXME: do additional stuff with IMoniker here, depending on what RegisterFilter does */

		if (pMoniker)
		    IMoniker_Release(pMoniker);

                CoTaskMemFree(pTypes);
	    }
	}

	numDevs = midiOutGetNumDevs();

        res = DEVENUM_CreateAMCategoryKey(&CLSID_MidiRendererCategory);
        if (FAILED(res)) /* can't register any devices in this category */
            numDevs = 0;

	for (i = 0; i < numDevs; i++)
	{
	    if (midiOutGetDevCapsW(i, &mocaps, sizeof(MIDIOUTCAPSW))
	        == MMSYSERR_NOERROR)
	    {
                IMoniker * pMoniker = NULL;

                rfp2.nMediaTypes = 1;
                pTypes = CoTaskMemAlloc(rfp2.nMediaTypes * sizeof(REGPINTYPES));
                if (!pTypes)
                {
                    IFilterMapper2_Release(pMapper);
                    return E_OUTOFMEMORY;
                }

                /* FIXME: Not sure if these are correct */
                pTypes[0].clsMajorType = &MEDIATYPE_Midi;
                pTypes[0].clsMinorType = &MEDIASUBTYPE_None;

                rfp2.lpMediaType = pTypes;

                res = IFilterMapper2_RegisterFilter(pMapper,
		                              &CLSID_AVIMIDIRender,
					      mocaps.szPname,
					      &pMoniker,
					      &CLSID_MidiRendererCategory,
					      mocaps.szPname,
					      &rf2);

                /* FIXME: do additional stuff with IMoniker here, depending on what RegisterFilter does */
		/* Native version sets MidiOutId */

		if (pMoniker)
		    IMoniker_Release(pMoniker);

		if (i == iDefaultDevice)
		{
		    FIXME("Default device\n");
		}

                CoTaskMemFree(pTypes);
	    }
	}
    }

    if (pMapper)
        IFilterMapper2_Release(pMapper);
    return res;
}
