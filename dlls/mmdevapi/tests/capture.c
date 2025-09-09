/*
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
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

/* This test is for audio capture specific mechanisms
 * Tests:
 * - IAudioClient with eCapture and IAudioCaptureClient
 */

#include <stdint.h>
#include <math.h>

#include "wine/test.h"

#define COBJMACROS

#ifdef STANDALONE
#include "initguid.h"
#endif

#include "unknwn.h"
#include "uuids.h"
#include "mmdeviceapi.h"
#include "audioclient.h"

#define PCM WAVE_FORMAT_PCM
#define FLOAT WAVE_FORMAT_IEEE_FLOAT

static const unsigned int win_formats[][4] = {
    {PCM,    8000,  8,  1},   {PCM,    8000,  8,  2},   {PCM,  8000, 16, 1},   {PCM,  8000, 16, 2},
    {PCM,   11025,  8,  1},   {PCM,   11025,  8,  2},   {PCM, 11025, 16, 1},   {PCM, 11025, 16, 2},
    {PCM,   12000,  8,  1},   {PCM,   12000,  8,  2},   {PCM, 12000, 16, 1},   {PCM, 12000, 16, 2},
    {PCM,   16000,  8,  1},   {PCM,   16000,  8,  2},   {PCM, 16000, 16, 1},   {PCM, 16000, 16, 2},
    {PCM,   22050,  8,  1},   {PCM,   22050,  8,  2},   {PCM, 22050, 16, 1},   {PCM, 22050, 16, 2},
    {PCM,   44100,  8,  1},   {PCM,   44100,  8,  2},   {PCM, 44100, 16, 1},   {PCM, 44100, 16, 2},
    {PCM,   48000,  8,  1},   {PCM,   48000,  8,  2},   {PCM, 48000, 16, 1},   {PCM, 48000, 16, 2},
    {PCM,   96000,  8,  1},   {PCM,   96000,  8,  2},   {PCM, 96000, 16, 1},   {PCM, 96000, 16, 2},
    {FLOAT,  8000,  32, 1},   {FLOAT,  8000,  32, 2},
    {FLOAT, 11025,  32, 1},   {FLOAT, 11025,  32, 2},
    {FLOAT, 12000,  32, 1},   {FLOAT, 12000,  32, 2},
    {FLOAT, 16000,  32, 1},   {FLOAT, 16000,  32, 2},
    {FLOAT, 22050,  32, 1},   {FLOAT, 22050,  32, 2},
    {FLOAT, 44100,  32, 1},   {FLOAT, 44100,  32, 2},
    {FLOAT, 48000,  32, 1},   {FLOAT, 48000,  32, 2},
    {FLOAT, 96000,  32, 1},   {FLOAT, 96000,  32, 2},
};

#undef PCM
#undef FLOAT

#define NULL_PTR_ERR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER)

/* undocumented error code */
#define D3D11_ERROR_4E MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DIRECT3D11, 0x4e)

static IMMDeviceEnumerator *mme;
static IMMDevice *dev;
static const LARGE_INTEGER ullZero;

static void test_uninitialized(IAudioClient *ac)
{
    HRESULT hr;
    UINT32 num;
    REFERENCE_TIME t1;

    HANDLE handle = CreateEventW(NULL, FALSE, FALSE, NULL);
    IUnknown *unk;

    hr = IAudioClient_GetBufferSize(ac, &num);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetBufferSize call returns %08lx\n", hr);

    hr = IAudioClient_GetStreamLatency(ac, &t1);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetStreamLatency call returns %08lx\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &num);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetCurrentPadding call returns %08lx\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized Start call returns %08lx\n", hr);

    hr = IAudioClient_Stop(ac);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized Stop call returns %08lx\n", hr);

    hr = IAudioClient_Reset(ac);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized Reset call returns %08lx\n", hr);

    hr = IAudioClient_SetEventHandle(ac, handle);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized SetEventHandle call returns %08lx\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&unk);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetService call returns %08lx\n", hr);

    CloseHandle(handle);
}

struct read_packets_data
{
    UINT64 expected_dev_pos;
    UINT32 period;
    BOOL discontinuity_at_0;
    BOOL discontinuity_at_later;
};

