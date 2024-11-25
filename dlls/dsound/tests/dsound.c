/*
 * Tests basic sound playback in DirectSound.
 * In particular we test each standard Windows sound format to make sure
 * we handle the sound card/driver quirks correctly.
 *
 * Part of this test involves playing test tones. But this only makes
 * sense if someone is going to carefully listen to it, and would only
 * bother everyone else.
 * So this is only done if the test is being run in interactive mode.
 *
 * Copyright (c) 2002-2004 Francois Gouget
 * Copyright (c) 2007 Maarten Lankhorst
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

#include <windows.h>

#include "wine/test.h"
#include "mmsystem.h"
#define COBJMACROS
#include "dsound.h"
#include "dsconf.h"
#include "initguid.h"
#include "ks.h"
#include "ksmedia.h"

#include "dsound_test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static DWORD WINAPI test_apt_thread(void *param)
{
    HRESULT hr;
    struct apt_data *test_apt_data = (struct apt_data *)param;

    hr = CoGetApartmentType(&test_apt_data->type, &test_apt_data->qualifier);
    if (hr == CO_E_NOTINITIALIZED)
    {
        test_apt_data->type = APTTYPE_UNITIALIZED;
        test_apt_data->qualifier = 0;
    }

    return 0;
}

void check_apttype(struct apt_data *test_apt_data)
{
    HANDLE thread;
    MSG msg;

    memset(test_apt_data, 0xde, sizeof(*test_apt_data));

    thread = CreateThread(NULL, 0, test_apt_thread, test_apt_data, 0, NULL);
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    CloseHandle(thread);
}

static void IDirectSound_test(LPDIRECTSOUND dso, BOOL initialized,
                              LPCGUID lpGuid)
{
    HRESULT rc;
    DSCAPS dscaps;
    int ref;
    IUnknown * unknown;
    IDirectSound * ds;
    IDirectSound8 * ds8;
    DWORD speaker_config, new_speaker_config, ref_speaker_config;

    /* Try to Query for objects */
    rc=IDirectSound_QueryInterface(dso,&IID_IUnknown,(LPVOID*)&unknown);
    ok(rc==DS_OK,"IDirectSound_QueryInterface(IID_IUnknown) failed: %08lx\n", rc);
    if (rc==DS_OK)
        IUnknown_Release(unknown);

    rc=IDirectSound_QueryInterface(dso,&IID_IDirectSound,(LPVOID*)&ds);
    ok(rc==DS_OK,"IDirectSound_QueryInterface(IID_IDirectSound) failed: %08lx\n", rc);
    if (rc==DS_OK)
        IDirectSound_Release(ds);

    rc=IDirectSound_QueryInterface(dso,&IID_IDirectSound8,(LPVOID*)&ds8);
    ok(rc==E_NOINTERFACE,"IDirectSound_QueryInterface(IID_IDirectSound8) "
       "should have failed: %08lx\n",rc);
    if (rc==DS_OK)
        IDirectSound8_Release(ds8);

    if (initialized == FALSE) {
        /* try uninitialized object */
        rc=IDirectSound_GetCaps(dso,0);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound_GetCaps(NULL) "
           "should have returned DSERR_UNINITIALIZED, returned: %08lx\n", rc);

        rc=IDirectSound_GetCaps(dso,&dscaps);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound_GetCaps() "
           "should have returned DSERR_UNINITIALIZED, returned: %08lx\n", rc);

        rc=IDirectSound_Compact(dso);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound_Compact() "
           "should have returned DSERR_UNINITIALIZED, returned: %08lx\n", rc);

        rc=IDirectSound_GetSpeakerConfig(dso,&speaker_config);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound_GetSpeakerConfig() "
           "should have returned DSERR_UNINITIALIZED, returned: %08lx\n", rc);

        rc=IDirectSound_Initialize(dso,lpGuid);
        ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
           "IDirectSound_Initialize() failed: %08lx\n",rc);
        if (rc==DSERR_NODRIVER) {
            trace("  No Driver\n");
            goto EXIT;
        } else if (rc==E_FAIL) {
            trace("  No Device\n");
            goto EXIT;
        } else if (rc==DSERR_ALLOCATED) {
            trace("  Already In Use\n");
            goto EXIT;
        }
    }

    rc=IDirectSound_Initialize(dso,lpGuid);
    ok(rc==DSERR_ALREADYINITIALIZED, "IDirectSound_Initialize() "
       "should have returned DSERR_ALREADYINITIALIZED: %08lx\n", rc);

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound_GetCaps(dso,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSound_GetCaps(NULL) "
       "should have returned DSERR_INVALIDPARAM, returned: %08lx\n", rc);

    ZeroMemory(&dscaps, sizeof(dscaps));

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSound_GetCaps() "
       "should have returned DSERR_INVALIDPARAM, returned: %08lx\n", rc);

    dscaps.dwSize=sizeof(dscaps);

    /* DSOUND: Running on a certified driver */
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound_GetCaps() failed: %08lx\n",rc);

    rc=IDirectSound_Compact(dso);
    ok(rc==DSERR_PRIOLEVELNEEDED,"IDirectSound_Compact() failed: %08lx\n", rc);

    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);

    rc=IDirectSound_Compact(dso);
    ok(rc==DS_OK,"IDirectSound_Compact() failed: %08lx\n",rc);

    rc=IDirectSound_GetSpeakerConfig(dso,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSound_GetSpeakerConfig(NULL) "
       "should have returned DSERR_INVALIDPARAM, returned: %08lx\n", rc);

    rc=IDirectSound_GetSpeakerConfig(dso,&speaker_config);
    ok(rc==DS_OK,"IDirectSound_GetSpeakerConfig() failed: %08lx\n", rc);
    ref_speaker_config = speaker_config;

    speaker_config = DSSPEAKER_COMBINED(DSSPEAKER_STEREO,
                                        DSSPEAKER_GEOMETRY_WIDE);
    if (speaker_config == ref_speaker_config)
        speaker_config = DSSPEAKER_COMBINED(DSSPEAKER_STEREO,
                                            DSSPEAKER_GEOMETRY_NARROW);
    if(rc==DS_OK) {
        rc=IDirectSound_SetSpeakerConfig(dso,speaker_config);
        ok(rc==DS_OK,"IDirectSound_SetSpeakerConfig() failed: %08lx\n", rc);
    }
    if (rc==DS_OK) {
        rc=IDirectSound_GetSpeakerConfig(dso,&new_speaker_config);
        ok(rc==DS_OK,"IDirectSound_GetSpeakerConfig() failed: %08lx\n", rc);
        if (rc==DS_OK && speaker_config!=new_speaker_config && ref_speaker_config!=new_speaker_config)
               trace("IDirectSound_GetSpeakerConfig() failed to set speaker "
               "config: expected 0x%08lx or 0x%08lx, got 0x%08lx\n",
               speaker_config,ref_speaker_config,new_speaker_config);
        IDirectSound_SetSpeakerConfig(dso,ref_speaker_config);
    }

EXIT:
    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",ref);
}

