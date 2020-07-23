/*
 * Copyright 2019 Alistair Leslie-Hughes
 * Copyright 2020 Zebediah Figura
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
#include "windef.h"
#include "wingdi.h"
#include "mmreg.h"
#include "mmsystem.h"
#include "dmo.h"
#include "initguid.h"
#include "dsound.h"
#include "uuids.h"
#include "wine/test.h"

static const GUID test_iid = {0x33333333};
static LONG outer_ref = 1;

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IMediaObject)
            || IsEqualGUID(iid, &test_iid))
    {
        *out = (IUnknown *)0xdeadbeef;
        return S_OK;
    }
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI outer_AddRef(IUnknown *iface)
{
    return InterlockedIncrement(&outer_ref);
}

static ULONG WINAPI outer_Release(IUnknown *iface)
{
    return InterlockedDecrement(&outer_ref);
}

static const IUnknownVtbl outer_vtbl =
{
    outer_QueryInterface,
    outer_AddRef,
    outer_Release,
};

static IUnknown test_outer = {&outer_vtbl};

static void test_aggregation(const GUID *clsid)
{
    IMediaObject *dmo, *dmo2;
    IUnknown *unk, *unk2;
    HRESULT hr;
    ULONG ref;

    dmo = (IMediaObject *)0xdeadbeef;
    hr = CoCreateInstance(clsid, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IMediaObject, (void **)&dmo);
    todo_wine ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    if (hr != E_NOINTERFACE)
        return;
    ok(!dmo, "Got interface %p.\n", dmo);

    hr = CoCreateInstance(clsid, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);
    ok(unk != &test_outer, "Returned IUnknown should not be outer IUnknown.\n");
    ref = get_refcount(unk);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);

    ref = IUnknown_AddRef(unk);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);

    ref = IUnknown_Release(unk);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == unk, "Got unexpected IUnknown %p.\n", unk2);
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IMediaObject, (void **)&dmo);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaObject_QueryInterface(dmo, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    hr = IMediaObject_QueryInterface(dmo, &IID_IMediaObject, (void **)&dmo2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dmo2 == (IMediaObject *)0xdeadbeef, "Got unexpected IMediaObject %p.\n", dmo2);

    hr = IUnknown_QueryInterface(unk, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    hr = IMediaObject_QueryInterface(dmo, &test_iid, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    IMediaObject_Release(dmo);
    ref = IUnknown_Release(unk);
    ok(!ref, "Got unexpected refcount %d.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);
}

static void test_interfaces(const GUID *clsid, const GUID *iid)
{
    IUnknown *unk, *unk2, *unk3;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, iid, (void **)&unk);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (hr != S_OK)
        return;

    hr = IUnknown_QueryInterface(unk, &IID_IMediaObject, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IUnknown_QueryInterface(unk2, iid, (void **)&unk3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk3 == unk, "Interface pointers didn't match.\n");
    IUnknown_Release(unk3);
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IMediaObjectInPlace, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IUnknown_QueryInterface(unk2, iid, (void **)&unk3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk3 == unk, "Interface pointers didn't match.\n");
    IUnknown_Release(unk3);
    IUnknown_Release(unk2);

    ref = IUnknown_Release(unk);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_chorus_parameters(void)
{
    IDirectSoundFXChorus *chorus;
    DSFXChorus params;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&GUID_DSFX_STANDARD_CHORUS, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectSoundFXChorus, (void **)&chorus);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (hr != S_OK)
        return;

    hr = IDirectSoundFXChorus_GetAllParameters(chorus, &params);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(params.fWetDryMix == 50.0f, "Got wetness %.8e%%.\n", params.fWetDryMix);
    ok(params.fDepth == 10.0f, "Got depth %.8e.\n", params.fDepth);
    ok(params.fFeedback == 25.0f, "Got feedback %.8e.\n", params.fFeedback);
    ok(params.fFrequency == 1.1f, "Got LFO frequency %.8e.\n", params.fFrequency);
    ok(params.lWaveform == DSFXCHORUS_WAVE_SIN, "Got LFO waveform %d.\n", params.lWaveform);
    ok(params.fDelay == 16.0f, "Got delay %.8e.\n", params.fDelay);
    ok(params.lPhase == 3, "Got phase differential %d.\n", params.lPhase);

    ref = IDirectSoundFXChorus_Release(chorus);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_compressor_parameters(void)
{
    IDirectSoundFXCompressor *compressor;
    DSFXCompressor params;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&GUID_DSFX_STANDARD_COMPRESSOR, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectSoundFXCompressor, (void **)&compressor);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (hr != S_OK)
        return;

    hr = IDirectSoundFXCompressor_GetAllParameters(compressor, &params);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(params.fGain == 0.0f, "Got gain %.8e dB.\n", params.fGain);
    ok(params.fAttack == 10.0f, "Got attack time %.8e ms.\n", params.fAttack);
    ok(params.fThreshold == -20.0f, "Got threshold %.8e dB.\n", params.fThreshold);
    ok(params.fRatio == 3.0f, "Got ratio %.8e:1.\n", params.fRatio);
    ok(params.fPredelay == 4.0f, "Got pre-delay %.8e ms.\n", params.fPredelay);

    ref = IDirectSoundFXCompressor_Release(compressor);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_distortion_parameters(void)
{
    IDirectSoundFXDistortion *distortion;
    DSFXDistortion params;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&GUID_DSFX_STANDARD_DISTORTION, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectSoundFXDistortion, (void **)&distortion);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (hr != S_OK)
        return;

    hr = IDirectSoundFXDistortion_GetAllParameters(distortion, &params);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(params.fGain == -18.0f, "Got gain %.8e dB.\n", params.fGain);
    ok(params.fEdge == 15.0f, "Got edge %.8e%%.\n", params.fEdge);
    ok(params.fPostEQCenterFrequency == 2400.0f, "Got center frequency %.8e Hz.\n", params.fPostEQCenterFrequency);
    ok(params.fPostEQBandwidth == 2400.0f, "Got band width %.8e Hz.\n", params.fPostEQBandwidth);
    ok(params.fPreLowpassCutoff == 8000.0f, "Got pre-lowpass cutoff %.8e Hz.\n", params.fPreLowpassCutoff);

    ref = IDirectSoundFXDistortion_Release(distortion);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_echo_parameters(void)
{
    IDirectSoundFXEcho *echo;
    DSFXEcho params;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&GUID_DSFX_STANDARD_ECHO, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectSoundFXEcho, (void **)&echo);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (hr != S_OK)
        return;

    hr = IDirectSoundFXEcho_GetAllParameters(echo, &params);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(params.fWetDryMix == 50.0f, "Got %.8e%% wetness.\n", params.fWetDryMix);
    ok(params.fFeedback == 50.0f, "Got %.8e%% feedback.\n", params.fFeedback);
    ok(params.fLeftDelay == 500.0f, "Got left delay %.8e ms.\n", params.fLeftDelay);
    ok(params.fRightDelay == 500.0f, "Got right delay %.8e ms.\n", params.fRightDelay);
    ok(!params.lPanDelay, "Got delay swap %d.\n", params.lPanDelay);

    ref = IDirectSoundFXEcho_Release(echo);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_flanger_parameters(void)
{
    IDirectSoundFXFlanger *flanger;
    DSFXFlanger params;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&GUID_DSFX_STANDARD_FLANGER, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectSoundFXFlanger, (void **)&flanger);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (hr != S_OK)
        return;

    hr = IDirectSoundFXFlanger_GetAllParameters(flanger, &params);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(params.fWetDryMix == 50.0f, "Got %.8e%% wetness.\n", params.fWetDryMix);
    ok(params.fDepth == 100.0f, "Got %.8e * 0.01%% depth.\n", params.fDepth);
    ok(params.fFeedback == -50.0f, "Got %.8e%% feedback.\n", params.fFeedback);
    ok(params.lWaveform == DSFXFLANGER_WAVE_SIN, "Got LFO waveform %d.\n", params.lWaveform);
    ok(params.fDelay == 2.0f, "Got delay %.8e ms.\n", params.fDelay);
    ok(params.lPhase == DSFXFLANGER_PHASE_ZERO, "Got phase differential %d.\n", params.lPhase);

    ref = IDirectSoundFXFlanger_Release(flanger);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_gargle_parameters(void)
{
    IDirectSoundFXGargle *gargle;
    DSFXGargle params;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&GUID_DSFX_STANDARD_GARGLE, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectSoundFXGargle, (void **)&gargle);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (hr != S_OK)
        return;

    hr = IDirectSoundFXGargle_GetAllParameters(gargle, &params);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(params.dwRateHz == 20, "Got rate %u Hz.\n", params.dwRateHz);
    ok(params.dwWaveShape == DSFXGARGLE_WAVE_TRIANGLE, "Got wave shape %u.\n", params.dwWaveShape);

    ref = IDirectSoundFXGargle_Release(gargle);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_eq_parameters(void)
{
    IDirectSoundFXParamEq *eq;
    DSFXParamEq params;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&GUID_DSFX_STANDARD_PARAMEQ, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectSoundFXParamEq, (void **)&eq);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (hr != S_OK)
        return;

    hr = IDirectSoundFXParamEq_GetAllParameters(eq, &params);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(params.fCenter == 8000.0f, "Got center frequency %.8e Hz.\n", params.fCenter);
    ok(params.fBandwidth == 12.0f, "Got band width %.8e semitones.\n", params.fBandwidth);
    ok(params.fGain == 0.0f, "Got gain %.8e.\n", params.fGain);

    ref = IDirectSoundFXParamEq_Release(eq);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_reverb_parameters(void)
{
    IDirectSoundFXI3DL2Reverb *reverb;
    DSFXI3DL2Reverb params;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&GUID_DSFX_STANDARD_I3DL2REVERB, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectSoundFXI3DL2Reverb, (void **)&reverb);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (hr != S_OK)
        return;

    hr = IDirectSoundFXI3DL2Reverb_GetAllParameters(reverb, &params);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(params.lRoom == -1000, "Got room attenuation %d mB.\n", params.lRoom);
    ok(params.lRoomHF == -100, "Got room high-frequency attenuation %d mB.\n", params.lRoomHF);
    ok(params.flRoomRolloffFactor == 0.0f, "Got room rolloff factor %.8e.\n", params.flRoomRolloffFactor);
    ok(params.flDecayTime == 1.49f, "Got decay time %.8e s.\n", params.flDecayTime);
    ok(params.flDecayHFRatio == 0.83f, "Got decay time ratio %.8e.\n", params.flDecayHFRatio);
    ok(params.lReflections == -2602, "Got early reflection attenuation %d mB.\n", params.lReflections);
    ok(params.flReflectionsDelay == 0.007f, "Got first reflection delay %.8e s.\n", params.flReflectionsDelay);
    ok(params.lReverb == 200, "Got reverb attenuation %d mB.\n", params.lReverb);
    ok(params.flReverbDelay == 0.011f, "Got reverb delay %.8e s.\n", params.flReverbDelay);
    ok(params.flDiffusion == 100.0f, "Got diffusion %.8e%%.\n", params.flDiffusion);
    ok(params.flDensity == 100.0f, "Got density %.8e%%.\n", params.flDensity);
    ok(params.flHFReference == 5000.0f, "Got reference high frequency %.8e Hz.\n", params.flHFReference);

    ref = IDirectSoundFXI3DL2Reverb_Release(reverb);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

START_TEST(dsdmo)
{
    static const struct
    {
        const GUID *clsid;
        const GUID *iid;
    }
    tests[] =
    {
        {&GUID_DSFX_STANDARD_CHORUS,        &IID_IDirectSoundFXChorus},
        {&GUID_DSFX_STANDARD_COMPRESSOR,    &IID_IDirectSoundFXCompressor},
        {&GUID_DSFX_STANDARD_DISTORTION,    &IID_IDirectSoundFXDistortion},
        {&GUID_DSFX_STANDARD_ECHO,          &IID_IDirectSoundFXEcho},
        {&GUID_DSFX_STANDARD_FLANGER,       &IID_IDirectSoundFXFlanger},
        {&GUID_DSFX_STANDARD_GARGLE,        &IID_IDirectSoundFXGargle},
        {&GUID_DSFX_STANDARD_I3DL2REVERB,   &IID_IDirectSoundFXI3DL2Reverb},
        {&GUID_DSFX_STANDARD_PARAMEQ,       &IID_IDirectSoundFXParamEq},
        {&GUID_DSFX_WAVES_REVERB,           &IID_IDirectSoundFXWavesReverb},
    };
    unsigned int i;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        test_aggregation(tests[i].clsid);
        test_interfaces(tests[i].clsid, tests[i].iid);
    }

    test_chorus_parameters();
    test_compressor_parameters();
    test_distortion_parameters();
    test_echo_parameters();
    test_flanger_parameters();
    test_gargle_parameters();
    test_reverb_parameters();
    test_eq_parameters();

    CoUninitialize();
}
