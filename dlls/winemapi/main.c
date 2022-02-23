/*
 *             Wine MAPI provider
 *
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "mapidefs.h"
#include "mapi.h"
#include "mapix.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winemapi);


ULONG WINAPI MAPIAddress(LHANDLE session, ULONG_PTR uiparam, LPSTR caption,
    ULONG editfields, LPSTR labels, ULONG nRecips, lpMapiRecipDesc lpRecips,
    FLAGS flags, ULONG reserved, LPULONG newRecips, lpMapiRecipDesc * lppNewRecips)
{
    FIXME("(stub)\n");
    return MAPI_E_NOT_SUPPORTED;
}

ULONG WINAPI MAPIDeleteMail(LHANDLE session, ULONG_PTR uiparam, LPSTR msg_id,
    FLAGS flags, ULONG reserved)
{
    FIXME("(stub)\n");
    return MAPI_E_NOT_SUPPORTED;
}

ULONG WINAPI MAPIDetails(LHANDLE session, ULONG_PTR uiparam, lpMapiRecipDesc recip,
    FLAGS flags, ULONG reserved)
{
    FIXME("(stub)\n");
    return MAPI_E_NOT_SUPPORTED;
}

ULONG WINAPI MAPIFindNext(LHANDLE session, ULONG_PTR uiparam, LPSTR msg_type,
    LPSTR seed_msg_id, FLAGS flags, ULONG reserved, LPSTR msg_id)
{
    FIXME("(stub)\n");
    return MAPI_E_NOT_SUPPORTED;
}

ULONG WINAPI MAPILogon(ULONG_PTR uiparam, LPSTR profile, LPSTR password,
    FLAGS flags, ULONG reserved, LPLHANDLE session)
{
    TRACE("(0x%08Ix %s %p 0x%08lx 0x%08lx %p)\n", uiparam,
          debugstr_a(profile), password, flags, reserved, session);

    if (session)
        *session = 1;

    return SUCCESS_SUCCESS;
}

ULONG WINAPI MAPILogoff(LHANDLE session, ULONG_PTR uiparam, FLAGS flags,
    ULONG reserved)
{
    TRACE("(0x%08Ix 0x%08Ix 0x%08lx 0x%08lx)\n", session,
          uiparam, flags, reserved);

    return SUCCESS_SUCCESS;
}

ULONG WINAPI MAPIReadMail(LHANDLE session, ULONG_PTR uiparam, LPSTR msg_id,
    FLAGS flags, ULONG reserved, lpMapiMessage msg)
{
    FIXME("(stub)\n");
    return MAPI_E_NOT_SUPPORTED;
}

ULONG WINAPI MAPIResolveName(LHANDLE session, ULONG_PTR uiparam, LPSTR name,
    FLAGS flags, ULONG reserved, lpMapiRecipDesc *recip)
{
    static const char smtp[] = "SMTP:";

    SCODE scode;
    char *p;

    TRACE("(0x%08Ix 0x%08Ix %s 0x%08lx 0x%08lx %p)\n", session, uiparam,
          debugstr_a(name), flags, reserved, recip);

    if (!name || !name[0])
        return MAPI_E_FAILURE;

    scode = MAPIAllocateBuffer(sizeof(**recip) + sizeof(smtp) + strlen(name),
                               (LPVOID *)recip);
    if (scode != S_OK)
        return MAPI_E_INSUFFICIENT_MEMORY;

    ZeroMemory(*recip, sizeof(**recip));
    p = (char *)(*recip + 1);
    strcpy(p, smtp);
    strcpy(p + sizeof(smtp) - 1, name);

    (*recip)->ulRecipClass = MAPI_TO;
    (*recip)->lpszName = p + sizeof(smtp) - 1;
    (*recip)->lpszAddress = p;
    return SUCCESS_SUCCESS;
}

ULONG WINAPI MAPISaveMail(LHANDLE session, ULONG_PTR uiparam, lpMapiMessage msg,
    FLAGS flags, ULONG reserved, LPSTR msg_id)
{
    FIXME("(stub)\n");
    return MAPI_E_NOT_SUPPORTED;
}
