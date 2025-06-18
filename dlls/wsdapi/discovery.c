/*
 * Web Services on Devices
 *
 * Copyright 2017 Owen Rudge for CodeWeavers
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
#define INITGUID

#include "wsdapi_internal.h"
#include "wine/debug.h"
#include "guiddef.h"

WINE_DEFAULT_DEBUG_CHANNEL(wsdapi);

static inline IWSDiscoveryPublisherImpl *impl_from_IWSDiscoveryPublisher(IWSDiscoveryPublisher *iface)
{
    return CONTAINING_RECORD(iface, IWSDiscoveryPublisherImpl, IWSDiscoveryPublisher_iface);
}

static HRESULT WINAPI IWSDiscoveryPublisherImpl_QueryInterface(IWSDiscoveryPublisher *iface, REFIID riid, void **ppv)
{
    IWSDiscoveryPublisherImpl *This = impl_from_IWSDiscoveryPublisher(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
    {
        WARN("Invalid parameter\n");
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IWSDiscoveryPublisher))
    {
        *ppv = &This->IWSDiscoveryPublisher_iface;
    }
    else
    {
        WARN("Unknown IID %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI IWSDiscoveryPublisherImpl_AddRef(IWSDiscoveryPublisher *iface)
{
    IWSDiscoveryPublisherImpl *This = impl_from_IWSDiscoveryPublisher(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI IWSDiscoveryPublisherImpl_Release(IWSDiscoveryPublisher *iface)
{
    IWSDiscoveryPublisherImpl *This = impl_from_IWSDiscoveryPublisher(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    struct notificationSink *sink, *cursor;
    struct message_id *msg_id, *msg_id_cursor;

    TRACE("(%p) ref=%ld\n", This, ref);

    if (ref == 0)
    {
        terminate_networking(This);

        if (This->xmlContext != NULL)
        {
            IWSDXMLContext_Release(This->xmlContext);
        }

        LIST_FOR_EACH_ENTRY_SAFE(sink, cursor, &This->notificationSinks, struct notificationSink, entry)
        {
            IWSDiscoveryPublisherNotify_Release(sink->notificationSink);
            list_remove(&sink->entry);
            free(sink);
        }

        DeleteCriticalSection(&This->notification_sink_critical_section);

        LIST_FOR_EACH_ENTRY_SAFE(msg_id, msg_id_cursor, &This->message_ids, struct message_id, entry)
        {
            free(msg_id->id);
            list_remove(&msg_id->entry);
            free(msg_id);
        }

        DeleteCriticalSection(&This->message_ids_critical_section);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI IWSDiscoveryPublisherImpl_SetAddressFamily(IWSDiscoveryPublisher *This, DWORD dwAddressFamily)
{
    IWSDiscoveryPublisherImpl *impl = impl_from_IWSDiscoveryPublisher(This);

    TRACE("(%p, %ld)\n", This, dwAddressFamily);

    /* Has the address family already been set? */
    if (impl->addressFamily != 0)
    {
        return STG_E_INVALIDFUNCTION;
    }

    if ((dwAddressFamily == WSDAPI_ADDRESSFAMILY_IPV4) || (dwAddressFamily == WSDAPI_ADDRESSFAMILY_IPV6) ||
        (dwAddressFamily == (WSDAPI_ADDRESSFAMILY_IPV4 | WSDAPI_ADDRESSFAMILY_IPV6)))
    {
        /* TODO: Check that the address family is supported by the system */
        impl->addressFamily = dwAddressFamily;
        return S_OK;
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI IWSDiscoveryPublisherImpl_RegisterNotificationSink(IWSDiscoveryPublisher *This, IWSDiscoveryPublisherNotify *pSink)
{
    IWSDiscoveryPublisherImpl *impl = impl_from_IWSDiscoveryPublisher(This);
    struct notificationSink *sink;

    TRACE("(%p, %p)\n", This, pSink);

    if (pSink == NULL)
    {
        return E_INVALIDARG;
    }

    sink = calloc(1, sizeof(*sink));

    if (!sink)
    {
        return E_OUTOFMEMORY;
    }

    sink->notificationSink = pSink;
    IWSDiscoveryPublisherNotify_AddRef(pSink);

    EnterCriticalSection(&impl->notification_sink_critical_section);
    list_add_tail(&impl->notificationSinks, &sink->entry);
    LeaveCriticalSection(&impl->notification_sink_critical_section);

    if ((!impl->publisherStarted) && (!init_networking(impl)))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI IWSDiscoveryPublisherImpl_UnRegisterNotificationSink(IWSDiscoveryPublisher *This, IWSDiscoveryPublisherNotify *pSink)
{
    IWSDiscoveryPublisherImpl *impl = impl_from_IWSDiscoveryPublisher(This);
    struct notificationSink *sink;

    TRACE("(%p, %p)\n", This, pSink);

    if (pSink == NULL)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection(&impl->notification_sink_critical_section);

    LIST_FOR_EACH_ENTRY(sink, &impl->notificationSinks, struct notificationSink, entry)
    {
        if (sink->notificationSink == pSink)
        {
            IWSDiscoveryPublisherNotify_Release(pSink);
            list_remove(&sink->entry);
            free(sink);

            LeaveCriticalSection(&impl->notification_sink_critical_section);
            return S_OK;
        }
    }

    LeaveCriticalSection(&impl->notification_sink_critical_section);

    /* Notification sink is not registered */
    return E_FAIL;
}

static HRESULT WINAPI IWSDiscoveryPublisherImpl_Publish(IWSDiscoveryPublisher *This, LPCWSTR pszId, ULONGLONG ullMetadataVersion, ULONGLONG ullInstanceId,
                                                        ULONGLONG ullMessageNumber, LPCWSTR pszSessionId, const WSD_NAME_LIST *pTypesList,
                                                        const WSD_URI_LIST *pScopesList, const WSD_URI_LIST *pXAddrsList)
{
    return IWSDiscoveryPublisher_PublishEx(This, pszId, ullMetadataVersion, ullInstanceId, ullMessageNumber,
        pszSessionId, pTypesList, pScopesList, pXAddrsList, NULL, NULL, NULL, NULL, NULL);
}

static HRESULT WINAPI IWSDiscoveryPublisherImpl_UnPublish(IWSDiscoveryPublisher *This, LPCWSTR pszId, ULONGLONG ullInstanceId, ULONGLONG ullMessageNumber,
                                                          LPCWSTR pszSessionId, const WSDXML_ELEMENT *pAny)
{
    IWSDiscoveryPublisherImpl *impl = impl_from_IWSDiscoveryPublisher(This);

    TRACE("(%p, %s, %s, %s, %s, %p)\n", This, debugstr_w(pszId), wine_dbgstr_longlong(ullInstanceId),
        wine_dbgstr_longlong(ullMessageNumber), debugstr_w(pszSessionId), pAny);

    if ((!impl->publisherStarted) || (pszId == NULL) || (lstrlenW(pszId) > WSD_MAX_TEXT_LENGTH) ||
        ((pszSessionId != NULL) && (lstrlenW(pszSessionId) > WSD_MAX_TEXT_LENGTH)))
    {
        return E_INVALIDARG;
    }

    return send_bye_message(impl, pszId, ullInstanceId, ullMessageNumber, pszSessionId, pAny);
}

static HRESULT WINAPI IWSDiscoveryPublisherImpl_MatchProbe(IWSDiscoveryPublisher *This, const WSD_SOAP_MESSAGE *pProbeMessage,
                                                           IWSDMessageParameters *pMessageParameters, LPCWSTR pszId, ULONGLONG ullMetadataVersion,
                                                           ULONGLONG ullInstanceId, ULONGLONG ullMessageNumber, LPCWSTR pszSessionId,
                                                           const WSD_NAME_LIST *pTypesList, const WSD_URI_LIST *pScopesList,
                                                           const WSD_URI_LIST *pXAddrsList)
{
    TRACE("(%p, %p, %p, %s, %s, %s, %s, %s, %p, %p, %p)\n", This, pProbeMessage, pMessageParameters, debugstr_w(pszId),
        wine_dbgstr_longlong(ullMetadataVersion), wine_dbgstr_longlong(ullInstanceId), wine_dbgstr_longlong(ullMessageNumber), debugstr_w(pszSessionId),
        pTypesList, pScopesList, pXAddrsList);

    return IWSDiscoveryPublisher_MatchProbeEx(This, pProbeMessage, pMessageParameters, pszId, ullMetadataVersion,
        ullInstanceId, ullMessageNumber, pszSessionId, pTypesList, pScopesList, pXAddrsList, NULL, NULL, NULL, NULL,
        NULL);
}

static HRESULT WINAPI IWSDiscoveryPublisherImpl_MatchResolve(IWSDiscoveryPublisher *This, const WSD_SOAP_MESSAGE *pResolveMessage,
                                                             IWSDMessageParameters *pMessageParameters, LPCWSTR pszId, ULONGLONG ullMetadataVersion,
                                                             ULONGLONG ullInstanceId, ULONGLONG ullMessageNumber, LPCWSTR pszSessionId,
                                                             const WSD_NAME_LIST *pTypesList, const WSD_URI_LIST *pScopesList,
                                                             const WSD_URI_LIST *pXAddrsList)
{
    FIXME("(%p, %p, %p, %s, %s, %s, %s, %s, %p, %p, %p)\n", This, pResolveMessage, pMessageParameters, debugstr_w(pszId),
        wine_dbgstr_longlong(ullMetadataVersion), wine_dbgstr_longlong(ullInstanceId), wine_dbgstr_longlong(ullMessageNumber), debugstr_w(pszSessionId),
        pTypesList, pScopesList, pXAddrsList);

    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDiscoveryPublisherImpl_PublishEx(IWSDiscoveryPublisher *This, LPCWSTR pszId, ULONGLONG ullMetadataVersion,
                                                          ULONGLONG ullInstanceId, ULONGLONG ullMessageNumber, LPCWSTR pszSessionId,
                                                          const WSD_NAME_LIST *pTypesList, const WSD_URI_LIST *pScopesList,
                                                          const WSD_URI_LIST *pXAddrsList, const WSDXML_ELEMENT *pHeaderAny,
                                                          const WSDXML_ELEMENT *pReferenceParameterAny, const WSDXML_ELEMENT *pPolicyAny,
                                                          const WSDXML_ELEMENT *pEndpointReferenceAny, const WSDXML_ELEMENT *pAny)
{
    IWSDiscoveryPublisherImpl *impl = impl_from_IWSDiscoveryPublisher(This);

    TRACE("(%p, %s, %s, %s, %s, %s, %p, %p, %p, %p, %p, %p, %p, %p)\n", This, debugstr_w(pszId), wine_dbgstr_longlong(ullMetadataVersion),
        wine_dbgstr_longlong(ullInstanceId), wine_dbgstr_longlong(ullMessageNumber), debugstr_w(pszSessionId), pTypesList, pScopesList, pXAddrsList,
        pHeaderAny, pReferenceParameterAny, pPolicyAny, pEndpointReferenceAny, pAny);

    if ((!impl->publisherStarted) || (pszId == NULL) || (lstrlenW(pszId) > WSD_MAX_TEXT_LENGTH) ||
        ((pszSessionId != NULL) && (lstrlenW(pszSessionId) > WSD_MAX_TEXT_LENGTH)))
    {
        return E_INVALIDARG;
    }

    return send_hello_message(impl, pszId, ullMetadataVersion, ullInstanceId, ullMessageNumber, pszSessionId,
        pTypesList, pScopesList, pXAddrsList, pHeaderAny, pReferenceParameterAny, pEndpointReferenceAny, pAny);
}

static BOOL is_name_in_list(WSDXML_NAME *name, const WSD_NAME_LIST *list)
{
    const WSD_NAME_LIST *next = list;

    while (next != NULL)
    {
        if ((lstrcmpW(next->Element->LocalName, name->LocalName) == 0) &&
            (lstrcmpW(next->Element->Space->PreferredPrefix, name->Space->PreferredPrefix) == 0))
        {
            return TRUE;
        }

        next = next->Next;
    }

    return FALSE;
}

static HRESULT WINAPI IWSDiscoveryPublisherImpl_MatchProbeEx(IWSDiscoveryPublisher *This, const WSD_SOAP_MESSAGE *pProbeMessage,
                                                             IWSDMessageParameters *pMessageParameters, LPCWSTR pszId, ULONGLONG ullMetadataVersion,
                                                             ULONGLONG ullInstanceId, ULONGLONG ullMessageNumber, LPCWSTR pszSessionId,
                                                             const WSD_NAME_LIST *pTypesList, const WSD_URI_LIST *pScopesList,
                                                             const WSD_URI_LIST *pXAddrsList, const WSDXML_ELEMENT *pHeaderAny,
                                                             const WSDXML_ELEMENT *pReferenceParameterAny, const WSDXML_ELEMENT *pPolicyAny,
                                                             const WSDXML_ELEMENT *pEndpointReferenceAny, const WSDXML_ELEMENT *pAny)
{
    IWSDiscoveryPublisherImpl *impl = impl_from_IWSDiscoveryPublisher(This);
    WSD_NAME_LIST *next_name;
    WSD_PROBE *probe_msg;

    TRACE("(%p, %p, %p, %s, %s, %s, %s, %s, %p, %p, %p, %p, %p, %p, %p, %p)\n", This, pProbeMessage, pMessageParameters, debugstr_w(pszId),
        wine_dbgstr_longlong(ullMetadataVersion), wine_dbgstr_longlong(ullInstanceId), wine_dbgstr_longlong(ullMessageNumber), debugstr_w(pszSessionId),
        pTypesList, pScopesList, pXAddrsList, pHeaderAny, pReferenceParameterAny, pPolicyAny, pEndpointReferenceAny, pAny);

    if (!impl->publisherStarted) return E_ABORT;

    if ((pszId == NULL) || (lstrlenW(pszId) > WSD_MAX_TEXT_LENGTH) ||
        ((pszSessionId != NULL) && (lstrlenW(pszSessionId) > WSD_MAX_TEXT_LENGTH)) || (pProbeMessage == NULL) ||
        (pProbeMessage->Body == NULL))
    {
        return E_INVALIDARG;
    }

    probe_msg = (WSD_PROBE *) pProbeMessage->Body;
    next_name = probe_msg->Types;

    /* Verify that all names in the probe message are present in the types list */
    while (next_name != NULL)
    {
        /* If a name isn't present, return success; we simply don't send a Probe Match message */
        if (!is_name_in_list(next_name->Element, pTypesList)) return S_OK;

        next_name = next_name->Next;
    }

    if ((probe_msg->Scopes != NULL) && (probe_msg->Scopes->Scopes != NULL))
        FIXME("Scopes matching currently unimplemented\n");

    return send_probe_matches_message(impl, pProbeMessage, pMessageParameters, pszId, ullMetadataVersion, ullInstanceId,
        ullMessageNumber, pszSessionId, pTypesList, pScopesList, pXAddrsList, pHeaderAny, pReferenceParameterAny,
        pEndpointReferenceAny, pAny);
}

static HRESULT WINAPI IWSDiscoveryPublisherImpl_MatchResolveEx(IWSDiscoveryPublisher *This, const WSD_SOAP_MESSAGE *pResolveMessage,
                                                               IWSDMessageParameters *pMessageParameters, LPCWSTR pszId, ULONGLONG ullMetadataVersion,
                                                               ULONGLONG ullInstanceId, ULONGLONG ullMessageNumber, LPCWSTR pszSessionId,
                                                               const WSD_NAME_LIST *pTypesList, const WSD_URI_LIST *pScopesList,
                                                               const WSD_URI_LIST *pXAddrsList, const WSDXML_ELEMENT *pHeaderAny,
                                                               const WSDXML_ELEMENT *pReferenceParameterAny, const WSDXML_ELEMENT *pPolicyAny,
                                                               const WSDXML_ELEMENT *pEndpointReferenceAny, const WSDXML_ELEMENT *pAny)
{
    FIXME("(%p, %p, %p, %s, %s, %s, %s, %s, %p, %p, %p, %p, %p, %p, %p, %p)\n", This, pResolveMessage, pMessageParameters, debugstr_w(pszId),
        wine_dbgstr_longlong(ullMetadataVersion), wine_dbgstr_longlong(ullInstanceId), wine_dbgstr_longlong(ullMessageNumber), debugstr_w(pszSessionId),
        pTypesList, pScopesList, pXAddrsList, pHeaderAny, pReferenceParameterAny, pPolicyAny, pEndpointReferenceAny, pAny);

    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDiscoveryPublisherImpl_RegisterScopeMatchingRule(IWSDiscoveryPublisher *This, IWSDScopeMatchingRule *pScopeMatchingRule)
{
    FIXME("(%p, %p)\n", This, pScopeMatchingRule);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDiscoveryPublisherImpl_UnRegisterScopeMatchingRule(IWSDiscoveryPublisher *This, IWSDScopeMatchingRule *pScopeMatchingRule)
{
    FIXME("(%p, %p)\n", This, pScopeMatchingRule);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDiscoveryPublisherImpl_GetXMLContext(IWSDiscoveryPublisher *This, IWSDXMLContext **ppContext)
{
    IWSDiscoveryPublisherImpl *impl = impl_from_IWSDiscoveryPublisher(This);

    TRACE("%p, %p)\n", This, ppContext);

    if (ppContext == NULL)
        return E_INVALIDARG;

    if (impl->xmlContext != NULL)
    {
        IWSDXMLContext_AddRef(impl->xmlContext);
    }

    *ppContext = impl->xmlContext;
    return S_OK;
}

static const IWSDiscoveryPublisherVtbl publisher_vtbl =
{
    IWSDiscoveryPublisherImpl_QueryInterface,
    IWSDiscoveryPublisherImpl_AddRef,
    IWSDiscoveryPublisherImpl_Release,
    IWSDiscoveryPublisherImpl_SetAddressFamily,
    IWSDiscoveryPublisherImpl_RegisterNotificationSink,
    IWSDiscoveryPublisherImpl_UnRegisterNotificationSink,
    IWSDiscoveryPublisherImpl_Publish,
    IWSDiscoveryPublisherImpl_UnPublish,
    IWSDiscoveryPublisherImpl_MatchProbe,
    IWSDiscoveryPublisherImpl_MatchResolve,
    IWSDiscoveryPublisherImpl_PublishEx,
    IWSDiscoveryPublisherImpl_MatchProbeEx,
    IWSDiscoveryPublisherImpl_MatchResolveEx,
    IWSDiscoveryPublisherImpl_RegisterScopeMatchingRule,
    IWSDiscoveryPublisherImpl_UnRegisterScopeMatchingRule,
    IWSDiscoveryPublisherImpl_GetXMLContext
};

HRESULT WINAPI WSDCreateDiscoveryPublisher(IWSDXMLContext *pContext, IWSDiscoveryPublisher **ppPublisher)
{
    IWSDiscoveryPublisherImpl *obj;
    HRESULT ret;

    TRACE("(%p, %p)\n", pContext, ppPublisher);

    if (ppPublisher == NULL)
    {
        WARN("Invalid parameter: ppPublisher == NULL\n");
        return E_POINTER;
    }

    *ppPublisher = NULL;

    obj = calloc(1, sizeof(*obj));

    if (!obj)
    {
        WARN("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    obj->IWSDiscoveryPublisher_iface.lpVtbl = &publisher_vtbl;
    obj->ref = 1;

    if (pContext == NULL)
    {
        ret = WSDXMLCreateContext(&obj->xmlContext);

        if (FAILED(ret))
        {
            WARN("Unable to create XML context\n");
            free(obj);
            return ret;
        }
    }
    else
    {
        obj->xmlContext = pContext;
        IWSDXMLContext_AddRef(pContext);
    }

    ret = register_namespaces(obj->xmlContext);

    if (FAILED(ret))
    {
        WARN("Unable to register default namespaces\n");
        free(obj);

        return ret;
    }

    InitializeCriticalSection(&obj->notification_sink_critical_section);
    list_init(&obj->notificationSinks);

    InitializeCriticalSection(&obj->message_ids_critical_section);
    list_init(&obj->message_ids);

    *ppPublisher = &obj->IWSDiscoveryPublisher_iface;
    TRACE("Returning iface %p\n", *ppPublisher);

    return S_OK;
}

HRESULT WINAPI WSDCreateDiscoveryProvider(IWSDXMLContext *context, IWSDiscoveryProvider **provider)
{
    FIXME("(%p, %p) stub\n", context, provider);

    return E_NOTIMPL;
}
