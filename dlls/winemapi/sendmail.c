/*
 * MAPISendMail implementation
 *
 * Copyright 2005 Hans Leidekker
 * Copyright 2009 Owen Rudge for CodeWeavers
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

#include <stdio.h>
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "mapi.h"
#include "winreg.h"
#include "shellapi.h"
#include "shlwapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winemapi);

/**************************************************************************
 *  MAPISendMail
 *
 * Send a mail.
 *
 * PARAMS
 *  session  [I] Handle to a MAPI session.
 *  uiparam  [I] Parent window handle.
 *  message  [I] Pointer to a MAPIMessage structure.
 *  flags    [I] Flags.
 *  reserved [I] Reserved, pass 0.
 *
 * RETURNS
 *  Success: SUCCESS_SUCCESS
 *  Failure: MAPI_E_FAILURE
 *
 */
ULONG WINAPI MAPISendMail(LHANDLE session, ULONG_PTR uiparam,
    lpMapiMessage message, FLAGS flags, ULONG reserved)
{
    FIXME("(0x%08x 0x%08lx %p 0x%08x 0x%08x): stub\n", session, uiparam,
           message, flags, reserved);

    return MAPI_E_NOT_SUPPORTED;
}

ULONG WINAPI MAPISendDocuments(ULONG_PTR uiparam, LPSTR delim, LPSTR paths,
    LPSTR filenames, ULONG reserved)
{
    return MAPI_E_NOT_SUPPORTED;
}
