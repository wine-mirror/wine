/*
 * Unit tests for dsound functions
 *
 * Copyright (c) 2002 Francois Gouget
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

#include "wine/test.h"
#include "dsound.h"



BOOL WINAPI dsenum_callback(LPGUID lpGuid, LPCSTR lpcstrDescription,
                            LPCSTR lpcstrModule, LPVOID lpContext)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER dsbo=NULL;
    DSCAPS dscaps;
    DSBCAPS dsbcaps;
    DSBUFFERDESC bufdesc;
    WAVEFORMATEX wfx,wfx2;
    DWORD size,status,freq;

    trace("Testing %s - %s\n",lpcstrDescription,lpcstrModule);
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    dscaps.dwSize=0;
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: 0x%lx\n",rc);

    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"GetCaps failed: 0x%lx\n",rc);
    if (rc==DS_OK) {
        trace("  DirectSound Caps: flags=0x%08lx secondary min=%ld max=%ld\n",
              dscaps.dwFlags,dscaps.dwMinSecondarySampleRate,
              dscaps.dwMaxSecondarySampleRate);
    }

    /* Testing the primary buffers */
    rc=IDirectSound_SetCooperativeLevel(dso,GetDesktopWindow(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"SetCooperativeLevel failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    bufdesc.dwBufferBytes=0;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=NULL;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&dsbo,NULL);
    ok(rc==DS_OK,"CreateSoundBuffer failed to create a primary buffer 0x%lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    dsbcaps.dwSize=0;
    rc=IDirectSoundBuffer_GetCaps(dsbo,&dsbcaps);
    ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: 0x%lx\n",rc);

    dsbcaps.dwSize=sizeof(dsbcaps);
    rc=IDirectSoundBuffer_GetCaps(dsbo,&dsbcaps);
    ok(rc==DS_OK,"GetCaps failed: 0x%lx\n",rc);
    if (rc==DS_OK) {
        trace("  PrimaryBuffer Caps: flags=0x%08lx size=%ld\n",dsbcaps.dwFlags,dsbcaps.dwBufferBytes);
    }

    /* Query the format size. Note that it may not match sizeof(wfx) */
    size=0;
    rc=IDirectSoundBuffer_GetFormat(dsbo,NULL,0,&size);
    ok(rc==DS_OK && size!=0,
       "GetFormat should have returned the needed size: rc=0x%lx size=%ld\n",
       rc,size);

    rc=IDirectSoundBuffer_GetFormat(dsbo,&wfx,sizeof(wfx),NULL);
    ok(rc==DS_OK,"GetFormat failed: 0x%lx\n",rc);
    if (rc==DS_OK) {
        trace("  tag=0x%04x %ldx%dx%d avg.B/s=%ld align=%d\n",
              wfx.wFormatTag,wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels,
              wfx.nAvgBytesPerSec,wfx.nBlockAlign);
    }

    rc=IDirectSoundBuffer_GetFrequency(dsbo,&freq);
    ok(rc==DS_OK || rc==DSERR_CONTROLUNAVAIL,"GetFrequency failed: 0x%lx\n",rc);
    if (rc==DS_OK) {
        ok(freq==wfx.nSamplesPerSec,
           "The frequency returned by GetFrequency %ld does not match the format %ld\n",
           freq,wfx.nSamplesPerSec);
    }

    rc=IDirectSoundBuffer_GetStatus(dsbo,&status);
    ok(rc==DS_OK,"GetStatus failed: 0x%lx\n",rc);
    if (rc==DS_OK) {
        trace("  status=0x%04lx\n",status);
    }

    wfx2.wFormatTag=WAVE_FORMAT_PCM;
    wfx2.nChannels=2;
    wfx2.wBitsPerSample=16;
    wfx2.nSamplesPerSec=11025;
    wfx2.nBlockAlign=wfx2.nChannels*wfx2.wBitsPerSample/8;
    wfx2.nAvgBytesPerSec=wfx2.nSamplesPerSec*wfx2.nBlockAlign;
    wfx2.cbSize=0;
    rc=IDirectSoundBuffer_SetFormat(dsbo,&wfx2);
    ok(rc==DS_OK,"SetFormat failed: 0x%lx\n",rc);

    rc=IDirectSoundBuffer_GetFormat(dsbo,&wfx,sizeof(wfx),NULL);
    ok(rc==DS_OK,"GetFormat failed: 0x%lx\n",rc);
    if (rc==DS_OK) {
        ok(wfx.wFormatTag==wfx2.wFormatTag && wfx.nChannels==wfx2.nChannels &&
           wfx.wBitsPerSample==wfx2.wBitsPerSample && wfx.nSamplesPerSec==wfx2.nSamplesPerSec &&
           wfx.nBlockAlign==wfx2.nBlockAlign && wfx.nAvgBytesPerSec==wfx2.nAvgBytesPerSec,
           "SetFormat did not work right: tag=0x%04x %ldx%dx%d avg.B/s=%ld align=%d\n",
           wfx.wFormatTag,wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels,
           wfx.nAvgBytesPerSec,wfx.nBlockAlign);
    }

EXIT:
    if (dsbo!=NULL)
        IDirectSoundBuffer_Release(dsbo);
    if (dso!=NULL)
        IDirectSound_Release(dso);
    return 1;
}

void dsound_out_tests()
{
    HRESULT rc;
    rc=DirectSoundEnumerateA(&dsenum_callback,NULL);
    ok(rc==DS_OK,"DirectSoundEnumerate failed: %ld\n",rc);
}

START_TEST(dsound)
{
    dsound_out_tests();
}
