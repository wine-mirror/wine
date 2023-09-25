/*
 * Copyright 2023 Rémi Bernon for CodeWeavers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dmusic_private.h"
#include "soundfont.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

struct sample
{
    WSMPL head;
    WLOOP loops[];
};

C_ASSERT(sizeof(struct sample) == offsetof(struct sample, loops[0]));

struct wave
{
    IUnknown IUnknown_iface;
    LONG ref;

    struct sample *sample;
    WAVEFORMATEX *format;
    UINT data_size;
    void *data;
};

static inline struct wave *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct wave, IUnknown_iface);
}

static HRESULT WINAPI wave_QueryInterface(IUnknown *iface, REFIID riid, void **ret_iface)
{
    struct wave *This = impl_from_IUnknown(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *ret_iface = &This->IUnknown_iface;
        IUnknown_AddRef(&This->IUnknown_iface);
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
    *ret_iface = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI wave_AddRef(IUnknown *iface)
{
    struct wave *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI wave_Release(IUnknown *iface)
{
    struct wave *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        free(This->format);
        free(This->data);
        free(This->sample);
        free(This);
    }

    return ref;
}

static const IUnknownVtbl unknown_vtbl =
{
    wave_QueryInterface,
    wave_AddRef,
    wave_Release,
};

static HRESULT wave_create(IUnknown **ret_iface)
{
    struct wave *obj;

    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->IUnknown_iface.lpVtbl = &unknown_vtbl;
    obj->ref = 1;

    *ret_iface = &obj->IUnknown_iface;
    return S_OK;
}

static HRESULT parse_wsmp_chunk(struct wave *This, IStream *stream, struct chunk_entry *chunk)
{
    struct sample *sample;
    WSMPL wsmpl;
    HRESULT hr;
    UINT size;

    if (chunk->size < sizeof(wsmpl)) return E_INVALIDARG;
    if (FAILED(hr = stream_read(stream, &wsmpl, sizeof(wsmpl)))) return hr;
    if (chunk->size != wsmpl.cbSize + sizeof(WLOOP) * wsmpl.cSampleLoops) return E_INVALIDARG;
    if (wsmpl.cbSize != sizeof(wsmpl)) return E_INVALIDARG;
    if (wsmpl.cSampleLoops > 1) FIXME("Not implemented: found more than one wave loop\n");

    size = offsetof(struct sample, loops[wsmpl.cSampleLoops]);
    if (!(sample = malloc(size))) return E_OUTOFMEMORY;
    sample->head = wsmpl;

    size = sizeof(WLOOP) * wsmpl.cSampleLoops;
    if (FAILED(hr = stream_read(stream, sample->loops, size))) free(sample);
    else This->sample = sample;

    return hr;
}

static HRESULT parse_wave_chunk(struct wave *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case mmioFOURCC('f','m','t',' '):
            if (!(This->format = malloc(chunk.size))) return E_OUTOFMEMORY;
            hr = stream_chunk_get_data(stream, &chunk, This->format, chunk.size);
            break;

        case mmioFOURCC('d','a','t','a'):
            if (!(This->data = malloc(chunk.size))) return E_OUTOFMEMORY;
            hr = stream_chunk_get_data(stream, &chunk, This->data, chunk.size);
            if (SUCCEEDED(hr)) This->data_size = chunk.size;
            break;

        case FOURCC_WSMP:
            hr = parse_wsmp_chunk(This, stream, &chunk);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

HRESULT wave_create_from_chunk(IStream *stream, struct chunk_entry *parent, IUnknown **ret_iface)
{
    struct wave *This;
    IUnknown *iface;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", stream, parent, ret_iface);

    if (FAILED(hr = wave_create(&iface))) return hr;
    This = impl_from_IUnknown(iface);

    if (FAILED(hr = parse_wave_chunk(This, stream, parent)))
    {
        IUnknown_Release(iface);
        return DMUS_E_UNSUPPORTED_STREAM;
    }

    if (TRACE_ON(dmusic))
    {
        UINT i;

        TRACE("*** Created DirectMusicWave %p\n", This);
        TRACE(" - format: %p\n", This->format);
        if (This->format)
        {
            TRACE("    - wFormatTag: %u\n", This->format->wFormatTag);
            TRACE("    - nChannels: %u\n", This->format->nChannels);
            TRACE("    - nSamplesPerSec: %lu\n", This->format->nSamplesPerSec);
            TRACE("    - nAvgBytesPerSec: %lu\n", This->format->nAvgBytesPerSec);
            TRACE("    - nBlockAlign: %u\n", This->format->nBlockAlign);
            TRACE("    - wBitsPerSample: %u\n", This->format->wBitsPerSample);
            TRACE("    - cbSize: %u\n", This->format->cbSize);
        }
        if (This->sample)
        {
            TRACE(" - sample: {size: %lu, unity_note: %u, fine_tune: %d, attenuation: %ld, options: %#lx, loops: %lu}\n",
                    This->sample->head.cbSize, This->sample->head.usUnityNote,
                    This->sample->head.sFineTune, This->sample->head.lAttenuation,
                    This->sample->head.fulOptions, This->sample->head.cSampleLoops);
            for (i = 0; i < This->sample->head.cSampleLoops; i++)
                TRACE(" - loops[%u]: {size: %lu, type: %lu, start: %lu, length: %lu}\n", i,
                        This->sample->loops[i].cbSize, This->sample->loops[i].ulType,
                        This->sample->loops[i].ulStart, This->sample->loops[i].ulLength);
        }
    }

    *ret_iface = iface;
    return S_OK;
}

HRESULT wave_create_from_soundfont(struct soundfont *soundfont, UINT index, IUnknown **ret_iface)
{
    struct sf_sample *sf_sample = soundfont->shdr + index;
    struct sample *sample = NULL;
    WAVEFORMATEX *format = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    UINT data_size, offset;
    struct wave *This;
    void *data = NULL;
    IUnknown *iface;

    TRACE("(%p, %u, %p)\n", soundfont, index, ret_iface);

    if (sf_sample->sample_link) FIXME("Stereo sample not supported\n");

    if (!(format = calloc(1, sizeof(*format)))) goto failed;
    format->wFormatTag = WAVE_FORMAT_PCM;
    format->nChannels = 1;
    format->wBitsPerSample = 16;
    format->nSamplesPerSec = sf_sample->sample_rate;
    format->nBlockAlign = format->wBitsPerSample * format->nChannels / 8;
    format->nAvgBytesPerSec = format->nBlockAlign * format->nSamplesPerSec;

    if (!(sample = calloc(1, offsetof(struct sample, loops[1])))) goto failed;
    sample->head.cbSize = sizeof(sample->head);
    sample->head.cSampleLoops = 1;
    sample->loops[0].ulStart = sf_sample->start_loop - sf_sample->start;
    sample->loops[0].ulLength = sf_sample->end_loop - sf_sample->start_loop;

    data_size = sf_sample->end - sf_sample->start;
    if (!(data = malloc(data_size * format->nBlockAlign))) goto failed;
    offset = sf_sample->start * format->nBlockAlign / format->nChannels;
    memcpy(data, soundfont->sdta + offset, data_size);

    if (FAILED(hr = wave_create(&iface))) goto failed;

    This = impl_from_IUnknown(iface);
    This->format = format;
    This->sample = sample;
    This->data_size = data_size;
    This->data = data;

    if (TRACE_ON(dmusic))
    {
        UINT i;

        TRACE("*** Created DirectMusicWave %p\n", This);
        TRACE(" - format: %p\n", This->format);
        if (This->format)
        {
            TRACE("    - wFormatTag: %u\n", This->format->wFormatTag);
            TRACE("    - nChannels: %u\n", This->format->nChannels);
            TRACE("    - nSamplesPerSec: %lu\n", This->format->nSamplesPerSec);
            TRACE("    - nAvgBytesPerSec: %lu\n", This->format->nAvgBytesPerSec);
            TRACE("    - nBlockAlign: %u\n", This->format->nBlockAlign);
            TRACE("    - wBitsPerSample: %u\n", This->format->wBitsPerSample);
            TRACE("    - cbSize: %u\n", This->format->cbSize);
        }

        TRACE(" - sample: {size: %lu, unity_note: %u, fine_tune: %d, attenuation: %ld, options: %#lx, loops: %lu}\n",
                This->sample->head.cbSize, This->sample->head.usUnityNote,
                This->sample->head.sFineTune, This->sample->head.lAttenuation,
                This->sample->head.fulOptions, This->sample->head.cSampleLoops);
        for (i = 0; i < This->sample->head.cSampleLoops; i++)
            TRACE(" - loops[%u]: {size: %lu, type: %lu, start: %lu, length: %lu}\n", i,
                    This->sample->loops[i].cbSize, This->sample->loops[i].ulType,
                    This->sample->loops[i].ulStart, This->sample->loops[i].ulLength);
    }

    *ret_iface = iface;
    return S_OK;

failed:
    free(data);
    free(sample);
    free(format);
    return hr;
}

HRESULT wave_download_to_port(IUnknown *iface, IDirectMusicPortDownload *port, DWORD *id)
{
    struct download_buffer
    {
        DMUS_DOWNLOADINFO info;
        ULONG offsets[2];
        DMUS_WAVE wave;
        DMUS_WAVEDATA data;
    } *buffer;

    struct wave *This = impl_from_IUnknown(iface);
    DWORD size = offsetof(struct download_buffer, data.byData[This->data_size]);
    IDirectMusicDownload *download;
    HRESULT hr;

    if (FAILED(hr = IDirectMusicPortDownload_AllocateBuffer(port, size, &download))) return hr;

    if (SUCCEEDED(hr = IDirectMusicDownload_GetBuffer(download, (void **)&buffer, &size))
            && SUCCEEDED(hr = IDirectMusicPortDownload_GetDLId(port, &buffer->info.dwDLId, 1)))
    {
        buffer->info.dwDLType = DMUS_DOWNLOADINFO_WAVE;
        buffer->info.dwNumOffsetTableEntries = 2;
        buffer->info.cbSize = size;

        buffer->offsets[0] = offsetof(struct download_buffer, wave);
        buffer->offsets[1] = offsetof(struct download_buffer, data);

        buffer->wave.WaveformatEx = *This->format;
        buffer->wave.ulWaveDataIdx = 1;
        buffer->wave.ulCopyrightIdx = 0;
        buffer->wave.ulFirstExtCkIdx = 0;

        buffer->data.cbSize = This->data_size;
        memcpy(buffer->data.byData, This->data, This->data_size);

        if (SUCCEEDED(hr = IDirectMusicPortDownload_Download(port, download))) *id = buffer->info.dwDLId;
        else WARN("Failed to download wave to port, hr %#lx\n", hr);
    }

    IDirectMusicDownload_Release(download);
    return hr;
}

HRESULT wave_download_to_dsound(IUnknown *iface, IDirectSound *dsound, IDirectSoundBuffer **ret_iface)
{
    struct wave *This = impl_from_IUnknown(iface);
    DSBUFFERDESC desc =
    {
        .dwSize = sizeof(desc),
        .dwBufferBytes = This->data_size,
        .lpwfxFormat = This->format,
    };
    IDirectSoundBuffer *buffer;
    HRESULT hr;
    void *data;
    DWORD size;

    TRACE("%p, %p, %p\n", This, dsound, ret_iface);

    if (FAILED(hr = IDirectSound_CreateSoundBuffer(dsound, &desc, &buffer, NULL)))
    {
        WARN("Failed to create direct sound buffer, hr %#lx\n", hr);
        return hr;
    }

    if (SUCCEEDED(hr = IDirectSoundBuffer_Lock(buffer, 0, This->data_size, &data, &size, NULL, 0, 0)))
    {
        memcpy(data, This->data, This->data_size);
        hr = IDirectSoundBuffer_Unlock(buffer, data, This->data_size, NULL, 0);
    }

    if (FAILED(hr))
    {
        WARN("Failed to download wave to dsound, hr %#lx\n", hr);
        IDirectSoundBuffer_Release(buffer);
        return hr;
    }

    *ret_iface = buffer;
    return S_OK;
}

HRESULT wave_get_duration(IUnknown *iface, REFERENCE_TIME *duration)
{
    struct wave *This = impl_from_IUnknown(iface);
    *duration = (REFERENCE_TIME)This->data_size * 10000000 / This->format->nAvgBytesPerSec;
    return S_OK;
}
