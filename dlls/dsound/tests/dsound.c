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

#include <malloc.h>
#include <math.h>

#include "wine/test.h"
#include "dsound.h"


/* The time slice determines how often we will service the buffer and the
 * buffer will be four time slices long
 */
#define TIME_SLICE    100
#define BUFFER_LEN    (4*TIME_SLICE)
#define TONE_DURATION (6*TIME_SLICE)

/* This test can play a test tone. But this only makes sense if someone
 * is going to carefully listen to it, and would only bother everyone else.
 * So this is only done if the test is being run in interactive mode.
 */

#define PI 3.14159265358979323846
static char* wave_generate_la(WAVEFORMATEX* wfx, double duration, DWORD* size)
{
    int i;
    int nb_samples;
    char* buf;
    char* b;

    nb_samples=(int)(duration*wfx->nSamplesPerSec);
    *size=nb_samples*wfx->nBlockAlign;
    b=buf=malloc(*size);
    for (i=0;i<nb_samples;i++) {
        double y=sin(440.0*2*PI*i/wfx->nSamplesPerSec);
        if (wfx->wBitsPerSample==8) {
            unsigned char sample=(unsigned char)((double)127.5*(y+1.0));
            *b++=sample;
            if (wfx->nChannels==2)
               *b++=sample;
        } else {
            signed short sample=(signed short)((double)32767.5*y-0.5);
            b[0]=sample & 0xff;
            b[1]=sample >> 8;
            b+=2;
            if (wfx->nChannels==2) {
                b[0]=sample & 0xff;
                b[1]=sample >> 8;
                b+=2;
            }
        }
    }
    return buf;
}

static HWND get_hwnd()
{
    HWND hwnd=GetForegroundWindow();
    if (!hwnd)
        hwnd=GetDesktopWindow();
    return hwnd;
}

static void init_format(WAVEFORMATEX* wfx, int rate, int depth, int channels)
{
    wfx->wFormatTag=WAVE_FORMAT_PCM;
    wfx->nChannels=channels;
    wfx->wBitsPerSample=depth;
    wfx->nSamplesPerSec=rate;
    wfx->nBlockAlign=wfx->nChannels*wfx->wBitsPerSample/8;
    wfx->nAvgBytesPerSec=wfx->nSamplesPerSec*wfx->nBlockAlign;
    wfx->cbSize=0;
}

typedef struct {
    char* wave;
    DWORD wave_len;

    LPDIRECTSOUNDBUFFER dsbo;
    LPWAVEFORMATEX wfx;
    DWORD buffer_size;
    DWORD written;
    DWORD offset;

    DWORD last_pos;
} play_state_t;

static int buffer_refill(play_state_t* state, DWORD size)
{
    LPVOID ptr1,ptr2;
    DWORD len1,len2;
    HRESULT rc;

    if (size>state->wave_len-state->written)
        size=state->wave_len-state->written;

    rc=IDirectSoundBuffer_Lock(state->dsbo,state->offset,size,
                               &ptr1,&len1,&ptr2,&len2,0);
    ok(rc==DS_OK,"Lock: 0x%lx",rc);
    if (rc!=DS_OK)
        return -1;

    memcpy(ptr1,state->wave+state->written,len1);
    state->written+=len1;
    if (ptr2!=NULL) {
        memcpy(ptr2,state->wave+state->written,len2);
        state->written+=len2;
    }
    state->offset=state->written % state->buffer_size;
    rc=IDirectSoundBuffer_Unlock(state->dsbo,ptr1,len1,ptr2,len2);
    ok(rc==DS_OK,"Unlock: 0x%lx",rc);
    if (rc!=DS_OK)
        return -1;
    return size;
}

static int buffer_silence(play_state_t* state, DWORD size)
{
    LPVOID ptr1,ptr2;
    DWORD len1,len2;
    HRESULT rc;
    BYTE s;

    rc=IDirectSoundBuffer_Lock(state->dsbo,state->offset,size,
                               &ptr1,&len1,&ptr2,&len2,0);
    ok(rc==DS_OK,"Lock: 0x%lx",rc);
    if (rc!=DS_OK)
        return -1;

    s=(state->wfx->wBitsPerSample==8?0x80:0);
    memset(ptr1,s,len1);
    if (ptr2!=NULL) {
        memset(ptr2,s,len2);
    }
    state->offset=(state->offset+size) % state->buffer_size;
    rc=IDirectSoundBuffer_Unlock(state->dsbo,ptr1,len1,ptr2,len2);
    ok(rc==DS_OK,"Unlock: 0x%lx",rc);
    if (rc!=DS_OK)
        return -1;
    return size;
}

