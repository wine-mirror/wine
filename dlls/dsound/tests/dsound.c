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

#include <math.h>
#include <stdlib.h>

#include "wine/test.h"
#include "objbase.h"
#include "initguid.h"
#include "dsound.h"

static const unsigned int formats[][3]={
    { 8000,  8, 1},
    { 8000,  8, 2},
    { 8000, 16, 1},
    { 8000, 16, 2},
    {11025,  8, 1},
    {11025,  8, 2},
    {11025, 16, 1},
    {11025, 16, 2},
    {22050,  8, 1},
    {22050,  8, 2},
    {22050, 16, 1},
    {22050, 16, 2},
    {44100,  8, 1},
    {44100,  8, 2},
    {44100, 16, 1},
    {44100, 16, 2},
    {48000,  8, 1},
    {48000,  8, 2},
    {48000, 16, 1},
    {48000, 16, 2},
    {96000,  8, 1},
    {96000,  8, 2},
    {96000, 16, 1},
    {96000, 16, 2}
};
#define NB_FORMATS (sizeof(formats)/sizeof(*formats))

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
    }

    if (play) {
        play_state_t state;
        LONG volume;

        trace("    Playing 440Hz LA at %ldx%2dx%d\n",
              wfx.nSamplesPerSec, wfx.wBitsPerSample,wfx.nChannels);

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
    int f,ref;

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

    /* We must call SetCooperativeLevel before calling CreateSoundBuffer */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"SetCooperativeLevel failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

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
        ref=IDirectSoundBuffer_Release(dsbo);
        ok(ref==0,"IDirectSoundBuffer_Release has %d references\n",ref); 
    }

    /* Testing 3D primary buffer */
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRL3D;
    bufdesc.dwBufferBytes=0;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=NULL;
    trace("  Testing 3D primary buffer\n");
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&dsbo,NULL);
    ok(rc==DS_OK,"CreateSoundBuffer failed to create a 3D primary buffer 0x%lx\n",rc);
    if (rc==DS_OK) {
        test_buffer(dso,dsbo,1,winetest_interactive && !(dscaps.dwFlags & DSCAPS_EMULDRIVER));
        ref=IDirectSoundBuffer_Release(dsbo);
        ok(ref==0,"IDirectSoundBuffer_Release has %d references\n",ref); 
    }
    
    /* Set the CooperativeLevel back to normal */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"SetCooperativeLevel failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* Testing secondary buffers */
    for (f=0;f<NB_FORMATS;f++) {
	init_format(&wfx,formats[f][0],formats[f][1],formats[f][2]);
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
            ref=IDirectSoundBuffer_Release(dsbo);
            ok(ref==0,"IDirectSoundBuffer_Release has %d references\n",ref); 
	}
    }

    /* Testing 3D secondary buffers */
    for (f=0;f<NB_FORMATS;f++) {
	init_format(&wfx,formats[f][0],formats[f][1],formats[f][2]);
	bufdesc.dwSize=sizeof(bufdesc);
	bufdesc.dwFlags=DSBCAPS_CTRLDEFAULT|DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_CTRL3D;
	bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec*BUFFER_LEN/1000;
	bufdesc.dwReserved=0;
	bufdesc.lpwfxFormat=&wfx;
	trace("  Testing a 3D secondary buffer at %ldx%dx%d\n",
	    wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels);
	rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&dsbo,NULL);
	ok(rc==DS_OK,"CreateSoundBuffer failed to create a 3D secondary buffer 0x%lx\n",rc);
	if (rc==DS_OK) {
	    test_buffer(dso,dsbo,0,winetest_interactive);
            ref=IDirectSoundBuffer_Release(dsbo);
            ok(ref==0,"IDirectSoundBuffer_Release has %d references\n",ref); 
	}
    }

EXIT:
    if (dso!=NULL) {
        ref=IDirectSound_Release(dso);
	ok(ref==0,"IDirectSound_Release has %d references\n",ref);
    }
    return 1;
}

static void dsound_out_tests()
{
    HRESULT rc;
    rc=DirectSoundEnumerateA(&dsenum_callback,NULL);
    ok(rc==DS_OK,"DirectSoundEnumerate failed: %ld\n",rc);
}

#define NOTIFICATIONS    5

typedef struct {
    char* wave;
    DWORD wave_len;

    LPDIRECTSOUNDCAPTUREBUFFER dscbo;
    LPWAVEFORMATEX wfx;
    DSBPOSITIONNOTIFY posnotify[NOTIFICATIONS];
    HANDLE event;
    LPDIRECTSOUNDNOTIFY notify;

    DWORD buffer_size;
    DWORD read;
    DWORD offset;
    DWORD size;

    DWORD last_pos;
} capture_state_t;

