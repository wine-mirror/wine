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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include <windows.h>

#include <math.h>
#include <stdlib.h>

#include "wine/test.h"
#include "windef.h"
#include "wingdi.h"
#include "dsound.h"
#include "dxerr8.h"

#include "dsound_test.h"

static void dsound_dsound_tests()
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    DSCAPS dscaps;
    int ref;
    IUnknown * unknown;
    IDirectSound * ds;
    IDirectSound8 * ds8;

    /* try the COM class factory method of creation */
    rc=CoCreateInstance(&CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectSound, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance failed: %s\n",DXGetErrorString8(rc));
    if (dso) {
        /* Try to Query for objects */
        rc=IDirectSound_QueryInterface(dso,&IID_IUnknown,(LPVOID*)&unknown);
        ok(rc==DS_OK,"IDirectSound_QueryInterface(IID_IUnknown) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound_Release(unknown);

        rc=IDirectSound_QueryInterface(dso,&IID_IDirectSound,(LPVOID*)&ds);
        ok(rc==DS_OK,"IDirectSound_QueryInterface(IID_IDirectSound) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound_Release(ds);

        rc=IDirectSound_QueryInterface(dso,&IID_IDirectSound8,(LPVOID*)&ds8);
        ok(rc==E_NOINTERFACE,"IDirectSound_QueryInterface(IID_IDirectSound8) should have failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound8_Release(ds8);

        /* try unitialized object */
        rc=IDirectSound_GetCaps(dso,0);
        ok(rc==DSERR_UNINITIALIZED,"GetCaps should have returned DSERR_UNINITIALIZED, returned: %s\n",DXGetErrorString8(rc));

        rc=IDirectSound_Initialize(dso,NULL);
        ok(rc==DS_OK,"IDirectSound_Initialize(NULL) failed: %s\n",DXGetErrorString8(rc));

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound_GetCaps(dso,0);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        ZeroMemory(&dscaps, sizeof(dscaps));

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound_GetCaps(dso,&dscaps);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        dscaps.dwSize=sizeof(dscaps);

        /* DSOUND: Running on a certified driver */
        rc=IDirectSound_GetCaps(dso,&dscaps);
        ok(rc==DS_OK,"GetCaps failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK) {
            trace("  DirectSound Caps: flags=0x%08lx secondary min=%ld max=%ld\n",
                  dscaps.dwFlags,dscaps.dwMinSecondarySampleRate,
                  dscaps.dwMaxSecondarySampleRate);
        }

        ref=IDirectSound_Release(dso);
        ok(ref==0,"IDirectSound_Release has %d references, should have 0\n",ref);
    }

    /* try with no device specified */
    rc=DirectSoundCreate(NULL,&dso,NULL);
    ok(rc==S_OK,"DirectSoundCreate(NULL) failed: %s\n",DXGetErrorString8(rc));
    if (dso) {
        /* Try to Query for objects */
        rc=IDirectSound_QueryInterface(dso,&IID_IUnknown,(LPVOID*)&unknown);
        ok(rc==DS_OK,"IDirectSound_QueryInterface(IID_IUnknown) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound_Release(unknown);

        rc=IDirectSound_QueryInterface(dso,&IID_IDirectSound,(LPVOID*)&ds);
        ok(rc==DS_OK,"IDirectSound_QueryInterface(IID_IDirectSound) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound_Release(ds);

        rc=IDirectSound_QueryInterface(dso,&IID_IDirectSound8,(LPVOID*)&ds8);
        ok(rc==E_NOINTERFACE,"IDirectSound_QueryInterface(IID_IDirectSound8) should have failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound8_Release(ds8);

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound_GetCaps(dso,0);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        ZeroMemory(&dscaps, sizeof(dscaps));

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound_GetCaps(dso,&dscaps);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        dscaps.dwSize=sizeof(dscaps);

        /* DSOUND: Running on a certified driver */
        rc=IDirectSound_GetCaps(dso,&dscaps);
        ok(rc==DS_OK,"GetCaps failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK) {
            trace("  DirectSound Caps: flags=0x%08lx secondary min=%ld max=%ld\n",
                  dscaps.dwFlags,dscaps.dwMinSecondarySampleRate,
                  dscaps.dwMaxSecondarySampleRate);
        }

        ref=IDirectSound_Release(dso);
        ok(ref==0,"IDirectSound_Release has %d references, should have 0\n",ref);
    }

    /* try with default playback device specified */
    rc=DirectSoundCreate(&DSDEVID_DefaultPlayback,&dso,NULL);
    ok(rc==S_OK,"DirectSoundCreate(DSDEVID_DefaultPlayback) failed: %s\n",DXGetErrorString8(rc));
    if (dso) {
        /* Try to Query for objects */
        rc=IDirectSound_QueryInterface(dso,&IID_IUnknown,(LPVOID*)&unknown);
        ok(rc==DS_OK,"IDirectSound_QueryInterface(IID_IUnknown) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound_Release(unknown);

        rc=IDirectSound_QueryInterface(dso,&IID_IDirectSound,(LPVOID*)&ds);
        ok(rc==DS_OK,"IDirectSound_QueryInterface(IID_IDirectSound) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound_Release(ds);

        rc=IDirectSound_QueryInterface(dso,&IID_IDirectSound8,(LPVOID*)&ds8);
        ok(rc==E_NOINTERFACE,"IDirectSound_QueryInterface(IID_IDirectSound8) should have failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound8_Release(ds8);

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound_GetCaps(dso,0);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        ZeroMemory(&dscaps, sizeof(dscaps));

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound_GetCaps(dso,&dscaps);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        dscaps.dwSize=sizeof(dscaps);

        /* DSOUND: Running on a certified driver */
        rc=IDirectSound_GetCaps(dso,&dscaps);
        ok(rc==DS_OK,"GetCaps failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK) {
            trace("  DirectSound Caps: flags=0x%08lx secondary min=%ld max=%ld\n",
                  dscaps.dwFlags,dscaps.dwMinSecondarySampleRate,
                  dscaps.dwMaxSecondarySampleRate);
        }

        ref=IDirectSound_Release(dso);
        ok(ref==0,"IDirectSound_Release has %d references, should have 0\n",ref);
    }

    /* try with default voice playback device specified */
    rc=DirectSoundCreate(&DSDEVID_DefaultVoicePlayback,&dso,NULL);
    ok(rc==S_OK,"DirectSoundCreate(DSDEVID_DefaultVoicePlayback) failed: %s\n",DXGetErrorString8(rc));
    if (dso) {
        /* Try to Query for objects */
        rc=IDirectSound_QueryInterface(dso,&IID_IUnknown,(LPVOID*)&unknown);
        ok(rc==DS_OK,"IDirectSound_QueryInterface(IID_IUnknown) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound_Release(unknown);

        rc=IDirectSound_QueryInterface(dso,&IID_IDirectSound,(LPVOID*)&ds);
        ok(rc==DS_OK,"IDirectSound_QueryInterface(IID_IDirectSound) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound_Release(ds);

        rc=IDirectSound_QueryInterface(dso,&IID_IDirectSound8,(LPVOID*)&ds8);
        ok(rc==E_NOINTERFACE,"IDirectSound_QueryInterface(IID_IDirectSound8) should have failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound8_Release(ds8);

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound_GetCaps(dso,0);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        ZeroMemory(&dscaps, sizeof(dscaps));

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound_GetCaps(dso,&dscaps);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        dscaps.dwSize=sizeof(dscaps);

        /* DSOUND: Running on a certified driver */
        rc=IDirectSound_GetCaps(dso,&dscaps);
        ok(rc==DS_OK,"GetCaps failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK) {
            trace("  DirectSound Caps: flags=0x%08lx secondary min=%ld max=%ld\n",
                  dscaps.dwFlags,dscaps.dwMinSecondarySampleRate,
                  dscaps.dwMaxSecondarySampleRate);
        }

        ref=IDirectSound_Release(dso);
        ok(ref==0,"IDirectSound_Release has %d references, should have 0\n",ref);
    }
}

static void dsound_dsound8_tests()
{
    HRESULT rc;
    LPDIRECTSOUND8 dso=NULL;
    DSCAPS dscaps;
    int ref;
    IUnknown * unknown;
    IDirectSound * ds;
    IDirectSound8 * ds8;

    /* try the COM class factory method of creation */
    rc=CoCreateInstance(&CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectSound8, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance failed: %s\n",DXGetErrorString8(rc));
    if (dso) {
        /* Try to Query for objects */
        rc=IDirectSound8_QueryInterface(dso,&IID_IUnknown,(LPVOID*)&unknown);
        ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IUnknown) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound8_Release(unknown);

        rc=IDirectSound8_QueryInterface(dso,&IID_IDirectSound,(LPVOID*)&ds);
        ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IDirectSound) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound_Release(ds);

        rc=IDirectSound8_QueryInterface(dso,&IID_IDirectSound8,(LPVOID*)&ds8);
        ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IDirectSound8) should have failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound8_Release(ds8);

        /* try unitialized object */
        rc=IDirectSound8_GetCaps(dso,0);
        ok(rc==DSERR_UNINITIALIZED,"GetCaps should have returned DSERR_UNINITIALIZED, returned: %s\n",DXGetErrorString8(rc));

        rc=IDirectSound8_Initialize(dso,NULL);
        ok(rc==DS_OK,"IDirectSound_Initialize(NULL) failed: %s\n",DXGetErrorString8(rc));

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound8_GetCaps(dso,0);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        ZeroMemory(&dscaps, sizeof(dscaps));

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound8_GetCaps(dso,&dscaps);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        dscaps.dwSize=sizeof(dscaps);

        /* DSOUND: Running on a certified driver */
        rc=IDirectSound8_GetCaps(dso,&dscaps);
        ok(rc==DS_OK,"GetCaps failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK) {
            trace("  DirectSound Caps: flags=0x%08lx secondary min=%ld max=%ld\n",
                  dscaps.dwFlags,dscaps.dwMinSecondarySampleRate,
                  dscaps.dwMaxSecondarySampleRate);
        }

        ref=IDirectSound8_Release(dso);
        ok(ref==0,"IDirectSound8_Release has %d references, should have 0\n",ref);
    }

    /* try with no device specified */
    rc=DirectSoundCreate8(NULL,&dso,NULL);
    ok(rc==S_OK,"DirectSoundCreate8 failed: %s\n",DXGetErrorString8(rc));
    if (dso) {
        /* Try to Query for objects */
        rc=IDirectSound8_QueryInterface(dso,&IID_IUnknown,(LPVOID*)&unknown);
        ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IUnknown) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound8_Release(unknown);

        rc=IDirectSound8_QueryInterface(dso,&IID_IDirectSound,(LPVOID*)&ds);
        ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IDirectSound) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound_Release(ds);

        rc=IDirectSound8_QueryInterface(dso,&IID_IDirectSound8,(LPVOID*)&ds8);
        ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IDirectSound8) should have failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound8_Release(ds8);

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound8_GetCaps(dso,0);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        ZeroMemory(&dscaps, sizeof(dscaps));

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound8_GetCaps(dso,&dscaps);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        dscaps.dwSize=sizeof(dscaps);

        /* DSOUND: Running on a certified driver */
        rc=IDirectSound8_GetCaps(dso,&dscaps);
        ok(rc==DS_OK,"GetCaps failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK) {
            trace("  DirectSound Caps: flags=0x%08lx secondary min=%ld max=%ld\n",
                  dscaps.dwFlags,dscaps.dwMinSecondarySampleRate,
                  dscaps.dwMaxSecondarySampleRate);
        }

        ref=IDirectSound8_Release(dso);
        ok(ref==0,"IDirectSound_Release has %d references, should have 0\n",ref);
    }

    /* try with default playback device specified */
    rc=DirectSoundCreate8(&DSDEVID_DefaultPlayback,&dso,NULL);
    ok(rc==S_OK,"DirectSoundCreate8 failed: %s\n",DXGetErrorString8(rc));
    if (dso) {
        /* Try to Query for objects */
        rc=IDirectSound8_QueryInterface(dso,&IID_IUnknown,(LPVOID*)&unknown);
        ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IUnknown) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound8_Release(unknown);

        rc=IDirectSound8_QueryInterface(dso,&IID_IDirectSound,(LPVOID*)&ds);
        ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IDirectSound) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound_Release(ds);

        rc=IDirectSound8_QueryInterface(dso,&IID_IDirectSound8,(LPVOID*)&ds8);
        ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IDirectSound8) should have failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound8_Release(ds8);

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound8_GetCaps(dso,0);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        ZeroMemory(&dscaps, sizeof(dscaps));

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound8_GetCaps(dso,&dscaps);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        dscaps.dwSize=sizeof(dscaps);

        /* DSOUND: Running on a certified driver */
        rc=IDirectSound8_GetCaps(dso,&dscaps);
        ok(rc==DS_OK,"GetCaps failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK) {
            trace("  DirectSound Caps: flags=0x%08lx secondary min=%ld max=%ld\n",
                  dscaps.dwFlags,dscaps.dwMinSecondarySampleRate,
                  dscaps.dwMaxSecondarySampleRate);
        }

        ref=IDirectSound8_Release(dso);
        ok(ref==0,"IDirectSound_Release has %d references, should have 0\n",ref);
    }

    /* try with default voice playback device specified */
    rc=DirectSoundCreate8(&DSDEVID_DefaultVoicePlayback,&dso,NULL);
    ok(rc==S_OK,"DirectSoundCreate8 failed: %s\n",DXGetErrorString8(rc));
    if (dso) {
        /* Try to Query for objects */
        rc=IDirectSound8_QueryInterface(dso,&IID_IUnknown,(LPVOID*)&unknown);
        ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IUnknown) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound8_Release(unknown);

        rc=IDirectSound8_QueryInterface(dso,&IID_IDirectSound,(LPVOID*)&ds);
        ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IDirectSound) failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound_Release(ds);

        rc=IDirectSound8_QueryInterface(dso,&IID_IDirectSound8,(LPVOID*)&ds8);
        ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IDirectSound8) should have failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK)
            IDirectSound8_Release(ds8);

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound8_GetCaps(dso,0);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        ZeroMemory(&dscaps, sizeof(dscaps));

        /* DSOUND: Error: Invalid caps buffer */
        rc=IDirectSound8_GetCaps(dso,&dscaps);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

        dscaps.dwSize=sizeof(dscaps);

        /* DSOUND: Running on a certified driver */
        rc=IDirectSound8_GetCaps(dso,&dscaps);
        ok(rc==DS_OK,"GetCaps failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK) {
            trace("  DirectSound Caps: flags=0x%08lx secondary min=%ld max=%ld\n",
                  dscaps.dwFlags,dscaps.dwMinSecondarySampleRate,
                  dscaps.dwMaxSecondarySampleRate);
        }

        ref=IDirectSound8_Release(dso);
        ok(ref==0,"IDirectSound_Release has %d references, should have 0\n",ref);
    }
}

