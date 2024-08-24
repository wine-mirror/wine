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

extern HRESULT WINAPI QUARTZ_DllRegisterServer(void);
extern HRESULT WINAPI QUARTZ_DllUnregisterServer(void);

/***********************************************************************
 *		DllRegisterServer (QUARTZ.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    static const REGPINTYPES video_renderer_inputs[] =
    {
        {&MEDIATYPE_Video, &GUID_NULL},
    };
    static const REGFILTERPINS2 video_renderer_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(video_renderer_inputs),
            .lpMediaType = video_renderer_inputs,
            .dwFlags = REG_PINFLAG_B_RENDERER,
        },
    };
    static const REGFILTER2 video_renderer_default_reg =
    {
        .dwVersion = 2,
        .dwMerit = MERIT_PREFERRED + 1,
        .cPins2 = ARRAY_SIZE(video_renderer_pins),
        .rgPins2 = video_renderer_pins,
    };
    static const REGFILTER2 video_renderer_reg =
    {
        .dwVersion = 2,
        .dwMerit = MERIT_PREFERRED,
        .cPins2 = ARRAY_SIZE(video_renderer_pins),
        .rgPins2 = video_renderer_pins,
    };

    static const REGPINTYPES vmr9_filter_inputs[] =
    {
        {&MEDIATYPE_Video, &GUID_NULL},
    };
    static const REGFILTERPINS2 vmr9_filter_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(vmr9_filter_inputs),
            .lpMediaType = vmr9_filter_inputs,
            .dwFlags = REG_PINFLAG_B_RENDERER,
        },
    };
    static const REGFILTER2 vmr9_filter_reg =
    {
        .dwVersion = 2,
        .dwMerit = MERIT_DO_NOT_USE,
        .cPins2 = ARRAY_SIZE(vmr9_filter_pins),
        .rgPins2 = vmr9_filter_pins,
    };

    static const REGPINTYPES avi_decompressor_inputs[] =
    {
        {&MEDIATYPE_Video, &GUID_NULL},
    };
    static const REGPINTYPES avi_decompressor_outputs[] =
    {
        {&MEDIATYPE_Video, &GUID_NULL},
    };
    static const REGFILTERPINS2 avi_decompressor_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(avi_decompressor_inputs),
            .lpMediaType = avi_decompressor_inputs,
        },
        {
            .nMediaTypes = ARRAY_SIZE(avi_decompressor_outputs),
            .lpMediaType = avi_decompressor_outputs,
            .dwFlags = REG_PINFLAG_B_OUTPUT,
        },
    };
    static const REGFILTER2 avi_decompressor_reg =
    {
        .dwVersion = 2,
        .dwMerit = MERIT_NORMAL - 16,
        .cPins2 = ARRAY_SIZE(avi_decompressor_pins),
        .rgPins2 = avi_decompressor_pins,
    };

    static const REGPINTYPES async_reader_outputs[] =
    {
        {&MEDIATYPE_Stream, &GUID_NULL},
    };
    static const REGFILTERPINS2 async_reader_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(async_reader_outputs),
            .lpMediaType = async_reader_outputs,
            .dwFlags = REG_PINFLAG_B_OUTPUT,
        },
    };
    static const REGFILTER2 async_reader_reg =
    {
        .dwVersion = 2,
        .dwMerit = MERIT_UNLIKELY,
        .cPins2 = ARRAY_SIZE(async_reader_pins),
        .rgPins2 = async_reader_pins,
    };

    static const REGPINTYPES acm_wrapper_inputs[] =
    {
        {&MEDIATYPE_Audio, &GUID_NULL},
    };
    static const REGPINTYPES acm_wrapper_outputs[] =
    {
        {&MEDIATYPE_Audio, &GUID_NULL},
    };
    static const REGFILTERPINS2 acm_wrapper_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(acm_wrapper_inputs),
            .lpMediaType = acm_wrapper_inputs,
        },
        {
            .nMediaTypes = ARRAY_SIZE(acm_wrapper_outputs),
            .lpMediaType = acm_wrapper_outputs,
            .dwFlags = REG_PINFLAG_B_OUTPUT,
        },
    };
    static const REGFILTER2 acm_wrapper_reg =
    {
        .dwVersion = 2,
        .dwMerit = MERIT_NORMAL - 16,
        .cPins2 = ARRAY_SIZE(acm_wrapper_pins),
        .rgPins2 = acm_wrapper_pins,
    };

    IFilterMapper2 *mapper;
    HRESULT hr;

    TRACE("\n");

    if (FAILED(hr = QUARTZ_DllRegisterServer()))
        return hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper)))
        return hr;

    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_VideoRenderer, L"Video Renderer", NULL,
            &CLSID_LegacyAmFilterCategory, NULL, &video_renderer_reg)))
        goto done;
    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_VideoRendererDefault, L"Video Renderer", NULL,
            &CLSID_LegacyAmFilterCategory, NULL, &video_renderer_default_reg)))
        goto done;
    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_VideoMixingRenderer9, L"Video Mixing Renderer 9", NULL,
            &CLSID_LegacyAmFilterCategory, NULL, &vmr9_filter_reg)))
        goto done;
    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_AVIDec, L"AVI Decompressor", NULL,
            &CLSID_LegacyAmFilterCategory, NULL, &avi_decompressor_reg)))
        goto done;
    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_AsyncReader, L"File Source (Async.)", NULL,
            &CLSID_LegacyAmFilterCategory, NULL, &async_reader_reg)))
        goto done;
    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_ACMWrapper, L"ACM Wrapper", NULL,
            &CLSID_LegacyAmFilterCategory, NULL, &acm_wrapper_reg)))
        goto done;

done:
    IFilterMapper2_Release(mapper);
    return hr;
}

/***********************************************************************
 *		DllUnregisterServer (QUARTZ.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    IFilterMapper2 *mapper;
    HRESULT hr;

    TRACE("\n");

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper)))
        return hr;

    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, &CLSID_LegacyAmFilterCategory, NULL, &CLSID_VideoRenderer)))
        goto done;
    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, &CLSID_LegacyAmFilterCategory, NULL, &CLSID_VideoRendererDefault)))
        goto done;
    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, &CLSID_LegacyAmFilterCategory, NULL, &CLSID_VideoMixingRenderer9)))
        goto done;
    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, &CLSID_LegacyAmFilterCategory, NULL, &CLSID_AVIDec)))
        goto done;
    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, &CLSID_LegacyAmFilterCategory, NULL, &CLSID_AsyncReader)))
        goto done;
    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, &CLSID_LegacyAmFilterCategory, NULL, &CLSID_ACMWrapper)))
        goto done;

done:
    IFilterMapper2_Release(mapper);
    if (SUCCEEDED(hr))
        hr = QUARTZ_DllUnregisterServer();

    return hr;
}