static void read_packets(IAudioClient *ac, IAudioCaptureClient *acc, HANDLE handle,
        unsigned int packet_count, struct read_packets_data *data)
{
    unsigned int idx = 0;
    UINT32 padding;
    HRESULT hr;

    data->discontinuity_at_0 = FALSE;
    data->discontinuity_at_later = FALSE;

    while (idx < packet_count)
    {
        UINT64 dev_pos, dev_pos2, qpc_pos, qpc_pos2;
        UINT32 next_packet_size, frames, frames2;
        DWORD flags, flags2;
        BYTE *ptr;

        winetest_push_context("packet %u", idx);

        hr = IAudioCaptureClient_GetNextPacketSize(acc, &next_packet_size);
        ok(hr == S_OK, "GetNextPacketSize returns %08lx\n", hr);

        if (next_packet_size == 0)
        {
            /* There is some room for flakyness here, in theory a packet could arrive between the
             * GetNextPacketSize() call and the GetBuffer() call. Hopefully this is not very probable. */
            ptr = (void*)0xdeadf00ddeadf00d;
            flags = 0xdeadf11d;
            dev_pos = 0xdeadf22ddeadf22d;
            qpc_pos = 0xdeadf33ddeadf33d;
            hr = IAudioCaptureClient_GetBuffer(acc, &ptr, &frames, &flags, &dev_pos, &qpc_pos);
            ok(hr == AUDCLNT_S_BUFFER_EMPTY, "GetBuffer returns %08lx\n", hr);
            ok(broken((uintptr_t)ptr == (uintptr_t)0xdeadf00ddeadf00d) || /* <= win8 */
                    !ptr, "Unexpected data after GetBuffer: %p\n", ptr);
            ok(flags == 0xdeadf11d, "Unexpected flags after GetBuffer: %08lx\n", flags);
            ok(dev_pos == 0xdeadf22ddeadf22d, "Unexpected device position after GetBuffer: %I64u\n", dev_pos);
            ok(qpc_pos == 0xdeadf33ddeadf33d, "Unexpected QPC position after GetBuffer: %I64u\n", qpc_pos);

            ok(WaitForSingleObject(handle, 1000) == WAIT_OBJECT_0, "Waiting on event handle failed!\n");

            winetest_pop_context();
            continue;
        }

        hr = IAudioCaptureClient_GetBuffer(acc, &ptr, &frames, &flags, &dev_pos, &qpc_pos);
        ok(hr == S_OK, "GetBuffer returns %08lx\n", hr);

        ok(next_packet_size == frames, "GetNextPacketSize returns %u, GetBuffer returns %u frames\n",
                next_packet_size, frames);

        hr = IAudioCaptureClient_GetNextPacketSize(acc, &next_packet_size);
        ok(hr == S_OK, "GetNextPacketSize returns %08lx\n", hr);
        ok(next_packet_size == frames, "GetNextPacketSize returns %u, GetBuffer returns %u frames\n",
                next_packet_size, frames);

        hr = IAudioClient_GetCurrentPadding(ac, &padding);
        ok(hr == S_OK, "GetCurrentPadding returns %08lx\n", hr);
        ok(padding >= frames, "GetCurrentPadding returns %u, GetBuffer returns %u frames\n",
                padding, frames);

        if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY)
        {
            if (idx == 0)
                data->discontinuity_at_0 = TRUE;
            else
                data->discontinuity_at_later = TRUE;
        }

        if (data->expected_dev_pos != UINT64_MAX)
        {
            /* Win <= 8 and some older Win 10 builds sometimes handle discontinuities incorrectly. */
            ok(dev_pos >= data->expected_dev_pos || broken(dev_pos < data->expected_dev_pos),
                    "GetBuffer returns %I64u device position, expected at least %I64u\n",
                    dev_pos, data->expected_dev_pos);
            if (!(flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY))
                ok(dev_pos == data->expected_dev_pos || broken(dev_pos != data->expected_dev_pos),
                        "GetBuffer returns %I64u device position, expected %I64u\n",
                        dev_pos, data->expected_dev_pos);
        }

        ptr = (void*)0xdeadf00ddeadf00d;
        flags2 = 0xdeadf11d;
        dev_pos2 = 0xdeadf22ddeadf22d;
        qpc_pos2 = 0xdeadf33ddeadf33d;
        hr = IAudioCaptureClient_GetBuffer(acc, &ptr, &frames2, &flags2, &dev_pos2, &qpc_pos2);
        ok(hr == AUDCLNT_E_OUT_OF_ORDER, "Second GetBuffer returns %08lx\n", hr);
        ok(broken((uintptr_t)ptr == (uintptr_t)0xdeadf00ddeadf00d) || /* <= win8 */
                !ptr, "Unexpected data after second GetBuffer: %p\n", ptr);
        ok(flags2 == 0xdeadf11d, "Unexpected flags after second GetBuffer: %08lx\n", flags2);
        ok(dev_pos2 == 0xdeadf22ddeadf22d, "Unexpected device position after second GetBuffer: %I64u\n", dev_pos2);
        ok(qpc_pos2 == 0xdeadf33ddeadf33d, "Unexpected QPC position after second GetBuffer: %I64u\n", qpc_pos2);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, 0);
        ok(hr == S_OK, "Releasing buffer returns %08lx\n", hr);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, 0);
        ok(hr == S_OK, "Releasing buffer again returns %08lx\n", hr);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
        ok(hr == AUDCLNT_E_OUT_OF_ORDER, "Releasing buffer again returns %08lx\n", hr);

        hr = IAudioCaptureClient_GetNextPacketSize(acc, &next_packet_size);
        ok(hr == S_OK, "GetNextPacketSize returns %08lx\n", hr);
        ok(next_packet_size == frames, "GetNextPacketSize returns %u, GetBuffer returns %u frames\n",
                next_packet_size, frames);

        hr = IAudioCaptureClient_GetBuffer(acc, &ptr, &frames2, &flags2, &dev_pos2, &qpc_pos2);
        ok(hr == S_OK, "GetBuffer returns %08lx\n", hr);

        ok(frames == frames2, "First GetBuffer returns %u frames, second GetBuffer returns %u\n",
                frames, frames2);
        ok(flags == flags2, "First GetBuffer returns %08lx flags, second GetBuffer returns %08lx\n",
                flags, flags2);
        ok(dev_pos == dev_pos2, "First GetBuffer returns %I64u device position, second GetBuffer returns %I64u\n",
                dev_pos, dev_pos2);
        /* Works with Pulse, but fails with ALSA and CoreAudio. */
        todo_wine_if(qpc_pos != qpc_pos2)
        ok(qpc_pos == qpc_pos2, "First GetBuffer returns %I64u device QPC, second GetBuffer returns %I64u\n",
                qpc_pos, qpc_pos2);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, frames - 1);
        ok(hr == AUDCLNT_E_INVALID_SIZE, "Releasing buffer with the wrong frame count returns %08lx\n", hr);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
        ok(hr == S_OK, "Releasing buffer returns %08lx\n", hr);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, 0);
        ok(hr == S_OK, "Releasing buffer again returns %08lx\n", hr);

        data->expected_dev_pos = dev_pos + frames;

        ++idx;

        winetest_pop_context();
    }

    /* Here we should have basically emptied the buffer, but we allow one or two
     * packets to arrive concurrently. */
    hr = IAudioClient_GetCurrentPadding(ac, &padding);
    ok(hr == S_OK, "GetCurrentPadding returns %08lx\n", hr);
    ok(padding <= 2 * data->period, "GetCurrentPadding %u is larger then twice the period%u\n",
            padding, data->period);
}

