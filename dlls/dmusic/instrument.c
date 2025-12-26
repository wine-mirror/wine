/*
 * IDirectMusicInstrument Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
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

#include <math.h>

#include "dmusic_private.h"
#include "soundfont.h"
#include "dls2.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

static const GUID IID_IDirectMusicInstrumentPRIVATE = { 0xbcb20080, 0xa40c, 0x11d1, { 0x86, 0xbc, 0x00, 0xc0, 0x4f, 0xbf, 0x8f, 0xef } };

struct downloaded_wave
{
    struct list entry;

    DWORD index;
    DWORD id;
};

#define CONN_SRC_CC2  0x0082
#define CONN_SRC_RPN0 0x0100

#define CONN_TRN_BIPOLAR (1<<4)
#define CONN_TRN_INVERT  (1<<5)

#define CONN_TRANSFORM(src, ctrl, dst) (((src) & 0x3f) << 10) | (((ctrl) & 0x3f) << 4) | ((dst) & 0xf)

struct articulation
{
    struct list entry;
    CONNECTIONLIST list;
    CONNECTION connections[];
};

C_ASSERT(sizeof(struct articulation) == offsetof(struct articulation, connections[0]));

struct region
{
    struct list entry;
    struct list articulations;
    RGNHEADER header;
    WAVELINK wave_link;
    WSMPL wave_sample;
    WLOOP wave_loop;
};

static void region_destroy(struct region *region)
{
    struct articulation *articulation, *next;

    LIST_FOR_EACH_ENTRY_SAFE(articulation, next, &region->articulations, struct articulation, entry)
    {
        list_remove(&articulation->entry);
        free(articulation);
    }

    free(region);
}

struct instrument
{
    IDirectMusicInstrument IDirectMusicInstrument_iface;
    IDirectMusicDownloadedInstrument IDirectMusicDownloadedInstrument_iface;
    LONG ref;

    INSTHEADER header;
    IDirectMusicDownload *download;
    struct collection *collection;
    struct list downloaded_waves;
    struct list articulations;
    struct list regions;
};

static inline struct instrument *impl_from_IDirectMusicInstrument(IDirectMusicInstrument *iface)
{
    return CONTAINING_RECORD(iface, struct instrument, IDirectMusicInstrument_iface);
}

static HRESULT WINAPI instrument_QueryInterface(LPDIRECTMUSICINSTRUMENT iface, REFIID riid, LPVOID *ret_iface)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectMusicInstrument))
    {
        *ret_iface = iface;
        IDirectMusicInstrument_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IDirectMusicInstrumentPRIVATE))
    {
        /* it seems to me that this interface is only basic IUnknown, without any
         * other inherited functions... *sigh* this is the worst scenario, since it means
         * that whoever calls it knows the layout of original implementation table and therefore
         * tries to get data by direct access... expect crashes
         */
        FIXME("*sigh*... requested private/unspecified interface\n");

        *ret_iface = iface;
        IDirectMusicInstrument_AddRef(iface);
        return S_OK;
    }

    WARN("(%p)->(%s, %p): not found\n", iface, debugstr_dmguid(riid), ret_iface);

    return E_NOINTERFACE;
}

