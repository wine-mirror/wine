/*
 * Implementation of IEnumMediaTypes Interface
 *
 * Copyright 2003 Robert Shearman
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"

#include "qcap_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

BOOL CompareMediaTypes(const AM_MEDIA_TYPE * pmt1, const AM_MEDIA_TYPE * pmt2,
                       BOOL bWildcards)
{
    TRACE("pmt1: ");
    dump_AM_MEDIA_TYPE(pmt1);
    TRACE("pmt2: ");
    dump_AM_MEDIA_TYPE(pmt2);
    return (((bWildcards && (IsEqualGUID(&pmt1->majortype, &GUID_NULL) ||
              IsEqualGUID(&pmt2->majortype, &GUID_NULL))) ||
              IsEqualGUID(&pmt1->majortype, &pmt2->majortype)) &&
              ((bWildcards && (IsEqualGUID(&pmt1->subtype, &GUID_NULL) ||
              IsEqualGUID(&pmt2->subtype, &GUID_NULL))) ||
              IsEqualGUID(&pmt1->subtype, &pmt2->subtype)));
}

void dump_AM_MEDIA_TYPE(const AM_MEDIA_TYPE * pmt)
{
    if (!pmt)
        return;
    TRACE("\t%s\n\t%s\n\t...\n\t%s\n", debugstr_guid(&pmt->majortype),
          debugstr_guid(&pmt->subtype), debugstr_guid(&pmt->formattype));
}
