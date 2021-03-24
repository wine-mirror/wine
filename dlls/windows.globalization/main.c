/* WinRT Windows.Globalization implementation
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
#include "winstring.h"
#include "wine/debug.h"
#include "objbase.h"

#include "initguid.h"
#include "activation.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Globalization
#include "windows.globalization.h"
#define WIDL_using_Windows_System_UserProfile
#include "windows.system.userprofile.h"

WINE_DEFAULT_DEBUG_CHANNEL(locale);

static const char *debugstr_hstring(HSTRING hstr)
{
    const WCHAR *str;
    UINT32 len;
    if (hstr && !((ULONG_PTR)hstr >> 16)) return "(invalid)";
    str = WindowsGetStringRawBuffer(hstr, &len);
    return wine_dbgstr_wn(str, len);
}

struct windows_globalization
{
    IActivationFactory IActivationFactory_iface;
    IGlobalizationPreferencesStatics IGlobalizationPreferencesStatics_iface;
    LONG ref;
};

static inline struct windows_globalization *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct windows_globalization, IActivationFactory_iface);
}

static inline struct windows_globalization *impl_from_IGlobalizationPreferencesStatics(IGlobalizationPreferencesStatics *iface)
{
    return CONTAINING_RECORD(iface, struct windows_globalization, IGlobalizationPreferencesStatics_iface);
}

static HRESULT STDMETHODCALLTYPE windows_globalization_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **out)
{
    struct windows_globalization *impl = impl_from_IActivationFactory(iface);
    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IActivationFactory_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IGlobalizationPreferencesStatics))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IGlobalizationPreferencesStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE windows_globalization_AddRef(
        IActivationFactory *iface)
{
    struct windows_globalization *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE windows_globalization_Release(
        IActivationFactory *iface)
{
    struct windows_globalization *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE windows_globalization_GetIids(
        IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_globalization_GetRuntimeClassName(
        IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_globalization_GetTrustLevel(
        IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_globalization_ActivateInstance(
        IActivationFactory *iface, IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    windows_globalization_QueryInterface,
    windows_globalization_AddRef,
    windows_globalization_Release,
    /* IInspectable methods */
    windows_globalization_GetIids,
    windows_globalization_GetRuntimeClassName,
    windows_globalization_GetTrustLevel,
    /* IActivationFactory methods */
    windows_globalization_ActivateInstance,
};

static HRESULT STDMETHODCALLTYPE globalization_preferences_QueryInterface(
        IGlobalizationPreferencesStatics *iface, REFIID iid, void **object)
{
    struct windows_globalization *impl = impl_from_IGlobalizationPreferencesStatics(iface);
    return windows_globalization_QueryInterface(&impl->IActivationFactory_iface, iid, object);
}

static ULONG STDMETHODCALLTYPE globalization_preferences_AddRef(
        IGlobalizationPreferencesStatics *iface)
{
    struct windows_globalization *impl = impl_from_IGlobalizationPreferencesStatics(iface);
    return windows_globalization_AddRef(&impl->IActivationFactory_iface);
}

static ULONG STDMETHODCALLTYPE globalization_preferences_Release(
        IGlobalizationPreferencesStatics *iface)
{
    struct windows_globalization *impl = impl_from_IGlobalizationPreferencesStatics(iface);
    return windows_globalization_Release(&impl->IActivationFactory_iface);
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_GetIids(
        IGlobalizationPreferencesStatics *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_GetRuntimeClassName(
        IGlobalizationPreferencesStatics *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_GetTrustLevel(
        IGlobalizationPreferencesStatics *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_get_Calendars(
        IGlobalizationPreferencesStatics *iface, IVectorView_HSTRING **out)
{
    FIXME("iface %p, out %p stub!\n", iface, out);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_get_Clocks(
        IGlobalizationPreferencesStatics *iface, IVectorView_HSTRING **out)
{
    FIXME("iface %p, out %p stub!\n", iface, out);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_get_Currencies(
        IGlobalizationPreferencesStatics *iface, IVectorView_HSTRING **out)
{
    FIXME("iface %p, out %p stub!\n", iface, out);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_get_Languages(
        IGlobalizationPreferencesStatics *iface, IVectorView_HSTRING **out)
{
    FIXME("iface %p, out %p stub!\n", iface, out);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_get_HomeGeographicRegion(
        IGlobalizationPreferencesStatics *iface, HSTRING* out)
{
    FIXME("iface %p, out %p stub!\n", iface, out);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_get_WeekStartsOn(
        IGlobalizationPreferencesStatics *iface, enum DayOfWeek* out)
{
    FIXME("iface %p, out %p stub!\n", iface, out);
    return E_NOTIMPL;
}

static const struct IGlobalizationPreferencesStaticsVtbl globalization_preferences_vtbl =
{
    globalization_preferences_QueryInterface,
    globalization_preferences_AddRef,
    globalization_preferences_Release,
    /* IInspectable methods */
    globalization_preferences_GetIids,
    globalization_preferences_GetRuntimeClassName,
    globalization_preferences_GetTrustLevel,
    /* IGlobalizationPreferencesStatics methods */
    globalization_preferences_get_Calendars,
    globalization_preferences_get_Clocks,
    globalization_preferences_get_Currencies,
    globalization_preferences_get_Languages,
    globalization_preferences_get_HomeGeographicRegion,
    globalization_preferences_get_WeekStartsOn,
};

static struct windows_globalization windows_globalization =
{
    {&activation_factory_vtbl},
    {&globalization_preferences_vtbl},
    0
};

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    FIXME("clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory(HSTRING classid, IActivationFactory **factory)
{
    TRACE("classid %s, factory %p.\n", debugstr_hstring(classid), factory);
    *factory = &windows_globalization.IActivationFactory_iface;
    IUnknown_AddRef(*factory);
    return S_OK;
}
