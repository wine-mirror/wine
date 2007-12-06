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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES ON THIS FILE:
 * - Implements ICreateDevEnum interface which creates an IEnumMoniker
 *   implementation
 * - Also creates the special registry keys created at run-time
 */

#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include "devenum_private.h"
#include "vfw.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "mmddk.h"

WINE_DEFAULT_DEBUG_CHANNEL(devenum);

extern HINSTANCE DEVENUM_hInstance;

const WCHAR wszInstanceKeyName[] ={'I','n','s','t','a','n','c','e',0};

static const WCHAR wszRegSeparator[] =   {'\\', 0 };
static const WCHAR wszActiveMovieKey[] = {'S','o','f','t','w','a','r','e','\\',
                                   'M','i','c','r','o','s','o','f','t','\\',
                                   'A','c','t','i','v','e','M','o','v','i','e','\\',
                                   'd','e','v','e','n','u','m','\\',0};

static ULONG WINAPI DEVENUM_ICreateDevEnum_AddRef(ICreateDevEnum * iface);
static HRESULT DEVENUM_CreateSpecialCategories(void);

/**********************************************************************
 * DEVENUM_ICreateDevEnum_QueryInterface (also IUnknown)
 */
static HRESULT WINAPI DEVENUM_ICreateDevEnum_QueryInterface(
    ICreateDevEnum * iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (ppvObj == NULL) return E_POINTER;

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
    TRACE("\n");

    DEVENUM_LockModule();

    return 2; /* non-heap based object */
}

/**********************************************************************
 * DEVENUM_ICreateDevEnum_Release (also IUnknown)
 */
static ULONG WINAPI DEVENUM_ICreateDevEnum_Release(ICreateDevEnum * iface)
{
    TRACE("\n");

    DEVENUM_UnlockModule();

    return 1; /* non-heap based object */
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
    HKEY hkey;
    HKEY hbasekey;
    CreateDevEnumImpl *This = (CreateDevEnumImpl *)iface;

    TRACE("(%p)->(%s, %p, %x)\n\tDeviceClass:\t%s\n", This, debugstr_guid(clsidDeviceClass), ppEnumMoniker, dwFlags, debugstr_guid(clsidDeviceClass));

    if (!ppEnumMoniker)
        return E_POINTER;

    *ppEnumMoniker = NULL;

    if (IsEqualGUID(clsidDeviceClass, &CLSID_AudioRendererCategory) ||
        IsEqualGUID(clsidDeviceClass, &CLSID_AudioInputDeviceCategory) ||
        IsEqualGUID(clsidDeviceClass, &CLSID_VideoInputDeviceCategory) ||
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
        strcatW(wszRegKey, wszRegSeparator);

        if (!StringFromGUID2(clsidDeviceClass, wszRegKey + CLSID_STR_LEN, MAX_PATH - CLSID_STR_LEN))
            return E_OUTOFMEMORY;

        strcatW(wszRegKey, wszRegSeparator);
        strcatW(wszRegKey, wszInstanceKeyName);
    }

    if (RegOpenKeyW(hbasekey, wszRegKey, &hkey) != ERROR_SUCCESS)
    {
        if (IsEqualGUID(clsidDeviceClass, &CLSID_AudioRendererCategory) ||
            IsEqualGUID(clsidDeviceClass, &CLSID_AudioInputDeviceCategory) ||
            IsEqualGUID(clsidDeviceClass, &CLSID_VideoInputDeviceCategory) ||
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

    return DEVENUM_IEnumMoniker_Construct(hkey, ppEnumMoniker);
}

/**********************************************************************
 * ICreateDevEnum_Vtbl
 */
static const ICreateDevEnumVtbl ICreateDevEnum_Vtbl =
{
    DEVENUM_ICreateDevEnum_QueryInterface,
    DEVENUM_ICreateDevEnum_AddRef,
    DEVENUM_ICreateDevEnum_Release,
    DEVENUM_ICreateDevEnum_CreateClassEnumerator,
};

/**********************************************************************
 * static CreateDevEnum instance
 */
CreateDevEnumImpl DEVENUM_CreateDevEnum = { &ICreateDevEnum_Vtbl };

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
    {
        LONG lRes = RegCreateKeyW(HKEY_CURRENT_USER, wszRegKey, &hkeyDummy);
        res = HRESULT_FROM_WIN32(lRes);
    }

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
static HRESULT DEVENUM_CreateSpecialCategories(void)
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
    rfp2.cInstances = 1;
    rfp2.nMediums = 0;
    rfp2.lpMedium = NULL;
    rfp2.clsPinCategory = &IID_NULL;

    if (!LoadStringW(DEVENUM_hInstance, IDS_DEVENUM_DS, szDSoundNameFormat, sizeof(szDSoundNameFormat)/sizeof(szDSoundNameFormat[0])-1))
    {
        ERR("Couldn't get string resource (GetLastError() is %d)\n", GetLastError());
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

	rfp2.dwFlags = REG_PINFLAG_B_RENDERER;
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

	rfp2.dwFlags = REG_PINFLAG_B_OUTPUT;
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

	rfp2.dwFlags = REG_PINFLAG_B_RENDERER;
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
        res = DEVENUM_CreateAMCategoryKey(&CLSID_VideoInputDeviceCategory);
        if (SUCCEEDED(res))
            for (i = 0; i < 10; i++)
            {
                WCHAR szDeviceName[32], szDeviceVersion[32], szDevicePath[10];

                if (capGetDriverDescriptionW ((WORD) i,
                                              szDeviceName, sizeof(szDeviceName)/sizeof(WCHAR),
                                              szDeviceVersion, sizeof(szDeviceVersion)/sizeof(WCHAR)))
                {
                    IMoniker * pMoniker = NULL;
                    IPropertyBag * pPropBag = NULL;
                    WCHAR dprintf[] = { 'v','i','d','e','o','%','d',0 };
                    snprintfW(szDevicePath, sizeof(szDevicePath)/sizeof(WCHAR), dprintf, i);
                    /* The above code prevents 1 device with a different ID overwriting another */

                    rfp2.nMediaTypes = 1;
                    pTypes = CoTaskMemAlloc(rfp2.nMediaTypes * sizeof(REGPINTYPES));
                    if (!pTypes) {
                        IFilterMapper2_Release(pMapper);
                        return E_OUTOFMEMORY;
                    }

                    pTypes[0].clsMajorType = &MEDIATYPE_Video;
                    pTypes[0].clsMinorType = &MEDIASUBTYPE_None;

                    rfp2.lpMediaType = pTypes;

                    res = IFilterMapper2_RegisterFilter(pMapper,
                                                        &CLSID_VfwCapture,
                                                        szDeviceName,
                                                        &pMoniker,
                                                        &CLSID_VideoInputDeviceCategory,
                                                        szDevicePath,
                                                        &rf2);

                    if (pMoniker) {
                       OLECHAR wszVfwIndex[] = { 'V','F','W','I','n','d','e','x',0 };
                       VARIANT var;
                       V_VT(&var) = VT_I4;
                       V_UNION(&var, ulVal) = i;
                       res = IMoniker_BindToStorage(pMoniker, NULL, NULL, &IID_IPropertyBag, (LPVOID)&pPropBag);
                       if (SUCCEEDED(res))
                          res = IPropertyBag_Write(pPropBag, wszVfwIndex, &var);
                       IMoniker_Release(pMoniker);
                    }

                    if (i == iDefaultDevice) FIXME("Default device\n");
                    CoTaskMemFree(pTypes);
                }
            }
    }

    if (pMapper)
        IFilterMapper2_Release(pMapper);
    return res;
}
