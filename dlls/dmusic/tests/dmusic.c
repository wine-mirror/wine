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
#include "dmusicf.h"
#include "dmksctrl.h"

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    ULONG expect_ref = get_refcount(iface_ptr);
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected;
    IUnknown *unk;

    expected = supported ? S_OK : E_NOINTERFACE;
    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected, "got hr %#lx, expected %#lx.\n", hr, expected);
    if (SUCCEEDED(hr))
    {
        LONG ref = get_refcount(unk);
        ok_(__FILE__, line)(ref == expect_ref + 1, "got %ld\n", ref);
        IUnknown_Release(unk);
        ref = get_refcount(iface_ptr);
        ok_(__FILE__, line)(ref == expect_ref, "got %ld\n", ref);
    }
}

static void stream_begin_chunk(IStream *stream, const char type[5], ULARGE_INTEGER *offset)
{
    static const LARGE_INTEGER zero = {0};
    HRESULT hr;
    hr = IStream_Write(stream, type, 4, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IStream_Seek(stream, zero, STREAM_SEEK_CUR, offset);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IStream_Write(stream, "\0\0\0\0", 4, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
}

static void stream_end_chunk(IStream *stream, ULARGE_INTEGER *offset)
{
    static const LARGE_INTEGER zero = {0};
    ULARGE_INTEGER position;
    HRESULT hr;
    UINT size;
    hr = IStream_Seek(stream, zero, STREAM_SEEK_CUR, &position);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IStream_Seek(stream, *(LARGE_INTEGER *)offset, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    size = position.QuadPart - offset->QuadPart - 4;
    hr = IStream_Write(stream, &size, 4, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IStream_Seek(stream, *(LARGE_INTEGER *)&position, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IStream_Write(stream, &zero, (position.QuadPart & 1), NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
}

#define CHUNK_BEGIN(stream, type)                                \
    do {                                                         \
        ULARGE_INTEGER __off;                                    \
        IStream *__stream = (stream);                            \
        stream_begin_chunk(stream, type, &__off);                \
        do

#define CHUNK_RIFF(stream, form)                                 \
    do {                                                         \
        ULARGE_INTEGER __off;                                    \
        IStream *__stream = (stream);                            \
        stream_begin_chunk(stream, "RIFF", &__off);              \
        IStream_Write(stream, form, 4, NULL);                    \
        do

#define CHUNK_LIST(stream, form)                                 \
    do {                                                         \
        ULARGE_INTEGER __off;                                    \
        IStream *__stream = (stream);                            \
        stream_begin_chunk(stream, "LIST", &__off);              \
        IStream_Write(stream, form, 4, NULL);                    \
        do

#define CHUNK_END                                                \
        while (0);                                               \
        stream_end_chunk(__stream, &__off);                      \
    } while (0)

#define CHUNK_DATA(stream, type, data)                           \
    CHUNK_BEGIN(stream, type)                                    \
    {                                                            \
        IStream_Write((stream), &(data), sizeof(data), NULL);    \
    }                                                            \
    CHUNK_END

static BOOL compare_time(REFERENCE_TIME x, REFERENCE_TIME y, unsigned int max_diff)
{
    REFERENCE_TIME diff = x > y ? x - y : y - x;
    return diff <= max_diff;
}

static void test_dmusic(void)
{
    IDirectMusic *dmusic = NULL;
    HRESULT hr;
    ULONG index = 0;
    DMUS_PORTCAPS port_caps;
    DMUS_PORTPARAMS port_params;
    IDirectMusicPort *port = NULL;

    hr = CoCreateInstance(&CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusic, (LPVOID*)&dmusic);
    ok(hr == S_OK, "Cannot create DirectMusic object: %#lx\n", hr);

    port_params.dwSize = sizeof(port_params);
    port_params.dwValidParams = DMUS_PORTPARAMS_CHANNELGROUPS | DMUS_PORTPARAMS_AUDIOCHANNELS;
    port_params.dwChannelGroups = 1;
    port_params.dwAudioChannels = 2;

    /* No port can be created before SetDirectSound is called */
    hr = IDirectMusic_CreatePort(dmusic, &GUID_NULL, &port_params, &port, NULL);
    ok(hr == DMUS_E_DSOUND_NOT_SET, "IDirectMusic_CreatePort returned: %#lx\n", hr);

    hr = IDirectMusic_SetDirectSound(dmusic, NULL, NULL);
    ok(hr == S_OK, "IDirectMusic_SetDirectSound returned: %#lx\n", hr);

    /* Check wrong params */
    hr = IDirectMusic_CreatePort(dmusic, &GUID_NULL, &port_params, &port, (IUnknown*)dmusic);
    ok(hr == CLASS_E_NOAGGREGATION, "IDirectMusic_CreatePort returned: %#lx\n", hr);
    hr = IDirectMusic_CreatePort(dmusic, NULL, &port_params, &port, NULL);
    ok(hr == E_POINTER, "IDirectMusic_CreatePort returned: %#lx\n", hr);
    hr = IDirectMusic_CreatePort(dmusic, &GUID_NULL, NULL, &port, NULL);
    ok(hr == E_INVALIDARG, "IDirectMusic_CreatePort returned: %#lx\n", hr);
    hr = IDirectMusic_CreatePort(dmusic, &GUID_NULL, &port_params, NULL, NULL);
    ok(hr == E_POINTER, "IDirectMusic_CreatePort returned: %#lx\n", hr);

    /* Test creation of default port with GUID_NULL */
    hr = IDirectMusic_CreatePort(dmusic, &GUID_NULL, &port_params, &port, NULL);
    ok(hr == S_OK, "IDirectMusic_CreatePort returned: %#lx\n", hr);

    port_caps.dwSize = sizeof(port_caps);
    while (IDirectMusic_EnumPort(dmusic, index, &port_caps) == S_OK)
    {
        ok(port_caps.dwSize == sizeof(port_caps), "DMUS_PORTCAPS dwSize member is wrong (%lu)\n", port_caps.dwSize);
        trace("Port %lu:\n", index);
        trace("  dwFlags            = %lx\n", port_caps.dwFlags);
        trace("  guidPort           = %s\n", wine_dbgstr_guid(&port_caps.guidPort));
        trace("  dwClass            = %lu\n", port_caps.dwClass);
        trace("  dwType             = %lu\n", port_caps.dwType);
        trace("  dwMemorySize       = %lu\n", port_caps.dwMemorySize);
        trace("  dwMaxChannelGroups = %lu\n", port_caps.dwMaxChannelGroups);
        trace("  dwMaxVoices        = %lu\n", port_caps.dwMaxVoices);
        trace("  dwMaxAudioChannels = %lu\n", port_caps.dwMaxAudioChannels);
        trace("  dwEffectFlags      = %lx\n", port_caps.dwEffectFlags);
        trace("  wszDescription     = %s\n", wine_dbgstr_w(port_caps.wszDescription));
        index++;
    }

    if (port)
        IDirectMusicPort_Release(port);
    IDirectMusic_Release(dmusic);
}

static void test_setdsound(void)
{
    IDirectMusic *dmusic;
    IDirectSound *dsound, *dsound2;
    DMUS_PORTPARAMS params;
    IDirectMusicPort *port = NULL;
    HRESULT hr;
    ULONG ref;

    params.dwSize = sizeof(params);
    params.dwValidParams = DMUS_PORTPARAMS_CHANNELGROUPS | DMUS_PORTPARAMS_AUDIOCHANNELS;
    params.dwChannelGroups = 1;
    params.dwAudioChannels = 2;

    /* Old dsound without SetCooperativeLevel() */
    hr = DirectSoundCreate(NULL, &dsound, NULL);
    if (hr == DSERR_NODRIVER ) {
        skip("No driver\n");
        return;
    }
    ok(hr == S_OK, "DirectSoundCreate failed: %#lx\n", hr);
    hr = CoCreateInstance(&CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusic8,
            (void **)&dmusic);
    ok(hr == S_OK, "DirectMusic create failed: %#lx\n", hr);
    hr = IDirectMusic_SetDirectSound(dmusic, dsound, NULL);
    ok(hr == S_OK, "SetDirectSound failed: %#lx\n", hr);
    hr = IDirectMusic_CreatePort(dmusic, &GUID_NULL, &params, &port, NULL);
    ok(hr == S_OK, "CreatePort returned: %#lx\n", hr);
    IDirectMusicPort_Release(port);
    IDirectMusic_Release(dmusic);
    IDirectSound_Release(dsound);

    /* dsound ref counting */
    hr = CoCreateInstance(&CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusic8,
            (void **)&dmusic);
    ok(hr == S_OK, "DirectMusic create failed: %#lx\n", hr);
    hr = DirectSoundCreate8(NULL, (IDirectSound8 **)&dsound, NULL);
    ok(hr == S_OK, "DirectSoundCreate failed: %#lx\n", hr);
    hr = IDirectSound_SetCooperativeLevel(dsound, GetForegroundWindow(), DSSCL_PRIORITY);
    ok(hr == S_OK, "SetCooperativeLevel failed: %#lx\n", hr);
    hr = IDirectMusic_SetDirectSound(dmusic, dsound, NULL);
    ok(hr == S_OK, "SetDirectSound failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 2, "dsound ref count got %ld expected 2\n", ref);
    hr = IDirectMusic_CreatePort(dmusic, &GUID_NULL, &params, &port, NULL);
    ok(hr == S_OK, "CreatePort returned: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 2, "dsound ref count got %ld expected 2\n", ref);
    IDirectMusicPort_AddRef(port);
    ref = IDirectMusicPort_Release(port);
    ok(ref == 1, "port ref count got %ld expected 1\n", ref);
    hr = IDirectMusicPort_Activate(port, TRUE);
    ok(hr == S_OK, "Port Activate returned: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 4, "dsound ref count got %ld expected 4\n", ref);
    IDirectMusicPort_AddRef(port);
    ref = IDirectMusicPort_Release(port);
    ok(ref == 1, "port ref count got %ld expected 1\n", ref);

    /* Releasing dsound from dmusic */
    hr = IDirectMusic_SetDirectSound(dmusic, NULL, NULL);
    ok(hr == DMUS_E_DSOUND_ALREADY_SET, "SetDirectSound failed: %#lx\n", hr);
    hr = IDirectMusicPort_Activate(port, FALSE);
    ok(hr == S_OK, "Port Activate returned: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 2, "dsound ref count got %ld expected 2\n", ref);
    hr = IDirectMusic_SetDirectSound(dmusic, NULL, NULL);
    ok(hr == S_OK, "SetDirectSound failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 1, "dsound ref count got %ld expected 1\n", ref);

    /* Setting the same dsound twice */
    hr = IDirectMusic_SetDirectSound(dmusic, dsound, NULL);
    ok(hr == S_OK, "SetDirectSound failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 2, "dsound ref count got %ld expected 2\n", ref);
    hr = IDirectMusic_SetDirectSound(dmusic, dsound, NULL);
    ok(hr == S_OK, "SetDirectSound failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 2, "dsound ref count got %ld expected 2\n", ref);

    /* Replacing one dsound with another */
    hr = DirectSoundCreate8(NULL, (IDirectSound8 **)&dsound2, NULL);
    ok(hr == S_OK, "DirectSoundCreate failed: %#lx\n", hr);
    hr = IDirectMusic_SetDirectSound(dmusic, dsound2, NULL);
    ok(hr == S_OK, "SetDirectSound failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 1, "dsound ref count got %ld expected 1\n", ref);
    ref = get_refcount(dsound2);
    ok(ref == 2, "dsound2 ref count got %ld expected 2\n", ref);

    /* Replacing the dsound in the port */
    hr = IDirectMusicPort_SetDirectSound(port, dsound, NULL);
    ok(hr == S_OK, "SetDirectSound failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 2, "dsound ref count got %ld expected 2\n", ref);
    ref = get_refcount(dsound2);
    ok(ref == 2, "dsound2 ref count got %ld expected 2\n", ref);
    /* Setting the dsound again on the port will mess with the parent dmusic */
    hr = IDirectMusicPort_SetDirectSound(port, dsound, NULL);
    ok(hr == S_OK, "SetDirectSound failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 3, "dsound ref count got %ld expected 3\n", ref);
    ref = get_refcount(dsound2);
    ok(ref == 1, "dsound2 ref count got %ld expected 1\n", ref);
    IDirectSound_AddRef(dsound2); /* Crash prevention */
    hr = IDirectMusicPort_Activate(port, TRUE);
    ok(hr == S_OK, "Activate returned: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 4, "dsound ref count got %ld expected 4\n", ref);
    ref = get_refcount(dsound2);
    ok(ref == 2, "dsound2 ref count got %ld expected 2\n", ref);
    hr = IDirectMusicPort_Activate(port, TRUE);
    ok(hr == S_FALSE, "Activate returned: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 4, "dsound ref count got %ld expected 4\n", ref);
    ref = get_refcount(dsound2);
    ok(ref == 2, "dsound2 ref count got %ld expected 2\n", ref);

    /* Deactivating the port messes with the dsound refcount in the parent dmusic */
    hr = IDirectMusicPort_Activate(port, FALSE);
    ok(hr == S_OK, "Port Activate returned: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 3, "dsound ref count got %ld expected 3\n", ref);
    ref = get_refcount(dsound2);
    ok(ref == 1, "dsound2 ref count got %ld expected 1\n", ref);
    hr = IDirectMusicPort_Activate(port, FALSE);
    ok(hr == S_FALSE, "Port Activate returned: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 3, "dsound ref count got %ld expected 3\n", ref);
    ref = get_refcount(dsound2);
    ok(ref == 1, "dsound2 ref count got %ld expected 1\n", ref);

    IDirectMusicPort_Release(port);
    IDirectMusic_Release(dmusic);
    while (IDirectSound_Release(dsound));
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
    ok(hr == S_OK, "Cannot create DirectMusic object: %#lx\n", hr);

    desc.dwSize = sizeof(DMUS_BUFFERDESC);
    desc.dwFlags = 0;
    desc.cbBuffer = 1023;
    memcpy(&desc.guidBufferFormat, &GUID_NULL, sizeof(GUID));

    hr = IDirectMusic_CreateMusicBuffer(dmusic, &desc, &dmbuffer, NULL);
    ok(hr == S_OK, "IDirectMusic_CreateMusicBuffer returned %#lx\n", hr);

    hr = IDirectMusicBuffer_GetBufferFormat(dmbuffer, &format);
    ok(hr == S_OK, "IDirectMusicBuffer_GetBufferFormat returned %#lx\n", hr);
    ok(IsEqualGUID(&format, &KSDATAFORMAT_SUBTYPE_MIDI), "Wrong format returned %s\n", wine_dbgstr_guid(&format));
    hr = IDirectMusicBuffer_GetMaxBytes(dmbuffer, &size);
    ok(hr == S_OK, "IDirectMusicBuffer_GetMaxBytes returned %#lx\n", hr);
    ok(size == 1024, "Buffer size is %lu instead of 1024\n", size);

    hr = IDirectMusicBuffer_GetStartTime(dmbuffer, &time);
    ok(hr == DMUS_E_BUFFER_EMPTY, "IDirectMusicBuffer_GetStartTime returned %#lx\n", hr);
    hr = IDirectMusicBuffer_SetStartTime(dmbuffer, 10);
    ok(hr == S_OK, "IDirectMusicBuffer_GetStartTime returned %#lx\n", hr);
    hr = IDirectMusicBuffer_GetStartTime(dmbuffer, &time);
    ok(hr == DMUS_E_BUFFER_EMPTY, "IDirectMusicBuffer_GetStartTime returned %#lx\n", hr);

    hr = IDirectMusicBuffer_PackStructured(dmbuffer, 20, 0, 0);
    ok(hr == DMUS_E_INVALID_EVENT, "IDirectMusicBuffer_PackStructured returned %#lx\n", hr);
    hr = IDirectMusicBuffer_PackStructured(dmbuffer, 20, 0, 0x000090); /* note on : chan 0, note 0 & vel 0 */
    ok(hr == S_OK, "IDirectMusicBuffer_PackStructured returned %#lx\n", hr);
    hr = IDirectMusicBuffer_PackStructured(dmbuffer, 30, 0, 0x000080); /* note off : chan 0, note 0 & vel 0 */
    ok(hr == S_OK, "IDirectMusicBuffer_PackStructured returned %#lx\n", hr);
    hr = IDirectMusicBuffer_GetUsedBytes(dmbuffer, &bytes);
    ok(hr == S_OK, "IDirectMusicBuffer_GetUsedBytes returned %#lx\n", hr);
    ok(bytes == 48, "Buffer size is %lu instead of 48\n", bytes);

    hr = IDirectMusicBuffer_GetStartTime(dmbuffer, &time);
    ok(hr == S_OK, "IDirectMusicBuffer_GetStartTime returned %#lx\n", hr);
    ok(time == 20, "Buffer start time is wrong\n");
    hr = IDirectMusicBuffer_SetStartTime(dmbuffer, 40);
    ok(hr == S_OK, "IDirectMusicBuffer_GetStartTime returned %#lx\n", hr);
    hr = IDirectMusicBuffer_GetStartTime(dmbuffer, &time);
    ok(hr == S_OK, "IDirectMusicBuffer_GetStartTime returned %#lx\n", hr);
    ok(time == 40, "Buffer start time is wrong\n");

    hr = IDirectMusicBuffer_GetRawBufferPtr(dmbuffer, &data);
    ok(hr == S_OK, "IDirectMusicBuffer_GetRawBufferPtr returned %#lx\n", hr);
    if (hr == S_OK)
    {
        DMUS_EVENTHEADER* header;
        DWORD message;

        /* Check message 1 */
        header = (DMUS_EVENTHEADER*)data;
        data += sizeof(DMUS_EVENTHEADER);
        ok(header->cbEvent == 3, "cbEvent is %lu instead of 3\n", header->cbEvent);
        ok(header->dwChannelGroup == 0, "dwChannelGroup is %lu instead of 0\n", header->dwChannelGroup);
        ok(header->rtDelta == 0, "rtDelta is %s instead of 0\n", wine_dbgstr_longlong(header->rtDelta));
        ok(header->dwFlags == DMUS_EVENT_STRUCTURED, "dwFlags is %lx instead of %x\n", header->dwFlags, DMUS_EVENT_STRUCTURED);
        message = *(DWORD*)data & 0xffffff; /* Only 3 bytes are relevant */
        data += sizeof(DWORD);
        ok(message == 0x000090, "Message is %0lx instead of 0x000090\n", message);

        /* Check message 2 */
        header = (DMUS_EVENTHEADER*)data;
        data += sizeof(DMUS_EVENTHEADER);
        ok(header->cbEvent == 3, "cbEvent is %lu instead of 3\n", header->cbEvent);
        ok(header->dwChannelGroup == 0, "dwChannelGroup is %lu instead of 0\n", header->dwChannelGroup);
        ok(header->rtDelta == 10, "rtDelta is %s instead of 0\n", wine_dbgstr_longlong(header->rtDelta));
        ok(header->dwFlags == DMUS_EVENT_STRUCTURED, "dwFlags is %lx instead of %x\n", header->dwFlags, DMUS_EVENT_STRUCTURED);
        message = *(DWORD*)data & 0xffffff; /* Only 3 bytes are relevant */
        ok(message == 0x000080, "Message 2 is %0lx instead of 0x000080\n", message);
    }

    if (dmbuffer)
        IDirectMusicBuffer_Release(dmbuffer);
    IDirectMusic_Release(dmusic);
}

static void test_COM(void)
{
    IDirectMusic8 *dm8 = (IDirectMusic8*)0xdeadbeef;
    IDirectMusic *dm;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusic, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER, &IID_IUnknown,
            (void**)&dm8);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusic8 create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dm8, "dm8 = %p\n", dm8);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicObject,
            (void**)&dm8);
    ok(hr == E_NOINTERFACE, "DirectMusic8 create failed: %#lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount for DirectMusic and DirectMusic8 */
    hr = CoCreateInstance(&CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusic8,
            (void**)&dm8);
    if (hr == E_NOINTERFACE)
    {
        win_skip("DirectMusic too old (no IDirectMusic8)\n");
        return;
    }
    ok(hr == S_OK, "DirectMusic8 create failed: %#lx, expected S_OK\n", hr);
    refcount = IDirectMusic8_AddRef(dm8);
    ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);

    hr = IDirectMusic8_QueryInterface(dm8, &IID_IDirectMusic, (void**)&dm);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusic failed: %#lx\n", hr);
    refcount = IDirectMusic_AddRef(dm);
    ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
    IDirectMusic_Release(dm);

    hr = IDirectMusic8_QueryInterface(dm8, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %#lx\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 5, "refcount == %lu, expected 5\n", refcount);
    refcount = IUnknown_Release(unk);

    ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
    while (IDirectMusic8_Release(dm8));
}

static void test_COM_dmcoll(void)
{
    IDirectMusicCollection *dmc = (IDirectMusicCollection*)0xdeadbeef;
    IDirectMusicObject *dmo;
    IPersistStream *ps;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicCollection, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmc);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicCollection create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmc, "dmc = %p\n", dmc);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicCollection, NULL, CLSCTX_INPROC_SERVER,
            &IID_IClassFactory, (void**)&dmc);
    ok(hr == E_NOINTERFACE, "DirectMusicCollection create failed: %#lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicCollection interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicCollection, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicCollection, (void**)&dmc);
    ok(hr == S_OK, "DirectMusicCollection create failed: %#lx, expected S_OK\n", hr);
    refcount = IDirectMusicCollection_AddRef(dmc);
    ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);

    hr = IDirectMusicCollection_QueryInterface(dmc, &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicObject failed: %#lx\n", hr);
    refcount = IDirectMusicObject_AddRef(dmo);
    ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
    refcount = IDirectMusicObject_Release(dmo);

    hr = IDirectMusicCollection_QueryInterface(dmc, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 5, "refcount == %lu, expected 5\n", refcount);
    refcount = IPersistStream_Release(ps);

    hr = IDirectMusicCollection_QueryInterface(dmc, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %#lx\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 6, "refcount == %lu, expected 6\n", refcount);
    refcount = IUnknown_Release(unk);

    ok(refcount == 5, "refcount == %lu, expected 5\n", refcount);
    while (IDirectMusicCollection_Release(dmc));
}

static void test_dmcoll(void)
{
    IDirectMusicCollection *dmc;
    IDirectMusicObject *dmo;
    IPersistStream *ps;
    DMUS_OBJECTDESC desc;
    CLSID class;
    ULARGE_INTEGER size;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicCollection, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicCollection, (void**)&dmc);
    ok(hr == S_OK, "DirectMusicCollection create failed: %#lx, expected S_OK\n", hr);

    /* IDirectMusicObject */
    hr = IDirectMusicCollection_QueryInterface(dmc, &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicObject failed: %#lx\n", hr);
    hr = IDirectMusicObject_GetDescriptor(dmo, NULL);
    ok(hr == E_POINTER, "IDirectMusicObject_GetDescriptor: expected E_POINTER, got  %#lx\n", hr);
    hr = IDirectMusicObject_SetDescriptor(dmo, NULL);
    ok(hr == E_POINTER, "IDirectMusicObject_SetDescriptor: expected E_POINTER, got  %#lx\n", hr);
    ZeroMemory(&desc, sizeof(desc));
    hr = IDirectMusicObject_GetDescriptor(dmo, &desc);
    ok(hr == S_OK, "IDirectMusicObject_GetDescriptor failed: %#lx\n", hr);
    ok(desc.dwValidData == DMUS_OBJ_CLASS,
            "Fresh object has more valid data (%#lx) than DMUS_OBJ_CLASS\n", desc.dwValidData);
    /* DMUS_OBJ_CLASS is immutable */
    desc.dwValidData = DMUS_OBJ_CLASS;
    hr = IDirectMusicObject_SetDescriptor(dmo, &desc);
    ok(hr == S_FALSE , "IDirectMusicObject_SetDescriptor failed: %#lx\n", hr);
    ok(!desc.dwValidData, "dwValidData wasn't cleared:  %#lx\n", desc.dwValidData);
    desc.dwValidData = DMUS_OBJ_CLASS;
    desc.guidClass = CLSID_DirectMusicSegment;
    hr = IDirectMusicObject_SetDescriptor(dmo, &desc);
    ok(hr == S_FALSE && !desc.dwValidData, "IDirectMusicObject_SetDescriptor failed: %#lx\n", hr);
    hr = IDirectMusicObject_GetDescriptor(dmo, &desc);
    ok(hr == S_OK, "IDirectMusicObject_GetDescriptor failed: %#lx\n", hr);
    ok(IsEqualGUID(&desc.guidClass, &CLSID_DirectMusicCollection),
            "guidClass changed, should be CLSID_DirectMusicCollection\n");

    /* Unimplemented IPersistStream methods*/
    hr = IDirectMusicCollection_QueryInterface(dmc, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    ok(hr == E_NOTIMPL, "IPersistStream_GetClassID failed: %#lx\n", hr);
    hr = IPersistStream_IsDirty(ps);
    ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %#lx\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %#lx\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %#lx\n", hr);

    while (IDirectMusicCollection_Release(dmc));
}

static BOOL missing_dmusic(void)
{
    IDirectMusic8 *dm;
    HRESULT hr = CoCreateInstance(&CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusic,
            (void**)&dm);

    if (hr == S_OK && dm)
    {
        IDirectMusic_Release(dm);
        return FALSE;
    }
    return TRUE;
}

static IDirectMusicPort *create_synth_port(IDirectMusic **dmusic)
{
    IDirectMusicPort *port = NULL;
    DMUS_PORTPARAMS params;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusic, (void **)dmusic);
    ok(hr == S_OK, "Cannot create DirectMusic object: %#lx\n", hr);

    params.dwSize = sizeof(params);
    params.dwValidParams = DMUS_PORTPARAMS_CHANNELGROUPS | DMUS_PORTPARAMS_AUDIOCHANNELS;
    params.dwChannelGroups = 1;
    params.dwAudioChannels = 2;
    hr = IDirectMusic_SetDirectSound(*dmusic, NULL, NULL);
    ok(hr == S_OK, "IDirectMusic_SetDirectSound failed: %#lx\n", hr);
    hr = IDirectMusic_CreatePort(*dmusic, &GUID_NULL, &params, &port, NULL);
    ok(hr == S_OK, "IDirectMusic_CreatePort failed: %#lx\n", hr);

    return port;
}

static void test_COM_synthport(void)
{
    IDirectMusic *dmusic;
    IDirectMusicPort *port;
    IDirectMusicPortDownload *dmpd;
    IDirectMusicThru *dmt;
    IKsControl *iksc;
    IReferenceClock *clock;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    port = create_synth_port(&dmusic);

    /* Same refcount for all DirectMusicPort interfaces */
    refcount = IDirectMusicPort_AddRef(port);
    ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);

    hr = IDirectMusicPort_QueryInterface(port, &IID_IDirectMusicPortDownload, (void**)&dmpd);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicPortDownload failed: %#lx\n", hr);
    refcount = IDirectMusicPortDownload_AddRef(dmpd);
    ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
    IDirectMusicPortDownload_Release(dmpd);

    hr = IDirectMusicPort_QueryInterface(port, &IID_IKsControl, (void**)&iksc);
    ok(hr == S_OK, "QueryInterface for IID_IKsControl failed: %#lx\n", hr);
    refcount = IKsControl_AddRef(iksc);
    ok(refcount == 5, "refcount == %lu, expected 5\n", refcount);
    IKsControl_Release(iksc);

    hr = IDirectMusicPort_QueryInterface(port, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %#lx\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 6, "refcount == %lu, expected 6\n", refcount);
    IUnknown_Release(unk);

    /* Unsupported interface */
    hr = IDirectMusicPort_QueryInterface(port, &IID_IDirectMusicThru, (void**)&dmt);
    todo_wine ok(hr == E_NOINTERFACE, "QueryInterface for IID_IDirectMusicThru failed: %#lx\n", hr);
    hr = IDirectMusicPort_QueryInterface(port, &IID_IReferenceClock, (void**)&clock);
    ok(hr == E_NOINTERFACE, "QueryInterface for IID_IReferenceClock failed: %#lx\n", hr);

    while (IDirectMusicPort_Release(port));
    refcount = IDirectMusic_Release(dmusic);
    ok(!refcount, "Got outstanding refcount %ld.\n", refcount);
}

struct chunk {
    FOURCC id;
    DWORD size;
    FOURCC type;
};

#define CHUNK_HDR_SIZE (sizeof(FOURCC) + sizeof(DWORD))

/* Generate a RIFF file format stream from an array of FOURCC ids.
   RIFF and LIST need to be followed by the form type respectively list type,
   followed by the chunks of the list and terminated with 0. */
static IStream *gen_riff_stream(const FOURCC *ids)
{
    static const LARGE_INTEGER zero;
    int level = -1;
    DWORD *sizes[4];    /* Stack for the sizes of RIFF and LIST chunks */
    char riff[1024];
    char *p = riff;
    struct chunk *ck;
    IStream *stream;

    do {
        ck = (struct chunk *)p;
        ck->id = *ids++;
        switch (ck->id) {
            case 0:
                *sizes[level] = p - (char *)sizes[level] - sizeof(DWORD);
                level--;
                break;
            case FOURCC_LIST:
            case FOURCC_RIFF:
                level++;
                sizes[level] = &ck->size;
                ck->type = *ids++;
                p += sizeof(*ck);
                break;
            case DMUS_FOURCC_GUID_CHUNK:
                ck->size = sizeof(GUID_NULL);
                p += CHUNK_HDR_SIZE;
                memcpy(p, &GUID_NULL, sizeof(GUID_NULL));
                p += ck->size;
                break;
            case DMUS_FOURCC_VERSION_CHUNK:
            {
                DMUS_VERSION ver = {5, 8};

                ck->size = sizeof(ver);
                p += CHUNK_HDR_SIZE;
                memcpy(p, &ver, sizeof(ver));
                p += ck->size;
                break;
            }
            case mmioFOURCC('I','N','A','M'):
                ck->size = 5;
                p += CHUNK_HDR_SIZE;
                strcpy(p, "INAM");
                p += ck->size + 1; /* WORD aligned */
                break;
            default:
            {
                /* Just convert the FOURCC id to a WCHAR string */
                WCHAR *s;

                ck->size = 5 * sizeof(WCHAR);
                p += CHUNK_HDR_SIZE;
                s = (WCHAR *)p;
                s[0] = (char)(ck->id);
                s[1] = (char)(ck->id >> 8);
                s[2] = (char)(ck->id >> 16);
                s[3] = (char)(ck->id >> 24);
                s[4] = 0;
                p += ck->size;
            }
        }
    } while (level >= 0);

    ck = (struct chunk *)riff;
    CreateStreamOnHGlobal(NULL, TRUE, &stream);
    IStream_Write(stream, riff, ck->size + CHUNK_HDR_SIZE, NULL);
    IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);

    return stream;
}

static void test_parsedescriptor(void)
{
    IDirectMusicObject *dmo;
    IStream *stream;
    DMUS_OBJECTDESC desc = {0};
    HRESULT hr;
    const FOURCC alldesc[] =
    {
        FOURCC_RIFF, FOURCC_DLS, DMUS_FOURCC_CATEGORY_CHUNK, FOURCC_LIST,
        DMUS_FOURCC_UNFO_LIST, DMUS_FOURCC_UNAM_CHUNK, DMUS_FOURCC_UCOP_CHUNK,
        DMUS_FOURCC_UCMT_CHUNK, DMUS_FOURCC_USBJ_CHUNK, 0, DMUS_FOURCC_VERSION_CHUNK,
        DMUS_FOURCC_GUID_CHUNK, 0
    };
    const FOURCC dupes[] =
    {
        FOURCC_RIFF, FOURCC_DLS, DMUS_FOURCC_CATEGORY_CHUNK, DMUS_FOURCC_CATEGORY_CHUNK,
        DMUS_FOURCC_VERSION_CHUNK, DMUS_FOURCC_VERSION_CHUNK, DMUS_FOURCC_GUID_CHUNK,
        DMUS_FOURCC_GUID_CHUNK, FOURCC_LIST, DMUS_FOURCC_INFO_LIST, mmioFOURCC('I','N','A','M'), 0,
        FOURCC_LIST, DMUS_FOURCC_INFO_LIST, mmioFOURCC('I','N','A','M'), 0, 0
    };
    FOURCC empty[] = {FOURCC_RIFF, FOURCC_DLS, 0};
    FOURCC inam[] =
    {
        FOURCC_RIFF, FOURCC_DLS, FOURCC_LIST, DMUS_FOURCC_UNFO_LIST,
        mmioFOURCC('I','N','A','M'), 0, 0
    };

    hr = CoCreateInstance(&CLSID_DirectMusicCollection, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void **)&dmo);
    ok(hr == S_OK, "DirectMusicCollection create failed: %#lx, expected S_OK\n", hr);

    /* Nothing loaded */
    hr = IDirectMusicObject_GetDescriptor(dmo, &desc);
    ok(hr == S_OK, "GetDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == DMUS_OBJ_CLASS, "Got valid data %#lx, expected DMUS_OBJ_OBJECT\n",
            desc.dwValidData);
    ok(IsEqualGUID(&desc.guidClass, &CLSID_DirectMusicCollection),
            "Got class guid %s, expected CLSID_DirectMusicCollection\n",
            wine_dbgstr_guid(&desc.guidClass));

    /* Empty RIFF stream */
    stream = gen_riff_stream(empty);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == DMUS_OBJ_CLASS, "Got valid data %#lx, expected DMUS_OBJ_CLASS\n",
            desc.dwValidData);
    ok(IsEqualGUID(&desc.guidClass, &CLSID_DirectMusicCollection),
            "Got class guid %s, expected CLSID_DirectMusicCollection\n",
            wine_dbgstr_guid(&desc.guidClass));
    IStream_Release(stream);

    /* NULL pointers */
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, NULL, &desc);
    ok(hr == E_POINTER, "ParseDescriptor failed: %#lx, expected E_POINTER\n", hr);
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, NULL);
    ok(hr == E_POINTER, "ParseDescriptor failed: %#lx, expected E_POINTER\n", hr);

    /* Wrong form */
    empty[1] = DMUS_FOURCC_CONTAINER_FORM;
    stream = gen_riff_stream(empty);
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == DMUS_E_NOTADLSCOL, "ParseDescriptor failed: %#lx, expected DMUS_E_NOTADLSCOL\n", hr);
    IStream_Release(stream);

    /* All desc chunks */
    stream = gen_riff_stream(alldesc);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == (DMUS_OBJ_CLASS | DMUS_OBJ_VERSION),
            "Got valid data %#lx, expected DMUS_OBJ_CLASS | DMUS_OBJ_VERSION\n", desc.dwValidData);
    ok(IsEqualGUID(&desc.guidClass, &CLSID_DirectMusicCollection),
            "Got class guid %s, expected CLSID_DirectMusicCollection\n",
            wine_dbgstr_guid(&desc.guidClass));
    ok(IsEqualGUID(&desc.guidObject, &GUID_NULL), "Got object guid %s, expected GUID_NULL\n",
            wine_dbgstr_guid(&desc.guidClass));
    ok(desc.vVersion.dwVersionMS == 5 && desc.vVersion.dwVersionLS == 8,
            "Got version %lu.%lu, expected 5.8\n", desc.vVersion.dwVersionMS,
            desc.vVersion.dwVersionLS);
    IStream_Release(stream);

    /* UNFO list with INAM */
    inam[3] = DMUS_FOURCC_UNFO_LIST;
    stream = gen_riff_stream(inam);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == DMUS_OBJ_CLASS, "Got valid data %#lx, expected DMUS_OBJ_CLASS\n",
            desc.dwValidData);
    IStream_Release(stream);

    /* INFO list with INAM */
    inam[3] = DMUS_FOURCC_INFO_LIST;
    stream = gen_riff_stream(inam);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == (DMUS_OBJ_CLASS | DMUS_OBJ_NAME),
            "Got valid data %#lx, expected DMUS_OBJ_CLASS | DMUS_OBJ_NAME\n", desc.dwValidData);
    ok(!lstrcmpW(desc.wszName, L"INAM"), "Got name '%s', expected 'INAM'\n",
            wine_dbgstr_w(desc.wszName));
    IStream_Release(stream);

    /* Duplicated chunks */
    stream = gen_riff_stream(dupes);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == (DMUS_OBJ_CLASS | DMUS_OBJ_NAME | DMUS_OBJ_VERSION),
            "Got valid data %#lx, expected DMUS_OBJ_CLASS | DMUS_OBJ_NAME | DMUS_OBJ_VERSION\n",
            desc.dwValidData);
    ok(!lstrcmpW(desc.wszName, L"INAM"), "Got name '%s', expected 'INAM'\n",
            wine_dbgstr_w(desc.wszName));
    IStream_Release(stream);

    IDirectMusicObject_Release(dmo);
}

static void test_master_clock(void)
{
    static const GUID guid_system_clock = {0x58d58419, 0x71b4, 0x11d1, {0xa7, 0x4c, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12}};
    static const GUID guid_dsound_clock = {0x58d58420, 0x71b4, 0x11d1, {0xa7, 0x4c, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12}};
    IReferenceClock *clock, *clock2;
    REFERENCE_TIME time1, time2;
    LARGE_INTEGER counter, freq;
    DMUS_CLOCKINFO clock_info;
    IDirectMusic *dmusic;
    DWORD_PTR cookie;
    HRESULT hr;
    ULONG ref;
    GUID guid;

    hr = CoCreateInstance(&CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusic, (void **)&dmusic);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IDirectMusic_GetMasterClock(dmusic, NULL, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    memset(&guid, 0xcc, sizeof(guid));
    hr = IDirectMusic_GetMasterClock(dmusic, &guid, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(IsEqualGUID(&guid, &guid_system_clock), "Got guid %s.\n", wine_dbgstr_guid(&guid));

    clock = (IReferenceClock *)0xdeadbeef;
    hr = IDirectMusic_GetMasterClock(dmusic, NULL, &clock);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(clock && clock != (IReferenceClock *)0xdeadbeef, "Got clock %p.\n", clock);

    hr = IDirectMusic_GetMasterClock(dmusic, NULL, &clock2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(clock2 == clock, "Clocks didn't match.\n");
    IReferenceClock_Release(clock2);

    memset(&guid, 0xcc, sizeof(guid));
    hr = IDirectMusic_GetMasterClock(dmusic, &guid, &clock2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(IsEqualGUID(&guid, &guid_system_clock), "Got guid %s.\n", wine_dbgstr_guid(&guid));
    ok(clock2 == clock, "Clocks didn't match.\n");
    IReferenceClock_Release(clock2);

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    hr = IReferenceClock_GetTime(clock, &time1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    time2 = counter.QuadPart * 10000000.0 / freq.QuadPart;
    ok(compare_time(time1,  time2, 20 * 10000), "Expected about %s, got %s.\n",
            wine_dbgstr_longlong(time2), wine_dbgstr_longlong(time1));

    hr = IReferenceClock_GetTime(clock, &time2);
    ok(hr == (time2 == time1 ? S_FALSE : S_OK), "Got hr %#lx.\n", hr);

    Sleep(100);
    hr = IReferenceClock_GetTime(clock, &time2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time2 - time1 > 80 * 10000, "Expected about %s, but got %s.\n",
            wine_dbgstr_longlong(time1 + 100 * 10000), wine_dbgstr_longlong(time2));

    hr = IReferenceClock_AdviseTime(clock, 0, 0, 0, &cookie);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IReferenceClock_AdvisePeriodic(clock, 0, 0, 0, &cookie);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IReferenceClock_Unadvise(clock, 0);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    IReferenceClock_Release(clock);

    hr = IDirectMusic_EnumMasterClock(dmusic, 0, NULL);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    memset(&clock_info, 0xcc, sizeof(DMUS_CLOCKINFO));
    clock_info.dwSize = sizeof(DMUS_CLOCKINFO7);
    hr = IDirectMusic_EnumMasterClock(dmusic, 0, &clock_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(clock_info.ctType == DMUS_CLOCK_SYSTEM, "Got type %#x.\n", clock_info.ctType);
    ok(IsEqualGUID(&clock_info.guidClock, &guid_system_clock), "Got guid %s.\n",
            wine_dbgstr_guid(&clock_info.guidClock));
    ok(clock_info.dwFlags == 0xcccccccc, "Got flags %#lx.\n", clock_info.dwFlags);

    memset(&clock_info, 0xcc, sizeof(DMUS_CLOCKINFO));
    clock_info.dwSize = sizeof(DMUS_CLOCKINFO7);
    hr = IDirectMusic_EnumMasterClock(dmusic, 1, &clock_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(clock_info.ctType == DMUS_CLOCK_SYSTEM, "Got type %#x.\n", clock_info.ctType);
    ok(IsEqualGUID(&clock_info.guidClock, &guid_dsound_clock), "Got guid %s.\n",
            wine_dbgstr_guid(&clock_info.guidClock));
    ok(clock_info.dwFlags == 0xcccccccc, "Got flags %#lx.\n", clock_info.dwFlags);

    memset(&clock_info, 0xcc, sizeof(DMUS_CLOCKINFO));
    clock_info.dwSize = sizeof(DMUS_CLOCKINFO7);
    hr = IDirectMusic_EnumMasterClock(dmusic, 2, &clock_info);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    ref = IDirectMusic_Release(dmusic);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_synthport(void)
{
    IDirectMusic *dmusic;
    IDirectMusicBuffer *buf;
    IDirectMusicPort *port;
    DMUS_BUFFERDESC desc;
    DMUS_PORTCAPS caps;
    WAVEFORMATEX fmt;
    DWORD fmtsize, bufsize;

    HRESULT hr;

    port = create_synth_port(&dmusic);

    /* Create a IDirectMusicPortBuffer */
    desc.dwSize = sizeof(DMUS_BUFFERDESC);
    desc.dwFlags = 0;
    desc.cbBuffer = 1023;
    memcpy(&desc.guidBufferFormat, &GUID_NULL, sizeof(GUID));
    hr = IDirectMusic_CreateMusicBuffer(dmusic, &desc, &buf, NULL);
    ok(hr == S_OK, "IDirectMusic_CreateMusicBuffer failed: %#lx\n", hr);

    /* Unsupported methods */
    hr = IDirectMusicPort_Read(port, NULL);
    todo_wine ok(hr == E_POINTER, "Read returned: %#lx\n", hr);
    hr = IDirectMusicPort_Read(port, buf);
    ok(hr == E_NOTIMPL, "Read returned: %#lx\n", hr);
    hr = IDirectMusicPort_SetReadNotificationHandle(port, NULL);
    ok(hr == E_NOTIMPL, "SetReadNotificationHandle returned: %#lx\n", hr);
    hr = IDirectMusicPort_Compact(port);
    ok(hr == E_NOTIMPL, "Compact returned: %#lx\n", hr);

    /* GetCaps */
    hr = IDirectMusicPort_GetCaps(port, NULL);
    ok(hr == E_INVALIDARG, "GetCaps failed: %#lx\n", hr);
    memset(&caps, 0, sizeof(caps));
    hr = IDirectMusicPort_GetCaps(port, &caps);
    ok(hr == E_INVALIDARG, "GetCaps failed: %#lx\n", hr);
    caps.dwSize = sizeof(caps);
    hr = IDirectMusicPort_GetCaps(port, &caps);
    ok(hr == S_OK, "GetCaps failed: %#lx\n", hr);
    ok(caps.dwSize == sizeof(caps), "dwSize was modified to %ld\n", caps.dwSize);
    ok(IsEqualGUID(&caps.guidPort, &CLSID_DirectMusicSynth), "Expected port guid CLSID_DirectMusicSynth, got %s\n",
            wine_dbgstr_guid(&caps.guidPort));
    ok(caps.dwClass == DMUS_PC_OUTPUTCLASS, "Got wrong dwClass: %#lx\n", caps.dwClass);
    ok(caps.dwType == DMUS_PORT_USER_MODE_SYNTH, "Got wrong dwType: %#lx\n", caps.dwType);
    ok(caps.dwFlags == (DMUS_PC_AUDIOPATH|DMUS_PC_DIRECTSOUND|DMUS_PC_DLS|DMUS_PC_DLS2|DMUS_PC_SOFTWARESYNTH|
                DMUS_PC_WAVE), "Unexpected dwFlags returned: %#lx\n", caps.dwFlags);
    ok(caps.dwMemorySize == DMUS_PC_SYSTEMMEMORY, "Got dwMemorySize: %#lx\n", caps.dwMemorySize);
    ok(caps.dwMaxChannelGroups == 1000, "Got dwMaxChannelGroups: %ld\n", caps.dwMaxChannelGroups);
    ok(caps.dwMaxVoices == 1000, "Got dwMaxVoices: %ld\n", caps.dwMaxVoices);
    ok(caps.dwMaxAudioChannels == 2, "Got dwMaxAudioChannels: %#lx\n", caps.dwMaxAudioChannels);
    ok(caps.dwEffectFlags == DMUS_EFFECT_REVERB, "Unexpected dwEffectFlags returned: %#lx\n", caps.dwEffectFlags);
    trace("Port wszDescription: %s\n", wine_dbgstr_w(caps.wszDescription));

    /* GetFormat */
    hr = IDirectMusicPort_GetFormat(port, NULL, NULL, NULL);
    ok(hr == E_POINTER, "GetFormat failed: %#lx\n", hr);
    hr = IDirectMusicPort_GetFormat(port, NULL, &fmtsize, NULL);
    ok(hr == S_OK, "GetFormat failed: %#lx\n", hr);
    ok(fmtsize == sizeof(fmt), "format size; %ld\n", fmtsize);
    fmtsize = 0;
    hr = IDirectMusicPort_GetFormat(port, &fmt, &fmtsize, NULL);
    ok(hr == S_OK, "GetFormat failed: %#lx\n", hr);
    ok(fmtsize == sizeof(fmt), "format size; %ld\n", fmtsize);
    hr = IDirectMusicPort_GetFormat(port, NULL, NULL, &bufsize);
    ok(hr == E_POINTER, "GetFormat failed: %#lx\n", hr);
    hr = IDirectMusicPort_GetFormat(port, NULL, &fmtsize, &bufsize);
    ok(hr == S_OK, "GetFormat failed: %#lx\n", hr);
    hr = IDirectMusicPort_GetFormat(port, &fmt, &fmtsize, &bufsize);
    ok(hr == S_OK, "GetFormat failed: %#lx\n", hr);
    ok(bufsize == fmt.nSamplesPerSec * fmt.nChannels * 4, "buffer size: %ld\n", bufsize);

    IDirectMusicPort_Release(port);
    IDirectMusic_Release(dmusic);
}

static void test_port_kscontrol(void)
{
    IDirectMusicPort *port;
    IDirectMusic *dmusic;
    IKsControl *control;
    KSPROPERTY property;
    DWORD volume_size;
    LONG volume;
    HRESULT hr;

    port = create_synth_port(&dmusic);
    hr = IDirectMusicPort_QueryInterface(port, &IID_IKsControl, (void **)&control);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicPort_Release(port);

    property.Set = GUID_DMUS_PROP_Volume;
    property.Id = 0;
    property.Flags = KSPROPERTY_TYPE_GET;
    hr = IKsControl_KsProperty(control, &property, sizeof(property), &volume, sizeof(volume), &volume_size);
    ok(hr == DMUS_E_GET_UNSUPPORTED, "got hr %#lx.\n", hr);

    property.Set = GUID_DMUS_PROP_Volume;
    property.Id = 1;
    property.Flags = KSPROPERTY_TYPE_GET;
    hr = IKsControl_KsProperty(control, &property, sizeof(property), &volume, sizeof(volume), &volume_size);
    ok(hr == DMUS_E_UNKNOWN_PROPERTY, "got hr %#lx.\n", hr);

    volume = 0;
    property.Set = GUID_DMUS_PROP_Volume;
    property.Id = 0;
    property.Flags = KSPROPERTY_TYPE_SET;
    hr = IKsControl_KsProperty(control, &property, sizeof(property), &volume, sizeof(volume), &volume_size);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    volume = 0;
    property.Set = GUID_DMUS_PROP_Volume;
    property.Id = 1;
    property.Flags = KSPROPERTY_TYPE_SET;
    hr = IKsControl_KsProperty(control, &property, sizeof(property), &volume, sizeof(volume), &volume_size);
    ok(hr == DMUS_E_UNKNOWN_PROPERTY, "got hr %#lx.\n", hr);

    IDirectMusicPort_Release(port);
    IDirectMusic_Release(dmusic);
}

static void test_port_download(void)
{
    struct wave_download
    {
        DMUS_DOWNLOADINFO info;
        ULONG offsets[2];
        DMUS_WAVE wave;
        DMUS_WAVEDATA wave_data;
    };

    static void *invalid_ptr = (void *)0xdeadbeef;
    IDirectMusicDownload *download, *tmp_download;
    struct wave_download *wave_download;
    IDirectMusicPortDownload *port;
    IDirectMusicPort *tmp_port;
    DWORD ids[4], append, size;
    IDirectMusic *dmusic;
    void *buffer;
    HRESULT hr;

    tmp_port = create_synth_port(&dmusic);
    hr = IDirectMusicPort_QueryInterface(tmp_port, &IID_IDirectMusicPortDownload, (void **)&port);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicPort_Release(tmp_port);

    /* GetBuffer only works with pre-allocated DLId */
    hr = IDirectMusicPortDownload_GetBuffer(port, 0, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicPortDownload_GetBuffer(port, 0, &download);
    ok(hr == DMUS_E_INVALID_DOWNLOADID, "got %#lx\n", hr);
    hr = IDirectMusicPortDownload_GetBuffer(port, 0xdeadbeef, &download);
    ok(hr == DMUS_E_INVALID_DOWNLOADID, "got %#lx\n", hr);

    /* AllocateBuffer use the exact requested size */
    hr = IDirectMusicPortDownload_AllocateBuffer(port, 0, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicPortDownload_AllocateBuffer(port, 0, &download);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    hr = IDirectMusicPortDownload_AllocateBuffer(port, 1, &download);
    ok(hr == S_OK, "got %#lx\n", hr);
    size = 0xdeadbeef;
    buffer = invalid_ptr;
    hr = IDirectMusicDownload_GetBuffer(download, &buffer, &size);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(size == 1, "got %#lx\n", size);
    ok(buffer != invalid_ptr, "got %p\n", buffer);
    IDirectMusicDownload_Release(download);

    /* GetDLId allocates the given number of slots and returns only the first */
    hr = IDirectMusicPortDownload_GetDLId(port, NULL, 0);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicPortDownload_GetDLId(port, ids, 0);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    memset(ids, 0xcc, sizeof(ids));
    hr = IDirectMusicPortDownload_GetDLId(port, ids, 4);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(ids[0] == 0, "got %#lx\n", ids[0]);
    ok(ids[1] == 0xcccccccc, "got %#lx\n", ids[1]);

    /* GetBuffer looks up allocated ids to find downloaded buffers */
    hr = IDirectMusicPortDownload_GetBuffer(port, 2, &download);
    ok(hr == DMUS_E_NOT_DOWNLOADED_TO_PORT, "got %#lx\n", hr);

    hr = IDirectMusicPortDownload_GetAppend(port, NULL);
    todo_wine ok(hr == E_POINTER, "got %#lx\n", hr);
    append = 0xdeadbeef;
    hr = IDirectMusicPortDownload_GetAppend(port, &append);
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine ok(append == 2, "got %#lx\n", append);

    /* test Download / Unload on invalid and valid buffers */

    download = invalid_ptr;
    hr = IDirectMusicPortDownload_AllocateBuffer(port, sizeof(struct wave_download), &download);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(download != invalid_ptr, "got %p\n", download);
    size = 0xdeadbeef;
    wave_download = invalid_ptr;
    hr = IDirectMusicDownload_GetBuffer(download, (void **)&wave_download, &size);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(size == sizeof(struct wave_download), "got %#lx\n", size);
    ok(wave_download != invalid_ptr, "got %p\n", wave_download);
    wave_download->info.cbSize = sizeof(struct wave_download);
    wave_download->info.dwDLId = 2;
    wave_download->info.dwDLType = 0;
    wave_download->info.dwNumOffsetTableEntries = 0;
    hr = IDirectMusicPortDownload_GetBuffer(port, 2, &tmp_download);
    ok(hr == DMUS_E_NOT_DOWNLOADED_TO_PORT, "got %#lx\n", hr);

    hr = IDirectMusicPortDownload_Download(port, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicPortDownload_Download(port, download);
    todo_wine ok(hr == DMUS_E_UNKNOWNDOWNLOAD, "got %#lx\n", hr);

    wave_download->info.dwDLType = DMUS_DOWNLOADINFO_WAVE;
    wave_download->info.dwNumOffsetTableEntries = 2;
    wave_download->offsets[0] = offsetof(struct wave_download, wave);
    wave_download->offsets[1] = offsetof(struct wave_download, wave_data);
    wave_download->wave.WaveformatEx.wFormatTag = WAVE_FORMAT_PCM;
    wave_download->wave.WaveformatEx.nChannels = 1;
    wave_download->wave.WaveformatEx.nSamplesPerSec = 44100;
    wave_download->wave.WaveformatEx.nAvgBytesPerSec = 44100;
    wave_download->wave.WaveformatEx.nBlockAlign = 1;
    wave_download->wave.WaveformatEx.wBitsPerSample = 8;
    wave_download->wave.WaveformatEx.cbSize = 0;
    wave_download->wave.ulWaveDataIdx = 1;
    wave_download->wave.ulCopyrightIdx = 0;
    wave_download->wave.ulFirstExtCkIdx = 0;
    wave_download->wave_data.cbSize = 1;

    hr = IDirectMusicPortDownload_Download(port, download);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPortDownload_Download(port, download);
    ok(hr == DMUS_E_ALREADY_DOWNLOADED, "got %#lx\n", hr);

    tmp_download = invalid_ptr;
    hr = IDirectMusicPortDownload_GetBuffer(port, 2, &tmp_download);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(tmp_download == download, "got %p\n", tmp_download);
    IDirectMusicDownload_Release(tmp_download);

    hr = IDirectMusicPortDownload_Unload(port, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicPortDownload_Unload(port, download);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPortDownload_GetBuffer(port, 2, &tmp_download);
    ok(hr == DMUS_E_NOT_DOWNLOADED_TO_PORT, "got %#lx\n", hr);

    hr = IDirectMusicPortDownload_Unload(port, download);
    ok(hr == S_OK, "got %#lx\n", hr);

    /* DLIds are never released */
    hr = IDirectMusicPortDownload_GetDLId(port, ids, 1);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(ids[0] == 4, "got %#lx\n", ids[0]);

    IDirectMusicDownload_Release(download);

    IDirectMusicPortDownload_Release(port);
}

static void test_download_instrument(void)
{
    static const LARGE_INTEGER zero = {0};
    IDirectMusicDownloadedInstrument *downloaded;
    IDirectMusicCollection *collection;
    IDirectMusicInstrument *instrument, *tmp_instrument;
    IPersistStream *persist;
    IDirectMusicPort *port;
    IDirectMusic *dmusic;
    WCHAR name[MAX_PATH];
    IStream *stream;
    DWORD patch;
    HRESULT hr;

    port = create_synth_port(&dmusic);

    hr = CoCreateInstance(&CLSID_DirectMusicCollection, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicCollection, (void **)&collection);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicCollection_QueryInterface(collection, &IID_IPersistStream, (void **)&persist);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = CreateStreamOnHGlobal(0, TRUE, &stream);
    ok(hr == S_OK, "got %#lx\n", hr);

    CHUNK_RIFF(stream, "DLS ")
    {
        DLSHEADER colh = {.cInstruments = 1};
        struct
        {
            POOLTABLE head;
            POOLCUE cues[1];
        } ptbl =
        {
            .head = {.cbSize = sizeof(POOLTABLE), .cCues = ARRAY_SIZE(ptbl.cues)},
            .cues = {{.ulOffset = 0}}, /* offsets in wvpl */
        };

        CHUNK_DATA(stream, "colh", colh);
        CHUNK_LIST(stream, "lins")
        {
            CHUNK_LIST(stream, "ins ")
            {
                INSTHEADER insh = {.cRegions = 1, .Locale = {.ulBank = 0x12, .ulInstrument = 0x34}};

                CHUNK_DATA(stream, "insh", insh);
                CHUNK_LIST(stream, "lrgn")
                {
                    CHUNK_LIST(stream, "rgn ")
                    {
                        RGNHEADER rgnh =
                        {
                            .RangeKey = {.usLow = 0, .usHigh = 127},
                            .RangeVelocity = {.usLow = 1, .usHigh = 127},
                        };
                        WAVELINK wlnk = {.ulChannel = 1, .ulTableIndex = 0};
                        WSMPL wsmp = {.cbSize = sizeof(WSMPL)};

                        CHUNK_DATA(stream, "rgnh", rgnh);
                        CHUNK_DATA(stream, "wsmp", wsmp);
                        CHUNK_DATA(stream, "wlnk", wlnk);
                    }
                    CHUNK_END;
                }
                CHUNK_END;

                CHUNK_LIST(stream, "lart")
                {
                    CONNECTIONLIST connections = {.cbSize = sizeof(connections)};
                    CHUNK_DATA(stream, "art1", connections);
                }
                CHUNK_END;
            }
            CHUNK_END;
        }
        CHUNK_END;
        CHUNK_DATA(stream, "ptbl", ptbl);
        CHUNK_LIST(stream, "wvpl")
        {
            CHUNK_LIST(stream, "wave")
            {
                WAVEFORMATEX fmt =
                {
                    .wFormatTag = WAVE_FORMAT_PCM,
                    .nChannels = 1,
                    .wBitsPerSample = 8,
                    .nSamplesPerSec = 22050,
                    .nAvgBytesPerSec = 22050,
                    .nBlockAlign = 1,
                };
                BYTE data[16] = {0};

                /* native returns DMUS_E_INVALIDOFFSET from DownloadInstrument if data is last */
                CHUNK_DATA(stream, "data", data);
                CHUNK_DATA(stream, "fmt ", fmt);
            }
            CHUNK_END;
        }
        CHUNK_END;
    }
    CHUNK_END;

    hr = IStream_Seek(stream, zero, 0, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IPersistStream_Load(persist, stream);
    ok(hr == S_OK, "got %#lx\n", hr);
    IPersistStream_Release(persist);
    IStream_Release(stream);

    patch = 0xdeadbeef;
    wcscpy(name, L"DeadBeef");
    hr = IDirectMusicCollection_EnumInstrument(collection, 0, &patch, name, ARRAY_SIZE(name));
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(patch == 0x1234, "got %#lx\n", patch);
    ok(*name == 0, "got %s\n", debugstr_w(name));
    hr = IDirectMusicCollection_EnumInstrument(collection, 1, &patch, name, ARRAY_SIZE(name));
    ok(hr == S_FALSE, "got %#lx\n", hr);

    hr = IDirectMusicCollection_GetInstrument(collection, 0x1234, &instrument);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicInstrument_GetPatch(instrument, &patch);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(patch == 0x1234, "got %#lx\n", patch);
    hr = IDirectMusicInstrument_SetPatch(instrument, 0x4321);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicInstrument_GetPatch(instrument, &patch);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(patch == 0x4321, "got %#lx\n", patch);

    hr = IDirectMusicCollection_GetInstrument(collection, 0x1234, &tmp_instrument);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(instrument == tmp_instrument, "got %p\n", tmp_instrument);
    hr = IDirectMusicInstrument_GetPatch(tmp_instrument, &patch);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(patch == 0x4321, "got %#lx\n", patch);
    IDirectMusicInstrument_Release(tmp_instrument);

    check_interface(instrument, &IID_IDirectMusicObject, FALSE);
    check_interface(instrument, &IID_IDirectMusicDownload, FALSE);
    check_interface(instrument, &IID_IDirectMusicDownloadedInstrument, FALSE);

    hr = IDirectMusicPort_DownloadInstrument(port, instrument, &downloaded, NULL, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    check_interface(downloaded, &IID_IDirectMusicObject, FALSE);
    check_interface(downloaded, &IID_IDirectMusicDownload, FALSE);
    check_interface(downloaded, &IID_IDirectMusicInstrument, FALSE);

    hr = IDirectMusicPort_UnloadInstrument(port, downloaded);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicDownloadedInstrument_Release(downloaded);

    IDirectMusicInstrument_Release(instrument);

    IDirectMusicCollection_Release(collection);
    IDirectMusicPort_Release(port);
    IDirectMusic_Release(dmusic);
}

struct result
{
    DWORD patch;
    WCHAR name[DMUS_MAX_NAME];
};

static int __cdecl result_cmp(const void *a, const void *b)
{
    const struct result *ra = a, *rb = b;
    if (ra->patch != rb->patch) return ra->patch < rb->patch ? -1 : 1;
    return wcscmp(ra->name, rb->name);
}

static void test_default_gm_collection(void)
{
    DMUS_OBJECTDESC desc =
    {
        .dwSize = sizeof(DMUS_OBJECTDESC),
        .dwValidData = DMUS_OBJ_OBJECT | DMUS_OBJ_CLASS,
        .guidClass = CLSID_DirectMusicCollection,
        .guidObject = GUID_DefaultGMCollection,
    };
    struct result expected[] =
    {
        {         0, L"Piano 1     "},
        {       0x1, L"Piano 2     "},
        {       0x2, L"Piano 3     "},
        {       0x3, L"Honky-tonk  "},
        {       0x4, L"E.Piano 1   "},
        {       0x5, L"E.Piano 2   "},
        {       0x6, L"Harpsichord "},
        {       0x7, L"Clav.       "},
        {       0x8, L"Celesta     "},
        {       0x9, L"Glockenspiel"},
        {       0xa, L"Music Box   "},
        {       0xb, L"Vibraphone  "},
        {       0xc, L"Marimba     "},
        {       0xd, L"Xylophone   "},
        {       0xe, L"Tubular-bell"},
        {       0xf, L"Santur      "},
        {      0x10, L"Organ 1     "},
        {      0x11, L"Organ 2     "},
        {      0x12, L"Organ 3     "},
        {      0x13, L"Church Org.1"},
        {      0x14, L"Reed Organ  "},
        {      0x15, L"Accordion Fr"},
        {      0x16, L"Harmonica   "},
        {      0x17, L"Bandoneon   "},
        {      0x18, L"Nylon-str.Gt"},
        {      0x19, L"Steel-str.Gt"},
        {      0x1a, L"Jazz Gt.    "},
        {      0x1b, L"Clean Gt.   "},
        {      0x1c, L"Muted Gt.   "},
        {      0x1d, L"Overdrive Gt"},
        {      0x1e, L"DistortionGt"},
        {      0x1f, L"Gt.Harmonics"},
        {      0x20, L"Acoustic Bs."},
        {      0x21, L"Fingered Bs."},
        {      0x22, L"Picked Bs.  "},
        {      0x23, L"Fretless Bs."},
        {      0x24, L"Slap Bass 1 "},
        {      0x25, L"Slap Bass 2 "},
        {      0x26, L"Synth Bass 1"},
        {      0x27, L"Synth Bass 2"},
        {      0x28, L"Violin      "},
        {      0x29, L"Viola       "},
        {      0x2a, L"Cello       "},
        {      0x2b, L"Contrabass  "},
        {      0x2c, L"Tremolo Str "},
        {      0x2d, L"PizzicatoStr"},
        {      0x2e, L"Harp        "},
        {      0x2f, L"Timpani     "},
        {      0x30, L"Strings     "},
        {      0x31, L"Slow Strings"},
        {      0x32, L"Syn.Strings1"},
        {      0x33, L"Syn.Strings2"},
        {      0x34, L"Choir Aahs  "},
        {      0x35, L"Voice Oohs  "},
        {      0x36, L"SynVox      "},
        {      0x37, L"OrchestraHit"},
        {      0x38, L"Trumpet     "},
        {      0x39, L"Trombone    "},
        {      0x3a, L"Tuba        "},
        {      0x3b, L"MutedTrumpet"},
        {      0x3c, L"French Horns"},
        {      0x3d, L"Brass 1     "},
        {      0x3e, L"Synth Brass1"},
        {      0x3f, L"Synth Brass2"},
        {      0x40, L"Soprano Sax "},
        {      0x41, L"Alto Sax    "},
        {      0x42, L"Tenor Sax   "},
        {      0x43, L"Baritone Sax"},
        {      0x44, L"Oboe        "},
        {      0x45, L"English Horn"},
        {      0x46, L"Bassoon     "},
        {      0x47, L"Clarinet    "},
        {      0x48, L"Piccolo     "},
        {      0x49, L"Flute       "},
        {      0x4a, L"Recorder    "},
        {      0x4b, L"Pan Flute   "},
        {      0x4c, L"Bottle Blow "},
        {      0x4d, L"Shakuhachi  "},
        {      0x4e, L"Whistle     "},
        {      0x4f, L"Ocarina     "},
        {      0x50, L"Square Wave "},
        {      0x51, L"Saw Wave    "},
        {      0x52, L"Syn.Calliope"},
        {      0x53, L"Chiffer Lead"},
        {      0x54, L"Charang     "},
        {      0x55, L"Solo Vox    "},
        {      0x56, L"5th Saw Wave"},
        {      0x57, L"Bass & Lead "},
        {      0x58, L"Fantasia    "},
        {      0x59, L"Warm Pad    "},
        {      0x5a, L"Polysynth   "},
        {      0x5b, L"Space Voice "},
        {      0x5c, L"Bowed Glass "},
        {      0x5d, L"Metal Pad   "},
        {      0x5e, L"Halo Pad    "},
        {      0x5f, L"Sweep Pad   "},
        {      0x60, L"Ice Rain    "},
        {      0x61, L"Soundtrack  "},
        {      0x62, L"Crystal     "},
        {      0x63, L"Atmosphere  "},
        {      0x64, L"Brightness  "},
        {      0x65, L"Goblin      "},
        {      0x66, L"Echo Drops  "},
        {      0x67, L"Star Theme  "},
        {      0x68, L"Sitar       "},
        {      0x69, L"Banjo       "},
        {      0x6a, L"Shamisen    "},
        {      0x6b, L"Koto        "},
        {      0x6c, L"Kalimba     "},
        {      0x6d, L"Bagpipe     "},
        {      0x6e, L"Fiddle      "},
        {      0x6f, L"Shanai      "},
        {      0x70, L"Tinkle Bell "},
        {      0x71, L"Agogo       "},
        {      0x72, L"Steel Drums "},
        {      0x73, L"Woodblock   "},
        {      0x74, L"Taiko       "},
        {      0x75, L"Melo. Tom 1 "},
        {      0x76, L"Synth Drum  "},
        {      0x77, L"Reverse Cym."},
        {      0x78, L"Gt.FretNoise"},
        {      0x79, L"Breath Noise"},
        {      0x7a, L"Seashore    "},
        {      0x7b, L"Bird        "},
        {      0x7c, L"Telephone 1 "},
        {      0x7d, L"Helicopter  "},
        {      0x7e, L"Applause    "},
        {      0x7f, L"Gun Shot    "},
        {   0x10026, L"SynthBass101"},
        {   0x10039, L"Trombone 2  "},
        {   0x1003c, L"Fr.Horn 2   "},
        {   0x10050, L"Square      "},
        {   0x10051, L"Saw         "},
        {   0x10062, L"Syn Mallet  "},
        {   0x10066, L"Echo Bell   "},
        {   0x10068, L"Sitar 2     "},
        {   0x10078, L"Gt.Cut Noise"},
        {   0x10079, L"Fl.Key Click"},
        {   0x1007a, L"Rain        "},
        {   0x1007b, L"Dog         "},
        {   0x1007c, L"Telephone 2 "},
        {   0x1007d, L"Car-Engine  "},
        {   0x1007e, L"Laughing    "},
        {   0x1007f, L"Machine Gun "},
        {   0x20066, L"Echo Pan    "},
        {   0x20078, L"String Slap "},
        {   0x2007a, L"Thunder     "},
        {   0x2007b, L"Horse-Gallop"},
        {   0x2007c, L"DoorCreaking"},
        {   0x2007d, L"Car-Stop    "},
        {   0x2007e, L"Screaming   "},
        {   0x2007f, L"Lasergun    "},
        {   0x3007a, L"Wind        "},
        {   0x3007b, L"Bird 2      "},
        {   0x3007c, L"Door        "},
        {   0x3007d, L"Car-Pass    "},
        {   0x3007e, L"Punch       "},
        {   0x3007f, L"Explosion   "},
        {   0x4007a, L"Stream      "},
        {   0x4007c, L"Scratch     "},
        {   0x4007d, L"Car-Crash   "},
        {   0x4007e, L"Heart Beat  "},
        {   0x5007a, L"Bubble      "},
        {   0x5007c, L"Wind Chimes "},
        {   0x5007d, L"Siren       "},
        {   0x5007e, L"Footsteps   "},
        {   0x6007d, L"Train       "},
        {   0x7007d, L"Jetplane    "},
        {   0x80000, L"Piano 1     "},
        {   0x80001, L"Piano 2     "},
        {   0x80002, L"Piano 3     "},
        {   0x80003, L"Honky-tonk  "},
        {   0x80004, L"Detuned EP 1"},
        {   0x80005, L"Detuned EP 2"},
        {   0x80006, L"Coupled Hps."},
        {   0x8000b, L"Vibraphone  "},
        {   0x8000c, L"Marimba     "},
        {   0x8000e, L"Church Bell "},
        {   0x80010, L"Detuned Or.1"},
        {   0x80011, L"Detuned Or.2"},
        {   0x80013, L"Church Org.2"},
        {   0x80015, L"Accordion It"},
        {   0x80018, L"Ukulele     "},
        {   0x80019, L"12-str.Gt   "},
        {   0x8001a, L"Hawaiian Gt."},
        {   0x8001b, L"Chorus Gt.  "},
        {   0x8001c, L"Funk Gt.    "},
        {   0x8001e, L"Feedback Gt."},
        {   0x8001f, L"Gt. Feedback"},
        {   0x80026, L"Synth Bass 3"},
        {   0x80027, L"Synth Bass 4"},
        {   0x80028, L"Slow Violin "},
        {   0x80030, L"Orchestra   "},
        {   0x80032, L"Syn.Strings3"},
        {   0x8003d, L"Brass 2     "},
        {   0x8003e, L"Synth Brass3"},
        {   0x8003f, L"Synth Brass4"},
        {   0x80050, L"Sine Wave   "},
        {   0x80051, L"Doctor Solo "},
        {   0x8006b, L"Taisho Koto "},
        {   0x80073, L"Castanets   "},
        {   0x80074, L"Concert BD  "},
        {   0x80075, L"Melo. Tom 2 "},
        {   0x80076, L"808 Tom     "},
        {   0x8007d, L"Starship    "},
        {   0x9000e, L"Carillon    "},
        {   0x90076, L"Elec Perc.  "},
        {   0x9007d, L"Burst Noise "},
        {  0x100000, L"Piano 1d    "},
        {  0x100004, L"E.Piano 1v  "},
        {  0x100005, L"E.Piano 2v  "},
        {  0x100006, L"Harpsichord "},
        {  0x100010, L"60's Organ 1"},
        {  0x100013, L"Church Org.3"},
        {  0x100018, L"Nylon Gt.o  "},
        {  0x100019, L"Mandolin    "},
        {  0x10001c, L"Funk Gt.2   "},
        {  0x100027, L"Rubber Bass "},
        {  0x10003e, L"AnalogBrass1"},
        {  0x10003f, L"AnalogBrass2"},
        {  0x180004, L"60's E.Piano"},
        {  0x180006, L"Harpsi.o    "},
        {  0x200010, L"Organ 4     "},
        {  0x200011, L"Organ 5     "},
        {  0x200018, L"Nylon Gt.2  "},
        {  0x200034, L"Choir Aahs 2"},
        {0x80000000, L"Standard    "},
        {0x80000008, L"Room        "},
        {0x80000010, L"Power       "},
        {0x80000018, L"Electronic  "},
        {0x80000019, L"TR-808      "},
        {0x80000020, L"Jazz        "},
        {0x80000028, L"Brush       "},
        {0x80000030, L"Orchestra   "},
        {0x80000038, L"SFX         "},
    }, results[ARRAY_SIZE(expected) + 1];
    IDirectMusicCollection *collection;
    IDirectMusicLoader *loader;
    HRESULT hr;
    DWORD i;

    hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicLoader, (void**)&loader);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicLoader_GetObject(loader, &desc, &IID_IDirectMusicCollection, (void **)&collection);
    if (hr == DMUS_E_LOADER_NOFILENAME)
    {
        skip("Failed to open default GM collection, skipping tests. Missing system SoundFont?\n");
        goto skip_tests;
    }
    ok(hr == S_OK, "got %#lx\n", hr);

    for (i = 0; hr == S_OK && i < ARRAY_SIZE(results); i++)
    {
        results[i].patch = 0xdeadbeef;
        wcscpy(results[i].name, L"DeadBeef");
        hr = IDirectMusicCollection_EnumInstrument(collection, i, &results[i].patch,
                results[i].name, ARRAY_SIZE(results[i].name));
    }
    if (hr == S_FALSE) i--;
    ok(hr == S_FALSE, "got %#lx\n", hr);
    ok(i > 0, "got %lu\n", i);
    todo_wine ok(i == ARRAY_SIZE(expected), "got %lu\n", i);

    qsort(results, i, sizeof(*results), result_cmp);

    while (i--)
    {
        winetest_push_context("%lu", i);
        trace("got %#lx %s\n", results[i].patch, debugstr_w(results[i].name));
        todo_wine_if(expected[i].patch >= 128)
        ok(results[i].patch == expected[i].patch, "got %#lx\n", results[i].patch);
        /* system soundfont names are not very predictable, let's not check them */
        winetest_pop_context();
    }

    IDirectMusicCollection_Release(collection);

skip_tests:
    IDirectMusicLoader_Release(loader);
}

START_TEST(dmusic)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (missing_dmusic())
    {
        skip("DirectMusic not available\n");
        CoUninitialize();
        return;
    }
    test_COM();
    test_COM_dmcoll();
    test_COM_synthport();
    test_dmusic();
    test_setdsound();
    test_dmbuffer();
    test_dmcoll();
    test_parsedescriptor();
    test_master_clock();
    test_synthport();
    test_port_kscontrol();
    test_port_download();
    test_download_instrument();
    test_default_gm_collection();

    CoUninitialize();
}
