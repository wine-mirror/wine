/*
 * Unit tests for winmm functions
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
#include "winbase.h"
#include "mmsystem.h"

/*
 * Note that in most of this test we may get MMSYSERR_BADDEVICEID errors
 * at about any time if the user starts another application that uses the
 * sound device. So we should not report these as test failures.
 *
 * This test can play a test tone. But this only makes sense if someone
 * is going to carefully listen to it, and would only bother everyone else.
 * So this is only done if the test is being run in interactive mode.
 */

static const unsigned int win_formats[][4]={
    {WAVE_FORMAT_1M08,  11025,  8, 1},
    {WAVE_FORMAT_1S08,  11025,  8, 2},
    {WAVE_FORMAT_1M16,  11025, 16, 1},
    {WAVE_FORMAT_1S16,  11025, 16, 2},
    {WAVE_FORMAT_2M08,  22050,  8, 1},
    {WAVE_FORMAT_2S08,  22050,  8, 2},
    {WAVE_FORMAT_2M16,  22050, 16, 1},
    {WAVE_FORMAT_2S16,  22050, 16, 2},
    {WAVE_FORMAT_4M08,  44100,  8, 1},
    {WAVE_FORMAT_4S08,  44100,  8, 2},
    {WAVE_FORMAT_4M16,  44100, 16, 1},
    {WAVE_FORMAT_4S16,  44100, 16, 2},
#ifdef WAVE_FORMAT_48M08
    {WAVE_FORMAT_48M08, 48000,  8, 1},
    {WAVE_FORMAT_48S08, 48000,  8, 2},
    {WAVE_FORMAT_48M16, 48000, 16, 1},
    {WAVE_FORMAT_48S16, 48000, 16, 2},
    {WAVE_FORMAT_96M08, 96000,  8, 1},
    {WAVE_FORMAT_96S08, 96000,  8, 2},
    {WAVE_FORMAT_96M16, 96000, 16, 1},
    {WAVE_FORMAT_96S16, 96000, 16, 2}
#endif
};
#define NB_WIN_FORMATS (sizeof(win_formats)/sizeof(*win_formats))

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

static void wave_out_test_deviceOut(int device, int format, DWORD flags)
{
    WAVEFORMATEX wfx;
    HWAVEOUT wout;
    HANDLE hevent;
    WAVEHDR frag;
    MMRESULT rc;
    DWORD volume;

    hevent=CreateEvent(NULL,FALSE,FALSE,NULL);
    ok(hevent!=NULL,"CreateEvent: error=%ld\n",GetLastError());
    if (hevent==NULL)
        return;

    wfx.wFormatTag=WAVE_FORMAT_PCM;
    wfx.nChannels=win_formats[format][3];
    wfx.wBitsPerSample=win_formats[format][2];
    wfx.nSamplesPerSec=win_formats[format][1];
    wfx.nBlockAlign=wfx.nChannels*wfx.wBitsPerSample/8;
    wfx.nAvgBytesPerSec=wfx.nSamplesPerSec*wfx.nBlockAlign;
    wfx.cbSize=0;

    wout=NULL;
    rc=waveOutOpen(&wout,device,&wfx,(DWORD)hevent,0,CALLBACK_EVENT|flags);
    /* Note: Win9x doesn't know WAVE_FORMAT_DIRECT */
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_BADDEVICEID ||
       (rc==MMSYSERR_INVALFLAG && (flags & WAVE_FORMAT_DIRECT)),
       "waveOutOpen: device=%d rc=%d",device,rc);
    if (rc!=MMSYSERR_NOERROR) {
        CloseHandle(hevent);
        return;
    }

    ok(wfx.nChannels==win_formats[format][3] &&
       wfx.wBitsPerSample==win_formats[format][2] &&
       wfx.nSamplesPerSec==win_formats[format][1],
       "got the wrong format: %ldx%2dx%d instead of %dx%2dx%d\n",
       wfx.nSamplesPerSec, wfx.wBitsPerSample,
       wfx.nChannels, win_formats[format][1], win_formats[format][2],
       win_formats[format][3]);

    frag.lpData=wave_generate_la(&wfx,1.0,&frag.dwBufferLength);
    frag.dwFlags=0;
    frag.dwLoops=0;

    rc=waveOutGetVolume(wout,&volume);
    ok(rc==MMSYSERR_NOERROR,"waveOutGetVolume: device=%d rc=%d\n",device,rc);

    rc=waveOutPrepareHeader(wout, &frag, sizeof(frag));
    ok(rc==MMSYSERR_NOERROR,
       "waveOutPrepareHeader: device=%d rc=%d\n",device,rc);

    if (winetest_interactive && rc==MMSYSERR_NOERROR) {
        trace("Playing 440Hz LA at %ldx%2dx%d %04lx\n",
              wfx.nSamplesPerSec, wfx.wBitsPerSample,wfx.nChannels,flags);
        rc=waveOutSetVolume(wout,0x20002000);
        ok(rc==MMSYSERR_NOERROR,"waveOutSetVolume: device=%d rc=%d\n",device,rc);
        WaitForSingleObject(hevent,INFINITE);

        rc=waveOutWrite(wout, &frag, sizeof(frag));
        ok(rc==MMSYSERR_NOERROR,"waveOutWrite: device=%d rc=%d\n",device,rc);
        WaitForSingleObject(hevent,INFINITE);

        rc=waveOutSetVolume(wout,volume);
        ok(rc==MMSYSERR_NOERROR,"waveOutSetVolume: device=%d rc=%d\n",device,rc);
    }

    rc=waveOutUnprepareHeader(wout, &frag, sizeof(frag));
    ok(rc==MMSYSERR_NOERROR,
       "waveOutUnprepareHeader: device=%d rc=%d\n",device,rc);
    free(frag.lpData);

    CloseHandle(hevent);
    waveOutClose(wout);
}

