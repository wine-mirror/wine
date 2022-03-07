/* WinRT Windows.Gaming.Input implementation
 *
 * Copyright 2022 Rémi Bernon for CodeWeavers
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
#include <stddef.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winstring.h"
#include "objbase.h"

#include "activation.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Gaming_Input
#define WIDL_using_Windows_Gaming_Input_Custom
#include "windows.gaming.input.custom.h"

extern HINSTANCE windows_gaming_input;
extern ICustomGameControllerFactory *controller_factory;
extern ICustomGameControllerFactory *gamepad_factory;
extern IActivationFactory *manager_factory;

extern HRESULT vector_create( REFIID iid, REFIID view_iid, void **out );

extern void provider_create( const WCHAR *device_path );
extern void provider_remove( const WCHAR *device_path );

extern void manager_on_provider_created( IGameControllerProvider *provider );
extern void manager_on_provider_removed( IGameControllerProvider *provider );

#define DEFINE_IINSPECTABLE( pfx, iface_type, impl_type, base_iface )                              \
    static inline impl_type *impl_from_##iface_type( iface_type *iface )                           \
    {                                                                                              \
        return CONTAINING_RECORD( iface, impl_type, iface_type##_iface );                          \
    }                                                                                              \
    static HRESULT WINAPI pfx##_QueryInterface( iface_type *iface, REFIID iid, void **out )        \
    {                                                                                              \
        impl_type *impl = impl_from_##iface_type( iface );                                         \
        return IInspectable_QueryInterface( (IInspectable *)&impl->base_iface, iid, out );         \
    }                                                                                              \
    static ULONG WINAPI pfx##_AddRef( iface_type *iface )                                          \
    {                                                                                              \
        impl_type *impl = impl_from_##iface_type( iface );                                         \
        return IInspectable_AddRef( (IInspectable *)&impl->base_iface );                           \
    }                                                                                              \
    static ULONG WINAPI pfx##_Release( iface_type *iface )                                         \
    {                                                                                              \
        impl_type *impl = impl_from_##iface_type( iface );                                         \
        return IInspectable_Release( (IInspectable *)&impl->base_iface );                          \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetIids( iface_type *iface, ULONG *iid_count, IID **iids )         \
    {                                                                                              \
        impl_type *impl = impl_from_##iface_type( iface );                                         \
        return IInspectable_GetIids( (IInspectable *)&impl->base_iface, iid_count, iids );         \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetRuntimeClassName( iface_type *iface, HSTRING *class_name )      \
    {                                                                                              \
        impl_type *impl = impl_from_##iface_type( iface );                                         \
        return IInspectable_GetRuntimeClassName( (IInspectable *)&impl->base_iface, class_name );  \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetTrustLevel( iface_type *iface, TrustLevel *trust_level )        \
    {                                                                                              \
        impl_type *impl = impl_from_##iface_type( iface );                                         \
        return IInspectable_GetTrustLevel( (IInspectable *)&impl->base_iface, trust_level );       \
    }
