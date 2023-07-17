/*
 *	self-registerable dll functions for quartz.dll
 *
 * Copyright (C) 2003 John K. Hohm
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

#define COBJMACROS
#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"

#include "ole2.h"
#include "uuids.h"
#include "strmif.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

/*
 * Near the bottom of this file are the exported DllRegisterServer and
 * DllUnregisterServer, which make all this worthwhile.
 */

struct mediatype
{
    CLSID const *majortype;	/* NULL for end of list */
    CLSID const *subtype;
    DWORD fourcc;
};

struct pin
{
    DWORD flags;		/* 0xFFFFFFFF for end of list */
    struct mediatype mediatypes[11];
};

struct regsvr_filter
{
    CLSID const *clsid;		/* NULL for end of list */
    CLSID const *category;
    WCHAR name[50];
    DWORD merit;
    struct pin pins[11];
};

/***********************************************************************
 *		register_filters
 */
static HRESULT register_filters(struct regsvr_filter const *list)
{
    HRESULT hr;
    IFilterMapper2* pFM2 = NULL;

    CoInitialize(NULL);
    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterMapper2, (LPVOID*)&pFM2);

    if (SUCCEEDED(hr)) {
	for (; SUCCEEDED(hr) && list->clsid; ++list) {
	    REGFILTER2 rf2;
	    REGFILTERPINS2* prfp2;
	    int i;

	    for (i = 0; list->pins[i].flags != 0xFFFFFFFF; i++) ;
	    rf2.dwVersion = 2;
	    rf2.dwMerit = list->merit;
	    rf2.cPins2 = i;
	    rf2.rgPins2 = prfp2 = CoTaskMemAlloc(i*sizeof(REGFILTERPINS2));
	    if (!prfp2) {
		hr = E_OUTOFMEMORY;
		break;
	    }
	    for (i = 0; list->pins[i].flags != 0xFFFFFFFF; i++) {
		REGPINTYPES* lpMediatype;
		CLSID* lpClsid;
		int j, nbmt;
                
		for (nbmt = 0; list->pins[i].mediatypes[nbmt].majortype; nbmt++) ;
		/* Allocate a single buffer for regpintypes struct and clsids */
		lpMediatype = CoTaskMemAlloc(nbmt*(sizeof(REGPINTYPES) + 2*sizeof(CLSID)));
		if (!lpMediatype) {
		    hr = E_OUTOFMEMORY;
		    break;
		}
		lpClsid = (CLSID*) (lpMediatype + nbmt);
		for (j = 0; j < nbmt; j++) {
		    (lpMediatype + j)->clsMajorType = lpClsid + j*2;
		    memcpy(lpClsid + j*2, list->pins[i].mediatypes[j].majortype, sizeof(CLSID));
		    (lpMediatype + j)->clsMinorType = lpClsid + j*2 + 1;
		    if (list->pins[i].mediatypes[j].subtype)
			memcpy(lpClsid + j*2 + 1, list->pins[i].mediatypes[j].subtype, sizeof(CLSID));
		    else {
                        /* Subtypes are often a combination of major type + fourcc/tag */
			memcpy(lpClsid + j*2 + 1, list->pins[i].mediatypes[j].majortype, sizeof(CLSID));
			*(DWORD*)(lpClsid + j*2 + 1) = list->pins[i].mediatypes[j].fourcc;
		    }
		}
		prfp2[i].dwFlags = list->pins[i].flags;
		prfp2[i].cInstances = 0;
		prfp2[i].nMediaTypes = j;
		prfp2[i].lpMediaType = lpMediatype;
		prfp2[i].nMediums = 0;
		prfp2[i].lpMedium = NULL;
		prfp2[i].clsPinCategory = NULL;
	    }

	    if (FAILED(hr)) {
		ERR("failed to register with hresult %#lx\n", hr);
		CoTaskMemFree(prfp2);
		break;
	    }

	    hr = IFilterMapper2_RegisterFilter(pFM2, list->clsid, list->name, NULL, list->category, NULL, &rf2);

	    while (i) {
		CoTaskMemFree((REGPINTYPES*)prfp2[i-1].lpMediaType);
		i--;
	    }
	    CoTaskMemFree(prfp2);
	}
    }

    if (pFM2)
	IFilterMapper2_Release(pFM2);

    CoUninitialize();

    return hr;
}