static void IDirectSound_tests(void)
{
    HRESULT rc;
    IDirectSound *dso=(IDirectSound*)0xdeadbeef;
    LPCLASSFACTORY cf=NULL;

    trace("Testing IDirectSound\n");

    rc=CoGetClassObject(&CLSID_DirectSound, CLSCTX_INPROC_SERVER, NULL,
                        &IID_IClassFactory, (void**)&cf);
    ok(rc==S_OK,"CoGetClassObject(CLSID_DirectSound, IID_IClassFactory) "
       "failed: %08lx\n", rc);

    rc=CoGetClassObject(&CLSID_DirectSound, CLSCTX_INPROC_SERVER, NULL,
                        &IID_IUnknown, (void**)&cf);
    ok(rc==S_OK,"CoGetClassObject(CLSID_DirectSound, IID_IUnknown) "
       "failed: %08lx\n", rc);

    /* COM aggregation */
    rc=CoCreateInstance(&CLSID_DirectSound, (IUnknown*)0xdeadbeef, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound, (void**)&dso);
    ok(rc==CLASS_E_NOAGGREGATION || broken(rc==DSERR_INVALIDPARAM),
       "DirectMusicPerformance create failed: %08lx, expected CLASS_E_NOAGGREGATION\n", rc);

    /* try the COM class factory method of creation with no device specified */
    rc=CoCreateInstance(&CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSound) failed: %08lx\n", rc);
    if (dso)
        IDirectSound_test(dso, FALSE, NULL);

    /* try the COM class factory method of creation with default playback
     * device specified */
    rc=CoCreateInstance(&CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSound) failed: %08lx\n", rc);
    if (dso)
        IDirectSound_test(dso, FALSE, &DSDEVID_DefaultPlayback);

    /* try the COM class factory method of creation with default voice
     * playback device specified */
    rc=CoCreateInstance(&CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSound) failed: %08lx\n", rc);
    if (dso)
        IDirectSound_test(dso, FALSE, &DSDEVID_DefaultVoicePlayback);

    /* try the COM class factory method of creation with a bad
     * IID specified */
    rc=CoCreateInstance(&CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
                        &CLSID_DirectSoundPrivate, (void**)&dso);
    ok(rc==E_NOINTERFACE,
       "CoCreateInstance(CLSID_DirectSound,CLSID_DirectSoundPrivate) "
       "should have failed: %08lx\n",rc);

    /* try the COM class factory method of creation with a bad
     * GUID and IID specified */
    rc=CoCreateInstance(&CLSID_DirectSoundPrivate, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound, (void**)&dso);
    ok(rc==REGDB_E_CLASSNOTREG,
       "CoCreateInstance(CLSID_DirectSoundPrivate,IID_IDirectSound) "
       "should have failed: %08lx\n",rc);

    /* try with no device specified */
    rc = DirectSoundCreate(NULL, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCreate(NULL) failed: %08lx\n",rc);
    if (rc==S_OK && dso)
        IDirectSound_test(dso, TRUE, NULL);

    /* try with default playback device specified */
    rc = DirectSoundCreate(&DSDEVID_DefaultPlayback, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCreate(DSDEVID_DefaultPlayback) failed: %08lx\n", rc);
    if (rc==DS_OK && dso)
        IDirectSound_test(dso, TRUE, NULL);

    /* try with default voice playback device specified */
    rc = DirectSoundCreate(&DSDEVID_DefaultVoicePlayback, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCreate(DSDEVID_DefaultVoicePlayback) failed: %08lx\n", rc);
    if (rc==DS_OK && dso)
        IDirectSound_test(dso, TRUE, NULL);

    /* try with a bad device specified */
    rc = DirectSoundCreate(&DSDEVID_DefaultVoiceCapture, &dso, NULL);
    ok(rc==DSERR_NODRIVER,"DirectSoundCreate(DSDEVID_DefaultVoiceCapture) "
       "should have failed: %08lx\n",rc);
    if (rc==DS_OK && dso)
        IDirectSound_Release(dso);
}

static HRESULT test_dsound(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    int ref;

    /* DSOUND: Error: Invalid interface buffer */
    rc = DirectSoundCreate(lpGuid, 0, NULL);
    ok(rc==DSERR_INVALIDPARAM,"DirectSoundCreate() should have returned "
       "DSERR_INVALIDPARAM, returned: %08lx\n",rc);

    /* Create the DirectSound object */
    rc = DirectSoundCreate(lpGuid, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCreate() failed: %08lx\n",rc);
    if (rc!=DS_OK)
        return rc;

    /* Try the enumerated device */
    IDirectSound_test(dso, TRUE, lpGuid);

    /* Try the COM class factory method of creation with enumerated device */
    rc=CoCreateInstance(&CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSound) failed: %08lx\n", rc);
    if (dso)
        IDirectSound_test(dso, FALSE, lpGuid);

    /* Create a DirectSound object */
    rc = DirectSoundCreate(lpGuid, &dso, NULL);
    ok(rc==DS_OK,"DirectSoundCreate() failed: %08lx\n",rc);
    if (rc==DS_OK) {
        LPDIRECTSOUND dso1=NULL;

        /* Create a second DirectSound object */
        rc = DirectSoundCreate(lpGuid, &dso1, NULL);
        ok(rc==DS_OK,"DirectSoundCreate() failed: %08lx\n",rc);
        if (rc==DS_OK) {
            /* Release the second DirectSound object */
            ref=IDirectSound_Release(dso1);
            ok(ref==0,"IDirectSound_Release() has %d references, should have "
               "0\n",ref);
            ok(dso!=dso1,"DirectSound objects should be unique: dso=%p,dso1=%p\n",dso,dso1);
        }

        /* Release the first DirectSound object */
        ref=IDirectSound_Release(dso);
        ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",
           ref);
        if (ref!=0)
            return DSERR_GENERIC;
    } else
        return rc;

    /* Create a DirectSound object */
    rc = DirectSoundCreate(lpGuid, &dso, NULL);
    ok(rc==DS_OK,"DirectSoundCreate() failed: %08lx\n",rc);
    if (rc==DS_OK) {
        LPDIRECTSOUNDBUFFER secondary;
        DSBUFFERDESC bufdesc;
        WAVEFORMATEX wfx;

        init_format(&wfx,WAVE_FORMAT_PCM,11025,8,1);
        ZeroMemory(&bufdesc, sizeof(bufdesc));
        bufdesc.dwSize=sizeof(bufdesc);
        bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRL3D;
        bufdesc.dwBufferBytes=align(wfx.nAvgBytesPerSec*BUFFER_LEN/1000,
                                    wfx.nBlockAlign);
        bufdesc.lpwfxFormat=&wfx;
        rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
        ok((rc==DS_OK && secondary!=NULL) || broken(rc == DSERR_CONTROLUNAVAIL), /* vmware drivers on w2k */
           "IDirectSound_CreateSoundBuffer() failed to create a secondary "
           "buffer %08lx\n",rc);
        if (rc==DS_OK && secondary!=NULL) {
            LPDIRECTSOUND3DBUFFER buffer3d;
            rc = IDirectSoundBuffer_QueryInterface(secondary, &IID_IDirectSound3DBuffer,
                                           (void **)&buffer3d);
            ok(rc==DS_OK && buffer3d!=NULL,"IDirectSound_QueryInterface() "
               "failed: %08lx\n",rc);
            if (rc==DS_OK && buffer3d!=NULL) {
                ref=IDirectSound3DBuffer_AddRef(buffer3d);
                ok(ref==2,"IDirectSound3DBuffer_AddRef() has %d references, "
                   "should have 2\n",ref);
            }
            ref=IDirectSoundBuffer_AddRef(secondary);
            ok(ref==2,"IDirectSoundBuffer_AddRef() has %d references, "
               "should have 2\n",ref);
        }
        /* release with buffer */
        ref=IDirectSound_Release(dso);
        ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",
           ref);
        if (ref!=0)
            return DSERR_GENERIC;
    } else
        return rc;

    return DS_OK;
}

static HRESULT test_primary(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL,second=NULL,third=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    WAVEFORMATEX wfx;
    int ref;

    /* Create the DirectSound object */
    rc = DirectSoundCreate(lpGuid, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate() failed: %08lx\n",rc);
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound_GetCaps() failed: %08lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* DSOUND: Error: Invalid buffer description pointer */
    rc=IDirectSound_CreateSoundBuffer(dso,0,0,NULL);
    ok(rc==DSERR_INVALIDPARAM,
       "IDirectSound_CreateSoundBuffer() should have failed: %08lx\n", rc);

    /* DSOUND: Error: NULL pointer is invalid */
    /* DSOUND: Error: Invalid buffer description pointer */
    rc=IDirectSound_CreateSoundBuffer(dso,0,&primary,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,
       "IDirectSound_CreateSoundBuffer() should have failed: rc=%08lx,"
       "dsbo=%p\n",rc,primary);

    /* DSOUND: Error: Invalid size */
    /* DSOUND: Error: Invalid buffer description */
    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc)-1;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,
       "IDirectSound_CreateSoundBuffer() should have failed: rc=%08lx,"
       "primary=%p\n",rc,primary);

    /* DSOUND: Error: DSBCAPS_PRIMARYBUFFER flag with non-NULL lpwfxFormat */
    /* DSOUND: Error: Invalid buffer description pointer */
    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    bufdesc.lpwfxFormat=&wfx;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,
       "IDirectSound_CreateSoundBuffer() should have failed: rc=%08lx,"
       "primary=%p\n",rc,primary);

    /* DSOUND: Error: No DSBCAPS_PRIMARYBUFFER flag with NULL lpwfxFormat */
    /* DSOUND: Error: Invalid buffer description pointer */
    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=0;
    bufdesc.lpwfxFormat=NULL;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,
       "IDirectSound_CreateSoundBuffer() should have failed: rc=%08lx,"
       "primary=%p\n",rc,primary);

    /* We must call SetCooperativeLevel before calling CreateSoundBuffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* Testing the primary buffer */
    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRLVOLUME;
    bufdesc.lpwfxFormat = &wfx;
    init_format(&wfx,WAVE_FORMAT_PCM,11025,8,2);
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSound_CreateSoundBuffer() should have "
       "returned DSERR_INVALIDPARAM, returned: %08lx\n", rc);
    if (rc==DS_OK && primary!=NULL)
        IDirectSoundBuffer_Release(primary);

    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRLVOLUME;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok((rc==DS_OK && primary!=NULL) || (rc==DSERR_CONTROLUNAVAIL),
       "IDirectSound_CreateSoundBuffer() failed to create a primary buffer: %08lx\n",rc);
    if (rc==DSERR_CONTROLUNAVAIL)
        trace("  No Primary\n");
    else if (rc==DS_OK && primary!=NULL) {
        LONG vol;
        IDirectSoundNotify *notify;

        /* Try to create a second primary buffer */
        /* DSOUND: Error: The primary buffer already exists.
         * Any changes made to the buffer description will be ignored. */
        rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&second,NULL);
        ok(rc==DS_OK && second==primary,
           "IDirectSound_CreateSoundBuffer() should have returned original "
           "primary buffer: %08lx\n",rc);
        ref=IDirectSoundBuffer_Release(second);
        ok(ref==1,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 1\n",ref);

        /* Try to duplicate a primary buffer */
        /* DSOUND: Error: Can't duplicate primary buffers */
        rc=IDirectSound_DuplicateSoundBuffer(dso,primary,&third);
        /* rc=0x88780032 */
        ok(rc!=DS_OK,"IDirectSound_DuplicateSoundBuffer() primary buffer "
           "should have failed %08lx\n",rc);

        rc=IDirectSoundBuffer_GetVolume(primary,&vol);
        ok(rc==DS_OK,"IDirectSoundBuffer_GetVolume() failed: %08lx\n", rc);

        if (winetest_interactive) {
            trace("Playing a 5 seconds reference tone at the current "
                  "volume.\n");
            if (rc==DS_OK)
                trace("(the current volume is %ld according to DirectSound)\n",
                      vol);
            trace("All subsequent tones should be identical to this one.\n");
            trace("Listen for stutter, changes in pitch, volume, etc.\n");
        }
        test_buffer(dso,&primary,1,FALSE,0,FALSE,0,winetest_interactive &&
                    !(dscaps.dwFlags & DSCAPS_EMULDRIVER),5.0,0,0,0,0,FALSE,0);

        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references\n",ref);

        ref=IDirectSoundBuffer_AddRef(primary);
        ok(ref==1,"IDirectSoundBuffer_AddRef() primary has %d references\n",ref);

        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references\n",ref);

        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references\n",ref);

        rc=IDirectSoundBuffer_QueryInterface(primary,&IID_IDirectSoundNotify,(void **)&notify);
        ok(rc==E_NOINTERFACE,"IDirectSoundBuffer_QueryInterface() failed %08lx\n",rc);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);

