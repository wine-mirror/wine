/*
 * Tests the panning and 3D functions of DirectSound
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <windows.h>

#include <math.h>

#include "wine/test.h"
#include "mmsystem.h"
#include "dsound.h"
#include "ks.h"
#include "ksmedia.h"
#include "dsound_test.h"

typedef struct {
    char* wave;
    DWORD wave_len;

    LPDIRECTSOUNDBUFFER dsbo;
    LPWAVEFORMATEX wfx;
    DWORD buffer_size;
    DWORD written;
    DWORD played;
    DWORD offset;
} play_state_t;

static int buffer_refill8(play_state_t* state, DWORD size)
{
    LPVOID ptr1,ptr2;
    DWORD len1,len2;
    HRESULT rc;

    if (size>state->wave_len-state->written)
        size=state->wave_len-state->written;

    rc=IDirectSoundBuffer_Lock(state->dsbo,state->offset,size,
                               &ptr1,&len1,&ptr2,&len2,0);
    ok(rc==DS_OK,"IDirectSoundBuffer_Lock() failed: %08lx\n", rc);
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
    ok(rc==DS_OK,"IDirectSoundBuffer_Unlock() failed: %08lx\n", rc);
    if (rc!=DS_OK)
        return -1;
    return size;
}

static int buffer_silence8(play_state_t* state, DWORD size)
{
    LPVOID ptr1,ptr2;
    DWORD len1,len2;
    HRESULT rc;
    BYTE s;

    rc=IDirectSoundBuffer_Lock(state->dsbo,state->offset,size,
                               &ptr1,&len1,&ptr2,&len2,0);
    ok(rc==DS_OK,"IDirectSoundBuffer_Lock() failed: %08lx\n", rc);
    if (rc!=DS_OK)
        return -1;

    s=(state->wfx->wBitsPerSample==8?0x80:0);
    memset(ptr1,s,len1);
    if (ptr2!=NULL) {
        memset(ptr2,s,len2);
    }
    state->offset=(state->offset+size) % state->buffer_size;
    rc=IDirectSoundBuffer_Unlock(state->dsbo,ptr1,len1,ptr2,len2);
    ok(rc==DS_OK,"IDirectSoundBuffer_Unlock() failed: %08lx\n", rc);
    if (rc!=DS_OK)
        return -1;
    return size;
}

static BOOL buffer_service8(play_state_t* state)
{
    DWORD last_play_pos,play_pos,buf_free;
    HRESULT rc;

    rc=IDirectSoundBuffer_GetCurrentPosition(state->dsbo,&play_pos,NULL);
    ok(rc==DS_OK,"IDirectSoundBuffer_GetCurrentPosition() failed: %08lx\n", rc);
    if (rc!=DS_OK) {
        goto STOP;
    }

    /* Update the amount played */
    last_play_pos=state->played % state->buffer_size;
    if (play_pos<last_play_pos)
        state->played+=state->buffer_size-last_play_pos+play_pos;
    else
        state->played+=play_pos-last_play_pos;

    if (winetest_debug > 1)
        trace("buf size=%ld last_play_pos=%ld play_pos=%ld played=%ld / %ld\n",
              state->buffer_size,last_play_pos,play_pos,state->played,
              state->wave_len);

    if (state->played>state->wave_len)
    {
        /* Everything has been played */
        goto STOP;
    }

    /* Refill the buffer */
    if (state->offset<=play_pos)
        buf_free=play_pos-state->offset;
    else
        buf_free=state->buffer_size-state->offset+play_pos;

    if (winetest_debug > 1)
        trace("offset=%ld free=%ld written=%ld / %ld\n",
              state->offset,buf_free,state->written,state->wave_len);
    if (buf_free==0)
        return TRUE;

    if (state->written<state->wave_len)
    {
        int w=buffer_refill8(state,buf_free);
        if (w==-1)
            goto STOP;
        buf_free-=w;
        if (state->written==state->wave_len && winetest_debug > 1)
            trace("last sound byte at %ld\n",
                  (state->written % state->buffer_size));
    }

    if (buf_free>0) {
        /* Fill with silence */
        if (winetest_debug > 1)
            trace("writing %ld bytes of silence\n",buf_free);
        if (buffer_silence8(state,buf_free)==-1)
            goto STOP;
    }
    return TRUE;

STOP:
    if (winetest_debug > 1)
        trace("stopping playback\n");
    rc=IDirectSoundBuffer_Stop(state->dsbo);
    ok(rc==DS_OK,"IDirectSoundBuffer_Stop() failed: %08lx\n", rc);
    return FALSE;
}