static int buffer_service(play_state_t* state)
{
    DWORD play_pos,write_pos,buf_free;
    HRESULT rc;

    rc=IDirectSoundBuffer_GetCurrentPosition(state->dsbo,&play_pos,&write_pos);
    ok(rc==DS_OK,"GetCurrentPosition: %lx",rc);
    if (rc!=DS_OK) {
        goto STOP;
    }

    /* Refill the buffer */
    if (state->offset<=play_pos) {
        buf_free=play_pos-state->offset;
    } else {
        buf_free=state->buffer_size-state->offset+play_pos;
    }
    if (winetest_debug > 1)
        trace("buf pos=%ld free=%ld written=%ld / %ld\n",
              play_pos,buf_free,state->written,state->wave_len);
    if (buf_free==0)
        return 1;

    if (state->written<state->wave_len) {
        int w=buffer_refill(state,buf_free);
        if (w==-1)
            goto STOP;
        buf_free-=w;
        if (state->written==state->wave_len) {
            state->last_pos=(state->offset<play_pos)?play_pos:0;
            if (winetest_debug > 1)
                trace("last sound byte at %ld\n",
                      (state->written % state->buffer_size));
        }
    } else {
        if (state->last_pos!=0 && play_pos<state->last_pos) {
            /* We wrapped around the end of the buffer */
            state->last_pos=0;
        }
        if (state->last_pos==0 &&
            play_pos>(state->written % state->buffer_size)) {
            /* Now everything has been played */
            goto STOP;
        }
    }

    if (buf_free>0) {
        /* Fill with silence */
        if (winetest_debug > 1)
            trace("writing %ld bytes of silence\n",buf_free);
        if (buffer_silence(state,buf_free)==-1)
            goto STOP;
    }
    return 1;

STOP:
    if (winetest_debug > 1)
        trace("stopping playback\n");
    rc=IDirectSoundBuffer_Stop(state->dsbo);
    ok(rc==DS_OK,"Stop failed: rc=%ld",rc);
    return 0;
}

static void test_buffer(LPDIRECTSOUND dso, LPDIRECTSOUNDBUFFER dsbo,
                        int primary, int play)
{
    HRESULT rc;
    DSBCAPS dsbcaps;
    WAVEFORMATEX wfx,wfx2;
    DWORD size,status,freq;

    dsbcaps.dwSize=0;
    rc=IDirectSoundBuffer_GetCaps(dsbo,&dsbcaps);
    ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: 0x%lx\n",rc);

    dsbcaps.dwSize=sizeof(dsbcaps);
    rc=IDirectSoundBuffer_GetCaps(dsbo,&dsbcaps);
    ok(rc==DS_OK,"GetCaps failed: 0x%lx\n",rc);
    if (rc==DS_OK) {
        trace("    Caps: flags=0x%08lx size=%ld\n",dsbcaps.dwFlags,
              dsbcaps.dwBufferBytes);
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
        trace("    tag=0x%04x %ldx%dx%d avg.B/s=%ld align=%d\n",
              wfx.wFormatTag,wfx.nSamplesPerSec,wfx.wBitsPerSample,
              wfx.nChannels,wfx.nAvgBytesPerSec,wfx.nBlockAlign);
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
        trace("    status=0x%04lx\n",status);
    }

    if (primary) {
        /* We must call SetCooperativeLevel to be allowed to call SetFormat */
        rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
        ok(rc==DS_OK,"SetCooperativeLevel failed: 0x%lx\n",rc);
        if (rc!=DS_OK)
            return;

        init_format(&wfx2,11025,16,2);
        rc=IDirectSoundBuffer_SetFormat(dsbo,&wfx2);
        ok(rc==DS_OK,"SetFormat failed: 0x%lx\n",rc);

        /* There is no garantee that SetFormat will actually change the
         * format to what we asked for. It depends on what the soundcard
         * supports. So we must re-query the format.
         */
        rc=IDirectSoundBuffer_GetFormat(dsbo,&wfx,sizeof(wfx),NULL);
        ok(rc==DS_OK,"GetFormat failed: 0x%lx\n",rc);
        if (rc==DS_OK) {
            trace("    tag=0x%04x %ldx%dx%d avg.B/s=%ld align=%d\n",
                  wfx.wFormatTag,wfx.nSamplesPerSec,wfx.wBitsPerSample,
                  wfx.nChannels,wfx.nAvgBytesPerSec,wfx.nBlockAlign);
    }

        /* Set the CooperativeLevel back to normal */
        rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
        ok(rc==DS_OK,"SetCooperativeLevel failed: 0x%lx\n",rc);
    }

    if (play) {
        play_state_t state;
        LONG volume;

        trace("    Playing 440Hz LA at %ldx%2dx%d\n",
              wfx.nSamplesPerSec, wfx.wBitsPerSample,wfx.nChannels);

        if (primary) {
            /* We must call SetCooperativeLevel to be allowed to call Lock */
            rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_WRITEPRIMARY);
            ok(rc==DS_OK,"SetCooperativeLevel failed: 0x%lx\n",rc);
            if (rc!=DS_OK)
                return;
        }

        if (dsbcaps.dwFlags & DSBCAPS_CTRLVOLUME) {
            rc=IDirectSoundBuffer_GetVolume(dsbo,&volume);
            ok(rc==DS_OK,"GetVolume failed: 0x%lx\n",rc);

            rc=IDirectSoundBuffer_SetVolume(dsbo,-300);
            ok(rc==DS_OK,"SetVolume failed: 0x%lx\n",rc);
        }
        rc=IDirectSoundBuffer_GetVolume(dsbo,&volume);
        ok(rc==DS_OK,"GetVolume failed: 0x%lx\n",rc);
        if (rc==DS_OK) {
            trace("    volume=%ld\n",volume);
        }

        state.wave=wave_generate_la(&wfx,((double)TONE_DURATION)/1000,&state.wave_len);

        state.dsbo=dsbo;
        state.wfx=&wfx;
        state.buffer_size=dsbcaps.dwBufferBytes;
        state.written=state.offset=0;
        buffer_refill(&state,state.buffer_size);

        rc=IDirectSoundBuffer_Play(dsbo,0,0,DSBPLAY_LOOPING);
        ok(rc==DS_OK,"Play: 0x%lx\n",rc);

        rc=IDirectSoundBuffer_GetStatus(dsbo,&status);
        ok(rc==DS_OK,"GetStatus failed: 0x%lx\n",rc);
        ok(status==(DSBSTATUS_PLAYING|DSBSTATUS_LOOPING),
           "GetStatus: bad status: %lx",status);

        while (buffer_service(&state)) {
            WaitForSingleObject(GetCurrentProcess(),TIME_SLICE/2);
        }

        if (dsbcaps.dwFlags & DSBCAPS_CTRLVOLUME) {
            rc=IDirectSoundBuffer_SetVolume(dsbo,volume);
            ok(rc==DS_OK,"SetVolume failed: 0x%lx\n",rc);
        }

        free(state.wave);
        if (primary) {
            /* Set the CooperativeLevel back to normal */
            rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
            ok(rc==DS_OK,"SetCooperativeLevel failed: 0x%lx\n",rc);
        }
    }
}

