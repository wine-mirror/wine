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

#include "wine/test.h"
#include "winbase.h"
#include <mmsystem.h>

/*
 * Note that in most of this test we may get MMSYSERR_BADDEVICEID errors
 * at about any time if the user starts another application that uses the
 * sound device. So we should not report these as test failures.
 */

#define TEST_FORMATS 12
static const unsigned int win_formats[TEST_FORMATS][4]={
    {WAVE_FORMAT_1M08, 11025,  8, 1},
    {WAVE_FORMAT_1S08, 11025,  8, 2},
    {WAVE_FORMAT_1M16, 11025, 16, 1},
    {WAVE_FORMAT_1S16, 11025, 16, 2},
    {WAVE_FORMAT_2M08, 22050,  8, 1},
    {WAVE_FORMAT_2S08, 22050,  8, 2},
    {WAVE_FORMAT_2M16, 22050, 16, 1},
    {WAVE_FORMAT_2S16, 22050, 16, 2},
    {WAVE_FORMAT_4M08, 44100,  8, 1},
    {WAVE_FORMAT_4S08, 44100,  8, 2},
    {WAVE_FORMAT_4M16, 44100, 16, 1},
    {WAVE_FORMAT_4S16, 44100, 16, 2}
};

void wave_out_tests()
{
    WAVEOUTCAPS caps;
    WAVEFORMATEX format;
    HWAVEOUT wout;
    MMRESULT rc;
    UINT ndev,d,f;
    int success;

    ndev=waveOutGetNumDevs();
    winetest_trace("found %d WaveOut devices\n",ndev);

    todo_wine {
        rc=waveOutGetDevCapsA(ndev+1,&caps,sizeof(caps));
        ok(rc==MMSYSERR_BADDEVICEID,
           "waveOutGetDevCa psA: MMSYSERR_BADDEVICEID expected, got %d",rc);
    }

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
        success=(rc==MMSYSERR_NOERROR || rc==MMSYSERR_BADDEVICEID);
        ok(success,"failed to get capabilities of device %d: rc=%d",d,rc);
        if (!success)
            continue;

        winetest_trace("  %d: \"%s\" %d.%d (%d:%d): channels=%d formats=%04lx support=%04lx\n",
                       d,caps.szPname,caps.vDriverVersion >> 8,
                       caps.vDriverVersion & 0xff,
                       caps.wMid,caps.wPid,
                       caps.wChannels,caps.dwFormats,caps.dwSupport);

        for (f=0;f<TEST_FORMATS;f++) {
            if (!(caps.dwFormats & win_formats[f][0]))
                continue;

            format.wFormatTag=WAVE_FORMAT_PCM;
            format.nChannels=win_formats[f][3];
            format.wBitsPerSample=win_formats[f][2];
            format.nSamplesPerSec=win_formats[f][1];
            format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
            format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
            format.cbSize=0;

            rc=waveOutOpen(&wout,d,&format,0,0,CALLBACK_NULL);
            success=(rc==MMSYSERR_NOERROR || rc==MMSYSERR_BADDEVICEID);
            ok(success, "failed to open device %d: rc=%d",d,rc);
            if (success) {
                ok(format.nChannels==win_formats[f][3] &&
                   format.wBitsPerSample==win_formats[f][2] &&
                   format.nSamplesPerSec==win_formats[f][1],
                   "got the wrong format: %ldx%2dx%d instead of %dx%2dx%d\n",
                   format.nSamplesPerSec, format.wBitsPerSample,
                   format.nChannels, win_formats[f][1], win_formats[f][2],
                   win_formats[f][3]);
            }
            if (rc==MMSYSERR_NOERROR)
                waveOutClose(wout);

            /* Try again with WAVE_FORMAT_DIRECT */
            rc=waveOutOpen(&wout,d,&format,0,0,CALLBACK_NULL|WAVE_FORMAT_DIRECT);
            success=(rc==MMSYSERR_NOERROR || rc==MMSYSERR_BADDEVICEID);
            ok(success, "failed to open device %d: rc=%d",d,rc);
            if (success) {
                ok(format.nChannels==win_formats[f][3] &&
                   format.wBitsPerSample==win_formats[f][2] &&
                   format.nSamplesPerSec==win_formats[f][1],
                   "got the wrong format: %ldx%2dx%d instead of %dx%2dx%d\n",
                   format.nSamplesPerSec, format.wBitsPerSample,
                   format.nChannels, win_formats[f][1], win_formats[f][2],
                   win_formats[f][3]);
            }
            if (rc==MMSYSERR_NOERROR)
                waveOutClose(wout);
        }

        /* Check an invalid format to test error handling */
        todo_wine {
        winetest_trace("Testing invalid 2MHz format\n");
        format.wFormatTag=WAVE_FORMAT_PCM;
        format.nChannels=2;
        format.wBitsPerSample=16;
        format.nSamplesPerSec=2000000; /* 2MHz! */
        format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
        format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
        format.cbSize=0;
        rc=waveOutOpen(&wout,d,&format,0,0,CALLBACK_NULL);
        success=(rc==WAVERR_BADFORMAT);
        ok(success, "opening the device at 2MHz should fail %d: rc=%d",d,rc);
        if (rc==MMSYSERR_NOERROR) {
            winetest_trace("     got %ldx%2dx%d for %dx%2dx%d\n",
                           format.nSamplesPerSec, format.wBitsPerSample,
                           format.nChannels,
                           win_formats[f][1], win_formats[f][2],
                           win_formats[f][3]);
            waveOutClose(wout);
        }
        }

        format.wFormatTag=WAVE_FORMAT_PCM;
        format.nChannels=2;
        format.wBitsPerSample=16;
        format.nSamplesPerSec=2000000; /* 2MHz! */
        format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
        format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
        format.cbSize=0;
        rc=waveOutOpen(&wout,d,&format,0,0,CALLBACK_NULL|WAVE_FORMAT_DIRECT);
        success=(rc==WAVERR_BADFORMAT);
        ok(success, "opening the device at 2MHz should fail %d: rc=%d",d,rc);
        if (rc==MMSYSERR_NOERROR) {
            winetest_trace("     got %ldx%2dx%d for %dx%2dx%d\n",
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