void test_buffer8(LPDIRECTSOUND8 dso, LPDIRECTSOUNDBUFFER * dsbo,
                  BOOL is_primary, BOOL set_volume, LONG volume,
                  BOOL set_pan, LONG pan, BOOL play, double duration,
                  BOOL buffer3d, LPDIRECTSOUND3DLISTENER listener,
                  BOOL move_listener, BOOL move_sound)
{
    HRESULT rc;
    DSBCAPS dsbcaps;
    WAVEFORMATEX wfx,wfx2;
    DWORD size,status,freq;
    BOOL ieee = FALSE;
    int ref;

    /* DSOUND: Error: Invalid caps pointer */
    rc=IDirectSoundBuffer_GetCaps(*dsbo,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundBuffer_GetCaps() should have "
       "returned DSERR_INVALIDPARAM, returned: %08lx\n",rc);

    ZeroMemory(&dsbcaps, sizeof(dsbcaps));

    /* DSOUND: Error: Invalid caps pointer */
    rc=IDirectSoundBuffer_GetCaps(*dsbo,&dsbcaps);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundBuffer_GetCaps() should have "
       "returned DSERR_INVALIDPARAM, returned: %08lx\n",rc);

    dsbcaps.dwSize=sizeof(dsbcaps);
    rc=IDirectSoundBuffer_GetCaps(*dsbo,&dsbcaps);
    ok(rc==DS_OK,"IDirectSoundBuffer_GetCaps() failed: %08lx\n", rc);
    if (rc==DS_OK && winetest_debug > 1) {
        trace("    Caps: flags=0x%08lx size=%ld\n",dsbcaps.dwFlags,
              dsbcaps.dwBufferBytes);
    }

    /* Query the format size. */
    size=0;
    rc=IDirectSoundBuffer_GetFormat(*dsbo,NULL,0,&size);
    ok(rc==DS_OK && size!=0,"IDirectSoundBuffer_GetFormat() should have "
       "returned the needed size: rc=%08lx size=%ld\n",rc,size);

    ok(size == sizeof(WAVEFORMATEX) || size == sizeof(WAVEFORMATEXTENSIBLE),
       "Expected a correct structure size, got %ld\n", size);

    if (size == sizeof(WAVEFORMATEX)) {
        rc=IDirectSoundBuffer_GetFormat(*dsbo,&wfx,size,NULL);
        ieee = (wfx.wFormatTag == WAVE_FORMAT_IEEE_FLOAT);
    } else if (size == sizeof(WAVEFORMATEXTENSIBLE)) {
        WAVEFORMATEXTENSIBLE wfxe;
        rc=IDirectSoundBuffer_GetFormat(*dsbo,(WAVEFORMATEX*)&wfxe,size,NULL);
        wfx = wfxe.Format;
        ieee = IsEqualGUID(&wfxe.SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
    } else
        return;

    ok(rc==DS_OK,"IDirectSoundBuffer_GetFormat() failed: %08lx\n", rc);
    if (rc==DS_OK && winetest_debug > 1) {
        trace("    Format: %s tag=0x%04x %ldx%dx%d avg.B/s=%ld align=%d\n",
              is_primary ? "Primary" : "Secondary",
              wfx.wFormatTag,wfx.nSamplesPerSec,wfx.wBitsPerSample,
              wfx.nChannels,wfx.nAvgBytesPerSec,wfx.nBlockAlign);
    }

    /* DSOUND: Error: Invalid frequency buffer */
    rc=IDirectSoundBuffer_GetFrequency(*dsbo,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundBuffer_GetFrequency() should have "
       "returned DSERR_INVALIDPARAM, returned: %08lx\n",rc);

    /* DSOUND: Error: Primary buffers don't support CTRLFREQUENCY */
    rc=IDirectSoundBuffer_GetFrequency(*dsbo,&freq);
    ok((rc==DS_OK && !is_primary) || (rc==DSERR_CONTROLUNAVAIL&&is_primary) ||
       (rc==DSERR_CONTROLUNAVAIL&&!(dsbcaps.dwFlags&DSBCAPS_CTRLFREQUENCY)),
       "IDirectSoundBuffer_GetFrequency() failed: %08lx\n",rc);
    if (rc==DS_OK) {
        ok(freq==wfx.nSamplesPerSec,"The frequency returned by GetFrequency "
           "%ld does not match the format %ld\n",freq,wfx.nSamplesPerSec);
    }

    /* DSOUND: Error: Invalid status pointer */
    rc=IDirectSoundBuffer_GetStatus(*dsbo,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundBuffer_GetStatus() should have "
       "returned DSERR_INVALIDPARAM, returned: %08lx\n",rc);

    rc=IDirectSoundBuffer_GetStatus(*dsbo,&status);
    ok(rc==DS_OK,"IDirectSoundBuffer_GetStatus() failed: %08lx\n", rc);
    ok(status==0,"status=0x%lx instead of 0\n",status);

    if (is_primary) {
        DSBCAPS new_dsbcaps;
        /* We must call SetCooperativeLevel to be allowed to call SetFormat */
        /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
        rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
        ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel(DSSCL_PRIORITY) "
           "failed: %08lx\n",rc);
        if (rc!=DS_OK)
            return;

        /* DSOUND: Error: Invalid format pointer */
        rc=IDirectSoundBuffer_SetFormat(*dsbo,0);
        ok(rc==DSERR_INVALIDPARAM,"IDirectSoundBuffer_SetFormat() should have "
           "returned DSERR_INVALIDPARAM, returned: %08lx\n",rc);

        init_format(&wfx2,WAVE_FORMAT_PCM,11025,16,2);
        rc=IDirectSoundBuffer_SetFormat(*dsbo,&wfx2);
        ok(rc==DS_OK,"IDirectSoundBuffer_SetFormat(%s) failed: %08lx\n",
           format_string(&wfx2), rc);

        /* There is no guarantee that SetFormat will actually change the
	 * format to what we asked for. It depends on what the soundcard
	 * supports. So we must re-query the format.
	 */
        rc=IDirectSoundBuffer_GetFormat(*dsbo,&wfx,sizeof(wfx),NULL);
        ok(rc==DS_OK,"IDirectSoundBuffer_GetFormat() failed: %08lx\n", rc);
        if (rc==DS_OK &&
            (wfx.wFormatTag!=wfx2.wFormatTag ||
             wfx.nSamplesPerSec!=wfx2.nSamplesPerSec ||
             wfx.wBitsPerSample!=wfx2.wBitsPerSample ||
             wfx.nChannels!=wfx2.nChannels)) {
            trace("Requested format tag=0x%04x %ldx%dx%d avg.B/s=%ld align=%d\n",
                  wfx2.wFormatTag,wfx2.nSamplesPerSec,wfx2.wBitsPerSample,
                  wfx2.nChannels,wfx2.nAvgBytesPerSec,wfx2.nBlockAlign);
            trace("Got tag=0x%04x %ldx%dx%d avg.B/s=%ld align=%d\n",
                  wfx.wFormatTag,wfx.nSamplesPerSec,wfx.wBitsPerSample,
                  wfx.nChannels,wfx.nAvgBytesPerSec,wfx.nBlockAlign);
        }

        ZeroMemory(&new_dsbcaps, sizeof(new_dsbcaps));
        new_dsbcaps.dwSize = sizeof(new_dsbcaps);
        rc=IDirectSoundBuffer_GetCaps(*dsbo,&new_dsbcaps);
        ok(rc==DS_OK,"IDirectSoundBuffer_GetCaps() failed: %08lx\n", rc);
        if (rc==DS_OK && winetest_debug > 1) {
            trace("    new Caps: flags=0x%08lx size=%ld\n",new_dsbcaps.dwFlags,
                  new_dsbcaps.dwBufferBytes);
        }

        /* Check for primary buffer size change */
        ok(new_dsbcaps.dwBufferBytes == dsbcaps.dwBufferBytes,
           "    buffer size changed after SetFormat() - "
           "previous size was %lu, current size is %lu\n",
           dsbcaps.dwBufferBytes, new_dsbcaps.dwBufferBytes);
        dsbcaps.dwBufferBytes = new_dsbcaps.dwBufferBytes;

        /* Check for primary buffer flags change */
        ok(new_dsbcaps.dwFlags == dsbcaps.dwFlags,
           "    flags changed after SetFormat() - "
           "previous flags were %08lx, current flags are %08lx\n",
           dsbcaps.dwFlags, new_dsbcaps.dwFlags);

        /* Set the CooperativeLevel back to normal */
        /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
        rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
        ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel(DSSCL_NORMAL) "
           "failed: %08lx\n",rc);
    }

    if (play) {
        play_state_t state;
        DS3DLISTENER listener_param;
        LPDIRECTSOUND3DBUFFER buffer=NULL;
        DS3DBUFFER buffer_param;
        DWORD start_time,now;
        LPVOID buffer1;
        DWORD length1;

        if (winetest_interactive) {
            trace("    Playing %g second 440Hz tone at %ldx%dx%d\n", duration,
                  wfx.nSamplesPerSec, wfx.wBitsPerSample,wfx.nChannels);
        }

        if (is_primary) {
            /* We must call SetCooperativeLevel to be allowed to call Lock */
            /* DSOUND: Setting DirectSound cooperative level to
             * DSSCL_WRITEPRIMARY */
            rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),
                                                 DSSCL_WRITEPRIMARY);
            ok(rc==DS_OK,
               "IDirectSound8_SetCooperativeLevel(DSSCL_WRITEPRIMARY) failed: %08lx\n",rc);
            if (rc!=DS_OK)
                return;
        }
        if (buffer3d) {
            LPDIRECTSOUNDBUFFER temp_buffer;

            rc=IDirectSoundBuffer_QueryInterface(*dsbo,&IID_IDirectSound3DBuffer,
                                                 (LPVOID *)&buffer);
            ok(rc==DS_OK,"IDirectSoundBuffer_QueryInterface() failed: %08lx\n", rc);
            if (rc!=DS_OK)
                return;

            /* check the COM interface */
            rc=IDirectSoundBuffer_QueryInterface(*dsbo, &IID_IDirectSoundBuffer,
                                                 (LPVOID *)&temp_buffer);
            ok(rc==DS_OK && temp_buffer!=NULL,
               "IDirectSoundBuffer_QueryInterface() failed: %08lx\n", rc);
            ok(temp_buffer==*dsbo,"COM interface broken: %p != %p\n",
               temp_buffer,*dsbo);
            ref=IDirectSoundBuffer_Release(temp_buffer);
            ok(ref==1,"IDirectSoundBuffer_Release() has %d references, "
               "should have 1\n",ref);

            ref=IDirectSoundBuffer_Release(*dsbo);
            ok(ref==0,"IDirectSoundBuffer_Release() has %d references, "
               "should have 0\n",ref);

            rc=IDirectSound3DBuffer_QueryInterface(buffer,
                                                   &IID_IDirectSoundBuffer,
                                                   (LPVOID *)dsbo);
            ok(rc==DS_OK && *dsbo!=NULL,"IDirectSound3DBuffer_QueryInterface() "
               "failed: %08lx\n",rc);

            /* DSOUND: Error: Invalid buffer */
            rc=IDirectSound3DBuffer_GetAllParameters(buffer,0);
            ok(rc==DSERR_INVALIDPARAM,"IDirectSound3DBuffer_GetAllParameters() "
               "failed: %08lx\n",rc);

            ZeroMemory(&buffer_param, sizeof(buffer_param));

            /* DSOUND: Error: Invalid buffer */
            rc=IDirectSound3DBuffer_GetAllParameters(buffer,&buffer_param);
            ok(rc==DSERR_INVALIDPARAM,"IDirectSound3DBuffer_GetAllParameters() "
               "failed: %08lx\n",rc);

            buffer_param.dwSize=sizeof(buffer_param);
            rc=IDirectSound3DBuffer_GetAllParameters(buffer,&buffer_param);
            ok(rc==DS_OK,"IDirectSound3DBuffer_GetAllParameters() failed: %08lx\n", rc);
        }
        if (set_volume) {
            if (dsbcaps.dwFlags & DSBCAPS_CTRLVOLUME) {
                LONG val;
                rc=IDirectSoundBuffer_GetVolume(*dsbo,&val);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetVolume() failed: %08lx\n", rc);

                rc=IDirectSoundBuffer_SetVolume(*dsbo,volume);
                ok(rc==DS_OK,"IDirectSoundBuffer_SetVolume() failed: %08lx\n", rc);
            } else {
                /* DSOUND: Error: Buffer does not have CTRLVOLUME */
                rc=IDirectSoundBuffer_GetVolume(*dsbo,&volume);
                ok(rc==DSERR_CONTROLUNAVAIL,"IDirectSoundBuffer_GetVolume() "
                   "should have returned DSERR_CONTROLUNAVAIL, returned: %08lx\n", rc);
            }
        }

        if (set_pan) {
            if (dsbcaps.dwFlags & DSBCAPS_CTRLPAN) {
                LONG val;
                rc=IDirectSoundBuffer_GetPan(*dsbo,&val);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetPan() failed: %08lx\n", rc);

                rc=IDirectSoundBuffer_SetPan(*dsbo,pan);
                ok(rc==DS_OK,"IDirectSoundBuffer_SetPan() failed: %08lx\n", rc);
            } else {
                /* DSOUND: Error: Buffer does not have CTRLPAN */
                rc=IDirectSoundBuffer_GetPan(*dsbo,&pan);
                ok(rc==DSERR_CONTROLUNAVAIL,"IDirectSoundBuffer_GetPan() "
                   "should have returned DSERR_CONTROLUNAVAIL, returned: %08lx\n", rc);
            }
        }

        /* try an offset past the end of the buffer */
        rc = IDirectSoundBuffer_Lock(*dsbo, dsbcaps.dwBufferBytes, 0, &buffer1,
                                      &length1, NULL, NULL,
                                      DSBLOCK_ENTIREBUFFER);
        ok(rc==DSERR_INVALIDPARAM, "IDirectSoundBuffer_Lock() should have "
           "returned DSERR_INVALIDPARAM, returned %08lx\n", rc);

        /* try a size larger than the buffer */
        rc = IDirectSoundBuffer_Lock(*dsbo, 0, dsbcaps.dwBufferBytes + 1,
                                     &buffer1, &length1, NULL, NULL,
                                     DSBLOCK_FROMWRITECURSOR);
        ok(rc==DSERR_INVALIDPARAM, "IDirectSoundBuffer_Lock() should have "
           "returned DSERR_INVALIDPARAM, returned %08lx\n", rc);

        state.wave=wave_generate_la(&wfx,duration,&state.wave_len,ieee);

        state.dsbo=*dsbo;
        state.wfx=&wfx;
        state.buffer_size=dsbcaps.dwBufferBytes;
        state.played=state.written=state.offset=0;
        buffer_refill8(&state,state.buffer_size);

        rc=IDirectSoundBuffer_Play(*dsbo,0,0,DSBPLAY_LOOPING);
        ok(rc==DS_OK,"IDirectSoundBuffer_Play() failed: %08lx\n", rc);

        rc=IDirectSoundBuffer_GetStatus(*dsbo,&status);
        ok(rc==DS_OK,"IDirectSoundBuffer_GetStatus() failed: %08lx\n", rc);
        ok(status==(DSBSTATUS_PLAYING|DSBSTATUS_LOOPING),
           "GetStatus: bad status: %lx\n",status);

        if (listener) {
            ZeroMemory(&listener_param,sizeof(listener_param));
            listener_param.dwSize=sizeof(listener_param);
            rc=IDirectSound3DListener_GetAllParameters(listener,&listener_param);
            ok(rc==DS_OK,"IDirectSound3dListener_GetAllParameters() "
               "failed: %08lx\n",rc);
            if (move_listener) {
                listener_param.vPosition.x = -5.0f;
                listener_param.vVelocity.x = (float)(10.0/duration);
            }
            rc=IDirectSound3DListener_SetAllParameters(listener,
                                                       &listener_param,
                                                       DS3D_IMMEDIATE);
            ok(rc==DS_OK,"IDirectSound3dListener_SetPosition() failed: %08lx\n", rc);
        }
        if (buffer3d) {
            if (move_sound) {
                buffer_param.vPosition.x = 100.0f;
                buffer_param.vVelocity.x = (float)(-200.0/duration);
            }
            buffer_param.flMinDistance = 10;
            rc=IDirectSound3DBuffer_SetAllParameters(buffer,&buffer_param,
                                                     DS3D_IMMEDIATE);
            ok(rc==DS_OK,"IDirectSound3dBuffer_SetPosition() failed: %08lx\n", rc);
        }

        start_time=GetTickCount();
        while (buffer_service8(&state)) {
            WaitForSingleObject(GetCurrentProcess(),TIME_SLICE);
            now=GetTickCount();
            if (listener && move_listener) {
                listener_param.vPosition.x = (float)(-5.0+10.0*(now-start_time)/1000/duration);
                if (winetest_debug>2)
                    trace("listener position=%g\n",listener_param.vPosition.x);
                rc=IDirectSound3DListener_SetPosition(listener,
                    listener_param.vPosition.x,listener_param.vPosition.y,
                    listener_param.vPosition.z,DS3D_IMMEDIATE);
                ok(rc==DS_OK,"IDirectSound3dListener_SetPosition() failed: %08lx\n",rc);
            }
            if (buffer3d && move_sound) {
                buffer_param.vPosition.x = (float)(100-200.0*(now-start_time)/1000/duration);
                if (winetest_debug>2)
                    trace("sound position=%g\n",buffer_param.vPosition.x);
                rc=IDirectSound3DBuffer_SetPosition(buffer,
                    buffer_param.vPosition.x,buffer_param.vPosition.y,
                    buffer_param.vPosition.z,DS3D_IMMEDIATE);
                ok(rc==DS_OK,"IDirectSound3dBuffer_SetPosition() failed: %08lx\n", rc);
            }
        }
        /* Check the sound duration was within 10% of the expected value */
        now=GetTickCount();
        ok(fabs(1000*duration-now+start_time)<=100*duration,
           "The sound played for %ld ms instead of %g ms\n",
           now-start_time,1000*duration);

        HeapFree(GetProcessHeap(), 0, state.wave);
        if (is_primary) {
            /* Set the CooperativeLevel back to normal */
            /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
            rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
            ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel(DSSCL_NORMAL) "
               "failed: %08lx\n",rc);
        }
        if (buffer3d) {
            ref=IDirectSound3DBuffer_Release(buffer);
            ok(ref==0,"IDirectSound3DBuffer_Release() has %d references, "
               "should have 0\n",ref);
        }
    }
}