static HRESULT test_dsound(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    DSCAPS dscaps;
    int ref;
    IUnknown * unknown;
    IDirectSound * ds;
    IDirectSound8 * ds8;

    /* DSOUND: Error: Invalid interface buffer */
    rc=DirectSoundCreate(lpGuid,0,NULL);
    ok(rc==DSERR_INVALIDPARAM,"DirectSoundCreate should have failed: %s\n",DXGetErrorString8(rc));

    /* Create the DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return rc;

    /* Try to Query for objects */
    rc=IDirectSound_QueryInterface(dso,&IID_IUnknown,(LPVOID*)&unknown);
    ok(rc==DS_OK,"IDirectSound_QueryInterface(IID_IUnknown) failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK)
        IDirectSound_Release(unknown);

    rc=IDirectSound_QueryInterface(dso,&IID_IDirectSound,(LPVOID*)&ds);
    ok(rc==DS_OK,"IDirectSound_QueryInterface(IID_IDirectSound) failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK)
        IDirectSound_Release(ds);

    rc=IDirectSound_QueryInterface(dso,&IID_IDirectSound8,(LPVOID*)&ds8);
    ok(rc==E_NOINTERFACE,"IDirectSound_QueryInterface(IID_IDirectSound8) should have failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK)
        IDirectSound8_Release(ds8);

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound_GetCaps(dso,0);
    ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

    ZeroMemory(&dscaps, sizeof(dscaps));

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

    dscaps.dwSize=sizeof(dscaps);

    /* DSOUND: Running on a certified driver */
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"GetCaps failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK) {
        trace("  DirectSound Caps: flags=0x%08lx secondary min=%ld max=%ld\n",
              dscaps.dwFlags,dscaps.dwMinSecondarySampleRate,
              dscaps.dwMaxSecondarySampleRate);
    }

    /* Release the DirectSound object */
    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    /* Create a DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK) {
        LPDIRECTSOUND dso1=NULL;

        /* Create a second DirectSound object */
        rc=DirectSoundCreate(lpGuid,&dso1,NULL);
        ok(rc==DS_OK,"DirectSoundCreate failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK) {
            /* Release the second DirectSound object */
            ref=IDirectSound_Release(dso1);
            ok(ref==0,"IDirectSound_Release has %d references, should have 0\n",ref);
            ok(dso!=dso1,"DirectSound objects should be unique: dso=0x%08lx,dso1=0x%08lx\n",(DWORD)dso,(DWORD)dso1);
        }

        /* Release the first DirectSound object */
        ref=IDirectSound_Release(dso);
        ok(ref==0,"IDirectSound_Release has %d references, should have 0\n",ref);
        if (ref!=0)
            return DSERR_GENERIC;
    } else
        return rc;

    /* Create a DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK) {
        LPDIRECTSOUNDBUFFER secondary;
        DSBUFFERDESC bufdesc;
        WAVEFORMATEX wfx;

        init_format(&wfx,WAVE_FORMAT_PCM,11025,8,1);
        ZeroMemory(&bufdesc, sizeof(bufdesc));
        bufdesc.dwSize=sizeof(bufdesc);
        bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRL3D;
        bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec*BUFFER_LEN/1000;
        bufdesc.lpwfxFormat=&wfx;
        rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
        ok(rc==DS_OK && secondary!=NULL,"CreateSoundBuffer failed to create a secondary buffer 0x%lx\n", rc);
        if (rc==DS_OK && secondary!=NULL) {
            LPDIRECTSOUND3DBUFFER buffer3d;
            rc=IDirectSound_QueryInterface(secondary, &IID_IDirectSound3DBuffer, (void **)&buffer3d);
            ok(rc==DS_OK && buffer3d!=NULL,"QueryInterface failed:  %s\n",DXGetErrorString8(rc));
            if (rc==DS_OK && buffer3d!=NULL) {
                ref=IDirectSound3DBuffer_AddRef(buffer3d);
                ok(ref==2,"IDirectSound3DBuffer_AddRef has %d references, should have 2\n",ref);
            }
            ref=IDirectSoundBuffer_AddRef(secondary);
            ok(ref==2,"IDirectSoundBuffer_AddRef has %d references, should have 2\n",ref);
        }
        /* release with buffer */
        ref=IDirectSound_Release(dso);
        ok(ref==0,"IDirectSound_Release has %d references, should have 0\n",ref);
        if (ref!=0)
            return DSERR_GENERIC;
    } else
        return rc;

    return DS_OK;
}