static ULONG WINAPI instrument_AddRef(LPDIRECTMUSICINSTRUMENT iface)
{
    struct instrument *This = impl_from_IDirectMusicInstrument(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI instrument_Release(LPDIRECTMUSICINSTRUMENT iface)
{
    struct instrument *This = impl_from_IDirectMusicInstrument(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", iface, ref);

    if (!ref)
    {
        struct articulation *articulation, *next_articulation;
        struct region *region, *next_region;

        LIST_FOR_EACH_ENTRY_SAFE(articulation, next_articulation, &This->articulations, struct articulation, entry)
        {
            list_remove(&articulation->entry);
            free(articulation);
        }

        LIST_FOR_EACH_ENTRY_SAFE(region, next_region, &This->regions, struct region, entry)
        {
            list_remove(&region->entry);
            region_destroy(region);
        }

        collection_internal_release(This->collection);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI instrument_GetPatch(LPDIRECTMUSICINSTRUMENT iface, DWORD* pdwPatch)
{
    struct instrument *This = impl_from_IDirectMusicInstrument(iface);

    TRACE("(%p)->(%p)\n", This, pdwPatch);

    *pdwPatch = MIDILOCALE2Patch(&This->header.Locale);

    return S_OK;
}

static HRESULT WINAPI instrument_SetPatch(LPDIRECTMUSICINSTRUMENT iface, DWORD dwPatch)
{
    struct instrument *This = impl_from_IDirectMusicInstrument(iface);

    TRACE("(%p, %ld): stub\n", This, dwPatch);

    Patch2MIDILOCALE(dwPatch, &This->header.Locale);

    return S_OK;
}

static const IDirectMusicInstrumentVtbl instrument_vtbl =
{
    instrument_QueryInterface,
    instrument_AddRef,
    instrument_Release,
    instrument_GetPatch,
    instrument_SetPatch,
};

static inline struct instrument* impl_from_IDirectMusicDownloadedInstrument(IDirectMusicDownloadedInstrument *iface)
{
    return CONTAINING_RECORD(iface, struct instrument, IDirectMusicDownloadedInstrument_iface);
}

static HRESULT WINAPI downloaded_instrument_QueryInterface(IDirectMusicDownloadedInstrument *iface, REFIID riid, VOID **ret_iface)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicDownloadedInstrument))
    {
        IDirectMusicDownloadedInstrument_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", iface, debugstr_dmguid(riid), ret_iface);

    return E_NOINTERFACE;
}

static ULONG WINAPI downloaded_instrument_AddRef(IDirectMusicDownloadedInstrument *iface)
{
    struct instrument *This = impl_from_IDirectMusicDownloadedInstrument(iface);
    return IDirectMusicInstrument_AddRef(&This->IDirectMusicInstrument_iface);
}

static ULONG WINAPI downloaded_instrument_Release(IDirectMusicDownloadedInstrument *iface)
{
    struct instrument *This = impl_from_IDirectMusicDownloadedInstrument(iface);
    return IDirectMusicInstrument_Release(&This->IDirectMusicInstrument_iface);
}

static const IDirectMusicDownloadedInstrumentVtbl downloaded_instrument_vtbl =
{
    downloaded_instrument_QueryInterface,
    downloaded_instrument_AddRef,
    downloaded_instrument_Release,
};

static HRESULT instrument_create(struct collection *collection, IDirectMusicInstrument **ret_iface)
{
    struct instrument *instrument;

    *ret_iface = NULL;
    if (!(instrument = calloc(1, sizeof(*instrument)))) return E_OUTOFMEMORY;
    instrument->IDirectMusicInstrument_iface.lpVtbl = &instrument_vtbl;
    instrument->IDirectMusicDownloadedInstrument_iface.lpVtbl = &downloaded_instrument_vtbl;
    instrument->ref = 1;
    collection_internal_addref((instrument->collection = collection));
    list_init(&instrument->downloaded_waves);
    list_init(&instrument->articulations);
    list_init(&instrument->regions);

    TRACE("Created DirectMusicInstrument %p\n", instrument);
    *ret_iface = &instrument->IDirectMusicInstrument_iface;
    return S_OK;
}

static HRESULT parse_art1_chunk(struct instrument *This, IStream *stream, struct chunk_entry *chunk,
        struct list *articulations)
{
    struct articulation *articulation;
    CONNECTIONLIST list;
    HRESULT hr;
    UINT size;

    if (chunk->size < sizeof(list)) return E_INVALIDARG;
    if (FAILED(hr = stream_read(stream, &list, sizeof(list)))) return hr;
    if (chunk->size != list.cbSize + sizeof(CONNECTION) * list.cConnections) return E_INVALIDARG;
    if (list.cbSize != sizeof(list)) return E_INVALIDARG;

    size = offsetof(struct articulation, connections[list.cConnections]);
    if (!(articulation = malloc(size))) return E_OUTOFMEMORY;
    articulation->list = list;

    size = sizeof(CONNECTION) * list.cConnections;
    if (FAILED(hr = stream_read(stream, articulation->connections, size))) free(articulation);
    else list_add_tail(articulations, &articulation->entry);

    return hr;
}

static HRESULT parse_lart_list(struct instrument *This, IStream *stream, struct chunk_entry *parent,
        struct list *articulations)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case FOURCC_ART1:
            hr = parse_art1_chunk(This, stream, &chunk, articulations);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

static HRESULT parse_rgn_chunk(struct instrument *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    struct region *region;
    HRESULT hr;

    if (!(region = malloc(sizeof(*region)))) return E_OUTOFMEMORY;
    list_init(&region->articulations);

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case FOURCC_RGNH:
            hr = stream_chunk_get_data(stream, &chunk, &region->header, sizeof(region->header));
            break;

        case FOURCC_WSMP:
            if (chunk.size < sizeof(region->wave_sample)) hr = E_INVALIDARG;
            else hr = stream_read(stream, &region->wave_sample, sizeof(region->wave_sample));
            if (SUCCEEDED(hr) && region->wave_sample.cSampleLoops)
            {
                if (region->wave_sample.cSampleLoops > 1) FIXME("More than one wave loop is not implemented\n");
                if (chunk.size != sizeof(WSMPL) + region->wave_sample.cSampleLoops * sizeof(WLOOP)) hr = E_INVALIDARG;
                else hr = stream_read(stream, &region->wave_loop, sizeof(region->wave_loop));
            }
            break;

        case FOURCC_WLNK:
            hr = stream_chunk_get_data(stream, &chunk, &region->wave_link, sizeof(region->wave_link));
            break;

        case MAKE_IDTYPE(FOURCC_LIST, FOURCC_LART):
            hr = parse_lart_list(This, stream, &chunk, &region->articulations);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    if (FAILED(hr)) region_destroy(region);
    else list_add_tail(&This->regions, &region->entry);

    return hr;
}

static HRESULT parse_lrgn_list(struct instrument *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case MAKE_IDTYPE(FOURCC_LIST, FOURCC_RGN):
            hr = parse_rgn_chunk(This, stream, &chunk);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

static HRESULT parse_ins_chunk(struct instrument *This, IStream *stream, struct chunk_entry *parent,
        DMUS_OBJECTDESC *desc)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    if (FAILED(hr = dmobj_parsedescriptor(stream, parent, desc, DMUS_OBJ_NAME_INFO|DMUS_OBJ_GUID_DLID))
            || FAILED(hr = stream_reset_chunk_data(stream, parent)))
        return hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case FOURCC_INSH:
            hr = stream_chunk_get_data(stream, &chunk, &This->header, sizeof(This->header));
            break;

        case FOURCC_DLID:
        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_INFO_LIST):
            /* already parsed by dmobj_parsedescriptor */
            break;

        case MAKE_IDTYPE(FOURCC_LIST, FOURCC_LRGN):
            hr = parse_lrgn_list(This, stream, &chunk);
            break;

        case MAKE_IDTYPE(FOURCC_LIST, FOURCC_LART):
            hr = parse_lart_list(This, stream, &chunk, &This->articulations);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

HRESULT instrument_create_from_chunk(IStream *stream, struct chunk_entry *parent,
        struct collection *collection, DMUS_OBJECTDESC *desc, IDirectMusicInstrument **ret_iface)
{
    IDirectMusicInstrument *iface;
    struct instrument *This;
    HRESULT hr;

    TRACE("(%p, %p, %p, %p, %p)\n", stream, parent, collection, desc, ret_iface);

    if (FAILED(hr = instrument_create(collection, &iface))) return hr;
    This = impl_from_IDirectMusicInstrument(iface);

    if (FAILED(hr = parse_ins_chunk(This, stream, parent, desc)))
    {
        IDirectMusicInstrument_Release(iface);
        return DMUS_E_UNSUPPORTED_STREAM;
    }

    if (TRACE_ON(dmusic))
    {
        struct region *region;
        UINT i;

        TRACE("Created DirectMusicInstrument %p\n", This);
        TRACE(" - header:\n");
        TRACE("    - regions: %ld\n", This->header.cRegions);
        TRACE("    - locale: {bank: %#lx, instrument: %#lx} (patch %#lx)\n",
                This->header.Locale.ulBank, This->header.Locale.ulInstrument,
                MIDILOCALE2Patch(&This->header.Locale));
        if (desc->dwValidData & DMUS_OBJ_OBJECT) TRACE(" - guid: %s\n", debugstr_dmguid(&desc->guidObject));
        if (desc->dwValidData & DMUS_OBJ_NAME) TRACE(" - name: %s\n", debugstr_w(desc->wszName));

        TRACE(" - regions:\n");
        LIST_FOR_EACH_ENTRY(region, &This->regions, struct region, entry)
        {
            TRACE("   - region:\n");
            TRACE("     - header: {key: %u - %u, vel: %u - %u, options: %#x, group: %#x}\n",
                    region->header.RangeKey.usLow, region->header.RangeKey.usHigh,
                    region->header.RangeVelocity.usLow, region->header.RangeVelocity.usHigh,
                    region->header.fusOptions, region->header.usKeyGroup);
            TRACE("     - wave_link: {options: %#x, group: %u, channel: %lu, index: %lu}\n",
                    region->wave_link.fusOptions, region->wave_link.usPhaseGroup,
                    region->wave_link.ulChannel, region->wave_link.ulTableIndex);
            TRACE("     - wave_sample: {size: %lu, unity_note: %u, fine_tune: %d, attenuation: %ld, options: %#lx, loops: %lu}\n",
                    region->wave_sample.cbSize, region->wave_sample.usUnityNote,
                    region->wave_sample.sFineTune, region->wave_sample.lAttenuation,
                    region->wave_sample.fulOptions, region->wave_sample.cSampleLoops);
            for (i = 0; i < region->wave_sample.cSampleLoops; i++)
                TRACE("     - wave_loop[%u]: {size: %lu, type: %lu, start: %lu, length: %lu}\n", i,
                        region->wave_loop.cbSize, region->wave_loop.ulType,
                        region->wave_loop.ulStart, region->wave_loop.ulLength);
        }
    }

    *ret_iface = iface;
    return S_OK;
}

struct sf_generators
{
    union sf_amount amount[SF_GEN_END_OPER];
};

static const struct sf_generators SF_DEFAULT_GENERATORS =
{
    .amount =
    {
        [SF_GEN_INITIAL_FILTER_FC] = {.value = 13500},
        [SF_GEN_DELAY_MOD_LFO] = {.value = -12000},
        [SF_GEN_DELAY_VIB_LFO] = {.value = -12000},
        [SF_GEN_DELAY_MOD_ENV] = {.value = -12000},
        [SF_GEN_ATTACK_MOD_ENV] = {.value = -12000},
        [SF_GEN_HOLD_MOD_ENV] = {.value = -12000},
        [SF_GEN_DECAY_MOD_ENV] = {.value = -12000},
        [SF_GEN_RELEASE_MOD_ENV] = {.value = -12000},
        [SF_GEN_DELAY_VOL_ENV] = {.value = -12000},
        [SF_GEN_ATTACK_VOL_ENV] = {.value = -12000},
        [SF_GEN_HOLD_VOL_ENV] = {.value = -12000},
        [SF_GEN_DECAY_VOL_ENV] = {.value = -12000},
        [SF_GEN_RELEASE_VOL_ENV] = {.value = -12000},
        [SF_GEN_KEY_RANGE] = {.range = {.low = 0, .high = 127}},
        [SF_GEN_VEL_RANGE] = {.range = {.low = 0, .high = 127}},
        [SF_GEN_KEYNUM] = {.value = -1},
        [SF_GEN_VELOCITY] = {.value = -1},
        [SF_GEN_SCALE_TUNING] = {.value = 100},
        [SF_GEN_OVERRIDING_ROOT_KEY] = {.value = -1},
    }
};

static const struct sf_mod SF_DEFAULT_MODULATORS[] =
{
    {
        .src_mod = SF_MOD_CTRL_GEN_VELOCITY | SF_MOD_DIR_DECREASING | SF_MOD_SRC_CONCAVE,
        .dest_gen = SF_GEN_INITIAL_ATTENUATION,
        .amount = 960,
    },
    {
        .src_mod = SF_MOD_CTRL_GEN_VELOCITY | SF_MOD_DIR_DECREASING,
        .dest_gen = SF_GEN_INITIAL_FILTER_FC,
        .amount = -2400,
        .amount_src_mod = SF_MOD_CTRL_GEN_VELOCITY | SF_MOD_DIR_DECREASING | SF_MOD_SRC_SWITCH,
    },
    {
        .src_mod = SF_MOD_CTRL_GEN_CHAN_PRESSURE,
        .dest_gen = SF_GEN_VIB_LFO_TO_PITCH,
        .amount = 50,
    },
    {
        .src_mod = 1 | SF_MOD_CTRL_MIDI,
        .dest_gen = SF_GEN_VIB_LFO_TO_PITCH,
        .amount = 50,
    },
    {
        .src_mod = 7 | SF_MOD_CTRL_MIDI | SF_MOD_DIR_DECREASING | SF_MOD_SRC_CONCAVE,
        .dest_gen = SF_GEN_INITIAL_ATTENUATION,
        .amount = 960,
    },
    {
        .src_mod = 10 | SF_MOD_CTRL_MIDI | SF_MOD_POL_BIPOLAR,
        .dest_gen = SF_GEN_PAN,
        .amount = 1000,
    },
    {
        .src_mod = 11 | SF_MOD_CTRL_MIDI | SF_MOD_DIR_DECREASING | SF_MOD_SRC_CONCAVE,
        .dest_gen = SF_GEN_INITIAL_ATTENUATION,
        .amount = 960,
    },
    {
        .src_mod = 91 | SF_MOD_CTRL_MIDI,
        .dest_gen = SF_GEN_REVERB_EFFECTS_SEND,
        .amount = 200,
    },
    {
        .src_mod = 93 | SF_MOD_CTRL_MIDI,
        .dest_gen = SF_GEN_CHORUS_EFFECTS_SEND,
        .amount = 200,
    },
};

struct sf_modulators
{
    UINT count;
    UINT capacity;
    struct sf_mod *mods;
};

static void copy_modulators(struct sf_modulators *dst, const struct sf_mod *mods, UINT count)
{
    if (dst->capacity < count)
    {
        struct sf_mod *mods;

        if (!(mods = malloc(count * sizeof(struct sf_mod))))
            return;

        free(dst->mods);

        dst->mods = mods;
        dst->capacity = count;
    }

    memcpy(dst->mods, mods, count * sizeof(struct sf_mod));
    dst->count = count;
}

static struct sf_mod *add_modulator(struct sf_modulators *modulators, struct sf_mod *mod)
{
    struct sf_mod *new_mod;

    if (modulators->capacity < modulators->count + 1)
    {
        UINT new_capacity = max(modulators->count + 1, modulators->capacity * 2);
        struct sf_mod *mods;

        if (!(mods = realloc(modulators->mods, new_capacity * sizeof(struct sf_mod))))
            return NULL;
        modulators->capacity = new_capacity;
        modulators->mods = mods;
    }

    new_mod = &modulators->mods[modulators->count];
    *new_mod = *mod;
    ++modulators->count;
    return new_mod;
}

static struct sf_mod *find_modulator(struct sf_modulators *modulators, struct sf_mod *mod)
{
    UINT i;
    for (i = 0; i < modulators->count; ++i)
    {
        if (modulators->mods[i].dest_gen != mod->dest_gen)
            continue;
        if (modulators->mods[i].src_mod != mod->src_mod)
            continue;
        if (modulators->mods[i].amount_src_mod != mod->amount_src_mod)
            continue;
        if (modulators->mods[i].transform != mod->transform)
            continue;
        return &modulators->mods[i];
    }
    return NULL;
}

static BOOL parse_soundfont_generators(struct soundfont *soundfont, UINT index, BOOL preset,
        struct sf_generators *generators)
{
    struct sf_bag *bag = (preset ? soundfont->pbag : soundfont->ibag) + index;
    struct sf_gen *gen, *gens = preset ? soundfont->pgen : soundfont->igen;

    for (gen = gens + bag->gen_ndx; gen < gens + (bag + 1)->gen_ndx; gen++)
    {
        switch (gen->oper)
        {
        case SF_GEN_START_ADDRS_OFFSET:
        case SF_GEN_END_ADDRS_OFFSET:
        case SF_GEN_STARTLOOP_ADDRS_OFFSET:
        case SF_GEN_ENDLOOP_ADDRS_OFFSET:
        case SF_GEN_START_ADDRS_COARSE_OFFSET:
        case SF_GEN_END_ADDRS_COARSE_OFFSET:
        case SF_GEN_STARTLOOP_ADDRS_COARSE_OFFSET:
        case SF_GEN_KEYNUM:
        case SF_GEN_VELOCITY:
        case SF_GEN_ENDLOOP_ADDRS_COARSE_OFFSET:
        case SF_GEN_SAMPLE_MODES:
        case SF_GEN_EXCLUSIVE_CLASS:
        case SF_GEN_OVERRIDING_ROOT_KEY:
            if (!preset) generators->amount[gen->oper] = gen->amount;
            else WARN("Ignoring invalid preset generator %s\n", debugstr_sf_gen(gen));
            break;

        case SF_GEN_INSTRUMENT:
            if (preset) generators->amount[gen->oper] = gen->amount;
            else WARN("Ignoring invalid instrument generator %s\n", debugstr_sf_gen(gen));
            /* should always be the last generator */
            return FALSE;

        case SF_GEN_SAMPLE_ID:
            if (!preset) generators->amount[gen->oper] = gen->amount;
            else WARN("Ignoring invalid preset generator %s\n", debugstr_sf_gen(gen));
            /* should always be the last generator */
            return FALSE;

        default:
            generators->amount[gen->oper] = gen->amount;
            break;
        }
    }

    return TRUE;
}

static void parse_soundfont_modulators(struct soundfont *soundfont, UINT index, BOOL preset,
        struct sf_modulators *modulators)
{
    struct sf_bag *bag = (preset ? soundfont->pbag : soundfont->ibag) + index;
    struct sf_mod *mod, *mods = preset ? soundfont->pmod : soundfont->imod;
    struct sf_mod *modulator;

    for (mod = mods + bag->mod_ndx; mod < mods + (bag + 1)->mod_ndx; mod++)
    {
        if ((modulator = find_modulator(modulators, mod)))
        {
            *modulator = *mod;
            continue;
        }
        if (!add_modulator(modulators, mod))
            return;
    }
}

static const USHORT gen_to_conn_dst[SF_GEN_END_OPER] =
{
    [SF_GEN_MOD_LFO_TO_PITCH] = CONN_DST_PITCH,
    [SF_GEN_VIB_LFO_TO_PITCH] = CONN_DST_PITCH,
    [SF_GEN_MOD_ENV_TO_PITCH] = CONN_DST_PITCH,
    [SF_GEN_INITIAL_FILTER_FC] = CONN_DST_FILTER_CUTOFF,
    [SF_GEN_INITIAL_FILTER_Q] = CONN_DST_FILTER_Q,
    [SF_GEN_MOD_LFO_TO_FILTER_FC] = CONN_DST_FILTER_CUTOFF,
    [SF_GEN_MOD_ENV_TO_FILTER_FC] = CONN_DST_FILTER_CUTOFF,
    [SF_GEN_MOD_LFO_TO_VOLUME] = CONN_DST_GAIN,
    [SF_GEN_CHORUS_EFFECTS_SEND] = CONN_DST_CHORUS,
    [SF_GEN_REVERB_EFFECTS_SEND] = CONN_DST_REVERB,
    [SF_GEN_PAN] = CONN_DST_PAN,
    [SF_GEN_DELAY_MOD_LFO] = CONN_DST_LFO_STARTDELAY,
    [SF_GEN_FREQ_MOD_LFO] = CONN_DST_LFO_FREQUENCY,
    [SF_GEN_DELAY_VIB_LFO] = CONN_DST_VIB_STARTDELAY,
    [SF_GEN_FREQ_VIB_LFO] = CONN_DST_VIB_FREQUENCY,
    [SF_GEN_DELAY_MOD_ENV] = CONN_DST_EG2_DELAYTIME,
    [SF_GEN_ATTACK_MOD_ENV] = CONN_DST_EG2_ATTACKTIME,
    [SF_GEN_HOLD_MOD_ENV] = CONN_DST_EG2_HOLDTIME,
    [SF_GEN_DECAY_MOD_ENV] = CONN_DST_EG2_DECAYTIME,
    [SF_GEN_SUSTAIN_MOD_ENV] = CONN_DST_EG2_SUSTAINLEVEL,
    [SF_GEN_RELEASE_MOD_ENV] = CONN_DST_EG2_RELEASETIME,
    [SF_GEN_KEYNUM_TO_MOD_ENV_HOLD] = CONN_DST_NONE,
    [SF_GEN_KEYNUM_TO_MOD_ENV_DECAY] = CONN_DST_NONE,
    [SF_GEN_DELAY_VOL_ENV] = CONN_DST_EG1_DELAYTIME,
    [SF_GEN_ATTACK_VOL_ENV] = CONN_DST_EG1_ATTACKTIME,
    [SF_GEN_HOLD_VOL_ENV] = CONN_DST_EG1_HOLDTIME,
    [SF_GEN_DECAY_VOL_ENV] = CONN_DST_EG1_DECAYTIME,
    [SF_GEN_SUSTAIN_VOL_ENV] = CONN_DST_EG1_SUSTAINLEVEL,
    [SF_GEN_RELEASE_VOL_ENV] = CONN_DST_EG1_RELEASETIME,
    [SF_GEN_KEYNUM_TO_VOL_ENV_HOLD] = CONN_DST_EG1_HOLDTIME,
    [SF_GEN_KEYNUM_TO_VOL_ENV_DECAY] = CONN_DST_EG1_DECAYTIME,
    [SF_GEN_INITIAL_ATTENUATION] = CONN_DST_GAIN,
};

static const USHORT gen_to_conn_src[SF_GEN_END_OPER] =
{
    [SF_GEN_MOD_LFO_TO_PITCH] = CONN_SRC_LFO,
    [SF_GEN_VIB_LFO_TO_PITCH] = CONN_SRC_VIBRATO,
    [SF_GEN_MOD_ENV_TO_PITCH] = CONN_SRC_EG2,
    [SF_GEN_MOD_LFO_TO_FILTER_FC] = CONN_SRC_LFO,
    [SF_GEN_MOD_ENV_TO_FILTER_FC] = CONN_SRC_EG2,
    [SF_GEN_MOD_LFO_TO_VOLUME] = CONN_SRC_LFO,
    [SF_GEN_KEYNUM_TO_VOL_ENV_HOLD] = CONN_SRC_KEYNUMBER,
    [SF_GEN_KEYNUM_TO_VOL_ENV_DECAY] = CONN_SRC_KEYNUMBER,
};

static const USHORT mod_ctrl_to_conn_src[] =
{
    [SF_MOD_CTRL_GEN_NONE] = CONN_SRC_NONE,
    [SF_MOD_CTRL_GEN_VELOCITY] = CONN_SRC_KEYONVELOCITY,
    [SF_MOD_CTRL_GEN_KEY] = CONN_SRC_KEYNUMBER,
    [SF_MOD_CTRL_GEN_POLY_PRESSURE] = CONN_SRC_POLYPRESSURE,
    [SF_MOD_CTRL_GEN_CHAN_PRESSURE] = CONN_SRC_CHANNELPRESSURE,
    [SF_MOD_CTRL_GEN_PITCH_WHEEL] = CONN_SRC_PITCHWHEEL,
    [SF_MOD_CTRL_GEN_PITCH_WHEEL_SENSITIVITY] = CONN_SRC_RPN0,
};

static USHORT mod_src_to_conn_src(sf_modulator src)
{
    if (src & SF_MOD_CTRL_MIDI)
        return CONN_SRC_CC1 - 1 + (src & 0x7f);

    if ((src & 0x7f) >= ARRAYSIZE(mod_ctrl_to_conn_src))
        return CONN_SRC_NONE;

    return mod_ctrl_to_conn_src[src & 0x7f];
}

static USHORT mod_src_to_conn_transform(sf_modulator src)
{
    USHORT transform = 0;

    if (src & SF_MOD_DIR_DECREASING)
        transform |= CONN_TRN_INVERT;

    if (src & SF_MOD_POL_BIPOLAR)
        transform |= CONN_TRN_BIPOLAR;

    switch (src & 0xc00)
    {
    case SF_MOD_SRC_LINEAR:
        transform |= CONN_TRN_NONE;
        break;
    case SF_MOD_SRC_CONCAVE:
        transform |= CONN_TRN_CONCAVE;
        break;
    case SF_MOD_SRC_CONVEX:
        transform |= CONN_TRN_CONVEX;
        break;
    case SF_MOD_SRC_SWITCH:
        transform |= CONN_TRN_SWITCH;
        break;
    }

    return transform;
}

static HRESULT instrument_add_soundfont_region(struct instrument *This, struct soundfont *soundfont,
        struct sf_generators *generators, struct sf_modulators *modulators)
{
    UINT start_loop, end_loop, unity_note, sample_index = generators->amount[SF_GEN_SAMPLE_ID].value;
    struct sf_sample *sample = soundfont->shdr + sample_index;
    struct articulation *articulation;
    DWORD connection_count;
    struct region *region;
    double attenuation;
    sf_generator oper;
    UINT i;

    if (!(region = calloc(1, sizeof(*region)))) return E_OUTOFMEMORY;
    list_init(&region->articulations);

    connection_count = SF_GEN_END_OPER + modulators->count;
    if (!(articulation = calloc(1, offsetof(struct articulation, connections[connection_count]))))
    {
        free(region);
        return E_OUTOFMEMORY;
    }
    articulation->list.cbSize = sizeof(CONNECTIONLIST);

    for (oper = 0; oper < SF_GEN_END_OPER; ++oper)
    {
        CONNECTION *conn = &articulation->connections[articulation->list.cConnections];
        USHORT dst = gen_to_conn_dst[oper];

        if (dst == CONN_DST_NONE || dst == CONN_DST_GAIN)
            continue;

        conn->usSource = gen_to_conn_src[oper];
        conn->usDestination = dst;

        if (oper == SF_GEN_SUSTAIN_MOD_ENV || oper == SF_GEN_SUSTAIN_VOL_ENV)
            conn->lScale = (1000 - (SHORT)generators->amount[oper].value) * 65536;
        else
            conn->lScale = (SHORT)generators->amount[oper].value * 65536;

        ++articulation->list.cConnections;
    }

    for (i = 0; i < modulators->count; ++i)
    {
        CONNECTION *conn = &articulation->connections[articulation->list.cConnections];
        struct sf_mod *mod = &modulators->mods[i];
        USHORT src_transform, ctrl_transform;

        if (mod->dest_gen >= ARRAYSIZE(gen_to_conn_dst))
            continue;
        conn->usDestination = gen_to_conn_dst[mod->dest_gen];

        if (gen_to_conn_src[mod->dest_gen] == CONN_SRC_NONE)
        {
            conn->usSource = mod_src_to_conn_src(mod->src_mod);
            conn->usControl = mod_src_to_conn_src(mod->amount_src_mod);
            src_transform = mod_src_to_conn_transform(mod->src_mod);
            ctrl_transform = mod_src_to_conn_transform(mod->amount_src_mod);
        }
        else if (mod_src_to_conn_src(mod->amount_src_mod) == CONN_SRC_NONE)
        {
            conn->usSource = gen_to_conn_src[mod->dest_gen];
            conn->usControl = mod_src_to_conn_src(mod->src_mod);
            src_transform = CONN_TRN_NONE;
            ctrl_transform = mod_src_to_conn_transform(mod->src_mod);
        }
        else
        {
            FIXME("Modulator requires a three-input connection.\n");
            continue;
        }
        conn->usTransform = (src_transform << 10) | (ctrl_transform << 4);

        if (mod->dest_gen == SF_GEN_SUSTAIN_MOD_ENV || mod->dest_gen == SF_GEN_SUSTAIN_VOL_ENV)
            conn->lScale = mod->amount * -65536;
        else if (mod->dest_gen == SF_GEN_MOD_LFO_TO_VOLUME && mod->src_mod == SF_MOD_CTRL_GEN_CHAN_PRESSURE)
            conn->lScale = mod->amount * 655360;
        else
            conn->lScale = mod->amount * 65536;

        ++articulation->list.cConnections;
    }

    list_add_tail(&region->articulations, &articulation->entry);

    region->header.RangeKey.usLow = generators->amount[SF_GEN_KEY_RANGE].range.low;
    region->header.RangeKey.usHigh = generators->amount[SF_GEN_KEY_RANGE].range.high;
    region->header.RangeVelocity.usLow = generators->amount[SF_GEN_VEL_RANGE].range.low;
    region->header.RangeVelocity.usHigh = generators->amount[SF_GEN_VEL_RANGE].range.high;
    region->header.usKeyGroup = generators->amount[SF_GEN_EXCLUSIVE_CLASS].value;

    region->wave_link.ulTableIndex = sample_index;

    /* SF2 implementation for the original hardware applies a factor of 0.4 to
     * the attenuation value. Although this does not comply with the SF2 spec,
     * most soundfonts expect this behavior. */
    attenuation = (SHORT)generators->amount[SF_GEN_INITIAL_ATTENUATION].value * 0.4;
    /* Normally, FluidSynth adds a resonance hump compensation in
     * fluid_iir_filter_q_from_dB, but as DLS has no such compensation, it's
     * disabled in the budled version of FluidSynth. Add it back here. */
    attenuation += -15.05;
    /* Add some attenuation to normalize the volume. The value was determined
     * experimentally by comparing instruments from SF2 soundfonts to the
     * gm.dls equivalents. The value is approximate, as there is some volume
     * variation from instrument to instrument. */
    attenuation += 80.;
    unity_note = generators->amount[SF_GEN_OVERRIDING_ROOT_KEY].value;
    if (unity_note == (WORD)-1) unity_note = sample->original_key;
    region->wave_sample.usUnityNote = unity_note - (SHORT)generators->amount[SF_GEN_COARSE_TUNE].value;
    region->wave_sample.sFineTune = sample->correction + (SHORT)generators->amount[SF_GEN_FINE_TUNE].value;
    region->wave_sample.lAttenuation = (LONG)round(attenuation * -65536.);

    start_loop = generators->amount[SF_GEN_STARTLOOP_ADDRS_OFFSET].value;
    start_loop += generators->amount[SF_GEN_STARTLOOP_ADDRS_COARSE_OFFSET].value * 32768;
    end_loop = generators->amount[SF_GEN_ENDLOOP_ADDRS_OFFSET].value;
    end_loop += generators->amount[SF_GEN_ENDLOOP_ADDRS_COARSE_OFFSET].value * 32768;
    region->wave_loop.ulStart = sample->start_loop + start_loop - sample->start;
    region->wave_loop.ulLength = sample->end_loop + end_loop - sample->start_loop;

    switch (generators->amount[SF_GEN_SAMPLE_MODES].value & 0x3)
    {
    case SF_UNLOOPED:
    case SF_NOTUSED:
        break;
    case SF_LOOP_DURING_RELEASE:
        region->wave_sample.cSampleLoops = 1;
        region->wave_loop.ulType = WLOOP_TYPE_FORWARD;
        break;
    case SF_LOOP_UNTIL_RELEASE:
        region->wave_sample.cSampleLoops = 1;
        region->wave_loop.ulType = WLOOP_TYPE_RELEASE;
        break;
    }

    list_add_tail(&This->regions, &region->entry);
    This->header.cRegions++;
    return S_OK;
}

static HRESULT instrument_add_soundfont_instrument(struct instrument *This, struct soundfont *soundfont,
        UINT index, struct sf_generators *preset_generators, struct sf_modulators *preset_modulators)
{
    struct sf_generators global_generators = SF_DEFAULT_GENERATORS;
    struct sf_instrument *instrument = soundfont->inst + index;
    struct sf_modulators global_modulators = {};
    struct sf_modulators modulators = {};
    UINT i = instrument->bag_ndx;
    sf_generator oper;
    HRESULT hr = S_OK;
    UINT j;

    copy_modulators(&global_modulators, SF_DEFAULT_MODULATORS, ARRAYSIZE(SF_DEFAULT_MODULATORS));

    for (i = instrument->bag_ndx; SUCCEEDED(hr) && i < (instrument + 1)->bag_ndx; i++)
    {
        struct sf_generators generators = global_generators;

        copy_modulators(&modulators, global_modulators.mods, global_modulators.count);

        parse_soundfont_modulators(soundfont, i, FALSE, &modulators);
        if (parse_soundfont_generators(soundfont, i, FALSE, &generators))
        {
            if (i > instrument->bag_ndx)
                WARN("Ignoring instrument zone without a sample id\n");
            else
            {
                global_generators = generators;
                copy_modulators(&global_modulators, modulators.mods, modulators.count);
            }
            continue;
        }

        for (oper = 0; oper < SF_GEN_END_OPER; ++oper)
        {
            if (oper == SF_GEN_KEY_RANGE || oper == SF_GEN_VEL_RANGE)
            {
                generators.amount[oper].range.low = max(
                    generators.amount[oper].range.low,
                    preset_generators->amount[oper].range.low);
                generators.amount[oper].range.high = min(
                    generators.amount[oper].range.high,
                    preset_generators->amount[oper].range.high);
                continue;
            }
            generators.amount[oper].value += preset_generators->amount[oper].value;
        }

        for (j = 0; j < preset_modulators->count; ++j)
        {
            struct sf_mod *preset_mod = &preset_modulators->mods[j];
            struct sf_mod *mod;
            if ((mod = find_modulator(&modulators, preset_mod)))
            {
                mod->amount += preset_mod->amount;
                continue;
            }
            add_modulator(&modulators, preset_mod);
        }

        hr = instrument_add_soundfont_region(This, soundfont, &generators, &modulators);
    }

    free(modulators.mods);
    free(global_modulators.mods);

    return hr;
}

HRESULT instrument_create_from_soundfont(struct soundfont *soundfont, UINT index,
        struct collection *collection, DMUS_OBJECTDESC *desc, IDirectMusicInstrument **ret_iface)
{
    struct sf_preset *preset = soundfont->phdr + index;
    struct sf_generators global_generators =
    {
        .amount =
        {
            [SF_GEN_KEY_RANGE] = {.range = {.low = 0, .high = 127}},
            [SF_GEN_VEL_RANGE] = {.range = {.low = 0, .high = 127}},
        },
    };
    struct sf_modulators global_modulators = {};
    struct sf_modulators modulators = {};
    IDirectMusicInstrument *iface;
    struct instrument *This;
    HRESULT hr;
    UINT i;

    TRACE("(%p, %u, %p, %p, %p)\n", soundfont, index, collection, desc, ret_iface);

    if (FAILED(hr = instrument_create(collection, &iface))) return hr;
    This = impl_from_IDirectMusicInstrument(iface);

    This->header.Locale.ulBank = (preset->bank & 0x7f) | ((preset->bank << 1) & 0x7f00);
    if (preset->bank == 128)
        This->header.Locale.ulBank = F_INSTRUMENT_DRUMS;
    This->header.Locale.ulInstrument = preset->preset;
    MultiByteToWideChar(CP_ACP, 0, preset->name, strlen(preset->name) + 1,
            desc->wszName, sizeof(desc->wszName));

    for (i = preset->bag_ndx; SUCCEEDED(hr) && i < (preset + 1)->bag_ndx; i++)
    {
        struct sf_generators generators = global_generators;
        UINT instrument;

        copy_modulators(&modulators, global_modulators.mods, global_modulators.count);

        parse_soundfont_modulators(soundfont, i, TRUE, &modulators);
        if (parse_soundfont_generators(soundfont, i, TRUE, &generators))
        {
            if (i > preset->bag_ndx)
                WARN("Ignoring preset zone without an instrument\n");
            else
            {
                global_generators = generators;
                copy_modulators(&global_modulators, modulators.mods, modulators.count);
            }
            continue;
        }

        instrument = generators.amount[SF_GEN_INSTRUMENT].value;
        hr = instrument_add_soundfont_instrument(This, soundfont, instrument, &generators, &modulators);
    }

    free(modulators.mods);
    free(global_modulators.mods);

    if (FAILED(hr))
    {
        IDirectMusicInstrument_Release(iface);
        return hr;
    }

    if (TRACE_ON(dmusic))
    {
        struct region *region;
        UINT i;

        TRACE("Created DirectMusicInstrument %p\n", This);
        TRACE(" - header:\n");
        TRACE("    - regions: %ld\n", This->header.cRegions);
        TRACE("    - locale: {bank: %#lx, instrument: %#lx} (patch %#lx)\n",
                This->header.Locale.ulBank, This->header.Locale.ulInstrument,
                MIDILOCALE2Patch(&This->header.Locale));
        if (desc->dwValidData & DMUS_OBJ_OBJECT) TRACE(" - guid: %s\n", debugstr_dmguid(&desc->guidObject));
        if (desc->dwValidData & DMUS_OBJ_NAME) TRACE(" - name: %s\n", debugstr_w(desc->wszName));

        TRACE(" - regions:\n");
        LIST_FOR_EACH_ENTRY(region, &This->regions, struct region, entry)
        {
            TRACE("   - region:\n");
            TRACE("     - header: {key: %u - %u, vel: %u - %u, options: %#x, group: %#x}\n",
                    region->header.RangeKey.usLow, region->header.RangeKey.usHigh,
                    region->header.RangeVelocity.usLow, region->header.RangeVelocity.usHigh,
                    region->header.fusOptions, region->header.usKeyGroup);
            TRACE("     - wave_link: {options: %#x, group: %u, channel: %lu, index: %lu}\n",
                    region->wave_link.fusOptions, region->wave_link.usPhaseGroup,
                    region->wave_link.ulChannel, region->wave_link.ulTableIndex);
            TRACE("     - wave_sample: {size: %lu, unity_note: %u, fine_tune: %d, attenuation: %ld, options: %#lx, loops: %lu}\n",
                    region->wave_sample.cbSize, region->wave_sample.usUnityNote,
                    region->wave_sample.sFineTune, region->wave_sample.lAttenuation,
                    region->wave_sample.fulOptions, region->wave_sample.cSampleLoops);
            for (i = 0; i < region->wave_sample.cSampleLoops; i++)
                TRACE("     - wave_loop[%u]: {size: %lu, type: %lu, start: %lu, length: %lu}\n", i,
                        region->wave_loop.cbSize, region->wave_loop.ulType,
                        region->wave_loop.ulStart, region->wave_loop.ulLength);
        }
    }

    *ret_iface = iface;
    return S_OK;
}

static void write_articulation_download(struct list *articulations, ULONG *offsets,
        BYTE **ptr, UINT index, DWORD *first, UINT *end)
{
    DMUS_ARTICULATION2 *dmus_articulation2 = NULL;
    struct articulation *articulation;
    CONNECTIONLIST *list;
    UINT size;

    LIST_FOR_EACH_ENTRY(articulation, articulations, struct articulation, entry)
    {
        if (dmus_articulation2) dmus_articulation2->ulNextArtIdx = index;
        else *first = index;

        offsets[index++] = sizeof(DMUS_DOWNLOADINFO) + *ptr - (BYTE *)offsets;
        dmus_articulation2 = (DMUS_ARTICULATION2 *)*ptr;
        (*ptr) += sizeof(DMUS_ARTICULATION2);

        dmus_articulation2->ulArtIdx = index;
        dmus_articulation2->ulFirstExtCkIdx = 0;
        dmus_articulation2->ulNextArtIdx = 0;

        size = articulation->list.cConnections * sizeof(CONNECTION);
        offsets[index++] = sizeof(DMUS_DOWNLOADINFO) + *ptr - (BYTE *)offsets;
        list = (CONNECTIONLIST *)*ptr;
        (*ptr) += sizeof(CONNECTIONLIST) + size;

        *list = articulation->list;
        memcpy(list + 1, articulation->connections, size);
    }

    *end = index;
}

struct download_buffer
{
    DMUS_DOWNLOADINFO info;
    ULONG offsets[];
};

C_ASSERT(sizeof(struct download_buffer) == offsetof(struct download_buffer, offsets[0]));

HRESULT instrument_download_to_port(IDirectMusicInstrument *iface, IDirectMusicPortDownload *port,
        IDirectMusicDownloadedInstrument **downloaded)
{
    struct instrument *This = impl_from_IDirectMusicInstrument(iface);
    struct downloaded_wave *downloaded_wave;
    struct articulation *articulation;
    struct download_buffer *buffer;
    IDirectMusicDownload *download;
    DWORD size, offset_count;
    struct region *region;
    IDirectMusicObject *wave;
    HRESULT hr;

    if (This->download) goto done;

    size = sizeof(DMUS_DOWNLOADINFO);
    size += sizeof(ULONG) + sizeof(DMUS_INSTRUMENT);
    offset_count = 1;

    LIST_FOR_EACH_ENTRY(articulation, &This->articulations, struct articulation, entry)
    {
        size += sizeof(ULONG) + sizeof(DMUS_ARTICULATION2);
        size += sizeof(ULONG) + sizeof(CONNECTIONLIST);
        size += articulation->list.cConnections * sizeof(CONNECTION);
        offset_count += 2;
    }

    LIST_FOR_EACH_ENTRY(region, &This->regions, struct region, entry)
    {
        size += sizeof(ULONG) + sizeof(DMUS_REGION);
        offset_count++;

        LIST_FOR_EACH_ENTRY(articulation, &region->articulations, struct articulation, entry)
        {
            size += sizeof(ULONG) + sizeof(DMUS_ARTICULATION2);
            size += sizeof(ULONG) + sizeof(CONNECTIONLIST);
            size += articulation->list.cConnections * sizeof(CONNECTION);
            offset_count += 2;
        }
    }

    if (FAILED(hr = IDirectMusicPortDownload_AllocateBuffer(port, size, &download))) return hr;

    if (SUCCEEDED(hr = IDirectMusicDownload_GetBuffer(download, (void **)&buffer, &size))
            && SUCCEEDED(hr = IDirectMusicPortDownload_GetDLId(port, &buffer->info.dwDLId, 1)))
    {
        BYTE *ptr = (BYTE *)&buffer->offsets[offset_count];
        DMUS_INSTRUMENT *dmus_instrument;
        DMUS_REGION *dmus_region = NULL;
        UINT index = 0;

        buffer->info.dwDLType = DMUS_DOWNLOADINFO_INSTRUMENT2;
        buffer->info.dwNumOffsetTableEntries = offset_count;
        buffer->info.cbSize = size;

        buffer->offsets[index++] = ptr - (BYTE *)buffer;
        dmus_instrument = (DMUS_INSTRUMENT *)ptr;
        ptr += sizeof(DMUS_INSTRUMENT);

        dmus_instrument->ulPatch = MIDILOCALE2Patch(&This->header.Locale);
        dmus_instrument->ulFirstRegionIdx = 0;
        dmus_instrument->ulCopyrightIdx = 0;
        dmus_instrument->ulGlobalArtIdx = 0;

        write_articulation_download(&This->articulations, buffer->offsets, &ptr, index,
                &dmus_instrument->ulGlobalArtIdx, &index);

        LIST_FOR_EACH_ENTRY(region, &This->regions, struct region, entry)
        {
            if (dmus_region) dmus_region->ulNextRegionIdx = index;
            else dmus_instrument->ulFirstRegionIdx = index;

            buffer->offsets[index++] = ptr - (BYTE *)buffer;
            dmus_region = (DMUS_REGION *)ptr;
            ptr += sizeof(DMUS_REGION);

            dmus_region->RangeKey = region->header.RangeKey;
            dmus_region->RangeVelocity = region->header.RangeVelocity;
            dmus_region->fusOptions = region->header.fusOptions;
            dmus_region->usKeyGroup = region->header.usKeyGroup;
            dmus_region->ulRegionArtIdx = 0;
            dmus_region->ulNextRegionIdx = 0;
            dmus_region->ulFirstExtCkIdx = 0;
            dmus_region->WaveLink = region->wave_link;
            dmus_region->WSMP = region->wave_sample;
            dmus_region->WLOOP[0] = region->wave_loop;

            LIST_FOR_EACH_ENTRY(downloaded_wave, &This->downloaded_waves, struct downloaded_wave, entry)
            {
                if (downloaded_wave->index == region->wave_link.ulTableIndex)
                    break;
            }
            if (&downloaded_wave->entry == &This->downloaded_waves)
            {
                downloaded_wave = calloc(1, sizeof(struct downloaded_wave));
                if (!downloaded_wave)
                {
                    hr = E_OUTOFMEMORY;
                    goto failed;
                }
                downloaded_wave->index = region->wave_link.ulTableIndex;
                if (SUCCEEDED(hr = collection_get_wave(This->collection, region->wave_link.ulTableIndex, &wave)))
                {
                    hr = wave_download_to_port(wave, port, &downloaded_wave->id);
                    IDirectMusicObject_Release(wave);
                }
                if (FAILED(hr))
                {
                    free(downloaded_wave);
                    goto failed;
                }
                list_add_tail(&This->downloaded_waves, &downloaded_wave->entry);
            }

            dmus_region->WaveLink.ulTableIndex = downloaded_wave->id;

            write_articulation_download(&region->articulations, buffer->offsets, &ptr, index,
                    &dmus_region->ulRegionArtIdx, &index);
        }

        if (FAILED(hr = IDirectMusicPortDownload_Download(port, download))) goto failed;
    }

    This->download = download;

done:
    *downloaded = &This->IDirectMusicDownloadedInstrument_iface;
    IDirectMusicDownloadedInstrument_AddRef(*downloaded);
    return S_OK;

failed:
    WARN("Failed to download instrument to port, hr %#lx\n", hr);
    IDirectMusicDownload_Release(download);
    return hr;
}

HRESULT instrument_unload_from_port(IDirectMusicDownloadedInstrument *iface, IDirectMusicPortDownload *port)
{
    struct instrument *This = impl_from_IDirectMusicDownloadedInstrument(iface);
    HRESULT hr;

    if (!This->download) return DMUS_E_NOT_DOWNLOADED_TO_PORT;

    if (FAILED(hr = IDirectMusicPortDownload_Unload(port, This->download)))
        WARN("Failed to unload instrument download buffer, hr %#lx\n", hr);
    else
    {
        struct downloaded_wave *downloaded_wave;
        IDirectMusicDownload *wave_download;
        void *next;

        LIST_FOR_EACH_ENTRY_SAFE(downloaded_wave, next, &This->downloaded_waves, struct downloaded_wave, entry)
        {
            if (FAILED(hr = IDirectMusicPortDownload_GetBuffer(port, downloaded_wave->id, &wave_download)))
                WARN("Failed to get wave download with id %#lx, hr %#lx\n", downloaded_wave->id, hr);
            else
            {
                if (FAILED(hr = IDirectMusicPortDownload_Unload(port, wave_download)))
                    WARN("Failed to unload wave download buffer, hr %#lx\n", hr);
                IDirectMusicDownload_Release(wave_download);
            }
            list_remove(&downloaded_wave->entry);
            free(downloaded_wave);
        }
    }

    IDirectMusicDownload_Release(This->download);
    This->download = NULL;

    return hr;
}
