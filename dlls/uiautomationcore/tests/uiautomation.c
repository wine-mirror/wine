/*
 * UI Automation tests
 *
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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

#include "windows.h"
#include "initguid.h"
#include "uiautomation.h"
#include "ocidl.h"
#include "wine/iaccessible2.h"

#include "wine/test.h"

static HRESULT (WINAPI *pUiaProviderFromIAccessible)(IAccessible *, long, DWORD, IRawElementProviderSimple **);
static HRESULT (WINAPI *pUiaDisconnectProvider)(IRawElementProviderSimple *);

#define DEFINE_EXPECT(func) \
    static int expect_ ## func = 0, called_ ## func = 0

#define SET_EXPECT(func) \
    do { called_ ## func = 0; expect_ ## func = 1; } while(0)

#define SET_EXPECT_MULTI(func, num) \
    do { called_ ## func = 0; expect_ ## func = num; } while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func++; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func--; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = 0; \
    }while(0)

#define CHECK_CALLED_MULTI(func, num) \
    do { \
        ok(called_ ## func == num, "expected " #func " %d times (got %d)\n", num, called_ ## func); \
        expect_ ## func = called_ ## func = 0; \
    }while(0)

#define NAVDIR_INTERNAL_HWND 10
#define UIA_RUNTIME_ID_PREFIX 42

DEFINE_EXPECT(winproc_GETOBJECT_CLIENT);
DEFINE_EXPECT(winproc_GETOBJECT_UiaRoot);
DEFINE_EXPECT(Accessible_accNavigate);
DEFINE_EXPECT(Accessible_get_accParent);
DEFINE_EXPECT(Accessible_get_accChildCount);
DEFINE_EXPECT(Accessible_get_accName);
DEFINE_EXPECT(Accessible_get_accRole);
DEFINE_EXPECT(Accessible_get_accState);
DEFINE_EXPECT(Accessible_accLocation);
DEFINE_EXPECT(Accessible_get_accChild);
DEFINE_EXPECT(Accessible_get_uniqueID);
DEFINE_EXPECT(Accessible2_get_accParent);
DEFINE_EXPECT(Accessible2_get_accChildCount);
DEFINE_EXPECT(Accessible2_get_accName);
DEFINE_EXPECT(Accessible2_get_accRole);
DEFINE_EXPECT(Accessible2_get_accState);
DEFINE_EXPECT(Accessible2_accLocation);
DEFINE_EXPECT(Accessible2_QI_IAccIdentity);
DEFINE_EXPECT(Accessible2_get_uniqueID);
DEFINE_EXPECT(Accessible_child_accNavigate);
DEFINE_EXPECT(Accessible_child_get_accParent);
DEFINE_EXPECT(Accessible_child_get_accChildCount);
DEFINE_EXPECT(Accessible_child_get_accName);
DEFINE_EXPECT(Accessible_child_get_accRole);
DEFINE_EXPECT(Accessible_child_get_accState);
DEFINE_EXPECT(Accessible_child_accLocation);
DEFINE_EXPECT(Accessible_child2_accNavigate);
DEFINE_EXPECT(Accessible_child2_get_accParent);
DEFINE_EXPECT(Accessible_child2_get_accChildCount);
DEFINE_EXPECT(Accessible_child2_get_accName);
DEFINE_EXPECT(Accessible_child2_get_accRole);
DEFINE_EXPECT(Accessible_child2_get_accState);
DEFINE_EXPECT(Accessible_child2_accLocation);

static BOOL check_variant_i4(VARIANT *v, int val)
{
    if (V_VT(v) == VT_I4 && V_I4(v) == val)
        return TRUE;

    return FALSE;
}

static BOOL check_variant_bool(VARIANT *v, BOOL val)
{
    if (V_VT(v) == VT_BOOL && V_BOOL(v) == (val ? VARIANT_TRUE : VARIANT_FALSE))
        return TRUE;

    return FALSE;
}

static BOOL iface_cmp(IUnknown *iface1, IUnknown *iface2)
{
    IUnknown *unk1, *unk2;
    BOOL cmp;

    IUnknown_QueryInterface(iface1, &IID_IUnknown, (void**)&unk1);
    IUnknown_QueryInterface(iface2, &IID_IUnknown, (void**)&unk2);
    cmp = (unk1 == unk2) ? TRUE : FALSE;

    IUnknown_Release(unk1);
    IUnknown_Release(unk2);
    return cmp;
}

static struct Accessible
{
    IAccessible IAccessible_iface;
    IAccessible2 IAccessible2_iface;
    IOleWindow IOleWindow_iface;
    IServiceProvider IServiceProvider_iface;
    LONG ref;

    IAccessible *parent;
    HWND acc_hwnd;
    HWND ow_hwnd;
    INT role;
    INT state;
    LONG child_count;
    LPCWSTR name;
    LONG left, top, width, height;
    BOOL enable_ia2;
    LONG unique_id;
} Accessible, Accessible2, Accessible_child, Accessible_child2;

static inline struct Accessible* impl_from_Accessible(IAccessible *iface)
{
    return CONTAINING_RECORD(iface, struct Accessible, IAccessible_iface);
}

static HRESULT WINAPI Accessible_QueryInterface(IAccessible *iface, REFIID riid, void **obj)
{
    struct Accessible *This = impl_from_Accessible(iface);

    *obj = NULL;
    if (IsEqualIID(riid, &IID_IAccIdentity))
    {
        if (This == &Accessible2)
            CHECK_EXPECT(Accessible2_QI_IAccIdentity);
        ok(This == &Accessible2, "unexpected call\n");
        return E_NOINTERFACE;
    }

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDispatch) ||
            IsEqualIID(riid, &IID_IAccessible))
        *obj = iface;
    else if (IsEqualIID(riid, &IID_IOleWindow))
        *obj = &This->IOleWindow_iface;
    else if (IsEqualIID(riid, &IID_IServiceProvider))
        *obj = &This->IServiceProvider_iface;
    else if (IsEqualIID(riid, &IID_IAccessible2) && This->enable_ia2)
        *obj = &This->IAccessible2_iface;
    else
        return E_NOINTERFACE;

    IAccessible_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI Accessible_AddRef(IAccessible *iface)
{
    struct Accessible *This = impl_from_Accessible(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI Accessible_Release(IAccessible *iface)
{
    struct Accessible *This = impl_from_Accessible(iface);
    return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI Accessible_GetTypeInfoCount(IAccessible *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_GetTypeInfo(IAccessible *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **out_tinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_GetIDsOfNames(IAccessible *iface, REFIID riid,
        LPOLESTR *rg_names, UINT name_count, LCID lcid, DISPID *rg_disp_id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_Invoke(IAccessible *iface, DISPID disp_id_member,
        REFIID riid, LCID lcid, WORD flags, DISPPARAMS *disp_params,
        VARIANT *var_result, EXCEPINFO *excep_info, UINT *arg_err)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accParent(IAccessible *iface, IDispatch **out_parent)
{
    struct Accessible *This = impl_from_Accessible(iface);

    if (This == &Accessible_child)
        CHECK_EXPECT(Accessible_child_get_accParent);
    else if (This == &Accessible_child2)
        CHECK_EXPECT(Accessible_child2_get_accParent);
    else if (This == &Accessible2)
        CHECK_EXPECT(Accessible2_get_accParent);
    else
        CHECK_EXPECT(Accessible_get_accParent);

    if (This->parent)
        return IAccessible_QueryInterface(This->parent, &IID_IDispatch, (void **)out_parent);

    *out_parent = NULL;
    return S_FALSE;
}

static HRESULT WINAPI Accessible_get_accChildCount(IAccessible *iface, LONG *out_count)
{
    struct Accessible *This = impl_from_Accessible(iface);

    if (This == &Accessible_child)
        CHECK_EXPECT(Accessible_child_get_accChildCount);
    else if (This == &Accessible_child2)
        CHECK_EXPECT(Accessible_child2_get_accChildCount);
    else if (This == &Accessible2)
        CHECK_EXPECT(Accessible2_get_accChildCount);
    else
        CHECK_EXPECT(Accessible_get_accChildCount);

    if (This->child_count)
    {
        *out_count = This->child_count;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accChild(IAccessible *iface, VARIANT child_id,
        IDispatch **out_child)
{
    struct Accessible *This = impl_from_Accessible(iface);

    CHECK_EXPECT(Accessible_get_accChild);
    ok(This == &Accessible, "unexpected call\n");

    *out_child = NULL;
    if (V_VT(&child_id) != VT_I4)
        return E_INVALIDARG;

    if (This == &Accessible)
    {
        switch (V_I4(&child_id))
        {
        case CHILDID_SELF:
            return IAccessible_QueryInterface(&This->IAccessible_iface, &IID_IDispatch, (void **)out_child);

        /* Simple element children. */
        case 1:
        case 3:
            return S_FALSE;

        case 2:
            return IAccessible_QueryInterface(&Accessible_child.IAccessible_iface, &IID_IDispatch, (void **)out_child);

        case 4:
            return IAccessible_QueryInterface(&Accessible_child2.IAccessible_iface, &IID_IDispatch, (void **)out_child);

        default:
            break;

        }
    }
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accName(IAccessible *iface, VARIANT child_id,
        BSTR *out_name)
{
    struct Accessible *This = impl_from_Accessible(iface);

    *out_name = NULL;
    if (This == &Accessible_child)
        CHECK_EXPECT(Accessible_child_get_accName);
    else if (This == &Accessible_child2)
        CHECK_EXPECT(Accessible_child2_get_accName);
    else if (This == &Accessible2)
        CHECK_EXPECT(Accessible2_get_accName);
    else
        CHECK_EXPECT(Accessible_get_accName);

    if (This->name)
    {
        *out_name = SysAllocString(This->name);
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accValue(IAccessible *iface, VARIANT child_id,
        BSTR *out_value)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accDescription(IAccessible *iface, VARIANT child_id,
        BSTR *out_description)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accRole(IAccessible *iface, VARIANT child_id,
        VARIANT *out_role)
{
    struct Accessible *This = impl_from_Accessible(iface);

    if (This == &Accessible_child)
        CHECK_EXPECT(Accessible_child_get_accRole);
    else if (This == &Accessible_child2)
        CHECK_EXPECT(Accessible_child2_get_accRole);
    else if (This == &Accessible2)
        CHECK_EXPECT(Accessible2_get_accRole);
    else
        CHECK_EXPECT(Accessible_get_accRole);

    if (This->role)
    {
        V_VT(out_role) = VT_I4;
        V_I4(out_role) = This->role;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accState(IAccessible *iface, VARIANT child_id,
        VARIANT *out_state)
{
    struct Accessible *This = impl_from_Accessible(iface);

    if (This == &Accessible_child)
        CHECK_EXPECT(Accessible_child_get_accState);
    else if (This == &Accessible_child2)
        CHECK_EXPECT(Accessible_child2_get_accState);
    else if (This == &Accessible2)
        CHECK_EXPECT(Accessible2_get_accState);
    else
        CHECK_EXPECT(Accessible_get_accState);

    if (V_VT(&child_id) != VT_I4)
        return E_INVALIDARG;

    if (This == &Accessible && V_I4(&child_id) != CHILDID_SELF)
    {
        switch (V_I4(&child_id))
        {
        case 1:
            V_VT(out_state) = VT_I4;
            V_I4(out_state) = STATE_SYSTEM_INVISIBLE;
            break;

        case 3:
            V_VT(out_state) = VT_I4;
            V_I4(out_state) = STATE_SYSTEM_FOCUSABLE;
            break;

        default:
            return E_INVALIDARG;
        }

        return S_OK;
    }

    if (This->state)
    {
        V_VT(out_state) = VT_I4;
        V_I4(out_state) = This->state;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accHelp(IAccessible *iface, VARIANT child_id,
        BSTR *out_help)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accHelpTopic(IAccessible *iface,
        BSTR *out_help_file, VARIANT child_id, LONG *out_topic_id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accKeyboardShortcut(IAccessible *iface, VARIANT child_id,
        BSTR *out_kbd_shortcut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accFocus(IAccessible *iface, VARIANT *pchild_id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accSelection(IAccessible *iface, VARIANT *out_selection)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accDefaultAction(IAccessible *iface, VARIANT child_id,
        BSTR *out_default_action)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_accSelect(IAccessible *iface, LONG select_flags,
        VARIANT child_id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_accLocation(IAccessible *iface, LONG *out_left,
        LONG *out_top, LONG *out_width, LONG *out_height, VARIANT child_id)
{
    struct Accessible *This = impl_from_Accessible(iface);

    if (This == &Accessible_child)
        CHECK_EXPECT(Accessible_child_accLocation);
    else if (This == &Accessible_child2)
        CHECK_EXPECT(Accessible_child2_accLocation);
    else if (This == &Accessible2)
        CHECK_EXPECT(Accessible2_accLocation);
    else
        CHECK_EXPECT(Accessible_accLocation);

    if (This->width && This->height)
    {
        *out_left = This->left;
        *out_top = This->top;
        *out_width = This->width;
        *out_height = This->height;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_accNavigate(IAccessible *iface, LONG nav_direction,
        VARIANT child_id_start, VARIANT *out_var)
{
    struct Accessible *This = impl_from_Accessible(iface);

    if (This == &Accessible_child)
        CHECK_EXPECT(Accessible_child_accNavigate);
    else if (This == &Accessible_child2)
        CHECK_EXPECT(Accessible_child2_accNavigate);
    else
        CHECK_EXPECT(Accessible_accNavigate);
    VariantInit(out_var);

    /*
     * This is an undocumented way for UI Automation to get an HWND for
     * IAccessible's contained in a Direct Annotation wrapper object.
     */
    if ((nav_direction == NAVDIR_INTERNAL_HWND) && check_variant_i4(&child_id_start, CHILDID_SELF) &&
            This->acc_hwnd)
    {
        V_VT(out_var) = VT_I4;
        V_I4(out_var) = HandleToUlong(This->acc_hwnd);
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI Accessible_accHitTest(IAccessible *iface, LONG left, LONG top,
        VARIANT *out_child_id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_accDoDefaultAction(IAccessible *iface, VARIANT child_id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_put_accName(IAccessible *iface, VARIANT child_id,
        BSTR name)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_put_accValue(IAccessible *iface, VARIANT child_id,
        BSTR value)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IAccessibleVtbl AccessibleVtbl = {
    Accessible_QueryInterface,
    Accessible_AddRef,
    Accessible_Release,
    Accessible_GetTypeInfoCount,
    Accessible_GetTypeInfo,
    Accessible_GetIDsOfNames,
    Accessible_Invoke,
    Accessible_get_accParent,
    Accessible_get_accChildCount,
    Accessible_get_accChild,
    Accessible_get_accName,
    Accessible_get_accValue,
    Accessible_get_accDescription,
    Accessible_get_accRole,
    Accessible_get_accState,
    Accessible_get_accHelp,
    Accessible_get_accHelpTopic,
    Accessible_get_accKeyboardShortcut,
    Accessible_get_accFocus,
    Accessible_get_accSelection,
    Accessible_get_accDefaultAction,
    Accessible_accSelect,
    Accessible_accLocation,
    Accessible_accNavigate,
    Accessible_accHitTest,
    Accessible_accDoDefaultAction,
    Accessible_put_accName,
    Accessible_put_accValue
};

static inline struct Accessible* impl_from_Accessible2(IAccessible2 *iface)
{
    return CONTAINING_RECORD(iface, struct Accessible, IAccessible2_iface);
}

static HRESULT WINAPI Accessible2_QueryInterface(IAccessible2 *iface, REFIID riid, void **obj)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_QueryInterface(&This->IAccessible_iface, riid, obj);
}

static ULONG WINAPI Accessible2_AddRef(IAccessible2 *iface)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_AddRef(&This->IAccessible_iface);
}

static ULONG WINAPI Accessible2_Release(IAccessible2 *iface)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_Release(&This->IAccessible_iface);
}

static HRESULT WINAPI Accessible2_GetTypeInfoCount(IAccessible2 *iface, UINT *pctinfo)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_GetTypeInfoCount(&This->IAccessible_iface, pctinfo);
}

static HRESULT WINAPI Accessible2_GetTypeInfo(IAccessible2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **out_tinfo)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_GetTypeInfo(&This->IAccessible_iface, iTInfo, lcid, out_tinfo);
}

static HRESULT WINAPI Accessible2_GetIDsOfNames(IAccessible2 *iface, REFIID riid,
        LPOLESTR *rg_names, UINT name_count, LCID lcid, DISPID *rg_disp_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_GetIDsOfNames(&This->IAccessible_iface, riid, rg_names, name_count,
            lcid, rg_disp_id);
}

static HRESULT WINAPI Accessible2_Invoke(IAccessible2 *iface, DISPID disp_id_member,
        REFIID riid, LCID lcid, WORD flags, DISPPARAMS *disp_params,
        VARIANT *var_result, EXCEPINFO *excep_info, UINT *arg_err)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_Invoke(&This->IAccessible_iface, disp_id_member, riid, lcid, flags,
            disp_params, var_result, excep_info, arg_err);
}

static HRESULT WINAPI Accessible2_get_accParent(IAccessible2 *iface, IDispatch **out_parent)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accParent(&This->IAccessible_iface, out_parent);
}

static HRESULT WINAPI Accessible2_get_accChildCount(IAccessible2 *iface, LONG *out_count)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accChildCount(&This->IAccessible_iface, out_count);
}

static HRESULT WINAPI Accessible2_get_accChild(IAccessible2 *iface, VARIANT child_id,
        IDispatch **out_child)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accChild(&This->IAccessible_iface, child_id, out_child);
}

static HRESULT WINAPI Accessible2_get_accName(IAccessible2 *iface, VARIANT child_id,
        BSTR *out_name)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accName(&This->IAccessible_iface, child_id, out_name);
}

static HRESULT WINAPI Accessible2_get_accValue(IAccessible2 *iface, VARIANT child_id,
        BSTR *out_value)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accValue(&This->IAccessible_iface, child_id, out_value);
}

static HRESULT WINAPI Accessible2_get_accDescription(IAccessible2 *iface, VARIANT child_id,
        BSTR *out_description)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accDescription(&This->IAccessible_iface, child_id, out_description);
}

static HRESULT WINAPI Accessible2_get_accRole(IAccessible2 *iface, VARIANT child_id,
        VARIANT *out_role)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accRole(&This->IAccessible_iface, child_id, out_role);
}

static HRESULT WINAPI Accessible2_get_accState(IAccessible2 *iface, VARIANT child_id,
        VARIANT *out_state)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accState(&This->IAccessible_iface, child_id, out_state);
}

static HRESULT WINAPI Accessible2_get_accHelp(IAccessible2 *iface, VARIANT child_id,
        BSTR *out_help)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accHelp(&This->IAccessible_iface, child_id, out_help);
}

static HRESULT WINAPI Accessible2_get_accHelpTopic(IAccessible2 *iface,
        BSTR *out_help_file, VARIANT child_id, LONG *out_topic_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accHelpTopic(&This->IAccessible_iface, out_help_file, child_id,
            out_topic_id);
}

static HRESULT WINAPI Accessible2_get_accKeyboardShortcut(IAccessible2 *iface, VARIANT child_id,
        BSTR *out_kbd_shortcut)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accKeyboardShortcut(&This->IAccessible_iface, child_id,
            out_kbd_shortcut);
}

static HRESULT WINAPI Accessible2_get_accFocus(IAccessible2 *iface, VARIANT *pchild_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accFocus(&This->IAccessible_iface, pchild_id);
}

static HRESULT WINAPI Accessible2_get_accSelection(IAccessible2 *iface, VARIANT *out_selection)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accSelection(&This->IAccessible_iface, out_selection);
}

static HRESULT WINAPI Accessible2_get_accDefaultAction(IAccessible2 *iface, VARIANT child_id,
        BSTR *out_default_action)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accDefaultAction(&This->IAccessible_iface, child_id,
            out_default_action);
}

static HRESULT WINAPI Accessible2_accSelect(IAccessible2 *iface, LONG select_flags,
        VARIANT child_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_accSelect(&This->IAccessible_iface, select_flags, child_id);
}

static HRESULT WINAPI Accessible2_accLocation(IAccessible2 *iface, LONG *out_left,
        LONG *out_top, LONG *out_width, LONG *out_height, VARIANT child_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_accLocation(&This->IAccessible_iface, out_left, out_top, out_width,
            out_height, child_id);
}

static HRESULT WINAPI Accessible2_accNavigate(IAccessible2 *iface, LONG nav_direction,
        VARIANT child_id_start, VARIANT *out_var)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_accNavigate(&This->IAccessible_iface, nav_direction, child_id_start,
            out_var);
}

static HRESULT WINAPI Accessible2_accHitTest(IAccessible2 *iface, LONG left, LONG top,
        VARIANT *out_child_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_accHitTest(&This->IAccessible_iface, left, top, out_child_id);
}

static HRESULT WINAPI Accessible2_accDoDefaultAction(IAccessible2 *iface, VARIANT child_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_accDoDefaultAction(&This->IAccessible_iface, child_id);
}

static HRESULT WINAPI Accessible2_put_accName(IAccessible2 *iface, VARIANT child_id,
        BSTR name)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_put_accName(&This->IAccessible_iface, child_id, name);
}

static HRESULT WINAPI Accessible2_put_accValue(IAccessible2 *iface, VARIANT child_id,
        BSTR value)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_put_accValue(&This->IAccessible_iface, child_id, value);
}