static HRESULT test_secondary8(LPGUID lpGuid, BOOL play,
                               BOOL has_3d, BOOL has_3dbuffer,
                               BOOL has_listener, BOOL has_duplicate,
                               BOOL move_listener, BOOL move_sound)
{
    HRESULT rc;
    LPDIRECTSOUND8 dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL,secondary=NULL;
    LPDIRECTSOUND3DLISTENER listener=NULL;
    DSBUFFERDESC bufdesc;
    WAVEFORMATEX wfx, wfx1;
    int ref;

    /* Create the DirectSound object */
    rc = DirectSoundCreate8(lpGuid, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER,"DirectSoundCreate8() failed: %08lx\n", rc);
    if (rc!=DS_OK)
        return rc;

    /* We must call SetCooperativeLevel before creating primary buffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel(DSSCL_PRIORITY) failed: %08lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    if (has_3d)
        bufdesc.dwFlags|=DSBCAPS_CTRL3D;
    else
        bufdesc.dwFlags|=(DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLPAN);
    rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok((rc==DS_OK && primary!=NULL) || (rc == DSERR_CONTROLUNAVAIL),
       "IDirectSound8_CreateSoundBuffer() failed to create a %sprimary buffer: %08lx\n",has_3d?"3D ":"", rc);
    if (rc == DSERR_CONTROLUNAVAIL)
        trace("  No Primary\n");
    else if (rc==DS_OK && primary!=NULL) {
        rc=IDirectSoundBuffer_GetFormat(primary,&wfx1,sizeof(wfx1),NULL);
        ok(rc==DS_OK,"IDirectSoundBuffer8_Getformat() failed: %08lx\n", rc);
        if (rc!=DS_OK)
            goto EXIT1;

        if (has_listener) {
            rc=IDirectSoundBuffer_QueryInterface(primary,
                                                 &IID_IDirectSound3DListener,
                                                 (void **)&listener);
            ok(rc==DS_OK && listener!=NULL,
               "IDirectSoundBuffer_QueryInterface() failed to get a 3D "
               "listener %08lx\n",rc);
            ref=IDirectSoundBuffer_Release(primary);
            ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
               "should have 0\n",ref);
            if (rc==DS_OK && listener!=NULL) {
                DS3DLISTENER listener_param;
                ZeroMemory(&listener_param,sizeof(listener_param));
                /* DSOUND: Error: Invalid buffer */
                rc=IDirectSound3DListener_GetAllParameters(listener,0);
                ok(rc==DSERR_INVALIDPARAM,
                   "IDirectSound3dListener_GetAllParameters() should have "
                   "returned DSERR_INVALIDPARAM, returned: %08lx\n", rc);

                /* DSOUND: Error: Invalid buffer */
                rc=IDirectSound3DListener_GetAllParameters(listener,
                                                           &listener_param);
                ok(rc==DSERR_INVALIDPARAM,
                   "IDirectSound3dListener_GetAllParameters() should have "
                   "returned DSERR_INVALIDPARAM, returned: %08lx\n", rc);

                listener_param.dwSize=sizeof(listener_param);
                rc=IDirectSound3DListener_GetAllParameters(listener,
                                                           &listener_param);
                ok(rc==DS_OK,"IDirectSound3dListener_GetAllParameters() "
                   "failed: %08lx\n",rc);
            } else {
                ok(listener==NULL, "IDirectSoundBuffer_QueryInterface() "
                   "failed but returned a listener anyway\n");
                ok(rc!=DS_OK, "IDirectSoundBuffer_QueryInterface() succeeded "
                   "but returned a NULL listener\n");
                if (listener) {
                    ref=IDirectSound3DListener_Release(listener);
                    ok(ref==0,"IDirectSound3dListener_Release() listener has "
                       "%d references, should have 0\n",ref);
                }
                goto EXIT2;
            }
        }

        init_format(&wfx,WAVE_FORMAT_PCM,22050,16,2);
        secondary=NULL;
        ZeroMemory(&bufdesc, sizeof(bufdesc));
        bufdesc.dwSize=sizeof(bufdesc);
        bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
        if (has_3d)
            bufdesc.dwFlags|=DSBCAPS_CTRL3D;
        else
            bufdesc.dwFlags|=
                (DSBCAPS_CTRLFREQUENCY|DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLPAN);
        bufdesc.dwBufferBytes=align(wfx.nAvgBytesPerSec*BUFFER_LEN/1000,
                                    wfx.nBlockAlign);
        bufdesc.lpwfxFormat=&wfx;
        if (has_3d) {
            /* a stereo 3D buffer should fail */
            rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DSERR_INVALIDPARAM,
               "IDirectSound8_CreateSoundBuffer(secondary) should have "
               "returned DSERR_INVALIDPARAM, returned %08lx\n", rc);
            init_format(&wfx,WAVE_FORMAT_PCM,22050,16,1);

            /* Invalid flag combination */
            bufdesc.dwFlags|=DSBCAPS_CTRLPAN;
            rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DSERR_INVALIDPARAM,
               "IDirectSound8_CreateSoundBuffer(secondary) should have "
               "returned DSERR_INVALIDPARAM, returned %08lx\n", rc);
            bufdesc.dwFlags&=~DSBCAPS_CTRLPAN;
        }

        if (winetest_interactive) {
            trace("  Testing a %s%ssecondary buffer %s%s%s%sat %ldx%dx%d "
                  "with a primary buffer at %ldx%dx%d\n",
                  has_3dbuffer?"3D ":"",
                  has_duplicate?"duplicated ":"",
                  listener!=NULL||move_sound?"with ":"",
                  move_listener?"moving ":"",
                  listener!=NULL?"listener ":"",
                  listener&&move_sound?"and moving sound ":move_sound?
                  "moving sound ":"",
                  wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels,
                  wfx1.nSamplesPerSec,wfx1.wBitsPerSample,wfx1.nChannels);
        }
        rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
        ok(rc==DS_OK && secondary!=NULL,"IDirectSound8_CreateSoundBuffer() "
           "failed to create a %s%ssecondary buffer %s%s%s%sat %ldx%dx%d (%s): %08lx\n",
           has_3dbuffer?"3D ":"", has_duplicate?"duplicated ":"",
           listener!=NULL||move_sound?"with ":"", move_listener?"moving ":"",
           listener!=NULL?"listener ":"",
           listener&&move_sound?"and moving sound ":move_sound?
           "moving sound ":"",
           wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels,
           getDSBCAPS(bufdesc.dwFlags),rc);
        if (rc==DS_OK && secondary!=NULL) {
            if (!has_3d) {
                LONG refvol,vol,refpan,pan;

                /* Check the initial secondary buffer's volume and pan */
                rc=IDirectSoundBuffer_GetVolume(secondary,&vol);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetVolume(secondary) failed: %08lx\n",rc);
                ok(vol==0,"wrong volume for a new secondary buffer: %ld\n",vol);
                rc=IDirectSoundBuffer_GetPan(secondary,&pan);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetPan(secondary) failed: %08lx\n",rc);
                ok(pan==0,"wrong pan for a new secondary buffer: %ld\n",pan);

                /* Check that changing the secondary buffer's volume and pan
                 * does not impact the primary buffer's volume and pan
                 */
                rc=IDirectSoundBuffer_GetVolume(primary,&refvol);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetVolume(primary) failed: %08lx\n",rc);
                rc=IDirectSoundBuffer_GetPan(primary,&refpan);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetPan(primary) failed: %08lx\n",rc);

                rc=IDirectSoundBuffer_SetVolume(secondary,-1000);
                ok(rc==DS_OK,"IDirectSoundBuffer_SetVolume(secondary) failed: %08lx\n",rc);
                rc=IDirectSoundBuffer_GetVolume(secondary,&vol);
                ok(rc==DS_OK,"IDirectSoundBuffer_SetVolume(secondary) failed: %08lx\n",rc);
                ok(vol==-1000,"secondary: wrong volume %ld instead of -1000\n",
                   vol);
                rc=IDirectSoundBuffer_SetPan(secondary,-1000);
                ok(rc==DS_OK,"IDirectSoundBuffer_SetPan(secondary) failed: %08lx\n",rc);
                rc=IDirectSoundBuffer_GetPan(secondary,&pan);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetPan(secondary) failed: %08lx\n",rc);
                ok(pan==-1000,"secondary: wrong pan %ld instead of -1000\n",
                   pan);

                rc=IDirectSoundBuffer_GetVolume(primary,&vol);
                ok(rc==DS_OK,"IDirectSoundBuffer_`GetVolume(primary) failed: i%08lx\n",rc);
                ok(vol==refvol,"The primary volume changed from %ld to %ld\n",
                   refvol,vol);
                rc=IDirectSoundBuffer_GetPan(primary,&pan);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetPan(primary) failed: %08lx\n",rc);
                ok(pan==refpan,"The primary pan changed from %ld to %ld\n",
                   refpan,pan);

                rc=IDirectSoundBuffer_SetVolume(secondary,0);
                ok(rc==DS_OK,"IDirectSoundBuffer_SetVolume(secondary) failed: %08lx\n",rc);
                rc=IDirectSoundBuffer_SetPan(secondary,0);
                ok(rc==DS_OK,"IDirectSoundBuffer_SetPan(secondary) failed: %08lx\n",rc);
            } else {
                LONG pan;

                rc=IDirectSoundBuffer_GetPan(secondary,&pan);
                ok(rc==DSERR_CONTROLUNAVAIL,"IDirectSoundBuffer_GetPan() "
                   "should have returned DSERR_CONTROLUNAVAIL, returned: %08lx\n", rc);
                rc=IDirectSoundBuffer_SetPan(secondary,0);
                ok(rc==DSERR_CONTROLUNAVAIL,"IDirectSoundBuffer_SetPan() "
                   "should have returned DSERR_CONTROLUNAVAIL, returned: %08lx\n", rc);
            }

            if (has_duplicate) {
                LPDIRECTSOUNDBUFFER duplicated=NULL;

                /* DSOUND: Error: Invalid source buffer */
                rc=IDirectSound8_DuplicateSoundBuffer(dso,0,0);
                ok(rc==DSERR_INVALIDPARAM,
                   "IDirectSound8_DuplicateSoundBuffer() should have returned "
                   "DSERR_INVALIDPARAM, returned: %08lx\n",rc);

                /* DSOUND: Error: Invalid dest buffer */
                rc=IDirectSound8_DuplicateSoundBuffer(dso,secondary,0);
                ok(rc==DSERR_INVALIDPARAM,
                   "IDirectSound8_DuplicateSoundBuffer() should have returned "
                   "DSERR_INVALIDPARAM, returned: %08lx\n",rc);

                /* DSOUND: Error: Invalid source buffer */
                rc=IDirectSound8_DuplicateSoundBuffer(dso,0,&duplicated);
                ok(rc==DSERR_INVALIDPARAM,
                   "IDirectSound8_DuplicateSoundBuffer() should have returned "
                   "DSERR_INVALIDPARAM, returned: %08lx\n",rc);

                duplicated=NULL;
                rc=IDirectSound8_DuplicateSoundBuffer(dso,secondary,
                                                      &duplicated);
                ok(rc==DS_OK && duplicated!=NULL,
                   "IDirectSound8_DuplicateSoundBuffer() failed to duplicate "
                   "a secondary buffer: %08lx\n",rc);

                if (rc==DS_OK && duplicated!=NULL) {
                    ref=IDirectSoundBuffer_Release(secondary);
                    ok(ref==0,"IDirectSoundBuffer_Release() secondary has %d "
                       "references, should have 0\n",ref);
                    secondary=duplicated;
                }
            }

            if (rc==DS_OK && secondary!=NULL) {
                double duration;
                duration=(move_listener || move_sound?4.0:1.0);
                test_buffer8(dso,&secondary,FALSE,FALSE,0,FALSE,0,
                             winetest_interactive,duration,has_3dbuffer,
                             listener,move_listener,move_sound);
                ref=IDirectSoundBuffer_Release(secondary);
                ok(ref==0,"IDirectSoundBuffer_Release() %s has %d references, "
                   "should have 0\n",has_duplicate?"duplicated":"secondary",
                   ref);
            }
        }
