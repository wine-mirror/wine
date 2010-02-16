/*
 * Test winmm midi
 *
 * Copyright 2010 Jörg Höhle
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

#include <stdio.h>
#include "windows.h"
#include "mmsystem.h"
#include "wine/test.h"

extern const char* mmsys_error(MMRESULT error); /* from wave.c */

/* Test in order of increasing probability to hang.
 * On many UNIX systems, the Timidity softsynth provides
 * MIDI sequencer services and it is not particularly robust.
 */

#define MYCBINST 0x4CAFE5A8 /* not used with window or thread callbacks */
#define WHATEVER 0xFEEDF00D

static BOOL spurious_message(LPMSG msg)
{
  /* WM_DEVICECHANGE 0x0219 appears randomly */
  if(msg->message == WM_DEVICECHANGE) {
    trace("skipping spurious message %04x\n", msg->message);
    return TRUE;
  }
  return FALSE;
}

static UINT      cbmsg  = 0;
static DWORD_PTR cbval1 = WHATEVER;
static DWORD_PTR cbval2 = 0;
static DWORD_PTR cbinst = MYCBINST;

static void CALLBACK callback_func(HWAVEOUT hwo, UINT uMsg,
                                   DWORD_PTR dwInstance,
                                   DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    if (winetest_debug>1)
        trace("Callback! msg=%x %lx %lx\n", uMsg, dwParam1, dwParam2);
    cbmsg = uMsg;
    cbval1 = dwParam1;   /* mhdr or 0 */
    cbval2 = dwParam2;   /* always 0 */
    cbinst = dwInstance; /* MYCBINST, see midiOut/StreamOpen */
}

static void test_notification(HWND hwnd, const char* command, UINT m1, DWORD_PTR p2)
{   /* Use message type 0 as meaning no message */
    MSG msg;
    if (hwnd) {
        /* msg.wParam is hmidiout, msg.lParam is the mhdr (or 0) */
        BOOL seen;
        do { seen = PeekMessageA(&msg, hwnd, 0, 0, PM_REMOVE); }
        while(seen && spurious_message(&msg));
        if (seen) {
            trace("Message %x, wParam=%lx, lParam=%lx from %s\n",
                  msg.message, msg.wParam, msg.lParam, command);
            ok(msg.hwnd==hwnd, "Didn't get the handle to our test window\n");
            ok(msg.message==m1 && msg.lParam==p2, "bad message %x/%lx from %s, expect %x/%lx\n", msg.message, msg.lParam, command, m1, p2);
        }
        else ok(m1==0, "Expect message %x from %s\n", m1, command);
    }
    else {
        /* FIXME: MOM_POSITIONCB and MOM_DONE are so close that a queue is needed. */
        if (cbmsg) {
            ok(cbmsg==m1 && cbval1==p2 && cbval2==0, "bad callback %x/%lx/%lx from %s, expect %x/%lx\n", cbmsg, cbval1, cbval2, command, m1, p2);
            cbmsg = 0; /* Mark as read */
            cbval1 = cbval2 = WHATEVER;
            ok(cbinst==MYCBINST, "callback dwInstance changed to %lx\n", cbinst);
        }
        else ok(m1==0, "Expect callback %x from %s\n", m1, command);
    }
}


