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

#include "xaudio2.h"
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
#if XAUDIO2_VER == 0
    IXAudio20SourceVoice IXAudio20SourceVoice_iface;
#elif XAUDIO2_VER <= 3
    IXAudio23SourceVoice IXAudio23SourceVoice_iface;
#elif XAUDIO2_VER <= 7
    IXAudio27SourceVoice IXAudio27SourceVoice_iface;
#endif

    IXAudio2SubmixVoice IXAudio2SubmixVoice_iface;
#if XAUDIO2_VER == 0
    IXAudio20SubmixVoice IXAudio20SubmixVoice_iface;
#elif XAUDIO2_VER <= 3
    IXAudio23SubmixVoice IXAudio23SubmixVoice_iface;
#elif XAUDIO2_VER <= 7
    IXAudio27SubmixVoice IXAudio27SubmixVoice_iface;
#endif

    IXAudio2MasteringVoice IXAudio2MasteringVoice_iface;
#if XAUDIO2_VER == 0
    IXAudio20MasteringVoice IXAudio20MasteringVoice_iface;
#elif XAUDIO2_VER <= 3
    IXAudio23MasteringVoice IXAudio23MasteringVoice_iface;
#elif XAUDIO2_VER <= 7
    IXAudio27MasteringVoice IXAudio27MasteringVoice_iface;
#endif

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

#if XAUDIO2_VER == 0
    IXAudio20 IXAudio20_iface;
#elif XAUDIO2_VER <= 2
    IXAudio22 IXAudio22_iface;
#elif XAUDIO2_VER <= 3
    IXAudio23 IXAudio23_iface;
#elif XAUDIO2_VER <= 7
    IXAudio27 IXAudio27_iface;
#endif

    CRITICAL_SECTION lock;

    struct list voices;

    FAudio *faudio;

    FAudioEngineCallback FAudioEngineCallback_vtbl;

    XA2VoiceImpl mst;

    DWORD last_query_glitches;

    UINT32 ncbs;
    IXAudio2EngineCallback **cbs;
} IXAudio2Impl;

#if XAUDIO2_VER == 0
extern const IXAudio20SourceVoiceVtbl XAudio20SourceVoice_Vtbl DECLSPEC_HIDDEN;
extern const IXAudio20SubmixVoiceVtbl XAudio20SubmixVoice_Vtbl DECLSPEC_HIDDEN;
extern const IXAudio20MasteringVoiceVtbl XAudio20MasteringVoice_Vtbl DECLSPEC_HIDDEN;
extern XA2VoiceImpl *impl_from_IXAudio20SourceVoice(IXAudio20SourceVoice *iface) DECLSPEC_HIDDEN;
extern XA2VoiceImpl *impl_from_IXAudio20SubmixVoice(IXAudio20SubmixVoice *iface) DECLSPEC_HIDDEN;
extern XA2VoiceImpl *impl_from_IXAudio20MasteringVoice(IXAudio20MasteringVoice *iface) DECLSPEC_HIDDEN;
#elif XAUDIO2_VER <= 3
extern const IXAudio23SourceVoiceVtbl XAudio23SourceVoice_Vtbl DECLSPEC_HIDDEN;
extern const IXAudio23SubmixVoiceVtbl XAudio23SubmixVoice_Vtbl DECLSPEC_HIDDEN;
extern const IXAudio23MasteringVoiceVtbl XAudio23MasteringVoice_Vtbl DECLSPEC_HIDDEN;
extern XA2VoiceImpl *impl_from_IXAudio23SourceVoice(IXAudio23SourceVoice *iface) DECLSPEC_HIDDEN;
extern XA2VoiceImpl *impl_from_IXAudio23SubmixVoice(IXAudio23SubmixVoice *iface) DECLSPEC_HIDDEN;
extern XA2VoiceImpl *impl_from_IXAudio23MasteringVoice(IXAudio23MasteringVoice *iface) DECLSPEC_HIDDEN;
#elif XAUDIO2_VER <= 7
extern const IXAudio27SourceVoiceVtbl XAudio27SourceVoice_Vtbl DECLSPEC_HIDDEN;
extern const IXAudio27SubmixVoiceVtbl XAudio27SubmixVoice_Vtbl DECLSPEC_HIDDEN;
extern const IXAudio27MasteringVoiceVtbl XAudio27MasteringVoice_Vtbl DECLSPEC_HIDDEN;
extern XA2VoiceImpl *impl_from_IXAudio27SourceVoice(IXAudio27SourceVoice *iface) DECLSPEC_HIDDEN;
extern XA2VoiceImpl *impl_from_IXAudio27SubmixVoice(IXAudio27SubmixVoice *iface) DECLSPEC_HIDDEN;
extern XA2VoiceImpl *impl_from_IXAudio27MasteringVoice(IXAudio27MasteringVoice *iface) DECLSPEC_HIDDEN;
#endif

#if XAUDIO2_VER == 0
extern const IXAudio20Vtbl XAudio20_Vtbl DECLSPEC_HIDDEN;
#elif XAUDIO2_VER <= 2
extern const IXAudio22Vtbl XAudio22_Vtbl DECLSPEC_HIDDEN;
#elif XAUDIO2_VER <= 3
extern const IXAudio23Vtbl XAudio23_Vtbl DECLSPEC_HIDDEN;
#elif XAUDIO2_VER <= 7
extern const IXAudio27Vtbl XAudio27_Vtbl DECLSPEC_HIDDEN;
#endif

/* xaudio_dll.c */
extern HRESULT xaudio2_initialize(IXAudio2Impl *This, UINT32 flags, XAUDIO2_PROCESSOR proc) DECLSPEC_HIDDEN;
extern FAudioEffectChain *wrap_effect_chain(const XAUDIO2_EFFECT_CHAIN *pEffectChain) DECLSPEC_HIDDEN;
extern void engine_cb(FAudioEngineCallEXT proc, FAudio *faudio, float *stream, void *user) DECLSPEC_HIDDEN;
extern DWORD WINAPI engine_thread(void *user) DECLSPEC_HIDDEN;

/* xapo.c */
extern HRESULT make_xapo_factory(REFCLSID clsid, REFIID riid, void **ppv) DECLSPEC_HIDDEN;

/* xaudio_allocator.c */
extern void* XAudio_Internal_Malloc(size_t size) DECLSPEC_HIDDEN;
extern void XAudio_Internal_Free(void* ptr) DECLSPEC_HIDDEN;
extern void* XAudio_Internal_Realloc(void* ptr, size_t size) DECLSPEC_HIDDEN;
