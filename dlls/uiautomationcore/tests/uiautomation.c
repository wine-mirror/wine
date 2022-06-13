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

DEFINE_EXPECT(winproc_GETOBJECT_CLIENT);
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

static IAccessible *acc_client;
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

START_TEST(uiautomation)
{
    HMODULE uia_dll = LoadLibraryA("uiautomationcore.dll");

    test_UiaHostProviderFromHwnd();
    test_uia_reserved_value_ifaces();
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
