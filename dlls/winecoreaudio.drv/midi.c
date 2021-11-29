/*
 * MIDI driver for macOS (PE-side)
 *
 * Copyright 1994       Martin Ayotte
 * Copyright 1998       Luiz Otavio L. Zorzella
 * Copyright 1998, 1999 Eric POUECH
 * Copyright 2005, 2006 Emmanuel Maillard
 * Copyright 2021       Huw Davies
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

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "mmddk.h"
#include "mmdeviceapi.h"
#include "audioclient.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/unixlib.h"
#include "coreaudio.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(midi);

#include <CoreAudio/CoreAudio.h>

#define WINE_DEFINITIONS
#include "coremidi.h"

static DWORD MIDIIn_NumDevs = 0;

static CRITICAL_SECTION midiInLock; /* Critical section for MIDI In */
static CFStringRef MIDIInThreadPortName;

static DWORD WINAPI MIDIIn_MessageThread(LPVOID p);

static MIDIPortRef MIDIInPort = NULL;

MIDISource *sources;

static void notify_client(struct notify_context *notify)
{
    TRACE("dev_id=%d msg=%d param1=%04lX param2=%04lX\n", notify->dev_id, notify->msg, notify->param_1, notify->param_2);

    DriverCallback(notify->callback, notify->flags, notify->device, notify->msg,
                   notify->instance, notify->param_1, notify->param_2);
}

static LONG CoreAudio_MIDIInit(void)
{
    struct midi_init_params params;
    DWORD err;

    params.err = &err;

    UNIX_CALL(midi_init, &params);
    if (err != DRV_SUCCESS)
    {
        ERR("can't create midi client\n");
        return err;
    }

    MIDIIn_NumDevs = params.num_srcs;
    sources = params.srcs;
    MIDIInPort = params.midi_in_port;

    if (MIDIIn_NumDevs > 0)
    {
        InitializeCriticalSection(&midiInLock);
        midiInLock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": midiInLock");

        MIDIInThreadPortName = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("MIDIInThreadPortName.%u"), getpid());
        CloseHandle( CreateThread(NULL, 0, MIDIIn_MessageThread, NULL, 0, NULL));
    }
    return err;
}

static LONG CoreAudio_MIDIRelease(void)
{
    TRACE("\n");

    UNIX_CALL(midi_release, NULL);
    sources = NULL;

    if (MIDIIn_NumDevs > 0)
    {
        midiInLock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&midiInLock);
    }

    return DRV_SUCCESS;
}


/**************************************************************************
 * 			MIDI_NotifyClient			[internal]
 */
static void MIDI_NotifyClient(UINT wDevID, WORD wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    DWORD_PTR 		dwCallBack;
    UINT 		uFlags;
    HANDLE		hDev;
    DWORD_PTR 		dwInstance;

    TRACE("wDevID=%d wMsg=%d dwParm1=%04lX dwParam2=%04lX\n", wDevID, wMsg, dwParam1, dwParam2);

    switch (wMsg) {
    case MIM_DATA:
    case MIM_LONGDATA:
    case MIM_ERROR:
    case MIM_LONGERROR:
    case MIM_MOREDATA:
        dwCallBack = sources[wDevID].midiDesc.dwCallback;
	uFlags = sources[wDevID].wFlags;
	hDev = sources[wDevID].midiDesc.hMidi;
	dwInstance = sources[wDevID].midiDesc.dwInstance;
        break;
    default:
	ERR("Unsupported MSW-MIDI message %u\n", wMsg);
	return;
    }

    DriverCallback(dwCallBack, uFlags, hDev, wMsg, dwInstance, dwParam1, dwParam2);
}

static DWORD MIDIIn_AddBuffer(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("wDevID=%d lpMidiHdr=%p dwSize=%d\n", wDevID, lpMidiHdr, dwSize);

    if (wDevID >= MIDIIn_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }
    if (lpMidiHdr == NULL) {
	WARN("Invalid Parameter\n");
	return MMSYSERR_INVALPARAM;
    }
    if (dwSize < offsetof(MIDIHDR,dwOffset)) {
	WARN("Invalid Parameter\n");
	return MMSYSERR_INVALPARAM;
    }
    if (lpMidiHdr->dwBufferLength == 0) {
	WARN("Invalid Parameter\n");
	return MMSYSERR_INVALPARAM;
    }
    if (lpMidiHdr->dwFlags & MHDR_INQUEUE) {
	WARN("Still playing\n");
	return MIDIERR_STILLPLAYING;
    }
    if (!(lpMidiHdr->dwFlags & MHDR_PREPARED)) {
	WARN("Unprepared\n");
	return MIDIERR_UNPREPARED;
    }

    EnterCriticalSection(&midiInLock);
    lpMidiHdr->dwFlags &= ~WHDR_DONE;
    lpMidiHdr->dwFlags |= MHDR_INQUEUE;
    lpMidiHdr->dwBytesRecorded = 0;
    lpMidiHdr->lpNext = 0;
    if (sources[wDevID].lpQueueHdr == 0) {
	sources[wDevID].lpQueueHdr = lpMidiHdr;
    } else {
	LPMIDIHDR ptr;
	for (ptr = sources[wDevID].lpQueueHdr;
	     ptr->lpNext != 0;
	     ptr = ptr->lpNext);
	ptr->lpNext = lpMidiHdr;
    }
    LeaveCriticalSection(&midiInLock);

    return MMSYSERR_NOERROR;
}

