/*
 * SystemRestore implementation
 *
 * Copyright 2020 Dmitry Timoshkov
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wbemcli.h"

#include "wine/debug.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

HRESULT create_restore_point( IWbemClassObject *obj, IWbemClassObject *in, IWbemClassObject **out )
{
    FIXME("stub\n");
    return S_OK;
}

HRESULT disable_restore( IWbemClassObject *obj, IWbemClassObject *in, IWbemClassObject **out )
{
    FIXME("stub\n");
    return S_OK;
}

HRESULT enable_restore( IWbemClassObject *obj, IWbemClassObject *in, IWbemClassObject **out )
{
    FIXME("stub\n");
    return S_OK;
}

HRESULT get_last_restore_status( IWbemClassObject *obj, IWbemClassObject *in, IWbemClassObject **out )
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT restore( IWbemClassObject *obj, IWbemClassObject *in, IWbemClassObject **out )
{
    FIXME("stub\n");
    return S_OK;
}
