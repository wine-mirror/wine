/*
 * Test winmm sound playback in each sound format
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "mmsystem.h"
#include "mmddk.h"

#include "winmm_test.h"

/*
 * Note that in most of this test we may get MMSYSERR_BADDEVICEID errors
 * at about any time if the user starts another application that uses the
 * sound device. So we should not report these as test failures.
 *
 * This test can play a test tone. But this only makes sense if someone
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

const char * dev_name(int device)
{
    static char name[16];
    if (device == WAVE_MAPPER)
        return "WAVE_MAPPER";
    sprintf(name, "%d", device);
    return name;
}

const char* mmsys_error(MMRESULT error)
{
#define ERR_TO_STR(dev) case dev: return #dev
    static char	unknown[32];
    switch (error) {
    ERR_TO_STR(MMSYSERR_NOERROR);
    ERR_TO_STR(MMSYSERR_ERROR);
    ERR_TO_STR(MMSYSERR_BADDEVICEID);
    ERR_TO_STR(MMSYSERR_NOTENABLED);
    ERR_TO_STR(MMSYSERR_ALLOCATED);
    ERR_TO_STR(MMSYSERR_INVALHANDLE);
    ERR_TO_STR(MMSYSERR_NODRIVER);
    ERR_TO_STR(MMSYSERR_NOMEM);
    ERR_TO_STR(MMSYSERR_NOTSUPPORTED);
    ERR_TO_STR(MMSYSERR_BADERRNUM);
    ERR_TO_STR(MMSYSERR_INVALFLAG);
    ERR_TO_STR(MMSYSERR_INVALPARAM);
    ERR_TO_STR(WAVERR_BADFORMAT);
    ERR_TO_STR(WAVERR_STILLPLAYING);
    ERR_TO_STR(WAVERR_UNPREPARED);
    ERR_TO_STR(WAVERR_SYNC);
    }
    sprintf(unknown, "Unknown(0x%08x)", error);
    return unknown;
#undef ERR_TO_STR
}

static const char * wave_out_error(MMRESULT error)
{
    static char msg[1024];
    static char long_msg[1100];
    MMRESULT rc;

    rc = waveOutGetErrorText(error, msg, sizeof(msg));
    if (rc != MMSYSERR_NOERROR)
        sprintf(long_msg, "waveOutGetErrorText(%x) failed with error %x", error, rc);
    else
        sprintf(long_msg, "%s(%s)", mmsys_error(error), msg);
    return long_msg;
}

const char * wave_open_flags(DWORD flags)
{
    static char msg[1024];
    int first = TRUE;
    msg[0] = 0;
    if ((flags & CALLBACK_TYPEMASK) == CALLBACK_EVENT) {
        strcat(msg, "CALLBACK_EVENT");
        first = FALSE;
    }
    if ((flags & CALLBACK_TYPEMASK) == CALLBACK_FUNCTION) {
        if (!first) strcat(msg, "|");
        strcat(msg, "CALLBACK_FUNCTION");
        first = FALSE;
    }
    if ((flags & CALLBACK_TYPEMASK) == CALLBACK_NULL) {
        if (!first) strcat(msg, "|");
        strcat(msg, "CALLBACK_NULL");
        first = FALSE;
    }
    if ((flags & CALLBACK_TYPEMASK) == CALLBACK_THREAD) {
        if (!first) strcat(msg, "|");
        strcat(msg, "CALLBACK_THREAD");
        first = FALSE;
    }
    if ((flags & CALLBACK_TYPEMASK) == CALLBACK_WINDOW) {
        if (!first) strcat(msg, "|");
        strcat(msg, "CALLBACK_WINDOW");
        first = FALSE;
    }
    if ((flags & WAVE_ALLOWSYNC) == WAVE_ALLOWSYNC) {
        if (!first) strcat(msg, "|");
        strcat(msg, "WAVE_ALLOWSYNC");
        first = FALSE;
    }
    if ((flags & WAVE_FORMAT_DIRECT) == WAVE_FORMAT_DIRECT) {
        if (!first) strcat(msg, "|");
        strcat(msg, "WAVE_FORMAT_DIRECT");
        first = FALSE;
    }
    if ((flags & WAVE_FORMAT_QUERY) == WAVE_FORMAT_QUERY) {
        if (!first) strcat(msg, "|");
        strcat(msg, "WAVE_FORMAT_QUERY");
        first = FALSE;
    }
    if ((flags & WAVE_MAPPED) == WAVE_MAPPED) {
        if (!first) strcat(msg, "|");
        strcat(msg, "WAVE_MAPPED");
        first = FALSE;
    }
    return msg;
}

static const char * wave_out_caps(DWORD dwSupport)
{
#define ADD_FLAG(f) if (dwSupport & f) strcat(msg, " " #f)
    static char msg[256];
    msg[0] = 0;

    ADD_FLAG(WAVECAPS_PITCH);
    ADD_FLAG(WAVECAPS_PLAYBACKRATE);
    ADD_FLAG(WAVECAPS_VOLUME);
    ADD_FLAG(WAVECAPS_LRVOLUME);
    ADD_FLAG(WAVECAPS_SYNC);
    ADD_FLAG(WAVECAPS_SAMPLEACCURATE);

    return msg[0] ? msg + 1 : "";
#undef ADD_FLAG
}

static const char * wave_time_format(UINT type)
{
    static char msg[32];
#define TIME_FORMAT(f) case f: return #f
    switch (type) {
    TIME_FORMAT(TIME_MS);
    TIME_FORMAT(TIME_SAMPLES);
    TIME_FORMAT(TIME_BYTES);
    TIME_FORMAT(TIME_SMPTE);
    TIME_FORMAT(TIME_MIDI);
    TIME_FORMAT(TIME_TICKS);
    }
#undef TIME_FORMAT
    sprintf(msg, "Unknown(0x%04x)", type);
    return msg;
}

static void check_position(int device, HWAVEOUT wout, DWORD bytes, LPWAVEFORMATEX pwfx )
{
    MMTIME mmtime;
    DWORD samples;
    MMRESULT rc;

    samples=bytes*8/pwfx->wBitsPerSample/pwfx->nChannels;

    mmtime.wType = TIME_BYTES;
    rc=waveOutGetPosition(wout, &mmtime, sizeof(mmtime));
    ok(rc==MMSYSERR_NOERROR,
       "waveOutGetPosition: device=%s rc=%s\n",dev_name(device),wave_out_error(rc));
    if (mmtime.wType == TIME_BYTES)
        ok(mmtime.u.cb==bytes,
           "waveOutGetPosition returned %ld bytes, should be %ld\n",
           mmtime.u.cb, bytes);
    else
        trace("TIME_BYTES not supported, returned %s\n",wave_time_format(mmtime.wType));

    mmtime.wType = TIME_SAMPLES;
    rc=waveOutGetPosition(wout, &mmtime, sizeof(mmtime));
    ok(rc==MMSYSERR_NOERROR,
       "waveOutGetPosition: device=%s rc=%s\n",dev_name(device),wave_out_error(rc));
    if (mmtime.wType == TIME_SAMPLES)
        ok(mmtime.u.sample==samples,
           "waveOutGetPosition returned %ld samples, should be %ld\n",
           mmtime.u.sample, samples);
    else
        trace("TIME_SAMPLES not supported, returned %s\n",wave_time_format(mmtime.wType));

    mmtime.wType = TIME_MS;
    rc=waveOutGetPosition(wout, &mmtime, sizeof(mmtime));
    ok(rc==MMSYSERR_NOERROR,
       "waveOutGetPosition: device=%s rc=%s\n",dev_name(device),wave_out_error(rc));
    if (mmtime.wType == TIME_MS)
        ok(mmtime.u.ms==samples*1000/pwfx->nSamplesPerSec,
           "waveOutGetPosition returned %ld ms, should be %ld\n",
           mmtime.u.ms, samples*1000/pwfx->nSamplesPerSec);
    else
        trace("TIME_MS not supported, returned %s\n",wave_time_format(mmtime.wType));

    mmtime.wType = TIME_SMPTE;
    rc=waveOutGetPosition(wout, &mmtime, sizeof(mmtime));
    ok(rc==MMSYSERR_NOERROR,
       "waveOutGetPosition: device=%s rc=%s\n",dev_name(device),wave_out_error(rc));
    if (mmtime.wType == TIME_SMPTE)
    {
        double duration=((double)samples)/pwfx->nSamplesPerSec;
        BYTE frames=ceil(fmod(duration*mmtime.u.smpte.fps, mmtime.u.smpte.fps));
        ok(mmtime.u.smpte.hour==(BYTE)(floor(duration/(60*60))) &&
           mmtime.u.smpte.min==(BYTE)(fmod(floor(duration/60), 60)) &&
           mmtime.u.smpte.sec==(BYTE)(fmod(duration,60)) &&
           mmtime.u.smpte.frame==frames,
           "waveOutGetPosition returned %d:%d:%d %d, should be %d:%d:%d %d\n",
           mmtime.u.smpte.hour, mmtime.u.smpte.min, mmtime.u.smpte.sec, mmtime.u.smpte.frame,
           (BYTE)(floor(duration/(60*60))),
           (BYTE)(fmod(floor(duration/60), 60)),
           (BYTE)(fmod(duration,60)),
           frames);
    }
    else
        trace("TIME_SMPTE not supported, returned %s\n",wave_time_format(mmtime.wType));
}

static void wave_out_test_deviceOut(int device, double duration, LPWAVEFORMATEX pwfx, DWORD format, DWORD flags, LPWAVEOUTCAPS pcaps)
{
    HWAVEOUT wout;
    HANDLE hevent;
    WAVEHDR frag;
    MMRESULT rc;
    DWORD volume;
    WORD nChannels = pwfx->nChannels;
    WORD wBitsPerSample = pwfx->wBitsPerSample;
    DWORD nSamplesPerSec = pwfx->nSamplesPerSec;

    hevent=CreateEvent(NULL,FALSE,FALSE,NULL);
    ok(hevent!=NULL,"CreateEvent: error=%ld\n",GetLastError());
    if (hevent==NULL)
        return;

    wout=NULL;
    rc=waveOutOpen(&wout,device,pwfx,(DWORD)hevent,0,CALLBACK_EVENT|flags);
    /* Note: Win9x doesn't know WAVE_FORMAT_DIRECT */
    /* It is acceptable to fail on formats that are not specified to work */
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_BADDEVICEID ||
       rc==MMSYSERR_NOTENABLED || rc==MMSYSERR_NODRIVER || rc==MMSYSERR_ALLOCATED ||
       ((rc==WAVERR_BADFORMAT || rc==MMSYSERR_NOTSUPPORTED) &&
       (flags & WAVE_FORMAT_DIRECT) && !(pcaps->dwFormats & format)) ||
       ((rc==WAVERR_BADFORMAT || rc==MMSYSERR_NOTSUPPORTED) &&
       (!(flags & WAVE_FORMAT_DIRECT) || (flags & WAVE_MAPPED)) && !(pcaps->dwFormats & format)) ||
       (rc==MMSYSERR_INVALFLAG && (flags & WAVE_FORMAT_DIRECT)),
       "waveOutOpen: device=%s format=%ldx%2dx%d flags=%lx(%s) rc=%s\n",dev_name(device),
       pwfx->nSamplesPerSec,pwfx->wBitsPerSample,pwfx->nChannels,CALLBACK_EVENT|flags,
       wave_open_flags(CALLBACK_EVENT|flags),wave_out_error(rc));
    if ((rc==WAVERR_BADFORMAT || rc==MMSYSERR_NOTSUPPORTED) &&
       (flags & WAVE_FORMAT_DIRECT) && (pcaps->dwFormats & format))
        trace(" Reason: The device lists this format as supported in it's capabilities but opening it failed.\n");
    if ((rc==WAVERR_BADFORMAT || rc==MMSYSERR_NOTSUPPORTED) &&
       !(pcaps->dwFormats & format))
        trace("waveOutOpen: device=%s format=%ldx%2dx%d %s rc=%s failed but format not supported so OK.\n",
              dev_name(device), pwfx->nSamplesPerSec,pwfx->wBitsPerSample,pwfx->nChannels,
              flags & WAVE_FORMAT_DIRECT ? "flags=WAVE_FORMAT_DIRECT" :
              flags & WAVE_MAPPED ? "flags=WAVE_MAPPED" : "", mmsys_error(rc));
    if (rc!=MMSYSERR_NOERROR) {
        CloseHandle(hevent);
        return;
    }

    ok(pwfx->nChannels==nChannels &&
       pwfx->wBitsPerSample==wBitsPerSample &&
       pwfx->nSamplesPerSec==nSamplesPerSec,
       "got the wrong format: %ldx%2dx%d instead of %ldx%2dx%d\n",
       pwfx->nSamplesPerSec, pwfx->wBitsPerSample,
       pwfx->nChannels, nSamplesPerSec, wBitsPerSample, nChannels);

    frag.lpData=wave_generate_la(pwfx,duration,&frag.dwBufferLength);
    frag.dwFlags=0;
    frag.dwLoops=0;

    rc=waveOutGetVolume(wout,&volume);
    ok(rc==MMSYSERR_NOERROR,"waveOutGetVolume: device=%s rc=%s\n",dev_name(device),wave_out_error(rc));

    rc=waveOutPrepareHeader(wout, &frag, sizeof(frag));
    ok(rc==MMSYSERR_NOERROR,
       "waveOutPrepareHeader: device=%s rc=%s\n",dev_name(device),wave_out_error(rc));

    if (winetest_interactive && rc==MMSYSERR_NOERROR) {
        DWORD start,end;
        trace("Playing %g second 440Hz tone at %5ldx%2dx%d %s\n",duration,
              pwfx->nSamplesPerSec, pwfx->wBitsPerSample,pwfx->nChannels,
              flags & WAVE_FORMAT_DIRECT ? "WAVE_FORMAT_DIRECT" :
              flags & WAVE_MAPPED ? "WAVE_MAPPED" : "");

        /* Check that the position is 0 at start */
        check_position(device, wout, 0.0, pwfx);

        rc=waveOutSetVolume(wout,0x20002000);
        ok(rc==MMSYSERR_NOERROR,"waveOutSetVolume: device=%s rc=%s\n",dev_name(device),wave_out_error(rc));
        WaitForSingleObject(hevent,INFINITE);

        start=GetTickCount();
        rc=waveOutWrite(wout, &frag, sizeof(frag));
        ok(rc==MMSYSERR_NOERROR,"waveOutWrite: device=%s rc=%s\n",dev_name(device),wave_out_error(rc));
        WaitForSingleObject(hevent,INFINITE);

        /* Check the sound duration was within 10% of the expected value */
        end=GetTickCount();
        trace("sound duration=%ld\n",end-start);
        ok(fabs(1000*duration-end+start)<=100*duration,"The sound played for %ld ms instead of %g ms\n",end-start,1000*duration);

        rc=waveOutSetVolume(wout,volume);
        ok(rc==MMSYSERR_NOERROR,"waveOutSetVolume: device=%s rc=%s\n",dev_name(device),wave_out_error(rc));

        check_position(device, wout, frag.dwBufferLength, pwfx);
    }

    rc=waveOutUnprepareHeader(wout, &frag, sizeof(frag));
    ok(rc==MMSYSERR_NOERROR,
       "waveOutUnprepareHeader: device=%s rc=%s\n",dev_name(device),wave_out_error(rc));
    free(frag.lpData);

    CloseHandle(hevent);
    rc=waveOutClose(wout);
    ok(rc==MMSYSERR_NOERROR,"waveOutClose: device=%s rc=%s\n",dev_name(device),wave_out_error(rc));
}