static void test_capture(IAudioClient *ac, HANDLE handle, WAVEFORMATEX *wfx)
{
    struct read_packets_data packets_data;
    IAudioCaptureClient *acc;
    HRESULT hr;
    UINT32 frames, next, pad, sum = 0;
    BYTE *data;
    DWORD flags;
    UINT64 pos, qpc;
    REFERENCE_TIME period;

    hr = IAudioClient_GetService(ac, &IID_IAudioCaptureClient, (void**)&acc);
    ok(hr == S_OK, "IAudioClient_GetService(IID_IAudioCaptureClient) returns %08lx\n", hr);
    if (hr != S_OK)
        return;

    ok(ResetEvent(handle), "ResetEvent\n");

    hr = IAudioCaptureClient_GetNextPacketSize(acc, &next);
    ok(hr == S_OK, "IAudioCaptureClient_GetNextPacketSize returns %08lx\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding call returns %08lx\n", hr);
    ok(next == pad, "GetNextPacketSize %u vs. GCP %u\n", next, pad);
    /* later GCP will grow, while GNPS is 0 or period size */

    hr = IAudioCaptureClient_GetNextPacketSize(acc, NULL);
    ok(hr == E_POINTER, "IAudioCaptureClient_GetNextPacketSize(NULL) returns %08lx\n", hr);

    data = (void*)0xdeadf00d;
    frames = 0xdeadbeef;
    flags = 0xabadcafe;
    hr = IAudioCaptureClient_GetBuffer(acc, &data, NULL, NULL, NULL, NULL);
    ok(hr == E_POINTER, "IAudioCaptureClient_GetBuffer(data, NULL, NULL) returns %08lx\n", hr);

    hr = IAudioCaptureClient_GetBuffer(acc, NULL, &frames, NULL, NULL, NULL);
    ok(hr == E_POINTER, "IAudioCaptureClient_GetBuffer(NULL, &frames, NULL) returns %08lx\n", hr);

    hr = IAudioCaptureClient_GetBuffer(acc, NULL, NULL, &flags, NULL, NULL);
    ok(hr == E_POINTER, "IAudioCaptureClient_GetBuffer(NULL, NULL, &flags) returns %08lx\n", hr);

    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, NULL, NULL, NULL);
    ok(hr == E_POINTER, "IAudioCaptureClient_GetBuffer(&ata, &frames, NULL) returns %08lx\n", hr);
    ok(broken((DWORD_PTR)data == 0xdeadf00d) || /* <= win8 */
            data == NULL, "data is reset to %p\n", data);
    ok(frames == 0xdeadbeef, "frames is reset to %08x\n", frames);
    ok(flags == 0xabadcafe, "flags is reset to %08lx\n", flags);

    hr = IAudioClient_GetDevicePeriod(ac, &period, NULL);
    ok(hr == S_OK, "GetDevicePeriod failed: %08lx\n", hr);
    packets_data.period = MulDiv(period, wfx->nSamplesPerSec, 10000000); /* as in render.c */

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start on a stopped stream returns %08lx\n", hr);

    packets_data.expected_dev_pos = UINT64_MAX;

    winetest_push_context("read 10 packets");

    read_packets(ac, acc, handle, 10, &packets_data);

    todo_wine
    ok(packets_data.discontinuity_at_0, "No discontinuity at first packet\n");
    ok(!packets_data.discontinuity_at_later, "Discontinuity at later packet\n");

    winetest_pop_context();

    sum = packets_data.expected_dev_pos;

    Sleep(350); /* for sure there's data now */

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding call returns %08lx\n", hr);

    /** GetNextPacketSize
     * returns either 0 or one period worth of frames
     * whereas GetCurrentPadding grows when input is not consumed. */
    hr = IAudioCaptureClient_GetNextPacketSize(acc, &next);
    ok(hr == S_OK, "IAudioCaptureClient_GetNextPacketSize returns %08lx\n", hr);
    flaky_wine
    ok(next <  pad, "GetNextPacketSize %u vs. GCP %u\n", next, pad);

    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos, &qpc);
    flaky_wine
    ok(hr == S_OK, "Valid IAudioCaptureClient_GetBuffer returns %08lx\n", hr);
    ok(next == frames, "GetNextPacketSize %u vs. GetBuffer %u\n", next, frames);

    if(hr == S_OK){
        UINT32 frames2 = frames;
        UINT64 pos2, qpc2;
        ok(frames, "Amount of frames locked is 0!\n");
        ok(pos == sum, "Position %u expected %u\n", (UINT)pos, sum);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, 0);
        ok(hr == S_OK, "Releasing 0 returns %08lx\n", hr);

        /* GCP did not decrement, no data consumed */
        hr = IAudioClient_GetCurrentPadding(ac, &frames);
        ok(hr == S_OK, "GetCurrentPadding call returns %08lx\n", hr);
        ok(frames == pad || frames == pad + next /* concurrent feeder */,
           "GCP %u past ReleaseBuffer(0) initially %u\n", frames, pad);

        /* should re-get the same data */
        hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos2, &qpc2);
        ok(hr == S_OK, "Valid IAudioCaptureClient_GetBuffer returns %08lx\n", hr);
        ok(frames2 == frames, "GetBuffer after ReleaseBuffer(0) %u/%u\n", frames2, frames);
        ok(pos2 == pos, "Position after ReleaseBuffer(0) %u/%u\n", (UINT)pos2, (UINT)pos);
        todo_wine_if(qpc2 != qpc)
            /* FIXME: Some drivers fail */
            ok(qpc2 == qpc, "HPC after ReleaseBuffer(0) %u vs. %u\n", (UINT)qpc2, (UINT)qpc);
    }

    /* trace after the GCP test because log output to MS-DOS console disturbs timing */
    trace("Sleep.1 position %d pad %u flags %lx, amount of frames locked: %u\n",
          hr==S_OK ? (UINT)pos : -1, pad, flags, frames);

    if(hr == S_OK){
        UINT32 frames2 = 0xabadcafe;
        BYTE *data2 = (void*)0xdeadf00d;
        flags = 0xabadcafe;

        ok(pos == sum, "Position %u expected %u\n", (UINT)pos, sum);

        pos = qpc = 0xdeadbeef;
        hr = IAudioCaptureClient_GetBuffer(acc, &data2, &frames2, &flags, &pos, &qpc);
        ok(hr == AUDCLNT_E_OUT_OF_ORDER, "Out of order IAudioCaptureClient_GetBuffer returns %08lx\n", hr);
        ok(frames2 == 0xabadcafe, "Out of order frames changed to %x\n", frames2);
        ok(broken(data2 == (void*)0xdeadf00d) /* <= win8 */ ||
                data2 == NULL, "Out of order data changed to %p\n", data2);
        ok(flags == 0xabadcafe, "Out of order flags changed to %lx\n", flags);
        ok(pos == 0xdeadbeef, "Out of order position changed to %x\n", (UINT)pos);
        ok(qpc == 0xdeadbeef, "Out of order timer changed to %x\n", (UINT)qpc);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, frames+1);
        ok(hr == AUDCLNT_E_INVALID_SIZE, "Releasing buffer+1 returns %08lx\n", hr);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, 1);
        ok(hr == AUDCLNT_E_INVALID_SIZE, "Releasing 1 returns %08lx\n", hr);

        hr = IAudioClient_Reset(ac);
        ok(hr == AUDCLNT_E_NOT_STOPPED, "Reset failed: %08lx\n", hr);
    }

    hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
    ok(hr == S_OK, "Releasing buffer returns %08lx\n", hr);

    if (frames) {
        sum += frames;
        hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
        ok(hr == AUDCLNT_E_OUT_OF_ORDER, "Releasing buffer twice returns %08lx\n", hr);
    }

    frames = packets_data.period;
    flaky_wine
    ok(next == frames, "GetNextPacketSize %u vs. GetDevicePeriod %u\n", next, frames);

    /* GetBufferSize is not a multiple of the period size! */
    hr = IAudioClient_GetBufferSize(ac, &next);
    ok(hr == S_OK, "GetBufferSize failed: %08lx\n", hr);
    trace("GetBufferSize %u period size %u\n", next, frames);

    Sleep(600); /* overrun */

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding call returns %08lx\n", hr);

    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos, &qpc);
    flaky_wine
    ok(hr == S_OK, "Valid IAudioCaptureClient_GetBuffer returns %08lx\n", hr);

    trace("Overrun position %d pad %u flags %lx, amount of frames locked: %u\n",
          hr==S_OK ? (UINT)pos : -1, pad, flags, frames);

    if(hr == S_OK){
        if(flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY){
            /* Native's position is one period further than what we read.
             * Perhaps that's precisely the meaning of DATA_DISCONTINUITY:
             * signal when the position jump left a gap. */
            ok(pos >= sum + frames, "Position %u last %u frames %u\n", (UINT)pos, sum, frames);
            sum = pos;
        }else{ /* win10 */
            ok(pos == sum, "Position %u last %u frames %u\n", (UINT)pos, sum, frames);
        }

        ok(pad == next, "GCP %u vs. BufferSize %u\n", (UINT32)pad, next);
    }

    hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
    ok(hr == S_OK, "Releasing buffer returns %08lx\n", hr);
    sum += frames;

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding call returns %08lx\n", hr);

    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos, &qpc);
    flaky_wine
    ok(hr == S_OK, "Valid IAudioCaptureClient_GetBuffer returns %08lx\n", hr);

    trace("Cont'ed position %d pad %u flags %lx, amount of frames locked: %u\n",
          hr==S_OK ? (UINT)pos : -1, pad, flags, frames);

    if(hr == S_OK){
        flaky_wine
        ok(pos == sum, "Position %u expected %u\n", (UINT)pos, sum);
        flaky_wine
        ok(!flags, "flags %lu\n", flags);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
        ok(hr == S_OK, "Releasing buffer returns %08lx\n", hr);
        sum += frames;
    }

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop on a started stream returns %08lx\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start on a stopped stream returns %08lx\n", hr);

    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos, &qpc);
    flaky_wine
    ok(hr == S_OK, "Valid IAudioCaptureClient_GetBuffer returns %08lx\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding call returns %08lx\n", hr);

    trace("Restart position %d pad %u flags %lx, amount of frames locked: %u\n",
          hr==S_OK ? (UINT)pos : -1, pad, flags, frames);
    flaky_wine
    ok(pad > sum, "restarted GCP %u\n", pad); /* GCP is still near buffer size */

    if(frames){
        flaky_wine
        ok(pos == sum, "Position %u expected %u\n", (UINT)pos, sum);
        ok(!flags, "flags %lu\n", flags);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
        ok(hr == S_OK, "Releasing buffer returns %08lx\n", hr);
        sum += frames;
    }

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop on a started stream returns %08lx\n", hr);

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset on a stopped stream returns %08lx\n", hr);
    sum += pad - frames;

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding call returns %08lx\n", hr);
    ok(!pad, "reset GCP %u\n", pad);

    flags = 0xabadcafe;
    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos, &qpc);
    ok(hr == AUDCLNT_S_BUFFER_EMPTY,
       "Initial IAudioCaptureClient_GetBuffer returns %08lx\n", hr);

    trace("Reset   position %d pad %u flags %lx, amount of frames locked: %u\n",
          hr==S_OK ? (UINT)pos : -1, pad, flags, frames);

    if(SUCCEEDED(hr))
        IAudioCaptureClient_ReleaseBuffer(acc, frames);

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start on a stopped stream returns %08lx\n", hr);

    packets_data.expected_dev_pos = sum;

    read_packets(ac, acc, handle, 10, &packets_data);

    IAudioCaptureClient_Release(acc);
}

