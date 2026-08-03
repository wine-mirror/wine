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
#include "dinput.h"

#include "activation.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#define WIDL_using_Windows_Foundation_Numerics
#include "windows.foundation.h"
#define WIDL_using_Windows_Devices_Power
#define WIDL_using_Windows_Gaming_Input
#define WIDL_using_Windows_Gaming_Input_Custom
#define WIDL_using_Windows_Gaming_Input_ForceFeedback
#include "windows.gaming.input.custom.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "provider.h"

extern HINSTANCE windows_gaming_input;
extern ICustomGameControllerFactory *controller_factory;
extern ICustomGameControllerFactory *gamepad_factory;
extern ICustomGameControllerFactory *racing_wheel_factory;
extern IGameControllerFactoryManagerStatics2 *manager_factory;
extern IInspectable *constant_effect_factory;
extern IInspectable *ramp_effect_factory;
extern IInspectable *periodic_effect_factory;
extern IInspectable *condition_effect_factory;

struct vector_iids
{
    const GUID *vector;
    const GUID *view;
    const GUID *iterable;
    const GUID *iterator;
};
extern HRESULT vector_create( const struct vector_iids *iids, void **out );

extern void provider_create( const WCHAR *device_path );
extern void provider_remove( const WCHAR *device_path );

extern void manager_on_provider_created( IGameControllerProvider *provider );
extern void manager_on_provider_removed( IGameControllerProvider *provider );

extern HRESULT event_handlers_append( struct list *list, IEventHandler_IInspectable *handler, EventRegistrationToken *token );
extern HRESULT event_handlers_remove( struct list *list, EventRegistrationToken *token );
extern void event_handlers_notify( struct list *list, IInspectable *element );

extern HRESULT force_feedback_motor_create( IDirectInputDevice8W *device, IForceFeedbackMotor **out );
extern HRESULT force_feedback_effect_create( enum WineForceFeedbackEffectType type, IInspectable *outer, IWineForceFeedbackEffectImpl **out );

typedef HRESULT (WINAPI *async_operation_callback)( IUnknown *invoker, IUnknown *param, PROPVARIANT *result );
extern HRESULT async_operation_boolean_create( IUnknown *invoker, IUnknown *param, async_operation_callback callback,
                                               IAsyncOperation_boolean **out );
extern HRESULT async_operation_effect_result_create( IUnknown *invoker, IUnknown *param, async_operation_callback callback,
                                                     IAsyncOperation_ForceFeedbackLoadEffectResult **out );

#define DEFINE_IINSPECTABLE_( pfx, iface_type, impl_type, impl_from, iface_mem, expr )             \
    static inline impl_type *impl_from( iface_type *iface )                                        \
    {                                                                                              \
        return CONTAINING_RECORD( iface, impl_type, iface_mem );                                   \
    }                                                                                              \
    static HRESULT WINAPI pfx##_QueryInterface( iface_type *iface, REFIID iid, void **out )        \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_QueryInterface( (IInspectable *)(expr), iid, out );                    \
    }                                                                                              \
    static ULONG WINAPI pfx##_AddRef( iface_type *iface )                                          \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_AddRef( (IInspectable *)(expr) );                                      \
    }                                                                                              \
    static ULONG WINAPI pfx##_Release( iface_type *iface )                                         \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_Release( (IInspectable *)(expr) );                                     \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetIids( iface_type *iface, ULONG *iid_count, IID **iids )         \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_GetIids( (IInspectable *)(expr), iid_count, iids );                    \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetRuntimeClassName( iface_type *iface, HSTRING *class_name )      \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_GetRuntimeClassName( (IInspectable *)(expr), class_name );             \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetTrustLevel( iface_type *iface, TrustLevel *trust_level )        \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_GetTrustLevel( (IInspectable *)(expr), trust_level );                  \
    }
#define DEFINE_IINSPECTABLE( pfx, iface_type, impl_type, base_iface )                              \
    DEFINE_IINSPECTABLE_( pfx, iface_type, impl_type, impl_from_##iface_type, iface_type##_iface, &impl->base_iface )
#define DEFINE_IINSPECTABLE_OUTER( pfx, iface_type, impl_type, outer_iface )                       \
    DEFINE_IINSPECTABLE_( pfx, iface_type, impl_type, impl_from_##iface_type, iface_type##_iface, impl->outer_iface )

static inline const char *debugstr_vector3( const Vector3 *vector )
{
    if (!vector) return "(null)";
    return wine_dbg_sprintf( "[%f, %f, %f]", vector->X, vector->Y, vector->Z );
}
