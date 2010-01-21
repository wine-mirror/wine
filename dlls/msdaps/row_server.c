/*
 *    Row and rowset servers / proxies.
 *
 * Copyright 2010 Huw Davies
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
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "objbase.h"
#include "oleauto.h"
#include "oledb.h"

#include "row_server.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

HRESULT create_row_server(IUnknown *outer, void **obj)
{
    FIXME("(%p, %p): stub\n", outer, obj);
    *obj = NULL;
    return E_NOTIMPL;
}

HRESULT create_rowset_server(IUnknown *outer, void **obj)
{
    FIXME("(%p, %p): stub\n", outer, obj);
    *obj = NULL;
    return E_NOTIMPL;
}

HRESULT create_row_marshal(IUnknown *outer, void **obj)
{
    FIXME("(%p, %p): stub\n", outer, obj);
    *obj = NULL;
    return E_NOTIMPL;
}

HRESULT create_rowset_marshal(IUnknown *outer, void **obj)
{
    FIXME("(%p, %p): stub\n", outer, obj);
    *obj = NULL;
    return E_NOTIMPL;
}
