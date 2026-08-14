/* WinRT Windows.UI.Text.Core.CoreTextEditContext Implementation
 *
 * Written by Weather
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

#include "editcontext.h"

WINE_DEFAULT_DEBUG_CHANNEL(textinput);

static inline struct core_text_edit_context *impl_from_ICoreTextEditContext( ICoreTextEditContext *iface )
{
    return CONTAINING_RECORD( iface, struct core_text_edit_context, ICoreTextEditContext_iface );
}

static HRESULT WINAPI core_text_edit_context_QueryInterface( ICoreTextEditContext *iface, REFIID iid, void **out )
{
    struct core_text_edit_context *impl = impl_from_ICoreTextEditContext( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_ICoreTextEditContext ))
    {
        *out = &impl->ICoreTextEditContext_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI core_text_edit_context_AddRef( ICoreTextEditContext *iface )
{
    struct core_text_edit_context *impl = impl_from_ICoreTextEditContext( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI core_text_edit_context_Release( ICoreTextEditContext *iface )
{
    struct core_text_edit_context *impl = impl_from_ICoreTextEditContext( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI core_text_edit_context_GetIids( ICoreTextEditContext *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI core_text_edit_context_GetRuntimeClassName( ICoreTextEditContext *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI core_text_edit_context_GetTrustLevel( ICoreTextEditContext *iface, TrustLevel *trust_level )
{
    TRACE( "iface %p, trust_level %p.\n", iface, trust_level );
    *trust_level = FullTrust;
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_get_Name(ICoreTextEditContext *iface, HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_text_edit_context_put_Name(ICoreTextEditContext *iface, HSTRING value)
{
    FIXME("iface %p, value %p stub!\n", iface, (void*)value);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_text_edit_context_get_InputScope(ICoreTextEditContext *iface, CoreTextInputScope *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_text_edit_context_put_InputScope(ICoreTextEditContext *iface, CoreTextInputScope value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_text_edit_context_get_IsReadOnly(ICoreTextEditContext *iface, boolean *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_text_edit_context_put_IsReadOnly(ICoreTextEditContext *iface, boolean value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_text_edit_context_get_InputPaneDisplayPolicy(ICoreTextEditContext *iface, CoreTextInputPaneDisplayPolicy *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_text_edit_context_put_InputPaneDisplayPolicy(ICoreTextEditContext *iface, CoreTextInputPaneDisplayPolicy value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_add_TextRequested(ICoreTextEditContext *iface, ITypedEventHandler_CoreTextEditContext_CoreTextTextRequestedEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("iface %p, handler %p, cookie %p stub!\n", iface, handler, cookie);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_remove_TextRequested(ICoreTextEditContext *iface, EventRegistrationToken cookie)
{
    FIXME("iface %p, cookie %p stub!\n", iface, (void*)&cookie);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_add_SelectionRequested(ICoreTextEditContext *iface, ITypedEventHandler_CoreTextEditContext_CoreTextSelectionRequestedEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("iface %p, handler %p, cookie %p stub!\n", iface, handler, cookie);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_remove_SelectionRequested(ICoreTextEditContext *iface, EventRegistrationToken cookie)
{
    FIXME("iface %p, cookie %p stub!\n", iface, (void*)&cookie);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_add_LayoutRequested(ICoreTextEditContext *iface, ITypedEventHandler_CoreTextEditContext_CoreTextLayoutRequestedEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("iface %p, handler %p, cookie %p stub!\n", iface, handler, cookie);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_remove_LayoutRequested(ICoreTextEditContext *iface, EventRegistrationToken cookie)
{
    FIXME("iface %p, cookie %p stub!\n", iface, (void*)&cookie);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_add_TextUpdating(ICoreTextEditContext *iface, ITypedEventHandler_CoreTextEditContext_CoreTextTextUpdatingEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("iface %p, handler %p, cookie %p stub!\n", iface, handler, cookie);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_remove_TextUpdating(ICoreTextEditContext *iface, EventRegistrationToken cookie)
{
    FIXME("iface %p, cookie %p stub!\n", iface, (void*)&cookie);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_add_SelectionUpdating(ICoreTextEditContext *iface, ITypedEventHandler_CoreTextEditContext_CoreTextSelectionUpdatingEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("iface %p, handler %p, cookie %p stub!\n", iface, handler, cookie);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_remove_SelectionUpdating(ICoreTextEditContext *iface, EventRegistrationToken cookie)
{
    FIXME("iface %p, cookie %p stub!\n", iface, (void*)&cookie);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_add_FormatUpdating(ICoreTextEditContext *iface, ITypedEventHandler_CoreTextEditContext_CoreTextFormatUpdatingEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("iface %p, FormatUpdating add stub!\n", iface);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_remove_FormatUpdating(ICoreTextEditContext *iface, EventRegistrationToken cookie)
{
    FIXME("iface %p, FormatUpdating remove stub!\n", iface);
    return S_OK;
}

/* CompositionStarted (flattened form) */
static HRESULT WINAPI core_text_edit_context_add_CompositionStarted(ICoreTextEditContext *iface, ITypedEventHandler_CoreTextEditContext_CoreTextCompositionStartedEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("iface %p, CompositionStarted add stub!\n", iface);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_remove_CompositionStarted(ICoreTextEditContext *iface, EventRegistrationToken cookie)
{
    FIXME("iface %p, CompositionStarted remove stub!\n", iface);
    return S_OK;
}

/* CompositionCompleted (flattened form) */
static HRESULT WINAPI core_text_edit_context_add_CompositionCompleted(ICoreTextEditContext *iface, ITypedEventHandler_CoreTextEditContext_CoreTextCompositionCompletedEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("iface %p, CompositionCompleted add stub!\n", iface);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_remove_CompositionCompleted(ICoreTextEditContext *iface, EventRegistrationToken cookie)
{
    FIXME("iface %p, CompositionCompleted remove stub!\n", iface);
    return S_OK;
}

/* FocusRemoved (flattened form) */
static HRESULT WINAPI core_text_edit_context_add_FocusRemoved(ICoreTextEditContext *iface, ITypedEventHandler_CoreTextEditContext_IInspectable *handler, EventRegistrationToken *cookie)
{
    FIXME("iface %p, FocusRemoved add stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_text_edit_context_remove_FocusRemoved(ICoreTextEditContext *iface, EventRegistrationToken cookie)
{
    FIXME("iface %p, FocusRemoved remove stub!\n", iface);
    return E_NOTIMPL;
}

/* Notifications */
static HRESULT WINAPI core_text_edit_context_NotifyFocusEnter(ICoreTextEditContext *iface)
{
    FIXME("iface %p, NotifyFocusEnter stub!\n", iface);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_NotifyFocusLeave(ICoreTextEditContext *iface)
{
    FIXME("iface %p, NotifyFocusLeave stub!\n", iface);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_NotifyTextChanged(ICoreTextEditContext *iface,
                                                               CoreTextRange modifiedRange,
                                                               INT32 newLength,
                                                               CoreTextRange newSelection)
{
    FIXME("iface %p, modifiedRange %p, newLength %d, newSelection %p stub!\n",
          iface, (void*)&modifiedRange, (int)newLength, (void*)&newSelection);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_NotifySelectionChanged(ICoreTextEditContext *iface,
                                                                    CoreTextRange selection)
{
    FIXME("iface %p, selection %p stub!\n", iface, (void*)&selection);
    return S_OK;
}

static HRESULT WINAPI core_text_edit_context_NotifyLayoutChanged(ICoreTextEditContext *iface)
{
    FIXME("iface %p, NotifyLayoutChanged stub!\n", iface);
    return S_OK;
}

const struct ICoreTextEditContextVtbl core_text_edit_context_vtbl =
{
    /* IUnknown */
    core_text_edit_context_QueryInterface,
    core_text_edit_context_AddRef,
    core_text_edit_context_Release,

    /* IInspectable */
    core_text_edit_context_GetIids,
    core_text_edit_context_GetRuntimeClassName,
    core_text_edit_context_GetTrustLevel,

    /* ICoreTextEditContext (in IDL order) */
    core_text_edit_context_get_Name,                      /* propget Name */
    core_text_edit_context_put_Name,                      /* propput Name */
    core_text_edit_context_get_InputScope,                /* propget InputScope */
    core_text_edit_context_put_InputScope,                /* propput InputScope */
    core_text_edit_context_get_IsReadOnly,                /* propget IsReadOnly */
    core_text_edit_context_put_IsReadOnly,                /* propput IsReadOnly */
    core_text_edit_context_get_InputPaneDisplayPolicy,    /* propget InputPaneDisplayPolicy */
    core_text_edit_context_put_InputPaneDisplayPolicy,    /* propput InputPaneDisplayPolicy */

    core_text_edit_context_add_TextRequested,             /* eventadd TextRequested */
    core_text_edit_context_remove_TextRequested,          /* eventremove TextRequested */
    core_text_edit_context_add_SelectionRequested,        /* eventadd SelectionRequested */
    core_text_edit_context_remove_SelectionRequested,     /* eventremove SelectionRequested */
    core_text_edit_context_add_LayoutRequested,           /* eventadd LayoutRequested */
    core_text_edit_context_remove_LayoutRequested,        /* eventremove LayoutRequested */
    core_text_edit_context_add_TextUpdating,              /* eventadd TextUpdating */
    core_text_edit_context_remove_TextUpdating,           /* eventremove TextUpdating */
    core_text_edit_context_add_SelectionUpdating,         /* eventadd SelectionUpdating */
    core_text_edit_context_remove_SelectionUpdating,      /* eventremove SelectionUpdating */
    core_text_edit_context_add_FormatUpdating,            /* eventadd FormatUpdating */
    core_text_edit_context_remove_FormatUpdating,         /* eventremove FormatUpdating */
    core_text_edit_context_add_CompositionStarted,        /* eventadd CompositionStarted */
    core_text_edit_context_remove_CompositionStarted,     /* eventremove CompositionStarted */
    core_text_edit_context_add_CompositionCompleted,      /* eventadd CompositionCompleted */
    core_text_edit_context_remove_CompositionCompleted,   /* eventremove CompositionCompleted */
    core_text_edit_context_add_FocusRemoved,              /* eventadd FocusRemoved */
    core_text_edit_context_remove_FocusRemoved,           /* eventremove FocusRemoved */
    core_text_edit_context_NotifyFocusEnter,              /* NotifyFocusEnter */
    core_text_edit_context_NotifyFocusLeave,              /* NotifyFocusLeave */
    core_text_edit_context_NotifyTextChanged,             /* NotifyTextChanged */
    core_text_edit_context_NotifySelectionChanged,        /* NotifySelectionChanged */
    core_text_edit_context_NotifyLayoutChanged            /* NotifyLayoutChanged */
};

HRESULT core_text_edit_context_create( ICoreTextEditContext **out )
{
    struct core_text_edit_context *impl;

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->ICoreTextEditContext_iface.lpVtbl = &core_text_edit_context_vtbl;
    impl->ref = 1;

    *out = &impl->ICoreTextEditContext_iface;
    return S_OK;
}