EXIT:
    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

/*
 * Test the primary buffer at different formats while keeping the
 * secondary buffer at a constant format.
 */
static HRESULT test_primary_secondary(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL,secondary=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    WAVEFORMATEX wfx, wfx2;
    int f,ref,tag;

    /* Create the DirectSound object */
    rc = DirectSoundCreate(lpGuid, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate() failed: %08lx\n",rc);
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound_GetCaps() failed: %08lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before creating primary buffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);
    if (rc!=DS_OK)
        goto EXIT;

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,
       "IDirectSound_CreateSoundBuffer() failed to create a primary buffer %08lx\n",rc);

    if (rc==DS_OK && primary!=NULL) {
        for (f = 0; f < ARRAY_SIZE(formats); f++) {
          for (tag = 0; tag < ARRAY_SIZE(format_tags); tag++) {
            /* if float, we only want to test 32-bit */
            if ((format_tags[tag] == WAVE_FORMAT_IEEE_FLOAT) && (formats[f][1] != 32))
                continue;

            /* We must call SetCooperativeLevel to be allowed to call
             * SetFormat */
            /* DSOUND: Setting DirectSound cooperative level to
             * DSSCL_PRIORITY */
            rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
            ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);
            if (rc!=DS_OK)
                goto EXIT;

            init_format(&wfx,format_tags[tag],formats[f][0],formats[f][1],
                        formats[f][2]);
            wfx2=wfx;
            rc=IDirectSoundBuffer_SetFormat(primary,&wfx);

            if (wfx.wBitsPerSample <= 16)
                ok(rc==DS_OK,"IDirectSoundBuffer_SetFormat(%s) failed: %08lx\n",
                   format_string(&wfx), rc);
            else
                ok(rc==DS_OK || rc == E_INVALIDARG, "SetFormat (%s) failed: %08lx\n",
                   format_string(&wfx), rc);

            /* There is no guarantee that SetFormat will actually change the
             * format to what we asked for. It depends on what the soundcard
             * supports. So we must re-query the format.
             */
            rc=IDirectSoundBuffer_GetFormat(primary,&wfx,sizeof(wfx),NULL);
            ok(rc==DS_OK,"IDirectSoundBuffer_GetFormat() failed: %08lx\n", rc);
            if (rc==DS_OK &&
                (wfx.wFormatTag!=wfx2.wFormatTag ||
                 wfx.nSamplesPerSec!=wfx2.nSamplesPerSec ||
                 wfx.wBitsPerSample!=wfx2.wBitsPerSample ||
                 wfx.nChannels!=wfx2.nChannels)) {
                trace("Requested primary format tag=0x%04x %ldx%dx%d "
                      "avg.B/s=%ld align=%d\n",
                      wfx2.wFormatTag,wfx2.nSamplesPerSec,wfx2.wBitsPerSample,
                      wfx2.nChannels,wfx2.nAvgBytesPerSec,wfx2.nBlockAlign);
                trace("Got tag=0x%04x %ldx%dx%d avg.B/s=%ld align=%d\n",
                      wfx.wFormatTag,wfx.nSamplesPerSec,wfx.wBitsPerSample,
                      wfx.nChannels,wfx.nAvgBytesPerSec,wfx.nBlockAlign);
            }

            /* Set the CooperativeLevel back to normal */
            /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
            rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
            ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);

            init_format(&wfx2,WAVE_FORMAT_PCM,11025,16,2);

            secondary=NULL;
            ZeroMemory(&bufdesc, sizeof(bufdesc));
            bufdesc.dwSize=sizeof(bufdesc);
            bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
            bufdesc.dwBufferBytes=align(wfx.nAvgBytesPerSec*BUFFER_LEN/1000,
                                        wfx.nBlockAlign);
            bufdesc.lpwfxFormat=&wfx2;
            if (winetest_interactive) {
                trace("  Testing a primary buffer at %ldx%dx%d (fmt=%d) with a "
                      "secondary buffer at %ldx%dx%d\n",
                      wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels,format_tags[tag],
                      wfx2.nSamplesPerSec,wfx2.wBitsPerSample,wfx2.nChannels);
            }
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok((rc==DS_OK && secondary!=NULL) || broken(rc == DSERR_CONTROLUNAVAIL), /* vmware drivers on w2k */
               "IDirectSound_CreateSoundBuffer() failed to create a secondary buffer %08lx\n",rc);

            if (rc==DS_OK && secondary!=NULL) {
                test_buffer(dso,&secondary,0,FALSE,0,FALSE,0,
                            winetest_interactive,1.0,0,NULL,0,0,FALSE,0);

                ref=IDirectSoundBuffer_Release(secondary);
                ok(ref==0,"IDirectSoundBuffer_Release() has %d references, "
                   "should have 0\n",ref);
            }
          }
        }

        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);

EXIT:
    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static HRESULT test_secondary(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL,secondary=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    WAVEFORMATEX wfx, wfx1;
    DWORD f, tag;
    int ref;

    /* Create the DirectSound object */
    rc = DirectSoundCreate(lpGuid, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate() failed: %08lx\n",rc);
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound_GetCaps() failed: %08lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before creating primary buffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);
    if (rc!=DS_OK)
        goto EXIT;

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,
       "IDirectSound_CreateSoundBuffer() failed to create a primary buffer %08lx\n",rc);

    if (rc==DS_OK && primary!=NULL) {
        rc=IDirectSoundBuffer_GetFormat(primary,&wfx1,sizeof(wfx1),NULL);
        ok(rc==DS_OK,"IDirectSoundBuffer8_Getformat() failed: %08lx\n", rc);
        if (rc!=DS_OK)
            goto EXIT1;

        for (f = 0; f < ARRAY_SIZE(formats); f++) {
          for (tag = 0; tag < ARRAY_SIZE(format_tags); tag++) {
            WAVEFORMATEXTENSIBLE wfxe;

            /* if float, we only want to test 32-bit */
            if ((format_tags[tag] == WAVE_FORMAT_IEEE_FLOAT) && (formats[f][1] != 32))
                continue;

            init_format(&wfx,format_tags[tag],formats[f][0],formats[f][1],
                        formats[f][2]);
            secondary=NULL;
            ZeroMemory(&bufdesc, sizeof(bufdesc));
            bufdesc.dwSize=sizeof(bufdesc);
            bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
            bufdesc.dwBufferBytes=align(wfx.nAvgBytesPerSec*BUFFER_LEN/1000,
                                        wfx.nBlockAlign);
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DSERR_INVALIDPARAM,"IDirectSound_CreateSoundBuffer() "
               "should have returned DSERR_INVALIDPARAM, returned: %08lx\n", rc);
            if (rc==DS_OK && secondary!=NULL)
                IDirectSoundBuffer_Release(secondary);

            secondary=NULL;
            ZeroMemory(&bufdesc, sizeof(bufdesc));
            bufdesc.dwSize=sizeof(bufdesc);
            bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
            bufdesc.dwBufferBytes=align(wfx.nAvgBytesPerSec*BUFFER_LEN/1000,
                                        wfx.nBlockAlign);
            bufdesc.lpwfxFormat=&wfx;
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);

            if (wfx.wBitsPerSample > 16)
                ok(broken((rc == DSERR_CONTROLUNAVAIL || rc == DSERR_INVALIDCALL || rc == DSERR_INVALIDPARAM /* 2003 */) && !secondary)
                    || rc == DS_OK, /* driver dependent? */
                    "IDirectSound_CreateSoundBuffer() "
                    "should have returned (DSERR_CONTROLUNAVAIL or DSERR_INVALIDCALL) "
                    "and NULL, returned: %08lx %p\n", rc, secondary);
            else
                ok((rc==DS_OK && secondary!=NULL) || broken(rc == DSERR_CONTROLUNAVAIL), /* vmware drivers on w2k */
                    "IDirectSound_CreateSoundBuffer() failed to create a secondary buffer %08lx\n",rc);

            if (secondary)
                IDirectSoundBuffer_Release(secondary);
            secondary = NULL;

            bufdesc.lpwfxFormat=(WAVEFORMATEX*)&wfxe;
            wfxe.Format = wfx;
            wfxe.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
            wfxe.SubFormat = (format_tags[tag] == WAVE_FORMAT_PCM ? KSDATAFORMAT_SUBTYPE_PCM : KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
            wfxe.Format.cbSize = 1;
            wfxe.Samples.wValidBitsPerSample = wfx.wBitsPerSample;
            wfxe.dwChannelMask = (wfx.nChannels == 1 ? KSAUDIO_SPEAKER_MONO : KSAUDIO_SPEAKER_STEREO);

            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok((rc==DSERR_INVALIDPARAM || rc==DSERR_INVALIDCALL /* 2003 */) && !secondary,
                "IDirectSound_CreateSoundBuffer() returned: %08lx %p\n",
                rc, secondary);
            if (secondary)
            {
                IDirectSoundBuffer_Release(secondary);
                secondary=NULL;
            }

            wfxe.Format.cbSize = sizeof(wfxe) - sizeof(wfx) + 1;

            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(((rc==DSERR_CONTROLUNAVAIL || rc==DSERR_INVALIDCALL || rc==DSERR_INVALIDPARAM)
                && !secondary)
               || rc==DS_OK, /* 2003 / 2008 */
                "IDirectSound_CreateSoundBuffer() returned: %08lx %p\n",
                rc, secondary);
            if (secondary)
            {
                IDirectSoundBuffer_Release(secondary);
                secondary=NULL;
            }

            wfxe.Format.cbSize = sizeof(wfxe) - sizeof(wfx);
            wfxe.SubFormat = GUID_NULL;
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok((rc==DSERR_INVALIDPARAM || rc==DSERR_INVALIDCALL) && !secondary,
                "IDirectSound_CreateSoundBuffer() returned: %08lx %p\n",
                rc, secondary);
            if (secondary)
            {
                IDirectSoundBuffer_Release(secondary);
                secondary=NULL;
            }
            wfxe.SubFormat = (format_tags[tag] == WAVE_FORMAT_PCM ? KSDATAFORMAT_SUBTYPE_PCM : KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

            ++wfxe.Samples.wValidBitsPerSample;
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DSERR_INVALIDPARAM && !secondary,
                "IDirectSound_CreateSoundBuffer() returned: %08lx %p\n",
                rc, secondary);
            if (secondary)
            {
                IDirectSoundBuffer_Release(secondary);
                secondary=NULL;
            }
            --wfxe.Samples.wValidBitsPerSample;

            wfxe.Samples.wValidBitsPerSample = 0;
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DS_OK && secondary,
                "IDirectSound_CreateSoundBuffer() returned: %08lx %p\n",
                rc, secondary);
            if (secondary)
            {
                IDirectSoundBuffer_Release(secondary);
                secondary=NULL;
            }
            wfxe.Samples.wValidBitsPerSample = wfxe.Format.wBitsPerSample;

            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DS_OK && secondary!=NULL,
                "IDirectSound_CreateSoundBuffer() failed to create a secondary buffer %08lx\n",rc);

            if (rc==DS_OK && secondary!=NULL) {
                if (winetest_interactive) {
                    trace("  Testing a secondary buffer at %ldx%dx%d (fmt=%d) "
                        "with a primary buffer at %ldx%dx%d\n",
                        wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels,format_tags[tag],
                        wfx1.nSamplesPerSec,wfx1.wBitsPerSample,wfx1.nChannels);
                }
                test_buffer(dso,&secondary,0,FALSE,0,FALSE,0,
                            winetest_interactive,1.0,0,NULL,0,0,FALSE,0);

                ref=IDirectSoundBuffer_Release(secondary);
                ok(ref==0,"IDirectSoundBuffer_Release() has %d references, "
                   "should have 0\n",ref);
            }
          }
        }