static DWORD MIDIIn_Reset(WORD wDevID)
{
    DWORD dwTime = GetTickCount();

    TRACE("%d\n", wDevID);
    if (wDevID >= MIDIIn_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }

    EnterCriticalSection(&midiInLock);
    while (sources[wDevID].lpQueueHdr) {
	LPMIDIHDR lpMidiHdr = sources[wDevID].lpQueueHdr;
	sources[wDevID].lpQueueHdr = lpMidiHdr->lpNext;
	lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
	lpMidiHdr->dwFlags |= MHDR_DONE;
	/* FIXME: when called from 16 bit, lpQueueHdr needs to be a segmented ptr */
	MIDI_NotifyClient(wDevID, MIM_LONGDATA, (DWORD_PTR)lpMidiHdr, dwTime);
    }
    LeaveCriticalSection(&midiInLock);

    return MMSYSERR_NOERROR;
}

/*
 * MIDI In Mach message handling
 */
static CFDataRef MIDIIn_MessageHandler(CFMessagePortRef local, SInt32 msgid, CFDataRef data, void *info)
{
    MIDIMessage *msg = NULL;
    int i = 0;
    MIDISource *src = NULL;
    DWORD sendData = 0;
    int pos = 0;
    DWORD currentTime;
    BOOL sysexStart;

    switch (msgid)
    {
        case 0:
            msg = (MIDIMessage *) CFDataGetBytePtr(data);
            TRACE("devID=%d\n", msg->devID);
             for (i = 0; i < msg->length; ++i) {
                TRACE("%02X ", msg->data[i]);
            }
            TRACE("\n");
            src = &sources[msg->devID];
            if (src->state < 1)
            {
                TRACE("input not started, thrown away\n");
                return NULL;
            }

            sysexStart = (msg->data[0] == 0xF0);

            if (sysexStart || src->state & 2) {
                int pos = 0;
                int len = msg->length;

                if (sysexStart) {
                    TRACE("Receiving sysex message\n");
                    src->state |= 2;
                }

                EnterCriticalSection(&midiInLock);
                currentTime = GetTickCount() - src->startTime;

                while (len) {
                    LPMIDIHDR lpMidiHdr = src->lpQueueHdr;

                    if (lpMidiHdr != NULL) {
                        int copylen = min(len, lpMidiHdr->dwBufferLength - lpMidiHdr->dwBytesRecorded);
                        memcpy(lpMidiHdr->lpData + lpMidiHdr->dwBytesRecorded, msg->data + pos, copylen);
                        lpMidiHdr->dwBytesRecorded += copylen;
                        len -= copylen;
                        pos += copylen;

                        TRACE("Copied %d bytes of sysex message\n", copylen);

                        if ((lpMidiHdr->dwBytesRecorded == lpMidiHdr->dwBufferLength) ||
                            (*(BYTE*)(lpMidiHdr->lpData + lpMidiHdr->dwBytesRecorded - 1) == 0xF7)) {
                            TRACE("Sysex message complete (or buffer limit reached), dispatching %d bytes\n", lpMidiHdr->dwBytesRecorded);
                            src->lpQueueHdr = lpMidiHdr->lpNext;
                            lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
                            lpMidiHdr->dwFlags |= MHDR_DONE;
                            MIDI_NotifyClient(msg->devID, MIM_LONGDATA, (DWORD_PTR)lpMidiHdr, currentTime);
                            src->state &= ~2;
                        }
                    }
                    else {
                        FIXME("Sysex data received but no buffer to store it!\n");
                        break;
                    }
                }

                LeaveCriticalSection(&midiInLock);
                return NULL;
            }

            EnterCriticalSection(&midiInLock);
            currentTime = GetTickCount() - src->startTime;

            while (pos < msg->length)
            {
                sendData = 0;
                switch (msg->data[pos] & 0xF0)
                {
                    case 0xF0:
                        sendData = (msg->data[pos] <<  0);
                        pos++;
                        break;

                    case 0xC0:
                    case 0xD0:
                        sendData = (msg->data[pos + 1] <<  8) | (msg->data[pos] <<  0);
                        pos += 2;
                        break;
                    default:
                        sendData = (msg->data[pos + 2] << 16) |
                                    (msg->data[pos + 1] <<  8) |
                                    (msg->data[pos] <<  0);
                        pos += 3;
                        break;
                }
                MIDI_NotifyClient(msg->devID, MIM_DATA, sendData, currentTime);
            }
            LeaveCriticalSection(&midiInLock);
            break;
        default:
            CFRunLoopStop(CFRunLoopGetCurrent());
            break;
    }
    return NULL;
}

