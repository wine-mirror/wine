/* WinRT Windows.Xbox.Input implementation
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
#define WIDL_using_Windows_Xbox_Input
#include "input.h"

#include "wine/debug.h"

/* Bridge to the real Windows.Gaming.Input.Gamepad backend, used to back
 * Windows.Xbox.Input.Gamepad with live controller data (see wgi_backend.c).
 * Windows.Gaming.Input and Windows.Xbox.Input both generate unprefixed
 * IGamepad/IGamepadStatics/GamepadReading/GamepadButtons/etc symbols, so
 * windows.gaming.input.h cannot be included from this header; wgi_backend.h
 * only uses plain C types. */
#include "wgi_backend.h"

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

/* minimal read-only IVectorView<T>/IIterable<T>/IIterator<T> helper, local
 * to this DLL. Elements are IInspectable-derived interface pointers, the
 * caller supplies the right IIDs for the instantiated generic type. */
struct vector_view_iids
{
    const GUID *view;
    const GUID *iterable;
    const GUID *iterator;
};

extern HRESULT vector_view_create( const struct vector_view_iids *iids, IInspectable **elements,
                                   UINT32 count, void **out );

/* Minimal IController/IControllerStatics definitions.
 * widl cannot currently regenerate input.h, so these are hand-written to
 * match the interface declarations in input.idl / WinDurango (MIT). */

typedef struct IController IController;
typedef struct IControllerVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IController*, REFIID, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IController*);
    ULONG   (STDMETHODCALLTYPE *Release)(IController*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(IController*, ULONG*, IID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(IController*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(IController*, TrustLevel*);
    HRESULT (STDMETHODCALLTYPE *get_Type)(IController*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *get_Id)(IController*, UINT64*);
    HRESULT (STDMETHODCALLTYPE *get_IsWireless)(IController*, boolean*);
} IControllerVtbl;
struct IController { CONST_VTBL IControllerVtbl *lpVtbl; };
DEFINE_GUID(IID_IController, 0xb8a54bbe,0x9f82,0x4bb2,0xa9,0xaa,0x66,0x88,0x99,0x40,0x67,0x80);

typedef struct IControllerStatics IControllerStatics;
typedef struct IControllerStaticsVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IControllerStatics*, REFIID, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IControllerStatics*);
    ULONG   (STDMETHODCALLTYPE *Release)(IControllerStatics*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(IControllerStatics*, ULONG*, IID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(IControllerStatics*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(IControllerStatics*, TrustLevel*);
    HRESULT (STDMETHODCALLTYPE *get_Controllers)(IControllerStatics*, void**);
} IControllerStaticsVtbl;
struct IControllerStatics { CONST_VTBL IControllerStaticsVtbl *lpVtbl; };
DEFINE_GUID(IID_IControllerStatics, 0x26e6d177,0xb1bb,0x4dd5,0x8f,0x79,0xc3,0xb0,0xc3,0xbc,0x3c,0x7b);

static const WCHAR RuntimeClass_Windows_Xbox_Input_Controller[] =
    L"Windows.Xbox.Input.Controller";

/* Windows.Xbox.System.User GUIDs (WinDurango Windows.Xbox.System.idl, MIT) */
DEFINE_GUID(IID_IUser,        0xa21a0e79,0x9a70,0x4f56,0xb1,0xc2,0x3e,0x30,0xfd,0xd7,0xab,0x4a);
DEFINE_GUID(IID_IUserStatics, 0x7a9b5cd3,0x47cb,0x4c89,0x9a,0x82,0xdc,0xe7,0xe6,0x02,0x3f,0x23);

/* gamepad.c */
extern IActivationFactory *xbox_gamepad_factory;

/* controller.c */
extern IActivationFactory *xbox_controller_factory;

/* user.c */
extern IActivationFactory *xbox_user_factory;

/* storage.c */
extern IActivationFactory *xbox_storage_factory;

/* appmodel.c */
extern IActivationFactory *xbox_appmodel_core_factory;
