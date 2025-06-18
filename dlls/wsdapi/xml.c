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

#include "wsdapi_internal.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wsdapi);

LPWSTR duplicate_string(void *parentMemoryBlock, LPCWSTR value)
{
    int valueLen;
    LPWSTR dup;

    valueLen = lstrlenW(value) + 1;

    dup = WSDAllocateLinkedMemory(parentMemoryBlock, valueLen * sizeof(WCHAR));

    if (dup) memcpy(dup, value, valueLen * sizeof(WCHAR));
    return dup;
}

static WSDXML_NAMESPACE *duplicate_namespace(void *parentMemoryBlock, WSDXML_NAMESPACE *ns)
{
    WSDXML_NAMESPACE *newNs;

    newNs = WSDAllocateLinkedMemory(parentMemoryBlock, sizeof(WSDXML_NAMESPACE));

    if (newNs == NULL)
    {
        return NULL;
    }

    newNs->Encoding = ns->Encoding;

    /* On Windows, both Names and NamesCount are set to null even if there are names present */
    newNs->NamesCount = 0;
    newNs->Names = NULL;

    newNs->PreferredPrefix = duplicate_string(newNs, ns->PreferredPrefix);
    newNs->Uri = duplicate_string(newNs, ns->Uri);

    return newNs;
}

