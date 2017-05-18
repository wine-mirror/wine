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

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "wsdapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(wsdapi);

#define WSD_MAX_TEXT_LENGTH 8192

static LPWSTR duplicate_string(void *parentMemoryBlock, LPCWSTR value)
{
    int valueLen;
    LPWSTR dup;

    valueLen = lstrlenW(value) + 1;

    dup = WSDAllocateLinkedMemory(parentMemoryBlock, valueLen * sizeof(WCHAR));

    if (dup) memcpy(dup, value, valueLen * sizeof(WCHAR));
    return dup;
}

static WSDXML_NAME *duplicate_name(void *parentMemoryBlock, WSDXML_NAME *name)
{
    WSDXML_NAME *dup;

    dup = WSDAllocateLinkedMemory(parentMemoryBlock, sizeof(WSDXML_NAME));

    if (dup == NULL)
    {
        return NULL;
    }

    dup->Space = name->Space;
    dup->LocalName = duplicate_string(dup, name->LocalName);

    if (dup->LocalName == NULL)
    {
        WSDFreeLinkedMemory(dup);
        return NULL;
    }

    return dup;
}

HRESULT WINAPI WSDXMLAddChild(WSDXML_ELEMENT *pParent, WSDXML_ELEMENT *pChild)
{
    WSDXML_NODE *currentNode;

    TRACE("(%p, %p)\n", pParent, pChild);

    if ((pParent == NULL) || (pChild == NULL) || (pChild->Node.Parent != NULL))
    {
        return E_INVALIDARG;
    }

    /* See if the parent already has a child */
    currentNode = pParent->FirstChild;

    if (currentNode == NULL)
    {
        pParent->FirstChild = (WSDXML_NODE *)pChild;
    }
    else
    {
        /* Find the last sibling node and make this child the next sibling */
        WSDXMLAddSibling((WSDXML_ELEMENT *)currentNode, pChild);
    }

    pChild->Node.Parent = pParent;

    /* Link the memory allocations */
    WSDAttachLinkedMemory(pParent, pChild);

    return S_OK;
}

HRESULT WINAPI WSDXMLAddSibling(WSDXML_ELEMENT *pFirst, WSDXML_ELEMENT *pSecond)
{
    WSDXML_NODE *currentNode;

    TRACE("(%p, %p)\n", pFirst, pSecond);

    if ((pFirst == NULL) || (pSecond == NULL))
    {
        return E_INVALIDARG;
    }

    /* See if the first node already has a sibling */
    currentNode = pFirst->Node.Next;

    if (currentNode == NULL)
    {
        pFirst->Node.Next = (WSDXML_NODE *)pSecond;
    }
    else
    {
        /* Find the last sibling node and make the second element the next sibling */
        while (1)
        {
            if (currentNode->Next == NULL)
            {
                currentNode->Next = (WSDXML_NODE *)pSecond;
                break;
            }

            currentNode = currentNode->Next;
        }
    }

    /* Reparent the second node under the first */
    pSecond->Node.Parent = pFirst->Node.Parent;

    /* Link the memory allocations */
    WSDAttachLinkedMemory(pFirst->Node.Parent, pSecond);

    return S_OK;
}

HRESULT WINAPI WSDXMLBuildAnyForSingleElement(WSDXML_NAME *pElementName, LPCWSTR pszText, WSDXML_ELEMENT **ppAny)
{
    WSDXML_TEXT *child;

    TRACE("(%p, %s, %p)\n", pElementName, debugstr_w(pszText), ppAny);

    if ((pElementName == NULL) || ((pszText != NULL) && (lstrlenW(pszText) > WSD_MAX_TEXT_LENGTH)))
    {
        return E_INVALIDARG;
    }

    if (ppAny == NULL)
    {
        return E_POINTER;
    }

    *ppAny = WSDAllocateLinkedMemory(NULL, sizeof(WSDXML_ELEMENT));

    if (*ppAny == NULL)
    {
        return E_OUTOFMEMORY;
    }

    ZeroMemory(*ppAny, sizeof(WSDXML_ELEMENT));

    (*ppAny)->Name = duplicate_name(*ppAny, pElementName);

    if ((*ppAny)->Name == NULL)
    {
        WSDFreeLinkedMemory(*ppAny);
        return E_OUTOFMEMORY;
    }

    if (pszText != NULL)
    {
        child = WSDAllocateLinkedMemory(*ppAny, sizeof(WSDXML_TEXT));

        if (child == NULL)
        {
            WSDFreeLinkedMemory(*ppAny);
            return E_OUTOFMEMORY;
        }

        child->Node.Parent = *ppAny;
        child->Node.Next = NULL;
        child->Node.Type = TextType;
        child->Text = duplicate_string(child, pszText);

        if (child->Text == NULL)
        {
            WSDFreeLinkedMemory(*ppAny);
            return E_OUTOFMEMORY;
        }

        (*ppAny)->FirstChild = (WSDXML_NODE *)child;
    }

    return S_OK;
}