static void test_midiIn_device(UINT udev, HWND hwnd)
{
    HMIDIIN hm;
    MMRESULT rc;
    MIDIINCAPSA capsA;
    MIDIHDR mhdr;

    rc = midiInGetDevCapsA(udev, &capsA, sizeof(capsA));
    ok((MIDIMAPPER==udev) ? (rc==MMSYSERR_BADDEVICEID || broken(rc==MMSYSERR_NODRIVER /*nt,w2k*/)) : rc==0,
       "midiInGetDevCaps(dev=%d) rc=%s\n", udev, mmsys_error(rc));
    if (!rc) {
        /* MIDI IN capsA.dwSupport may contain garbage, absent in old MS-Windows */
        trace("* %s: manufacturer=%d, product=%d, support=%X\n", capsA.szPname, capsA.wMid, capsA.wPid, capsA.dwSupport);
    }

    if (hwnd)
        rc = midiInOpen(&hm, udev, (DWORD_PTR)hwnd, (DWORD_PTR)MYCBINST, CALLBACK_WINDOW);
    else
        rc = midiInOpen(&hm, udev, (DWORD_PTR)callback_func, (DWORD_PTR)MYCBINST, CALLBACK_FUNCTION);
    ok((MIDIMAPPER!=udev) ? rc==0 : (rc==MMSYSERR_BADDEVICEID || broken(rc==MMSYSERR_NODRIVER /*nt,w2k*/)),
       "midiInOpen(dev=%d) rc=%s\n", udev, mmsys_error(rc));
    if (rc) return;

    test_notification(hwnd, "midiInOpen", MIM_OPEN, 0);

    mhdr.dwFlags = 0;
    mhdr.dwUser = 0x56FA552C;
    mhdr.dwBufferLength = 70000; /* > 64KB! */
    mhdr.lpData = HeapAlloc(GetProcessHeap(), 0 , mhdr.dwBufferLength);
    ok(mhdr.lpData!=NULL, "No %d bytes of memory!\n", mhdr.dwBufferLength);
    if (mhdr.lpData) {
        rc = midiInPrepareHeader(hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiInPrepare rc=%s\n", mmsys_error(rc));
        rc = midiInUnprepareHeader(hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiInUnprepare rc=%s\n", mmsys_error(rc));
        trace("MIDIHDR flags=%x when unsent\n", mhdr.dwFlags);

        HeapFree(GetProcessHeap(), 0, mhdr.lpData);
    }
    ok(mhdr.dwUser==0x56FA552C, "MIDIHDR.dwUser changed to %lx\n", mhdr.dwUser);

    rc = midiInReset(hm); /* Return any pending buffer */
    ok(!rc, "midiInReset rc=%s\n", mmsys_error(rc));

    rc = midiInClose(hm);
    ok(!rc, "midiInClose rc=%s\n", mmsys_error(rc));
    test_notification(hwnd, "midiInClose", MIM_CLOSE, 0);
    test_notification(hwnd, "midiIn over", 0, WHATEVER);
}

static void test_midi_infns(HWND hwnd)
{
    HMIDIIN hm;
    MMRESULT rc;
    UINT udev, ndevs = midiInGetNumDevs();

    rc = midiInOpen(&hm, ndevs, 0, 0, CALLBACK_NULL);
    ok(rc==MMSYSERR_BADDEVICEID, "midiInOpen udev>max rc=%s\n", mmsys_error(rc));
    if (!rc) {
        rc = midiInClose(hm);
        ok(!rc, "midiInClose rc=%s\n", mmsys_error(rc));
    }
    if (!ndevs) {
        trace("Found no MIDI IN device\n"); /* no skip for this common situation */
        rc = midiInOpen(&hm, MIDIMAPPER, 0, 0, CALLBACK_NULL);
        ok(rc==MMSYSERR_BADDEVICEID || broken(rc==MMSYSERR_NODRIVER /*nt,w2k*/), "midiInOpen MAPPER with no MIDI rc=%s\n", mmsys_error(rc));
        if (!rc) {
            rc = midiInClose(hm);
            ok(!rc, "midiInClose rc=%s\n", mmsys_error(rc));
        }
        return;
    }
    trace("Found %d MIDI IN devices\n", ndevs);
    for (udev=0; udev < ndevs; udev++) {
        trace("** Testing device %d\n", udev);
        test_midiIn_device(udev, hwnd);
        Sleep(50);
    }
    trace("** Testing MIDI mapper\n");
    test_midiIn_device(MIDIMAPPER, hwnd);
}


static void test_midi_mci(HWND hwnd)
{
    MCIERROR err;
    char buf[1024];
    memset(buf, 0, sizeof(buf));

    err = mciSendString("sysinfo sequencer quantity", buf, sizeof(buf), hwnd);
    ok(!err, "mci sysinfo sequencer quantity returned %d\n", err);
    if (!err) trace("Found %s MCI sequencer devices\n", buf);
}


static void test_midiOut_device(UINT udev, HWND hwnd)
{
    HMIDIOUT hm;
    MMRESULT rc;
    MIDIOUTCAPSA capsA;
    DWORD ovolume;
    MIDIHDR mhdr;

    rc = midiOutGetDevCapsA(udev, &capsA, sizeof(capsA));
    ok(!rc, "midiOutGetDevCaps(dev=%d) rc=%s\n", udev, mmsys_error(rc));
    if (!rc) {
      trace("* %s: manufacturer=%d, product=%d, tech=%d, support=%X: %d voices, %d notes\n",
            capsA.szPname, capsA.wMid, capsA.wPid, capsA.wTechnology, capsA.dwSupport, capsA.wVoices, capsA.wNotes);
    }

    if (hwnd)
        rc = midiOutOpen(&hm, udev, (DWORD_PTR)hwnd, (DWORD_PTR)MYCBINST, CALLBACK_WINDOW);
    else
        rc = midiOutOpen(&hm, udev, (DWORD_PTR)callback_func, (DWORD_PTR)MYCBINST, CALLBACK_FUNCTION);
    ok(!rc, "midiOutOpen(dev=%d) rc=%s\n", udev, mmsys_error(rc));
    if (rc) return;

    test_notification(hwnd, "midiOutOpen", MOM_OPEN, 0);

    rc = midiOutGetVolume(hm, &ovolume);
    ok(rc==((capsA.dwSupport & MIDICAPS_VOLUME) ? MMSYSERR_NOERROR : MMSYSERR_NOTSUPPORTED), "midiOutGetVolume rc=%s\n", mmsys_error(rc));
    /* The native mapper responds with FFFFFFFF initially,
     * real devices with the volume GUI SW-synth settings. */
    if (!rc) trace("Current volume %x on device %d\n", ovolume, udev);

    /* Tests with midiOutSetvolume show that the midi mapper forwards
     * the value to the real device, but Get initially always reports
     * FFFFFFFF.  Therefore, a Get+SetVolume pair with the mapper is
     * not adequate to restore the value prior to tests.
     */
    if (winetest_interactive && (capsA.dwSupport & MIDICAPS_VOLUME)) {
        DWORD volume2 = (ovolume < 0x80000000) ? 0xC000C000 : 0x40004000;
        rc = midiOutSetVolume(hm, volume2);
        ok(!rc, "midiOutSetVolume rc=%s\n", mmsys_error(rc));
        if (!rc) {
            DWORD volume3;
            rc = midiOutGetVolume(hm, &volume3);
            ok(!rc, "midiOutGetVolume new rc=%s\n", mmsys_error(rc));
            if (!rc) trace("New volume %x on device %d\n", volume3, udev);
            todo_wine ok(volume2==volume3, "volume Set %x = Get %x\n", volume2, volume3);

            rc = midiOutSetVolume(hm, ovolume);
            ok(!rc, "midiOutSetVolume restore rc=%s\n", mmsys_error(rc));
        }
    }
    rc = midiOutGetDevCapsA((UINT_PTR)hm, &capsA, sizeof(capsA));
    ok(!rc, "midiOutGetDevCaps(dev=%d) by handle rc=%s\n", udev, mmsys_error(rc));
    rc = midiInGetDevCapsA((UINT_PTR)hm, (LPMIDIINCAPSA)&capsA, sizeof(DWORD));
    ok(rc==MMSYSERR_BADDEVICEID, "midiInGetDevCaps(dev=%d) by out handle rc=%s\n", udev, mmsys_error(rc));

    {   DWORD e = 0x006F4893; /* velocity, note (#69 would be 440Hz) channel */
        trace("ShortMsg type %x\n", LOBYTE(LOWORD(e)));
        rc = midiOutShortMsg(hm, e);
        ok(!rc, "midiOutShortMsg rc=%s\n", mmsys_error(rc));
        if (!rc) Sleep(400); /* Hear note */
    }

    mhdr.dwFlags = 0;
    mhdr.dwUser = 0x56FA552C;
    mhdr.dwBufferLength = 70000; /* > 64KB! */
    mhdr.lpData = HeapAlloc(GetProcessHeap(), 0 , mhdr.dwBufferLength);
    ok(mhdr.lpData!=NULL, "No %d bytes of memory!\n", mhdr.dwBufferLength);
    if (mhdr.lpData) {
        rc = midiOutLongMsg(hm, &mhdr, sizeof(mhdr));
        ok(rc==MIDIERR_UNPREPARED, "midiOutLongMsg unprepared rc=%s\n", mmsys_error(rc));
        test_notification(hwnd, "midiOutLong unprepared", 0, WHATEVER);

        rc = midiOutPrepareHeader(hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiOutPrepare rc=%s\n", mmsys_error(rc));
        rc = midiOutUnprepareHeader(hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiOutUnprepare rc=%s\n", mmsys_error(rc));
        trace("MIDIHDR flags=%x when unsent\n", mhdr.dwFlags);

        HeapFree(GetProcessHeap(), 0, mhdr.lpData);
    }
    ok(mhdr.dwUser==0x56FA552C, "MIDIHDR.dwUser changed to %lx\n", mhdr.dwUser);

    rc = midiOutReset(hm); /* Quiet everything */
    ok(!rc, "midiOutReset rc=%s\n", mmsys_error(rc));

    rc = midiOutClose(hm);
    ok(!rc, "midiOutClose rc=%s\n", mmsys_error(rc));
    test_notification(hwnd, "midiOutClose", MOM_CLOSE, 0);
    test_notification(hwnd, "midiOut over", 0, WHATEVER);
}

static void test_position(HMIDISTRM hm, UINT typein, UINT typeout)
{
    MMRESULT rc;
    MMTIME mmtime;
    mmtime.wType = typein;
    rc = midiStreamPosition(hm, &mmtime, sizeof(MMTIME));
    /* Ugly, but a single ok() herein enables using the todo_wine prefix */
    ok(!rc && (mmtime.wType == typeout), "midiStreamPosition type %x converted to %x rc=%s\n", typein, mmtime.wType, mmsys_error(rc));
    if (!rc) switch(mmtime.wType) {
    case TIME_MS:
        trace("Stream position %ums\n", mmtime.u.ms);
        break;
    case TIME_TICKS:
        trace("Stream position %u ticks\n", mmtime.u.ticks);
        break;
    case TIME_MIDI:
        trace("Stream position song pointer %u\n", mmtime.u.midi.songptrpos);
        break;
    }
}

static BYTE strmEvents[] = { /* A set of variable-sized MIDIEVENT structs */
    0, 0, 0, 0,  0, 0, 0, 0, /* dwDeltaTime and dwStreamID */
    0, 0, 0, MEVT_NOP | 0x40, /* with MEVT_F_CALLBACK */
    0, 0, 0, 0,  0, 0, 0, 0, /* dwDeltaTime and dwStreamID */
    0x93, 0x48, 0x6F, MEVT_SHORTMSG,
};

static void test_midiStream(UINT udev, HWND hwnd)
{
    HMIDISTRM hm;
    MMRESULT rc, rc2;
    MIDIHDR mhdr;

    if (hwnd)
        rc = midiStreamOpen(&hm, &udev, 1, (DWORD_PTR)hwnd, (DWORD_PTR)MYCBINST, CALLBACK_WINDOW);
    else
        rc = midiStreamOpen(&hm, &udev, 1, (DWORD_PTR)callback_func, (DWORD_PTR)MYCBINST, CALLBACK_FUNCTION);
    ok(!rc, "midiStreamOpen(dev=%d) rc=%s\n", udev, mmsys_error(rc));
    if (rc) return;

    test_notification(hwnd, "midiStreamOpen", MOM_OPEN, 0);

    mhdr.dwFlags = 0;
    mhdr.dwUser = 0x56FA552C;
    mhdr.dwBufferLength = sizeof(strmEvents) * sizeof(strmEvents[0]);
    mhdr.dwBytesRecorded = mhdr.dwBufferLength; /* unused? */
    mhdr.lpData = (LPSTR)&strmEvents[0];
    if (mhdr.lpData) {
        rc = midiOutLongMsg((HMIDIOUT)hm, &mhdr, sizeof(mhdr));
        ok(rc==MIDIERR_UNPREPARED, "midiOutLongMsg unprepared rc=%s\n", mmsys_error(rc));
        test_notification(hwnd, "midiOutLong unprepared", 0, WHATEVER);

        rc = midiOutPrepareHeader((HMIDIOUT)hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiOutPrepare rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags & MHDR_PREPARED, "MHDR.dwFlags when prepared %x\n", mhdr.dwFlags);

        /* The device is still in paused mode and should queue the message. */
        rc = midiStreamOut(hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiStreamOut rc=%s\n", mmsys_error(rc));
        rc2 = rc;
        trace("MIDIHDR flags=%x when submitted\n", mhdr.dwFlags);
        /* w9X/me does not set MHDR_ISSTRM when StreamOut exits,
         * but it will be set on all systems after the job is finished. */

        Sleep(90);
        /* Wine <1.1.39 started playing immediately */
        test_notification(hwnd, "midiStream still paused", 0, WHATEVER);

    /* MSDN asks to use midiStreamRestart prior to midiStreamOut()
     * because the starting state is 'pause', but some apps seem to
     * work with the inverse order.
     */

        rc = midiStreamRestart(hm);
        ok(!rc, "midiStreamRestart rc=%s\n", mmsys_error(rc));

        if (!rc2) while(mhdr.dwFlags & MHDR_INQUEUE) {
            trace("async MIDI still queued\n");
            Sleep(100);
        } /* Checking INQUEUE is not the recommended way to wait for the end of a job, but we're testing. */
        /* MHDR_ISSTRM is not necessarily set when midiStreamOut returns
         * rather than when the queue is eventually processed. */
        ok(mhdr.dwFlags & MHDR_ISSTRM, "MHDR.dwFlags %x no ISSTRM when out of queue\n", mhdr.dwFlags);
        if (!rc2) while(!(mhdr.dwFlags & MHDR_DONE)) {
            /* Never to be seen except perhaps on multicore */
            trace("async MIDI still not done\n");
            Sleep(100);
        }
        ok(mhdr.dwFlags & MHDR_DONE, "MHDR.dwFlags %x not DONE when out of queue\n", mhdr.dwFlags);
        test_notification(hwnd, "midiStream callback", MOM_POSITIONCB, (DWORD_PTR)&mhdr);
        test_notification(hwnd, "midiStreamOut", MOM_DONE, (DWORD_PTR)&mhdr);

        rc = midiOutUnprepareHeader((HMIDIOUT)hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiOutUnprepare rc=%s\n", mmsys_error(rc));
        rc = midiOutUnprepareHeader((HMIDIOUT)hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiOutUnprepare #2 rc=%s\n", mmsys_error(rc));

        trace("MIDIHDR stream flags=%x when finished\n", mhdr.dwFlags);
        ok(mhdr.dwFlags & MHDR_DONE, "MHDR.dwFlags when done %x\n", mhdr.dwFlags);

        test_position(hm, TIME_MS,      TIME_MS);
        test_position(hm, TIME_TICKS,   TIME_TICKS);
        todo_wine test_position(hm, TIME_MIDI,    TIME_MIDI);
        test_position(hm, TIME_SMPTE,   TIME_MS);
        test_position(hm, TIME_SAMPLES, TIME_MS);
        test_position(hm, TIME_BYTES,   TIME_MS);

        Sleep(400); /* Hear note */

        rc = midiStreamRestart(hm);
        ok(!rc, "midiStreamRestart #2 rc=%s\n", mmsys_error(rc));

        mhdr.dwFlags |= MHDR_ISSTRM; /* just in case */
        /* Preset flags (e.g. MHDR_ISSTRM) do not disturb. */
        rc = midiOutPrepareHeader((HMIDIOUT)hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiOutPrepare used flags %x rc=%s\n", mhdr.dwFlags, mmsys_error(rc));
        rc = midiOutUnprepareHeader((HMIDIOUT)hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiOutUnprepare used flags %x rc=%s\n", mhdr.dwFlags, mmsys_error(rc));

        rc = midiStreamRestart(hm);
        ok(!rc, "midiStreamRestart #3 rc=%s\n", mmsys_error(rc));
    }
    ok(mhdr.dwUser==0x56FA552C, "MIDIHDR.dwUser changed to %lx\n", mhdr.dwUser);

    rc = midiStreamStop(hm);
    ok(!rc, "midiStreamStop rc=%s\n", mmsys_error(rc));

    mhdr.dwBufferLength = 70000; /* > 64KB! */
    mhdr.lpData = HeapAlloc(GetProcessHeap(), 0 , mhdr.dwBufferLength);
    ok(mhdr.lpData!=NULL, "No %d bytes of memory!\n", mhdr.dwBufferLength);
    if (mhdr.lpData) {
        mhdr.dwFlags = 0;
        /* PrepareHeader detects the too large buffer is for a stream. */
        rc = midiOutPrepareHeader((HMIDIOUT)hm, &mhdr, sizeof(mhdr));
        todo_wine ok(rc==MMSYSERR_INVALPARAM, "midiOutPrepare stream too large rc=%s\n", mmsys_error(rc));

        rc = midiOutUnprepareHeader((HMIDIOUT)hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiOutUnprepare rc=%s\n", mmsys_error(rc));

        HeapFree(GetProcessHeap(), 0, mhdr.lpData);
    }

    rc = midiStreamClose(hm);
    ok(!rc, "midiStreamClose rc=%s\n", mmsys_error(rc));
    test_notification(hwnd, "midiStreamClose", MOM_CLOSE, 0);
    test_notification(hwnd, "midiStream over", 0, WHATEVER);
}

static void test_midi_outfns(HWND hwnd)
{
    HMIDIOUT hm;
    MMRESULT rc;
    UINT udev, ndevs = midiOutGetNumDevs();

    rc = midiOutOpen(&hm, ndevs, 0, 0, CALLBACK_NULL);
    ok(rc==MMSYSERR_BADDEVICEID, "midiOutOpen udev>max rc=%s\n", mmsys_error(rc));
    if (!rc) {
        rc = midiOutClose(hm);
        ok(!rc, "midiOutClose rc=%s\n", mmsys_error(rc));
    }
    if (!ndevs) {
        skip("Found no MIDI out device\n");
        rc = midiOutOpen(&hm, MIDIMAPPER, 0, 0, CALLBACK_NULL);
        if (rc==MIDIERR_INVALIDSETUP) todo_wine /* Wine without snd-seq */
        ok(rc==MMSYSERR_BADDEVICEID || broken(rc==MMSYSERR_NODRIVER /*w2k*/), "midiOutOpen MAPPER with no MIDI rc=%s\n", mmsys_error(rc));
        else
        ok(rc==MMSYSERR_BADDEVICEID || broken(rc==MMSYSERR_NODRIVER /*w2k sound disabled*/),
           "midiOutOpen MAPPER with no MIDI rc=%s\n", mmsys_error(rc));
        if (!rc) {
            rc = midiOutClose(hm);
            ok(!rc, "midiOutClose rc=%s\n", mmsys_error(rc));
        }
        return;
    }
    trace("Found %d MIDI OUT devices\n", ndevs);

    test_midi_mci(hwnd);

    for (udev=0; udev < ndevs; udev++) {
        trace("** Testing device %d\n", udev);
        test_midiOut_device(udev, hwnd);
        Sleep(800); /* Let the synth rest */
        test_midiStream(udev, hwnd);
        Sleep(800);
    }
    trace("** Testing MIDI mapper\n");
    test_midiOut_device(MIDIMAPPER, hwnd);
    Sleep(800);
    test_midiStream(MIDIMAPPER, hwnd);
}

START_TEST(midi)
{
    HWND hwnd;
    if (1) /* select 1 for CALLBACK_WINDOW or 0 for CALLBACK_FUNCTION */
    hwnd = CreateWindowExA(0, "static", "winmm midi test", WS_POPUP, 0,0,100,100,
                           0, 0, 0, NULL);
    test_midi_infns(hwnd);
    test_midi_outfns(hwnd);
    if (hwnd) DestroyWindow(hwnd);
}