EXIT1:
        if (has_listener) {
            if (listener) {
                ref=IDirectSound3DListener_Release(listener);
                ok(ref==0,"IDirectSound3dListener_Release() listener has %d "
                   "references, should have 0\n",ref);
            }
        } else {
            ref=IDirectSoundBuffer_Release(primary);
            ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
               "should have 0\n",ref);
        }
    } else {
        ok(primary==NULL,"IDirectSound8_CreateSoundBuffer(primary) failed "
           "but primary created anyway\n");
        ok(rc!=DS_OK,"IDirectSound8_CreateSoundBuffer(primary) succeeded "
           "but primary not created\n");
        if (primary) {
            ref=IDirectSoundBuffer_Release(primary);
            ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
               "should have 0\n",ref);
        }
    }
EXIT2:
    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel(DSSCL_NORMAL) failed: %08lx\n",rc);

EXIT:
    ref=IDirectSound8_Release(dso);
    ok(ref==0,"IDirectSound8_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static HRESULT test_for_driver8(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND8 dso=NULL;
    int ref;

    /* Create the DirectSound object */
    rc = DirectSoundCreate8(lpGuid, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCreate8() failed: %08lx\n",rc);
    if (rc!=DS_OK)
        return rc;

    ref=IDirectSound8_Release(dso);
    ok(ref==0,"IDirectSound8_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static HRESULT test_primary8(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND8 dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    int ref, i;

    /* Create the DirectSound object */
    rc = DirectSoundCreate8(lpGuid, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER,"DirectSoundCreate8() failed: %08lx\n", rc);
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound8_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound8_GetCaps() failed: %08lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before calling CreateSoundBuffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel(DSSCL_PRIORITY) failed: %08lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* Testing the primary buffer */
    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLPAN;
    rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok((rc==DS_OK && primary!=NULL) || (rc == DSERR_CONTROLUNAVAIL),
       "IDirectSound8_CreateSoundBuffer() failed to create a primary buffer: %08lx\n",rc);
    if (rc == DSERR_CONTROLUNAVAIL)
        trace("  No Primary\n");
    else if (rc==DS_OK && primary!=NULL) {
        test_buffer8(dso,&primary,TRUE,TRUE,0,TRUE,0,
                     winetest_interactive && !(dscaps.dwFlags & DSCAPS_EMULDRIVER),
                     1.0,FALSE,NULL,FALSE,FALSE);
        if (winetest_interactive) {
            LONG volume,pan;

            volume = DSBVOLUME_MAX;
            for (i = 0; i < 6; i++) {
                test_buffer8(dso,&primary,TRUE,TRUE,volume,TRUE,0,
                             winetest_interactive && !(dscaps.dwFlags & DSCAPS_EMULDRIVER),
                             1.0,FALSE,NULL,FALSE,FALSE);
                volume -= ((DSBVOLUME_MAX-DSBVOLUME_MIN) / 40);
            }

            pan = DSBPAN_LEFT;
            for (i = 0; i < 7; i++) {
                test_buffer8(dso,&primary,TRUE,TRUE,0,TRUE,pan,
                             winetest_interactive && !(dscaps.dwFlags & DSCAPS_EMULDRIVER),
                             1.0,FALSE,NULL,FALSE,FALSE);
                pan += ((DSBPAN_RIGHT-DSBPAN_LEFT) / 6);
            }
        }
        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel(DSSCL_NORMAL) failed: %08lx\n",rc);

EXIT:
    ref=IDirectSound8_Release(dso);
    ok(ref==0,"IDirectSound8_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static HRESULT test_primary_3d8(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND8 dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    int ref;

    /* Create the DirectSound object */
    rc = DirectSoundCreate8(lpGuid, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER,"DirectSoundCreate8() failed: %08lx\n", rc);
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound8_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound8_GetCaps failed: %08lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before calling CreateSoundBuffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel(DSSCL_PRIORITY) failed: %08lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,"IDirectSound8_CreateSoundBuffer() failed "
       "to create a primary buffer: %08lx\n",rc);
    if (rc==DS_OK && primary!=NULL) {
        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
        primary=NULL;
        ZeroMemory(&bufdesc, sizeof(bufdesc));
        bufdesc.dwSize=sizeof(bufdesc);
        bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRL3D;
        rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
        ok(rc==DS_OK && primary!=NULL,"IDirectSound8_CreateSoundBuffer() "
           "failed to create a 3D primary buffer: %08lx\n",rc);
        if (rc==DS_OK && primary!=NULL) {
            test_buffer8(dso,&primary,TRUE,FALSE,0,FALSE,0,
                         winetest_interactive && !(dscaps.dwFlags & DSCAPS_EMULDRIVER),
                         1.0,FALSE,NULL,FALSE,FALSE);
            ref=IDirectSoundBuffer_Release(primary);
            ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
               "should have 0\n",ref);
        }
    }
    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel(DSSCL_NORMAL) failed: %08lx\n",rc);

EXIT:
    ref=IDirectSound8_Release(dso);
    ok(ref==0,"IDirectSound8_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static HRESULT test_primary_3d_with_listener8(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND8 dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    int ref;

    /* Create the DirectSound object */
    rc = DirectSoundCreate8(lpGuid, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER,"DirectSoundCreate8() failed: %08lx\n", rc);
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound8_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound8_GetCaps() failed: %08lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before calling CreateSoundBuffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel(DSSCL_PRIORITY) failed: %08lx\n",rc);
    if (rc!=DS_OK)
        goto EXIT;
    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRL3D;
    rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,"IDirectSound8_CreateSoundBuffer() failed "
       "to create a 3D primary buffer %08lx\n",rc);
    if (rc==DS_OK && primary!=NULL) {
        LPDIRECTSOUND3DLISTENER listener=NULL;
        rc=IDirectSoundBuffer_QueryInterface(primary,
                                             &IID_IDirectSound3DListener,
                                             (void **)&listener);
        ok(rc==DS_OK && listener!=NULL,"IDirectSoundBuffer_QueryInterface() "
           "failed to get a 3D listener: %08lx\n",rc);
        if (rc==DS_OK && listener!=NULL) {
            LPDIRECTSOUNDBUFFER temp_buffer=NULL;

            /* Checking the COM interface */
            rc=IDirectSoundBuffer_QueryInterface(primary,
                                                 &IID_IDirectSoundBuffer,
                                                 (LPVOID *)&temp_buffer);
            ok(rc==DS_OK && temp_buffer!=NULL,
               "IDirectSoundBuffer_QueryInterface() failed: %08lx\n", rc);
            ok(temp_buffer==primary,"COM interface broken: %p != %p\n",temp_buffer,primary);
            if (rc==DS_OK && temp_buffer!=NULL) {
                ref=IDirectSoundBuffer_Release(temp_buffer);
                ok(ref==1,"IDirectSoundBuffer_Release() has %d references, "
                   "should have 1\n",ref);

                temp_buffer=NULL;
                rc=IDirectSound3DListener_QueryInterface(listener,
                    &IID_IDirectSoundBuffer,(LPVOID *)&temp_buffer);
                ok(rc==DS_OK && temp_buffer!=NULL,
                   "IDirectSoundBuffer_QueryInterface() failed: %08lx\n", rc);
                ok(temp_buffer==primary,"COM interface broken: %p != %p\n",temp_buffer,primary);
                ref=IDirectSoundBuffer_Release(temp_buffer);
                ok(ref==1,"IDirectSoundBuffer_Release() has %d references, "
                   "should have 1\n",ref);

                /* Testing the buffer */
                test_buffer8(dso,&primary,TRUE,FALSE,0,FALSE,0,
                             winetest_interactive && !(dscaps.dwFlags & DSCAPS_EMULDRIVER),
                             1.0,FALSE,listener,FALSE,FALSE);
            }

            /* Testing the reference counting */
            ref=IDirectSound3DListener_Release(listener);
            ok(ref==0,"IDirectSound3DListener_Release() listener has %d "
               "references, should have 0\n",ref);
        }

        /* Testing the reference counting */
        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

EXIT:
    ref=IDirectSound8_Release(dso);
    ok(ref==0,"IDirectSound8_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
return DSERR_GENERIC;

    return rc;
}

static unsigned driver_count = 0;

static BOOL WINAPI dsenum_callback(LPGUID lpGuid, LPCSTR lpcstrDescription,
                                   LPCSTR lpcstrModule, LPVOID lpContext)
{
    HRESULT rc;
    trace("*** Testing %s - %s ***\n",lpcstrDescription,lpcstrModule);
    driver_count++;

    rc = test_for_driver8(lpGuid);
    if (rc == DSERR_NODRIVER) {
        trace("  No Driver\n");
        return TRUE;
    } else if (rc == DSERR_ALLOCATED) {
        trace("  Already In Use\n");
        return TRUE;
    } else if (rc == E_FAIL) {
        trace("  No Device\n");
        return TRUE;
    }

    trace("  Testing the primary buffer\n");
    test_primary8(lpGuid);

    trace("  Testing 3D primary buffer\n");
    test_primary_3d8(lpGuid);

    trace("  Testing 3D primary buffer with listener\n");
    test_primary_3d_with_listener8(lpGuid);

    /* Testing secondary buffers */
    test_secondary8(lpGuid,winetest_interactive,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE);
    test_secondary8(lpGuid,winetest_interactive,FALSE,FALSE,FALSE,TRUE, FALSE,FALSE);

    /* Testing 3D secondary buffers */
    test_secondary8(lpGuid,winetest_interactive,TRUE, FALSE,FALSE,FALSE,FALSE,FALSE);
    test_secondary8(lpGuid,winetest_interactive,TRUE, TRUE, FALSE,FALSE,FALSE,FALSE);
    test_secondary8(lpGuid,winetest_interactive,TRUE, TRUE, FALSE,TRUE, FALSE,FALSE);
    test_secondary8(lpGuid,winetest_interactive,TRUE, FALSE,TRUE, FALSE,FALSE,FALSE);
    test_secondary8(lpGuid,winetest_interactive,TRUE, FALSE,TRUE, TRUE, FALSE,FALSE);
    test_secondary8(lpGuid,winetest_interactive,TRUE, TRUE, TRUE, FALSE,FALSE,FALSE);
    test_secondary8(lpGuid,winetest_interactive,TRUE, TRUE, TRUE, TRUE, FALSE,FALSE);
    test_secondary8(lpGuid,winetest_interactive,TRUE, TRUE, TRUE, FALSE,TRUE, FALSE);
    test_secondary8(lpGuid,winetest_interactive,TRUE, TRUE, TRUE, FALSE,FALSE,TRUE );
    test_secondary8(lpGuid,winetest_interactive,TRUE, TRUE, TRUE, FALSE,TRUE, TRUE );

    return TRUE;
}

static void ds3d8_tests(void)
{
    HRESULT rc;
    rc = DirectSoundEnumerateA(dsenum_callback, NULL);
    ok(rc==DS_OK,"DirectSoundEnumerateA() failed: %08lx\n",rc);
    trace("tested %u DirectSound drivers\n", driver_count);
}

START_TEST(ds3d8)
{
    CoInitialize(NULL);

    ds3d8_tests();

    CoUninitialize();
}
