/*
 * Unit tests for dmusic functions
 *
 * Copyright (C) 2012 Christian Costa
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

#include <stdio.h>

#include "wine/test.h"
#include "uuids.h"
#include "ole2.h"
#include "initguid.h"
#include "dmusici.h"
#include "dmksctrl.h"

static inline const char* debugstr_guid(const GUID *id)
{
    static char string[39];
    sprintf(string, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            id->Data1, id->Data2, id->Data3,
            id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
            id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
    return string;
}

static inline const char* debugstr_longlong(ULONGLONG ll)
{
    static char string[17];
    if (sizeof(ll) > sizeof(unsigned long) && ll >> 32)
        sprintf(string, "%lx%08lx", (unsigned long)(ll >> 32), (unsigned long)ll);
    else
        sprintf(string, "%lx", (unsigned long)ll);
    return string;
}

static void test_dmusic(void)
{
    IDirectMusic *dmusic = NULL;
    HRESULT hr;
    ULONG index = 0;
    DMUS_PORTCAPS port_caps;
    DMUS_PORTPARAMS port_params;
    IDirectMusicPort *port = NULL;
    DMUS_CLOCKINFO clock_info;
    GUID guid_clock;
    IReferenceClock *clock = NULL;

    hr = CoCreateInstance(&CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusic, (LPVOID*)&dmusic);
    if (hr != S_OK)
    {
        skip("Cannot create DirectMusic object (%x)\n", hr);
        return;
    }

    hr = IDirectMusic_GetMasterClock(dmusic, &guid_clock, &clock);
    ok(hr == S_OK, "IDirectMusic_GetMasterClock returned: %x\n", hr);
    ok(clock != NULL, "No clock returned\n");
    trace("  guidPort = %s\n", debugstr_guid(&guid_clock));
    if (clock)
        IReferenceClock_Release(clock);

    port_params.dwSize = sizeof(port_params);
    port_params.dwValidParams = DMUS_PORTPARAMS_CHANNELGROUPS | DMUS_PORTPARAMS_AUDIOCHANNELS;
    port_params.dwChannelGroups = 1;
    port_params.dwAudioChannels = 2;

    /* No port can be created before SetDirectSound is called */
    hr = IDirectMusic_CreatePort(dmusic, &GUID_NULL, &port_params, &port, NULL);
    todo_wine ok(hr == DMUS_E_DSOUND_NOT_SET, "IDirectMusic_CreatePort returned: %x\n", hr);

    hr = IDirectMusic_SetDirectSound(dmusic, NULL, NULL);
    ok(hr == S_OK, "IDirectMusic_SetDirectSound returned: %x\n", hr);

    /* Check wrong params */
    hr = IDirectMusic_CreatePort(dmusic, &GUID_NULL, &port_params, &port, (IUnknown*)dmusic);
    ok(hr == CLASS_E_NOAGGREGATION, "IDirectMusic_CreatePort returned: %x\n", hr);
    hr = IDirectMusic_CreatePort(dmusic, NULL, &port_params, &port, NULL);
    ok(hr == E_POINTER, "IDirectMusic_CreatePort returned: %x\n", hr);
    hr = IDirectMusic_CreatePort(dmusic, &GUID_NULL, NULL, &port, NULL);
    ok(hr == E_INVALIDARG, "IDirectMusic_CreatePort returned: %x\n", hr);
    hr = IDirectMusic_CreatePort(dmusic, &GUID_NULL, &port_params, NULL, NULL);
    ok(hr == E_POINTER, "IDirectMusic_CreatePort returned: %x\n", hr);

    /* Test creation of default port with GUID_NULL */
    hr = IDirectMusic_CreatePort(dmusic, &GUID_NULL, &port_params, &port, NULL);
    ok(hr == S_OK, "IDirectMusic_CreatePort returned: %x\n", hr);

    port_caps.dwSize = sizeof(port_caps);
    while (IDirectMusic_EnumPort(dmusic, index, &port_caps) == S_OK)
    {
        ok(port_caps.dwSize == sizeof(port_caps), "DMUS_PORTCAPS dwSize member is wrong (%u)\n", port_caps.dwSize);
        trace("Port %u:\n", index);
        trace("  dwFlags            = %x\n", port_caps.dwFlags);
        trace("  guidPort           = %s\n", debugstr_guid(&port_caps.guidPort));
        trace("  dwClass            = %u\n", port_caps.dwClass);
        trace("  dwType             = %u\n", port_caps.dwType);
        trace("  dwMemorySize       = %u\n", port_caps.dwMemorySize);
        trace("  dwMaxChannelGroups = %u\n", port_caps.dwMaxChannelGroups);
        trace("  dwMaxVoices        = %u\n", port_caps.dwMaxVoices);
        trace("  dwMaxAudioChannels = %u\n", port_caps.dwMaxAudioChannels);
        trace("  dwEffectFlags      = %x\n", port_caps.dwEffectFlags);
        trace("  wszDescription     = %s\n", wine_dbgstr_w(port_caps.wszDescription));
        index++;
    }

    index = 0;
    clock_info.dwSize = sizeof(clock_info);
    while (IDirectMusic_EnumMasterClock(dmusic, index, &clock_info) == S_OK)
    {
        ok(clock_info.dwSize == sizeof(clock_info), "DMUS_CLOCKINFO dwSize member is wrong (%u)\n", clock_info.dwSize);
        trace("Clock %u:\n", index);
        trace("  ctType         = %u\n", clock_info.ctType);
        trace("  guidClock      = %s\n", debugstr_guid(&clock_info.guidClock));
        trace("  wszDescription = %s\n", wine_dbgstr_w(clock_info.wszDescription));
        index++;
    }

    if (port)
        IDirectMusicPort_Release(port);
    IDirectMusic_Release(dmusic);
}

