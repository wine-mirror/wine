/*
 * Copyright 2021 Nikolay Sivov for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "mfplay.h"

#include "wine/test.h"

static HRESULT WINAPI test_callback_QueryInterface(IMFPMediaPlayerCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFPMediaPlayerCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFPMediaPlayerCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_callback_AddRef(IMFPMediaPlayerCallback *iface)
{
    return 2;
}

static ULONG WINAPI test_callback_Release(IMFPMediaPlayerCallback *iface)
{
    return 1;
}

static void WINAPI test_callback_OnMediaPlayerEvent(IMFPMediaPlayerCallback *iface, MFP_EVENT_HEADER *event_header)
{
}

static const IMFPMediaPlayerCallbackVtbl test_callback_vtbl =
{
    test_callback_QueryInterface,
    test_callback_AddRef,
    test_callback_Release,
    test_callback_OnMediaPlayerEvent,
};

static void test_create_player(void)
{
    IMFPMediaPlayerCallback callback = { &test_callback_vtbl };
    IMFPMediaPlayer *player;
    HRESULT hr;

    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, NULL, NULL, &player);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IMFPMediaPlayer_Release(player);

    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, &callback, NULL, &player);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IMFPMediaPlayer_Release(player);
}

START_TEST(mfplay)
{
    test_create_player();
}
