/*
 * Copyright 2022 Nikolay Sivov for CodeWeavers
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

#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"

#include "wine/mfinternal.h"
#include "wine/debug.h"

extern IClassFactory *wave_sink_class_factory;
extern IClassFactory *mp3_sink_class_factory;
extern IClassFactory *mpeg4_sink_class_factory;

static inline HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID riid, void **out)
{
    *out = IsEqualGUID(riid, &IID_IClassFactory) || IsEqualGUID(riid, &IID_IUnknown) ? iface : NULL;
    return *out ? S_OK : E_NOINTERFACE;
}
static inline ULONG WINAPI class_factory_AddRef(IClassFactory *iface)
{
    return 2;
}
static inline ULONG WINAPI class_factory_Release(IClassFactory *iface)
{
    return 1;
}
static inline HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL dolock)
{
    return S_OK;
}

struct sink_class_factory
{
    IClassFactory IClassFactory_iface;
    IMFSinkClassFactory IMFSinkClassFactory_iface;
};

static inline struct sink_class_factory *sink_class_factory_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct sink_class_factory, IClassFactory_iface);
}
static inline HRESULT WINAPI sink_class_factory_QueryInterface(IMFSinkClassFactory *iface, REFIID riid, void **out)
{
    *out = IsEqualGUID(riid, &IID_IMFSinkClassFactory) || IsEqualGUID(riid, &IID_IUnknown) ? iface : NULL;
    return *out ? S_OK : E_NOINTERFACE;
}
static inline ULONG WINAPI sink_class_factory_AddRef(IMFSinkClassFactory *iface)
{
    return 2;
}
static inline ULONG WINAPI sink_class_factory_Release(IMFSinkClassFactory *iface)
{
    return 1;
}

static inline const char *debugstr_time(LONGLONG time)
{
    ULONGLONG abstime = time >= 0 ? time : -time;
    unsigned int i = 0, j = 0;
    char buffer[23], rev[23];

    while (abstime || i <= 8)
    {
        buffer[i++] = '0' + (abstime % 10);
        abstime /= 10;
        if (i == 7) buffer[i++] = '.';
    }
    if (time < 0) buffer[i++] = '-';

    while (i--) rev[j++] = buffer[i];
    while (rev[j-1] == '0' && rev[j-2] != '.') --j;
    rev[j] = 0;

    return wine_dbg_sprintf("%s", rev);
}