static HRESULT WINAPI Accessible2_get_nRelations(IAccessible2 *iface, LONG *out_nRelations)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_relation(IAccessible2 *iface, LONG relation_idx,
        IAccessibleRelation **out_relation)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_relations(IAccessible2 *iface, LONG count,
        IAccessibleRelation **out_relations, LONG *out_relation_count)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_role(IAccessible2 *iface, LONG *out_role)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_scrollTo(IAccessible2 *iface, enum IA2ScrollType scroll_type)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_scrollToPoint(IAccessible2 *iface,
        enum IA2CoordinateType coordinate_type, LONG x, LONG y)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_groupPosition(IAccessible2 *iface, LONG *out_group_level,
        LONG *out_similar_items_in_group, LONG *out_position_in_group)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_states(IAccessible2 *iface, AccessibleStates *out_states)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_extendedRole(IAccessible2 *iface, BSTR *out_extended_role)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_localizedExtendedRole(IAccessible2 *iface,
        BSTR *out_localized_extended_role)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_nExtendedStates(IAccessible2 *iface, LONG *out_nExtendedStates)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_extendedStates(IAccessible2 *iface, LONG count,
        BSTR **out_extended_states, LONG *out_extended_states_count)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_localizedExtendedStates(IAccessible2 *iface, LONG count,
        BSTR **out_localized_extended_states, LONG *out_localized_extended_states_count)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_uniqueID(IAccessible2 *iface, LONG *out_unique_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);

    if (This == &Accessible2)
        CHECK_EXPECT(Accessible2_get_uniqueID);
    else
        CHECK_EXPECT(Accessible_get_uniqueID);

    *out_unique_id = 0;
    if (This->unique_id)
    {
        *out_unique_id = This->unique_id;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_windowHandle(IAccessible2 *iface, HWND *out_hwnd)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_indexInParent(IAccessible2 *iface, LONG *out_idx_in_parent)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_locale(IAccessible2 *iface, IA2Locale *out_locale)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_attributes(IAccessible2 *iface, BSTR *out_attributes)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IAccessible2Vtbl Accessible2Vtbl = {
    Accessible2_QueryInterface,
    Accessible2_AddRef,
    Accessible2_Release,
    Accessible2_GetTypeInfoCount,
    Accessible2_GetTypeInfo,
    Accessible2_GetIDsOfNames,
    Accessible2_Invoke,
    Accessible2_get_accParent,
    Accessible2_get_accChildCount,
    Accessible2_get_accChild,
    Accessible2_get_accName,
    Accessible2_get_accValue,
    Accessible2_get_accDescription,
    Accessible2_get_accRole,
    Accessible2_get_accState,
    Accessible2_get_accHelp,
    Accessible2_get_accHelpTopic,
    Accessible2_get_accKeyboardShortcut,
    Accessible2_get_accFocus,
    Accessible2_get_accSelection,
    Accessible2_get_accDefaultAction,
    Accessible2_accSelect,
    Accessible2_accLocation,
    Accessible2_accNavigate,
    Accessible2_accHitTest,
    Accessible2_accDoDefaultAction,
    Accessible2_put_accName,
    Accessible2_put_accValue,
    Accessible2_get_nRelations,
    Accessible2_get_relation,
    Accessible2_get_relations,
    Accessible2_role,
    Accessible2_scrollTo,
    Accessible2_scrollToPoint,
    Accessible2_get_groupPosition,
    Accessible2_get_states,
    Accessible2_get_extendedRole,
    Accessible2_get_localizedExtendedRole,
    Accessible2_get_nExtendedStates,
    Accessible2_get_extendedStates,
    Accessible2_get_localizedExtendedStates,
    Accessible2_get_uniqueID,
    Accessible2_get_windowHandle,
    Accessible2_get_indexInParent,
    Accessible2_get_locale,
    Accessible2_get_attributes,
};

static inline struct Accessible* impl_from_OleWindow(IOleWindow *iface)
{
    return CONTAINING_RECORD(iface, struct Accessible, IOleWindow_iface);
}

static HRESULT WINAPI OleWindow_QueryInterface(IOleWindow *iface, REFIID riid, void **obj)
{
    struct Accessible *This = impl_from_OleWindow(iface);
    return IAccessible_QueryInterface(&This->IAccessible_iface, riid, obj);
}

static ULONG WINAPI OleWindow_AddRef(IOleWindow *iface)
{
    struct Accessible *This = impl_from_OleWindow(iface);
    return IAccessible_AddRef(&This->IAccessible_iface);
}

static ULONG WINAPI OleWindow_Release(IOleWindow *iface)
{
    struct Accessible *This = impl_from_OleWindow(iface);
    return IAccessible_Release(&This->IAccessible_iface);
}

static HRESULT WINAPI OleWindow_GetWindow(IOleWindow *iface, HWND *hwnd)
{
    struct Accessible *This = impl_from_OleWindow(iface);

    *hwnd = This->ow_hwnd;
    return *hwnd ? S_OK : E_FAIL;
}

static HRESULT WINAPI OleWindow_ContextSensitiveHelp(IOleWindow *iface, BOOL f_enter_mode)
{
    return E_NOTIMPL;
}

static const IOleWindowVtbl OleWindowVtbl = {
    OleWindow_QueryInterface,
    OleWindow_AddRef,
    OleWindow_Release,
    OleWindow_GetWindow,
    OleWindow_ContextSensitiveHelp
};

static inline struct Accessible* impl_from_ServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, struct Accessible, IServiceProvider_iface);
}

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **obj)
{
    struct Accessible *This = impl_from_ServiceProvider(iface);
    return IAccessible_QueryInterface(&This->IAccessible_iface, riid, obj);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    struct Accessible *This = impl_from_ServiceProvider(iface);
    return IAccessible_AddRef(&This->IAccessible_iface);
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    struct Accessible *This = impl_from_ServiceProvider(iface);
    return IAccessible_Release(&This->IAccessible_iface);
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface, REFGUID service_guid,
        REFIID riid, void **obj)
{
    struct Accessible *This = impl_from_ServiceProvider(iface);

    if (IsEqualIID(riid, &IID_IAccessible2) && IsEqualIID(service_guid, &IID_IAccessible2) &&
            This->enable_ia2)
        return IAccessible_QueryInterface(&This->IAccessible_iface, riid, obj);

    return E_NOTIMPL;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService,
};

static struct Accessible Accessible =
{
    { &AccessibleVtbl },
    { &Accessible2Vtbl },
    { &OleWindowVtbl },
    { &ServiceProviderVtbl },
    1,
    NULL,
    0, 0,
    0, 0, 0, NULL,
    0, 0, 0, 0,
    FALSE, 0,
};

static struct Accessible Accessible2 =
{
    { &AccessibleVtbl },
    { &Accessible2Vtbl },
    { &OleWindowVtbl },
    { &ServiceProviderVtbl },
    1,
    NULL,
    0, 0,
    0, 0, 0, NULL,
    0, 0, 0, 0,
    FALSE, 0,
};

static struct Accessible Accessible_child =
{
    { &AccessibleVtbl },
    { &Accessible2Vtbl },
    { &OleWindowVtbl },
    { &ServiceProviderVtbl },
    1,
    &Accessible.IAccessible_iface,
    0, 0,
    0, 0, 0, NULL,
    0, 0, 0, 0,
    FALSE, 0,
};

static struct Accessible Accessible_child2 =
{
    { &AccessibleVtbl },
    { &Accessible2Vtbl },
    { &OleWindowVtbl },
    { &ServiceProviderVtbl },
    1,
    &Accessible.IAccessible_iface,
    0, 0,
    0, 0, 0, NULL,
    0, 0, 0, 0,
    FALSE, 0,
};

static struct Provider
{
    IRawElementProviderSimple IRawElementProviderSimple_iface;
    IRawElementProviderFragment IRawElementProviderFragment_iface;
    IRawElementProviderFragmentRoot IRawElementProviderFragmentRoot_iface;
    LONG ref;

    const char *prov_name;
    IRawElementProviderFragment *parent;
    IRawElementProviderFragmentRoot *frag_root;
    enum ProviderOptions prov_opts;
    HWND hwnd;
    BOOL ret_invalid_prop_type;
    DWORD expected_tid;
    int runtime_id[2];
    DWORD last_call_tid;
} Provider, Provider2, Provider_child, Provider_child2;

static const WCHAR *uia_bstr_prop_str = L"uia-string";
static const ULONG uia_i4_prop_val = 0xdeadbeef;
static const ULONG uia_i4_arr_prop_val[] = { 0xfeedbeef, 0xdeadcafe, 0xfefedede };
static const double uia_r8_prop_val = 128.256f;
static const double uia_r8_arr_prop_val[] = { 2.4, 8.16, 32.64 };
static const IRawElementProviderSimple *uia_unk_arr_prop_val[] = { &Provider_child.IRawElementProviderSimple_iface,
                                                                   &Provider_child2.IRawElementProviderSimple_iface };
static SAFEARRAY *create_i4_safearray(void)
{
    SAFEARRAY *sa;
    LONG idx;

    if (!(sa = SafeArrayCreateVector(VT_I4, 0, ARRAY_SIZE(uia_i4_arr_prop_val))))
        return NULL;

    for (idx = 0; idx < ARRAY_SIZE(uia_i4_arr_prop_val); idx++)
        SafeArrayPutElement(sa, &idx, (void *)&uia_i4_arr_prop_val[idx]);

    return sa;
}

static SAFEARRAY *create_r8_safearray(void)
{
    SAFEARRAY *sa;
    LONG idx;

    if (!(sa = SafeArrayCreateVector(VT_R8, 0, ARRAY_SIZE(uia_r8_arr_prop_val))))
        return NULL;

    for (idx = 0; idx < ARRAY_SIZE(uia_r8_arr_prop_val); idx++)
        SafeArrayPutElement(sa, &idx, (void *)&uia_r8_arr_prop_val[idx]);

    return sa;
}

static SAFEARRAY *create_unk_safearray(void)
{
    SAFEARRAY *sa;
    LONG idx;

    if (!(sa = SafeArrayCreateVector(VT_UNKNOWN, 0, ARRAY_SIZE(uia_unk_arr_prop_val))))
        return NULL;

    for (idx = 0; idx < ARRAY_SIZE(uia_unk_arr_prop_val); idx++)
        SafeArrayPutElement(sa, &idx, (void *)uia_unk_arr_prop_val[idx]);

    return sa;
}

enum {
    PROV_GET_PROVIDER_OPTIONS,
    PROV_GET_PROPERTY_VALUE,
    PROV_GET_HOST_RAW_ELEMENT_PROVIDER,
    FRAG_NAVIGATE,
    FRAG_GET_RUNTIME_ID,
    FRAG_GET_FRAGMENT_ROOT,
};

static const char *prov_method_str[] = {
    "get_ProviderOptions",
    "GetPropertyValue",
    "get_HostRawElementProvider",
    "Navigate",
    "GetRuntimeId",
    "get_FragmentRoot",
};

static const char *get_prov_method_str(int method)
{
    if (method >= ARRAY_SIZE(prov_method_str))
        return NULL;
    else
        return prov_method_str[method];
}

enum {
    METHOD_OPTIONAL = 0x01,
    METHOD_TODO = 0x02,
};

struct prov_method_sequence {
    struct Provider *prov;
    int method;
    int flags;
};

static int sequence_cnt, sequence_size;
static struct prov_method_sequence *sequence;

static void flush_method_sequence(void)
{
    HeapFree(GetProcessHeap(), 0, sequence);
    sequence = NULL;
    sequence_cnt = sequence_size = 0;
}

static void add_method_call(struct Provider *prov, int method)
{
    struct prov_method_sequence prov_method = {0};

    if (!sequence)
    {
	sequence_size = 10;
	sequence = HeapAlloc(GetProcessHeap(), 0, sequence_size * sizeof(*sequence));
    }
    if (sequence_cnt == sequence_size)
    {
	sequence_size *= 2;
	sequence = HeapReAlloc(GetProcessHeap(), 0, sequence, sequence_size * sizeof(*sequence));
    }

    prov_method.prov = prov;
    prov_method.method = method;
    prov_method.flags = 0;
    sequence[sequence_cnt++] = prov_method;
}

#define ok_method_sequence( exp, context ) \
        ok_method_sequence_( (exp), (context), __FILE__, __LINE__)
static void ok_method_sequence_(const struct prov_method_sequence *expected_list, const char *context,
        const char *file, int line)
{
    const struct prov_method_sequence *expected = expected_list;
    const struct prov_method_sequence *actual;
    unsigned int count = 0;

    add_method_call(NULL, 0);
    actual = sequence;

    if (context)
        winetest_push_context("%s", context);

    while (expected->prov && actual->prov)
    {
	if (expected->prov == actual->prov && expected->method == actual->method)
	{
            if (expected->flags & METHOD_TODO)
                todo_wine ok_(file, line)(1, "%d: expected %s_%s, got %s_%s\n", count, expected->prov->prov_name,
                        get_prov_method_str(expected->method), actual->prov->prov_name, get_prov_method_str(actual->method));
            expected++;
            actual++;
        }
        else if (expected->flags & METHOD_TODO)
        {
            todo_wine ok_(file, line)(0, "%d: expected %s_%s, got %s_%s\n", count, expected->prov->prov_name,
                    get_prov_method_str(expected->method), actual->prov->prov_name, get_prov_method_str(actual->method));
	    expected++;
        }
        else if (expected->flags & METHOD_OPTIONAL)
	    expected++;
        else
        {
            ok_(file, line)(0, "%d: expected %s_%s, got %s_%s\n", count, expected->prov->prov_name,
                    get_prov_method_str(expected->method), actual->prov->prov_name, get_prov_method_str(actual->method));
            expected++;
            actual++;
        }
        count++;
    }

    /* Handle trailing optional/todo_wine methods. */
    while (expected->prov && ((expected->flags & METHOD_OPTIONAL) ||
                ((expected->flags & METHOD_TODO) && !strcmp(winetest_platform, "wine"))))
    {
        if (expected->flags & METHOD_TODO)
            todo_wine ok_(file, line)(0, "%d: expected %s_%s\n", count, expected->prov->prov_name,
                    get_prov_method_str(expected->method));
        count++;
	expected++;
    }

    if (expected->prov || actual->prov)
    {
        if (expected->prov)
            ok_( file, line)(0, "incomplete sequence: expected %s_%s, got nothing\n", expected->prov->prov_name,
                    get_prov_method_str(expected->method));
        else
            ok_( file, line)(0, "incomplete sequence: expected nothing, got %s_%s\n", actual->prov->prov_name,
                    get_prov_method_str(actual->method));
    }

    if (context)
        winetest_pop_context();

    flush_method_sequence();
}

/*
 * Parsing the string returned by UIA_ProviderDescriptionPropertyId is
 * the only way to know what an HUIANODE represents internally. It
 * returns a formatted string which always starts with:
 * "[pid:<process-id>,providerId:0x<hwnd-ptr> "
 * On Windows versions 10v1507 and below, "providerId:" is "hwnd:"
 *
 * This is followed by strings for each provider it represents. These are
 * formatted as:
 * "<prov-type>:<prov-desc> (<origin>)"
 * and are terminated with ";", the final provider has no ";" terminator,
 * instead it has "]".
 *
 * If the given provider is the one used for navigation towards a parent, it has
 * "(parent link)" as a suffix on "<prov-type>".
 *
 * <prov-type> is one of "Annotation", "Main", "Override", "Hwnd", or
 * "Nonclient".
 *
 * <prov-desc> is the string returned from calling GetPropertyValue on the
 * IRawElementProviderSimple being represented with a property ID of
 * UIA_ProviderDescriptionPropertyId.
 *
 * <origin> is the name of the module that the
 * IRawElementProviderSimple comes from. For unmanaged code, it's:
 * "unmanaged:<executable>"
 * and for managed code, it's:
 * "managed:<assembly-qualified-name>"
 *
 * An example:
 * [pid:1500,providerId:0x2F054C Main:Provider (unmanaged:uiautomation_test.exe); Hwnd(parent link):HWND Proxy (unmanaged:uiautomationcore.dll)]
 */
static BOOL get_provider_desc(BSTR prov_desc, const WCHAR *prov_type, WCHAR *out_name)
{
    const WCHAR *str, *str2;

    str = wcsstr(prov_desc, prov_type);
    if (!str)
        return FALSE;

    if (!out_name)
        return TRUE;

    str += wcslen(prov_type);
    str2 = wcschr(str, L'(');
    lstrcpynW(out_name, str, ((str2 - str)));

    return TRUE;
}

#define check_node_provider_desc( prov_desc, prov_type, prov_name, parent_link ) \
        check_node_provider_desc_( (prov_desc), (prov_type), (prov_name), (parent_link), __FILE__, __LINE__)
static void check_node_provider_desc_(BSTR prov_desc, const WCHAR *prov_type, const WCHAR *prov_name,
        BOOL parent_link, const char *file, int line)
{
    WCHAR buf[2048];

    if (parent_link)
        wsprintfW(buf, L"%s(parent link):", prov_type);
    else
        wsprintfW(buf, L"%s:", prov_type);

    if (!get_provider_desc(prov_desc, buf, buf))
    {
        if (parent_link)
            wsprintfW(buf, L"%s:", prov_type);
        else
            wsprintfW(buf, L"%s(parent link):", prov_type);

        if (!get_provider_desc(prov_desc, buf, buf))
        {
            ok_(file, line)(0, "failed to get provider string for %s\n", debugstr_w(prov_type));
            return;
        }
        else
        {
            if (parent_link)
                ok_(file, line)(0, "expected parent link provider %s\n", debugstr_w(prov_type));
            else
                ok_(file, line)(0, "unexpected parent link provider %s\n", debugstr_w(prov_type));
        }
    }

    if (prov_name)
        ok_(file, line)(!wcscmp(prov_name, buf), "unexpected provider name %s\n", debugstr_w(buf));
}

#define check_node_provider_desc_prefix( prov_desc, pid, prov_id ) \
        check_node_provider_desc_prefix_( (prov_desc), (pid), (prov_id), __FILE__, __LINE__)
static void check_node_provider_desc_prefix_(BSTR prov_desc, DWORD pid, HWND prov_id, const char *file, int line)
{
    const WCHAR *str, *str2;
    WCHAR buf[128];
    DWORD prov_pid;
    HWND prov_hwnd;
    WCHAR *end;

    str = wcsstr(prov_desc, L"pid:");
    str += wcslen(L"pid:");
    str2 = wcschr(str, L',');
    lstrcpynW(buf, str, (str2 - str) + 1);
    prov_pid = wcstoul(buf, &end, 10);
    ok_(file, line)(prov_pid == pid, "Unexpected pid %lu\n", prov_pid);

    str = wcsstr(prov_desc, L"providerId:");
    if (str)
        str += wcslen(L"providerId:");
    else
    {
        str = wcsstr(prov_desc, L"hwnd:");
        str += wcslen(L"hwnd:");
    }
    str2 = wcschr(str, L' ');
    lstrcpynW(buf, str, (str2 - str) + 1);
    prov_hwnd = ULongToHandle(wcstoul(buf, &end, 16));
    ok_(file, line)(prov_hwnd == prov_id, "Unexpected hwnd %p\n", prov_hwnd);
}

/*
 * For node providers that come from an HWND belonging to another process
 * or another thread, the provider is considered 'nested', a node in a node.
 */
static BOOL get_nested_provider_desc(BSTR prov_desc, const WCHAR *prov_type, BOOL parent_link, WCHAR *out_desc)
{
    const WCHAR *str, *str2;
    WCHAR buf[1024];

    if (!parent_link)
        wsprintfW(buf, L"%s:Nested ", prov_type);
    else
        wsprintfW(buf, L"%s(parent link):Nested ", prov_type);
    str = wcsstr(prov_desc, buf);
    /* Check with and without parent-link. */
    if (!str)
        return FALSE;

    if (!out_desc)
        return TRUE;

    str += wcslen(buf);
    str2 = wcschr(str, L']');
    /* We want to include the ']' character, so + 2. */
    lstrcpynW(out_desc, str, ((str2 - str) + 2));

    return TRUE;
}

#define check_runtime_id( exp_runtime_id, exp_size, runtime_id ) \
        check_runtime_id_( (exp_runtime_id), (exp_size), (runtime_id), __FILE__, __LINE__)
static void check_runtime_id_(int *exp_runtime_id, int exp_size, SAFEARRAY *runtime_id, const char *file, int line)
{
    LONG i, idx, lbound, ubound, elems;
    HRESULT hr;
    UINT dims;
    int val;

    dims = SafeArrayGetDim(runtime_id);
    ok_(file, line)(dims == 1, "Unexpected array dims %d\n", dims);

    hr = SafeArrayGetLBound(runtime_id, 1, &lbound);
    ok_(file, line)(hr == S_OK, "Failed to get LBound with hr %#lx\n", hr);

    hr = SafeArrayGetUBound(runtime_id, 1, &ubound);
    ok_(file, line)(hr == S_OK, "Failed to get UBound with hr %#lx\n", hr);

    elems = (ubound - lbound) + 1;
    ok_(file, line)(exp_size == elems, "Unexpected runtime_id array size %#lx\n", elems);

    for (i = 0; i < elems; i++)
    {
        idx = lbound + i;
        hr = SafeArrayGetElement(runtime_id, &idx, &val);
        ok_(file, line)(hr == S_OK, "Failed to get element with hr %#lx\n", hr);
        ok_(file, line)(val == exp_runtime_id[i], "Unexpected runtime_id[%ld] %#x\n", i, val);
    }
}

static inline struct Provider *impl_from_ProviderSimple(IRawElementProviderSimple *iface)
{
    return CONTAINING_RECORD(iface, struct Provider, IRawElementProviderSimple_iface);
}

HRESULT WINAPI ProviderSimple_QueryInterface(IRawElementProviderSimple *iface, REFIID riid, void **ppv)
{
    struct Provider *This = impl_from_ProviderSimple(iface);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IRawElementProviderSimple) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IRawElementProviderFragment))
        *ppv = &This->IRawElementProviderFragment_iface;
    else if (IsEqualIID(riid, &IID_IRawElementProviderFragmentRoot))
        *ppv = &This->IRawElementProviderFragmentRoot_iface;
    else
        return E_NOINTERFACE;

    IRawElementProviderSimple_AddRef(iface);
    return S_OK;
}

ULONG WINAPI ProviderSimple_AddRef(IRawElementProviderSimple *iface)
{
    struct Provider *This = impl_from_ProviderSimple(iface);
    return InterlockedIncrement(&This->ref);
}

ULONG WINAPI ProviderSimple_Release(IRawElementProviderSimple *iface)
{
    struct Provider *This = impl_from_ProviderSimple(iface);
    return InterlockedDecrement(&This->ref);
}

HRESULT WINAPI ProviderSimple_get_ProviderOptions(IRawElementProviderSimple *iface,
        enum ProviderOptions *ret_val)
{
    struct Provider *This = impl_from_ProviderSimple(iface);

    add_method_call(This, PROV_GET_PROVIDER_OPTIONS);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();

    *ret_val = 0;
    if (This->prov_opts)
    {
        *ret_val = This->prov_opts;
        return S_OK;
    }

    return E_NOTIMPL;
}

HRESULT WINAPI ProviderSimple_GetPatternProvider(IRawElementProviderSimple *iface,
        PATTERNID pattern_id, IUnknown **ret_val)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

HRESULT WINAPI ProviderSimple_GetPropertyValue(IRawElementProviderSimple *iface,
        PROPERTYID prop_id, VARIANT *ret_val)
{
    struct Provider *This = impl_from_ProviderSimple(iface);

    add_method_call(This, PROV_GET_PROPERTY_VALUE);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();

    VariantInit(ret_val);
    switch (prop_id)
    {
    case UIA_NativeWindowHandlePropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_R8;
            V_R8(ret_val) = uia_r8_prop_val;
        }
        else
        {
            V_VT(ret_val) = VT_I4;
            V_I4(ret_val) = HandleToULong(This->hwnd);
        }
        break;

    case UIA_ProcessIdPropertyId:
    case UIA_ControlTypePropertyId:
    case UIA_CulturePropertyId:
    case UIA_OrientationPropertyId:
    case UIA_LiveSettingPropertyId:
    case UIA_PositionInSetPropertyId:
    case UIA_SizeOfSetPropertyId:
    case UIA_LevelPropertyId:
    case UIA_LandmarkTypePropertyId:
    case UIA_FillColorPropertyId:
    case UIA_FillTypePropertyId:
    case UIA_VisualEffectsPropertyId:
    case UIA_HeadingLevelPropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_R8;
            V_R8(ret_val) = uia_r8_prop_val;
        }
        else
        {
            V_VT(ret_val) = VT_I4;
            V_I4(ret_val) = uia_i4_prop_val;
        }
        break;

    case UIA_RotationPropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_I4;
            V_I4(ret_val) = uia_i4_prop_val;
        }
        else
        {
            V_VT(ret_val) = VT_R8;
            V_R8(ret_val) = uia_r8_prop_val;
        }
        break;

    case UIA_LocalizedControlTypePropertyId:
    case UIA_NamePropertyId:
    case UIA_AcceleratorKeyPropertyId:
    case UIA_AccessKeyPropertyId:
    case UIA_AutomationIdPropertyId:
    case UIA_ClassNamePropertyId:
    case UIA_HelpTextPropertyId:
    case UIA_ItemTypePropertyId:
    case UIA_FrameworkIdPropertyId:
    case UIA_ItemStatusPropertyId:
    case UIA_AriaRolePropertyId:
    case UIA_AriaPropertiesPropertyId:
    case UIA_LocalizedLandmarkTypePropertyId:
    case UIA_FullDescriptionPropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_I4;
            V_I4(ret_val) = uia_i4_prop_val;
        }
        else
        {
            V_VT(ret_val) = VT_BSTR;
            V_BSTR(ret_val) = SysAllocString(uia_bstr_prop_str);
        }
        break;

    case UIA_HasKeyboardFocusPropertyId:
    case UIA_IsKeyboardFocusablePropertyId:
    case UIA_IsEnabledPropertyId:
    case UIA_IsControlElementPropertyId:
    case UIA_IsContentElementPropertyId:
    case UIA_IsPasswordPropertyId:
    case UIA_IsOffscreenPropertyId:
    case UIA_IsRequiredForFormPropertyId:
    case UIA_IsDataValidForFormPropertyId:
    case UIA_OptimizeForVisualContentPropertyId:
    case UIA_IsPeripheralPropertyId:
    case UIA_IsDialogPropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_R8;
            V_R8(ret_val) = uia_r8_prop_val;
        }
        else
        {
            V_VT(ret_val) = VT_BOOL;
            V_BOOL(ret_val) = VARIANT_TRUE;
        }
        break;

    case UIA_AnnotationTypesPropertyId:
    case UIA_OutlineColorPropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_ARRAY | VT_R8;
            V_ARRAY(ret_val) = create_r8_safearray();
        }
        else
        {
            V_VT(ret_val) = VT_ARRAY | VT_I4;
            V_ARRAY(ret_val) = create_i4_safearray();
        }
        break;

    case UIA_OutlineThicknessPropertyId:
    case UIA_SizePropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_ARRAY | VT_I4;
            V_ARRAY(ret_val) = create_i4_safearray();
        }
        else
        {
            V_VT(ret_val) = VT_ARRAY | VT_R8;
            V_ARRAY(ret_val) = create_r8_safearray();
        }
        break;

    case UIA_LabeledByPropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_I4;
            V_I4(ret_val) = uia_i4_prop_val;
        }
        else
        {
            V_VT(ret_val) = VT_UNKNOWN;
            V_UNKNOWN(ret_val) = (IUnknown *)&Provider_child.IRawElementProviderSimple_iface;
            IUnknown_AddRef(V_UNKNOWN(ret_val));
        }
        break;

    case UIA_AnnotationObjectsPropertyId:
    case UIA_DescribedByPropertyId:
    case UIA_FlowsFromPropertyId:
    case UIA_FlowsToPropertyId:
    case UIA_ControllerForPropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_ARRAY | VT_I4;
            V_ARRAY(ret_val) = create_i4_safearray();
        }
        else
        {
            V_VT(ret_val) = VT_UNKNOWN | VT_ARRAY;
            V_ARRAY(ret_val) = create_unk_safearray();
        }
        break;

    case UIA_ProviderDescriptionPropertyId:
    {
        WCHAR buf[1024] = {};

        mbstowcs(buf, This->prov_name, strlen(This->prov_name));
        V_VT(ret_val) = VT_BSTR;
        V_BSTR(ret_val) = SysAllocString(buf);
        break;
    }

    default:
        break;
    }

    return S_OK;
}