static HRESULT test_dsound8(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND8 dso=NULL;
    DSCAPS dscaps;
    int ref;
    IUnknown * unknown;
    IDirectSound * ds;
    IDirectSound8 * ds8;

    /* DSOUND: Error: Invalid interface buffer */
    rc=DirectSoundCreate8(lpGuid,0,NULL);
    ok(rc==DSERR_INVALIDPARAM,"DirectSoundCreate8 should have failed: %s\n",DXGetErrorString8(rc));

    /* Create the DirectSound8 object */
    rc=DirectSoundCreate8(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate8 failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return rc;

    /* Try to Query for objects */
    rc=IDirectSound8_QueryInterface(dso,&IID_IUnknown,(LPVOID*)&unknown);
    ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IUnknown) failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK)
        IDirectSound8_Release(unknown);

    rc=IDirectSound8_QueryInterface(dso,&IID_IDirectSound,(LPVOID*)&ds);
    ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IDirectSound) failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK)
        IDirectSound_Release(ds);

    rc=IDirectSound8_QueryInterface(dso,&IID_IDirectSound8,(LPVOID*)&ds8);
    ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IDirectSound8) failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK)
        IDirectSound8_Release(ds8);

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound8_GetCaps(dso,0);
    ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

    ZeroMemory(&dscaps, sizeof(dscaps));

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound8_GetCaps(dso,&dscaps);
    ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %s\n",DXGetErrorString8(rc));

    dscaps.dwSize=sizeof(dscaps);

    /* DSOUND: Running on a certified driver */
    rc=IDirectSound8_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"GetCaps failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK) {
        trace("  DirectSound Caps: flags=0x%08lx secondary min=%ld max=%ld\n",
              dscaps.dwFlags,dscaps.dwMinSecondarySampleRate,
              dscaps.dwMaxSecondarySampleRate);
    }

    /* Release the DirectSound8 object */
    ref=IDirectSound8_Release(dso);
    ok(ref==0,"IDirectSound_Release has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    /* Create a DirectSound8 object */
    rc=DirectSoundCreate8(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK) {
        LPDIRECTSOUND8 dso1=NULL;

        /* Create a second DirectSound8 object */
        rc=DirectSoundCreate8(lpGuid,&dso1,NULL);
        ok(rc==DS_OK,"DirectSoundCreate8 failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK) {
            /* Release the second DirectSound8 object */
            ref=IDirectSound8_Release(dso1);
            ok(ref==0,"IDirectSound8_Release has %d references, should have 0\n",ref);
            ok(dso!=dso1,"DirectSound8 objects should be unique: dso=0x%08lx,dso1=0x%08lx\n",(DWORD)dso,(DWORD)dso1);
        }

        /* Release the first DirectSound8 object */
        ref=IDirectSound8_Release(dso);
        ok(ref==0,"IDirectSound8_Release has %d references, should have 0\n",ref);
        if (ref!=0)
            return DSERR_GENERIC;
    } else
        return rc;

    /* Create a DirectSound8 object */
    rc=DirectSoundCreate8(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate8 failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK) {
        LPDIRECTSOUNDBUFFER secondary;
        DSBUFFERDESC bufdesc;
        WAVEFORMATEX wfx;

        init_format(&wfx,WAVE_FORMAT_PCM,11025,8,1);
        ZeroMemory(&bufdesc, sizeof(bufdesc));
        bufdesc.dwSize=sizeof(bufdesc);
        bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRL3D;
        bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec*BUFFER_LEN/1000;
        bufdesc.lpwfxFormat=&wfx;
        rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
        ok(rc==DS_OK && secondary!=NULL,"CreateSoundBuffer failed to create a secondary buffer 0x%lx\n", rc);
        if (rc==DS_OK && secondary!=NULL) {
            LPDIRECTSOUND3DBUFFER buffer3d;
            LPDIRECTSOUNDBUFFER8 buffer8;
            rc=IDirectSound_QueryInterface(secondary, &IID_IDirectSound3DBuffer, (void **)&buffer3d);
            ok(rc==DS_OK && buffer3d!=NULL,"QueryInterface failed:  %s\n",DXGetErrorString8(rc));
            if (rc==DS_OK && buffer3d!=NULL) {
                ref=IDirectSound3DBuffer_AddRef(buffer3d);
                ok(ref==2,"IDirectSound3DBuffer_AddRef has %d references, should have 2\n",ref);
            }
            rc=IDirectSound_QueryInterface(secondary, &IID_IDirectSoundBuffer8, (void **)&buffer8);
            if (rc==DS_OK && buffer8!=NULL) {
                ref=IDirectSoundBuffer8_AddRef(buffer8);
                ok(ref==3,"IDirectSoundBuffer8_AddRef has %d references, should have 3\n",ref);
            }
            ref=IDirectSoundBuffer_AddRef(secondary);
            ok(ref==4,"IDirectSoundBuffer_AddRef has %d references, should have 4\n",ref);
        }
        /* release with buffer */
        ref=IDirectSound8_Release(dso);
        ok(ref==0,"IDirectSound8_Release has %d references, should have 0\n",ref);
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
    int ref;

    /* Create the DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"GetCaps failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* DSOUND: Error: Invalid buffer description pointer */
    rc=IDirectSound_CreateSoundBuffer(dso,0,0,NULL);
    ok(rc==DSERR_INVALIDPARAM,"CreateSoundBuffer should have failed: 0x%lx\n",rc);

    /* DSOUND: Error: Invalid buffer description pointer */
    rc=IDirectSound_CreateSoundBuffer(dso,0,&primary,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,"CreateSoundBuffer should have failed: rc=0x%lx,dsbo=0x%lx\n",rc,(DWORD)primary);

    /* DSOUND: Error: Invalid buffer description pointer */
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,0,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,"CreateSoundBuffer should have failed: rc=0x%lx,dsbo=0x%lx\n",rc,(DWORD)primary);

    ZeroMemory(&bufdesc, sizeof(bufdesc));

    /* DSOUND: Error: Invalid size */
    /* DSOUND: Error: Invalid buffer description */
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,"CreateSoundBuffer should have failed: rc=0x%lx,primary=0x%lx\n",rc,(DWORD)primary);

    /* We must call SetCooperativeLevel before calling CreateSoundBuffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"SetCooperativeLevel failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* Testing the primary buffer */
    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRLVOLUME;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,"CreateSoundBuffer failed to create a primary buffer: 0x%lx\n",rc);
    if (rc==DS_OK && primary!=NULL) {
        LONG vol;

        /* Try to create a second primary buffer */
        /* DSOUND: Error: The primary buffer already exists.  Any changes made to the buffer description will be ignored. */
        rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&second,NULL);
        ok(rc==DS_OK && second==primary,"CreateSoundBuffer should have returned original primary buffer: 0x%lx\n",rc);
        ref=IDirectSoundBuffer_Release(second);
        ok(ref==1,"IDirectSoundBuffer_Release primary has %d references, should have 1\n",ref);

        /* Try to duplicate a primary buffer */
        /* DSOUND: Error: Can't duplicate primary buffers */
        rc=IDirectSound_DuplicateSoundBuffer(dso,primary,&third);
        /* rc=0x88780032 */
        ok(rc!=DS_OK,"IDirectSound_DuplicateSoundBuffer primary buffer should have failed 0x%lx\n",rc);

        rc=IDirectSoundBuffer_GetVolume(primary,&vol);
        ok(rc==DS_OK,"GetVolume failed: 0x%lx\n",rc);

        if (winetest_interactive)
        {
            trace("Playing a 5 seconds reference tone at the current volume.\n");
            if (rc==DS_OK)
                trace("(the current volume is %ld according to DirectSound)\n",vol);
            trace("All subsequent tones should be identical to this one.\n");
            trace("Listen for stutter, changes in pitch, volume, etc.\n");
        }
        test_buffer(dso,primary,1,FALSE,0,FALSE,0,winetest_interactive && !(dscaps.dwFlags & DSCAPS_EMULDRIVER),5.0,0,0,0,0);

        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release primary has %d references, should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"SetCooperativeLevel failed: 0x%lx\n",rc);

