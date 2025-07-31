/* WinRT weak reference helpers
 *
 * Copyright 2025 Zhiyi Zhang for CodeWeavers
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

#ifndef __WINE_WEAKREF_H
#define __WINE_WEAKREF_H

#include "weakreference.h"

struct weak_reference_source
{
    IWeakReferenceSource IWeakReferenceSource_iface;
    IWeakReference *weak_reference;
};

/* Initialize a IWeakReferenceSource with the object to manage */
HRESULT weak_reference_source_init( struct weak_reference_source *source, IUnknown *object );
/* Add a strong reference to the managed object */
ULONG weak_reference_strong_add_ref( struct weak_reference_source *source );
/* Release a strong reference to the managed object */
ULONG weak_reference_strong_release( struct weak_reference_source *source );

#endif /* __WINE_WEAKREF_H */