HRESULT WINAPI ProviderSimple_get_HostRawElementProvider(IRawElementProviderSimple *iface,
        IRawElementProviderSimple **ret_val)
{
    struct Provider *This = impl_from_ProviderSimple(iface);

    add_method_call(This, PROV_GET_HOST_RAW_ELEMENT_PROVIDER);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();

    *ret_val = NULL;
    if (This->hwnd)
        return UiaHostProviderFromHwnd(This->hwnd, ret_val);

    return S_OK;
}

IRawElementProviderSimpleVtbl ProviderSimpleVtbl = {
    ProviderSimple_QueryInterface,
    ProviderSimple_AddRef,
    ProviderSimple_Release,
    ProviderSimple_get_ProviderOptions,
    ProviderSimple_GetPatternProvider,
    ProviderSimple_GetPropertyValue,
    ProviderSimple_get_HostRawElementProvider,
};

static inline struct Provider *impl_from_ProviderFragment(IRawElementProviderFragment *iface)
{
    return CONTAINING_RECORD(iface, struct Provider, IRawElementProviderFragment_iface);
}

static HRESULT WINAPI ProviderFragment_QueryInterface(IRawElementProviderFragment *iface, REFIID riid,
        void **ppv)
{
    struct Provider *Provider = impl_from_ProviderFragment(iface);
    return IRawElementProviderSimple_QueryInterface(&Provider->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI ProviderFragment_AddRef(IRawElementProviderFragment *iface)
{
    struct Provider *Provider = impl_from_ProviderFragment(iface);
    return IRawElementProviderSimple_AddRef(&Provider->IRawElementProviderSimple_iface);
}

static ULONG WINAPI ProviderFragment_Release(IRawElementProviderFragment *iface)
{
    struct Provider *Provider = impl_from_ProviderFragment(iface);
    return IRawElementProviderSimple_Release(&Provider->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI ProviderFragment_Navigate(IRawElementProviderFragment *iface,
        enum NavigateDirection direction, IRawElementProviderFragment **ret_val)
{
    struct Provider *This = impl_from_ProviderFragment(iface);

    add_method_call(This, FRAG_NAVIGATE);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();

    *ret_val = NULL;
    if ((direction == NavigateDirection_Parent) && This->parent)
    {
        *ret_val = This->parent;
        IRawElementProviderFragment_AddRef(This->parent);
    }
    return S_OK;
}

static HRESULT WINAPI ProviderFragment_GetRuntimeId(IRawElementProviderFragment *iface,
        SAFEARRAY **ret_val)
{
    struct Provider *This = impl_from_ProviderFragment(iface);

    add_method_call(This, FRAG_GET_RUNTIME_ID);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();

    *ret_val = NULL;
    if (This->runtime_id[0] || This->runtime_id[1])
    {
        SAFEARRAY *sa;
        LONG idx;

        if (!(sa = SafeArrayCreateVector(VT_I4, 0, ARRAY_SIZE(This->runtime_id))))
            return E_FAIL;

        for (idx = 0; idx < ARRAY_SIZE(This->runtime_id); idx++)
            SafeArrayPutElement(sa, &idx, (void *)&This->runtime_id[idx]);

        *ret_val = sa;
    }

    return S_OK;
}

static HRESULT WINAPI ProviderFragment_get_BoundingRectangle(IRawElementProviderFragment *iface,
        struct UiaRect *ret_val)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderFragment_GetEmbeddedFragmentRoots(IRawElementProviderFragment *iface,
        SAFEARRAY **ret_val)
{
    ok(0, "unexpected call\n");
    *ret_val = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderFragment_SetFocus(IRawElementProviderFragment *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderFragment_get_FragmentRoot(IRawElementProviderFragment *iface,
        IRawElementProviderFragmentRoot **ret_val)
{
    struct Provider *This = impl_from_ProviderFragment(iface);

    add_method_call(This, FRAG_GET_FRAGMENT_ROOT);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();

    *ret_val = NULL;
    if (This->frag_root)
    {
        *ret_val = This->frag_root;
        IRawElementProviderFragmentRoot_AddRef(This->frag_root);
    }

    return S_OK;
}

static const IRawElementProviderFragmentVtbl ProviderFragmentVtbl = {
    ProviderFragment_QueryInterface,
    ProviderFragment_AddRef,
    ProviderFragment_Release,
    ProviderFragment_Navigate,
    ProviderFragment_GetRuntimeId,
    ProviderFragment_get_BoundingRectangle,
    ProviderFragment_GetEmbeddedFragmentRoots,
    ProviderFragment_SetFocus,
    ProviderFragment_get_FragmentRoot,
};

static inline struct Provider *impl_from_ProviderFragmentRoot(IRawElementProviderFragmentRoot *iface)
{
    return CONTAINING_RECORD(iface, struct Provider, IRawElementProviderFragmentRoot_iface);
}

static HRESULT WINAPI ProviderFragmentRoot_QueryInterface(IRawElementProviderFragmentRoot *iface, REFIID riid,
        void **ppv)
{
    struct Provider *Provider = impl_from_ProviderFragmentRoot(iface);
    return IRawElementProviderSimple_QueryInterface(&Provider->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI ProviderFragmentRoot_AddRef(IRawElementProviderFragmentRoot *iface)
{
    struct Provider *Provider = impl_from_ProviderFragmentRoot(iface);
    return IRawElementProviderSimple_AddRef(&Provider->IRawElementProviderSimple_iface);
}

static ULONG WINAPI ProviderFragmentRoot_Release(IRawElementProviderFragmentRoot *iface)
{
    struct Provider *Provider = impl_from_ProviderFragmentRoot(iface);
    return IRawElementProviderSimple_Release(&Provider->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI ProviderFragmentRoot_ElementProviderFromPoint(IRawElementProviderFragmentRoot *iface,
        double x, double y, IRawElementProviderFragment **ret_val)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderFragmentRoot_GetFocus(IRawElementProviderFragmentRoot *iface,
        IRawElementProviderFragment **ret_val)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IRawElementProviderFragmentRootVtbl ProviderFragmentRootVtbl = {
    ProviderFragmentRoot_QueryInterface,
    ProviderFragmentRoot_AddRef,
    ProviderFragmentRoot_Release,
    ProviderFragmentRoot_ElementProviderFromPoint,
    ProviderFragmentRoot_GetFocus,
};

static struct Provider Provider =
{
    { &ProviderSimpleVtbl },
    { &ProviderFragmentVtbl },
    { &ProviderFragmentRootVtbl },
    1,
    "Provider",
    NULL, NULL,
    0, 0, 0,
};

static struct Provider Provider2 =
{
    { &ProviderSimpleVtbl },
    { &ProviderFragmentVtbl },
    { &ProviderFragmentRootVtbl },
    1,
    "Provider2",
    NULL, NULL,
    0, 0, 0,
};

static struct Provider Provider_child =
{
    { &ProviderSimpleVtbl },
    { &ProviderFragmentVtbl },
    { &ProviderFragmentRootVtbl },
    1,
    "Provider_child",
    &Provider.IRawElementProviderFragment_iface, &Provider.IRawElementProviderFragmentRoot_iface,
    ProviderOptions_ServerSideProvider, 0, 0,
};

static struct Provider Provider_child2 =
{
    { &ProviderSimpleVtbl },
    { &ProviderFragmentVtbl },
    { &ProviderFragmentRootVtbl },
    1,
    "Provider_child2",
    &Provider.IRawElementProviderFragment_iface, &Provider.IRawElementProviderFragmentRoot_iface,
    ProviderOptions_ServerSideProvider, 0, 0,
};

static IAccessible *acc_client;
static IRawElementProviderSimple *prov_root;
static LRESULT WINAPI test_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_GETOBJECT:
        if (lParam == (DWORD)OBJID_CLIENT)
        {
            CHECK_EXPECT(winproc_GETOBJECT_CLIENT);
            if (acc_client)
                return LresultFromObject(&IID_IAccessible, wParam, (IUnknown *)acc_client);

            break;
        }
        else if (lParam == UiaRootObjectId)
        {
            CHECK_EXPECT(winproc_GETOBJECT_UiaRoot);
            if (prov_root)
                return UiaReturnRawElementProvider(hwnd, wParam, lParam, prov_root);

            break;
        }

        break;

    default:
        break;
    }

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

static void test_UiaHostProviderFromHwnd(void)
{
    IRawElementProviderSimple *p, *p2;
    enum ProviderOptions prov_opt;
    WNDCLASSA cls;
    HRESULT hr;
    HWND hwnd;
    VARIANT v;
    int i;

    cls.style = 0;
    cls.lpfnWndProc = test_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = NULL;
    cls.hbrBackground = NULL;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "HostProviderFromHwnd class";

    RegisterClassA(&cls);

    hwnd = CreateWindowExA(0, "HostProviderFromHwnd class", "Test window 1",
            WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
            0, 0, 100, 100, GetDesktopWindow(), NULL, GetModuleHandleA(NULL), NULL);
    ok(hwnd != NULL, "Failed to create a test window.\n");

    p = (void *)0xdeadbeef;
    hr = UiaHostProviderFromHwnd(NULL, &p);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(p == NULL, "Unexpected instance.\n");

    hr = UiaHostProviderFromHwnd(hwnd, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    p = NULL;
    hr = UiaHostProviderFromHwnd(hwnd, &p);
    ok(hr == S_OK, "Failed to get host provider, hr %#lx.\n", hr);

    p2 = NULL;
    hr = UiaHostProviderFromHwnd(hwnd, &p2);
    ok(hr == S_OK, "Failed to get host provider, hr %#lx.\n", hr);
    ok(p != p2, "Unexpected instance.\n");
    IRawElementProviderSimple_Release(p2);

    hr = IRawElementProviderSimple_get_HostRawElementProvider(p, &p2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(p2 == NULL, "Unexpected instance.\n");

    hr = IRawElementProviderSimple_GetPropertyValue(p, UIA_NativeWindowHandlePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == HandleToUlong(hwnd), "V_I4(&v) = %#lx, expected %#lx\n", V_I4(&v), HandleToUlong(hwnd));

    hr = IRawElementProviderSimple_GetPropertyValue(p, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BSTR, "V_VT(&v) = %d\n", V_VT(&v));
    VariantClear(&v);

    /* No patterns are implemented on the HWND Host provider. */
    for (i = UIA_InvokePatternId; i < (UIA_CustomNavigationPatternId + 1); i++)
    {
        IUnknown *unk;

        unk = (void *)0xdeadbeef;
        hr = IRawElementProviderSimple_GetPatternProvider(p, i, &unk);
        ok(hr == S_OK, "Unexpected hr %#lx, %d.\n", hr, i);
        ok(!unk, "Pattern %d returned %p\n", i, unk);
    }

    hr = IRawElementProviderSimple_get_ProviderOptions(p, &prov_opt);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok((prov_opt == ProviderOptions_ServerSideProvider) ||
            broken(prov_opt == ProviderOptions_ClientSideProvider), /* Windows < 10 1507 */
            "Unexpected provider options %#x\n", prov_opt);

    /* Test behavior post Window destruction. */
    DestroyWindow(hwnd);

    hr = IRawElementProviderSimple_GetPropertyValue(p, UIA_NativeWindowHandlePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == HandleToUlong(hwnd), "V_I4(&v) = %#lx, expected %#lx\n", V_I4(&v), HandleToUlong(hwnd));

    hr = IRawElementProviderSimple_GetPropertyValue(p, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BSTR, "V_VT(&v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hr = IRawElementProviderSimple_get_ProviderOptions(p, &prov_opt);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok((prov_opt == ProviderOptions_ServerSideProvider) ||
            broken(prov_opt == ProviderOptions_ClientSideProvider), /* Windows < 10 1507 */
            "Unexpected provider options %#x\n", prov_opt);

    IRawElementProviderSimple_Release(p);

    UnregisterClassA("HostProviderFromHwnd class", NULL);
}

static DWORD WINAPI uia_reserved_val_iface_marshal_thread(LPVOID param)
{
    IStream **stream = param;
    IUnknown *unk_ns, *unk_ns2, *unk_ma, *unk_ma2;
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoGetInterfaceAndReleaseStream(stream[0], &IID_IUnknown, (void **)&unk_ns);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoGetInterfaceAndReleaseStream(stream[1], &IID_IUnknown, (void **)&unk_ma);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = UiaGetReservedNotSupportedValue(&unk_ns2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = UiaGetReservedMixedAttributeValue(&unk_ma2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(unk_ns2 == unk_ns, "UiaGetReservedNotSupported pointer mismatch, unk_ns2 %p, unk_ns %p\n", unk_ns2, unk_ns);
    ok(unk_ma2 == unk_ma, "UiaGetReservedMixedAttribute pointer mismatch, unk_ma2 %p, unk_ma %p\n", unk_ma2, unk_ma);

    CoUninitialize();

    return 0;
}

static void test_uia_reserved_value_ifaces(void)
{
    IUnknown *unk_ns, *unk_ns2, *unk_ma, *unk_ma2;
    IStream *stream[2];
    IMarshal *marshal;
    HANDLE thread;
    ULONG refcnt;
    HRESULT hr;

    /* ReservedNotSupportedValue. */
    hr = UiaGetReservedNotSupportedValue(NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = UiaGetReservedNotSupportedValue(&unk_ns);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk_ns != NULL, "UiaGetReservedNotSupportedValue returned NULL interface.\n");

    refcnt = IUnknown_AddRef(unk_ns);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    refcnt = IUnknown_AddRef(unk_ns);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    refcnt = IUnknown_Release(unk_ns);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    hr = UiaGetReservedNotSupportedValue(&unk_ns2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk_ns2 != NULL, "UiaGetReservedNotSupportedValue returned NULL interface.");
    ok(unk_ns2 == unk_ns, "UiaGetReservedNotSupported pointer mismatch, unk_ns2 %p, unk_ns %p\n", unk_ns2, unk_ns);

    marshal = NULL;
    hr = IUnknown_QueryInterface(unk_ns, &IID_IMarshal, (void **)&marshal);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(marshal != NULL, "Failed to get IMarshal interface.\n");

    refcnt = IMarshal_AddRef(marshal);
    ok(refcnt == 2, "Expected refcnt %d, got %ld\n", 2, refcnt);

    refcnt = IMarshal_Release(marshal);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    refcnt = IMarshal_Release(marshal);
    ok(refcnt == 0, "Expected refcnt %d, got %ld\n", 0, refcnt);

    /* ReservedMixedAttributeValue. */
    hr = UiaGetReservedMixedAttributeValue(NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = UiaGetReservedMixedAttributeValue(&unk_ma);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk_ma != NULL, "UiaGetReservedMixedAttributeValue returned NULL interface.");

    refcnt = IUnknown_AddRef(unk_ma);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    refcnt = IUnknown_AddRef(unk_ma);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    refcnt = IUnknown_Release(unk_ma);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    hr = UiaGetReservedMixedAttributeValue(&unk_ma2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk_ma2 != NULL, "UiaGetReservedMixedAttributeValue returned NULL interface.");
    ok(unk_ma2 == unk_ma, "UiaGetReservedMixedAttribute pointer mismatch, unk_ma2 %p, unk_ma %p\n", unk_ma2, unk_ma);

    marshal = NULL;
    hr = IUnknown_QueryInterface(unk_ma, &IID_IMarshal, (void **)&marshal);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(marshal != NULL, "Failed to get IMarshal interface.\n");

    refcnt = IMarshal_AddRef(marshal);
    ok(refcnt == 2, "Expected refcnt %d, got %ld\n", 2, refcnt);

    refcnt = IMarshal_Release(marshal);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    refcnt = IMarshal_Release(marshal);
    ok(refcnt == 0, "Expected refcnt %d, got %ld\n", 0, refcnt);

    /* Test cross-thread marshaling behavior. */
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoMarshalInterThreadInterfaceInStream(&IID_IUnknown, unk_ns, &stream[0]);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = CoMarshalInterThreadInterfaceInStream(&IID_IUnknown, unk_ma, &stream[1]);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    thread = CreateThread(NULL, 0, uia_reserved_val_iface_marshal_thread, (void *)stream, 0, NULL);
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;
        while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    CloseHandle(thread);

    CoUninitialize();
}

struct msaa_role_uia_type {
    INT acc_role;
    INT uia_control_type;
};

static const struct msaa_role_uia_type msaa_role_uia_types[] = {
    { ROLE_SYSTEM_TITLEBAR,           UIA_TitleBarControlTypeId },
    { ROLE_SYSTEM_MENUBAR,            UIA_MenuBarControlTypeId },
    { ROLE_SYSTEM_SCROLLBAR,          UIA_ScrollBarControlTypeId },
    { ROLE_SYSTEM_GRIP,               UIA_ThumbControlTypeId },
    { ROLE_SYSTEM_WINDOW,             UIA_WindowControlTypeId },
    { ROLE_SYSTEM_MENUPOPUP,          UIA_MenuControlTypeId },
    { ROLE_SYSTEM_MENUITEM,           UIA_MenuItemControlTypeId },
    { ROLE_SYSTEM_TOOLTIP,            UIA_ToolTipControlTypeId },
    { ROLE_SYSTEM_APPLICATION,        UIA_WindowControlTypeId },
    { ROLE_SYSTEM_DOCUMENT,           UIA_DocumentControlTypeId },
    { ROLE_SYSTEM_PANE,               UIA_PaneControlTypeId },
    { ROLE_SYSTEM_GROUPING,           UIA_GroupControlTypeId },
    { ROLE_SYSTEM_SEPARATOR,          UIA_SeparatorControlTypeId },
    { ROLE_SYSTEM_TOOLBAR,            UIA_ToolBarControlTypeId },
    { ROLE_SYSTEM_STATUSBAR,          UIA_StatusBarControlTypeId },
    { ROLE_SYSTEM_TABLE,              UIA_TableControlTypeId },
    { ROLE_SYSTEM_COLUMNHEADER,       UIA_HeaderControlTypeId },
    { ROLE_SYSTEM_ROWHEADER,          UIA_HeaderControlTypeId },
    { ROLE_SYSTEM_CELL,               UIA_DataItemControlTypeId },
    { ROLE_SYSTEM_LINK,               UIA_HyperlinkControlTypeId },
    { ROLE_SYSTEM_LIST,               UIA_ListControlTypeId },
    { ROLE_SYSTEM_LISTITEM,           UIA_ListItemControlTypeId },
    { ROLE_SYSTEM_OUTLINE,            UIA_TreeControlTypeId },
    { ROLE_SYSTEM_OUTLINEITEM,        UIA_TreeItemControlTypeId },
    { ROLE_SYSTEM_PAGETAB,            UIA_TabItemControlTypeId },
    { ROLE_SYSTEM_INDICATOR,          UIA_ThumbControlTypeId },
    { ROLE_SYSTEM_GRAPHIC,            UIA_ImageControlTypeId },
    { ROLE_SYSTEM_STATICTEXT,         UIA_TextControlTypeId },
    { ROLE_SYSTEM_TEXT,               UIA_EditControlTypeId },
    { ROLE_SYSTEM_PUSHBUTTON,         UIA_ButtonControlTypeId },
    { ROLE_SYSTEM_CHECKBUTTON,        UIA_CheckBoxControlTypeId },
    { ROLE_SYSTEM_RADIOBUTTON,        UIA_RadioButtonControlTypeId },
    { ROLE_SYSTEM_COMBOBOX,           UIA_ComboBoxControlTypeId },
    { ROLE_SYSTEM_PROGRESSBAR,        UIA_ProgressBarControlTypeId },
    { ROLE_SYSTEM_SLIDER,             UIA_SliderControlTypeId },
    { ROLE_SYSTEM_SPINBUTTON,         UIA_SpinnerControlTypeId },
    { ROLE_SYSTEM_BUTTONDROPDOWN,     UIA_SplitButtonControlTypeId },
    { ROLE_SYSTEM_BUTTONMENU,         UIA_MenuItemControlTypeId },
    { ROLE_SYSTEM_BUTTONDROPDOWNGRID, UIA_ButtonControlTypeId },
    { ROLE_SYSTEM_PAGETABLIST,        UIA_TabControlTypeId },
    { ROLE_SYSTEM_SPLITBUTTON,        UIA_SplitButtonControlTypeId },
    /* These accessible roles have no equivalent in UI Automation. */
    { ROLE_SYSTEM_SOUND,              0 },
    { ROLE_SYSTEM_CURSOR,             0 },
    { ROLE_SYSTEM_CARET,              0 },
    { ROLE_SYSTEM_ALERT,              0 },
    { ROLE_SYSTEM_CLIENT,             0 },
    { ROLE_SYSTEM_CHART,              0 },
    { ROLE_SYSTEM_DIALOG,             0 },
    { ROLE_SYSTEM_BORDER,             0 },
    { ROLE_SYSTEM_COLUMN,             0 },
    { ROLE_SYSTEM_ROW,                0 },
    { ROLE_SYSTEM_HELPBALLOON,        0 },
    { ROLE_SYSTEM_CHARACTER,          0 },
    { ROLE_SYSTEM_PROPERTYPAGE,       0 },
    { ROLE_SYSTEM_DROPLIST,           0 },
    { ROLE_SYSTEM_DIAL,               0 },
    { ROLE_SYSTEM_HOTKEYFIELD,        0 },
    { ROLE_SYSTEM_DIAGRAM,            0 },
    { ROLE_SYSTEM_ANIMATION,          0 },
    { ROLE_SYSTEM_EQUATION,           0 },
    { ROLE_SYSTEM_WHITESPACE,         0 },
    { ROLE_SYSTEM_IPADDRESS,          0 },
    { ROLE_SYSTEM_OUTLINEBUTTON,      0 },
};

struct msaa_state_uia_prop {
    INT acc_state;
    INT prop_id;
};

static const struct msaa_state_uia_prop msaa_state_uia_props[] = {
    { STATE_SYSTEM_FOCUSED,      UIA_HasKeyboardFocusPropertyId },
    { STATE_SYSTEM_FOCUSABLE,    UIA_IsKeyboardFocusablePropertyId },
    { ~STATE_SYSTEM_UNAVAILABLE, UIA_IsEnabledPropertyId },
    { STATE_SYSTEM_PROTECTED,    UIA_IsPasswordPropertyId },
};

static void set_accessible_props(struct Accessible *acc, INT role, INT state,
        LONG child_count, LPCWSTR name, LONG left, LONG top, LONG width, LONG height)
{

    acc->role = role;
    acc->state = state;
    acc->child_count = child_count;
    acc->name = name;
    acc->left = left;
    acc->top = top;
    acc->width = width;
    acc->height = height;
}

static void set_accessible_ia2_props(struct Accessible *acc, BOOL enable_ia2, LONG unique_id)
{
    acc->enable_ia2 = enable_ia2;
    acc->unique_id = unique_id;
}

static void test_uia_prov_from_acc_ia2(void)
{
    IRawElementProviderSimple *elprov, *elprov2;
    HRESULT hr;

    /* Only one exposes an IA2 interface, no match. */
    set_accessible_props(&Accessible, 0, 0, 0, L"acc_name", 0, 0, 0, 0);
    set_accessible_ia2_props(&Accessible, TRUE, 0);
    set_accessible_props(&Accessible2, 0, 0, 0, L"acc_name", 0, 0, 0, 0);
    set_accessible_ia2_props(&Accessible2, FALSE, 0);

    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (Accessible.ref != 3)
    {
        IRawElementProviderSimple_Release(elprov);
        win_skip("UiaProviderFromIAccessible has no IAccessible2 support, skipping tests.\n");
        return;
    }

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);
    ok(Accessible2.ref == 1, "Unexpected refcnt %ld\n", Accessible2.ref);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * If &Accessible returns a failure code on get_uniqueID, &Accessible2's
     * uniqueID is not checked.
     */
    set_accessible_ia2_props(&Accessible, TRUE, 0);
    set_accessible_ia2_props(&Accessible2, TRUE, 0);
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_EXPECT(Accessible_get_accRole);
    SET_EXPECT(Accessible_get_accState);
    SET_EXPECT(Accessible_get_accChildCount);
    SET_EXPECT(Accessible_accLocation);
    SET_EXPECT(Accessible_get_accName);
    SET_EXPECT(Accessible_get_uniqueID);
    SET_EXPECT(Accessible2_get_accChildCount);
    SET_EXPECT(Accessible2_get_accName);
    SET_EXPECT(Accessible2_QI_IAccIdentity);
    SET_EXPECT(Accessible2_get_accParent);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    ok(Accessible2.ref == 1, "Unexpected refcnt %ld\n", Accessible2.ref);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_CALLED(Accessible_get_accRole);
    CHECK_CALLED(Accessible_get_accState);
    CHECK_CALLED(Accessible_get_accChildCount);
    CHECK_CALLED(Accessible_accLocation);
    CHECK_CALLED(Accessible_get_accName);
    CHECK_CALLED(Accessible_get_uniqueID);
    CHECK_CALLED(Accessible2_get_accChildCount);
    CHECK_CALLED(Accessible2_get_accName);
    todo_wine CHECK_CALLED(Accessible2_QI_IAccIdentity);
    todo_wine CHECK_CALLED(Accessible2_get_accParent);
    IRawElementProviderSimple_Release(elprov2);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* Unique ID matches. */
    set_accessible_ia2_props(&Accessible, TRUE, 1);
    set_accessible_ia2_props(&Accessible2, TRUE, 1);
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_EXPECT(Accessible_get_uniqueID);
    SET_EXPECT(Accessible2_get_uniqueID);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    ok(Accessible2.ref == 1, "Unexpected refcnt %ld\n", Accessible2.ref);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_CALLED(Accessible_get_uniqueID);
    CHECK_CALLED(Accessible2_get_uniqueID);
    IRawElementProviderSimple_Release(elprov2);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* Unique ID mismatch. */
    set_accessible_ia2_props(&Accessible, TRUE, 1);
    set_accessible_ia2_props(&Accessible2, TRUE, 2);
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_EXPECT(Accessible_get_uniqueID);
    SET_EXPECT(Accessible2_get_uniqueID);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);
    ok(Accessible2.ref == 1, "Unexpected refcnt %ld\n", Accessible2.ref);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_CALLED(Accessible_get_uniqueID);
    CHECK_CALLED(Accessible2_get_uniqueID);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, 0, 0, 0, NULL, 0, 0, 0, 0);
    set_accessible_ia2_props(&Accessible, FALSE, 0);
    set_accessible_props(&Accessible2, 0, 0, 0, NULL, 0, 0, 0, 0);
    set_accessible_ia2_props(&Accessible2, FALSE, 0);
}

#define check_fragment_acc( fragment, acc, cid) \
        check_fragment_acc_( (fragment), (acc), (cid), __LINE__)
static void check_fragment_acc_(IRawElementProviderFragment *elfrag, IAccessible *acc,
        INT cid, int line)
{
    ILegacyIAccessibleProvider *accprov;
    IAccessible *accessible;
    INT child_id;
    HRESULT hr;

    hr = IRawElementProviderFragment_QueryInterface(elfrag, &IID_ILegacyIAccessibleProvider, (void **)&accprov);
    ok_(__FILE__, line) (hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line) (!!accprov, "accprov == NULL\n");

    hr = ILegacyIAccessibleProvider_GetIAccessible(accprov, &accessible);
    ok_(__FILE__, line) (hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line) (accessible == acc, "accessible != acc\n");
    IAccessible_Release(accessible);

    hr = ILegacyIAccessibleProvider_get_ChildId(accprov, &child_id);
    ok_(__FILE__, line) (hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line) (child_id == cid, "child_id != cid\n");

    ILegacyIAccessibleProvider_Release(accprov);
}

static void test_uia_prov_from_acc_navigation(void)
{
    IRawElementProviderFragment *elfrag, *elfrag2, *elfrag3;
    IRawElementProviderSimple *elprov, *elprov2;
    HRESULT hr;

    /*
     * Full IAccessible parent, with 4 children:
     * childid 1 is a simple element, with STATE_SYSTEM_INVISIBLE.
     * childid 2 is Accessible_child.
     * childid 3 is a simple element with STATE_SYSTEM_NORMAL.
     * childid 4 is Accessible_child2.
     */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragment, (void **)&elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");

    /*
     * First time doing NavigateDirection_Parent will result in the same root
     * accessible check as get_HostRawElementProvider. If this IAccessible is
     * the root for its associated HWND, NavigateDirection_Parent and
     * NavigateDirection_Next/PreviousSibling will do nothing, as UI Automation
     * provides non-client area providers for the root IAccessible's parent
     * and siblings.
     */
    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 4,
            L"acc_name", 0, 0, 50, 50);
    set_accessible_props(&Accessible2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 4,
            L"acc_name", 0, 0, 50, 50);
    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_EXPECT(Accessible_get_accRole);
    SET_EXPECT(Accessible_get_accState);
    SET_EXPECT(Accessible_get_accChildCount);
    SET_EXPECT(Accessible_accLocation);
    SET_EXPECT(Accessible_get_accName);
    SET_EXPECT(Accessible2_get_accRole);
    SET_EXPECT(Accessible2_get_accState);
    SET_EXPECT(Accessible2_get_accChildCount);
    SET_EXPECT(Accessible2_accLocation);
    SET_EXPECT(Accessible2_get_accName);
    SET_EXPECT(Accessible2_QI_IAccIdentity);
    SET_EXPECT(Accessible2_get_accParent);
    elfrag2 = (void *)0xdeadbeef;
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_Parent, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_CALLED(Accessible_get_accRole);
    CHECK_CALLED(Accessible_get_accState);
    CHECK_CALLED(Accessible_get_accChildCount);
    CHECK_CALLED(Accessible_accLocation);
    CHECK_CALLED(Accessible_get_accName);
    CHECK_CALLED(Accessible2_get_accRole);
    CHECK_CALLED(Accessible2_get_accState);
    CHECK_CALLED(Accessible2_get_accChildCount);
    CHECK_CALLED(Accessible2_accLocation);
    CHECK_CALLED(Accessible2_get_accName);
    todo_wine CHECK_CALLED(Accessible2_QI_IAccIdentity);
    todo_wine CHECK_CALLED(Accessible2_get_accParent);
    acc_client = NULL;

    /* No check against root IAccessible, since it was done previously. */
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);

    /* Do nothing. */
    elfrag2 = (void *)0xdeadbeef;
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_Parent, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");

    elfrag2 = (void *)0xdeadbeef;
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_NextSibling, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");

    elfrag2 = (void *)0xdeadbeef;
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_PreviousSibling, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");

    /*
     * Retrieve childid 2 (Accessible_child) as first child. childid 1 is skipped due to
     * having a state of STATE_SYSTEM_INVISIBLE.
     */
    set_accessible_props(&Accessible_child, 0, STATE_SYSTEM_FOCUSABLE, 0, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible_child2, 0, STATE_SYSTEM_FOCUSABLE, 0, NULL, 0, 0, 0, 0);
    SET_EXPECT_MULTI(Accessible_get_accChildCount, 3);
    SET_EXPECT_MULTI(Accessible_get_accChild, 2);
    SET_EXPECT(Accessible_get_accState);
    SET_EXPECT(Accessible_child_get_accState);
    SET_EXPECT(Accessible_child_accNavigate);
    SET_EXPECT(Accessible_child_get_accParent);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_FirstChild, &elfrag2);
    ok(Accessible_child.ref == 2, "Unexpected refcnt %ld\n", Accessible_child.ref);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_CALLED_MULTI(Accessible_get_accChildCount, 3);
    CHECK_CALLED_MULTI(Accessible_get_accChild, 2);
    CHECK_CALLED(Accessible_child_get_accState);
    CHECK_CALLED(Accessible_child_accNavigate);
    CHECK_CALLED(Accessible_child_get_accParent);

    check_fragment_acc(elfrag2, &Accessible_child.IAccessible_iface, CHILDID_SELF);
    SET_EXPECT(Accessible_get_accChildCount);
    SET_EXPECT(Accessible_get_accChild);
    SET_EXPECT(Accessible_get_accState);
    hr = IRawElementProviderFragment_Navigate(elfrag2, NavigateDirection_NextSibling, &elfrag3);
    ok(Accessible.ref == 5, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag3, "elfrag2 == NULL\n");
    CHECK_CALLED(Accessible_get_accChildCount);
    CHECK_CALLED(Accessible_get_accChild);
    CHECK_CALLED(Accessible_get_accState);
    check_fragment_acc(elfrag3, &Accessible.IAccessible_iface, 3);

    IRawElementProviderFragment_Release(elfrag3);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);
    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible_child.ref == 1, "Unexpected refcnt %ld\n", Accessible_child.ref);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    /* Retrieve childid 3 as first child now that Accessible_child is invisible. */
    set_accessible_props(&Accessible_child, 0, STATE_SYSTEM_INVISIBLE, 0, NULL, 0, 0, 0, 0);
    SET_EXPECT_MULTI(Accessible_get_accChildCount, 4);
    SET_EXPECT_MULTI(Accessible_get_accChild, 3);
    SET_EXPECT_MULTI(Accessible_get_accState, 2);
    SET_EXPECT(Accessible_child_get_accState);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_FirstChild, &elfrag2);
    ok(Accessible.ref == 4, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_CALLED_MULTI(Accessible_get_accChildCount, 4);
    CHECK_CALLED_MULTI(Accessible_get_accChild, 3);
    CHECK_CALLED_MULTI(Accessible_get_accState, 2);
    CHECK_CALLED(Accessible_child_get_accState);
    check_fragment_acc(elfrag2, &Accessible.IAccessible_iface, 3);
    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    /* Retrieve childid 4 (Accessible_child2) as last child. */
    set_accessible_props(&Accessible_child2, 0, STATE_SYSTEM_FOCUSABLE, 0, NULL, 0, 0, 0, 0);
    SET_EXPECT_MULTI(Accessible_get_accChildCount, 2);
    SET_EXPECT(Accessible_get_accChild);
    SET_EXPECT(Accessible_child2_get_accState);
    SET_EXPECT(Accessible_child2_accNavigate);
    SET_EXPECT(Accessible_child2_get_accParent);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_LastChild, &elfrag2);
    ok(Accessible_child2.ref == 2, "Unexpected refcnt %ld\n", Accessible_child2.ref);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_CALLED_MULTI(Accessible_get_accChildCount, 2);
    CHECK_CALLED(Accessible_get_accChild);
    CHECK_CALLED(Accessible_child2_get_accState);
    CHECK_CALLED(Accessible_child2_accNavigate);
    CHECK_CALLED(Accessible_child2_get_accParent);

    check_fragment_acc(elfrag2, &Accessible_child2.IAccessible_iface, CHILDID_SELF);
    SET_EXPECT(Accessible_get_accChildCount);
    SET_EXPECT(Accessible_get_accChild);
    SET_EXPECT(Accessible_get_accState);
    hr = IRawElementProviderFragment_Navigate(elfrag2, NavigateDirection_PreviousSibling, &elfrag3);
    ok(Accessible.ref == 5, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag3, "elfrag2 == NULL\n");
    CHECK_CALLED(Accessible_get_accChildCount);
    CHECK_CALLED(Accessible_get_accChild);
    CHECK_CALLED(Accessible_get_accState);
    check_fragment_acc(elfrag3, &Accessible.IAccessible_iface, 3);

    IRawElementProviderFragment_Release(elfrag3);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);
    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible_child2.ref == 1, "Unexpected refcnt %ld\n", Accessible_child2.ref);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    /* Retrieve childid 3 as last child, now that Accessible_child2 is STATE_SYSTEM_INVISIBLE. */
    set_accessible_props(&Accessible_child2, 0, STATE_SYSTEM_INVISIBLE, 0, NULL, 0, 0, 0, 0);
    SET_EXPECT_MULTI(Accessible_get_accChildCount, 3);
    SET_EXPECT_MULTI(Accessible_get_accChild, 2);
    SET_EXPECT(Accessible_get_accState);
    SET_EXPECT(Accessible_child2_get_accState);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_LastChild, &elfrag2);
    ok(Accessible.ref == 4, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_CALLED_MULTI(Accessible_get_accChildCount, 3);
    CHECK_CALLED_MULTI(Accessible_get_accChild, 2);
    CHECK_CALLED(Accessible_get_accState);
    CHECK_CALLED(Accessible_child2_get_accState);
    check_fragment_acc(elfrag2, &Accessible.IAccessible_iface, 3);
    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    IRawElementProviderFragment_Release(elfrag);
    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * Full IAccessible child tests.
     */
    SET_EXPECT(Accessible_child_accNavigate);
    SET_EXPECT(Accessible_child_get_accParent);
    hr = pUiaProviderFromIAccessible(&Accessible_child.IAccessible_iface, 0, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(Accessible_child_accNavigate);
    CHECK_CALLED(Accessible_child_get_accParent);
    ok(Accessible_child.ref == 2, "Unexpected refcnt %ld\n", Accessible_child.ref);

    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragment, (void **)&elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");

    /*
     * After determining this isn't the root IAccessible, get_accParent will
     * be used to get the parent.
     */
    set_accessible_props(&Accessible2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 0, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible_child, ROLE_SYSTEM_CLIENT, STATE_SYSTEM_FOCUSABLE, 0, NULL, 0, 0, 0, 0);
    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_EXPECT(Accessible_child_get_accRole);
    SET_EXPECT(Accessible_child_get_accParent);
    SET_EXPECT(Accessible2_get_accRole);
    SET_EXPECT(Accessible2_QI_IAccIdentity);
    SET_EXPECT(Accessible2_get_accParent);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_Parent, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_CALLED(Accessible_child_get_accParent);
    CHECK_CALLED(Accessible_child_get_accRole);
    CHECK_CALLED(Accessible2_get_accRole);
    todo_wine CHECK_CALLED(Accessible2_QI_IAccIdentity);
    todo_wine CHECK_CALLED(Accessible2_get_accParent);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    check_fragment_acc(elfrag2, &Accessible.IAccessible_iface, CHILDID_SELF);
    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);
    acc_client = NULL;

    /* Second call only does get_accParent, no root check. */
    SET_EXPECT(Accessible_child_get_accParent);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_Parent, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_CALLED(Accessible_child_get_accParent);
    check_fragment_acc(elfrag2, &Accessible.IAccessible_iface, CHILDID_SELF);
    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* ChildCount of 0, do nothing for First/Last child.*/
    SET_EXPECT(Accessible_child_get_accChildCount);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_FirstChild, &elfrag2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");
    CHECK_CALLED(Accessible_child_get_accChildCount);

    SET_EXPECT(Accessible_child_get_accChildCount);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_LastChild, &elfrag2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");
    CHECK_CALLED(Accessible_child_get_accChildCount);

    /*
     * In the case of sibling navigation on an IAccessible that wasn't
     * received through previous navigation from a parent (i.e, from
     * NavigateDirection_First/LastChild), we have to figure out which
     * IAccessible child we represent by comparing against all children of our
     * IAccessible parent. If we find more than one IAccessible that matches,
     * or none at all that do, navigation will fail.
     */
    set_accessible_props(&Accessible_child, ROLE_SYSTEM_CLIENT, STATE_SYSTEM_FOCUSABLE, 1,
            L"acc_child", 0, 0, 50, 50);
    set_accessible_props(&Accessible_child2, ROLE_SYSTEM_CLIENT, STATE_SYSTEM_FOCUSABLE, 1,
            L"acc_child", 0, 0, 50, 50);
    SET_EXPECT_MULTI(Accessible_get_accChildCount, 5);
    SET_EXPECT_MULTI(Accessible_get_accChild, 4);
    SET_EXPECT(Accessible_child_get_accParent);
    SET_EXPECT(Accessible_child_get_accRole);
    SET_EXPECT(Accessible_child_get_accState);
    SET_EXPECT(Accessible_child_get_accChildCount);
    SET_EXPECT(Accessible_child_accLocation);
    SET_EXPECT(Accessible_child_get_accName);
    SET_EXPECT(Accessible_child2_get_accRole);
    SET_EXPECT(Accessible_child2_get_accState);
    SET_EXPECT(Accessible_child2_get_accChildCount);
    SET_EXPECT(Accessible_child2_accLocation);
    SET_EXPECT(Accessible_child2_get_accName);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_NextSibling, &elfrag2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");
    CHECK_CALLED_MULTI(Accessible_get_accChildCount, 5);
    CHECK_CALLED_MULTI(Accessible_get_accChild, 4);
    CHECK_CALLED(Accessible_child_get_accParent);
    CHECK_CALLED(Accessible_child_get_accRole);
    CHECK_CALLED(Accessible_child_get_accState);
    CHECK_CALLED(Accessible_child_get_accChildCount);
    CHECK_CALLED(Accessible_child_accLocation);
    CHECK_CALLED(Accessible_child_get_accName);
    CHECK_CALLED(Accessible_child2_get_accRole);
    CHECK_CALLED(Accessible_child2_get_accState);
    CHECK_CALLED(Accessible_child2_get_accChildCount);
    CHECK_CALLED(Accessible_child2_accLocation);
    CHECK_CALLED(Accessible_child2_get_accName);

    /* Now they have a role mismatch, we can determine our position. */
    set_accessible_props(&Accessible_child2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1,
            L"acc_child", 0, 0, 50, 50);
    SET_EXPECT_MULTI(Accessible_get_accChildCount, 6);
    SET_EXPECT_MULTI(Accessible_get_accChild, 5);
    /* Check ChildID 1 for STATE_SYSTEM_INVISIBLE. */
    SET_EXPECT(Accessible_get_accState);
    SET_EXPECT(Accessible_child_get_accParent);
    SET_EXPECT(Accessible_child_get_accRole);
    SET_EXPECT(Accessible_child2_get_accRole);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_PreviousSibling, &elfrag2);
    /*
     * Even though we didn't get a new fragment, now that we know our
     * position, a reference is added to the parent IAccessible.
     */
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");
    CHECK_CALLED_MULTI(Accessible_get_accChildCount, 6);
    CHECK_CALLED_MULTI(Accessible_get_accChild, 5);
    CHECK_CALLED(Accessible_get_accState);
    CHECK_CALLED(Accessible_child_get_accParent);
    CHECK_CALLED(Accessible_child_get_accRole);
    CHECK_CALLED(Accessible_child2_get_accRole);

    /* Now that we know our position, no extra nav work. */
    SET_EXPECT(Accessible_get_accChildCount);
    SET_EXPECT(Accessible_get_accChild);
    SET_EXPECT(Accessible_get_accState);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_NextSibling, &elfrag2);
    ok(Accessible.ref == 4, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_CALLED(Accessible_get_accChildCount);
    CHECK_CALLED(Accessible_get_accChild);
    CHECK_CALLED(Accessible_get_accState);
    if (elfrag2)
    {
        check_fragment_acc(elfrag2, &Accessible.IAccessible_iface, 3);
        IRawElementProviderFragment_Release(elfrag2);
        ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    }

    IRawElementProviderFragment_Release(elfrag);
    IRawElementProviderSimple_Release(elprov);
    ok(Accessible_child.ref == 1, "Unexpected refcnt %ld\n", Accessible_child.ref);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * Simple element child tests.
     */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, 1, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragment, (void **)&elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");

    /*
     * Simple child elements don't check the root IAccessible, because they
     * can't be the root IAccessible.
     */
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_Parent, &elfrag2);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    check_fragment_acc(elfrag2, &Accessible.IAccessible_iface, CHILDID_SELF);
    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * Test NavigateDirection_First/LastChild on simple child element. Does
     * nothing, as simple children cannot have children.
     */
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_FirstChild, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");

    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_LastChild, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");

    /*
     * NavigateDirection_Next/PreviousSibling behaves normally, no IAccessible
     * comparisons.
     */
    SET_EXPECT(Accessible_get_accChildCount);
    SET_EXPECT(Accessible_get_accChild);
    SET_EXPECT(Accessible_child_get_accState);
    SET_EXPECT(Accessible_child_accNavigate);
    SET_EXPECT(Accessible_child_get_accParent);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_NextSibling, &elfrag2);
    ok(Accessible_child.ref == 2, "Unexpected refcnt %ld\n", Accessible_child.ref);
    ok(Accessible.ref == 4, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_CALLED(Accessible_get_accChildCount);
    CHECK_CALLED(Accessible_get_accChild);
    CHECK_CALLED(Accessible_child_get_accState);
    CHECK_CALLED(Accessible_child_accNavigate);
    CHECK_CALLED(Accessible_child_get_accParent);
    check_fragment_acc(elfrag2, &Accessible_child.IAccessible_iface, CHILDID_SELF);

    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible_child.ref == 1, "Unexpected refcnt %ld\n", Accessible_child.ref);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);
    IRawElementProviderFragment_Release(elfrag);
    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, 0, 0, 0, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible2, 0, 0, 0, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible_child, 0, 0, 0, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible_child2, 0, 0, 0, NULL, 0, 0, 0, 0);
}