HRESULT WINAPI WSDXMLCleanupElement(WSDXML_ELEMENT *pAny)
{
    TRACE("(%p)\n", pAny);

    if (pAny == NULL)
    {
        return E_INVALIDARG;
    }

    WSDFreeLinkedMemory(pAny);
    return S_OK;
}

/* IWSDXMLContext implementation */

typedef struct IWSDXMLContextImpl
{
    IWSDXMLContext IWSDXMLContext_iface;
    LONG ref;
} IWSDXMLContextImpl;

static inline IWSDXMLContextImpl *impl_from_IWSDXMLContext(IWSDXMLContext *iface)
{
    return CONTAINING_RECORD(iface, IWSDXMLContextImpl, IWSDXMLContext_iface);
}

static HRESULT WINAPI IWSDXMLContextImpl_QueryInterface(IWSDXMLContext *iface, REFIID riid, void **ppv)
{
    IWSDXMLContextImpl *This = impl_from_IWSDXMLContext(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
    {
        WARN("Invalid parameter\n");
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IWSDXMLContext))
    {
        *ppv = &This->IWSDXMLContext_iface;
    }
    else
    {
        WARN("Unknown IID %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI IWSDXMLContextImpl_AddRef(IWSDXMLContext *iface)
{
    IWSDXMLContextImpl *This = impl_from_IWSDXMLContext(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI IWSDXMLContextImpl_Release(IWSDXMLContext *iface)
{
    IWSDXMLContextImpl *This = impl_from_IWSDXMLContext(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (ref == 0)
    {
        WSDFreeLinkedMemory(This);
    }

    return ref;
}

static HRESULT WINAPI IWSDXMLContextImpl_AddNamespace(IWSDXMLContext *iface, LPCWSTR pszUri, LPCWSTR pszSuggestedPrefix, WSDXML_NAMESPACE **ppNamespace)
{
    FIXME("(%p, %s, %s, %p)\n", iface, debugstr_w(pszUri), debugstr_w(pszSuggestedPrefix), ppNamespace);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDXMLContextImpl_AddNameToNamespace(IWSDXMLContext *iface, LPCWSTR pszUri, LPCWSTR pszName, WSDXML_NAME **ppName)
{
    FIXME("(%p, %s, %s, %p)\n", iface, debugstr_w(pszUri), debugstr_w(pszName), ppName);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDXMLContextImpl_SetNamespaces(IWSDXMLContext *iface, const PCWSDXML_NAMESPACE *pNamespaces, WORD wNamespacesCount, BYTE bLayerNumber)
{
    FIXME("(%p, %p, %d, %d)\n", iface, pNamespaces, wNamespacesCount, bLayerNumber);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDXMLContextImpl_SetTypes(IWSDXMLContext *iface, const PCWSDXML_TYPE *pTypes, DWORD dwTypesCount, BYTE bLayerNumber)
{
    FIXME("(%p, %p, %d, %d)\n", iface, pTypes, dwTypesCount, bLayerNumber);
    return E_NOTIMPL;
}

static const IWSDXMLContextVtbl xmlcontext_vtbl =
{
    IWSDXMLContextImpl_QueryInterface,
    IWSDXMLContextImpl_AddRef,
    IWSDXMLContextImpl_Release,
    IWSDXMLContextImpl_AddNamespace,
    IWSDXMLContextImpl_AddNameToNamespace,
    IWSDXMLContextImpl_SetNamespaces,
    IWSDXMLContextImpl_SetTypes
};

HRESULT WINAPI WSDXMLCreateContext(IWSDXMLContext **ppContext)
{
    IWSDXMLContextImpl *obj;

    TRACE("(%p)", ppContext);

    if (ppContext == NULL)
    {
        WARN("Invalid parameter: ppContext == NULL\n");
        return E_POINTER;
    }

    *ppContext = NULL;

    obj = WSDAllocateLinkedMemory(NULL, sizeof(*obj));

    if (!obj)
    {
        return E_OUTOFMEMORY;
    }

    obj->IWSDXMLContext_iface.lpVtbl = &xmlcontext_vtbl;
    obj->ref = 1;

    *ppContext = &obj->IWSDXMLContext_iface;
    TRACE("Returning iface %p\n", *ppContext);

    return S_OK;
}