static void test_audioclient(void)
{
    IAudioClient *ac;
    IUnknown *unk;
    HRESULT hr;
    ULONG ref;
    WAVEFORMATEX *pwfx, *pwfx2;
    REFERENCE_TIME t1, t2;
    HANDLE handle;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    handle = CreateEventW(NULL, FALSE, FALSE, NULL);

    unk = (void*)(LONG_PTR)0x12345678;
    hr = IAudioClient_QueryInterface(ac, &IID_NULL, (void**)&unk);
    ok(hr == E_NOINTERFACE, "QueryInterface(IID_NULL) returned %08lx\n", hr);
    ok(!unk, "QueryInterface(IID_NULL) returned non-null pointer %p\n", unk);

    hr = IAudioClient_QueryInterface(ac, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface(IID_IUnknown) returned %08lx\n", hr);
    if (unk)
    {
        ref = IUnknown_Release(unk);
        ok(ref == 1, "Released count is %lu\n", ref);
    }

    hr = IAudioClient_QueryInterface(ac, &IID_IAudioClient, (void**)&unk);
    ok(hr == S_OK, "QueryInterface(IID_IAudioClient) returned %08lx\n", hr);
    if (unk)
    {
        ref = IUnknown_Release(unk);
        ok(ref == 1, "Released count is %lu\n", ref);
    }

    hr = IAudioClient_GetDevicePeriod(ac, NULL, NULL);
    ok(hr == E_POINTER, "Invalid GetDevicePeriod call returns %08lx\n", hr);

    hr = IAudioClient_GetDevicePeriod(ac, &t1, NULL);
    ok(hr == S_OK, "Valid GetDevicePeriod call returns %08lx\n", hr);

    hr = IAudioClient_GetDevicePeriod(ac, NULL, &t2);
    ok(hr == S_OK, "Valid GetDevicePeriod call returns %08lx\n", hr);

    hr = IAudioClient_GetDevicePeriod(ac, &t1, &t2);
    ok(hr == S_OK, "Valid GetDevicePeriod call returns %08lx\n", hr);
    trace("Returned periods: %u.%04u ms %u.%04u ms\n",
          (UINT)(t1/10000), (UINT)(t1 % 10000),
          (UINT)(t2/10000), (UINT)(t2 % 10000));

    hr = IAudioClient_GetMixFormat(ac, NULL);
    ok(hr == E_POINTER, "GetMixFormat returns %08lx\n", hr);

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "Valid GetMixFormat returns %08lx\n", hr);

    if (hr == S_OK)
    {
        trace("pwfx: %p\n", pwfx);
        trace("Tag: %04x\n", pwfx->wFormatTag);
        trace("bits: %u\n", pwfx->wBitsPerSample);
        trace("chan: %u\n", pwfx->nChannels);
        trace("rate: %lu\n", pwfx->nSamplesPerSec);
        trace("align: %u\n", pwfx->nBlockAlign);
        trace("extra: %u\n", pwfx->cbSize);
        ok(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE, "wFormatTag is %x\n", pwfx->wFormatTag);
        if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            WAVEFORMATEXTENSIBLE *pwfxe = (void*)pwfx;
            trace("Res: %u\n", pwfxe->Samples.wReserved);
            trace("Mask: %lx\n", pwfxe->dwChannelMask);
            trace("Alg: %s\n",
                  IsEqualGUID(&pwfxe->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)?"PCM":
                  (IsEqualGUID(&pwfxe->SubFormat,
                               &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)?"FLOAT":"Other"));
        }

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
        ok(hr == S_OK, "Valid IsFormatSupported(Shared) call returns %08lx\n", hr);
        ok(pwfx2 == NULL, "pwfx2 is non-null\n");
        CoTaskMemFree(pwfx2);

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, NULL, NULL);
        ok(hr == E_POINTER, "IsFormatSupported(NULL) call returns %08lx\n", hr);

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, NULL);
        ok(hr == E_POINTER, "IsFormatSupported(Shared,NULL) call returns %08lx\n", hr);

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, NULL);
        ok(hr == S_OK || hr == AUDCLNT_E_UNSUPPORTED_FORMAT, "IsFormatSupported(Exclusive) call returns %08lx\n", hr);

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, &pwfx2);
        ok(hr == S_OK || hr == AUDCLNT_E_UNSUPPORTED_FORMAT, "IsFormatSupported(Exclusive) call returns %08lx\n", hr);
        ok(pwfx2 == NULL, "pwfx2 non-null on exclusive IsFormatSupported\n");

        hr = IAudioClient_IsFormatSupported(ac, 0xffffffff, pwfx, NULL);
        ok(hr == E_INVALIDARG/*w32*/ ||
           broken(hr == AUDCLNT_E_UNSUPPORTED_FORMAT/*w64 response from exclusive mode driver */),
           "IsFormatSupported(0xffffffff) call returns %08lx\n", hr);
    }

    test_uninitialized(ac);

    hr = IAudioClient_Initialize(ac, 3, 0, 5000000, 0, pwfx, NULL);
    ok(broken(hr == AUDCLNT_E_NOT_INITIALIZED) || /* <= win8 */
            hr == E_INVALIDARG, "Initialize with invalid sharemode returns %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0xffffffff, 5000000, 0, pwfx, NULL);
    ok(hr == E_INVALIDARG || hr == AUDCLNT_E_INVALID_STREAM_FLAG, "Initialize with invalid flags returns %08lx\n", hr);

    /* A period != 0 is ignored and the call succeeds.
     * Since we can only initialize successfully once, skip those tests.
     */
    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, NULL, NULL);
    ok(hr == E_POINTER, "Initialize with null format returns %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 4987654, 0, pwfx, NULL);
    ok(hr == S_OK, "Valid Initialize returns %08lx\n", hr);

    if (hr != S_OK)
    {
        skip("Cannot initialize %08lx, remainder of tests is useless\n", hr);
        goto cleanup;
    }

    hr = IAudioClient_GetStreamLatency(ac, NULL);
    ok(hr == E_POINTER, "GetStreamLatency(NULL) call returns %08lx\n", hr);

    hr = IAudioClient_GetStreamLatency(ac, &t1);
    ok(hr == S_OK, "Valid GetStreamLatency call returns %08lx\n", hr);
    trace("Returned latency: %u.%04u ms\n",
          (UINT)(t1/10000), (UINT)(t1 % 10000));

    hr = IAudioClient_SetEventHandle(ac, NULL);
    ok(hr == E_INVALIDARG, "SetEventHandle(NULL) returns %08lx\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == AUDCLNT_E_EVENTHANDLE_NOT_SET ||
            hr == D3D11_ERROR_4E /* win10 */, "Start before SetEventHandle returns %08lx\n", hr);

    hr = IAudioClient_SetEventHandle(ac, handle);
    ok(hr == S_OK, "SetEventHandle returns %08lx\n", hr);

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset on an already reset stream returns %08lx\n", hr);

    hr = IAudioClient_Stop(ac);
    ok(hr == S_FALSE, "Stop on a stopped stream returns %08lx\n", hr);

    test_capture(ac, handle, pwfx);

cleanup:
    IAudioClient_Release(ac);
    CloseHandle(handle);
    CoTaskMemFree(pwfx);
}

