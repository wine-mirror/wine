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

#include "dsound_test.h"


static HRESULT test_dsound(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    DSCAPS dscaps;
    int ref;

    /* DSOUND: Error: Invalid interface buffer */
    rc=DirectSoundCreate(lpGuid,0,NULL);
    ok(rc==DSERR_INVALIDPARAM,"DirectSoundCreate should have failed: 0x%lx\n",rc);

    /* Create the DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        return rc;

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound_GetCaps(dso,0);
    ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: 0x%lx\n",rc);

    ZeroMemory(&dscaps, sizeof(dscaps));

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: 0x%lx\n",rc);

    dscaps.dwSize=sizeof(dscaps);

    /* DSOUND: Running on a certified driver */
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"GetCaps failed: 0x%lx\n",rc);
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

#if 0
    /* FIXME: this works on windows */
    /* Create a DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate failed: 0x%lx\n",rc);
    if (rc==DS_OK) {
        LPDIRECTSOUND dso1=NULL;

        /* Create a second DirectSound object */
        rc=DirectSoundCreate(lpGuid,&dso1,NULL);
        ok(rc==DS_OK,"DirectSoundCreate failed: 0x%lx\n",rc);
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
#endif

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
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,"CreateSoundBuffer failed to create a primary buffer: 0x%lx\n",rc);
    if (rc==DS_OK && primary!=NULL) {
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

        if (winetest_interactive)
        {
            trace("Playing a 5 seconds reference tone.\n");
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
    bufdesc.dwFlags|=(DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLPAN);
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,"CreateSoundBuffer failed to create a primary buffer 0x%lx\n", rc);

    if (rc==DS_OK && primary!=NULL) {
        for (f=0;f<NB_FORMATS;f++) {
            init_format(&wfx,WAVE_FORMAT_PCM,formats[f][0],formats[f][1],formats[f][2]);
            secondary=NULL;
            ZeroMemory(&bufdesc, sizeof(bufdesc));
            bufdesc.dwSize=sizeof(bufdesc);
            bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
            bufdesc.dwFlags|=(DSBCAPS_CTRLFREQUENCY|DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLPAN);
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
    dsound_tests();
}
