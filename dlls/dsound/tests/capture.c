/*
 * Unit tests for capture functions
 *
 * Copyright (c) 2002 Francois Gouget
 * Copyright (c) 2003 Robert Reif
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

#define NOTIFICATIONS    5

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

    LPDIRECTSOUNDCAPTUREBUFFER dscbo;
    LPWAVEFORMATEX wfx;
    DSBPOSITIONNOTIFY posnotify[NOTIFICATIONS];
    HANDLE event[NOTIFICATIONS];
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

static void test_capture_buffer(LPDIRECTSOUNDCAPTURE dsco, 
				LPDIRECTSOUNDCAPTUREBUFFER dscbo, int record)
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
    for (i = 0; i < NOTIFICATIONS; i++)
	state.event[i] = CreateEvent( NULL, FALSE, FALSE, NULL );
    state.size = dscbcaps.dwBufferBytes / NOTIFICATIONS;

    rc=IDirectSoundCaptureBuffer_QueryInterface(dscbo,&IID_IDirectSoundNotify,(void **)&(state.notify));
    ok((rc==DS_OK)&&(state.notify!=NULL),"QueryInterface failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
	return;

    for (i = 0; i < NOTIFICATIONS; i++) {
	state.posnotify[i].dwOffset = (i * state.size) + state.size - 1;
	state.posnotify[i].hEventNotify = state.event[i];
    }

    rc=IDirectSoundNotify_SetNotificationPositions(state.notify,NOTIFICATIONS,state.posnotify);
    ok(rc==DS_OK,"SetNotificationPositions failed: 0x%lx\n",rc);
    if (rc!=DS_OK)
	return;

    rc=IDirectSoundNotify_Release(state.notify);
    ok(rc==0,"Release: 0x%lx\n",rc);
    if (rc!=0)
	return;

    if (record) {
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
	    rc=MsgWaitForMultipleObjects( NOTIFICATIONS, state.event, FALSE, 3000, QS_ALLEVENTS );
	    ok(rc==(WAIT_OBJECT_0+(i%NOTIFICATIONS)),"MsgWaitForMultipleObjects failed: 0x%lx\n",rc);
	    if (rc!=(WAIT_OBJECT_0+(i%NOTIFICATIONS))) {
		ok((rc==WAIT_TIMEOUT)||(rc==WAIT_FAILED),"Wrong notification: should be %d, got %ld\n", 
		    i%NOTIFICATIONS,rc-WAIT_OBJECT_0);
	    } else
		break;
	    if (!capture_buffer_service(&state))
		break;
	}

	rc=IDirectSoundCaptureBuffer_Stop(dscbo);
	ok(rc==DS_OK,"Stop: 0x%lx\n",rc);
	if (rc!=DS_OK)
	    return;
    }
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
	ok(ref==0,"IDirectSoundCapture_Release has %d references, should have 0\n",ref);
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
	ok(ref==0,"IDirectSoundCaptureBuffer_Release has %d references, should have 0\n",ref);
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
	ok(ref==0,"IDirectSoundCaptureBuffer_Release has %d references, should have 0\n",ref);
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
	ok(ref==0,"IDirectSoundCaptureBuffer_Release has %d references, should have 0\n",ref);
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
	ok(ref==0,"IDirectSoundCaptureBuffer_Release has %d references, should have 0\n",ref);
    }

    for (f=0;f<NB_FORMATS;f++) {
	dscbo=NULL;
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
	ok((rc==DS_OK)&&(dscbo!=NULL),"CreateCaptureBuffer failed to create a capture buffer 0x%lx\n",rc);
	if (rc==DS_OK) {
	    test_capture_buffer(dsco, dscbo, winetest_interactive);
	    ref=IDirectSoundCaptureBuffer_Release(dscbo);
	    ok(ref==0,"IDirectSoundCaptureBuffer_Release has %d references, should have 0\n",ref);
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
	ok(ref==0,"IDirectSoundCapture_Release has %d references, should have 0\n",ref);
    }

    return TRUE;
}

static void capture_tests()
{
    HRESULT rc;
    rc=DirectSoundCaptureEnumerateA(&dscenum_callback,NULL);
    ok(rc==DS_OK,"DirectSoundCaptureEnumerate failed: %ld\n",rc);
}

START_TEST(capture)
{
    capture_tests();
}
