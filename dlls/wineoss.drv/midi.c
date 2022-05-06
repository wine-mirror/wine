/*
 * MIDI driver for OSS (PE-side)
 *
 * Copyright 1994       Martin Ayotte
 * Copyright 1998       Luiz Otavio L. Zorzella (init procedures)
 * Copyright 1998, 1999 Eric POUECH
 * Copyright 2022       Huw Davies
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

/* TODO:
 *    + use better instrument definition for OPL/2 (midiPatch.c) or
 *      use existing instrument definition (from playmidi or kmid)
 *      with a .winerc option 
 *    + have a look at OPL/3 ?
 *    + implement asynchronous playback of MidiHdr
 *    + implement STREAM'ed MidiHdr (question: how shall we share the
 *      code between the midiStream functions in MMSYSTEM/WINMM and
 *      the code for the low level driver)
 *    + use a more accurate read mechanism than the one of snooping on
 *      timers (like select on fd)
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "mmddk.h"
#include "audioclient.h"

#include "wine/debug.h"
#include "wine/unixlib.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(midi);

/*======================================================================*
 *                  Low level MIDI implementation			*
 *======================================================================*/

static void notify_client(struct notify_context *notify)
{
    TRACE("dev_id = %d msg = %d param1 = %04IX param2 = %04IX\n",
          notify->dev_id, notify->msg, notify->param_1, notify->param_2);

    DriverCallback(notify->callback, notify->flags, notify->device, notify->msg,
                   notify->instance, notify->param_1, notify->param_2);
}

/*======================================================================*
 *                  	    MIDI entry points 				*
 *======================================================================*/

/**************************************************************************
 * 			midMessage (WINEOSS.@)
 */
DWORD WINAPI OSS_midMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser,
			    DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    struct midi_in_message_params params;
    struct notify_context notify;
    UINT err;

    TRACE("(%04X, %04X, %08IX, %08IX, %08IX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    params.dev_id = wDevID;
    params.msg = wMsg;
    params.user = dwUser;
    params.param_1 = dwParam1;
    params.param_2 = dwParam2;
    params.err = &err;
    params.notify = &notify;

    do
    {
        OSS_CALL(midi_in_message, &params);
        if ((!err || err == ERROR_RETRY) && notify.send_notify) notify_client(&notify);
    } while (err == ERROR_RETRY);

    return err;
}

/**************************************************************************
 * 				modMessage (WINEOSS.@)
 */
DWORD WINAPI OSS_modMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser,
			    DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    struct midi_out_message_params params;
    struct notify_context notify;
    UINT err;

    TRACE("(%04X, %04X, %08IX, %08IX, %08IX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    params.dev_id = wDevID;
    params.msg = wMsg;
    params.user = dwUser;
    params.param_1 = dwParam1;
    params.param_2 = dwParam2;
    params.err = &err;
    params.notify = &notify;

    OSS_CALL(midi_out_message, &params);

    if (!err && notify.send_notify) notify_client(&notify);

    return err;
}

static DWORD WINAPI notify_thread(void *p)
{
    struct midi_notify_wait_params params;
    struct notify_context notify;
    BOOL quit;

    params.notify = &notify;
    params.quit = &quit;

    while (1)
    {
        OSS_CALL(midi_notify_wait, &params);
        if (quit) break;
        if (notify.send_notify) notify_client(&notify);
    }
    return 0;
}

/**************************************************************************
 * 				DriverProc (WINEOSS.1)
 */
LRESULT CALLBACK OSS_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                LPARAM dwParam1, LPARAM dwParam2)
{
     TRACE("(%08IX, %p, %08X, %08IX, %08IX)\n",
           dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    switch(wMsg) {
    case DRV_LOAD:
        CloseHandle(CreateThread(NULL, 0, notify_thread, NULL, 0, NULL));
        return 1;
    case DRV_FREE:
        OSS_CALL(midi_release, NULL);
        return 1;
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_ENABLE:
    case DRV_DISABLE:
    case DRV_QUERYCONFIGURE:
    case DRV_CONFIGURE:
        return 1;
    case DRV_INSTALL:
    case DRV_REMOVE:
        return DRV_SUCCESS;
    default:
	return 0;
    }
}