static void test_uia_prov_from_acc_properties(void)
{
    IRawElementProviderSimple *elprov;
    HRESULT hr;
    VARIANT v;
    int i, x;

    /* MSAA role to UIA control type test. */
    VariantInit(&v);
    for (i = 0; i < ARRAY_SIZE(msaa_role_uia_types); i++)
    {
        const struct msaa_role_uia_type *role = &msaa_role_uia_types[i];

        /*
         * Roles get cached once a valid one is mapped, so create a new
         * element for each role.
         */
        hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

        Accessible.role = role->acc_role;
        SET_EXPECT(Accessible_get_accRole);
        VariantClear(&v);
        hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_ControlTypePropertyId, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (role->uia_control_type)
            ok(check_variant_i4(&v, role->uia_control_type), "MSAA role %d: V_I4(&v) = %ld\n", role->acc_role, V_I4(&v));
        else
            ok(V_VT(&v) == VT_EMPTY, "MSAA role %d: V_VT(&v) = %d\n", role->acc_role, V_VT(&v));
        CHECK_CALLED(Accessible_get_accRole);

        if (!role->uia_control_type)
            SET_EXPECT(Accessible_get_accRole);
        VariantClear(&v);
        hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_ControlTypePropertyId, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (role->uia_control_type)
            ok(check_variant_i4(&v, role->uia_control_type), "MSAA role %d: V_I4(&v) = %ld\n", role->acc_role, V_I4(&v));
        else
            ok(V_VT(&v) == VT_EMPTY, "MSAA role %d: V_VT(&v) = %d\n", role->acc_role, V_VT(&v));
        if (!role->uia_control_type)
            CHECK_CALLED(Accessible_get_accRole);

        IRawElementProviderSimple_Release(elprov);
        ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);
    }

    /* ROLE_SYSTEM_CLOCK has no mapping in Windows < 10 1809. */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    Accessible.role = ROLE_SYSTEM_CLOCK;
    SET_EXPECT(Accessible_get_accRole);
    VariantClear(&v);
    hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(check_variant_i4(&v, UIA_ButtonControlTypeId) || broken(V_VT(&v) == VT_EMPTY), /* Windows < 10 1809 */
            "MSAA role %d: V_I4(&v) = %ld\n", Accessible.role, V_I4(&v));
    CHECK_CALLED(Accessible_get_accRole);

    if (V_VT(&v) == VT_EMPTY)
        SET_EXPECT(Accessible_get_accRole);
    VariantClear(&v);
    hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_ControlTypePropertyId, &v);
    ok(check_variant_i4(&v, UIA_ButtonControlTypeId) || broken(V_VT(&v) == VT_EMPTY), /* Windows < 10 1809 */
            "MSAA role %d: V_I4(&v) = %ld\n", Accessible.role, V_I4(&v));
    if (V_VT(&v) == VT_EMPTY)
        CHECK_CALLED(Accessible_get_accRole);

    Accessible.role = 0;
    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    /* UIA PropertyId's that correspond directly to individual MSAA state flags. */
    for (i = 0; i < ARRAY_SIZE(msaa_state_uia_props); i++)
    {
        const struct msaa_state_uia_prop *state = &msaa_state_uia_props[i];

        for (x = 0; x < 2; x++)
        {
            Accessible.state = x ? state->acc_state : ~state->acc_state;
            SET_EXPECT(Accessible_get_accState);
            hr = IRawElementProviderSimple_GetPropertyValue(elprov, state->prop_id, &v);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(check_variant_bool(&v, x), "V_BOOL(&v) = %#x\n", V_BOOL(&v));
            CHECK_CALLED(Accessible_get_accState);
        }
    }
    Accessible.state = 0;

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);
}