static void wave_out_test_device(int device)
{
    WAVEOUTCAPS caps;
    WAVEFORMATEX format, oformat;
    HWAVEOUT wout;
    MMRESULT rc;
    UINT f;
    WCHAR * wname;
    CHAR * name;
    DWORD size;
    DWORD dwPageSize;
    BYTE * twoPages;
    SYSTEM_INFO sSysInfo;
    DWORD flOldProtect;
    BOOL res;

    GetSystemInfo(&sSysInfo);
    dwPageSize = sSysInfo.dwPageSize;

    rc=waveOutGetDevCapsA(device,&caps,sizeof(caps));
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_BADDEVICEID || rc==MMSYSERR_NODRIVER,
       "waveOutGetDevCapsA: failed to get capabilities of device %s: rc=%s\n",dev_name(device),wave_out_error(rc));
    if (rc==MMSYSERR_BADDEVICEID || rc==MMSYSERR_NODRIVER)
        return;

    rc=waveOutGetDevCapsA(device,0,sizeof(caps));
    ok(rc==MMSYSERR_INVALPARAM,
       "waveOutGetDevCapsA: MMSYSERR_INVALPARAM expected, got %s\n",wave_out_error(rc));

#if 0 /* FIXME: this works on windows but crashes wine */
    rc=waveOutGetDevCapsA(device,(LPWAVEOUTCAPS)1,sizeof(caps));
    ok(rc==MMSYSERR_INVALPARAM,
       "waveOutGetDevCapsA: MMSYSERR_INVALPARAM expected, got %s\n",wave_out_error(rc));