EXIT1:
        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);

EXIT:
    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static HRESULT test_block_align(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER secondary=NULL;
    DSBUFFERDESC bufdesc;
    DSBCAPS dsbcaps;
    WAVEFORMATEX wfx;
    DWORD pos, pos2;
    int ref;

    /* Create the DirectSound object */
    rc = DirectSoundCreate(lpGuid, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate() failed: %08lx\n",rc);
    if (rc!=DS_OK)
        return rc;

    init_format(&wfx,WAVE_FORMAT_PCM,11025,16,2);
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
    bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec + 1;
    bufdesc.lpwfxFormat=&wfx;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
    ok(rc == DS_OK || broken(rc == DSERR_CONTROLUNAVAIL), /* vmware drivers on w2k */
       "IDirectSound_CreateSoundBuffer() should have returned DS_OK, returned: %08lx\n", rc);

    if (rc==DS_OK && secondary!=NULL) {
        ZeroMemory(&dsbcaps, sizeof(dsbcaps));
        dsbcaps.dwSize = sizeof(dsbcaps);
        rc=IDirectSoundBuffer_GetCaps(secondary,&dsbcaps);
        ok(rc==DS_OK,"IDirectSoundBuffer_GetCaps() should have returned DS_OK, "
           "returned: %08lx\n", rc);
        if (rc==DS_OK && wfx.nBlockAlign > 1)
        {
            ok(dsbcaps.dwBufferBytes==(wfx.nAvgBytesPerSec + wfx.nBlockAlign),
               "Buffer size not a multiple of nBlockAlign: requested %ld, "
               "got %ld, should be %ld\n", bufdesc.dwBufferBytes,
               dsbcaps.dwBufferBytes, wfx.nAvgBytesPerSec + wfx.nBlockAlign);

            rc = IDirectSoundBuffer_SetCurrentPosition(secondary, 0);
            ok(rc == DS_OK, "Could not set position to 0: %08lx\n", rc);
            rc = IDirectSoundBuffer_GetCurrentPosition(secondary, &pos, NULL);
            ok(rc == DS_OK, "Could not get position: %08lx\n", rc);
            rc = IDirectSoundBuffer_SetCurrentPosition(secondary, 1);
            ok(rc == DS_OK, "Could not set position to 1: %08lx\n", rc);
            rc = IDirectSoundBuffer_GetCurrentPosition(secondary, &pos2, NULL);
            ok(rc == DS_OK, "Could not get new position: %08lx\n", rc);
            ok(pos == pos2, "Positions not the same! Old position: %ld, new position: %ld\n", pos, pos2);

            /* Set position to past the end of the buffer */
            rc = IDirectSoundBuffer_SetCurrentPosition(secondary, wfx.nAvgBytesPerSec + 100);
            ok(rc == E_INVALIDARG, "Set position to %lu succeeded\n", wfx.nAvgBytesPerSec + 100);
            rc = IDirectSoundBuffer_GetCurrentPosition(secondary, &pos2, NULL);
            ok(rc == DS_OK, "Could not get new position: %08lx\n", rc);
            ok(pos == pos2, "Positions not the same! Old position: %ld, new position: %ld\n", pos, pos2);
        }
        ref=IDirectSoundBuffer_Release(secondary);
        ok(ref==0,"IDirectSoundBuffer_Release() secondary has %d references, "
           "should have 0\n",ref);
    }

    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static struct fmt {
    int bits;
    int channels;
} fmts[] = { { 8, 1 }, { 8, 2 }, { 16, 1 }, {16, 2 } };

static HRESULT test_frequency(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL,secondary=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    WAVEFORMATEX wfx, wfx1;
    DWORD f, r;
    int ref;
    int rates[] = { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100,
                    48000, 96000 };

    /* Create the DirectSound object */
    rc = DirectSoundCreate(lpGuid, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate() failed: %08lx\n",rc);
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound_GetCaps() failed: %08lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before creating primary buffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);
    if (rc!=DS_OK)
        goto EXIT;

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,
       "IDirectSound_CreateSoundBuffer() failed to create a primary buffer %08lx\n",rc);

    if (rc==DS_OK && primary!=NULL) {
        rc=IDirectSoundBuffer_GetFormat(primary,&wfx1,sizeof(wfx1),NULL);
        ok(rc==DS_OK,"IDirectSoundBuffer8_Getformat() failed: %08lx\n", rc);
        if (rc!=DS_OK)
            goto EXIT1;

        for (f = 0; f < ARRAY_SIZE(fmts); f++) {
        for (r = 0; r < ARRAY_SIZE(rates); r++) {
            init_format(&wfx,WAVE_FORMAT_PCM,11025,fmts[f].bits,
                        fmts[f].channels);
            secondary=NULL;
            ZeroMemory(&bufdesc, sizeof(bufdesc));
            bufdesc.dwSize=sizeof(bufdesc);
            bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_CTRLFREQUENCY;
            bufdesc.dwBufferBytes=align((wfx.nAvgBytesPerSec*rates[r]/11025)*
                                        BUFFER_LEN/1000,wfx.nBlockAlign);
            bufdesc.lpwfxFormat=&wfx;
            if (winetest_interactive) {
                trace("  Testing a secondary buffer at %ldx%dx%d "
                      "with a primary buffer at %ldx%dx%d\n",
                      wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels,
                      wfx1.nSamplesPerSec,wfx1.wBitsPerSample,wfx1.nChannels);
            }
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok((rc==DS_OK && secondary!=NULL) || broken(rc == DSERR_CONTROLUNAVAIL), /* vmware drivers on w2k */
               "IDirectSound_CreateSoundBuffer() failed to create a secondary buffer %08lx\n",rc);

            if (rc==DS_OK && secondary!=NULL) {
                test_buffer(dso,&secondary,0,FALSE,0,FALSE,0,
                            winetest_interactive,1.0,0,NULL,0,0,TRUE,rates[r]);

                ref=IDirectSoundBuffer_Release(secondary);
                ok(ref==0,"IDirectSoundBuffer_Release() has %d references, "
                   "should have 0\n",ref);
            }
        }
        }
EXIT1:
        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);

EXIT:
    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static HRESULT test_notify(LPDIRECTSOUNDBUFFER dsb,
                           DWORD count, LPHANDLE event,
                           DWORD expected)
{
    HRESULT rc;
    DWORD ret, status;

    rc=IDirectSoundBuffer_SetCurrentPosition(dsb,0);
    ok(rc==DS_OK,
       "IDirectSoundBuffer_SetCurrentPosition failed %08lx\n",rc);
    if(rc!=DS_OK)
        return rc;

    rc=IDirectSoundBuffer_Play(dsb,0,0,0);
    ok(rc==DS_OK,"IDirectSoundBuffer_Play failed %08lx\n",rc);
    if(rc!=DS_OK)
        return rc;

    rc = IDirectSoundBuffer_GetStatus(dsb, &status);
    ok(rc == DS_OK,"Failed %08lx\n",rc);
    ok(status == DSBSTATUS_PLAYING,"got %08lx\n", status);

    rc=IDirectSoundBuffer_Stop(dsb);
    ok(rc==DS_OK,"IDirectSoundBuffer_Stop failed %08lx\n",rc);
    if(rc!=DS_OK)
        return rc;

    rc = IDirectSoundBuffer_GetStatus(dsb, &status);
    ok(rc == DS_OK,"Failed %08lx\n",rc);
    ok(status == 0 /* Stopped */,"got %08lx\n", status);

    ret = WaitForMultipleObjects(count, event, FALSE, 3000);
    ok(ret==expected,"expected %ld. got %ld\n",expected,ret);

    return rc;
}

static HRESULT test_duplicate(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL;
    DSBUFFERDESC bufdesc;
    int ref;

    /* Create the DirectSound object */
    rc = DirectSoundCreate(lpGuid, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate() failed: %08lx\n",rc);
    if (rc!=DS_OK)
        return rc;

    /* We must call SetCooperativeLevel before creating primary buffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);
    if (rc!=DS_OK)
        goto EXIT;

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,"IDirectSound_CreateSoundBuffer() failed "
       "to create a primary buffer %08lx\n",rc);

    if (rc==DS_OK && primary!=NULL) {
        LPDIRECTSOUNDBUFFER original=NULL;
        WAVEFORMATEX wfx;

        init_format(&wfx,WAVE_FORMAT_PCM,22050,16,1);
        ZeroMemory(&bufdesc, sizeof(bufdesc));
        bufdesc.dwSize=sizeof(bufdesc);
        bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_CTRLPOSITIONNOTIFY;
        bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec/100; /* very short buffer */
        bufdesc.lpwfxFormat=&wfx;
        rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&original,NULL);
        ok(rc==DS_OK && original!=NULL,
           "IDirectSound_CreateSoundBuffer() failed to create an original "
           "buffer %08lx\n",rc);
        if (rc==DS_OK && original!=NULL) {
            LPDIRECTSOUNDBUFFER duplicated=NULL;
            LPDIRECTSOUNDNOTIFY notify=NULL;
            HANDLE event[2];
            LPVOID buf=NULL;
            DWORD bufsize;
            int i;

            /* Prepare notify events */
            for (i = 0; i < ARRAY_SIZE(event); i++) {
                event[i] = CreateEventW(NULL, FALSE, FALSE, NULL);
            }

            /* Make silent buffer */
            rc=IDirectSoundBuffer_Lock(original,0,0,&buf,&bufsize,
                                       NULL,NULL,DSBLOCK_ENTIREBUFFER);
            ok(rc==DS_OK && buf!=NULL,
               "IDirectSoundBuffer_Lock failed to lock the buffer %08lx\n",rc);
            if (rc==DS_OK && buf!=NULL) {
                if (sizeof(void*)==4) { /* crashes on 64-bit */
                    /* broken apps like Asuka 120% Return BURNING Fest,
                       pass the pointer to GlobalHandle. */
                    HGLOBAL hmem = GlobalHandle(buf);
                    ok(!hmem,"GlobalHandle should return NULL "
                       "for buffer %p, got %p\n",buf,hmem);
                }
                ZeroMemory(buf,bufsize);
                rc=IDirectSoundBuffer_Unlock(original,buf,bufsize,
                                             NULL,0);
                ok(rc==DS_OK,"IDirectSoundBuffer_Unlock failed to unlock "
                   "%08lx\n",rc);
            }

            rc=IDirectSoundBuffer_QueryInterface(original,
                                                 &IID_IDirectSoundNotify,
                                                 (void**)&notify);
            ok(rc==DS_OK && notify!=NULL,
               "IDirectSoundBuffer_QueryInterface() failed to create a "
               "notification %08lx\n",rc);
            if (rc==DS_OK && notify!=NULL) {
                DSBPOSITIONNOTIFY dsbpn;
                LPDIRECTSOUNDNOTIFY dup_notify=NULL;

                dsbpn.dwOffset=DSBPN_OFFSETSTOP;
                dsbpn.hEventNotify=event[0];
                rc=IDirectSoundNotify_SetNotificationPositions(notify,
                                                               1,&dsbpn);
                ok(rc==DS_OK,"IDirectSoundNotify_SetNotificationPositions "
                   "failed %08lx\n",rc);

                rc=IDirectSound_DuplicateSoundBuffer(dso,original,&duplicated);
                ok(rc==DS_OK && duplicated!=NULL,
                   "IDirectSound_DuplicateSoundBuffer failed %08lx\n",rc);

                trace("testing duplicated buffer without notifications.\n");
                test_notify(duplicated, ARRAY_SIZE(event), event, WAIT_TIMEOUT);

                rc=IDirectSoundBuffer_QueryInterface(duplicated,
                                                     &IID_IDirectSoundNotify,
                                                     (void**)&dup_notify);
                ok(rc==DS_OK&&dup_notify!=NULL,
                   "IDirectSoundBuffer_QueryInterface() failed to create a "
                   "notification %08lx\n",rc);
                if(rc==DS_OK&&dup_notify!=NULL) {
                    dsbpn.dwOffset=DSBPN_OFFSETSTOP;
                    dsbpn.hEventNotify=event[1];
                    rc=IDirectSoundNotify_SetNotificationPositions(dup_notify,
                                                                   1,&dsbpn);
                    ok(rc==DS_OK,"IDirectSoundNotify_SetNotificationPositions "
                       "failed %08lx\n",rc);

                    trace("testing duplicated buffer with a notification.\n");
                    test_notify(duplicated, ARRAY_SIZE(event), event, WAIT_OBJECT_0 + 1);

                    ref=IDirectSoundNotify_Release(dup_notify);
                    ok(ref==0,"IDirectSoundNotify_Release() has %d references, "
                       "should have 0\n",ref);
                }
                ref=IDirectSoundNotify_Release(notify);
                ok(ref==0,"IDirectSoundNotify_Release() has %d references, "
                   "should have 0\n",ref);

                trace("testing original buffer with a notification.\n");
                test_notify(original, ARRAY_SIZE(event), event, WAIT_OBJECT_0);

                ref=IDirectSoundBuffer_Release(duplicated);
                ok(ref==0,"IDirectSoundBuffer_Release() has %d references, "
                   "should have 0\n",ref);
            }
            ref=IDirectSoundBuffer_Release(original);
            ok(ref==0,"IDirectSoundBuffer_Release() has %d references, "
               "should have 0\n",ref);
        }
        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() has %d references, "
           "should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);

EXIT:
    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static HRESULT do_invalid_fmt_test(IDirectSound *dso,
        IDirectSoundBuffer *buf, WAVEFORMATEX *wfx, IDirectSoundBuffer **out_buf)
{
    HRESULT rc;
    *out_buf = NULL;
    if(!buf){
        DSBUFFERDESC bufdesc;
        ZeroMemory(&bufdesc, sizeof(bufdesc));
        bufdesc.dwSize = sizeof(bufdesc);
        bufdesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY;
        bufdesc.dwBufferBytes = 4096;
        bufdesc.lpwfxFormat = wfx;
        rc = IDirectSound_CreateSoundBuffer(dso, &bufdesc, out_buf, NULL);
    }else{
        rc = IDirectSoundBuffer_SetFormat(buf, wfx);
        if(SUCCEEDED(rc)){
            IDirectSoundBuffer_AddRef(buf);
            *out_buf = buf;
        }
    }
    return rc;
}

/* if no buffer is given, use CreateSoundBuffer instead of SetFormat */
static void perform_invalid_fmt_tests(const char *testname, IDirectSound *dso, IDirectSoundBuffer *buf)
{
    WAVEFORMATEX wfx;
    WAVEFORMATEXTENSIBLE fmtex;
    HRESULT rc;
    IDirectSoundBuffer *got_buf;

    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 0;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
    ok(rc == E_INVALIDARG, "%s: SetFormat: %08lx\n", testname, rc);

    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 0;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
    ok(rc == E_INVALIDARG, "%s: SetFormat: %08lx\n", testname, rc);

    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 2;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
    ok(rc == E_INVALIDARG, "%s: SetFormat: %08lx\n", testname, rc);

    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 12;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
    ok(rc == E_INVALIDARG, "%s: SetFormat: %08lx\n", testname, rc);

    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 0;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
    ok(rc == E_INVALIDARG, "%s: SetFormat: %08lx\n", testname, rc);

    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = 0;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
    ok(rc == E_INVALIDARG, "%s: SetFormat: %08lx\n", testname, rc);

    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = 0;
    rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
    ok(rc == E_INVALIDARG, "%s: SetFormat: %08lx\n", testname, rc);

    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample / 8) - 1;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
    ok(rc == E_INVALIDARG, "%s: SetFormat: %08lx\n", testname, rc);

    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample / 8) + 1;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
    ok(rc == E_INVALIDARG, "%s: SetFormat: %08lx\n", testname, rc);

    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign + 1;
    rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
    ok(rc == S_OK, "%s: SetFormat: %08lx\n", testname, rc);

    rc = IDirectSoundBuffer_GetFormat(got_buf, &wfx, sizeof(wfx), NULL);
    ok(rc == S_OK, "%s: GetFormat: %08lx\n", testname, rc);
    ok(wfx.wFormatTag == WAVE_FORMAT_PCM, "%s: format: 0x%x\n", testname, wfx.wFormatTag);
    ok(wfx.nChannels == 2, "%s: channels: %u\n", testname, wfx.nChannels);
    ok(wfx.nSamplesPerSec == 44100, "%s: rate: %lu\n", testname, wfx.nSamplesPerSec);
    ok(wfx.wBitsPerSample == 16, "%s: bps: %u\n", testname, wfx.wBitsPerSample);
    ok(wfx.nBlockAlign == 4, "%s: blockalign: %u\n", testname, wfx.nBlockAlign);
    ok(wfx.nAvgBytesPerSec == 44100 * 4 + 1, "%s: avgbytes: %lu\n", testname, wfx.nAvgBytesPerSec);
    IDirectSoundBuffer_Release(got_buf);

    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign - 1;
    rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
    ok(rc == S_OK, "%s: SetFormat: %08lx\n", testname, rc);

    rc = IDirectSoundBuffer_GetFormat(got_buf, &wfx, sizeof(wfx), NULL);
    ok(rc == S_OK, "%s: GetFormat: %08lx\n", testname, rc);
    ok(wfx.wFormatTag == WAVE_FORMAT_PCM, "%s: format: 0x%x\n", testname, wfx.wFormatTag);
    ok(wfx.nChannels == 2, "%s: channels: %u\n", testname, wfx.nChannels);
    ok(wfx.nSamplesPerSec == 44100, "%s: rate: %lu\n", testname, wfx.nSamplesPerSec);
    ok(wfx.wBitsPerSample == 16, "%s: bps: %u\n", testname, wfx.wBitsPerSample);
    ok(wfx.nBlockAlign == 4, "%s: blockalign: %u\n", testname, wfx.nBlockAlign);
    ok(wfx.nAvgBytesPerSec == 44100 * 4 - 1, "%s: avgbytes: %lu\n", testname, wfx.nAvgBytesPerSec);
    IDirectSoundBuffer_Release(got_buf);

    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign + 1;
    rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
    ok(rc == S_OK, "%s: SetFormat: %08lx\n", testname, rc);

    rc = IDirectSoundBuffer_GetFormat(got_buf, &wfx, sizeof(wfx), NULL);
    ok(rc == S_OK, "%s: GetFormat: %08lx\n", testname, rc);
    ok(wfx.wFormatTag == WAVE_FORMAT_PCM, "%s: format: 0x%x\n", testname, wfx.wFormatTag);
    ok(wfx.nChannels == 2, "%s: channels: %u\n", testname, wfx.nChannels);
    ok(wfx.nSamplesPerSec == 44100, "%s: rate: %lu\n", testname, wfx.nSamplesPerSec);
    ok(wfx.wBitsPerSample == 16, "%s: bps: %u\n", testname, wfx.wBitsPerSample);
    ok(wfx.nBlockAlign == 4, "%s: blockalign: %u\n", testname, wfx.nBlockAlign);
    ok(wfx.nAvgBytesPerSec == 44100 * 4 + 1, "%s: avgbytes: %lu\n", testname, wfx.nAvgBytesPerSec);
    IDirectSoundBuffer_Release(got_buf);

    if(buf){
        wfx.wFormatTag = WAVE_FORMAT_ALAW;
        wfx.nChannels = 2;
        wfx.nSamplesPerSec = 44100;
        wfx.wBitsPerSample = 16;
        wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
        rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
        ok(rc == S_OK, "%s: SetFormat: %08lx\n", testname, rc);

        if(got_buf){
            rc = IDirectSoundBuffer_GetFormat(got_buf, &wfx, sizeof(wfx), NULL);
            ok(rc == S_OK, "%s: GetFormat: %08lx\n", testname, rc);
            ok(wfx.wFormatTag == WAVE_FORMAT_ALAW, "%s: format: 0x%x\n", testname, wfx.wFormatTag);
            ok(wfx.nChannels == 2, "%s: channels: %u\n", testname, wfx.nChannels);
            ok(wfx.nSamplesPerSec == 44100, "%s: rate: %lu\n", testname, wfx.nSamplesPerSec);
            ok(wfx.wBitsPerSample == 16, "%s: bps: %u\n", testname, wfx.wBitsPerSample);
            ok(wfx.nBlockAlign == 4, "%s: blockalign: %u\n", testname, wfx.nBlockAlign);
            ok(wfx.nAvgBytesPerSec == 44100 * 4, "%s: avgbytes: %lu\n", testname, wfx.nAvgBytesPerSec);
            IDirectSoundBuffer_Release(got_buf);
        }
    }

    fmtex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    fmtex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    fmtex.Format.nChannels = 2;
    fmtex.Format.nSamplesPerSec = 44100;
    fmtex.Format.wBitsPerSample = 16;
    fmtex.Format.nBlockAlign = fmtex.Format.nChannels * fmtex.Format.wBitsPerSample / 8;
    fmtex.Format.nAvgBytesPerSec = fmtex.Format.nSamplesPerSec * fmtex.Format.nBlockAlign;
    fmtex.Samples.wValidBitsPerSample = 0;
    fmtex.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    fmtex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    rc = do_invalid_fmt_test(dso, buf, (WAVEFORMATEX*)&fmtex, &got_buf);
    ok(rc == S_OK, "%s: SetFormat: %08lx\n", testname, rc);

    rc = IDirectSoundBuffer_GetFormat(got_buf, (WAVEFORMATEX*)&fmtex, sizeof(fmtex), NULL);
    ok(rc == S_OK, "%s: GetFormat: %08lx\n", testname, rc);
    ok(fmtex.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE, "%s: format: 0x%x\n", testname, fmtex.Format.wFormatTag);
    ok(fmtex.Format.nChannels == 2, "%s: channels: %u\n", testname, fmtex.Format.nChannels);
    ok(fmtex.Format.nSamplesPerSec == 44100, "%s: rate: %lu\n", testname, fmtex.Format.nSamplesPerSec);
    ok(fmtex.Format.wBitsPerSample == 16, "%s: bps: %u\n", testname, fmtex.Format.wBitsPerSample);
    ok(fmtex.Format.nBlockAlign == 4, "%s: blockalign: %u\n", testname, fmtex.Format.nBlockAlign);
    ok(fmtex.Format.nAvgBytesPerSec == 44100 * 4, "%s: avgbytes: %lu\n", testname, fmtex.Format.nAvgBytesPerSec);
    ok(fmtex.Samples.wValidBitsPerSample == 0 || /* <= XP */
            fmtex.Samples.wValidBitsPerSample == 16, /* >= Vista */
            "%s: validbits: %u\n", testname, fmtex.Samples.wValidBitsPerSample);
    ok(IsEqualGUID(&fmtex.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM), "%s: subtype incorrect\n", testname);
    IDirectSoundBuffer_Release(got_buf);

    fmtex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    fmtex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    fmtex.Format.nChannels = 2;
    fmtex.Format.nSamplesPerSec = 44100;
    fmtex.Format.wBitsPerSample = 24;
    fmtex.Format.nBlockAlign = fmtex.Format.nChannels * fmtex.Format.wBitsPerSample / 8;
    fmtex.Format.nAvgBytesPerSec = fmtex.Format.nSamplesPerSec * fmtex.Format.nBlockAlign;
    fmtex.Samples.wValidBitsPerSample = 20;
    fmtex.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    fmtex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    rc = do_invalid_fmt_test(dso, buf, (WAVEFORMATEX*)&fmtex, &got_buf);
    ok(rc == S_OK, "%s: SetFormat: %08lx\n", testname, rc);

    rc = IDirectSoundBuffer_GetFormat(got_buf, (WAVEFORMATEX*)&fmtex, sizeof(fmtex), NULL);
    ok(rc == S_OK, "%s: GetFormat: %08lx\n", testname, rc);
    ok(fmtex.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE, "%s: format: 0x%x\n", testname, fmtex.Format.wFormatTag);
    ok(fmtex.Format.nChannels == 2, "%s: channels: %u\n", testname, fmtex.Format.nChannels);
    ok(fmtex.Format.nSamplesPerSec == 44100, "%s: rate: %lu\n", testname, fmtex.Format.nSamplesPerSec);
    ok(fmtex.Format.wBitsPerSample == 24, "%s: bps: %u\n", testname, fmtex.Format.wBitsPerSample);
    ok(fmtex.Format.nBlockAlign == 6, "%s: blockalign: %u\n", testname, fmtex.Format.nBlockAlign);
    ok(fmtex.Format.nAvgBytesPerSec == 44100 * 6, "%s: avgbytes: %lu\n", testname, fmtex.Format.nAvgBytesPerSec);
    ok(fmtex.Samples.wValidBitsPerSample == 20, "%s: validbits: %u\n", testname, fmtex.Samples.wValidBitsPerSample);
    ok(IsEqualGUID(&fmtex.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM), "%s: subtype incorrect\n", testname);
    IDirectSoundBuffer_Release(got_buf);

    fmtex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    fmtex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    fmtex.Format.nChannels = 2;
    fmtex.Format.nSamplesPerSec = 44100;
    fmtex.Format.wBitsPerSample = 24;
    fmtex.Format.nBlockAlign = fmtex.Format.nChannels * fmtex.Format.wBitsPerSample / 8;
    fmtex.Format.nAvgBytesPerSec = fmtex.Format.nSamplesPerSec * fmtex.Format.nBlockAlign;
    fmtex.Samples.wValidBitsPerSample = 32;
    fmtex.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    fmtex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    rc = do_invalid_fmt_test(dso, buf, (WAVEFORMATEX*)&fmtex, &got_buf);
    ok(rc == E_INVALIDARG, "%s: SetFormat: %08lx\n", testname, rc);

    /* The following 4 tests show that formats with more than two channels require WAVEFORMATEXTENSIBLE */
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
    ok(rc == S_OK, "%s: SetFormat: %08lx\n", testname, rc);
    IDirectSoundBuffer_Release(got_buf);

    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 4;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
    ok(rc == (buf ? DSERR_ALLOCATED : DSERR_INVALIDPARAM), "%s: SetFormat: %08lx\n", testname, rc);

    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 6;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    rc = do_invalid_fmt_test(dso, buf, &wfx, &got_buf);
    ok(rc == (buf ? DSERR_ALLOCATED : DSERR_INVALIDPARAM), "%s: SetFormat: %08lx\n", testname, rc);

    fmtex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    fmtex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    fmtex.Format.nChannels = 6;
    fmtex.Format.nSamplesPerSec = 44100;
    fmtex.Format.wBitsPerSample = 16;
    fmtex.Format.nBlockAlign = fmtex.Format.nChannels * fmtex.Format.wBitsPerSample / 8;
    fmtex.Format.nAvgBytesPerSec = fmtex.Format.nSamplesPerSec * fmtex.Format.nBlockAlign;
    fmtex.Samples.wValidBitsPerSample = fmtex.Format.wBitsPerSample;
    fmtex.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
    fmtex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    rc = do_invalid_fmt_test(dso, buf, (WAVEFORMATEX *)&fmtex, &got_buf);
    ok(rc == S_OK, "%s: SetFormat: %08lx\n", testname, rc);
    IDirectSoundBuffer_Release(got_buf);
}

static HRESULT test_invalid_fmts(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER buffer=NULL;
    DSBUFFERDESC bufdesc;

    /* Create the DirectSound object */
    rc = DirectSoundCreate(lpGuid, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate() failed: %08lx\n",rc);
    if (rc!=DS_OK)
        return rc;

    /* We must call SetCooperativeLevel before creating primary buffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);
    if (rc!=DS_OK){
        IDirectSound_Release(dso);
        return rc;
    }

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&buffer,NULL);
    ok(rc==DS_OK && buffer!=NULL,"IDirectSound_CreateSoundBuffer() failed "
       "to create a primary buffer %08lx\n",rc);
    if (rc==DS_OK && buffer!=NULL) {
        perform_invalid_fmt_tests("primary", dso, buffer);
        IDirectSoundBuffer_Release(buffer);
    }

    perform_invalid_fmt_tests("secondary", dso, NULL);

    IDirectSound_Release(dso);

    return S_OK;
}

static void test_notifications(LPGUID lpGuid)
{
    HRESULT rc;
    IDirectSound *dso;
    IDirectSoundBuffer *buf;
    IDirectSoundNotify *buf_notif;
    DSBUFFERDESC bufdesc;
    WAVEFORMATEX wfx;
    DSBPOSITIONNOTIFY notifies[2];
    HANDLE handles[2];
    DWORD expect, status;
    int cycles;

    rc = DirectSoundCreate(lpGuid, &dso, NULL);
    ok(rc == DS_OK || rc == DSERR_NODRIVER || rc == DSERR_ALLOCATED,
           "DirectSoundCreate() failed: %08lx\n", rc);
    if(rc != DS_OK)
        return;

    rc = IDirectSound_SetCooperativeLevel(dso, get_hwnd(), DSSCL_PRIORITY);
    ok(rc == DS_OK, "IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);
    if(rc != DS_OK){
        IDirectSound_Release(dso);
        return;
    }

    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize = sizeof(bufdesc);
    bufdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
    bufdesc.dwBufferBytes = wfx.nSamplesPerSec * wfx.nBlockAlign / 2; /* 0.5s */
    bufdesc.lpwfxFormat = &wfx;
    rc = IDirectSound_CreateSoundBuffer(dso, &bufdesc, &buf, NULL);
    ok(rc == DS_OK && buf != NULL, "IDirectSound_CreateSoundBuffer() failed "
           "to create a buffer %08lx\n", rc);

    rc = IDirectSoundBuffer_QueryInterface(buf, &IID_IDirectSoundNotify, (void**)&buf_notif);
    ok(rc == E_NOINTERFACE, "QueryInterface(IID_IDirectSoundNotify): %08lx\n", rc);
    IDirectSoundBuffer_Release(buf);

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize = sizeof(bufdesc);
    bufdesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY;
    bufdesc.dwBufferBytes = wfx.nSamplesPerSec * wfx.nBlockAlign / 2; /* 0.5s */
    bufdesc.lpwfxFormat = &wfx;
    rc = IDirectSound_CreateSoundBuffer(dso, &bufdesc, &buf, NULL);
    ok(rc == DS_OK && buf != NULL, "IDirectSound_CreateSoundBuffer() failed "
           "to create a buffer %08lx\n", rc);

    rc = IDirectSoundBuffer_QueryInterface(buf, &IID_IDirectSoundNotify, (void**)&buf_notif);
    ok(rc == DS_OK, "QueryInterface(IID_IDirectSoundNotify): %08lx\n", rc);

    notifies[0].dwOffset = 0;
    handles[0] = notifies[0].hEventNotify = CreateEventW(NULL, FALSE, FALSE, NULL);
    notifies[1].dwOffset = bufdesc.dwBufferBytes / 2;
    handles[1] = notifies[1].hEventNotify = CreateEventW(NULL, FALSE, FALSE, NULL);

    rc = IDirectSoundNotify_SetNotificationPositions(buf_notif, 2, notifies);
    ok(rc == DS_OK, "SetNotificationPositions: %08lx\n", rc);

    IDirectSoundNotify_Release(buf_notif);

    rc = IDirectSoundBuffer_Play(buf, 0, 0, DSBPLAY_LOOPING);
    ok(rc == DS_OK, "Play: %08lx\n", rc);

    expect = 0;
    for(cycles = 0; cycles < 6 /* 1.5s */; ++cycles){
        DWORD wait;

        wait = WaitForMultipleObjects(2, handles, FALSE, 1000);
        ok(wait <= WAIT_OBJECT_0 + 1 && wait - WAIT_OBJECT_0 == expect,
           "Got unexpected notification order or timeout: %lu\n", wait);

        rc = IDirectSoundBuffer_GetStatus(buf, &status);
        ok(rc == DS_OK,"Failed %08lx\n",rc);
        ok(status == (DSBSTATUS_PLAYING | DSBSTATUS_LOOPING),"got %08lx\n", status);

        expect = !expect;
    }

    rc = IDirectSoundBuffer_Stop(buf);
    ok(rc == DS_OK, "Stop: %08lx\n", rc);

    rc = IDirectSoundBuffer_GetStatus(buf, &status);
    ok(rc == DS_OK,"Failed %08lx\n",rc);
    ok(status == 0,"got %08lx\n", status);

    CloseHandle(notifies[0].hEventNotify);
    CloseHandle(notifies[1].hEventNotify);

    IDirectSoundBuffer_Release(buf);
    IDirectSound_Release(dso);
}

static void test_notifications_noloop(LPGUID lpGuid)
{
    HRESULT rc;
    IDirectSound *dso;
    IDirectSoundBuffer *buf;
    IDirectSoundNotify *buf_notif;
    DSBUFFERDESC bufdesc;
    WAVEFORMATEX wfx;
    DSBPOSITIONNOTIFY notifies[2];
    HANDLE handles[2];
    DWORD status, wait;

    rc = DirectSoundCreate(lpGuid, &dso, NULL);
    ok(rc == DS_OK || rc == DSERR_NODRIVER || rc == DSERR_ALLOCATED,
           "DirectSoundCreate() failed: %08lx\n", rc);
    if(rc != DS_OK)
        return;

    rc = IDirectSound_SetCooperativeLevel(dso, get_hwnd(), DSSCL_PRIORITY);
    ok(rc == DS_OK, "IDirectSound_SetCooperativeLevel() failed: %08lx\n", rc);
    if(rc != DS_OK){
        IDirectSound_Release(dso);
        return;
    }

    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize = sizeof(bufdesc);
    bufdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
    bufdesc.dwBufferBytes = wfx.nSamplesPerSec * wfx.nBlockAlign / 2; /* 0.5s */
    bufdesc.lpwfxFormat = &wfx;
    rc = IDirectSound_CreateSoundBuffer(dso, &bufdesc, &buf, NULL);
    ok(rc == DS_OK && buf != NULL, "IDirectSound_CreateSoundBuffer() failed "
           "to create a buffer %08lx\n", rc);

    rc = IDirectSoundBuffer_QueryInterface(buf, &IID_IDirectSoundNotify, (void**)&buf_notif);
    ok(rc == E_NOINTERFACE, "QueryInterface(IID_IDirectSoundNotify): %08lx\n", rc);
    IDirectSoundBuffer_Release(buf);

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize = sizeof(bufdesc);
    bufdesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY;
    bufdesc.dwBufferBytes = wfx.nSamplesPerSec * wfx.nBlockAlign / 2; /* 0.5s */
    bufdesc.lpwfxFormat = &wfx;
    rc = IDirectSound_CreateSoundBuffer(dso, &bufdesc, &buf, NULL);
    ok(rc == DS_OK && buf != NULL, "IDirectSound_CreateSoundBuffer() failed "
           "to create a buffer %08lx\n", rc);

    rc = IDirectSoundBuffer_QueryInterface(buf, &IID_IDirectSoundNotify, (void**)&buf_notif);
    ok(rc == DS_OK, "QueryInterface(IID_IDirectSoundNotify): %08lx\n", rc);

    notifies[0].dwOffset = 0;
    handles[0] = notifies[0].hEventNotify = CreateEventW(NULL, FALSE, FALSE, NULL);
    notifies[1].dwOffset = bufdesc.dwBufferBytes - 4;
    handles[1] = notifies[1].hEventNotify = CreateEventW(NULL, FALSE, FALSE, NULL);

    rc = IDirectSoundNotify_SetNotificationPositions(buf_notif, 2, notifies);
    ok(rc == DS_OK, "SetNotificationPositions: %08lx\n", rc);

    IDirectSoundNotify_Release(buf_notif);

    rc = IDirectSoundBuffer_Play(buf, 0, 0, 0);
    ok(rc == DS_OK, "Play: %08lx\n", rc);

    wait = WaitForMultipleObjects(2, handles, FALSE, 1000);
    ok(wait == WAIT_OBJECT_0, "Got unexpected notification order or timeout: %lu\n", wait);
    rc = IDirectSoundBuffer_GetStatus(buf, &status);
    ok(rc == DS_OK,"Failed %08lx\n",rc);
    ok(status == DSBSTATUS_PLAYING,"got %08lx\n", status);

    wait = WaitForMultipleObjects(2, handles, FALSE, 1000);
    ok(wait == WAIT_OBJECT_0+1, "Got unexpected notification order or timeout: %lu\n", wait);
    rc = IDirectSoundBuffer_GetStatus(buf, &status);
    ok(rc == DS_OK,"Failed %08lx\n",rc);
    ok(status == 0,"got %08lx\n", status);

    rc = IDirectSoundBuffer_Stop(buf);
    ok(rc == DS_OK, "Stop: %08lx\n", rc);

    rc = IDirectSoundBuffer_GetStatus(buf, &status);
    ok(rc == DS_OK,"Failed %08lx\n",rc);
    ok(status == 0,"got %08lx\n", status);

    CloseHandle(notifies[0].hEventNotify);
    CloseHandle(notifies[1].hEventNotify);

    IDirectSoundBuffer_Release(buf);
    IDirectSound_Release(dso);
}

static unsigned int number;

static BOOL WINAPI dsenum_callback(LPGUID lpGuid, LPCSTR lpcstrDescription,
                                   LPCSTR lpcstrModule, LPVOID lpContext)
{
    HRESULT rc;
    trace("*** Testing %s - %s ***\n",lpcstrDescription,lpcstrModule);

    /* Don't test the primary device */
    if (!number++)
    {
        ok (!lpcstrModule[0], "lpcstrModule(%s) != NULL\n", lpcstrModule);
        return TRUE;
    }

    rc = test_dsound(lpGuid);
    if (rc == DSERR_NODRIVER)
        trace("  No Driver\n");
    else if (rc == DSERR_ALLOCATED)
        trace("  Already In Use\n");
    else if (rc == E_FAIL)
        trace("  No Device\n");
    else {
        test_block_align(lpGuid);
        test_primary(lpGuid);
        test_primary_secondary(lpGuid);
        test_secondary(lpGuid);
        test_frequency(lpGuid);
        test_duplicate(lpGuid);
        test_invalid_fmts(lpGuid);
        test_notifications(lpGuid);
        test_notifications_noloop(lpGuid);
    }

    return TRUE;
}

static void dsound_tests(void)
{
    HRESULT rc;
    rc = DirectSoundEnumerateA(&dsenum_callback, NULL);
    ok(rc==DS_OK,"DirectSoundEnumerateA() failed: %08lx\n",rc);
}

static void test_hw_buffers(void)
{
    IDirectSound *ds;
    IDirectSoundBuffer *primary, *primary2, **secondaries, *secondary;
    IDirectSoundBuffer8 *buf8;
    DSCAPS caps;
    DSBCAPS bufcaps;
    DSBUFFERDESC bufdesc;
    WAVEFORMATEX fmt;
    UINT i;
    HRESULT hr;

    hr = DirectSoundCreate(NULL, &ds, NULL);
    ok(hr == S_OK || hr == DSERR_NODRIVER || hr == DSERR_ALLOCATED || hr == E_FAIL,
            "DirectSoundCreate failed: %08lx\n", hr);
    if(hr != S_OK)
        return;

    caps.dwSize = sizeof(caps);

    hr = IDirectSound_GetCaps(ds, &caps);
    ok(hr == S_OK, "GetCaps failed: %08lx\n", hr);

    ok(caps.dwPrimaryBuffers == 1, "Got wrong number of primary buffers: %lu\n",
            caps.dwPrimaryBuffers);

    /* DSBCAPS_LOC* is ignored for primary buffers */
    bufdesc.dwSize = sizeof(bufdesc);
    bufdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCHARDWARE |
        DSBCAPS_PRIMARYBUFFER;
    bufdesc.dwBufferBytes = 0;
    bufdesc.dwReserved = 0;
    bufdesc.lpwfxFormat = NULL;
    bufdesc.guid3DAlgorithm = GUID_NULL;

    hr = IDirectSound_CreateSoundBuffer(ds, &bufdesc, &primary, NULL);
    ok(hr == S_OK, "CreateSoundBuffer failed: %08lx\n", hr);
    if(hr != S_OK){
        IDirectSound_Release(ds);
        return;
    }

    bufdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE |
        DSBCAPS_PRIMARYBUFFER;

    hr = IDirectSound_CreateSoundBuffer(ds, &bufdesc, &primary2, NULL);
    ok(hr == S_OK, "CreateSoundBuffer failed: %08lx\n", hr);
    ok(primary == primary2, "Got different primary buffers: %p, %p\n", primary, primary2);
    if(hr == S_OK)
        IDirectSoundBuffer_Release(primary2);

    buf8 = (IDirectSoundBuffer8 *)0xDEADBEEF;
    hr = IDirectSoundBuffer_QueryInterface(primary, &IID_IDirectSoundBuffer8,
            (void**)&buf8);
    ok(hr == E_NOINTERFACE, "QueryInterface gave wrong failure: %08lx\n", hr);
    ok(buf8 == NULL, "Pointer didn't get set to NULL\n");

    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 48000;
    fmt.wBitsPerSample = 16;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;
    fmt.cbSize = 0;

    bufdesc.lpwfxFormat = &fmt;
    bufdesc.dwBufferBytes = fmt.nSamplesPerSec * fmt.nBlockAlign / 10;
    bufdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCHARDWARE |
        DSBCAPS_CTRLVOLUME;

    secondaries = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(IDirectSoundBuffer *) * caps.dwMaxHwMixingAllBuffers);

    /* try to fill all of the hw buffers */
    trace("dwMaxHwMixingAllBuffers: %lu\n", caps.dwMaxHwMixingAllBuffers);
    trace("dwMaxHwMixingStaticBuffers: %lu\n", caps.dwMaxHwMixingStaticBuffers);
    trace("dwMaxHwMixingStreamingBuffers: %lu\n", caps.dwMaxHwMixingStreamingBuffers);
    for(i = 0; i < caps.dwMaxHwMixingAllBuffers; ++i){
        hr = IDirectSound_CreateSoundBuffer(ds, &bufdesc, &secondaries[i], NULL);
        ok(hr == S_OK || hr == E_NOTIMPL || broken(hr == DSERR_CONTROLUNAVAIL) || broken(hr == E_FAIL),
                "CreateSoundBuffer(%u) failed: %08lx\n", i, hr);
        if(hr != S_OK)
            break;

        bufcaps.dwSize = sizeof(bufcaps);
        hr = IDirectSoundBuffer_GetCaps(secondaries[i], &bufcaps);
        ok(hr == S_OK, "GetCaps failed: %08lx\n", hr);
        ok((bufcaps.dwFlags & DSBCAPS_LOCHARDWARE) != 0,
                "Buffer wasn't allocated in hardware, dwFlags: %lx\n", bufcaps.dwFlags);
    }

    /* see if we can create one more */
    hr = IDirectSound_CreateSoundBuffer(ds, &bufdesc, &secondary, NULL);
    ok((i == caps.dwMaxHwMixingAllBuffers && hr == DSERR_ALLOCATED) || /* out of hw buffers */
            (caps.dwMaxHwMixingAllBuffers == 0 && hr == DSERR_INVALIDCALL) || /* no hw buffers at all */
            hr == E_NOTIMPL || /* don't support hw buffers */
            broken(hr == DSERR_CONTROLUNAVAIL) || /* vmware winxp, others? */
            broken(hr == E_FAIL) || /* broken AC97 driver */
            broken(hr == S_OK) /* broken driver allows more hw bufs than dscaps claims */,
            "CreateSoundBuffer(%u) gave wrong error: %08lx\n", i, hr);
    if(hr == S_OK)
        IDirectSoundBuffer_Release(secondary);

    for(i = 0; i < caps.dwMaxHwMixingAllBuffers; ++i)
        if(secondaries[i])
            IDirectSoundBuffer_Release(secondaries[i]);
    HeapFree(GetProcessHeap(), 0, secondaries);

    IDirectSoundBuffer_Release(primary);
    IDirectSound_Release(ds);
}

static void test_implicit_mta(void)
{
    HRESULT hr;
    IDirectSound *dso;
    struct apt_data test_apt_data;

    check_apttype(&test_apt_data);
    ok(test_apt_data.type == APTTYPE_UNITIALIZED, "got apt type %d.\n", test_apt_data.type);

    /* test DirectSound object */
    hr = CoCreateInstance(&CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDirectSound, (void**)&dso);
    ok(hr == S_OK, "CoCreateInstance(CLSID_DirectSound) failed: %08lx\n", hr);

    check_apttype(&test_apt_data);
    ok(test_apt_data.type == APTTYPE_UNITIALIZED, "got apt type %d.\n", test_apt_data.type);

    hr = IDirectSound_Initialize(dso, NULL);
    ok(hr == DS_OK || hr == DSERR_NODRIVER || hr == DSERR_ALLOCATED || hr == E_FAIL,
       "IDirectSound_Initialize() failed: %08lx\n", hr);
    if (hr == DS_OK) {
        check_apttype(&test_apt_data);
        ok(test_apt_data.type == APTTYPE_MTA, "got apt type %d.\n", test_apt_data.type);
        ok(test_apt_data.qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA,
           "got apt type qualifier %d.\n", test_apt_data.qualifier);
    }
    IDirectSound_Release(dso);

    check_apttype(&test_apt_data);
    ok(test_apt_data.type == APTTYPE_UNITIALIZED, "got apt type %d.\n", test_apt_data.type);

    /* test DirectSoundCreate */
    hr = DirectSoundCreate(NULL, &dso, NULL);
    ok(hr == DS_OK || hr == DSERR_NODRIVER || hr == DSERR_ALLOCATED || hr == E_FAIL,
       "DirectSoundCreate() failed: %08lx\n", hr);
    if (hr == DS_OK) {
        check_apttype(&test_apt_data);
        ok(test_apt_data.type == APTTYPE_MTA, "got apt type %d.\n", test_apt_data.type);
        ok(test_apt_data.qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA,
           "got apt type qualifier %d.\n", test_apt_data.qualifier);
        IDirectSound_Release(dso);
    }

    check_apttype(&test_apt_data);
    ok(test_apt_data.type == APTTYPE_UNITIALIZED, "got apt type %d.\n", test_apt_data.type);
}

START_TEST(dsound)
{
    CoInitialize(NULL);

    /* Run implicit MTA tests before IDirectSound_test so that a MTA won't be created before this test is run. */
    test_implicit_mta();
    IDirectSound_tests();
    dsound_tests();
    test_hw_buffers();

    CoUninitialize();
}
