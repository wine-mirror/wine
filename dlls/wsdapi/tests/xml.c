/*
 * Web Services on Devices
 * XML tests
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

#define COBJMACROS

#include <windows.h>

#include "wine/test.h"
#include "wsdapi.h"

static void BuildAnyForSingleElement_tests(void)
{
    WSDXML_ELEMENT *element;
    WSDXML_NAME name;
    WSDXML_NAMESPACE ns;
    WCHAR nameText[] = {'E','l','1',0};
    WCHAR text[] = {'H','e','l','l','o',0};
    static const WCHAR uri[] = {'h','t','t','p',':','/','/','t','e','s','t','.','t','e','s','t','/',0};
    static const WCHAR prefix[] = {'t',0};
    LPWSTR largeText;
    const int largeTextSize = 10000 * sizeof(WCHAR);
    HRESULT hr;

    /* Populate structures */
    ns.Uri = uri;
    ns.PreferredPrefix = prefix;

    name.LocalName = nameText;
    name.Space = &ns;

    /* Test invalid arguments */
    hr = WSDXMLBuildAnyForSingleElement(NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "BuildAnyForSingleElement failed with %08x\n", hr);

    hr = WSDXMLBuildAnyForSingleElement(&name, NULL, NULL);
    ok(hr == E_POINTER, "BuildAnyForSingleElement failed with %08x\n", hr);

    /* Test calling the function with a text size that exceeds 8192 characters */
    largeText = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, largeTextSize + sizeof(WCHAR));
    memset(largeText, 'a', largeTextSize);

    hr = WSDXMLBuildAnyForSingleElement(&name, largeText, &element);
    ok(hr == E_INVALIDARG, "BuildAnyForSingleElement failed with %08x\n", hr);

    /* Test with valid parameters but no text */

    hr = WSDXMLBuildAnyForSingleElement(&name, NULL, &element);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08x\n", hr);

    ok(element->Name != &name, "element->Name has not been duplicated\n");
    todo_wine ok(element->Name->Space != name.Space, "element->Name->Space has not been duplicated\n");
    ok(element->Name->LocalName != name.LocalName, "element->LocalName has not been duplicated\n");
    ok(lstrcmpW(element->Name->LocalName, name.LocalName) == 0, "element->LocalName = '%s', expected '%s'\n",
        wine_dbgstr_w(element->Name->LocalName), wine_dbgstr_w(name.LocalName));
    ok(element->FirstChild == NULL, "element->FirstChild == %p\n", element->FirstChild);
    ok(element->Node.Next == NULL, "element->Node.Next == %p\n", element->Node.Next);
    ok(element->Node.Type == ElementType, "element->Node.Type == %d\n", element->Node.Type);

    WSDFreeLinkedMemory(element);

    /* Test with valid parameters and text */

    hr = WSDXMLBuildAnyForSingleElement(&name, text, &element);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08x\n", hr);

    ok(element->Name != &name, "element->Name has not been duplicated\n");
    todo_wine ok(element->Name->Space != name.Space, "element->Name->Space has not been duplicated\n");
    ok(element->Name->LocalName != name.LocalName, "element->LocalName has not been duplicated\n");
    ok(lstrcmpW(element->Name->LocalName, name.LocalName) == 0, "element->LocalName = '%s', expected '%s'\n",
        wine_dbgstr_w(element->Name->LocalName), wine_dbgstr_w(name.LocalName));
    ok(element->FirstChild != NULL, "element->FirstChild == %p\n", element->FirstChild);
    ok(element->Node.Next == NULL, "element->Node.Next == %p\n", element->Node.Next);
    ok(element->Node.Type == ElementType, "element->Node.Type == %d\n", element->Node.Type);

    if (element->FirstChild != NULL)
    {
        ok(element->FirstChild->Parent == element, "element->FirstChild->Parent = %p, expected %p\n", element->FirstChild->Parent, element);
        ok(element->FirstChild->Next == NULL, "element->FirstChild.Next == %p\n", element->FirstChild->Next);
        ok(element->FirstChild->Type == TextType, "element->FirstChild.Type == %d\n", element->Node.Type);
        ok(((WSDXML_TEXT *)element->FirstChild)->Text != NULL, "element->FirstChild.Text is null\n");
        ok(lstrcmpW(((WSDXML_TEXT *)element->FirstChild)->Text, text) == 0, "element->FirstChild->Text = '%s', expected '%s'\n",
            wine_dbgstr_w(((WSDXML_TEXT *)element->FirstChild)->Text), wine_dbgstr_w(text));
    }

    WSDFreeLinkedMemory(element);
}

START_TEST(xml)
{
    BuildAnyForSingleElement_tests();
}