#endif

    rc=waveOutGetDevCapsA(device,&caps,4);
    ok(rc==MMSYSERR_NOERROR,
       "waveOutGetDevCapsA: MMSYSERR_NOERROR expected, got %s\n",wave_out_error(rc));

    name=NULL;
    rc=waveOutMessage((HWAVEOUT)device, DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)&size, 0);
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_INVALPARAM || rc==MMSYSERR_NOTSUPPORTED,
       "waveOutMessage: failed to get interface size for device: %s rc=%s\n",dev_name(device),wave_out_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        wname = (WCHAR *)malloc(size);
        rc=waveOutMessage((HWAVEOUT)device, DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)wname, size);
        ok(rc==MMSYSERR_NOERROR,"waveOutMessage: failed to get interface name for device: %s rc=%s\n",dev_name(device),wave_out_error(rc));
        ok(lstrlenW(wname)+1==size/sizeof(WCHAR),"got an incorrect size: %ld instead of %d\n",size,(lstrlenW(wname)+1)*sizeof(WCHAR));
        if (rc==MMSYSERR_NOERROR) {
            name = malloc(size/sizeof(WCHAR));
            WideCharToMultiByte(CP_ACP, 0, wname, size/sizeof(WCHAR), name, size/sizeof(WCHAR), NULL, NULL);
        }
        free(wname);
    }
    else if (rc==MMSYSERR_NOTSUPPORTED) {
        name=strdup("not supported");
    }

    trace("  %s: \"%s\" (%s) %d.%d (%d:%d): channels=%d formats=%05lx support=%04lx(%s)\n",
          dev_name(device),caps.szPname,(name?name:"failed"),caps.vDriverVersion >> 8,
          caps.vDriverVersion & 0xff,
          caps.wMid,caps.wPid,
          caps.wChannels,caps.dwFormats,caps.dwSupport,wave_out_caps(caps.dwSupport));
    free(name);

    if (winetest_interactive && (device != WAVE_MAPPER))
    {
        trace("Playing a 5 seconds reference tone.\n");
        trace("All subsequent tones should be identical to this one.\n");
        trace("Listen for stutter, changes in pitch, volume, etc.\n");
        format.wFormatTag=WAVE_FORMAT_PCM;
        format.nChannels=1;
        format.wBitsPerSample=8;
        format.nSamplesPerSec=22050;
        format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
        format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
        format.cbSize=0;
        wave_out_test_deviceOut(device,5.0,&format,WAVE_FORMAT_2M08,0,&caps);
    }

    for (f=0;f<NB_WIN_FORMATS;f++) {
        format.wFormatTag=WAVE_FORMAT_PCM;
        format.nChannels=win_formats[f][3];
        format.wBitsPerSample=win_formats[f][2];
        format.nSamplesPerSec=win_formats[f][1];
        format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
        format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
        format.cbSize=0;
        wave_out_test_deviceOut(device,1.0,&format,win_formats[f][0],0,&caps);
        wave_out_test_deviceOut(device,1.0,&format,win_formats[f][0],WAVE_FORMAT_DIRECT,&caps);
        if (device != WAVE_MAPPER)
            wave_out_test_deviceOut(device,1.0,&format,win_formats[f][0],WAVE_MAPPED,&caps);
    }

    /* Try a PCMWAVEFORMAT aligned next to an unaccessable page for bounds checking */
    twoPages = VirtualAlloc(NULL, 2 * dwPageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    ok(twoPages!=NULL,"Failed to allocate 2 pages of memory\n");
    if (twoPages) {
        res = VirtualProtect(twoPages + dwPageSize, dwPageSize, PAGE_NOACCESS, &flOldProtect);
        ok(res, "Failed to set memory access on second page\n");
        if (res) {
            LPWAVEFORMATEX pwfx = (LPWAVEFORMATEX)(twoPages + dwPageSize - sizeof(PCMWAVEFORMAT));
            pwfx->wFormatTag=WAVE_FORMAT_PCM;
            pwfx->nChannels=1;
            pwfx->wBitsPerSample=8;
            pwfx->nSamplesPerSec=22050;
            pwfx->nBlockAlign=pwfx->nChannels*pwfx->wBitsPerSample/8;
            pwfx->nAvgBytesPerSec=pwfx->nSamplesPerSec*pwfx->nBlockAlign;
            wave_out_test_deviceOut(device,1.0,pwfx,WAVE_FORMAT_2M08,0,&caps);
            wave_out_test_deviceOut(device,1.0,pwfx,WAVE_FORMAT_2M08,WAVE_FORMAT_DIRECT,&caps);
            if (device != WAVE_MAPPER)
                wave_out_test_deviceOut(device,1.0,pwfx,WAVE_FORMAT_2M08,WAVE_MAPPED,&caps);
        }
        VirtualFree(twoPages, 2 * dwPageSize, MEM_RELEASE);
    }

    /* Try invalid formats to test error handling */
    trace("Testing invalid format: 11 bits per sample\n");
    format.wFormatTag=WAVE_FORMAT_PCM;
    format.nChannels=2;
    format.wBitsPerSample=11;
    format.nSamplesPerSec=22050;
    format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
    format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
    format.cbSize=0;
    oformat=format;
    rc=waveOutOpen(&wout,device,&format,0,0,CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==WAVERR_BADFORMAT || rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM,
       "waveOutOpen: opening the device in 11 bits mode should fail %s: rc=%s\n",dev_name(device),wave_out_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        trace("     got %ldx%2dx%d for %ldx%2dx%d\n",
              format.nSamplesPerSec, format.wBitsPerSample,
              format.nChannels,
              oformat.nSamplesPerSec, oformat.wBitsPerSample,
              oformat.nChannels);
        waveOutClose(wout);
    }

    trace("Testing invalid format: 2 MHz sample rate\n");
    format.wFormatTag=WAVE_FORMAT_PCM;
    format.nChannels=2;
    format.wBitsPerSample=16;
    format.nSamplesPerSec=2000000;
    format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
    format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
    format.cbSize=0;
    oformat=format;
    rc=waveOutOpen(&wout,device,&format,0,0,CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==WAVERR_BADFORMAT || rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM,
       "waveOutOpen: opening the device at 2 MHz sample rate should fail %s: rc=%s\n",dev_name(device),wave_out_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        trace("     got %ldx%2dx%d for %ldx%2dx%d\n",
              format.nSamplesPerSec, format.wBitsPerSample,
              format.nChannels,
              oformat.nSamplesPerSec, oformat.wBitsPerSample,
              oformat.nChannels);
        waveOutClose(wout);
    }
}