static void test_dmbuffer(void)
{
    IDirectMusic *dmusic;
    IDirectMusicBuffer *dmbuffer = NULL;
    HRESULT hr;
    DMUS_BUFFERDESC desc;
    GUID format;
    DWORD size;
    DWORD bytes;
    REFERENCE_TIME time;
    LPBYTE data;

    hr = CoCreateInstance(&CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusic, (LPVOID*)&dmusic);
    if (hr != S_OK)
    {
        skip("Cannot create DirectMusic object (%x)\n", hr);
        return;
    }

    desc.dwSize = sizeof(DMUS_BUFFERDESC);
    desc.dwFlags = 0;
    desc.cbBuffer = 1023;
    memcpy(&desc.guidBufferFormat, &GUID_NULL, sizeof(GUID));

    hr = IDirectMusic_CreateMusicBuffer(dmusic, &desc, &dmbuffer, NULL);
    ok(hr == S_OK, "IDirectMusic_CreateMusicBuffer return %x\n", hr);

    hr = IDirectMusicBuffer_GetBufferFormat(dmbuffer, &format);
    ok(hr == S_OK, "IDirectMusicBuffer_GetBufferFormat returned %x\n", hr);
    ok(IsEqualGUID(&format, &KSDATAFORMAT_SUBTYPE_MIDI), "Wrong format returned %s\n", debugstr_guid(&format));
    hr = IDirectMusicBuffer_GetMaxBytes(dmbuffer, &size);
    ok(hr == S_OK, "IDirectMusicBuffer_GetMaxBytes returned %x\n", hr);
    ok(size == 1024, "Buffer size is %u instead of 1024\n", size);

    hr = IDirectMusicBuffer_GetStartTime(dmbuffer, &time);
    ok(hr == DMUS_E_BUFFER_EMPTY, "IDirectMusicBuffer_GetStartTime returned %x\n", hr);
    hr = IDirectMusicBuffer_SetStartTime(dmbuffer, 10);
    ok(hr == S_OK, "IDirectMusicBuffer_GetStartTime returned %x\n", hr);
    hr = IDirectMusicBuffer_GetStartTime(dmbuffer, &time);
    ok(hr == DMUS_E_BUFFER_EMPTY, "IDirectMusicBuffer_GetStartTime returned %x\n", hr);

    hr = IDirectMusicBuffer_PackStructured(dmbuffer, 20, 0, 0);
    ok(hr == DMUS_E_INVALID_EVENT, "IDirectMusicBuffer_PackStructured returned %x\n", hr);
    hr = IDirectMusicBuffer_PackStructured(dmbuffer, 20, 0, 0x000090); /* note on : chan 0, note 0 & vel 0 */
    ok(hr == S_OK, "IDirectMusicBuffer_PackStructured returned %x\n", hr);
    hr = IDirectMusicBuffer_PackStructured(dmbuffer, 30, 0, 0x000080); /* note off : chan 0, note 0 & vel 0 */
    ok(hr == S_OK, "IDirectMusicBuffer_PackStructured returned %x\n", hr);
    hr = IDirectMusicBuffer_GetUsedBytes(dmbuffer, &bytes);
    ok(hr == S_OK, "IDirectMusicBuffer_GetUsedBytes returned %x\n", hr);
    ok(bytes == 48, "Buffer size is %u instead of 48\n", bytes);

    hr = IDirectMusicBuffer_GetStartTime(dmbuffer, &time);
    ok(hr == S_OK, "IDirectMusicBuffer_GetStartTime returned %x\n", hr);
    ok(time == 20, "Buffer start time is wrong\n");
    hr = IDirectMusicBuffer_SetStartTime(dmbuffer, 40);
    ok(hr == S_OK, "IDirectMusicBuffer_GetStartTime returned %x\n", hr);
    hr = IDirectMusicBuffer_GetStartTime(dmbuffer, &time);
    ok(hr == S_OK, "IDirectMusicBuffer_GetStartTime returned %x\n", hr);
    ok(time == 40, "Buffer start time is wrong\n");

    hr = IDirectMusicBuffer_GetRawBufferPtr(dmbuffer, &data);
    ok(hr == S_OK, "IDirectMusicBuffer_GetRawBufferPtr returned %x\n", hr);
    if (hr == S_OK)
    {
        DMUS_EVENTHEADER* header;
        DWORD message;

        /* Check message 1 */
        header = (DMUS_EVENTHEADER*)data;
        data += sizeof(DMUS_EVENTHEADER);
        ok(header->cbEvent == 3, "cbEvent is %u instead of 3\n", header->cbEvent);
        ok(header->dwChannelGroup == 0, "dwChannelGroup is %u instead of 0\n", header->dwChannelGroup);
        ok(header->rtDelta == 0, "rtDelta is %s instead of 0\n", debugstr_longlong(header->rtDelta));
        ok(header->dwFlags == DMUS_EVENT_STRUCTURED, "dwFlags is %x instead of %x\n", header->dwFlags, DMUS_EVENT_STRUCTURED);
        message = *(DWORD*)data & 0xffffff; /* Only 3 bytes are relevant */
        data += sizeof(DWORD);
        ok(message == 0x000090, "Message is %0x instead of 0x000090\n", message);

        /* Check message 2 */
        header = (DMUS_EVENTHEADER*)data;
        data += sizeof(DMUS_EVENTHEADER);
        ok(header->cbEvent == 3, "cbEvent is %u instead of 3\n", header->cbEvent);
        ok(header->dwChannelGroup == 0, "dwChannelGroup is %u instead of 0\n", header->dwChannelGroup);
        ok(header->rtDelta == 10, "rtDelta is %s instead of 0\n", debugstr_longlong(header->rtDelta));
        ok(header->dwFlags == DMUS_EVENT_STRUCTURED, "dwFlags is %x instead of %x\n", header->dwFlags, DMUS_EVENT_STRUCTURED);
        message = *(DWORD*)data & 0xffffff; /* Only 3 bytes are relevant */
        ok(message == 0x000080, "Message 2 is %0x instead of 0x000080\n", message);
    }

    if (dmbuffer)
        IDirectMusicBuffer_Release(dmbuffer);
    IDirectMusic_Release(dmusic);
}

START_TEST(dmusic)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    test_dmusic();
    test_dmbuffer();

    CoUninitialize();
}