WSDXML_NAME *duplicate_name(void *parentMemoryBlock, WSDXML_NAME *name)
{
    WSDXML_NAME *dup;

    dup = WSDAllocateLinkedMemory(parentMemoryBlock, sizeof(WSDXML_NAME));

    if (dup == NULL)
    {
        return NULL;
    }

    dup->Space = duplicate_namespace(dup, name->Space);
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

HRESULT WINAPI WSDXMLGetValueFromAny(const WCHAR *pszNamespace, const WCHAR *pszName, WSDXML_ELEMENT *pAny, LPCWSTR *ppszValue)
{
    WSDXML_ELEMENT *element;
    WSDXML_TEXT *text;

    if (pAny == NULL)
        return E_INVALIDARG;

    if (ppszValue == NULL)
        return E_POINTER;

    if ((pszNamespace == NULL) || (pszName == NULL) || (lstrlenW(pszNamespace) > WSD_MAX_TEXT_LENGTH) || (lstrlenW(pszName) > WSD_MAX_TEXT_LENGTH))
        return E_INVALIDARG;

    element = pAny;

    while (element != NULL)
    {
        if (element->Node.Type == ElementType)
        {
            if ((lstrcmpW(element->Name->LocalName, pszName) == 0) && (lstrcmpW(element->Name->Space->Uri, pszNamespace) == 0))
            {
                if ((element->FirstChild == NULL) || (element->FirstChild->Type != TextType))
                {
                    return E_FAIL;
                }

                text = (WSDXML_TEXT *) element->FirstChild;
                *ppszValue = (LPCWSTR) text->Text;

                return S_OK;
            }
        }

        element = (WSDXML_ELEMENT *) element->Node.Next;
    }

    return E_FAIL;
}

/* IWSDXMLContext implementation */

struct xmlNamespace
{
    struct list entry;
    WSDXML_NAMESPACE *namespace;
};

typedef struct IWSDXMLContextImpl
{
    IWSDXMLContext IWSDXMLContext_iface;
    LONG ref;

    struct list *namespaces;
    int nextUnknownPrefix;
} IWSDXMLContextImpl;

static WSDXML_NAMESPACE *find_namespace(struct list *namespaces, LPCWSTR uri)
{
    struct xmlNamespace *ns;

    LIST_FOR_EACH_ENTRY(ns, namespaces, struct xmlNamespace, entry)
    {
        if (lstrcmpW(ns->namespace->Uri, uri) == 0)
        {
            return ns->namespace;
        }
    }

    return NULL;
}

static WSDXML_NAME *find_name(WSDXML_NAMESPACE *ns, LPCWSTR name)
{
    int i;

    for (i = 0; i < ns->NamesCount; i++)
    {
        if (lstrcmpW(ns->Names[i].LocalName, name) == 0)
        {
            return &ns->Names[i];
        }
    }

    return NULL;
}

static WSDXML_NAME *add_name(WSDXML_NAMESPACE *ns, LPCWSTR name)
{
    WSDXML_NAME *names;
    WSDXML_NAME *newName;
    int i;

    names = WSDAllocateLinkedMemory(ns, sizeof(WSDXML_NAME) * (ns->NamesCount + 1));

    if (names == NULL)
    {
        return NULL;
    }

    if (ns->NamesCount > 0)
    {
        /* Copy the existing names array over to the new allocation */
        memcpy(names, ns->Names, sizeof(WSDXML_NAME) * ns->NamesCount);

        for (i = 0; i < ns->NamesCount; i++)
        {
            /* Attach the local name memory to the new names allocation */
            WSDAttachLinkedMemory(names, names[i].LocalName);
        }

        WSDFreeLinkedMemory(ns->Names);
    }

    ns->Names = names;

    newName = &names[ns->NamesCount];

    newName->LocalName = duplicate_string(names, name);
    newName->Space = ns;

    if (newName->LocalName == NULL)
    {
        return NULL;
    }

    ns->NamesCount++;
    return newName;
}

static BOOL is_prefix_unique(struct list *namespaces, LPCWSTR prefix)
{
    struct xmlNamespace *ns;

    LIST_FOR_EACH_ENTRY(ns, namespaces, struct xmlNamespace, entry)
    {
        if (lstrcmpW(ns->namespace->PreferredPrefix, prefix) == 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}

static LPWSTR generate_namespace_prefix(IWSDXMLContextImpl *impl, void *parentMemoryBlock, LPCWSTR uri)
{
    WCHAR suggestedPrefix[7];

    /* Find a unique prefix */
    while (impl->nextUnknownPrefix < 1000)
    {
        wsprintfW(suggestedPrefix, L"un%d", impl->nextUnknownPrefix++);

        /* For the unlikely event where somebody has explicitly created a prefix called 'unX', check it is unique */
        if (is_prefix_unique(impl->namespaces, suggestedPrefix))
        {
            return duplicate_string(parentMemoryBlock, suggestedPrefix);
        }
    }

    return NULL;
}

static WSDXML_NAMESPACE *add_namespace(struct list *namespaces, LPCWSTR uri)
{
    struct xmlNamespace *ns = WSDAllocateLinkedMemory(namespaces, sizeof(struct xmlNamespace));

    if (ns == NULL)
    {
        return NULL;
    }

    ns->namespace = WSDAllocateLinkedMemory(ns, sizeof(WSDXML_NAMESPACE));

    if (ns->namespace == NULL)
    {
        WSDFreeLinkedMemory(ns);
        return NULL;
    }

    ZeroMemory(ns->namespace, sizeof(WSDXML_NAMESPACE));
    ns->namespace->Uri = duplicate_string(ns->namespace, uri);

    if (ns->namespace->Uri == NULL)
    {
        WSDFreeLinkedMemory(ns);
        return NULL;
    }

    list_add_tail(namespaces, &ns->entry);
    return ns->namespace;
}

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

    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI IWSDXMLContextImpl_Release(IWSDXMLContext *iface)
{
    IWSDXMLContextImpl *This = impl_from_IWSDXMLContext(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (ref == 0)
    {
        WSDFreeLinkedMemory(This);
    }

    return ref;
}

static HRESULT WINAPI IWSDXMLContextImpl_AddNamespace(IWSDXMLContext *iface, LPCWSTR pszUri, LPCWSTR pszSuggestedPrefix, WSDXML_NAMESPACE **ppNamespace)
{
    IWSDXMLContextImpl *This = impl_from_IWSDXMLContext(iface);
    WSDXML_NAMESPACE *ns;
    LPCWSTR newPrefix = NULL;
    BOOL setNewPrefix;

    TRACE("(%p, %s, %s, %p)\n", This, debugstr_w(pszUri), debugstr_w(pszSuggestedPrefix), ppNamespace);

    if ((pszUri == NULL) || (pszSuggestedPrefix == NULL) || (lstrlenW(pszUri) > WSD_MAX_TEXT_LENGTH) ||
        (lstrlenW(pszSuggestedPrefix) > WSD_MAX_TEXT_LENGTH))
    {
        return E_INVALIDARG;
    }

    ns = find_namespace(This->namespaces, pszUri);

    if (ns == NULL)
    {
        ns = add_namespace(This->namespaces, pszUri);

        if (ns == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }

    setNewPrefix = (ns->PreferredPrefix == NULL);

    if ((ns->PreferredPrefix == NULL) || (lstrcmpW(ns->PreferredPrefix, pszSuggestedPrefix) != 0))
    {
        newPrefix = pszSuggestedPrefix;
        setNewPrefix = TRUE;
    }

    if (setNewPrefix)
    {
        WSDFreeLinkedMemory((void *)ns->PreferredPrefix);
        ns->PreferredPrefix = NULL;

        if ((newPrefix != NULL) && (is_prefix_unique(This->namespaces, newPrefix)))
        {
            ns->PreferredPrefix = duplicate_string(ns, newPrefix);
        }
        else
        {
            ns->PreferredPrefix = generate_namespace_prefix(This, ns, pszUri);
            if (ns->PreferredPrefix == NULL)
            {
                return E_FAIL;
            }
        }
    }

    if (ppNamespace != NULL)
    {
        *ppNamespace = duplicate_namespace(NULL, ns);

        if (*ppNamespace == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}

static HRESULT WINAPI IWSDXMLContextImpl_AddNameToNamespace(IWSDXMLContext *iface, LPCWSTR pszUri, LPCWSTR pszName, WSDXML_NAME **ppName)
{
    IWSDXMLContextImpl *This = impl_from_IWSDXMLContext(iface);
    WSDXML_NAMESPACE *ns;
    WSDXML_NAME *name;

    TRACE("(%p, %s, %s, %p)\n", This, debugstr_w(pszUri), debugstr_w(pszName), ppName);

    if ((pszUri == NULL) || (pszName == NULL) || (lstrlenW(pszUri) > WSD_MAX_TEXT_LENGTH) || (lstrlenW(pszName) > WSD_MAX_TEXT_LENGTH))
    {
        return E_INVALIDARG;
    }

    ns = find_namespace(This->namespaces, pszUri);

    if (ns == NULL)
    {
        /* The namespace doesn't exist, add it */
        ns = add_namespace(This->namespaces, pszUri);

        if (ns == NULL)
        {
            return E_OUTOFMEMORY;
        }

        ns->PreferredPrefix = generate_namespace_prefix(This, ns, pszUri);
        if (ns->PreferredPrefix == NULL)
        {
            return E_FAIL;
        }
    }

    name = find_name(ns, pszName);

    if (name == NULL)
    {
        name = add_name(ns, pszName);

        if (name == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }

    if (ppName != NULL)
    {
        *ppName = duplicate_name(NULL, name);

        if (*ppName == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}

static HRESULT WINAPI IWSDXMLContextImpl_SetNamespaces(IWSDXMLContext *iface, const PCWSDXML_NAMESPACE *pNamespaces, WORD wNamespacesCount, BYTE bLayerNumber)
{
    FIXME("(%p, %p, %d, %d)\n", iface, pNamespaces, wNamespacesCount, bLayerNumber);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDXMLContextImpl_SetTypes(IWSDXMLContext *iface, const PCWSDXML_TYPE *pTypes, DWORD dwTypesCount, BYTE bLayerNumber)
{
    FIXME("(%p, %p, %ld, %d)\n", iface, pTypes, dwTypesCount, bLayerNumber);
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

    TRACE("(%p)\n", ppContext);

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
    obj->namespaces = WSDAllocateLinkedMemory(obj, sizeof(struct list));
    obj->nextUnknownPrefix = 0;

    if (obj->namespaces == NULL)
    {
        WSDFreeLinkedMemory(obj);
        return E_OUTOFMEMORY;
    }

    list_init(obj->namespaces);

    *ppContext = &obj->IWSDXMLContext_iface;
    TRACE("Returning iface %p\n", *ppContext);

    return S_OK;
}

WSDXML_NAMESPACE *xml_context_find_namespace_by_prefix(IWSDXMLContext *context, LPCWSTR prefix)
{
    IWSDXMLContextImpl *impl = impl_from_IWSDXMLContext(context);
    struct xmlNamespace *ns;

    if (prefix == NULL) return NULL;

    LIST_FOR_EACH_ENTRY(ns, impl->namespaces, struct xmlNamespace, entry)
    {
        if (lstrcmpW(ns->namespace->PreferredPrefix, prefix) == 0)
            return ns->namespace;
    }

    return NULL;
}