static void wave_out_tests()
{
    WAVEOUTCAPS caps;
    WAVEFORMATEX format;
    HWAVEOUT wout;
    MMRESULT rc;
    UINT ndev,d;

    ndev=waveOutGetNumDevs();
    trace("found %d WaveOut devices\n",ndev);

    rc=waveOutGetDevCapsA(ndev+1,&caps,sizeof(caps));
    ok(rc==MMSYSERR_BADDEVICEID,
       "waveOutGetDevCapsA: MMSYSERR_BADDEVICEID expected, got %s\n",mmsys_error(rc));

    rc=waveOutGetDevCapsA(WAVE_MAPPER,&caps,sizeof(caps));
    if (ndev>0)
        ok(rc==MMSYSERR_NOERROR,
           "waveOutGetDevCapsA: MMSYSERR_NOERROR expected, got %s\n",mmsys_error(rc));
    else
        ok(rc==MMSYSERR_BADDEVICEID || rc==MMSYSERR_NODRIVER,
           "waveOutGetDevCapsA: MMSYSERR_BADDEVICEID or MMSYSERR_NODRIVER expected, got %s\n",mmsys_error(rc));

    format.wFormatTag=WAVE_FORMAT_PCM;
    format.nChannels=2;
    format.wBitsPerSample=16;
    format.nSamplesPerSec=44100;
    format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
    format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
    format.cbSize=0;
    rc=waveOutOpen(&wout,ndev+1,&format,0,0,CALLBACK_NULL);
    ok(rc==MMSYSERR_BADDEVICEID,
       "waveOutOpen: MMSYSERR_BADDEVICEID expected, got %s\n",mmsys_error(rc));

    for (d=0;d<ndev;d++)
        wave_out_test_device(d);

    if (ndev>0)
        wave_out_test_device(WAVE_MAPPER);
}

START_TEST(wave)
{
    wave_out_tests();
}