static void test_formats(AUDCLNT_SHAREMODE mode)
{
    IAudioClient *ac;
    HRESULT hr, hrs;
    WAVEFORMATEX fmt, *pwfx, *pwfx2;
    int i;

    fmt.cbSize = 0;

    for(i = 0; i < ARRAY_SIZE(win_formats); i++) {
        char format_chr;

        hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                NULL, (void**)&ac);
        ok(hr == S_OK, "Activation failed with %08lx\n", hr);
        if(hr != S_OK)
            continue;

        hr = IAudioClient_GetMixFormat(ac, &pwfx);
        ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

        fmt.wFormatTag     = win_formats[i][0];
        fmt.nSamplesPerSec = win_formats[i][1];
        fmt.wBitsPerSample = win_formats[i][2];
        fmt.nChannels      = win_formats[i][3];
        fmt.nBlockAlign    = fmt.nChannels * fmt.wBitsPerSample / 8;
        fmt.nAvgBytesPerSec= fmt.nBlockAlign * fmt.nSamplesPerSec;

        format_chr = fmt.wFormatTag == WAVE_FORMAT_PCM ? 'P' : 'F';

        pwfx2 = (WAVEFORMATEX*)0xDEADF00D;
        hr = IAudioClient_IsFormatSupported(ac, mode, &fmt, &pwfx2);
        hrs = hr;
        /* Only shared mode suggests something ... GetMixFormat! */
        ok(hr == S_OK || (mode == AUDCLNT_SHAREMODE_SHARED
           ? hr == S_FALSE : hr == AUDCLNT_E_UNSUPPORTED_FORMAT),
           "IsFormatSupported(%d, %c%lux%2ux%u) returns %08lx\n", mode,
           format_chr, fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr);
        if (hr == S_OK)
            trace("IsSupported(%s, %c%lux%2ux%u)\n",
                  mode == AUDCLNT_SHAREMODE_SHARED ? "shared " : "exclus.",
                  format_chr, fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels);

        /* In shared mode you can only change bit width, not sampling rate or channel count. */
        if (mode == AUDCLNT_SHAREMODE_SHARED)
        {
            BOOL compatible = fmt.nSamplesPerSec == pwfx->nSamplesPerSec && fmt.nChannels == pwfx->nChannels;
            HRESULT expected = compatible ? S_OK : S_FALSE;
            todo_wine_if(expected == S_FALSE)
            ok(hr == expected, "Got %lx expected %lx\n", hr, expected);
        }

        ok((hr == S_FALSE)^(pwfx2 == NULL), "hr %lx<->suggest %p\n", hr, pwfx2);
        if (pwfx2) {
            ok(pwfx2->wFormatTag     == pwfx->wFormatTag &&
               pwfx2->nSamplesPerSec == pwfx->nSamplesPerSec &&
               pwfx2->nChannels      == pwfx->nChannels &&
               pwfx2->wBitsPerSample == pwfx->wBitsPerSample,
               "Suggestion %c%lux%2ux%u differs from GetMixFormat\n",
               format_chr, pwfx2->nSamplesPerSec, pwfx2->wBitsPerSample, pwfx2->nChannels);
        }

        hr = IAudioClient_Initialize(ac, mode, 0, 5000000, 0, &fmt, NULL);
        if ((hrs == S_OK) ^ (hr == S_OK))
            trace("Initialize (%s, %c%lux%2ux%u) returns %08lx unlike IsFormatSupported\n",
                  mode == AUDCLNT_SHAREMODE_SHARED ? "shared " : "exclus.",
                  format_chr, fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr);
        if (mode == AUDCLNT_SHAREMODE_SHARED)
            ok(hrs == S_OK ? hr == S_OK : hr == AUDCLNT_E_UNSUPPORTED_FORMAT,
               "Initialize(shared,  %c%lux%2ux%u) returns %08lx\n",
               format_chr, fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr);
        else if (hrs == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED)
            /* Unsupported format implies "create failed" and shadows "not allowed" */
            ok(hr == AUDCLNT_E_ENDPOINT_CREATE_FAILED || hr == hrs,
               "Initialize(noexcl., %c%lux%2ux%u) returns %08lx(%08lx)\n",
               format_chr, fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr, hrs);
        else
            todo_wine_if(hr == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED)
            ok(hrs == S_OK ? hr == S_OK
               : hr == AUDCLNT_E_ENDPOINT_CREATE_FAILED || hr == AUDCLNT_E_UNSUPPORTED_FORMAT,
               "Initialize(exclus., %c%lux%2ux%u) returns %08lx\n",
               format_chr, fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr);

        CoTaskMemFree(pwfx2);
        CoTaskMemFree(pwfx);
        IAudioClient_Release(ac);
    }
}

