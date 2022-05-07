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
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "mmddk.h"
#include "mmdeviceapi.h"
#include "audioclient.h"
#include "wine/debug.h"
#include "wine/unixlib.h"
#include "coreaudio.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(midi);

static void notify_client(struct notify_context *notify)
{
    TRACE("dev_id=%d msg=%d param1=%04IX param2=%04IX\n", notify->dev_id, notify->msg, notify->param_1, notify->param_2);

    DriverCallback(notify->callback, notify->flags, notify->device, notify->msg,
                   notify->instance, notify->param_1, notify->param_2);
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
        UNIX_CALL(midi_notify_wait, &params);
        if (quit) break;
        if (notify.send_notify) notify_client(&notify);
    }
    return 0;
}

static LONG CoreAudio_MIDIInit(void)
{
    struct midi_init_params params;
    UINT err;

    params.err = &err;

    UNIX_CALL(midi_init, &params);
    if (err != DRV_SUCCESS)
    {
        ERR("can't create midi client\n");
        return err;
    }

    CloseHandle(CreateThread(NULL, 0, notify_thread, NULL, 0, NULL));

    return err;
}

static LONG CoreAudio_MIDIRelease(void)
{
    TRACE("\n");

    UNIX_CALL(midi_release, NULL);

    return DRV_SUCCESS;
}

/**************************************************************************
* 				modMessage
*/
DWORD WINAPI CoreAudio_modMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    struct midi_out_message_params params;
    struct notify_context notify;
    UINT err;

    TRACE("%d %08x %08Ix %08Ix %08Ix\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);

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
    UINT err;

    TRACE("%d %08x %08Ix %08Ix %08Ix\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);

    params.dev_id = wDevID;
    params.msg = wMsg;
    params.user = dwUser;
    params.param_1 = dwParam1;
    params.param_2 = dwParam2;
    params.err = &err;
    params.notify = &notify;

    do
    {
        UNIX_CALL(midi_in_message, &params);
        if ((!err || err == ERROR_RETRY) && notify.send_notify) notify_client(&notify);
    } while (err == ERROR_RETRY);

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
    TRACE("(%08lx)\n", dwDevID);
    return 1;
}

/**************************************************************************
 * 				DriverProc (WINECOREAUDIO.1)
 */
LRESULT CALLBACK CoreAudio_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                      LPARAM dwParam1, LPARAM dwParam2)
{
     TRACE("(%08IX, %p, %s (%08X), %08IX, %08IX)\n",
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
