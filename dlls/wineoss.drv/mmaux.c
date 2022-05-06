/*
 * Sample AUXILIARY Wine Driver
 *
 * Copyright 1994 Martin Ayotte
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "mmddk.h"
#include "audioclient.h"
#include "winternl.h"

#include "wine/debug.h"
#include "wine/unixlib.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmaux);

/**************************************************************************
 *		auxMessage (WINEOSS.2)
 */
DWORD WINAPI OSS_auxMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser,
			    DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    struct aux_message_params params;
    UINT err;

    TRACE("(%04X, %04X, %08IX, %08IX, %08IX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    params.dev_id = wDevID;
    params.msg = wMsg;
    params.user = dwUser;
    params.param_1 = dwParam1;
    params.param_2 = dwParam2;
    params.err = &err;
    OSS_CALL(aux_message, &params);

    return err;
}
