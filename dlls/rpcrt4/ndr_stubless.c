/*
 * NDR client stuff
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
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
 *
 * TODO:
 *  - actually implement RPCRT4_NdrClientCall2
 */

#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "rpc.h"
#include "rpcndr.h"

#include "wine/debug.h"

#include "ndr_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);


LONG_PTR RPCRT4_NdrClientCall2(PMIDL_STUB_DESC pStubDesc, PFORMAT_STRING pFormat, va_list args)
{
  FIXME("(%p,%p,...)\n", pStubDesc, pFormat);
  return 0;
}

/***********************************************************************
 *           NdrClientCall2 [RPCRT4.@]
 */
CLIENT_CALL_RETURN WINAPIV NdrClientCall2(PMIDL_STUB_DESC pStubDesc,
                                         PFORMAT_STRING pFormat,
                                         ...)
{
    CLIENT_CALL_RETURN ret;
    va_list args;

    TRACE("(%p,%p,...)\n", pStubDesc, pFormat);

    va_start(args, pFormat);
    ret = (CLIENT_CALL_RETURN)RPCRT4_NdrClientCall2(pStubDesc, pFormat, args);
    va_end(args);
    return ret;
}
