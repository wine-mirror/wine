/*
 * DirectShow filter for QuickTime Toolkit on Mac OS X
 *
 * Copyright (C) 2010 Aric Stewart, CodeWeavers
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
#include "wingdi.h"
#include "winerror.h"
#include "objbase.h"
#include "uuids.h"
#include "strmif.h"
#include "rpcproxy.h"

#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/strmbase.h"

#include "initguid.h"
DEFINE_GUID(CLSID_QTVDecoder, 0x683DDACB, 0x4354, 0x490C, 0xA0,0x58, 0xE0,0x5A,0xD0,0xF2,0x05,0x37);
DEFINE_GUID(CLSID_QTSplitter,    0xD0E70E49, 0x5927, 0x4894, 0xA3,0x86, 0x35,0x94,0x60,0xEE,0x87,0xC9);
DEFINE_GUID(WINESUBTYPE_QTSplitter,    0xFFFFFFFF, 0x5927, 0x4894, 0xA3,0x86, 0x35,0x94,0x60,0xEE,0x87,0xC9);

WINE_DEFAULT_DEBUG_CHANNEL(qtdecoder);

extern IUnknown * CALLBACK QTVDecoder_create(IUnknown * pUnkOuter, HRESULT* phr);
extern IUnknown * CALLBACK QTSplitter_create(IUnknown * pUnkOuter, HRESULT* phr);

static const WCHAR wQTVName[] =
{'Q','T',' ','V','i','d','e','o',' ','D','e','c','o','d','e','r',0};
static const WCHAR wQTDName[] =
{'Q','T',' ','V','i','d','e','o',' ','D','e','m','u','x',0};
static WCHAR wNull[] = {'\0'};

static const AMOVIESETUP_MEDIATYPE amfMTvideo[] =
{   { &MEDIATYPE_Video, &MEDIASUBTYPE_NULL } };
static const AMOVIESETUP_MEDIATYPE amfMTaudio[] =
{   { &MEDIATYPE_Audio, &MEDIASUBTYPE_NULL } };
static const AMOVIESETUP_MEDIATYPE amfMTstream[] =
{   { &MEDIATYPE_Stream, &WINESUBTYPE_QTSplitter},
    { &MEDIATYPE_Stream, &MEDIASUBTYPE_NULL } };

static const AMOVIESETUP_PIN amfQTVPin[] =
{   {   wNull,
        FALSE, FALSE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        1,
        amfMTvideo
    },
    {
        wNull,
        FALSE, TRUE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        1,
        amfMTvideo
    },
};

static const AMOVIESETUP_PIN amfQTDPin[] =
{   {   wNull,
        FALSE, FALSE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        2,
        amfMTstream
    },
    {
        wNull,
        FALSE, TRUE, TRUE, FALSE,
        &GUID_NULL,
        NULL,
        1,
        amfMTvideo
    },
    {
        wNull,
        FALSE, TRUE, TRUE, FALSE,
        &GUID_NULL,
        NULL,
        1,
        amfMTaudio
    },
};

static const AMOVIESETUP_FILTER amfQTV =
{   &CLSID_QTVDecoder,
    wQTVName,
    MERIT_NORMAL-1,
    2,
    amfQTVPin
};

static const AMOVIESETUP_FILTER amfQTD =
{   &CLSID_QTSplitter,
    wQTDName,
    MERIT_NORMAL-1,
    3,
    amfQTDPin
};

FactoryTemplate const g_Templates[] = {
    {
        wQTVName,
        &CLSID_QTVDecoder,
        QTVDecoder_create,
        NULL,
        &amfQTV,
    },
    {
        wQTDName,
        &CLSID_QTSplitter,
        QTSplitter_create,
        NULL,
        &amfQTD,
    }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
static HINSTANCE hInst = NULL;

/***********************************************************************
 *    Dll EntryPoint (wineqtdecoder.@)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    hInst = hInstDLL;
    return STRMBASE_DllMain(hInstDLL,fdwReason,lpv);
}

/***********************************************************************
 *    DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return STRMBASE_DllGetClassObject( rclsid, riid, ppv );
}

/***********************************************************************
 *    DllRegisterServer (wineqtdecoder.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT hr;
    TRACE("()\n");
    hr = AMovieDllRegisterServer2(TRUE);
    if (SUCCEEDED(hr))
        hr = __wine_register_resources(hInst);
    return hr;
}

/***********************************************************************
 *    DllUnregisterServer (wineqtdecoder.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hr;
    TRACE("\n");
    hr = AMovieDllRegisterServer2(FALSE);
    if (SUCCEEDED(hr))
        hr = __wine_unregister_resources(hInst);
    return hr;
}

/***********************************************************************
 *    DllCanUnloadNow (wineqtdecoder.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("\n");
    return STRMBASE_DllCanUnloadNow();
}
