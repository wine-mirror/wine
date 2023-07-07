/*
 * Copyright (c) 2015 Andrew Eikum for CodeWeavers
 * Copyright (c) 2018 Ethan Lee for CodeWeavers
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

#include "windef.h"
#include "wine/list.h"

/* Don't include xaudio2.h directly; it's generated from an IDL with ifdefs and
 * hence is frozen at version 2.7. Instead include that IDL in a local IDL and
 * include the generated header.
 *
 * Because shared sources are compiled from the C file in the xaudio2_7
 * directory, we need to use angle brackets here to prevent the compiler from
 * picking up xaudio_classes.h from that directory for other versions. */
#include <xaudio_classes.h>
#include "xapo.h"

#include <FAudio.h>
#include <FAPO.h>

#if XAUDIO2_VER == 0
#define COMPAT_E_INVALID_CALL E_INVALIDARG
#define COMPAT_E_DEVICE_INVALIDATED XAUDIO20_E_DEVICE_INVALIDATED
#else
#define COMPAT_E_INVALID_CALL XAUDIO2_E_INVALID_CALL
#define COMPAT_E_DEVICE_INVALIDATED XAUDIO2_E_DEVICE_INVALIDATED
#endif

typedef struct _XA2XAPOImpl {
    IXAPO *xapo;
    IXAPOParameters *xapo_params;

    LONG ref;

    FAPO FAPO_vtbl;
} XA2XAPOImpl;

typedef struct _XA2XAPOFXImpl {
    IXAPO IXAPO_iface;
    IXAPOParameters IXAPOParameters_iface;

    FAPO *fapo;
} XA2XAPOFXImpl;

typedef struct _XA2VoiceImpl {
    IXAudio2SourceVoice IXAudio2SourceVoice_iface;

    IXAudio2SubmixVoice IXAudio2SubmixVoice_iface;

    IXAudio2MasteringVoice IXAudio2MasteringVoice_iface;

    FAudioVoiceCallback FAudioVoiceCallback_vtbl;
    FAudioEffectChain *effect_chain;

    BOOL in_use;

    CRITICAL_SECTION lock;

    IXAudio2VoiceCallback *cb;

    FAudioVoice *faudio_voice;

    struct {
        FAudioEngineCallEXT proc;
        FAudio *faudio;
        float *stream;
    } engine_params;

    struct list entry;
} XA2VoiceImpl;

typedef struct _IXAudio2Impl {
    IXAudio2 IXAudio2_iface;

    CRITICAL_SECTION lock;

    struct list voices;

    FAudio *faudio;

    FAudioEngineCallback FAudioEngineCallback_vtbl;

    XA2VoiceImpl mst;

    DWORD last_query_glitches;

    UINT32 ncbs;
    IXAudio2EngineCallback **cbs;
} IXAudio2Impl;

/* xaudio_dll.c */
extern HRESULT xaudio2_initialize(IXAudio2Impl *This, UINT32 flags, XAUDIO2_PROCESSOR proc);
extern FAudioEffectChain *wrap_effect_chain(const XAUDIO2_EFFECT_CHAIN *pEffectChain);
extern void engine_cb(FAudioEngineCallEXT proc, FAudio *faudio, float *stream, void *user);
extern DWORD WINAPI engine_thread(void *user);

/* xapo.c */
extern HRESULT make_xapo_factory(REFCLSID clsid, REFIID riid, void **ppv);

/* xaudio_allocator.c */
extern void* XAudio_Internal_Malloc(size_t size);
extern void XAudio_Internal_Free(void* ptr);
extern void* XAudio_Internal_Realloc(void* ptr, size_t size);