static DWORD WINAPI MIDIIn_MessageThread(LPVOID p)
{
    CFMessagePortRef local;
    CFRunLoopSourceRef source;
    Boolean info;

    local = CFMessagePortCreateLocal(kCFAllocatorDefault, MIDIInThreadPortName, &MIDIIn_MessageHandler, NULL, &info);

    source = CFMessagePortCreateRunLoopSource(kCFAllocatorDefault, local, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode);

    CFRunLoopRun();

    CFRunLoopSourceInvalidate(source);
    CFRelease(source);
    CFRelease(local);
    CFRelease(MIDIInThreadPortName);
    MIDIInThreadPortName = NULL;

    return 0;
}

/**************************************************************************
* 				modMessage
*/
DWORD WINAPI CoreAudio_modMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    struct midi_out_message_params params;
    struct notify_context notify;
    DWORD err;

    TRACE("%d %08x %08lx %08lx %08lx\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);

    params.dev_id = wDevID;
    params.msg = wMsg;
    params.user = dwUser;
    params.param_1 = dwParam1;
    params.param_2 = dwParam2;
    params.err = &err;
    params.notify = &notify;

    UNIX_CALL(midi_out_message, &params);

    if (!err && notify.send_notify) notify_client(&notify);

    return err;
}

/**************************************************************************
* 			midMessage
*/
DWORD WINAPI CoreAudio_midMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    struct midi_in_message_params params;
    struct notify_context notify;
    DWORD err;

    TRACE("%d %08x %08lx %08lx %08lx\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    switch (wMsg) {
        case MIDM_ADDBUFFER:
            return MIDIIn_AddBuffer(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
        case MIDM_RESET:
            return MIDIIn_Reset(wDevID);
    }

    params.dev_id = wDevID;
    params.msg = wMsg;
    params.user = dwUser;
    params.param_1 = dwParam1;
    params.param_2 = dwParam2;
    params.err = &err;
    params.notify = &notify;

    UNIX_CALL(midi_in_message, &params);

    if (!err && notify.send_notify) notify_client(&notify);

    return err;
}

/**************************************************************************
 * 				CoreAudio_drvLoad       [internal]
 */
static LRESULT CoreAudio_drvLoad(void)
{
    TRACE("()\n");

    if (CoreAudio_MIDIInit() != DRV_SUCCESS)
        return DRV_FAILURE;

    return DRV_SUCCESS;
}

/**************************************************************************
 * 				CoreAudio_drvFree       [internal]
 */
static LRESULT CoreAudio_drvFree(void)
{
    TRACE("()\n");
    CoreAudio_MIDIRelease();
    return DRV_SUCCESS;
}

/**************************************************************************
 * 				CoreAudio_drvOpen       [internal]
 */
static LRESULT CoreAudio_drvOpen(LPSTR str)
{
    TRACE("(%s)\n", str);
    return 1;
}

/**************************************************************************
 * 				CoreAudio_drvClose      [internal]
 */
static DWORD CoreAudio_drvClose(DWORD dwDevID)
{
    TRACE("(%08x)\n", dwDevID);
    return 1;
}

/**************************************************************************
 * 				DriverProc (WINECOREAUDIO.1)
 */
LRESULT CALLBACK CoreAudio_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                      LPARAM dwParam1, LPARAM dwParam2)
{
     TRACE("(%08lX, %p, %s (%08X), %08lX, %08lX)\n",
           dwDevID, hDriv, wMsg == DRV_LOAD ? "DRV_LOAD" :
           wMsg == DRV_FREE ? "DRV_FREE" :
           wMsg == DRV_OPEN ? "DRV_OPEN" :
           wMsg == DRV_CLOSE ? "DRV_CLOSE" :
           wMsg == DRV_ENABLE ? "DRV_ENABLE" :
           wMsg == DRV_DISABLE ? "DRV_DISABLE" :
           wMsg == DRV_QUERYCONFIGURE ? "DRV_QUERYCONFIGURE" :
           wMsg == DRV_CONFIGURE ? "DRV_CONFIGURE" :
           wMsg == DRV_INSTALL ? "DRV_INSTALL" :
           wMsg == DRV_REMOVE ? "DRV_REMOVE" : "UNKNOWN",
           wMsg, dwParam1, dwParam2);

    switch(wMsg) {
    case DRV_LOAD:		return CoreAudio_drvLoad();
    case DRV_FREE:		return CoreAudio_drvFree();
    case DRV_OPEN:		return CoreAudio_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:		return CoreAudio_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "CoreAudio driver!", "CoreAudio driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}
