/*
 * Copyright 2022 Dmitry Timoshkov
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

#include <stdarg.h>

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <objbase.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dllhost);

struct surrogate
{
    ISurrogate ISurrogate_iface;
    LONG ref;
};

static inline struct surrogate *impl_from_ISurrogate(ISurrogate *iface)
{
    return CONTAINING_RECORD(iface, struct surrogate, ISurrogate_iface);
}

static HRESULT WINAPI surrogate_QueryInterface(ISurrogate *iface,
    REFIID iid, void **ppv)
{
    struct surrogate *surrogate = impl_from_ISurrogate(iface);

    TRACE("(%p,%s,%p)\n", iface, wine_dbgstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_ISurrogate))
    {
        ISurrogate_AddRef(&surrogate->ISurrogate_iface);
        *ppv = &surrogate->ISurrogate_iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI surrogate_AddRef(ISurrogate *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI surrogate_Release(ISurrogate *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI surrogate_LoadDllServer(ISurrogate *iface, const CLSID *clsid)
{
    FIXME("(%p,%s): stub\n", iface, wine_dbgstr_guid(clsid));
    return E_NOTIMPL;
}

static HRESULT WINAPI surrogate_FreeSurrogate(ISurrogate *iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static const ISurrogateVtbl Surrogate_Vtbl =
{
    surrogate_QueryInterface,
    surrogate_AddRef,
    surrogate_Release,
    surrogate_LoadDllServer,
    surrogate_FreeSurrogate
};

int WINAPI wWinMain(HINSTANCE hinst, HINSTANCE previnst, LPWSTR cmdline, int showcmd)
{
    HRESULT hr;
    CLSID clsid;
    struct surrogate surrogate;

    if (wcsnicmp(cmdline, L"/PROCESSID:", 11))
        return 0;

    cmdline += 11;

    surrogate.ISurrogate_iface.lpVtbl = &Surrogate_Vtbl;
    surrogate.ref = 1;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CLSIDFromString(cmdline, &clsid);
    if (hr == S_OK)
    {
        CoRegisterSurrogate(&surrogate.ISurrogate_iface);

        hr = ISurrogate_LoadDllServer(&surrogate.ISurrogate_iface, &clsid);
        if (hr != S_OK)
        {
            ERR("Can't create instance of %s\n", wine_dbgstr_guid(&clsid));
            goto cleanup;
        }

        /* FIXME: wait for FreeSurrogate being called */
        Sleep(INFINITE);
    }

cleanup:
    CoUninitialize();

    return 0;
}
