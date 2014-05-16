/*
 * Copyright 2014 Henri Verbeet for CodeWeavers
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
 *
 */

#include "config.h"
#include "wine/port.h"
#include "wine/debug.h"

#define COBJMACROS
#include "d2d1.h"

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

HRESULT WINAPI D2D1CreateFactory(D2D1_FACTORY_TYPE factory_type, REFIID iid,
        const D2D1_FACTORY_OPTIONS *factory_options, void **factory)
{
    FIXME("factory_type %#x, iid %s, factory_options %p, factory %p stub!\n",
            factory_type, debugstr_guid(iid), factory_options, factory);

    return E_NOTIMPL;
}
