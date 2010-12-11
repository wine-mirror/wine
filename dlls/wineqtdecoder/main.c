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

#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/strmbase.h"

#include "initguid.h"
DEFINE_GUID(CLSID_QTVDecoder, 0x683DDACB, 0x4354, 0x490C, 0xA0,0x58, 0xE0,0x5A,0xD0,0xF2,0x05,0x37);

WINE_DEFAULT_DEBUG_CHANNEL(qtdecoder);

extern IUnknown * CALLBACK QTVDecoder_create(IUnknown * pUnkOuter, HRESULT* phr);

static const WCHAR wQTVName[] =
{'Q','T',' ','V','i','d','e','o',' ','D','e','c','o','d','e','r',0};
static WCHAR wNull[] = {'\0'};

static const AMOVIESETUP_MEDIATYPE amfMTvideo[] =
{   { &MEDIATYPE_Video, &MEDIASUBTYPE_NULL } };

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

static const AMOVIESETUP_FILTER amfQTV =
{   &CLSID_QTVDecoder,
    wQTVName,
    MERIT_NORMAL,
    2,
    amfQTVPin
};

FactoryTemplate const g_Templates[] = {
    {
        wQTVName,
        &CLSID_QTVDecoder,
        QTVDecoder_create,
        NULL,
        &amfQTV,
    }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

/***********************************************************************
 *    Dll EntryPoint (wineqtdecoder.@)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
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
    TRACE("()\n");
    return AMovieDllRegisterServer2(TRUE);
}

/***********************************************************************
 *    DllUnregisterServer (wineqtdecoder.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    TRACE("\n");
    return AMovieDllRegisterServer2(FALSE);
}

/***********************************************************************
 *    DllCanUnloadNow (wineqtdecoder.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("\n");
    return STRMBASE_DllCanUnloadNow();
}