static void test_UiaProviderFromIAccessible(void)
{
    ILegacyIAccessibleProvider *accprov;
    IRawElementProviderSimple *elprov, *elprov2;
    enum ProviderOptions prov_opt;
    IAccessible *acc;
    IUnknown *unk;
    WNDCLASSA cls;
    HRESULT hr;
    HWND hwnd;
    VARIANT v;
    INT cid;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    cls.style = 0;
    cls.lpfnWndProc = test_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = NULL;
    cls.hbrBackground = NULL;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "UiaProviderFromIAccessible class";

    RegisterClassA(&cls);

    hwnd = CreateWindowA("UiaProviderFromIAccessible class", "Test window", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, NULL, NULL, NULL, NULL);

    hr = pUiaProviderFromIAccessible(NULL, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /*
     * UiaProviderFromIAccessible will not wrap an MSAA proxy, this is
     * detected by checking for the 'IIS_IsOleaccProxy' service from the
     * IServiceProvider interface.
     */
    hr = CreateStdAccessibleObject(hwnd, OBJID_CLIENT, &IID_IAccessible, (void**)&acc);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!!acc, "acc == NULL\n");

    hr = pUiaProviderFromIAccessible(acc, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    IAccessible_Release(acc);

    /* Don't return an HWND from accNavigate or OleWindow. */
    SET_EXPECT(Accessible_accNavigate);
    SET_EXPECT(Accessible_get_accParent);
    Accessible.acc_hwnd = NULL;
    Accessible.ow_hwnd = NULL;
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(Accessible_accNavigate);
    CHECK_CALLED(Accessible_get_accParent);

    /* Return an HWND from accNavigate, not OleWindow. */
    SET_EXPECT(Accessible_accNavigate);
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    acc_client = &Accessible.IAccessible_iface;
    Accessible.acc_hwnd = hwnd;
    Accessible.ow_hwnd = NULL;
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(Accessible_accNavigate);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);
    acc_client = NULL;

    /* Skip tests on Win10v1507. */
    if (called_winproc_GETOBJECT_CLIENT)
    {
        win_skip("UiaProviderFromIAccessible behaves inconsistently on Win10 1507, skipping tests.\n");
        return;
    }
    expect_winproc_GETOBJECT_CLIENT = FALSE;

    /* Return an HWND from parent IAccessible's IOleWindow interface. */
    SET_EXPECT(Accessible_child_accNavigate);
    SET_EXPECT(Accessible_child_get_accParent);
    Accessible.acc_hwnd = NULL;
    Accessible.ow_hwnd = hwnd;
    hr = pUiaProviderFromIAccessible(&Accessible_child.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(Accessible_child_accNavigate);
    CHECK_CALLED(Accessible_child_get_accParent);
    ok(Accessible_child.ref == 2, "Unexpected refcnt %ld\n", Accessible_child.ref);
    IRawElementProviderSimple_Release(elprov);
    ok(Accessible_child.ref == 1, "Unexpected refcnt %ld\n", Accessible_child.ref);

    /* Return an HWND from OleWindow, not accNavigate. */
    Accessible.acc_hwnd = NULL;
    Accessible.ow_hwnd = hwnd;
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    hr = IRawElementProviderSimple_get_ProviderOptions(elprov, &prov_opt);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok((prov_opt == (ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading)) ||
            broken(prov_opt == ProviderOptions_ClientSideProvider), /* Windows < 10 1507 */
            "Unexpected provider options %#x\n", prov_opt);

    hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BSTR, "V_VT(&v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hr = IRawElementProviderSimple_GetPatternProvider(elprov, UIA_LegacyIAccessiblePatternId, &unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!unk, "unk == NULL\n");
    ok(iface_cmp((IUnknown *)elprov, unk), "unk != elprov\n");

    hr = IUnknown_QueryInterface(unk, &IID_ILegacyIAccessibleProvider, (void **)&accprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!accprov, "accprov == NULL\n");

    hr = ILegacyIAccessibleProvider_get_ChildId(accprov, &cid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(cid == CHILDID_SELF, "cid != CHILDID_SELF\n");

    hr = ILegacyIAccessibleProvider_GetIAccessible(accprov, &acc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(acc == &Accessible.IAccessible_iface, "acc != &Accessible.IAccessible_iface\n");
    IAccessible_Release(acc);
    IUnknown_Release(unk);
    ILegacyIAccessibleProvider_Release(accprov);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* ChildID other than CHILDID_SELF. */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, 1, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * Simple child element (IAccessible without CHILDID_SELF) cannot be root
     * IAccessible. No checks against the root HWND IAccessible will be done.
     */
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL\n");

    hr = IRawElementProviderSimple_GetPatternProvider(elprov, UIA_LegacyIAccessiblePatternId, &unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!unk, "unk == NULL\n");
    ok(iface_cmp((IUnknown *)elprov, unk), "unk != elprov\n");

    hr = IUnknown_QueryInterface(unk, &IID_ILegacyIAccessibleProvider, (void **)&accprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!accprov, "accprov == NULL\n");

    hr = ILegacyIAccessibleProvider_get_ChildId(accprov, &cid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(cid == 1, "cid != CHILDID_SELF\n");

    hr = ILegacyIAccessibleProvider_GetIAccessible(accprov, &acc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(acc == &Accessible.IAccessible_iface, "acc != &Accessible.IAccessible_iface\n");
    IAccessible_Release(acc);
    IUnknown_Release(unk);
    ILegacyIAccessibleProvider_Release(accprov);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * &Accessible.IAccessible_iface will be compared against the default
     * client accessible object. Since we have all properties set to 0,
     * we return failure HRESULTs and all properties will get queried but not
     * compared.
     */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_EXPECT(Accessible_get_accRole);
    SET_EXPECT(Accessible_get_accState);
    SET_EXPECT(Accessible_get_accChildCount);
    SET_EXPECT(Accessible_accLocation);
    SET_EXPECT(Accessible_get_accName);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL\n");
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_CALLED(Accessible_get_accRole);
    CHECK_CALLED(Accessible_get_accState);
    CHECK_CALLED(Accessible_get_accChildCount);
    CHECK_CALLED(Accessible_accLocation);
    CHECK_CALLED(Accessible_get_accName);

    /* Second call won't send WM_GETOBJECT. */
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL\n");

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * Return &Accessible.IAccessible_iface in response to OBJID_CLIENT,
     * interface pointers will be compared, no method calls to check property
     * values.
     */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    elprov2 = (void *)0xdeadbeef;
    acc_client = &Accessible.IAccessible_iface;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    /* Second call, no checks. */
    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * Return &Accessible2.IAccessible_iface in response to OBJID_CLIENT,
     * interface pointers won't match, so properties will be compared.
     */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1,
            L"acc_name", 0, 0, 50, 50);
    set_accessible_props(&Accessible2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1,
            L"acc_name", 0, 0, 50, 50);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_EXPECT(Accessible_get_accRole);
    SET_EXPECT(Accessible_get_accState);
    SET_EXPECT(Accessible_get_accChildCount);
    SET_EXPECT(Accessible_accLocation);
    SET_EXPECT(Accessible_get_accName);
    SET_EXPECT(Accessible2_get_accRole);
    SET_EXPECT(Accessible2_get_accState);
    SET_EXPECT(Accessible2_get_accChildCount);
    SET_EXPECT(Accessible2_accLocation);
    SET_EXPECT(Accessible2_get_accName);
    /*
     * The IAccessible returned by WM_GETOBJECT will be checked for an
     * IAccIdentity interface to see if Dynamic Annotation properties should
     * be queried. If not present on the current IAccessible, it will check
     * the parent IAccessible for one.
     */
    SET_EXPECT(Accessible2_QI_IAccIdentity);
    SET_EXPECT(Accessible2_get_accParent);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    ok(Accessible2.ref == 1, "Unexpected refcnt %ld\n", Accessible2.ref);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_CALLED(Accessible_get_accRole);
    CHECK_CALLED(Accessible_get_accState);
    CHECK_CALLED(Accessible_get_accChildCount);
    CHECK_CALLED(Accessible_accLocation);
    CHECK_CALLED(Accessible_get_accName);
    CHECK_CALLED(Accessible2_get_accRole);
    CHECK_CALLED(Accessible2_get_accState);
    CHECK_CALLED(Accessible2_get_accChildCount);
    CHECK_CALLED(Accessible2_accLocation);
    CHECK_CALLED(Accessible2_get_accName);
    todo_wine CHECK_CALLED(Accessible2_QI_IAccIdentity);
    todo_wine CHECK_CALLED(Accessible2_get_accParent);
    IRawElementProviderSimple_Release(elprov2);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * If a failure HRESULT is returned from the IRawElementProviderSimple
     * IAccessible, the corresponding AOFW IAccessible method isn't called.
     * An exception is get_accChildCount, which is always called, but only
     * checked if the HRESULT return value is not a failure. If Role/State/Name
     * are not queried, no IAccIdentity check is done.
     */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, 0, 0, 0, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1,
            L"acc_name", 0, 0, 50, 50);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_EXPECT(Accessible_get_accRole);
    SET_EXPECT(Accessible_get_accState);
    SET_EXPECT(Accessible_get_accChildCount);
    SET_EXPECT(Accessible2_get_accChildCount);
    SET_EXPECT(Accessible_accLocation);
    SET_EXPECT(Accessible_get_accName);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_CALLED(Accessible_get_accRole);
    CHECK_CALLED(Accessible_get_accState);
    CHECK_CALLED(Accessible_get_accChildCount);
    CHECK_CALLED(Accessible2_get_accChildCount);
    CHECK_CALLED(Accessible_accLocation);
    CHECK_CALLED(Accessible_get_accName);

    acc_client = NULL;
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * Properties are checked in a sequence of accRole, accState,
     * accChildCount, accLocation, and finally accName. If a mismatch is found
     * early in the sequence, the rest aren't checked.
     */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 0, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible2, ROLE_SYSTEM_CLIENT, STATE_SYSTEM_FOCUSABLE, 0, NULL, 0, 0, 0, 0);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_EXPECT(Accessible_get_accRole);
    SET_EXPECT(Accessible2_get_accRole);
    SET_EXPECT(Accessible2_QI_IAccIdentity);
    SET_EXPECT(Accessible2_get_accParent);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_CALLED(Accessible_get_accRole);
    CHECK_CALLED(Accessible2_get_accRole);
    todo_wine CHECK_CALLED(Accessible2_QI_IAccIdentity);
    todo_wine CHECK_CALLED(Accessible2_get_accParent);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* 4/5 properties match, considered a match. */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1, NULL, 0, 0, 50, 50);
    set_accessible_props(&Accessible2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1, NULL, 0, 0, 50, 50);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_EXPECT(Accessible_get_accRole);
    SET_EXPECT(Accessible_get_accState);
    SET_EXPECT(Accessible_get_accChildCount);
    SET_EXPECT(Accessible_accLocation);
    SET_EXPECT(Accessible_get_accName);
    SET_EXPECT(Accessible2_get_accRole);
    SET_EXPECT(Accessible2_get_accState);
    SET_EXPECT(Accessible2_get_accChildCount);
    SET_EXPECT(Accessible2_accLocation);
    SET_EXPECT(Accessible2_QI_IAccIdentity);
    SET_EXPECT(Accessible2_get_accParent);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    ok(Accessible2.ref == 1, "Unexpected refcnt %ld\n", Accessible2.ref);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_CALLED(Accessible_get_accRole);
    CHECK_CALLED(Accessible_get_accState);
    CHECK_CALLED(Accessible_get_accChildCount);
    CHECK_CALLED(Accessible_accLocation);
    CHECK_CALLED(Accessible_get_accName);
    CHECK_CALLED(Accessible2_get_accRole);
    CHECK_CALLED(Accessible2_get_accState);
    CHECK_CALLED(Accessible2_get_accChildCount);
    CHECK_CALLED(Accessible2_accLocation);
    todo_wine CHECK_CALLED(Accessible2_QI_IAccIdentity);
    todo_wine CHECK_CALLED(Accessible2_get_accParent);
    IRawElementProviderSimple_Release(elprov2);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* 3/5 properties match, not considered a match. */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1, NULL, 0, 0, 0, 0);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_EXPECT(Accessible_get_accRole);
    SET_EXPECT(Accessible_get_accState);
    SET_EXPECT(Accessible_get_accChildCount);
    SET_EXPECT(Accessible_accLocation);
    SET_EXPECT(Accessible_get_accName);
    SET_EXPECT(Accessible2_get_accRole);
    SET_EXPECT(Accessible2_get_accState);
    SET_EXPECT(Accessible2_get_accChildCount);
    SET_EXPECT(Accessible2_QI_IAccIdentity);
    SET_EXPECT(Accessible2_get_accParent);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_CALLED(Accessible_get_accRole);
    CHECK_CALLED(Accessible_get_accState);
    CHECK_CALLED(Accessible_get_accChildCount);
    CHECK_CALLED(Accessible_accLocation);
    CHECK_CALLED(Accessible_get_accName);
    CHECK_CALLED(Accessible2_get_accRole);
    CHECK_CALLED(Accessible2_get_accState);
    CHECK_CALLED(Accessible2_get_accChildCount);
    todo_wine CHECK_CALLED(Accessible2_QI_IAccIdentity);
    todo_wine CHECK_CALLED(Accessible2_get_accParent);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* Only name matches, considered a match. */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, 0, 0, 0, L"acc_name", 0, 0, 0, 0);
    set_accessible_props(&Accessible2, 0, 0, 0, L"acc_name", 0, 0, 0, 0);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_EXPECT(Accessible_get_accRole);
    SET_EXPECT(Accessible_get_accState);
    SET_EXPECT(Accessible_get_accChildCount);
    SET_EXPECT(Accessible_accLocation);
    SET_EXPECT(Accessible_get_accName);
    SET_EXPECT(Accessible2_get_accChildCount);
    SET_EXPECT(Accessible2_get_accName);
    SET_EXPECT(Accessible2_QI_IAccIdentity);
    SET_EXPECT(Accessible2_get_accParent);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    ok(Accessible2.ref == 1, "Unexpected refcnt %ld\n", Accessible2.ref);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_CALLED(Accessible_get_accRole);
    CHECK_CALLED(Accessible_get_accState);
    CHECK_CALLED(Accessible_get_accChildCount);
    CHECK_CALLED(Accessible_accLocation);
    CHECK_CALLED(Accessible_get_accName);
    CHECK_CALLED(Accessible2_get_accChildCount);
    CHECK_CALLED(Accessible2_get_accName);
    todo_wine CHECK_CALLED(Accessible2_QI_IAccIdentity);
    todo_wine CHECK_CALLED(Accessible2_get_accParent);
    IRawElementProviderSimple_Release(elprov2);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    test_uia_prov_from_acc_properties();
    test_uia_prov_from_acc_navigation();
    test_uia_prov_from_acc_ia2();

    CoUninitialize();
    DestroyWindow(hwnd);
    UnregisterClassA("pUiaProviderFromIAccessible class", NULL);
    Accessible.acc_hwnd = NULL;
    Accessible.ow_hwnd = NULL;
}

struct uia_lookup_id {
    const GUID *guid;
    int id;
};

