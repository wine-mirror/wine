/*
 * Unit test suite for IDirectMusicPerformance
 *
 * Copyright 2010 Austin Lund
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
#include <windef.h>
#include <initguid.h>
#include <wine/test.h>
#include <dmusici.h>

IDirectMusicPerformance8 *idmusicperformance;

static HRESULT test_InitAudio(void)
{
    IDirectSound *pDirectSound;
    HRESULT hr;

    pDirectSound = NULL;
    hr = IDirectMusicPerformance8_InitAudio(idmusicperformance ,NULL,
        &pDirectSound, NULL, DMUS_APATH_SHARED_STEREOPLUSREVERB, 128, DMUS_AUDIOF_ALL, NULL);
    return hr;
}

static void test_PChannelInfo(void)
{
    IDirectMusicPort *pDirectMusicPort;
    HRESULT hr;

    pDirectMusicPort = NULL;
    hr = IDirectMusicPerformance8_PChannelInfo(idmusicperformance, 0, &pDirectMusicPort, NULL, NULL);
    ok(hr == S_OK, "Failed to call PChannelInfo (%x)\n", hr);
    ok(pDirectMusicPort != NULL, "IDirectMusicPort not set\n");
    if (hr == S_OK && pDirectMusicPort != NULL)
        IDirectMusicPort_Release(pDirectMusicPort);
}

static void test_GetDefaultAudioPath(void)
{
    IDirectMusicAudioPath *pDirectMusicAudioPath;
    HRESULT hr;

    hr = IDirectMusicPerformance8_GetDefaultAudioPath(idmusicperformance, &pDirectMusicAudioPath);
    ok(hr == S_OK, "Failed to call GetDefaultAudioPath (%x)\n", hr);
    if (hr == S_OK)
        IDirectMusicAudioPath_Release(pDirectMusicAudioPath);
}

static void test_CloseDown(void)
{
    HRESULT hr;

    hr = IDirectMusicPerformance8_CloseDown(idmusicperformance);
    ok(hr == S_OK, "Failed to call CloseDown (%x)\n", hr);
}

START_TEST( performance )
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        skip("Cannot initialize COM (%x)\n", hr);
        return;
    }

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL,
            CLSCTX_INPROC_SERVER, &IID_IDirectMusicPerformance8, (LPVOID *)&idmusicperformance);
    if (hr != S_OK) {
        skip("Cannot create DirectMusicPerformance object (%x)\n", hr);
        CoUninitialize();
        return;
    }

    hr = test_InitAudio();
    if (hr != S_OK) {
        skip("InitAudio failed (%x)\n", hr);
        return;
    }

    test_GetDefaultAudioPath();
    test_PChannelInfo();
    test_CloseDown();

    IDirectMusicPerformance8_Release(idmusicperformance);
    CoUninitialize();
}