static BOOL WINAPI dsenum_callback(LPGUID lpGuid, LPCSTR lpcstrDescription,
                                   LPCSTR lpcstrModule, LPVOID lpContext)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER dsbo=NULL;
    DSBUFFERDESC bufdesc;
    WAVEFORMATEX wfx;
    DSCAPS dscaps;

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

    /* Testing the primary buffer */
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    bufdesc.dwBufferBytes=0;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=NULL;
    trace("  Testing the primary buffer\n");
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&dsbo,NULL);
    ok(rc==DS_OK,"CreateSoundBuffer failed to create a primary buffer 0x%lx\n",rc);
    if (rc==DS_OK) {
        test_buffer(dso,dsbo,1,winetest_interactive && !(dscaps.dwFlags & DSCAPS_EMULDRIVER));
        IDirectSoundBuffer_Release(dsbo);
    }

    /* Testing secondary buffers */
    init_format(&wfx,11025,8,1);
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_CTRLDEFAULT|DSBCAPS_GETCURRENTPOSITION2;
    bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec*BUFFER_LEN/1000;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=&wfx;
    trace("  Testing a secondary buffer at %ldx%dx%d\n",
          wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels);
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&dsbo,NULL);
    ok(rc==DS_OK,"CreateSoundBuffer failed to create a secondary buffer 0x%lx\n",rc);
    if (rc==DS_OK) {
        test_buffer(dso,dsbo,0,winetest_interactive);
        IDirectSoundBuffer_Release(dsbo);
    }

    init_format(&wfx,48000,16,2);
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_CTRLDEFAULT|DSBCAPS_GETCURRENTPOSITION2;
    bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec*BUFFER_LEN/1000;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=&wfx;
    trace("  Testing a secondary buffer at %ldx%dx%d\n",
          wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels);
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&dsbo,NULL);
    ok(rc==DS_OK,"CreateSoundBuffer failed to create a secondary buffer 0x%lx\n",rc);
    if (rc==DS_OK) {
        test_buffer(dso,dsbo,0,winetest_interactive);
        IDirectSoundBuffer_Release(dsbo);
    }

EXIT:
    if (dso!=NULL)
        IDirectSound_Release(dso);
    return 1;
}

static void dsound_out_tests()
{
    HRESULT rc;
    rc=DirectSoundEnumerateA(&dsenum_callback,NULL);
    ok(rc==DS_OK,"DirectSoundEnumerate failed: %ld\n",rc);
}

START_TEST(dsound)
{
    dsound_out_tests();
}