EXIT:
    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release has %d references, should have 0\n",ref);
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
    WAVEFORMATEX wfx;
    int f,ref;

    /* Create the DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"GetCaps failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before creating primary buffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"SetCooperativeLevel failed: 0x%0lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,"CreateSoundBuffer failed to create a primary buffer 0x%lx\n", rc);

    if (rc==DS_OK && primary!=NULL) {
        for (f=0;f<NB_FORMATS;f++) {
            init_format(&wfx,WAVE_FORMAT_PCM,formats[f][0],formats[f][1],formats[f][2]);
            secondary=NULL;
            ZeroMemory(&bufdesc, sizeof(bufdesc));
            bufdesc.dwSize=sizeof(bufdesc);
            bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
            bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec*BUFFER_LEN/1000;
            bufdesc.lpwfxFormat=&wfx;
            trace("  Testing a secondary buffer at %ldx%dx%d\n",
                  wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels);
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DS_OK && secondary!=NULL,"CreateSoundBuffer failed to create a secondary buffer 0x%lx\n",rc);

            if (rc==DS_OK && secondary!=NULL) {
                test_buffer(dso,secondary,0,FALSE,0,FALSE,0,winetest_interactive,1.0,0,NULL,0,0);

                ref=IDirectSoundBuffer_Release(secondary);
                ok(ref==0,"IDirectSoundBuffer_Release has %d references, should have 0\n",ref);
            }
        }

        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release primary has %d references, should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"SetCooperativeLevel failed: 0x%0lx\n",rc);

EXIT:
    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static BOOL WINAPI dsenum_callback(LPGUID lpGuid, LPCSTR lpcstrDescription,
                                   LPCSTR lpcstrModule, LPVOID lpContext)
{
    trace("*** Testing %s - %s\n",lpcstrDescription,lpcstrModule);
    test_dsound(lpGuid);
    test_dsound8(lpGuid);
    test_primary(lpGuid);
    test_secondary(lpGuid);

    return 1;
}

static void dsound_tests()
{
    HRESULT rc;
    rc=DirectSoundEnumerateA(&dsenum_callback,NULL);
    ok(rc==DS_OK,"DirectSoundEnumerate failed: %ld\n",rc);
}

START_TEST(dsound)
{
    CoInitialize(NULL);

    dsound_dsound_tests();
    dsound_dsound8_tests();
    dsound_tests();
}