static void wave_out_tests()
{
    WAVEOUTCAPS caps;
    WAVEFORMATEX format;
    HWAVEOUT wout;
    MMRESULT rc;
    UINT ndev,d,f;

    ndev=waveOutGetNumDevs();
    trace("found %d WaveOut devices\n",ndev);

    rc=waveOutGetDevCapsA(ndev+1,&caps,sizeof(caps));
    ok(rc==MMSYSERR_BADDEVICEID,
       "waveOutGetDevCa psA: MMSYSERR_BADDEVICEID expected, got %d",rc);

    format.wFormatTag=WAVE_FORMAT_PCM;
    format.nChannels=2;
    format.wBitsPerSample=16;
    format.nSamplesPerSec=44100;
    format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
    format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
    format.cbSize=0;
    rc=waveOutOpen(&wout,ndev+1,&format,0,0,CALLBACK_NULL);
    ok(rc==MMSYSERR_BADDEVICEID,
       "waveOutOpen: MMSYSERR_BADDEVICEID expected, got %d",rc);

    for (d=0;d<ndev;d++) {
        rc=waveOutGetDevCapsA(d,&caps,sizeof(caps));
        ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_BADDEVICEID,
           "failed to get capabilities of device %d: rc=%d",d,rc);
        if (rc==MMSYSERR_BADDEVICEID)
            continue;

        trace("  %d: \"%s\" %d.%d (%d:%d): channels=%d formats=%04lx support=%04lx\n",
              d,caps.szPname,caps.vDriverVersion >> 8,
              caps.vDriverVersion & 0xff,
              caps.wMid,caps.wPid,
              caps.wChannels,caps.dwFormats,caps.dwSupport);

        for (f=0;f<NB_WIN_FORMATS;f++) {
            if (caps.dwFormats & win_formats[f][0]) {
                wave_out_test_deviceOut(d,f,0);
                wave_out_test_deviceOut(d,f,WAVE_FORMAT_DIRECT);
            }
        }

        /* Try an invalid format to test error handling */
        trace("Testing invalid 2MHz format\n");
        format.wFormatTag=WAVE_FORMAT_PCM;
        format.nChannels=2;
        format.wBitsPerSample=16;
        format.nSamplesPerSec=2000000; /* 2MHz! */
        format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
        format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
        format.cbSize=0;
        rc=waveOutOpen(&wout,d,&format,0,0,CALLBACK_NULL);
        ok(rc==WAVERR_BADFORMAT,
           "opening the device at 2MHz should fail %d: rc=%d",d,rc);
        if (rc==MMSYSERR_NOERROR) {
            trace("     got %ldx%2dx%d for %dx%2dx%d\n",
                  format.nSamplesPerSec, format.wBitsPerSample,
                  format.nChannels,
                  win_formats[f][1], win_formats[f][2],
                  win_formats[f][3]);
            waveOutClose(wout);
        }

        format.wFormatTag=WAVE_FORMAT_PCM;
        format.nChannels=2;
        format.wBitsPerSample=16;
        format.nSamplesPerSec=2000000; /* 2MHz! */
        format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
        format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
        format.cbSize=0;
        rc=waveOutOpen(&wout,d,&format,0,0,CALLBACK_NULL|WAVE_FORMAT_DIRECT);
        ok(rc==WAVERR_BADFORMAT || rc==MMSYSERR_INVALFLAG,
           "opening the device at 2MHz should fail %d: rc=%d",d,rc);
        if (rc==MMSYSERR_NOERROR) {
            trace("     got %ldx%2dx%d for %dx%2dx%d\n",
                  format.nSamplesPerSec, format.wBitsPerSample,
                  format.nChannels,
                  win_formats[f][1], win_formats[f][2],
                  win_formats[f][3]);
            waveOutClose(wout);
        }
    }
}

START_TEST(wave)
{
    wave_out_tests();
}
