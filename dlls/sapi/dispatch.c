/*
 * ITypeInfo cache for IDispatch
 *
 * Copyright 2019 Zebediah Figura
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

#include "wine/debug.h"

#define COBJMACROS

#include "objbase.h"
#include "sapiddk.h"

#include "sapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sapi);

static ITypeLib *typelib;
static ITypeInfo *typeinfos[last_tid];

static REFIID tid_id[] =
{
    &IID_ISpeechObjectToken,
    &IID_ISpeechObjectTokens,
    &IID_ISpeechVoice,
};

HRESULT get_typeinfo(enum type_id tid, ITypeInfo **ret)
{
    HRESULT hr;

    if (!typelib)
    {
        ITypeLib *tl;

        hr = LoadRegTypeLib(&LIBID_SpeechLib, 5, 4, LOCALE_SYSTEM_DEFAULT, &tl);
        if (FAILED(hr))
        {
            ERR("Failed to load typelib, hr %#lx.\n", hr);
            return hr;
        }
        if (InterlockedCompareExchangePointer((void **)&typelib, tl, NULL))
            ITypeLib_Release(tl);
    }
    if (!typeinfos[tid])
    {
        ITypeInfo *typeinfo;

        hr = ITypeLib_GetTypeInfoOfGuid(typelib, tid_id[tid], &typeinfo);
        if (FAILED(hr))
        {
            ERR("Failed to get type info for %s, hr %#lx.\n", debugstr_guid(tid_id[tid]), hr);
            return hr;
        }
        if (InterlockedCompareExchangePointer((void **)(typeinfos + tid), typeinfo, NULL))
            ITypeInfo_Release(typeinfo);
    }
    ITypeInfo_AddRef(*ret = typeinfos[tid]);
    return S_OK;
}

void release_typelib(void)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(typeinfos); ++i)
    {
        if (typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);
    }
    if (typelib)
        ITypeLib_Release(typelib);
}