static int capture_buffer_service(capture_state_t* state)
{
    HRESULT rc;
    LPVOID ptr1,ptr2;
    DWORD len1,len2;
    DWORD capture_pos,read_pos;

    rc=IDirectSoundCaptureBuffer_GetCurrentPosition(state->dscbo,&capture_pos,&read_pos);
    ok(rc==DS_OK,"GetCurrentPosition failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        return 0;

    rc=IDirectSoundCaptureBuffer_Lock(state->dscbo,state->offset,state->size,&ptr1,&len1,&ptr2,&len2,0);
    ok(rc==DS_OK,"Lock failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        return 0;

    rc=IDirectSoundCaptureBuffer_Unlock(state->dscbo,ptr1,len1,ptr2,len2);
    ok(rc==DS_OK,"Unlock failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        return 0;

    state->offset = (state->offset + state->size) % state->buffer_size;

    return 1;
}

static void test_capture_buffer(LPDIRECTSOUNDCAPTURE dsco, LPDIRECTSOUNDCAPTUREBUFFER dscbo)
{
    HRESULT rc;
    DSCBCAPS dscbcaps;
    WAVEFORMATEX wfx;
    DWORD size,status;
    capture_state_t state;
    int i;

    /* Private dsound.dll: Error: Invalid caps pointer */
    rc=IDirectSoundCaptureBuffer_GetCaps(dscbo,0);
    ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: 0x%lx\n",rc);

    /* Private dsound.dll: Error: Invalid caps pointer */
    dscbcaps.dwSize=0;
    rc=IDirectSoundCaptureBuffer_GetCaps(dscbo,&dscbcaps);
    ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: 0x%lx\n",rc);

    dscbcaps.dwSize=sizeof(dscbcaps);
    rc=IDirectSoundCaptureBuffer_GetCaps(dscbo,&dscbcaps);
    ok(rc==DS_OK,"GetCaps failed: 0x%lx\n",rc);
    if (rc==DS_OK) {
        trace("    Caps: size = %ld flags=0x%08lx buffer size=%ld\n",
            dscbcaps.dwSize,dscbcaps.dwFlags,dscbcaps.dwBufferBytes);
    }

    /* Query the format size. Note that it may not match sizeof(wfx) */
    /* Private dsound.dll: Error: Either pwfxFormat or pdwSizeWritten must be non-NULL */
    rc=IDirectSoundCaptureBuffer_GetFormat(dscbo,NULL,0,NULL);
    ok(rc==DSERR_INVALIDPARAM,
       "GetFormat should have returned an error: rc=0x%lx\n",rc);

    size=0;
    rc=IDirectSoundCaptureBuffer_GetFormat(dscbo,NULL,0,&size);
    ok(rc==DS_OK && size!=0,
       "GetFormat should have returned the needed size: rc=0x%lx size=%ld\n",
       rc,size);

    rc=IDirectSoundCaptureBuffer_GetFormat(dscbo,&wfx,sizeof(wfx),NULL);
    ok(rc==DS_OK,"GetFormat failed: 0x%lx\n",rc);
    if (rc==DS_OK) {
        trace("    tag=0x%04x %ldx%dx%d avg.B/s=%ld align=%d\n",
              wfx.wFormatTag,wfx.nSamplesPerSec,wfx.wBitsPerSample,
              wfx.nChannels,wfx.nAvgBytesPerSec,wfx.nBlockAlign);
    }

    /* Private dsound.dll: Error: Invalid status pointer */
    rc=IDirectSoundCaptureBuffer_GetStatus(dscbo,0);
    ok(rc==DSERR_INVALIDPARAM,"GetStatus should have failed: 0x%lx\n",rc);

    rc=IDirectSoundCaptureBuffer_GetStatus(dscbo,&status);
    ok(rc==DS_OK,"GetStatus failed: 0x%lx\n",rc);
    if (rc==DS_OK) {
        trace("    status=0x%04lx\n",status);
    }

    ZeroMemory(&state, sizeof(state));
    state.dscbo=dscbo;
    state.wfx=&wfx;
    state.buffer_size = dscbcaps.dwBufferBytes;
    state.event = CreateEvent( NULL, FALSE, FALSE, NULL );
    state.size = dscbcaps.dwBufferBytes / NOTIFICATIONS;

    rc=IDirectSoundCapture_QueryInterface(dscbo,&IID_IDirectSoundNotify,(void **)&(state.notify));
    ok(rc==DS_OK,"QueryInterface failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        return;

    for (i = 0; i < NOTIFICATIONS; i++) {
        state.posnotify[i].dwOffset = (i * state.size) + state.size - 1;
        state.posnotify[i].hEventNotify = state.event;
    }

    rc=IDirectSoundNotify_SetNotificationPositions(state.notify,NOTIFICATIONS,state.posnotify);
    ok(rc==DS_OK,"SetNotificationPositions failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        return;

    rc=IDirectSoundCaptureBuffer_Start(dscbo,DSCBSTART_LOOPING);
    ok(rc==DS_OK,"Start: 0x%lx\n",rc);
    if (rc!=DS_OK)
        return;

    rc=IDirectSoundCaptureBuffer_GetStatus(dscbo,&status);
    ok(rc==DS_OK,"GetStatus failed: 0x%lx\n",rc);
    ok(status==(DSCBSTATUS_CAPTURING|DSCBSTATUS_LOOPING),
       "GetStatus: bad status: %lx",status);
    if (rc!=DS_OK)
        return;

    /* wait for the notifications */
    for (i = 0; i < (NOTIFICATIONS * 2); i++) {
        rc=MsgWaitForMultipleObjects( 1, &(state.event), FALSE, 3000, QS_ALLEVENTS );
        ok(rc==WAIT_OBJECT_0,"MsgWaitForMultipleObjects failed: 0x%lx\n",rc);
        if (rc!=WAIT_OBJECT_0)
            break;
        if (!capture_buffer_service(&state))
            break;
    }

    rc=IDirectSoundCaptureBuffer_Stop(dscbo);
    ok(rc==DS_OK,"Stop: 0x%lx\n",rc);
    if (rc!=DS_OK)
        return;

    rc=IDirectSoundNotify_Release(state.notify);
    ok(rc==0,"Release: 0x%lx\n",rc);
    if (rc!=0)
        return;
}

static BOOL WINAPI dscenum_callback(LPGUID lpGuid, LPCSTR lpcstrDescription,
                                    LPCSTR lpcstrModule, LPVOID lpContext)
{
    HRESULT rc;
    LPDIRECTSOUNDCAPTURE dsco=NULL;
    LPDIRECTSOUNDCAPTUREBUFFER dscbo=NULL;
    DSCBUFFERDESC bufdesc;
    WAVEFORMATEX wfx;
    DSCCAPS dsccaps;
    int f, ref;

    /* Private dsound.dll: Error: Invalid interface buffer */
    trace("Testing %s - %s\n",lpcstrDescription,lpcstrModule);
    rc=DirectSoundCaptureCreate(lpGuid,NULL,NULL);
    ok(rc==DSERR_INVALIDPARAM,"DirectSoundCaptureCreate didn't fail: 0x%lx\n",rc);
    if (rc==DS_OK) {
	ref=IDirectSoundCapture_Release(dsco);
	ok(ref==0,"IDirectSoundCapture_Release has %d references\n",ref);
    }

    rc=DirectSoundCaptureCreate(lpGuid,&dsco,NULL);
    ok((rc==DS_OK)||(rc==DSERR_NODRIVER),"DirectSoundCaptureCreate failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* Private dsound.dll: Error: Invalid caps buffer */
    rc=IDirectSoundCapture_GetCaps(dsco,NULL);
    ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: 0x%lx\n",rc);

    /* Private dsound.dll: Error: Invalid caps buffer */
    dsccaps.dwSize=0;
    rc=IDirectSoundCapture_GetCaps(dsco,&dsccaps);
    ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: 0x%lx\n",rc);

    dsccaps.dwSize=sizeof(dsccaps);
    rc=IDirectSoundCapture_GetCaps(dsco,&dsccaps);
    ok(rc==DS_OK,"GetCaps failed: 0x%lx\n",rc);
    if (rc==DS_OK) {
        trace("  DirectSoundCapture Caps: size=%ld flags=0x%08lx formats=%05lx channels=%ld\n",
              dsccaps.dwSize,dsccaps.dwFlags,dsccaps.dwFormats,dsccaps.dwChannels);
    }

    /* Private dsound.dll: Error: Invalid size */
    /* Private dsound.dll: Error: Invalid capture buffer description */
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=0;
    bufdesc.dwFlags=0;
    bufdesc.dwBufferBytes=0;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=NULL;
    rc=IDirectSoundCapture_CreateCaptureBuffer(dsco,&bufdesc,&dscbo,NULL);
    ok(rc==DSERR_INVALIDPARAM,"CreateCaptureBuffer should have failed to create a capture buffer 0x%lx\n",rc);
    if (rc==DS_OK) {
        ref=IDirectSoundCaptureBuffer_Release(dscbo);
	ok(ref==0,"IDirectSoundCaptureBuffer_Release has %d references\n",ref);
    }

    /* Private dsound.dll: Error: Invalid buffer size */
    /* Private dsound.dll: Error: Invalid capture buffer description */
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=0;
    bufdesc.dwBufferBytes=0;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=NULL;
    rc=IDirectSoundCapture_CreateCaptureBuffer(dsco,&bufdesc,&dscbo,NULL);
    ok(rc==DSERR_INVALIDPARAM,"CreateCaptureBuffer should have failed to create a capture buffer 0x%lx\n",rc);
    if (rc==DS_OK) {
        ref=IDirectSoundCaptureBuffer_Release(dscbo);
	ok(ref==0,"IDirectSoundCaptureBuffer_Release has %d references\n",ref);
    }

    /* Private dsound.dll: Error: Invalid buffer size */
    /* Private dsound.dll: Error: Invalid capture buffer description */
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    ZeroMemory(&wfx, sizeof(wfx));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=0;
    bufdesc.dwBufferBytes=0;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=&wfx;
    rc=IDirectSoundCapture_CreateCaptureBuffer(dsco,&bufdesc,&dscbo,NULL);
    ok(rc==DSERR_INVALIDPARAM,"CreateCaptureBuffer should have failed to create a capture buffer 0x%lx\n",rc);
    if (rc==DS_OK) {
        ref=IDirectSoundCaptureBuffer_Release(dscbo);
	ok(ref==0,"IDirectSoundCaptureBuffer_Release has %d references\n",ref);
    }

    /* Private dsound.dll: Error: Invalid buffer size */
    /* Private dsound.dll: Error: Invalid capture buffer description */
    init_format(&wfx,11025,8,1);
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=0;
    bufdesc.dwBufferBytes=0;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=&wfx;
    rc=IDirectSoundCapture_CreateCaptureBuffer(dsco,&bufdesc,&dscbo,NULL);
    ok(rc==DSERR_INVALIDPARAM,"CreateCaptureBuffer should have failed to create a capture buffer 0x%lx\n",rc);
    if (rc==DS_OK) {
        ref=IDirectSoundCaptureBuffer_Release(dscbo);
	ok(ref==0,"IDirectSoundCaptureBuffer_Release has %d references\n",ref);
    }

    for (f=0;f<NB_FORMATS;f++) {
	init_format(&wfx,formats[f][0],formats[f][1],formats[f][2]);
	ZeroMemory(&bufdesc, sizeof(bufdesc));
	bufdesc.dwSize=sizeof(bufdesc);
	bufdesc.dwFlags=0;
	bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec;
	bufdesc.dwReserved=0;
	bufdesc.lpwfxFormat=&wfx;
	trace("  Testing the capture buffer at %ldx%dx%d\n",
	    wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels);
	rc=IDirectSoundCapture_CreateCaptureBuffer(dsco,&bufdesc,&dscbo,NULL);
	ok(rc==DS_OK,"CreateCaptureBuffer failed to create a capture buffer 0x%lx\n",rc);
	if (rc==DS_OK) {
	    test_capture_buffer(dsco, dscbo);
	    ref=IDirectSoundCaptureBuffer_Release(dscbo);
	    ok(ref==0,"IDirectSoundCaptureBuffer_Release has %d references\n",ref);
	}
    }

    /* Try an invalid format to test error handling */
#if 0
    init_format(&wfx,2000000,16,2);
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSCBCAPS_WAVEMAPPED;
    bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=&wfx;
    trace("  Testing the capture buffer at %ldx%dx%d\n",
	wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels);
    rc=IDirectSoundCapture_CreateCaptureBuffer(dsco,&bufdesc,&dscbo,NULL);
    ok(rc!=DS_OK,"CreateCaptureBuffer should have failed at 2 MHz 0x%lx\n",rc);
#endif

EXIT:
    if (dsco!=NULL) {
        ref=IDirectSoundCapture_Release(dsco);
	ok(ref==0,"IDirectSoundCapture_Release has %d references\n",ref);
    }

    return TRUE;
}

static void dsound_in_tests()
{
    HRESULT rc;
    rc=DirectSoundCaptureEnumerateA(&dscenum_callback,NULL);
    ok(rc==DS_OK,"DirectSoundCaptureEnumerate failed: %ld\n",rc);
}

START_TEST(dsound)
{
    dsound_out_tests();
    dsound_in_tests();
}