static const struct uia_lookup_id uia_property_lookup_ids[] = {
    { &RuntimeId_Property_GUID,                           UIA_RuntimeIdPropertyId },
    { &BoundingRectangle_Property_GUID,                   UIA_BoundingRectanglePropertyId },
    { &ProcessId_Property_GUID,                           UIA_ProcessIdPropertyId },
    { &ControlType_Property_GUID,                         UIA_ControlTypePropertyId },
    { &LocalizedControlType_Property_GUID,                UIA_LocalizedControlTypePropertyId },
    { &Name_Property_GUID,                                UIA_NamePropertyId },
    { &AcceleratorKey_Property_GUID,                      UIA_AcceleratorKeyPropertyId },
    { &AccessKey_Property_GUID,                           UIA_AccessKeyPropertyId },
    { &HasKeyboardFocus_Property_GUID,                    UIA_HasKeyboardFocusPropertyId },
    { &IsKeyboardFocusable_Property_GUID,                 UIA_IsKeyboardFocusablePropertyId },
    { &IsEnabled_Property_GUID,                           UIA_IsEnabledPropertyId },
    { &AutomationId_Property_GUID,                        UIA_AutomationIdPropertyId },
    { &ClassName_Property_GUID,                           UIA_ClassNamePropertyId },
    { &HelpText_Property_GUID,                            UIA_HelpTextPropertyId },
    { &ClickablePoint_Property_GUID,                      UIA_ClickablePointPropertyId },
    { &Culture_Property_GUID,                             UIA_CulturePropertyId },
    { &IsControlElement_Property_GUID,                    UIA_IsControlElementPropertyId },
    { &IsContentElement_Property_GUID,                    UIA_IsContentElementPropertyId },
    { &LabeledBy_Property_GUID,                           UIA_LabeledByPropertyId },
    { &IsPassword_Property_GUID,                          UIA_IsPasswordPropertyId },
    { &NewNativeWindowHandle_Property_GUID,               UIA_NativeWindowHandlePropertyId },
    { &ItemType_Property_GUID,                            UIA_ItemTypePropertyId },
    { &IsOffscreen_Property_GUID,                         UIA_IsOffscreenPropertyId },
    { &Orientation_Property_GUID,                         UIA_OrientationPropertyId },
    { &FrameworkId_Property_GUID,                         UIA_FrameworkIdPropertyId },
    { &IsRequiredForForm_Property_GUID,                   UIA_IsRequiredForFormPropertyId },
    { &ItemStatus_Property_GUID,                          UIA_ItemStatusPropertyId },
    { &IsDockPatternAvailable_Property_GUID,              UIA_IsDockPatternAvailablePropertyId },
    { &IsExpandCollapsePatternAvailable_Property_GUID,    UIA_IsExpandCollapsePatternAvailablePropertyId },
    { &IsGridItemPatternAvailable_Property_GUID,          UIA_IsGridItemPatternAvailablePropertyId },
    { &IsGridPatternAvailable_Property_GUID,              UIA_IsGridPatternAvailablePropertyId },
    { &IsInvokePatternAvailable_Property_GUID,            UIA_IsInvokePatternAvailablePropertyId },
    { &IsMultipleViewPatternAvailable_Property_GUID,      UIA_IsMultipleViewPatternAvailablePropertyId },
    { &IsRangeValuePatternAvailable_Property_GUID,        UIA_IsRangeValuePatternAvailablePropertyId },
    { &IsScrollPatternAvailable_Property_GUID,            UIA_IsScrollPatternAvailablePropertyId },
    { &IsScrollItemPatternAvailable_Property_GUID,        UIA_IsScrollItemPatternAvailablePropertyId },
    { &IsSelectionItemPatternAvailable_Property_GUID,     UIA_IsSelectionItemPatternAvailablePropertyId },
    { &IsSelectionPatternAvailable_Property_GUID,         UIA_IsSelectionPatternAvailablePropertyId },
    { &IsTablePatternAvailable_Property_GUID,             UIA_IsTablePatternAvailablePropertyId },
    { &IsTableItemPatternAvailable_Property_GUID,         UIA_IsTableItemPatternAvailablePropertyId },
    { &IsTextPatternAvailable_Property_GUID,              UIA_IsTextPatternAvailablePropertyId },
    { &IsTogglePatternAvailable_Property_GUID,            UIA_IsTogglePatternAvailablePropertyId },
    { &IsTransformPatternAvailable_Property_GUID,         UIA_IsTransformPatternAvailablePropertyId },
    { &IsValuePatternAvailable_Property_GUID,             UIA_IsValuePatternAvailablePropertyId },
    { &IsWindowPatternAvailable_Property_GUID,            UIA_IsWindowPatternAvailablePropertyId },
    { &Value_Value_Property_GUID,                         UIA_ValueValuePropertyId },
    { &Value_IsReadOnly_Property_GUID,                    UIA_ValueIsReadOnlyPropertyId },
    { &RangeValue_Value_Property_GUID,                    UIA_RangeValueValuePropertyId },
    { &RangeValue_IsReadOnly_Property_GUID,               UIA_RangeValueIsReadOnlyPropertyId },
    { &RangeValue_Minimum_Property_GUID,                  UIA_RangeValueMinimumPropertyId },
    { &RangeValue_Maximum_Property_GUID,                  UIA_RangeValueMaximumPropertyId },
    { &RangeValue_LargeChange_Property_GUID,              UIA_RangeValueLargeChangePropertyId },
    { &RangeValue_SmallChange_Property_GUID,              UIA_RangeValueSmallChangePropertyId },
    { &Scroll_HorizontalScrollPercent_Property_GUID,      UIA_ScrollHorizontalScrollPercentPropertyId },
    { &Scroll_HorizontalViewSize_Property_GUID,           UIA_ScrollHorizontalViewSizePropertyId },
    { &Scroll_VerticalScrollPercent_Property_GUID,        UIA_ScrollVerticalScrollPercentPropertyId },
    { &Scroll_VerticalViewSize_Property_GUID,             UIA_ScrollVerticalViewSizePropertyId },
    { &Scroll_HorizontallyScrollable_Property_GUID,       UIA_ScrollHorizontallyScrollablePropertyId },
    { &Scroll_VerticallyScrollable_Property_GUID,         UIA_ScrollVerticallyScrollablePropertyId },
    { &Selection_Selection_Property_GUID,                 UIA_SelectionSelectionPropertyId },
    { &Selection_CanSelectMultiple_Property_GUID,         UIA_SelectionCanSelectMultiplePropertyId },
    { &Selection_IsSelectionRequired_Property_GUID,       UIA_SelectionIsSelectionRequiredPropertyId },
    { &Grid_RowCount_Property_GUID,                       UIA_GridRowCountPropertyId },
    { &Grid_ColumnCount_Property_GUID,                    UIA_GridColumnCountPropertyId },
    { &GridItem_Row_Property_GUID,                        UIA_GridItemRowPropertyId },
    { &GridItem_Column_Property_GUID,                     UIA_GridItemColumnPropertyId },
    { &GridItem_RowSpan_Property_GUID,                    UIA_GridItemRowSpanPropertyId },
    { &GridItem_ColumnSpan_Property_GUID,                 UIA_GridItemColumnSpanPropertyId },
    { &GridItem_Parent_Property_GUID,                     UIA_GridItemContainingGridPropertyId },
    { &Dock_DockPosition_Property_GUID,                   UIA_DockDockPositionPropertyId },
    { &ExpandCollapse_ExpandCollapseState_Property_GUID,  UIA_ExpandCollapseExpandCollapseStatePropertyId },
    { &MultipleView_CurrentView_Property_GUID,            UIA_MultipleViewCurrentViewPropertyId },
    { &MultipleView_SupportedViews_Property_GUID,         UIA_MultipleViewSupportedViewsPropertyId },
    { &Window_CanMaximize_Property_GUID,                  UIA_WindowCanMaximizePropertyId },
    { &Window_CanMinimize_Property_GUID,                  UIA_WindowCanMinimizePropertyId },
    { &Window_WindowVisualState_Property_GUID,            UIA_WindowWindowVisualStatePropertyId },
    { &Window_WindowInteractionState_Property_GUID,       UIA_WindowWindowInteractionStatePropertyId },
    { &Window_IsModal_Property_GUID,                      UIA_WindowIsModalPropertyId },
    { &Window_IsTopmost_Property_GUID,                    UIA_WindowIsTopmostPropertyId },
    { &SelectionItem_IsSelected_Property_GUID,            UIA_SelectionItemIsSelectedPropertyId },
    { &SelectionItem_SelectionContainer_Property_GUID,    UIA_SelectionItemSelectionContainerPropertyId },
    { &Table_RowHeaders_Property_GUID,                    UIA_TableRowHeadersPropertyId },
    { &Table_ColumnHeaders_Property_GUID,                 UIA_TableColumnHeadersPropertyId },
    { &Table_RowOrColumnMajor_Property_GUID,              UIA_TableRowOrColumnMajorPropertyId },
    { &TableItem_RowHeaderItems_Property_GUID,            UIA_TableItemRowHeaderItemsPropertyId },
    { &TableItem_ColumnHeaderItems_Property_GUID,         UIA_TableItemColumnHeaderItemsPropertyId },
    { &Toggle_ToggleState_Property_GUID,                  UIA_ToggleToggleStatePropertyId },
    { &Transform_CanMove_Property_GUID,                   UIA_TransformCanMovePropertyId },
    { &Transform_CanResize_Property_GUID,                 UIA_TransformCanResizePropertyId },
    { &Transform_CanRotate_Property_GUID,                 UIA_TransformCanRotatePropertyId },
    { &IsLegacyIAccessiblePatternAvailable_Property_GUID, UIA_IsLegacyIAccessiblePatternAvailablePropertyId },
    { &LegacyIAccessible_ChildId_Property_GUID,           UIA_LegacyIAccessibleChildIdPropertyId },
    { &LegacyIAccessible_Name_Property_GUID,              UIA_LegacyIAccessibleNamePropertyId },
    { &LegacyIAccessible_Value_Property_GUID,             UIA_LegacyIAccessibleValuePropertyId },
    { &LegacyIAccessible_Description_Property_GUID,       UIA_LegacyIAccessibleDescriptionPropertyId },
    { &LegacyIAccessible_Role_Property_GUID,              UIA_LegacyIAccessibleRolePropertyId },
    { &LegacyIAccessible_State_Property_GUID,             UIA_LegacyIAccessibleStatePropertyId },
    { &LegacyIAccessible_Help_Property_GUID,              UIA_LegacyIAccessibleHelpPropertyId },
    { &LegacyIAccessible_KeyboardShortcut_Property_GUID,  UIA_LegacyIAccessibleKeyboardShortcutPropertyId },
    { &LegacyIAccessible_Selection_Property_GUID,         UIA_LegacyIAccessibleSelectionPropertyId },
    { &LegacyIAccessible_DefaultAction_Property_GUID,     UIA_LegacyIAccessibleDefaultActionPropertyId },
    { &AriaRole_Property_GUID,                            UIA_AriaRolePropertyId },
    { &AriaProperties_Property_GUID,                      UIA_AriaPropertiesPropertyId },
    { &IsDataValidForForm_Property_GUID,                  UIA_IsDataValidForFormPropertyId },
    { &ControllerFor_Property_GUID,                       UIA_ControllerForPropertyId },
    { &DescribedBy_Property_GUID,                         UIA_DescribedByPropertyId },
    { &FlowsTo_Property_GUID,                             UIA_FlowsToPropertyId },
    { &ProviderDescription_Property_GUID,                 UIA_ProviderDescriptionPropertyId },
    { &IsItemContainerPatternAvailable_Property_GUID,     UIA_IsItemContainerPatternAvailablePropertyId },
    { &IsVirtualizedItemPatternAvailable_Property_GUID,   UIA_IsVirtualizedItemPatternAvailablePropertyId },
    { &IsSynchronizedInputPatternAvailable_Property_GUID, UIA_IsSynchronizedInputPatternAvailablePropertyId },
    /* Implemented on Win8+ */
    { &OptimizeForVisualContent_Property_GUID,            UIA_OptimizeForVisualContentPropertyId },
    { &IsObjectModelPatternAvailable_Property_GUID,       UIA_IsObjectModelPatternAvailablePropertyId },
    { &Annotation_AnnotationTypeId_Property_GUID,         UIA_AnnotationAnnotationTypeIdPropertyId },
    { &Annotation_AnnotationTypeName_Property_GUID,       UIA_AnnotationAnnotationTypeNamePropertyId },
    { &Annotation_Author_Property_GUID,                   UIA_AnnotationAuthorPropertyId },
    { &Annotation_DateTime_Property_GUID,                 UIA_AnnotationDateTimePropertyId },
    { &Annotation_Target_Property_GUID,                   UIA_AnnotationTargetPropertyId },
    { &IsAnnotationPatternAvailable_Property_GUID,        UIA_IsAnnotationPatternAvailablePropertyId },
    { &IsTextPattern2Available_Property_GUID,             UIA_IsTextPattern2AvailablePropertyId },
    { &Styles_StyleId_Property_GUID,                      UIA_StylesStyleIdPropertyId },
    { &Styles_StyleName_Property_GUID,                    UIA_StylesStyleNamePropertyId },
    { &Styles_FillColor_Property_GUID,                    UIA_StylesFillColorPropertyId },
    { &Styles_FillPatternStyle_Property_GUID,             UIA_StylesFillPatternStylePropertyId },
    { &Styles_Shape_Property_GUID,                        UIA_StylesShapePropertyId },
    { &Styles_FillPatternColor_Property_GUID,             UIA_StylesFillPatternColorPropertyId },
    { &Styles_ExtendedProperties_Property_GUID,           UIA_StylesExtendedPropertiesPropertyId },
    { &IsStylesPatternAvailable_Property_GUID,            UIA_IsStylesPatternAvailablePropertyId },
    { &IsSpreadsheetPatternAvailable_Property_GUID,       UIA_IsSpreadsheetPatternAvailablePropertyId },
    { &SpreadsheetItem_Formula_Property_GUID,             UIA_SpreadsheetItemFormulaPropertyId },
    { &SpreadsheetItem_AnnotationObjects_Property_GUID,   UIA_SpreadsheetItemAnnotationObjectsPropertyId },
    { &SpreadsheetItem_AnnotationTypes_Property_GUID,     UIA_SpreadsheetItemAnnotationTypesPropertyId },
    { &IsSpreadsheetItemPatternAvailable_Property_GUID,   UIA_IsSpreadsheetItemPatternAvailablePropertyId },
    { &Transform2_CanZoom_Property_GUID,                  UIA_Transform2CanZoomPropertyId },
    { &IsTransformPattern2Available_Property_GUID,        UIA_IsTransformPattern2AvailablePropertyId },
    { &LiveSetting_Property_GUID,                         UIA_LiveSettingPropertyId },
    { &IsTextChildPatternAvailable_Property_GUID,         UIA_IsTextChildPatternAvailablePropertyId },
    { &IsDragPatternAvailable_Property_GUID,              UIA_IsDragPatternAvailablePropertyId },
    { &Drag_IsGrabbed_Property_GUID,                      UIA_DragIsGrabbedPropertyId },
    { &Drag_DropEffect_Property_GUID,                     UIA_DragDropEffectPropertyId },
    { &Drag_DropEffects_Property_GUID,                    UIA_DragDropEffectsPropertyId },
    { &IsDropTargetPatternAvailable_Property_GUID,        UIA_IsDropTargetPatternAvailablePropertyId },
    { &DropTarget_DropTargetEffect_Property_GUID,         UIA_DropTargetDropTargetEffectPropertyId },
    { &DropTarget_DropTargetEffects_Property_GUID,        UIA_DropTargetDropTargetEffectsPropertyId },
    { &Drag_GrabbedItems_Property_GUID,                   UIA_DragGrabbedItemsPropertyId },
    { &Transform2_ZoomLevel_Property_GUID,                UIA_Transform2ZoomLevelPropertyId },
    { &Transform2_ZoomMinimum_Property_GUID,              UIA_Transform2ZoomMinimumPropertyId },
    { &Transform2_ZoomMaximum_Property_GUID,              UIA_Transform2ZoomMaximumPropertyId },
    { &FlowsFrom_Property_GUID,                           UIA_FlowsFromPropertyId },
    { &IsTextEditPatternAvailable_Property_GUID,          UIA_IsTextEditPatternAvailablePropertyId },
    { &IsPeripheral_Property_GUID,                        UIA_IsPeripheralPropertyId },
    /* Implemented on Win10v1507+. */
    { &IsCustomNavigationPatternAvailable_Property_GUID,  UIA_IsCustomNavigationPatternAvailablePropertyId },
    { &PositionInSet_Property_GUID,                       UIA_PositionInSetPropertyId },
    { &SizeOfSet_Property_GUID,                           UIA_SizeOfSetPropertyId },
    { &Level_Property_GUID,                               UIA_LevelPropertyId },
    { &AnnotationTypes_Property_GUID,                     UIA_AnnotationTypesPropertyId },
    { &AnnotationObjects_Property_GUID,                   UIA_AnnotationObjectsPropertyId },
    /* Implemented on Win10v1809+. */
    { &LandmarkType_Property_GUID,                        UIA_LandmarkTypePropertyId },
    { &LocalizedLandmarkType_Property_GUID,               UIA_LocalizedLandmarkTypePropertyId },
    { &FullDescription_Property_GUID,                     UIA_FullDescriptionPropertyId },
    { &FillColor_Property_GUID,                           UIA_FillColorPropertyId },
    { &OutlineColor_Property_GUID,                        UIA_OutlineColorPropertyId },
    { &FillType_Property_GUID,                            UIA_FillTypePropertyId },
    { &VisualEffects_Property_GUID,                       UIA_VisualEffectsPropertyId },
    { &OutlineThickness_Property_GUID,                    UIA_OutlineThicknessPropertyId },
    { &CenterPoint_Property_GUID,                         UIA_CenterPointPropertyId },
    { &Rotation_Property_GUID,                            UIA_RotationPropertyId },
    { &Size_Property_GUID,                                UIA_SizePropertyId },
    { &IsSelectionPattern2Available_Property_GUID,        UIA_IsSelectionPattern2AvailablePropertyId },
    { &Selection2_FirstSelectedItem_Property_GUID,        UIA_Selection2FirstSelectedItemPropertyId },
    { &Selection2_LastSelectedItem_Property_GUID,         UIA_Selection2LastSelectedItemPropertyId },
    { &Selection2_CurrentSelectedItem_Property_GUID,      UIA_Selection2CurrentSelectedItemPropertyId },
    { &Selection2_ItemCount_Property_GUID,                UIA_Selection2ItemCountPropertyId },
    { &HeadingLevel_Property_GUID,                        UIA_HeadingLevelPropertyId },
    { &IsDialog_Property_GUID,                            UIA_IsDialogPropertyId },
};

static void test_UiaLookupId(void)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(uia_property_lookup_ids); i++)
    {
        int prop_id = UiaLookupId(AutomationIdentifierType_Property, uia_property_lookup_ids[i].guid);

        if (!prop_id)
        {
            win_skip("No propertyId for GUID %s, skipping further tests.\n", debugstr_guid(uia_property_lookup_ids[i].guid));
            break;
        }

        ok(prop_id == uia_property_lookup_ids[i].id, "Unexpected Property id, expected %d, got %d\n",
                uia_property_lookup_ids[i].id, prop_id);
    }
}

static const struct prov_method_sequence node_from_prov1[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { 0 }
};

static const struct prov_method_sequence node_from_prov2[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    /* Only called on Windows versions past Win10v1507. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_prov3[] = {
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    /* Only called on Windows versions past Win10v1507. */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_prov4[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    /* Only called on Windows versions past Win10v1507. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_prov5[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    /* Win10v1507 and below call this. */
    { &Provider2, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_TODO },
    { &Provider2, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    /* These three are only done on Win10v1507 and below. */
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider2, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    /* This is only done on Win10v1507. */
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    /* Only called on Windows versions past Win10v1507. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    /* Win10v1507 and below call this. */
    { &Provider2, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_prov6[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    /* Win10v1507 and below call this. */
    { &Provider2, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_TODO },
    { &Provider2, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    /* Only called on Windows versions past Win10v1507. */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider2, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    /* This is only done on Win10v1507. */
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    /* Only called on Windows versions past Win10v1507. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider2, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_prov7[] = {
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    /* Win10v1507 and below call this. */
    { &Provider2, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_TODO },
    { &Provider2, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    /* Only called on Windows versions past Win10v1507. */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider2, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider_child, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    /* This is only done on Win10v1507. */
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    /* Only called on Windows versions past Win10v1507. */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider2, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_prov8[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    /* Only called on Windows versions past Win10v1507. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { 0 }
};

static void check_uia_prop_val(PROPERTYID prop_id, enum UIAutomationType type, VARIANT *v);
static DWORD WINAPI uia_node_from_provider_test_com_thread(LPVOID param)
{
    HUIANODE node = param;
    HRESULT hr;
    VARIANT v;

    /*
     * Since this is a node representing an IRawElementProviderSimple with
     * ProviderOptions_UseComThreading set, it is only usable in threads that
     * have initialized COM.
     */
    hr = UiaGetPropertyValue(node, UIA_ProcessIdPropertyId, &v);
    ok(hr == CO_E_NOTINITIALIZED, "Unexpected hr %#lx\n", hr);

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = UiaGetPropertyValue(node, UIA_ProcessIdPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_uia_prop_val(UIA_ProcessIdPropertyId, UIAutomationType_Int, &v);

    /*
     * When retrieving a UIAutomationType_Element property, if UseComThreading
     * is set, we'll get an HUIANODE that will make calls inside of the
     * apartment of the node it is retrieved from. I.e, if we received a node
     * with UseComThreading set from another node with UseComThreading set
     * inside of an STA, the returned node will have all of its methods called
     * from the STA thread.
     */
    Provider_child.prov_opts = ProviderOptions_UseComThreading | ProviderOptions_ServerSideProvider;
    Provider_child.expected_tid = Provider.expected_tid;
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_uia_prop_val(UIA_LabeledByPropertyId, UIAutomationType_Element, &v);

    /* Unset ProviderOptions_UseComThreading. */
    Provider_child.prov_opts = ProviderOptions_ServerSideProvider;
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    /*
     * ProviderOptions_UseComThreading not set, GetPropertyValue will be
     * called on the current thread.
     */
    Provider_child.expected_tid = GetCurrentThreadId();
    check_uia_prop_val(UIA_LabeledByPropertyId, UIAutomationType_Element, &v);

    CoUninitialize();

    return 0;
}

static void test_uia_node_from_prov_com_threading(void)
{
    HANDLE thread;
    HUIANODE node;
    HRESULT hr;

    /* Test ProviderOptions_UseComThreading. */
    Provider.hwnd = NULL;
    prov_root = NULL;
    Provider.prov_opts = ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading;
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok_method_sequence(node_from_prov8, "node_from_prov8");

    /*
     * On Windows versions prior to Windows 10, UiaNodeFromProvider ignores the
     * ProviderOptions_UseComThreading flag.
     */
    if (hr == S_OK)
    {
        win_skip("Skipping ProviderOptions_UseComThreading tests for UiaNodeFromProvider.\n");
        UiaNodeRelease(node);
        return;
    }
    ok(hr == CO_E_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok_method_sequence(node_from_prov8, "node_from_prov8");

    Provider.expected_tid = GetCurrentThreadId();
    thread = CreateThread(NULL, 0, uia_node_from_provider_test_com_thread, (void *)node, 0, NULL);
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;
        while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    CloseHandle(thread);

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    Provider_child.expected_tid = Provider.expected_tid = 0;

    CoUninitialize();
}
static void test_UiaNodeFromProvider(void)
{
    WNDCLASSA cls;
    HUIANODE node;
    HRESULT hr;
    ULONG ref;
    HWND hwnd;
    VARIANT v;

    cls.style = 0;
    cls.lpfnWndProc = test_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = NULL;
    cls.hbrBackground = NULL;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "UiaNodeFromProvider class";

    RegisterClassA(&cls);

    hwnd = CreateWindowA("UiaNodeFromProvider class", "Test window", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, NULL, NULL, NULL, NULL);

    /* Run these tests early, we end up in an implicit MTA later. */
    test_uia_node_from_prov_com_threading();

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = UiaNodeFromProvider(NULL, &node);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Must have a successful call to get_ProviderOptions. */
    Provider.prov_opts = 0;
    node = (void *)0xdeadbeef;
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(!node, "node != NULL\n");
    ok_method_sequence(node_from_prov1, "node_from_prov1");

    /* No HWND exposed through Provider. */
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    node = (void *)0xdeadbeef;
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
        check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider", TRUE);
        VariantClear(&v);
    }

    ok_method_sequence(node_from_prov2, "node_from_prov2");

    /* HUIANODE represents a COM interface. */
    ref = IUnknown_AddRef((IUnknown *)node);
    ok(ref == 2, "Unexpected refcnt %ld\n", ref);

    ref = IUnknown_AddRef((IUnknown *)node);
    ok(ref == 3, "Unexpected refcnt %ld\n", ref);

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");

    ref = IUnknown_Release((IUnknown *)node);
    ok(ref == 1, "Unexpected refcnt %ld\n", ref);

    ref = IUnknown_Release((IUnknown *)node);
    ok(ref == 0, "Unexpected refcnt %ld\n", ref);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);

    /*
     * No HWND exposed through Provider_child, but it returns a parent from
     * NavigateDirection_Parent. Behavior doesn't change.
     */
    Provider_child.prov_opts = ProviderOptions_ServerSideProvider;
    node = (void *)0xdeadbeef;
    hr = UiaNodeFromProvider(&Provider_child.IRawElementProviderSimple_iface, &node);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
        check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child", TRUE);
        VariantClear(&v);
    }

    ok_method_sequence(node_from_prov3, "node_from_prov3");
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

    /* HWND exposed, but Provider2 not returned from WM_GETOBJECT. */
    Provider.hwnd = hwnd;
    prov_root = NULL;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Win10v1507 and below send this, Windows 7 sends it twice. */
    SET_EXPECT_MULTI(winproc_GETOBJECT_CLIENT, 2);
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);

        /* Newer versions of Windows have "Hwnd(parent link):" */
        if (get_provider_desc(V_BSTR(&v), L"Hwnd(parent link):", NULL))
        {
            check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider", FALSE);
            check_node_provider_desc(V_BSTR(&v), L"Nonclient", NULL, FALSE);
            check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, TRUE);
        }
        else
        {
            check_node_provider_desc(V_BSTR(&v), L"Annotation", NULL, TRUE);
            check_node_provider_desc(V_BSTR(&v), L"Main", NULL, FALSE);
            check_node_provider_desc(V_BSTR(&v), L"Nonclient", NULL, FALSE);
            check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider", FALSE);
        }
        VariantClear(&v);
    }

    ok_method_sequence(node_from_prov4, "node_from_prov4");

    ok(!!node, "node == NULL\n");
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);

    /* Return Provider2 in response to WM_GETOBJECT. */
    Provider.hwnd = Provider2.hwnd = hwnd;
    Provider.prov_opts = Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    prov_root = &Provider2.IRawElementProviderSimple_iface;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Windows 7 sends this. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    /* Win10v1507 and below hold a reference to the root provider for the HWND */
    ok(broken(Provider2.ref == 2) || Provider2.ref == 1, "Unexpected refcnt %ld\n", Provider2.ref);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok(!!node, "node == NULL\n");

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);

        /* Newer versions of Windows have "Hwnd(parent link):" */
        if (get_provider_desc(V_BSTR(&v), L"Hwnd(parent link):", NULL))
        {
            check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider", FALSE);
            check_node_provider_desc(V_BSTR(&v), L"Nonclient", NULL, FALSE);
            check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, TRUE);
        }
        else
        {
            check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider2", TRUE);
            check_node_provider_desc(V_BSTR(&v), L"Nonclient", NULL, FALSE);
            check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider", FALSE);
        }
        VariantClear(&v);
    }

    ok_method_sequence(node_from_prov5, "node_from_prov5");

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider2.ref == 1, "Unexpected refcnt %ld\n", Provider2.ref);

    /*
     * Windows 10 newer than v1507 only matches older behavior if
     * Provider is a ClientSideProvider.
     */
    Provider.prov_opts = ProviderOptions_ClientSideProvider;
    Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    prov_root = &Provider2.IRawElementProviderSimple_iface;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Windows 7 sends this. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
        check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider2", TRUE);
        check_node_provider_desc(V_BSTR(&v), L"Nonclient", NULL, FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider", FALSE);
        VariantClear(&v);
    }
    ok_method_sequence(node_from_prov6, "node_from_prov6");

    todo_wine ok(Provider2.ref == 2, "Unexpected refcnt %ld\n", Provider2.ref);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    ok(!!node, "node == NULL\n");
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider2.ref == 1, "Unexpected refcnt %ld\n", Provider2.ref);

    /* Provider_child has a parent, so it will be "(parent link)". */
    Provider_child.prov_opts = ProviderOptions_ClientSideProvider;
    Provider_child.hwnd = hwnd;
    Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    prov_root = &Provider2.IRawElementProviderSimple_iface;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Windows 7 sends this. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = UiaNodeFromProvider(&Provider_child.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
        check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider2", FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Nonclient", NULL, FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider_child", TRUE);
        VariantClear(&v);
    }
    ok_method_sequence(node_from_prov7, "node_from_prov7");

    todo_wine ok(Provider2.ref == 2, "Unexpected refcnt %ld\n", Provider2.ref);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    ok(!!node, "node == NULL\n");
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider2.ref == 1, "Unexpected refcnt %ld\n", Provider2.ref);

    CoUninitialize();
    DestroyWindow(hwnd);
    UnregisterClassA("UiaNodeFromProvider class", NULL);
    prov_root = NULL;
}