static void test_streamvolume(void)
{
    IAudioClient *ac;
    IAudioStreamVolume *asv;
    WAVEFORMATEX *fmt;
    UINT32 chans, i;
    HRESULT hr;
    float vol, *vols;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, fmt, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&asv);
    ok(hr == S_OK, "GetService failed: %08lx\n", hr);

    hr = IAudioStreamVolume_GetChannelCount(asv, NULL);
    ok(hr == E_POINTER, "GetChannelCount gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_GetChannelCount(asv, &chans);
    ok(hr == S_OK, "GetChannelCount failed: %08lx\n", hr);
    ok(chans == fmt->nChannels, "GetChannelCount gave wrong number of channels: %d\n", chans);

    hr = IAudioStreamVolume_GetChannelVolume(asv, fmt->nChannels, NULL);
    ok(hr == E_POINTER, "GetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, fmt->nChannels, &vol);
    ok(hr == E_INVALIDARG, "GetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, NULL);
    ok(hr == E_POINTER, "GetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, &vol);
    ok(hr == S_OK, "GetChannelVolume failed: %08lx\n", hr);
    ok(vol == 1.f, "Channel volume was not 1: %f\n", vol);

    hr = IAudioStreamVolume_SetChannelVolume(asv, fmt->nChannels, -1.f);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, -1.f);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, 2.f);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, 0.2f);
    ok(hr == S_OK, "SetChannelVolume failed: %08lx\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, &vol);
    ok(hr == S_OK, "GetChannelVolume failed: %08lx\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Channel volume wasn't 0.2: %f\n", vol);

    hr = IAudioStreamVolume_GetAllVolumes(asv, 0, NULL);
    ok(hr == E_POINTER, "GetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_GetAllVolumes(asv, fmt->nChannels, NULL);
    ok(hr == E_POINTER, "GetAllVolumes gave wrong error: %08lx\n", hr);

    vols = HeapAlloc(GetProcessHeap(), 0, fmt->nChannels * sizeof(float));
    ok(vols != NULL, "HeapAlloc failed\n");

    hr = IAudioStreamVolume_GetAllVolumes(asv, fmt->nChannels - 1, vols);
    ok(hr == E_INVALIDARG, "GetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_GetAllVolumes(asv, fmt->nChannels, vols);
    ok(hr == S_OK, "GetAllVolumes failed: %08lx\n", hr);
    ok(fabsf(vols[0] - 0.2f) < 0.05f, "Channel 0 volume wasn't 0.2: %f\n", vol);
    for(i = 1; i < fmt->nChannels; ++i)
        ok(vols[i] == 1.f, "Channel %d volume is not 1: %f\n", i, vols[i]);

    hr = IAudioStreamVolume_SetAllVolumes(asv, 0, NULL);
    ok(hr == E_POINTER, "SetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_SetAllVolumes(asv, fmt->nChannels, NULL);
    ok(hr == E_POINTER, "SetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_SetAllVolumes(asv, fmt->nChannels - 1, vols);
    ok(hr == E_INVALIDARG, "SetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_SetAllVolumes(asv, fmt->nChannels, vols);
    ok(hr == S_OK, "SetAllVolumes failed: %08lx\n", hr);

    HeapFree(GetProcessHeap(), 0, vols);
    IAudioStreamVolume_Release(asv);
    IAudioClient_Release(ac);
    CoTaskMemFree(fmt);
}

static void test_channelvolume(void)
{
    IAudioClient *ac;
    IChannelAudioVolume *acv;
    WAVEFORMATEX *fmt;
    UINT32 chans, i;
    HRESULT hr;
    float vol, *vols;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IChannelAudioVolume, (void**)&acv);
    ok(hr == S_OK, "GetService failed: %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IChannelAudioVolume_GetChannelCount(acv, NULL);
    ok(hr == NULL_PTR_ERR, "GetChannelCount gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_GetChannelCount(acv, &chans);
    ok(hr == S_OK, "GetChannelCount failed: %08lx\n", hr);
    ok(chans == fmt->nChannels, "GetChannelCount gave wrong number of channels: %d\n", chans);

    hr = IChannelAudioVolume_GetChannelVolume(acv, fmt->nChannels, NULL);
    ok(hr == NULL_PTR_ERR, "GetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, fmt->nChannels, &vol);
    ok(hr == E_INVALIDARG, "GetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, 0, NULL);
    ok(hr == NULL_PTR_ERR, "GetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, 0, &vol);
    ok(hr == S_OK, "GetChannelVolume failed: %08lx\n", hr);
    ok(vol == 1.f, "Channel volume was not 1: %f\n", vol);

    hr = IChannelAudioVolume_SetChannelVolume(acv, fmt->nChannels, -1.f, NULL);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, -1.f, NULL);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, 2.f, NULL);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, 0.2f, NULL);
    ok(hr == S_OK, "SetChannelVolume failed: %08lx\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, 0, &vol);
    ok(hr == S_OK, "GetChannelVolume failed: %08lx\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Channel volume wasn't 0.2: %f\n", vol);

    hr = IChannelAudioVolume_GetAllVolumes(acv, 0, NULL);
    ok(hr == NULL_PTR_ERR, "GetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_GetAllVolumes(acv, fmt->nChannels, NULL);
    ok(hr == NULL_PTR_ERR, "GetAllVolumes gave wrong error: %08lx\n", hr);

    vols = HeapAlloc(GetProcessHeap(), 0, fmt->nChannels * sizeof(float));
    ok(vols != NULL, "HeapAlloc failed\n");

    hr = IChannelAudioVolume_GetAllVolumes(acv, fmt->nChannels - 1, vols);
    ok(hr == E_INVALIDARG, "GetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_GetAllVolumes(acv, fmt->nChannels, vols);
    ok(hr == S_OK, "GetAllVolumes failed: %08lx\n", hr);
    ok(fabsf(vols[0] - 0.2f) < 0.05f, "Channel 0 volume wasn't 0.2: %f\n", vol);
    for(i = 1; i < fmt->nChannels; ++i)
        ok(vols[i] == 1.f, "Channel %d volume is not 1: %f\n", i, vols[i]);

    hr = IChannelAudioVolume_SetAllVolumes(acv, 0, NULL, NULL);
    ok(hr == NULL_PTR_ERR, "SetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_SetAllVolumes(acv, fmt->nChannels, NULL, NULL);
    ok(hr == NULL_PTR_ERR, "SetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_SetAllVolumes(acv, fmt->nChannels - 1, vols, NULL);
    ok(hr == E_INVALIDARG, "SetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_SetAllVolumes(acv, fmt->nChannels, vols, NULL);
    ok(hr == S_OK, "SetAllVolumes failed: %08lx\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, 1.0f, NULL);
    ok(hr == S_OK, "SetChannelVolume failed: %08lx\n", hr);

    HeapFree(GetProcessHeap(), 0, vols);
    IChannelAudioVolume_Release(acv);
    IAudioClient_Release(ac);
    CoTaskMemFree(fmt);
}

static void test_simplevolume(void)
{
    IAudioClient *ac;
    ISimpleAudioVolume *sav;
    WAVEFORMATEX *fmt;
    HRESULT hr;
    float vol;
    BOOL mute;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
    ok(hr == S_OK, "GetService failed: %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = ISimpleAudioVolume_GetMasterVolume(sav, NULL);
    ok(hr == NULL_PTR_ERR, "GetMasterVolume gave wrong error: %08lx\n", hr);

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "GetMasterVolume failed: %08lx\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, -1.f, NULL);
    ok(hr == E_INVALIDARG, "SetMasterVolume gave wrong error: %08lx\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 2.f, NULL);
    ok(hr == E_INVALIDARG, "SetMasterVolume gave wrong error: %08lx\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 0.2f, NULL);
    ok(hr == S_OK, "SetMasterVolume failed: %08lx\n", hr);

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "GetMasterVolume failed: %08lx\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Master volume wasn't 0.2: %f\n", vol);

    hr = ISimpleAudioVolume_GetMute(sav, NULL);
    ok(hr == NULL_PTR_ERR, "GetMute gave wrong error: %08lx\n", hr);

    mute = TRUE;
    hr = ISimpleAudioVolume_GetMute(sav, &mute);
    ok(hr == S_OK, "GetMute failed: %08lx\n", hr);
    ok(mute == FALSE, "Session is already muted\n");

    hr = ISimpleAudioVolume_SetMute(sav, TRUE, NULL);
    ok(hr == S_OK, "SetMute failed: %08lx\n", hr);

    mute = FALSE;
    hr = ISimpleAudioVolume_GetMute(sav, &mute);
    ok(hr == S_OK, "GetMute failed: %08lx\n", hr);
    ok(mute == TRUE, "Session should have been muted\n");

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "GetMasterVolume failed: %08lx\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Master volume wasn't 0.2: %f\n", vol);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 1.f, NULL);
    ok(hr == S_OK, "SetMasterVolume failed: %08lx\n", hr);

    mute = FALSE;
    hr = ISimpleAudioVolume_GetMute(sav, &mute);
    ok(hr == S_OK, "GetMute failed: %08lx\n", hr);
    ok(mute == TRUE, "Session should have been muted\n");

    hr = ISimpleAudioVolume_SetMute(sav, FALSE, NULL);
    ok(hr == S_OK, "SetMute failed: %08lx\n", hr);

    ISimpleAudioVolume_Release(sav);
    IAudioClient_Release(ac);
    CoTaskMemFree(fmt);
}

static void test_volume_dependence(void)
{
    IAudioClient *ac, *ac2;
    ISimpleAudioVolume *sav;
    IChannelAudioVolume *cav;
    IAudioStreamVolume *asv;
    WAVEFORMATEX *fmt;
    HRESULT hr;
    float vol;
    GUID session;
    UINT32 nch;

    hr = CoCreateGuid(&session);
    ok(hr == S_OK, "CoCreateGuid failed: %08lx\n", hr);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, &session);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
    ok(hr == S_OK, "GetService (SimpleAudioVolume) failed: %08lx\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IChannelAudioVolume, (void**)&cav);
    ok(hr == S_OK, "GetService (ChannelAudioVolume) failed: %08lx\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&asv);
    ok(hr == S_OK, "GetService (AudioStreamVolume) failed: %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, 0.2f);
    ok(hr == S_OK, "ASV_SetChannelVolume failed: %08lx\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(cav, 0, 0.4f, NULL);
    ok(hr == S_OK, "CAV_SetChannelVolume failed: %08lx\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 0.6f, NULL);
    ok(hr == S_OK, "SAV_SetMasterVolume failed: %08lx\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, &vol);
    ok(hr == S_OK, "ASV_GetChannelVolume failed: %08lx\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "ASV_GetChannelVolume gave wrong volume: %f\n", vol);

    hr = IChannelAudioVolume_GetChannelVolume(cav, 0, &vol);
    ok(hr == S_OK, "CAV_GetChannelVolume failed: %08lx\n", hr);
    ok(fabsf(vol - 0.4f) < 0.05f, "CAV_GetChannelVolume gave wrong volume: %f\n", vol);

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "SAV_GetMasterVolume failed: %08lx\n", hr);
    ok(fabsf(vol - 0.6f) < 0.05f, "SAV_GetMasterVolume gave wrong volume: %f\n", vol);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac2);
    if(SUCCEEDED(hr)){
        IChannelAudioVolume *cav2;
        IAudioStreamVolume *asv2;

        hr = IAudioClient_Initialize(ac2, AUDCLNT_SHAREMODE_SHARED,
                AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, &session);
        ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

        hr = IAudioClient_GetService(ac2, &IID_IChannelAudioVolume, (void**)&cav2);
        ok(hr == S_OK, "GetService failed: %08lx\n", hr);

        hr = IAudioClient_GetService(ac2, &IID_IAudioStreamVolume, (void**)&asv2);
        ok(hr == S_OK, "GetService failed: %08lx\n", hr);

        hr = IChannelAudioVolume_GetChannelVolume(cav2, 0, &vol);
        ok(hr == S_OK, "CAV_GetChannelVolume failed: %08lx\n", hr);
        ok(fabsf(vol - 0.4f) < 0.05f, "CAV_GetChannelVolume gave wrong volume: %f\n", vol);

        hr = IAudioStreamVolume_GetChannelVolume(asv2, 0, &vol);
        ok(hr == S_OK, "ASV_GetChannelVolume failed: %08lx\n", hr);
        ok(vol == 1.f, "ASV_GetChannelVolume gave wrong volume: %f\n", vol);

        hr = IChannelAudioVolume_GetChannelCount(cav2, &nch);
        ok(hr == S_OK, "CAV_GetChannelCount failed: %08lx\n", hr);
        ok(nch == fmt->nChannels, "Got wrong channel count, expected %u: %u\n", fmt->nChannels, nch);

        hr = IAudioStreamVolume_GetChannelCount(asv2, &nch);
        ok(hr == S_OK, "ASV_GetChannelCount failed: %08lx\n", hr);
        ok(nch == fmt->nChannels, "Got wrong channel count, expected %u: %u\n", fmt->nChannels, nch);

        IAudioStreamVolume_Release(asv2);
        IChannelAudioVolume_Release(cav2);
        IAudioClient_Release(ac2);
    }else
        skip("Unable to open the same device twice. Skipping session volume control tests\n");

    hr = IChannelAudioVolume_SetChannelVolume(cav, 0, 1.f, NULL);
    ok(hr == S_OK, "CAV_SetChannelVolume failed: %08lx\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 1.f, NULL);
    ok(hr == S_OK, "SAV_SetMasterVolume failed: %08lx\n", hr);

    CoTaskMemFree(fmt);
    ISimpleAudioVolume_Release(sav);
    IChannelAudioVolume_Release(cav);
    IAudioStreamVolume_Release(asv);
    IAudioClient_Release(ac);
}

static void test_marshal(void)
{
    IStream *pStream;
    IAudioClient *ac, *acDest;
    IAudioCaptureClient *cc, *ccDest;
    WAVEFORMATEX *pwfx;
    HRESULT hr;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_IAudioCaptureClient, (void**)&cc);
    ok(hr == S_OK, "GetService failed: %08lx\n", hr);
    if(hr != S_OK) {
        IAudioClient_Release(ac);
        return;
    }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok(hr == S_OK, "CreateStreamOnHGlobal failed 0x%08lx\n", hr);

    /* marshal IAudioClient */

    hr = CoMarshalInterface(pStream, &IID_IAudioClient, (IUnknown*)ac, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == S_OK, "CoMarshalInterface IAudioClient failed 0x%08lx\n", hr);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IAudioClient, (void **)&acDest);
    ok(hr == S_OK, "CoUnmarshalInterface IAudioClient failed 0x%08lx\n", hr);
    if (hr == S_OK)
        IAudioClient_Release(acDest);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    /* marshal IAudioCaptureClient */

    hr = CoMarshalInterface(pStream, &IID_IAudioCaptureClient, (IUnknown*)cc, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == S_OK, "CoMarshalInterface IAudioCaptureClient failed 0x%08lx\n", hr);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IAudioCaptureClient, (void **)&ccDest);
    ok(hr == S_OK, "CoUnmarshalInterface IAudioCaptureClient failed 0x%08lx\n", hr);
    if (hr == S_OK)
        IAudioCaptureClient_Release(ccDest);

    IStream_Release(pStream);

    IAudioClient_Release(ac);
    IAudioCaptureClient_Release(cc);

}

static void test_render_loopback(void)
{
    IAudioCaptureClient *capture;
    IAudioRenderClient *render;
    WAVEFORMATEX *pwfx;
    IMMDevice *device;
    IAudioClient *ac;
    HRESULT hr;

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mme, eRender, eMultimedia, &device);
    ok(hr == S_OK || hr == E_NOTFOUND, "GetDefaultAudioEndpoint failed: %#lx.\n", hr);

    if (hr != S_OK || !dev)
    {
        if (hr == E_NOTFOUND)
            skip("No sound card available\n");
        else
            skip("GetDefaultAudioEndpoint returns 0x%08lx\n", hr);
        return;
    }

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER, NULL, (void**)&ac);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_LOOPBACK, 5000000, 0, pwfx, NULL);
    ok(hr == AUDCLNT_E_WRONG_ENDPOINT_TYPE, "got %#lx.\n", hr);
    IAudioClient_Release(ac);
    CoTaskMemFree(pwfx);

    hr = IMMDevice_Activate(device, &IID_IAudioClient, CLSCTX_INPROC_SERVER, NULL, (void**)&ac);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 5000000, 0, pwfx, NULL);
    todo_wine_if(hr == E_NOTIMPL) ok(hr == S_OK, "got %#lx.\n", hr);
    if (FAILED(hr))
        goto done;

    hr = IAudioClient_GetService(ac, &IID_IAudioRenderClient, (void **)&render);
    ok(hr == AUDCLNT_E_WRONG_ENDPOINT_TYPE, "got %#lx.\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IAudioCaptureClient, (void **)&capture);
    ok(hr == S_OK, "got %#lx.\n", hr);
    IAudioCaptureClient_Release(capture);

done:
    IAudioClient_Release(ac);
    IMMDevice_Release(device);
    CoTaskMemFree(pwfx);
}

START_TEST(capture)
{
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&mme);
    if (FAILED(hr))
    {
        skip("mmdevapi not available: 0x%08lx\n", hr);
        goto cleanup;
    }

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mme, eCapture, eMultimedia, &dev);
    ok(hr == S_OK || hr == E_NOTFOUND, "GetDefaultAudioEndpoint failed: 0x%08lx\n", hr);
    if (hr != S_OK || !dev)
    {
        if (hr == E_NOTFOUND)
            skip("No sound card available\n");
        else
            skip("GetDefaultAudioEndpoint returns 0x%08lx\n", hr);
        goto cleanup;
    }

    test_audioclient();
    test_formats(AUDCLNT_SHAREMODE_EXCLUSIVE);
    test_formats(AUDCLNT_SHAREMODE_SHARED);
    test_streamvolume();
    test_channelvolume();
    test_simplevolume();
    test_volume_dependence();
    test_marshal();
    test_render_loopback();

    IMMDevice_Release(dev);

cleanup:
    if (mme)
        IMMDeviceEnumerator_Release(mme);
    CoUninitialize();
}
