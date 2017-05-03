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

static void AddChild_tests(void)
{
    WSDXML_ELEMENT *parent, *child1, *child2;
    WSDXML_NAME parentName, child1Name, child2Name;
    WSDXML_NAMESPACE ns;
    WCHAR parentNameText[] = {'D','a','d',0};
    WCHAR child1NameText[] = {'T','i','m',0};
    WCHAR child2NameText[] = {'B','o','b',0};
    static const WCHAR uri[] = {'h','t','t','p',':','/','/','t','e','s','t','.','t','e','s','t','/',0};
    static const WCHAR prefix[] = {'t',0};
    HRESULT hr;

    /* Test invalid values */
    hr = WSDXMLAddChild(NULL, NULL);
    ok(hr == E_INVALIDARG, "WSDXMLAddChild failed with %08x\n", hr);

    hr = WSDXMLAddChild(parent, NULL);
    ok(hr == E_INVALIDARG, "WSDXMLAddChild failed with %08x\n", hr);

    hr = WSDXMLAddChild(NULL, child1);
    ok(hr == E_INVALIDARG, "WSDXMLAddChild failed with %08x\n", hr);

    /* Populate structures */
    ns.Uri = uri;
    ns.PreferredPrefix = prefix;

    parentName.LocalName = parentNameText;
    parentName.Space = &ns;

    child1Name.LocalName = child1NameText;
    child1Name.Space = &ns;

    child2Name.LocalName = child2NameText;
    child2Name.Space = &ns;

    /* Create some elements */
    hr = WSDXMLBuildAnyForSingleElement(&parentName, NULL, &parent);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08x\n", hr);

    hr = WSDXMLBuildAnyForSingleElement(&child1Name, child1NameText, &child1);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08x\n", hr);

    hr = WSDXMLBuildAnyForSingleElement(&child2Name, NULL, &child2);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08x\n", hr);

    ok(parent->Node.Parent == NULL, "parent->Node.Parent == %p\n", parent->Node.Parent);
    ok(parent->FirstChild == NULL, "parent->FirstChild == %p\n", parent->FirstChild);
    ok(parent->Node.Next == NULL, "parent->Node.Next == %p\n", parent->Node.Next);
    ok(child1->Node.Parent == NULL, "child1->Node.Parent == %p\n", child1->Node.Parent);
    ok(child1->FirstChild != NULL, "child1->FirstChild == NULL\n");
    ok(child1->FirstChild->Type == TextType, "child1->FirstChild.Type == %d\n", child1->FirstChild->Type);
    ok(child1->Node.Next == NULL, "child1->Node.Next == %p\n", child1->Node.Next);

    /* Add the child to the parent */
    hr = WSDXMLAddChild(parent, child1);
    ok(hr == S_OK, "WSDXMLAddChild failed with %08x\n", hr);

    ok(parent->Node.Parent == NULL, "parent->Node.Parent == %p\n", parent->Node.Parent);
    ok(parent->FirstChild == (WSDXML_NODE *)child1, "parent->FirstChild == %p\n", parent->FirstChild);
    ok(parent->Node.Next == NULL, "parent->Node.Next == %p\n", parent->Node.Next);
    ok(child1->Node.Parent == parent, "child1->Node.Parent == %p\n", child1->Node.Parent);
    ok(child1->FirstChild != NULL, "child1->FirstChild == NULL\n");
    ok(child1->FirstChild->Type == TextType, "child1->FirstChild.Type == %d\n", child1->FirstChild->Type);
    ok(child1->Node.Next == NULL, "child1->Node.Next == %p\n", child1->Node.Next);

    /* Try to set child1 as the child of child2, which already has a parent */
    hr = WSDXMLAddChild(child2, child1);
    ok(hr == E_INVALIDARG, "WSDXMLAddChild failed with %08x\n", hr);

    /* Try to set child2 as the child of child1, which has a text node as a child */
    hr = WSDXMLAddChild(child2, child1);
    ok(hr == E_INVALIDARG, "WSDXMLAddChild failed with %08x\n", hr);

    /* Add child2 as a child of parent */
    hr = WSDXMLAddChild(parent, child2);
    ok(hr == S_OK, "WSDXMLAddChild failed with %08x\n", hr);

    ok(parent->Node.Parent == NULL, "parent->Node.Parent == %p\n", parent->Node.Parent);
    ok(parent->FirstChild == (WSDXML_NODE *)child1, "parent->FirstChild == %p\n", parent->FirstChild);
    ok(parent->Node.Next == NULL, "parent->Node.Next == %p\n", parent->Node.Next);
    ok(child1->Node.Parent == parent, "child1->Node.Parent == %p\n", child1->Node.Parent);
    ok(child1->FirstChild != NULL, "child1->FirstChild == NULL\n");
    ok(child1->FirstChild->Type == TextType, "child1->FirstChild.Type == %d\n", child1->FirstChild->Type);
    ok(child1->Node.Next == (WSDXML_NODE *)child2, "child1->Node.Next == %p\n", child1->Node.Next);
    ok(child2->Node.Parent == parent, "child2->Node.Parent == %p\n", child2->Node.Parent);
    ok(child2->FirstChild == NULL, "child2->FirstChild == %p\n", child2->FirstChild);
    ok(child2->Node.Next == NULL, "child2->Node.Next == %p\n", child2->Node.Next);

    WSDFreeLinkedMemory(parent);
}