/* Sequence for types other than UIAutomationType_Element. */
static const struct prov_method_sequence get_prop_seq[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    { 0 }
};

/* Sequence for getting a property that returns an invalid type. */
static const struct prov_method_sequence get_prop_invalid_type_seq[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    /* Windows 7 calls this. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { 0 }
};

/* UIAutomationType_Element sequence. */
static const struct prov_method_sequence get_elem_prop_seq[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    /* Only called on Windows versions past Win10v1507. */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL },
    { 0 }
};

/* UIAutomationType_ElementArray sequence. */
static const struct prov_method_sequence get_elem_arr_prop_seq[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider_child2, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child2, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider_child2, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider_child, PROV_GET_PROPERTY_VALUE },
    { &Provider_child2, PROV_GET_PROPERTY_VALUE },
    { 0 }
};

static void check_uia_prop_val(PROPERTYID prop_id, enum UIAutomationType type, VARIANT *v)
{
    LONG idx;

    switch (type)
    {
    case UIAutomationType_String:
        ok(V_VT(v) == VT_BSTR, "Unexpected VT %d\n", V_VT(v));
        ok(!lstrcmpW(V_BSTR(v), uia_bstr_prop_str), "Unexpected BSTR %s\n", wine_dbgstr_w(V_BSTR(v)));
        ok_method_sequence(get_prop_seq, NULL);
        break;

    case UIAutomationType_Bool:
        ok(V_VT(v) == VT_BOOL, "Unexpected VT %d\n", V_VT(v));

        /* UIA_IsKeyboardFocusablePropertyId is broken on Win8 and Win10v1507. */
        if (prop_id == UIA_IsKeyboardFocusablePropertyId)
            ok(check_variant_bool(v, TRUE) || broken(check_variant_bool(v, FALSE)),
                    "Unexpected BOOL %#x\n", V_BOOL(v));
        else
            ok(check_variant_bool(v, TRUE), "Unexpected BOOL %#x\n", V_BOOL(v));
        ok_method_sequence(get_prop_seq, NULL);
        break;

    case UIAutomationType_Int:
        ok(V_VT(v) == VT_I4, "Unexpected VT %d\n", V_VT(v));

        if (prop_id == UIA_NativeWindowHandlePropertyId)
            ok(ULongToHandle(V_I4(v)) == Provider.hwnd, "Unexpected I4 %#lx\n", V_I4(v));
        else
            ok(V_I4(v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(v));
        ok_method_sequence(get_prop_seq, NULL);
        break;

    case UIAutomationType_IntArray:
        ok(V_VT(v) == (VT_ARRAY | VT_I4), "Unexpected VT %d\n", V_VT(v));

        for (idx = 0; idx < ARRAY_SIZE(uia_i4_arr_prop_val); idx++)
        {
            ULONG val;

            SafeArrayGetElement(V_ARRAY(v), &idx, &val);
            ok(val == uia_i4_arr_prop_val[idx], "Unexpected I4 %#lx at idx %ld\n", val, idx);
        }
        ok_method_sequence(get_prop_seq, NULL);
        break;

    case UIAutomationType_Double:
        ok(V_VT(v) == VT_R8, "Unexpected VT %d\n", V_VT(v));
        ok(V_R8(v) == uia_r8_prop_val, "Unexpected R8 %lf\n", V_R8(v));
        ok_method_sequence(get_prop_seq, NULL);
        break;

    case UIAutomationType_DoubleArray:
        ok(V_VT(v) == (VT_ARRAY | VT_R8), "Unexpected VT %d\n", V_VT(v));
        for (idx = 0; idx < ARRAY_SIZE(uia_r8_arr_prop_val); idx++)
        {
            double val;

            SafeArrayGetElement(V_ARRAY(v), &idx, &val);
            ok(val == uia_r8_arr_prop_val[idx], "Unexpected R8 %lf at idx %ld\n", val, idx);
        }
        ok_method_sequence(get_prop_seq, NULL);
        break;

    case UIAutomationType_Element:
    {
        HUIANODE tmp_node;
        HRESULT hr;
        VARIANT v1;

#ifdef _WIN64
        ok(V_VT(v) == VT_I8, "Unexpected VT %d\n", V_VT(v));
        tmp_node = (HUIANODE)V_I8(v);
#else
        ok(V_VT(v) == VT_I4, "Unexpected VT %d\n", V_VT(v));
        tmp_node = (HUIANODE)V_I4(v);
#endif
        ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

        hr = UiaGetPropertyValue(tmp_node, UIA_ControlTypePropertyId, &v1);
        ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
        ok(V_VT(&v1) == VT_I4, "Unexpected VT %d\n", V_VT(&v1));
        ok(V_I4(&v1) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v1));

        ok(UiaNodeRelease(tmp_node), "Failed to release node\n");
        ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);
        ok_method_sequence(get_elem_prop_seq, NULL);
        break;
    }

    case UIAutomationType_ElementArray:
        ok(V_VT(v) == (VT_ARRAY | VT_UNKNOWN), "Unexpected VT %d\n", V_VT(v));
        if (V_VT(v) != (VT_ARRAY | VT_UNKNOWN))
            break;

        ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);
        ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);
        for (idx = 0; idx < ARRAY_SIZE(uia_unk_arr_prop_val); idx++)
        {
            HUIANODE tmp_node;
            HRESULT hr;
            VARIANT v1;

            SafeArrayGetElement(V_ARRAY(v), &idx, &tmp_node);

            hr = UiaGetPropertyValue(tmp_node, UIA_ControlTypePropertyId, &v1);
            ok(hr == S_OK, "node[%ld] Unexpected hr %#lx\n", idx, hr);
            ok(V_VT(&v1) == VT_I4, "node[%ld] Unexpected VT %d\n", idx, V_VT(&v1));
            ok(V_I4(&v1) == uia_i4_prop_val, "node[%ld] Unexpected I4 %#lx\n", idx, V_I4(&v1));

            ok(UiaNodeRelease(tmp_node), "Failed to release node[%ld]\n", idx);
            VariantClear(&v1);
        }

        VariantClear(v);
        ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);
        ok(Provider_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_child2.ref);
        ok_method_sequence(get_elem_arr_prop_seq, NULL);
        break;

    default:
        break;
    }

    VariantClear(v);
    V_VT(v) = VT_EMPTY;
}

struct uia_element_property {
    const GUID *prop_guid;
    enum UIAutomationType type;
    BOOL skip_invalid;
};

static const struct uia_element_property element_properties[] = {
    { &ProcessId_Property_GUID,                UIAutomationType_Int, TRUE },
    { &ControlType_Property_GUID,              UIAutomationType_Int },
    { &LocalizedControlType_Property_GUID,     UIAutomationType_String, TRUE },
    { &Name_Property_GUID,                     UIAutomationType_String },
    { &AcceleratorKey_Property_GUID,           UIAutomationType_String },
    { &AccessKey_Property_GUID,                UIAutomationType_String },
    { &HasKeyboardFocus_Property_GUID,         UIAutomationType_Bool },
    { &IsKeyboardFocusable_Property_GUID,      UIAutomationType_Bool },
    { &IsEnabled_Property_GUID,                UIAutomationType_Bool },
    { &AutomationId_Property_GUID,             UIAutomationType_String },
    { &ClassName_Property_GUID,                UIAutomationType_String },
    { &HelpText_Property_GUID,                 UIAutomationType_String },
    { &Culture_Property_GUID,                  UIAutomationType_Int },
    { &IsControlElement_Property_GUID,         UIAutomationType_Bool },
    { &IsContentElement_Property_GUID,         UIAutomationType_Bool },
    { &LabeledBy_Property_GUID,                UIAutomationType_Element },
    { &IsPassword_Property_GUID,               UIAutomationType_Bool },
    { &NewNativeWindowHandle_Property_GUID,    UIAutomationType_Int },
    { &ItemType_Property_GUID,                 UIAutomationType_String },
    { &IsOffscreen_Property_GUID,              UIAutomationType_Bool },
    { &Orientation_Property_GUID,              UIAutomationType_Int },
    { &FrameworkId_Property_GUID,              UIAutomationType_String },
    { &IsRequiredForForm_Property_GUID,        UIAutomationType_Bool },
    { &ItemStatus_Property_GUID,               UIAutomationType_String },
    { &AriaRole_Property_GUID,                 UIAutomationType_String },
    { &AriaProperties_Property_GUID,           UIAutomationType_String },
    { &IsDataValidForForm_Property_GUID,       UIAutomationType_Bool },
    { &ControllerFor_Property_GUID,            UIAutomationType_ElementArray },
    { &DescribedBy_Property_GUID,              UIAutomationType_ElementArray },
    { &FlowsTo_Property_GUID,                  UIAutomationType_ElementArray },
    /* Implemented on Win8+ */
    { &OptimizeForVisualContent_Property_GUID, UIAutomationType_Bool },
    { &LiveSetting_Property_GUID,              UIAutomationType_Int },
    { &FlowsFrom_Property_GUID,                UIAutomationType_ElementArray },
    { &IsPeripheral_Property_GUID,             UIAutomationType_Bool },
    /* Implemented on Win10v1507+. */
    { &PositionInSet_Property_GUID,            UIAutomationType_Int },
    { &SizeOfSet_Property_GUID,                UIAutomationType_Int },
    { &Level_Property_GUID,                    UIAutomationType_Int },
    { &AnnotationTypes_Property_GUID,          UIAutomationType_IntArray },
    { &AnnotationObjects_Property_GUID,        UIAutomationType_ElementArray },
    /* Implemented on Win10v1809+. */
    { &LandmarkType_Property_GUID,             UIAutomationType_Int },
    { &LocalizedLandmarkType_Property_GUID,    UIAutomationType_String, TRUE },
    { &FullDescription_Property_GUID,          UIAutomationType_String },
    { &FillColor_Property_GUID,                UIAutomationType_Int },
    { &OutlineColor_Property_GUID,             UIAutomationType_IntArray },
    { &FillType_Property_GUID,                 UIAutomationType_Int },
    { &VisualEffects_Property_GUID,            UIAutomationType_Int },
    { &OutlineThickness_Property_GUID,         UIAutomationType_DoubleArray },
    { &Rotation_Property_GUID,                 UIAutomationType_Double },
    { &Size_Property_GUID,                     UIAutomationType_DoubleArray },
    { &HeadingLevel_Property_GUID,             UIAutomationType_Int },
    { &IsDialog_Property_GUID,                 UIAutomationType_Bool },
};

static void test_UiaGetPropertyValue(void)
{
    const struct uia_element_property *elem_prop;
    IUnknown *unk_ns;
    unsigned int i;
    HUIANODE node;
    int prop_id;
    HRESULT hr;
    VARIANT v;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    Provider_child.prov_opts = Provider_child2.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = Provider_child.hwnd = Provider_child2.hwnd = NULL;
    node = (void *)0xdeadbeef;
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok_method_sequence(node_from_prov8, NULL);

    hr = UiaGetReservedNotSupportedValue(&unk_ns);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(element_properties); i++)
    {
        elem_prop = &element_properties[i];

        Provider.ret_invalid_prop_type = FALSE;
        VariantClear(&v);
        prop_id = UiaLookupId(AutomationIdentifierType_Property, elem_prop->prop_guid);
        if (!prop_id)
        {
            win_skip("No propertyId for GUID %s, skipping further tests.\n", debugstr_guid(elem_prop->prop_guid));
            break;
        }
        winetest_push_context("prop_id %d", prop_id);
        hr = UiaGetPropertyValue(node, prop_id, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_uia_prop_val(prop_id, elem_prop->type, &v);

        /*
         * Some properties have special behavior if an invalid value is
         * returned, skip them here.
         */
        if (!elem_prop->skip_invalid)
        {
            Provider.ret_invalid_prop_type = TRUE;
            hr = UiaGetPropertyValue(node, prop_id, &v);
            if (hr == E_NOTIMPL)
                todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            else
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            if (SUCCEEDED(hr))
            {
                ok_method_sequence(get_prop_invalid_type_seq, NULL);
                ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
                ok(V_UNKNOWN(&v) == unk_ns, "unexpected IUnknown %p\n", V_UNKNOWN(&v));
                VariantClear(&v);
            }
        }

        winetest_pop_context();
    }

    Provider.ret_invalid_prop_type = FALSE;
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);

    IUnknown_Release(unk_ns);
    CoUninitialize();
}

static const struct prov_method_sequence get_runtime_id1[] = {
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { 0 }
};

static const struct prov_method_sequence get_runtime_id2[] = {
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT },
    { 0 }
};

static const struct prov_method_sequence get_runtime_id3[] = {
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    /* Not called on Windows 7. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    /* Only called on Win8+. */
    { &Provider, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { &Provider2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider2, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider2, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { 0 }
};

static const struct prov_method_sequence get_runtime_id4[] = {
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    /* Not called on Windows 7. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    /* These methods are only called on Win8+. */
    { &Provider, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { &Provider2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { 0 }
};

static const struct prov_method_sequence get_runtime_id5[] = {
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { 0 }
};

static void test_UiaGetRuntimeId(void)
{
    const int root_prov_opts[] = { ProviderOptions_ServerSideProvider, ProviderOptions_ServerSideProvider | ProviderOptions_OverrideProvider,
                                   ProviderOptions_ClientSideProvider | ProviderOptions_NonClientAreaProvider };
    int rt_id[4], tmp, i;
    IUnknown *unk_ns;
    SAFEARRAY *sa;
    WNDCLASSA cls;
    HUIANODE node;
    HRESULT hr;
    HWND hwnd;
    VARIANT v;

    VariantInit(&v);
    cls.style = 0;
    cls.lpfnWndProc = test_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = NULL;
    cls.hbrBackground = NULL;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "UiaGetRuntimeId class";

    RegisterClassA(&cls);

    hwnd = CreateWindowA("UiaGetRuntimeId class", "Test window", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, NULL, NULL, NULL, NULL);

    hr = UiaGetReservedNotSupportedValue(&unk_ns);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    Provider.prov_opts = Provider2.prov_opts = Provider_child.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = Provider2.hwnd = Provider_child.hwnd = NULL;
    node = (void *)0xdeadbeef;
    hr = UiaNodeFromProvider(&Provider_child.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
        check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child", TRUE);
        VariantClear(&v);
    }
    ok_method_sequence(node_from_prov3, NULL);

    /* NULL runtime ID. */
    Provider_child.runtime_id[0] = Provider_child.runtime_id[1] = 0;
    sa = (void *)0xdeadbeef;
    hr = UiaGetRuntimeId(node, &sa);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!sa, "sa != NULL\n");
    ok_method_sequence(get_runtime_id1, "get_runtime_id1");

    /* No UiaAppendRuntimeId prefix, returns GetRuntimeId array directly. */
    Provider_child.runtime_id[0] = rt_id[0] = 5;
    Provider_child.runtime_id[1] = rt_id[1] = 2;
    sa = (void *)0xdeadbeef;
    hr = UiaGetRuntimeId(node, &sa);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!sa, "sa == NULL\n");
    check_runtime_id(rt_id, 2, sa);
    SafeArrayDestroy(sa);
    ok_method_sequence(get_runtime_id1, "get_runtime_id1");

    /*
     * If a provider returns a RuntimeId beginning with the constant
     * UiaAppendRuntimeId, UI Automation will add a prefix based on the
     * providers HWND fragment root before returning to the client. The added
     * prefix is 3 int values, with:
     *
     * idx[0] always being 42. (UIA_RUNTIME_ID_PREFIX)
     * idx[1] always being the HWND.
     * idx[2] has three different values depending on what type of provider
     * the fragment root is. Fragment roots can be an override provider, a
     * nonclient provider, or a main provider.
     */
    Provider_child.frag_root = NULL;
    Provider_child.runtime_id[0] = UiaAppendRuntimeId;
    Provider_child.runtime_id[1] = 2;
    sa = (void *)0xdeadbeef;

    /* Provider_child has no fragment root for UiaAppendRuntimeId. */
    hr = UiaGetRuntimeId(node, &sa);
    /* Windows 7 returns S_OK. */
    ok(hr == E_FAIL || broken(hr == S_OK), "Unexpected hr %#lx.\n", hr);
    ok(!sa, "sa != NULL\n");
    ok_method_sequence(get_runtime_id2, "get_runtime_id2");

    /*
     * UIA_RuntimeIdPropertyId won't return a failure code from
     * UiaGetPropertyValue.
     */
    hr = UiaGetPropertyValue(node, UIA_RuntimeIdPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
    ok(V_UNKNOWN(&v) == unk_ns, "unexpected IUnknown %p\n", V_UNKNOWN(&v));
    VariantClear(&v);
    ok_method_sequence(get_runtime_id2, "get_runtime_id2");

    /*
     * Provider_child returns a fragment root that doesn't expose an HWND. On
     * Win8+, fragment roots are navigated recursively until either a NULL
     * fragment root is returned, the same fragment root as the current one is
     * returned, or a fragment root with an HWND is returned.
     */
    Provider_child.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    Provider2.hwnd = NULL;
    Provider.frag_root = &Provider2.IRawElementProviderFragmentRoot_iface;
    Provider.hwnd = NULL;
    Provider_child.runtime_id[0] = UiaAppendRuntimeId;
    Provider_child.runtime_id[1] = 2;
    sa = (void *)0xdeadbeef;
    hr = UiaGetRuntimeId(node, &sa);
    /* Windows 7 returns S_OK. */
    ok(hr == E_FAIL || broken(hr == S_OK), "Unexpected hr %#lx.\n", hr);
    ok(!sa, "sa != NULL\n");
    ok_method_sequence(get_runtime_id3, "get_runtime_id3");

    /* Provider2 returns an HWND. */
    Provider.hwnd = NULL;
    Provider.frag_root = &Provider2.IRawElementProviderFragmentRoot_iface;
    Provider2.hwnd = hwnd;
    Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    Provider_child.runtime_id[0] = UiaAppendRuntimeId;
    Provider_child.runtime_id[1] = rt_id[3] = 2;
    sa = NULL;
    hr = UiaGetRuntimeId(node, &sa);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* Windows 7 returns a NULL RuntimeId due to no fragment recursion. */
    ok(!!sa || broken(!sa), "sa == NULL\n");
    SafeArrayDestroy(sa);

    ok_method_sequence(get_runtime_id4, "get_runtime_id4");

    /* Test RuntimeId values for different root fragment provider types. */
    Provider.frag_root = NULL;
    Provider.hwnd = hwnd;
    Provider_child.runtime_id[0] = UiaAppendRuntimeId;
    Provider_child.runtime_id[1] = rt_id[3] = 2;
    rt_id[0] = UIA_RUNTIME_ID_PREFIX;
    rt_id[1] = HandleToULong(hwnd);
    for (i = 0; i < ARRAY_SIZE(root_prov_opts); i++)
    {
        LONG lbound;

        Provider.prov_opts = root_prov_opts[i];
        sa = NULL;
        hr = UiaGetRuntimeId(node, &sa);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!!sa, "sa == NULL\n");

        hr = SafeArrayGetLBound(sa, 1, &lbound);
        ok(hr == S_OK, "Failed to get LBound with hr %#lx\n", hr);

        lbound = lbound + 2;
        hr = SafeArrayGetElement(sa, &lbound, &tmp);
        ok(hr == S_OK, "Failed to get element with hr %#lx\n", hr);
        if (i)
            ok(rt_id[2] != tmp, "Expected different runtime id value from previous\n");

        rt_id[2] = tmp;
        check_runtime_id(rt_id, 4, sa);

        SafeArrayDestroy(sa);
        ok_method_sequence(get_runtime_id5, "get_runtime_id5");
    }

    UiaNodeRelease(node);

    /* Test behavior on a node with an associated HWND. */
    Provider.prov_opts = ProviderOptions_ClientSideProvider;
    Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = Provider2.hwnd = hwnd;
    prov_root = &Provider2.IRawElementProviderSimple_iface;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Windows 7 sends this. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    VariantInit(&v);
    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
        check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider2", TRUE);
        check_node_provider_desc(V_BSTR(&v), L"Nonclient", NULL, FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider", FALSE);
        VariantClear(&v);
    }
    ok_method_sequence(node_from_prov6, "node_from_prov6");

    /* No methods called, RuntimeId is based on the node's HWND. */
    hr = UiaGetRuntimeId(node, &sa);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!sa, "sa == NULL\n");
    ok(!sequence_cnt, "Unexpected sequence_cnt %d\n", sequence_cnt);

    rt_id[0] = UIA_RUNTIME_ID_PREFIX;
    rt_id[1] = HandleToULong(hwnd);
    check_runtime_id(rt_id, 2, sa);
    SafeArrayDestroy(sa);
    UiaNodeRelease(node);

    DestroyWindow(hwnd);
    UnregisterClassA("UiaGetRuntimeId class", NULL);
    CoUninitialize();
}

static LONG Object_ref = 1;
static HRESULT WINAPI Object_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI Object_AddRef(IUnknown *iface)
{
    return InterlockedIncrement(&Object_ref);
}

static ULONG WINAPI Object_Release(IUnknown *iface)
{
    return InterlockedDecrement(&Object_ref);
}

static IUnknownVtbl ObjectVtbl = {
    Object_QueryInterface,
    Object_AddRef,
    Object_Release
};

static IUnknown Object = {&ObjectVtbl};
static void test_UiaHUiaNodeFromVariant(void)
{
    HUIANODE node;
    HRESULT hr;
    VARIANT v;

    hr = UiaHUiaNodeFromVariant(NULL, &node);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = UiaHUiaNodeFromVariant(&v, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    node = (void *)0xdeadbeef;
    V_VT(&v) = VT_R8;
    hr = UiaHUiaNodeFromVariant(&v, &node);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!node, "node != NULL\n");

    node = (void *)0xdeadbeef;
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = &Object;
    hr = UiaHUiaNodeFromVariant(&v, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node == (void *)&Object, "node != NULL\n");
    ok(Object_ref == 2, "Unexpected Object_ref %ld\n", Object_ref);
    VariantClear(&v);

#ifdef _WIN64
    node = (void *)0xdeadbeef;
    V_VT(&v) = VT_I4;
    V_I4(&v) = 0xbeefdead;
    hr = UiaHUiaNodeFromVariant(&v, &node);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!node, "node != NULL\n");

    node = (void *)0xdeadbeef;
    V_VT(&v) = VT_I8;
    V_I8(&v) = 0xbeefdead;
    hr = UiaHUiaNodeFromVariant(&v, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node == (void *)V_I8(&v), "node != V_I8\n");
#else
    node = (void *)0xdeadbeef;
    V_VT(&v) = VT_I8;
    V_I8(&v) = 0xbeefdead;
    hr = UiaHUiaNodeFromVariant(&v, &node);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!node, "node != NULL\n");

    node = (void *)0xdeadbeef;
    V_VT(&v) = VT_I4;
    V_I4(&v) = 0xbeefdead;
    hr = UiaHUiaNodeFromVariant(&v, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node == (void *)V_I4(&v), "node != V_I8\n");
