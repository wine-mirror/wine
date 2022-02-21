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

static ULONG get_refcount(IDirectSound *iface)
{
    IDirectSound_AddRef(iface);
    return IDirectSound_Release(iface);
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
    DWORD cookie;
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

    hr = IReferenceClock_AdviseTime(clock, 0, 0, NULL, &cookie);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IReferenceClock_AdvisePeriodic(clock, 0, 0, NULL, &cookie);
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

    CoUninitialize();
}