/***********************************************************************
 *		unregister_filters
 */
static HRESULT unregister_filters(struct regsvr_filter const *list)
{
    HRESULT hr;
    IFilterMapper2* pFM2;

    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterMapper2, (LPVOID*)&pFM2);

    if (SUCCEEDED(hr)) {
	for (; SUCCEEDED(hr) && list->clsid; ++list)
	    hr = IFilterMapper2_UnregisterFilter(pFM2, list->category, NULL, list->clsid);
	IFilterMapper2_Release(pFM2);
    }

    return hr;
}

/***********************************************************************
 *		filter list
 */

static struct regsvr_filter const filter_list[] = {
    {   &CLSID_VideoRenderer,
	&CLSID_LegacyAmFilterCategory,
	L"Video Renderer",
	0x800000,
	{   {   REG_PINFLAG_B_RENDERER,
		{   { &MEDIATYPE_Video, &GUID_NULL },
		    { NULL }
		},
	    },
	    { 0xFFFFFFFF },
	}
    },
    {   &CLSID_VideoRendererDefault,
        &CLSID_LegacyAmFilterCategory,
        L"Video Renderer",
        0x800001,
        {   {   REG_PINFLAG_B_RENDERER,
                {   { &MEDIATYPE_Video, &GUID_NULL },
                    { NULL }
                },
            },
            { 0xFFFFFFFF },
        }
    },
    {   &CLSID_VideoMixingRenderer9,
        &CLSID_LegacyAmFilterCategory,
        L"Video Mixing Renderer 9",
        0x200000,
        {   {   REG_PINFLAG_B_RENDERER,
                {   { &MEDIATYPE_Video, &GUID_NULL },
                    { NULL }
                },
            },
            { 0xFFFFFFFF },
        }
    },
    {   &CLSID_AVIDec,
	&CLSID_LegacyAmFilterCategory,
	L"AVI Decompressor",
	0x5ffff0,
	{   {   0,
		{   { &MEDIATYPE_Video, &GUID_NULL },
		    { NULL }
		},
	    },
	    {   REG_PINFLAG_B_OUTPUT,
		{   { &MEDIATYPE_Video, &GUID_NULL },
		    { NULL }
		},
	    },
	    { 0xFFFFFFFF },
	}
    },
    {   &CLSID_AsyncReader,
	&CLSID_LegacyAmFilterCategory,
	L"File Source (Async.)",
	0x400000,
	{   {   REG_PINFLAG_B_OUTPUT,
		{   { &MEDIATYPE_Stream, &GUID_NULL },
		    { NULL }
		},
	    },
	    { 0xFFFFFFFF },
	}
    },
    {   &CLSID_ACMWrapper,
	&CLSID_LegacyAmFilterCategory,
	L"ACM Wrapper",
	0x5ffff0,
	{   {   0,
		{   { &MEDIATYPE_Audio, &GUID_NULL },
		    { NULL }
		},
	    },
	    {   REG_PINFLAG_B_OUTPUT,
		{   { &MEDIATYPE_Audio, &GUID_NULL },
		    { NULL }
		},
	    },
	    { 0xFFFFFFFF },
	}
    },
    { NULL }		/* list terminator */
};

extern HRESULT WINAPI QUARTZ_DllRegisterServer(void);
extern HRESULT WINAPI QUARTZ_DllUnregisterServer(void);

/***********************************************************************
 *		DllRegisterServer (QUARTZ.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = QUARTZ_DllRegisterServer();
    if (SUCCEEDED(hr))
        hr = register_filters(filter_list);
    return hr;
}

/***********************************************************************
 *		DllUnregisterServer (QUARTZ.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = unregister_filters(filter_list);
    if (SUCCEEDED(hr))
        hr = QUARTZ_DllUnregisterServer();
    return hr;
}