#endif
}

static const struct prov_method_sequence node_from_hwnd1[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd2[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    /* Windows 10+ calls this. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd3[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd4[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    /* Only called on Windows versions past Win10v1507. */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ControlTypePropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd5[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_LabeledByPropertyId */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    /* Only done in Windows 8+. */
    { &Provider_child, FRAG_GET_RUNTIME_ID, METHOD_OPTIONAL },
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    /* These two are only done on Windows 7. */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_child, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd6[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    /* Next 4 are only done in Windows 8+. */
    { &Provider_child, FRAG_GET_RUNTIME_ID, METHOD_OPTIONAL },
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    /* Next two are only done on Win10v1809+. */
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    /* Next two are only done on Win10v1809+. */
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ControlTypePropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd7[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_TODO },
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd8[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ControlTypePropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd9[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    /* Only done in Windows 8+. */
    { &Provider, FRAG_GET_RUNTIME_ID, METHOD_OPTIONAL },
    { &Provider, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    /* These three are only done on Windows 7. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence disconnect_prov1[] = {
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { 0 }
};

static const struct prov_method_sequence disconnect_prov2[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, FRAG_GET_FRAGMENT_ROOT },
    { 0 }
};

static const struct prov_method_sequence disconnect_prov3[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider, FRAG_GET_RUNTIME_ID },
    { 0 }
};

static const struct prov_method_sequence disconnect_prov4[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { 0 }
};

static void test_UiaNodeFromHandle_client_proc(void)
{
    APTTYPEQUALIFIER apt_qualifier;
    APTTYPE apt_type;
    WCHAR buf[2048];
    HUIANODE node;
    HRESULT hr;
    HWND hwnd;
    VARIANT v;

    hwnd = FindWindowA("UiaNodeFromHandle class", "Test window");

    hr = CoGetApartmentType(&apt_type, &apt_qualifier);
    ok(hr == CO_E_NOTINITIALIZED, "Unexpected hr %#lx\n", hr);

    hr = UiaNodeFromHandle(hwnd, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        DWORD pid;

        memset(buf, 0, sizeof(buf));
        GetWindowThreadProcessId(hwnd, &pid);

        ok(get_nested_provider_desc(V_BSTR(&v), L"Main", FALSE, buf), "Failed to get nested provider description\n");
        check_node_provider_desc_prefix(buf, pid, hwnd);
        /* Win10v1507 and below have the nested provider as 'Hwnd'. */
        if (get_provider_desc(buf, L"Hwnd(parent link):", NULL))
            check_node_provider_desc(buf, L"Hwnd", L"Provider", TRUE);
        else
            check_node_provider_desc(buf, L"Main", L"Provider", TRUE);

        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
        check_node_provider_desc(V_BSTR(&v), L"Nonclient", NULL, FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, TRUE);
        VariantClear(&v);
    }

    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v));

    /*
     * On Windows 8 and above, if a node contains a nested provider, the
     * process will be in an implicit MTA until the node is released.
     */
    hr = CoGetApartmentType(&apt_type, &apt_qualifier);
    ok(hr == S_OK || broken(hr == CO_E_NOTINITIALIZED), "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(apt_type == APTTYPE_MTA, "Unexpected apt_type %#x\n", apt_type);
        ok(apt_qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA, "Unexpected apt_qualifier %#x\n", apt_qualifier);
    }

    UiaNodeRelease(node);

    /* Node released, we're out of the implicit MTA. */
    hr = CoGetApartmentType(&apt_type, &apt_qualifier);

    /* Windows 10v1709-1809 are stuck in an implicit MTA. */
    ok(hr == CO_E_NOTINITIALIZED || broken(hr == S_OK), "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(apt_type == APTTYPE_MTA, "Unexpected apt_type %#x\n", apt_type);
        ok(apt_qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA, "Unexpected apt_qualifier %#x\n", apt_qualifier);
    }
}

static DWORD WINAPI uia_node_from_handle_test_thread(LPVOID param)
{
    HUIANODE node, node2, node3;
    HWND hwnd = (HWND)param;
    WCHAR buf[2048];
    HRESULT hr;
    VARIANT v;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Only sent on Win7. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    prov_root = &Provider.IRawElementProviderSimple_iface;
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = hwnd;
    Provider.runtime_id[0] = Provider.runtime_id[1] = 0;
    Provider.frag_root = NULL;
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    hr = UiaNodeFromHandle(hwnd, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        memset(buf, 0, sizeof(buf));

        ok(get_nested_provider_desc(V_BSTR(&v), L"Main", FALSE, buf), "Failed to get nested provider description\n");
        check_node_provider_desc_prefix(buf, GetCurrentProcessId(), hwnd);
        /* Win10v1507 and below have the nested provider as 'Hwnd'. */
        if (get_provider_desc(buf, L"Hwnd(parent link):", NULL))
            check_node_provider_desc(buf, L"Hwnd", L"Provider", TRUE);
        else
            check_node_provider_desc(buf, L"Main", L"Provider", TRUE);

        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
        check_node_provider_desc(V_BSTR(&v), L"Nonclient", NULL, FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, TRUE);
        VariantClear(&v);
    }

    ok_method_sequence(node_from_hwnd3, "node_from_hwnd3");

    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_uia_prop_val(UIA_ControlTypePropertyId, UIAutomationType_Int, &v);

    /*
     * On Windows, nested providers are always called on a separate thread if
     * UseComThreading isn't set. Since we're doing COM marshaling, if we're
     * currently in an MTA, we just call the nested provider from the current
     * thread.
     */
    todo_wine ok(Provider.last_call_tid != GetCurrentThreadId() &&
            Provider.last_call_tid != GetWindowThreadProcessId(hwnd, NULL), "Expected method call on separate thread\n");

    /*
     * Elements returned from nested providers have to be able to get a
     * RuntimeId, or else we'll get E_FAIL on Win8+.
     */
    Provider_child.prov_opts = ProviderOptions_ServerSideProvider;
    Provider_child.expected_tid = Provider.expected_tid;
    Provider_child.runtime_id[0] = UiaAppendRuntimeId;
    Provider_child.runtime_id[1] = 2;
    Provider_child.frag_root = NULL;
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == E_FAIL || broken(hr == S_OK), "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = UiaHUiaNodeFromVariant(&v, &node2);
        ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
        UiaNodeRelease(node2);
    }

    ok_method_sequence(node_from_hwnd5, "node_from_hwnd5");

    /* RuntimeId check succeeds, we'll get a nested node. */
    Provider_child.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    hr = UiaGetPropertyValue(node2, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        /*
         * Even though this is a nested node, without any additional
         * providers, it will not have the 'Nested' prefix.
         */
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
        check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child", TRUE);
        VariantClear(&v);
    }

    hr = UiaGetPropertyValue(node2, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "Unexpected VT %d\n", V_VT(&v));
    ok(V_I4(&v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v));
    ok_method_sequence(node_from_hwnd6, "node_from_hwnd6");

    UiaNodeRelease(node2);

    /*
     * There is a delay between nested nodes being released and the
     * corresponding IRawElementProviderSimple release on newer Windows
     * versions.
     */
    if (Provider_child.ref != 1)
        Sleep(50);
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

    /*
     * Returned nested elements with an HWND will have client-side providers
     * added to them.
     */
    Provider_child.hwnd = hwnd;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Only sent on Win7. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    hr = UiaGetPropertyValue(node2, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        memset(buf, 0, sizeof(buf));

        ok(get_nested_provider_desc(V_BSTR(&v), L"Main", TRUE, buf), "Failed to get nested provider description\n");
        check_node_provider_desc_prefix(buf, GetCurrentProcessId(), hwnd);
        /* Win10v1507 and below have the nested provider as 'Hwnd'. */
        if (get_provider_desc(buf, L"Hwnd(parent link):", NULL))
            check_node_provider_desc(buf, L"Hwnd", L"Provider_child", TRUE);
        else
            check_node_provider_desc(buf, L"Main", L"Provider_child", TRUE);

        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
        check_node_provider_desc(V_BSTR(&v), L"Nonclient", NULL, FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, FALSE);
        VariantClear(&v);
    }

    ok_method_sequence(node_from_hwnd7, "node_from_hwnd7");
    UiaNodeRelease(node2);

    if (Provider_child.ref != 1)
        Sleep(50);
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    /* Win10v1809 can be slow to call Release on Provider. */
    if (Provider.ref != 1)
        Sleep(50);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);

    if (!pUiaDisconnectProvider)
    {
        win_skip("UiaDisconnectProvider not exported by uiautomationcore.dll\n");
        goto exit;
    }

    /*
     * UiaDisconnectProvider tests.
     */
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    prov_root = &Provider.IRawElementProviderSimple_iface;
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = hwnd;
    Provider.runtime_id[0] = Provider.runtime_id[1] = 0;
    Provider.frag_root = NULL;
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    hr = UiaNodeFromHandle(hwnd, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        memset(buf, 0, sizeof(buf));

        ok(get_nested_provider_desc(V_BSTR(&v), L"Main", FALSE, buf), "Failed to get nested provider description\n");
        check_node_provider_desc_prefix(buf, GetCurrentProcessId(), hwnd);
        /* Win10v1507 and below have the nested provider as 'Hwnd'. */
        if (get_provider_desc(buf, L"Hwnd(parent link):", NULL))
            check_node_provider_desc(buf, L"Hwnd", L"Provider", TRUE);
        else
            check_node_provider_desc(buf, L"Main", L"Provider", TRUE);

        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
        check_node_provider_desc(V_BSTR(&v), L"Nonclient", NULL, FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, TRUE);
        VariantClear(&v);
    }

    ok_method_sequence(node_from_hwnd3, "node_from_hwnd3");

    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_uia_prop_val(UIA_ControlTypePropertyId, UIAutomationType_Int, &v);

    /* Nodes returned from a nested node will be tracked and disconnectable. */
    Provider_child.prov_opts = ProviderOptions_ServerSideProvider;
    Provider_child.runtime_id[0] = UiaAppendRuntimeId;
    Provider_child.runtime_id[1] = 2;
    Provider_child.hwnd = NULL;
    Provider_child.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    hr = UiaGetPropertyValue(node2, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
        check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child", TRUE);
        VariantClear(&v);
    }

    hr = UiaGetPropertyValue(node2, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "Unexpected VT %d\n", V_VT(&v));
    ok(V_I4(&v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v));
    ok_method_sequence(node_from_hwnd6, "node_from_hwnd6");

    /* Get a new node for the same provider. */
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(Provider_child.ref == 3, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = UiaHUiaNodeFromVariant(&v, &node3);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    hr = UiaGetPropertyValue(node3, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
        check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child", TRUE);
        VariantClear(&v);
    }

    hr = UiaGetPropertyValue(node3, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "Unexpected VT %d\n", V_VT(&v));
    ok(V_I4(&v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v));
    ok_method_sequence(node_from_hwnd6, "node_from_hwnd6");

    /*
     * Both node2 and node3 represent Provider_child, one call to
     * UiaDisconnectProvider disconnects both.
     */
    hr = pUiaDisconnectProvider(&Provider_child.IRawElementProviderSimple_iface);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = UiaGetPropertyValue(node2, UIA_ControlTypePropertyId, &v);
    ok(hr == UIA_E_ELEMENTNOTAVAILABLE, "Unexpected hr %#lx\n", hr);

    hr = UiaGetPropertyValue(node3, UIA_ControlTypePropertyId, &v);
    ok(hr == UIA_E_ELEMENTNOTAVAILABLE, "Unexpected hr %#lx\n", hr);
    ok_method_sequence(disconnect_prov1, "disconnect_prov1");

    ok(UiaNodeRelease(node2), "UiaNodeRelease returned FALSE\n");
    ok(UiaNodeRelease(node3), "UiaNodeRelease returned FALSE\n");

    /*
     * Returns same failure code as UiaGetRuntimeId when we fail to get a
     * fragment root for AppendRuntimeId.
     */
    Provider.hwnd = NULL;
    Provider.runtime_id[0] = UiaAppendRuntimeId;
    Provider.runtime_id[1] = 2;
    Provider.frag_root = NULL;
    hr = pUiaDisconnectProvider(&Provider.IRawElementProviderSimple_iface);
    ok(hr == E_FAIL, "Unexpected hr %#lx\n", hr);
    ok_method_sequence(disconnect_prov2, "disconnect_prov2");

    /*
     * Comparisons for disconnection are only based on RuntimeId comparisons,
     * not interface pointer values. If an interface returns a NULL RuntimeId,
     * no disconnection will occur.
     */
    Provider.runtime_id[0] = Provider.runtime_id[1] = 0;
    hr = pUiaDisconnectProvider(&Provider.IRawElementProviderSimple_iface);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx\n", hr);
    ok_method_sequence(disconnect_prov3, "disconnect_prov3");

    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_uia_prop_val(UIA_ControlTypePropertyId, UIAutomationType_Int, &v);

    /* Finally, disconnect node. */
    Provider.hwnd = hwnd;
    hr = pUiaDisconnectProvider(&Provider.IRawElementProviderSimple_iface);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == UIA_E_ELEMENTNOTAVAILABLE, "Unexpected hr %#lx\n", hr);
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok_method_sequence(disconnect_prov4, "disconnect_prov4");

exit:
    CoUninitialize();

    return 0;
}

static void test_UiaNodeFromHandle(const char *name)
{
    APTTYPEQUALIFIER apt_qualifier;
    PROCESS_INFORMATION proc;
    char cmdline[MAX_PATH];
    STARTUPINFOA startup;
    HUIANODE node, node2;
    APTTYPE apt_type;
    DWORD exit_code;
    WNDCLASSA cls;
    HANDLE thread;
    HRESULT hr;
    HWND hwnd;
    VARIANT v;

    cls.style = 0;
    cls.lpfnWndProc = test_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = NULL;
    cls.hbrBackground = NULL;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "UiaNodeFromHandle class";
    RegisterClassA(&cls);

    hwnd = CreateWindowA("UiaNodeFromHandle class", "Test window", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, NULL, NULL, NULL, NULL);

    hr = UiaNodeFromHandle(hwnd, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = UiaNodeFromHandle(NULL, &node);
    ok(hr == UIA_E_ELEMENTNOTAVAILABLE, "Unexpected hr %#lx.\n", hr);

    /* COM uninitialized, no provider returned by UiaReturnRawElementProvider. */
    prov_root = NULL;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Only sent twice on Win7. */
    SET_EXPECT_MULTI(winproc_GETOBJECT_CLIENT, 2);
    hr = UiaNodeFromHandle(hwnd, &node);
    todo_wine ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    /*
     * COM initialized, no provider returned by UiaReturnRawElementProvider.
     * In this case, we get a default MSAA proxy.
     */
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    prov_root = NULL;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT_MULTI(winproc_GETOBJECT_CLIENT, 2);
    hr = UiaNodeFromHandle(hwnd, &node);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
        check_node_provider_desc(V_BSTR(&v), L"Annotation", NULL, FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Main", NULL, FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Nonclient", NULL, FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, TRUE);
        VariantClear(&v);
    }

    todo_wine ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");

    /*
     * COM initialized, but provider passed into UiaReturnRawElementProvider
     * returns a failure code on get_ProviderOptions. Same behavior as before.
     */
    Provider.prov_opts = 0;
    Provider.hwnd = hwnd;
    prov_root = &Provider.IRawElementProviderSimple_iface;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT_MULTI(winproc_GETOBJECT_CLIENT, 2);
    hr = UiaNodeFromHandle(hwnd, &node);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
        check_node_provider_desc(V_BSTR(&v), L"Annotation", NULL, FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Main", NULL, FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Nonclient", NULL, FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, TRUE);
        VariantClear(&v);
    }
    ok_method_sequence(node_from_hwnd1, "node_from_hwnd1");

    todo_wine ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");

    /*
     * COM initialized, but provider passed into UiaReturnRawElementProvider
     * on Win8+ will have its RuntimeId queried for UiaDisconnectProvider.
     * It will fail due to the lack of a valid fragment root, and we'll fall
     * back to an MSAA proxy. On Win7, RuntimeId isn't queried because the
     * disconnection functions weren't implemented yet.
     */
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = NULL;
    prov_root = &Provider.IRawElementProviderSimple_iface;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT_MULTI(winproc_GETOBJECT_CLIENT, 2);
    Provider.frag_root = NULL;
    Provider.runtime_id[0] = UiaAppendRuntimeId;
    Provider.runtime_id[1] = 1;
    hr = UiaNodeFromHandle(hwnd, &node);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 1 || broken(Provider.ref == 2), "Unexpected refcnt %ld\n", Provider.ref);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);

        if (get_provider_desc(V_BSTR(&v), L"Annotation:", NULL))
        {
            check_node_provider_desc(V_BSTR(&v), L"Annotation", NULL, FALSE);
            check_node_provider_desc(V_BSTR(&v), L"Main", NULL, FALSE);
        }
        else
            check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider", FALSE);

        check_node_provider_desc(V_BSTR(&v), L"Nonclient", NULL, FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, TRUE);
        VariantClear(&v);
    }
    ok_method_sequence(node_from_hwnd9, "node_from_hwnd9");
    todo_wine ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    /*
     * Bug on Windows 8 through Win10v1709 - if we have a RuntimeId failure,
     * refcount doesn't get decremented.
     */
    ok(Provider.ref == 1 || broken(Provider.ref == 2), "Unexpected refcnt %ld\n", Provider.ref);
    if (Provider.ref == 2)
        IRawElementProviderSimple_Release(&Provider.IRawElementProviderSimple_iface);

    CoUninitialize();

    /*
     * COM uninitialized, return a Provider from UiaReturnRawElementProvider
     * with ProviderOptions_ServerSideProvider.
     */
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = hwnd;
    prov_root = &Provider.IRawElementProviderSimple_iface;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Only sent on Win7. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    hr = UiaNodeFromHandle(hwnd, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
        check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider", FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Nonclient", NULL, FALSE);
        check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, TRUE);
        VariantClear(&v);
    }

    ok_method_sequence(node_from_hwnd2, "node_from_hwnd2");

    /* For same-thread HWND nodes, no disconnection will occur. */
    if (pUiaDisconnectProvider)
    {
        hr = pUiaDisconnectProvider(&Provider.IRawElementProviderSimple_iface);
        ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

        ok_method_sequence(disconnect_prov4, "disconnect_prov4");
    }

    /*
     * This is relevant too: Since we don't get a 'nested' node, all calls
     * will occur on the current thread.
     */
    Provider.expected_tid = GetCurrentThreadId();
    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_uia_prop_val(UIA_ControlTypePropertyId, UIAutomationType_Int, &v);

    /* UIAutomationType_Element properties will return a normal node. */
    Provider_child.prov_opts = ProviderOptions_ServerSideProvider;
    Provider_child.hwnd = NULL;
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    hr = UiaGetPropertyValue(node2, UIA_ProviderDescriptionPropertyId, &v);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
        check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child", TRUE);
        VariantClear(&v);
    }

    Provider_child.expected_tid = GetCurrentThreadId();
    hr = UiaGetPropertyValue(node2, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "Unexpected VT %d\n", V_VT(&v));
    ok(V_I4(&v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v));
    ok_method_sequence(node_from_hwnd4, "node_from_hwnd4");

    ok(UiaNodeRelease(node2), "UiaNodeRelease returned FALSE\n");

    Provider.expected_tid = Provider_child.expected_tid = 0;
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");

    /*
     * On Windows 8 and above, after the first successful call to
     * UiaReturnRawElementProvider the process ends up in an implicit MTA
     * until the process exits.
     */
    hr = CoGetApartmentType(&apt_type, &apt_qualifier);
    /* Wine's provider thread doesn't always terminate immediately. */
    if (hr == S_OK && !strcmp(winetest_platform, "wine"))
    {
        Sleep(10);
        hr = CoGetApartmentType(&apt_type, &apt_qualifier);
    }
    todo_wine ok(hr == S_OK || broken(hr == CO_E_NOTINITIALIZED), "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(apt_type == APTTYPE_MTA, "Unexpected apt_type %#x\n", apt_type);
        ok(apt_qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA, "Unexpected apt_qualifier %#x\n", apt_qualifier);
    }

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    thread = CreateThread(NULL, 0, uia_node_from_handle_test_thread, (void *)hwnd, 0, NULL);
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;
        while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    CloseHandle(thread);

    /* Test behavior from separate process. */
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = hwnd;
    prov_root = &Provider.IRawElementProviderSimple_iface;
    sprintf(cmdline, "\"%s\" uiautomation UiaNodeFromHandle_client_proc", name);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Only sent on Win7. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &proc);
    while (MsgWaitForMultipleObjects(1, &proc.hProcess, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;
        while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    GetExitCodeProcess(proc.hProcess, &exit_code);
    if (exit_code > 255)
        ok(0, "unhandled exception %08x in child process %04x\n", (UINT)exit_code, (UINT)GetProcessId(proc.hProcess));
    else if (exit_code)
        ok(0, "%u failures in child process\n", (UINT)exit_code);

    CloseHandle(proc.hProcess);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;
    ok_method_sequence(node_from_hwnd8, "node_from_hwnd8");

    CoUninitialize();

    DestroyWindow(hwnd);
    UnregisterClassA("UiaNodeFromHandle class", NULL);
    prov_root = NULL;
}

/*
 * Once a process returns a UI Automation provider with
 * UiaReturnRawElementProvider it ends up in an implicit MTA until exit. This
 * messes with tests around COM initialization, so we run these tests in
 * separate processes.
 */
static void launch_test_process(const char *name, const char *test_name)
{
    PROCESS_INFORMATION proc;
    STARTUPINFOA startup;
    char cmdline[MAX_PATH];

    sprintf(cmdline, "\"%s\" uiautomation %s", name, test_name);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &proc);
    wait_child_process(proc.hProcess);
}

START_TEST(uiautomation)
{
    HMODULE uia_dll = LoadLibraryA("uiautomationcore.dll");
    BOOL (WINAPI *pImmDisableIME)(DWORD);
    HMODULE hModuleImm32;
    char **argv;
    int argc;

    /* Make sure COM isn't initialized by imm32. */
    hModuleImm32 = LoadLibraryA("imm32.dll");
    if (hModuleImm32) {
        pImmDisableIME = (void *)GetProcAddress(hModuleImm32, "ImmDisableIME");
        if (pImmDisableIME)
            pImmDisableIME(0);
    }
    pImmDisableIME = NULL;
    FreeLibrary(hModuleImm32);

    if (uia_dll)
        pUiaDisconnectProvider = (void *)GetProcAddress(uia_dll, "UiaDisconnectProvider");

    argc = winetest_get_mainargs(&argv);
    if (argc == 3)
    {
        if (!strcmp(argv[2], "UiaNodeFromHandle"))
            test_UiaNodeFromHandle(argv[0]);
        else if (!strcmp(argv[2], "UiaNodeFromHandle_client_proc"))
            test_UiaNodeFromHandle_client_proc();

        FreeLibrary(uia_dll);
        return;
    }

    test_UiaHostProviderFromHwnd();
    test_uia_reserved_value_ifaces();
    test_UiaLookupId();
    test_UiaNodeFromProvider();
    test_UiaGetPropertyValue();
    test_UiaGetRuntimeId();
    test_UiaHUiaNodeFromVariant();
    launch_test_process(argv[0], "UiaNodeFromHandle");
    if (uia_dll)
    {
        pUiaProviderFromIAccessible = (void *)GetProcAddress(uia_dll, "UiaProviderFromIAccessible");
        if (pUiaProviderFromIAccessible)
            test_UiaProviderFromIAccessible();
        else
            win_skip("UiaProviderFromIAccessible not exported by uiautomationcore.dll\n");

        FreeLibrary(uia_dll);
    }
}
