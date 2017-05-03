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