static void AddSibling_tests(void)
{
    WSDXML_ELEMENT *parent, *child1, *child2, *child3;
    WSDXML_NAME parentName, child1Name, child2Name;
    WSDXML_NAMESPACE ns;
    WCHAR parentNameText[] = {'D','a','d',0};
    WCHAR child1NameText[] = {'T','i','m',0};
    WCHAR child2NameText[] = {'B','o','b',0};
    static const WCHAR uri[] = {'h','t','t','p',':','/','/','t','e','s','t','.','t','e','s','t','/',0};
    static const WCHAR prefix[] = {'t',0};
    HRESULT hr;

    /* Test invalid values */
    hr = WSDXMLAddSibling(NULL, NULL);
    ok(hr == E_INVALIDARG, "WSDXMLAddSibling failed with %08x\n", hr);

    hr = WSDXMLAddSibling(child1, NULL);
    ok(hr == E_INVALIDARG, "WSDXMLAddSibling failed with %08x\n", hr);

    hr = WSDXMLAddSibling(NULL, child2);
    ok(hr == E_INVALIDARG, "WSDXMLAddSibling failed with %08x\n", hr);

    /* Populate structures */
    ns.Uri = uri;
    ns.PreferredPrefix = prefix;

    parentName.LocalName = parentNameText;
    parentName.Space = &ns;

    child1Name.LocalName = child1NameText;
    child1Name.Space = &ns;

    child2Name.LocalName = child2NameText;
    child2Name.Space = &ns;

    /* Create some elements */
    hr = WSDXMLBuildAnyForSingleElement(&parentName, NULL, &parent);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08x\n", hr);

    hr = WSDXMLBuildAnyForSingleElement(&child1Name, child1NameText, &child1);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08x\n", hr);

    hr = WSDXMLBuildAnyForSingleElement(&child2Name, NULL, &child2);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08x\n", hr);

    hr = WSDXMLBuildAnyForSingleElement(&child2Name, NULL, &child3);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08x\n", hr);

    /* Add child1 to parent */
    hr = WSDXMLAddChild(parent, child1);
    ok(hr == S_OK, "WSDXMLAddChild failed with %08x\n", hr);

    ok(parent->Node.Parent == NULL, "parent->Node.Parent == %p\n", parent->Node.Parent);
    ok(parent->FirstChild == (WSDXML_NODE *)child1, "parent->FirstChild == %p\n", parent->FirstChild);
    ok(parent->Node.Next == NULL, "parent->Node.Next == %p\n", parent->Node.Next);
    ok(child1->Node.Parent == parent, "child1->Node.Parent == %p\n", child1->Node.Parent);
    ok(child1->FirstChild != NULL, "child1->FirstChild == NULL\n");
    ok(child1->FirstChild->Type == TextType, "child1->FirstChild.Type == %d\n", child1->FirstChild->Type);
    ok(child1->Node.Next == NULL, "child1->Node.Next == %p\n", child1->Node.Next);
    ok(child2->Node.Parent == NULL, "child2->Node.Parent == %p\n", child2->Node.Parent);
    ok(child2->FirstChild == NULL, "child2->FirstChild == %p\n", child2->FirstChild);
    ok(child2->Node.Next == NULL, "child2->Node.Next == %p\n", child2->Node.Next);

    /* Try to add child2 as sibling of child1 */
    hr = WSDXMLAddSibling(child1, child2);
    ok(hr == S_OK, "WSDXMLAddSibling failed with %08x\n", hr);

    ok(child1->Node.Parent == parent, "child1->Node.Parent == %p\n", child1->Node.Parent);
    ok(child1->Node.Next == (WSDXML_NODE *)child2, "child1->Node.Next == %p\n", child1->Node.Next);
    ok(child2->Node.Parent == parent, "child2->Node.Parent == %p\n", child2->Node.Parent);
    ok(child2->Node.Next == NULL, "child2->Node.Next == %p\n", child2->Node.Next);
    ok(parent->FirstChild == (WSDXML_NODE *)child1, "parent->FirstChild == %p\n", parent->FirstChild);

    /* Try to add child3 as sibling of child1 */
    hr = WSDXMLAddSibling(child1, child3);
    ok(hr == S_OK, "WSDXMLAddSibling failed with %08x\n", hr);

    ok(child1->Node.Parent == parent, "child1->Node.Parent == %p\n", child1->Node.Parent);
    ok(child1->Node.Next == (WSDXML_NODE *)child2, "child1->Node.Next == %p\n", child1->Node.Next);
    ok(child2->Node.Parent == parent, "child2->Node.Parent == %p\n", child2->Node.Parent);
    ok(child2->Node.Next == (WSDXML_NODE *)child3, "child2->Node.Next == %p\n", child2->Node.Next);
    ok(child3->Node.Parent == parent, "child2->Node.Parent == %p\n", child2->Node.Parent);
    ok(child3->Node.Next == NULL, "child2->Node.Next == %p\n", child2->Node.Next);
    ok(parent->FirstChild == (WSDXML_NODE *)child1, "parent->FirstChild == %p\n", parent->FirstChild);

    WSDFreeLinkedMemory(parent);
}

START_TEST(xml)
{
    BuildAnyForSingleElement_tests();
    AddChild_tests();
    AddSibling_tests();
}
