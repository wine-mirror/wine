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

static LPWSTR largeText;
static const int largeTextSize = 10000 * sizeof(WCHAR);

static void BuildAnyForSingleElement_tests(void)
{
    WSDXML_ELEMENT *element;
    WSDXML_NAME name;
    WSDXML_NAMESPACE ns;
    static const WCHAR *text = L"Hello";
    HRESULT hr;

    /* Populate structures */
    ns.Uri = L"http://test.test/";
    ns.PreferredPrefix = L"t";

    name.LocalName = (WCHAR *) L"El1";
    name.Space = &ns;

    /* Test invalid arguments */
    hr = WSDXMLBuildAnyForSingleElement(NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "BuildAnyForSingleElement failed with %08lx\n", hr);

    hr = WSDXMLBuildAnyForSingleElement(&name, NULL, NULL);
    ok(hr == E_POINTER, "BuildAnyForSingleElement failed with %08lx\n", hr);

    /* Test calling the function with a text size that exceeds 8192 characters */
    hr = WSDXMLBuildAnyForSingleElement(&name, largeText, &element);
    ok(hr == E_INVALIDARG, "BuildAnyForSingleElement failed with %08lx\n", hr);

    /* Test with valid parameters but no text */

    hr = WSDXMLBuildAnyForSingleElement(&name, NULL, &element);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08lx\n", hr);

    ok(element->Name != &name, "element->Name has not been duplicated\n");
    ok(element->Name->Space != name.Space, "element->Name->Space has not been duplicated\n");
    ok(element->Name->LocalName != name.LocalName, "element->LocalName has not been duplicated\n");
    ok(lstrcmpW(element->Name->LocalName, name.LocalName) == 0, "element->LocalName = %s, expected %s\n",
        wine_dbgstr_w(element->Name->LocalName), wine_dbgstr_w(name.LocalName));
    ok(element->FirstChild == NULL, "element->FirstChild == %p\n", element->FirstChild);
    ok(element->Node.Next == NULL, "element->Node.Next == %p\n", element->Node.Next);
    ok(element->Node.Type == ElementType, "element->Node.Type == %d\n", element->Node.Type);

    WSDFreeLinkedMemory(element);

    /* Test with valid parameters and text */

    hr = WSDXMLBuildAnyForSingleElement(&name, text, &element);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08lx\n", hr);

    ok(element->Name != &name, "element->Name has not been duplicated\n");
    ok(element->Name->Space != name.Space, "element->Name->Space has not been duplicated\n");
    ok(element->Name->LocalName != name.LocalName, "element->LocalName has not been duplicated\n");
    ok(lstrcmpW(element->Name->LocalName, name.LocalName) == 0, "element->LocalName = %s, expected %s\n",
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
        ok(lstrcmpW(((WSDXML_TEXT *)element->FirstChild)->Text, text) == 0, "element->FirstChild->Text = %s, expected %s\n",
            wine_dbgstr_w(((WSDXML_TEXT *)element->FirstChild)->Text), wine_dbgstr_w(text));
    }

    WSDFreeLinkedMemory(element);
}

static void AddChild_tests(void)
{
    WSDXML_ELEMENT *parent, *child1, *child2;
    WSDXML_NAME parentName, child1Name, child2Name;
    WSDXML_NAMESPACE ns;
    static const WCHAR *child1NameText = L"Tim";
    HRESULT hr;

    /* Test invalid values */
    hr = WSDXMLAddChild(NULL, NULL);
    ok(hr == E_INVALIDARG, "WSDXMLAddChild failed with %08lx\n", hr);

    /* Populate structures */
    ns.Uri = L"http://test.test/";
    ns.PreferredPrefix = L"t";

    parentName.LocalName = (WCHAR *) L"Dad";
    parentName.Space = &ns;

    child1Name.LocalName = (WCHAR *) child1NameText;
    child1Name.Space = &ns;

    child2Name.LocalName = (WCHAR *) L"Bob";
    child2Name.Space = &ns;

    /* Create some elements */
    hr = WSDXMLBuildAnyForSingleElement(&parentName, NULL, &parent);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08lx\n", hr);

    hr = WSDXMLBuildAnyForSingleElement(&child1Name, child1NameText, &child1);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08lx\n", hr);

    hr = WSDXMLBuildAnyForSingleElement(&child2Name, NULL, &child2);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08lx\n", hr);

    ok(parent->Node.Parent == NULL, "parent->Node.Parent == %p\n", parent->Node.Parent);
    ok(parent->FirstChild == NULL, "parent->FirstChild == %p\n", parent->FirstChild);
    ok(parent->Node.Next == NULL, "parent->Node.Next == %p\n", parent->Node.Next);
    ok(child1->Node.Parent == NULL, "child1->Node.Parent == %p\n", child1->Node.Parent);
    ok(child1->FirstChild != NULL, "child1->FirstChild == NULL\n");
    ok(child1->FirstChild->Type == TextType, "child1->FirstChild.Type == %d\n", child1->FirstChild->Type);
    ok(child1->Node.Next == NULL, "child1->Node.Next == %p\n", child1->Node.Next);

    /* Add the child to the parent */
    hr = WSDXMLAddChild(parent, child1);
    ok(hr == S_OK, "WSDXMLAddChild failed with %08lx\n", hr);

    ok(parent->Node.Parent == NULL, "parent->Node.Parent == %p\n", parent->Node.Parent);
    ok(parent->FirstChild == (WSDXML_NODE *)child1, "parent->FirstChild == %p\n", parent->FirstChild);
    ok(parent->Node.Next == NULL, "parent->Node.Next == %p\n", parent->Node.Next);
    ok(child1->Node.Parent == parent, "child1->Node.Parent == %p\n", child1->Node.Parent);
    ok(child1->FirstChild != NULL, "child1->FirstChild == NULL\n");
    ok(child1->FirstChild->Type == TextType, "child1->FirstChild.Type == %d\n", child1->FirstChild->Type);
    ok(child1->Node.Next == NULL, "child1->Node.Next == %p\n", child1->Node.Next);

    /* Try to set child1 as the child of child2, which already has a parent */
    hr = WSDXMLAddChild(child2, child1);
    ok(hr == E_INVALIDARG, "WSDXMLAddChild failed with %08lx\n", hr);

    /* Try to set child2 as the child of child1, which has a text node as a child */
    hr = WSDXMLAddChild(child2, child1);
    ok(hr == E_INVALIDARG, "WSDXMLAddChild failed with %08lx\n", hr);

    /* Add child2 as a child of parent */
    hr = WSDXMLAddChild(parent, child2);
    ok(hr == S_OK, "WSDXMLAddChild failed with %08lx\n", hr);

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
    static const WCHAR *child1NameText = L"Tim";
    HRESULT hr;

    /* Test invalid values */
    hr = WSDXMLAddSibling(NULL, NULL);
    ok(hr == E_INVALIDARG, "WSDXMLAddSibling failed with %08lx\n", hr);

    /* Populate structures */
    ns.Uri = L"http://test.test/";
    ns.PreferredPrefix = L"t";

    parentName.LocalName = (WCHAR *) L"Dad";
    parentName.Space = &ns;

    child1Name.LocalName = (WCHAR *) child1NameText;
    child1Name.Space = &ns;

    child2Name.LocalName = (WCHAR *) L"Bob";
    child2Name.Space = &ns;

    /* Create some elements */
    hr = WSDXMLBuildAnyForSingleElement(&parentName, NULL, &parent);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08lx\n", hr);

    hr = WSDXMLBuildAnyForSingleElement(&child1Name, child1NameText, &child1);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08lx\n", hr);

    hr = WSDXMLBuildAnyForSingleElement(&child2Name, NULL, &child2);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08lx\n", hr);

    hr = WSDXMLBuildAnyForSingleElement(&child2Name, NULL, &child3);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08lx\n", hr);

    /* Add child1 to parent */
    hr = WSDXMLAddChild(parent, child1);
    ok(hr == S_OK, "WSDXMLAddChild failed with %08lx\n", hr);

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
    ok(hr == S_OK, "WSDXMLAddSibling failed with %08lx\n", hr);

    ok(child1->Node.Parent == parent, "child1->Node.Parent == %p\n", child1->Node.Parent);
    ok(child1->Node.Next == (WSDXML_NODE *)child2, "child1->Node.Next == %p\n", child1->Node.Next);
    ok(child2->Node.Parent == parent, "child2->Node.Parent == %p\n", child2->Node.Parent);
    ok(child2->Node.Next == NULL, "child2->Node.Next == %p\n", child2->Node.Next);
    ok(parent->FirstChild == (WSDXML_NODE *)child1, "parent->FirstChild == %p\n", parent->FirstChild);

    /* Try to add child3 as sibling of child1 */
    hr = WSDXMLAddSibling(child1, child3);
    ok(hr == S_OK, "WSDXMLAddSibling failed with %08lx\n", hr);

    ok(child1->Node.Parent == parent, "child1->Node.Parent == %p\n", child1->Node.Parent);
    ok(child1->Node.Next == (WSDXML_NODE *)child2, "child1->Node.Next == %p\n", child1->Node.Next);
    ok(child2->Node.Parent == parent, "child2->Node.Parent == %p\n", child2->Node.Parent);
    ok(child2->Node.Next == (WSDXML_NODE *)child3, "child2->Node.Next == %p\n", child2->Node.Next);
    ok(child3->Node.Parent == parent, "child2->Node.Parent == %p\n", child2->Node.Parent);
    ok(child3->Node.Next == NULL, "child2->Node.Next == %p\n", child2->Node.Next);
    ok(parent->FirstChild == (WSDXML_NODE *)child1, "parent->FirstChild == %p\n", parent->FirstChild);

    WSDFreeLinkedMemory(parent);
}

static void GetValueFromAny_tests(void)
{
    WSDXML_ELEMENT *parent, *child1, *child2, *child3;
    WSDXML_NAME parentName, child1Name, child2Name, child3Name;
    WSDXML_NAMESPACE ns, ns2;
    static const WCHAR *child1NameText = L"Tim";
    static const WCHAR *child2NameText = L"Bob";
    static const WCHAR *child3NameText = L"Joe";
    static const WCHAR *child1Value = L"V1";
    static const WCHAR *child2Value = L"V2";
    static const WCHAR *uri = L"http://test.test/";
    static const WCHAR *uri2 = L"http://test2.test/";
    LPCWSTR returnedValue = NULL, oldReturnedValue;
    HRESULT hr;

    /* Populate structures */
    ns.Uri = uri;
    ns.PreferredPrefix = L"t";

    ns2.Uri = uri2;
    ns2.PreferredPrefix = L"u";

    parentName.LocalName = (WCHAR *) L"Dad";
    parentName.Space = &ns;

    child1Name.LocalName = (WCHAR *) child1NameText;
    child1Name.Space = &ns2;

    child2Name.LocalName = (WCHAR *) child2NameText;
    child2Name.Space = &ns;

    child3Name.LocalName = (WCHAR *) child3NameText;
    child3Name.Space = &ns;

    /* Create some elements */
    hr = WSDXMLBuildAnyForSingleElement(&parentName, NULL, &parent);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08lx\n", hr);

    hr = WSDXMLBuildAnyForSingleElement(&child1Name, child1Value, &child1);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08lx\n", hr);

    hr = WSDXMLBuildAnyForSingleElement(&child2Name, child2Value, &child2);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08lx\n", hr);

    hr = WSDXMLBuildAnyForSingleElement(&child3Name, NULL, &child3);
    ok(hr == S_OK, "BuildAnyForSingleElement failed with %08lx\n", hr);

    /* Attach them to the parent element */
    hr = WSDXMLAddChild(parent, child1);
    ok(hr == S_OK, "AddChild failed with %08lx\n", hr);

    hr = WSDXMLAddChild(parent, child2);
    ok(hr == S_OK, "AddChild failed with %08lx\n", hr);

    hr = WSDXMLAddChild(parent, child3);
    ok(hr == S_OK, "AddChild failed with %08lx\n", hr);

    /* Test invalid arguments */
    hr = WSDXMLGetValueFromAny(NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetValueFromAny returned unexpected result: %08lx\n", hr);

    hr = WSDXMLGetValueFromAny(NULL, NULL, NULL, &returnedValue);
    ok(hr == E_INVALIDARG, "GetValueFromAny returned unexpected result: %08lx\n", hr);

    hr = WSDXMLGetValueFromAny(NULL, NULL, parent, NULL);
    ok(hr == E_POINTER, "GetValueFromAny returned unexpected result: %08lx\n", hr);

    hr = WSDXMLGetValueFromAny(uri, NULL, parent, NULL);
    ok(hr == E_POINTER, "GetValueFromAny returned unexpected result: %08lx\n", hr);

    hr = WSDXMLGetValueFromAny(uri, child2NameText, parent, NULL);
    ok(hr == E_POINTER, "GetValueFromAny returned unexpected result: %08lx\n", hr);

    /* Test calling the function with a text size that exceeds 8192 characters */
    hr = WSDXMLGetValueFromAny(largeText, child2NameText, parent, &returnedValue);
    ok(hr == E_INVALIDARG, "GetValueFromAny returned unexpected result: %08lx\n", hr);

    hr = WSDXMLGetValueFromAny(uri, largeText, parent, &returnedValue);
    ok(hr == E_INVALIDARG, "GetValueFromAny returned unexpected result: %08lx\n", hr);

    /* Test with valid parameters */
    hr = WSDXMLGetValueFromAny(uri, child2NameText, child1, &returnedValue);
    ok(hr == S_OK, "GetValueFromAny failed with %08lx\n", hr);
    ok(returnedValue != NULL, "returnedValue == NULL\n");
    ok(lstrcmpW(returnedValue, child2Value) == 0, "returnedValue ('%s') != '%s'\n", wine_dbgstr_w(returnedValue), wine_dbgstr_w(child2Value));

    hr = WSDXMLGetValueFromAny(uri2, child1NameText, child1, &returnedValue);
    ok(hr == S_OK, "GetValueFromAny failed with %08lx\n", hr);
    ok(returnedValue != NULL, "returnedValue == NULL\n");
    ok(lstrcmpW(returnedValue, child1Value) == 0, "returnedValue ('%s') != '%s'\n", wine_dbgstr_w(returnedValue), wine_dbgstr_w(child1Value));

    oldReturnedValue = returnedValue;

    hr = WSDXMLGetValueFromAny(uri2, child2NameText, child1, &returnedValue);
    ok(hr == E_FAIL, "GetValueFromAny returned unexpected value: %08lx\n", hr);
    ok(returnedValue == oldReturnedValue, "returnedValue == %p\n", returnedValue);

    oldReturnedValue = returnedValue;

    hr = WSDXMLGetValueFromAny(uri, child3NameText, child1, &returnedValue);
    ok(hr == E_FAIL, "GetValueFromAny failed with %08lx\n", hr);
    ok(returnedValue == oldReturnedValue, "returnedValue == %p\n", returnedValue);

    WSDFreeLinkedMemory(parent);
}

static void XMLContext_AddNamespace_tests(void)
{
    static const WCHAR *ns1Uri = L"http://test.test";
    static const WCHAR *ns2Uri = L"http://wine.rocks";
    static const WCHAR *ns3Uri = L"http://test.again";
    static const WCHAR *prefix1 = L"tst";
    static const WCHAR *prefix2 = L"wine";
    static const WCHAR *unPrefix1 = L"un1";

    IWSDXMLContext *context;
    WSDXML_NAMESPACE *ns1 = NULL, *ns2 = NULL;
    HRESULT hr;

    hr = WSDXMLCreateContext(&context);
    ok(hr == S_OK, "WSDXMLCreateContext failed with %08lx\n", hr);
    ok(context != NULL, "context == NULL\n");

    /* Test calling AddNamespace with invalid arguments */
    hr = IWSDXMLContext_AddNamespace(context, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "AddNamespace failed with %08lx\n", hr);

    hr = IWSDXMLContext_AddNamespace(context, ns1Uri, NULL, NULL);
    ok(hr == E_INVALIDARG, "AddNamespace failed with %08lx\n", hr);

    hr = IWSDXMLContext_AddNamespace(context, NULL, prefix1, NULL);
    ok(hr == E_INVALIDARG, "AddNamespace failed with %08lx\n", hr);

    /* Test calling AddNamespace without the ppNamespace parameter */
    hr = IWSDXMLContext_AddNamespace(context, ns1Uri, prefix1, NULL);
    ok(hr == S_OK, "AddNamespace failed with %08lx\n", hr);

    /* Now retrieve the created namespace */
    hr = IWSDXMLContext_AddNamespace(context, ns1Uri, prefix1, &ns1);
    ok(hr == S_OK, "AddNamespace failed with %08lx\n", hr);

    /* Check the returned structure */
    ok(ns1 != NULL, "ns1 == NULL\n");

    if (ns1 != NULL)
    {
        ok(lstrcmpW(ns1->Uri, ns1Uri) == 0, "URI returned by AddNamespace is not as expected (%s)\n", wine_dbgstr_w(ns1->Uri));
        ok(lstrcmpW(ns1->PreferredPrefix, prefix1) == 0, "PreferredPrefix returned by AddNamespace is not as expected (%s)\n", wine_dbgstr_w(ns1->PreferredPrefix));
        ok(ns1->Names == NULL, "Names array is not empty\n");
        ok(ns1->NamesCount == 0, "NamesCount is not 0 (value = %d)\n", ns1->NamesCount);
        ok(ns1->Uri != ns1Uri, "URI has not been cloned\n");
        ok(ns1->PreferredPrefix != prefix1, "URI has not been cloned\n");
    }

    /* Test calling AddNamespace with parameters that are too large */
    hr = IWSDXMLContext_AddNamespace(context, largeText, prefix2, &ns2);
    ok(hr == E_INVALIDARG, "AddNamespace failed with %08lx\n", hr);

    hr = IWSDXMLContext_AddNamespace(context, ns2Uri, largeText, &ns2);
    ok(hr == E_INVALIDARG, "AddNamespace failed with %08lx\n", hr);

    hr = IWSDXMLContext_AddNamespace(context, largeText, largeText, &ns2);
    ok(hr == E_INVALIDARG, "AddNamespace failed with %08lx\n", hr);

    /* Test calling AddNamespace with a conflicting prefix */
    hr = IWSDXMLContext_AddNamespace(context, ns2Uri, prefix1, &ns2);
    ok(hr == S_OK, "AddNamespace failed with %08lx\n", hr);

    /* Check the returned structure */
    ok(ns2 != NULL, "ns2 == NULL\n");

    if (ns2 != NULL)
    {
        ok(lstrcmpW(ns2->Uri, ns2Uri) == 0, "URI returned by AddNamespace is not as expected (%s)\n", wine_dbgstr_w(ns2->Uri));
        ok(lstrcmpW(ns2->PreferredPrefix, L"un0") == 0, "PreferredPrefix returned by AddNamespace is not as expected (%s)\n", wine_dbgstr_w(ns2->PreferredPrefix));
        ok(ns2->Names == NULL, "Names array is not empty\n");
        ok(ns2->NamesCount == 0, "NamesCount is not 0 (value = %d)\n", ns2->NamesCount);
        ok(ns2->Uri != ns2Uri, "URI has not been cloned\n");
        ok(ns2->PreferredPrefix != prefix2, "URI has not been cloned\n");
    }

    WSDFreeLinkedMemory(ns1);
    WSDFreeLinkedMemory(ns2);

    /* Try explicitly creating a prefix called 'un1' */
    hr = IWSDXMLContext_AddNamespace(context, L"http://one.more", unPrefix1, &ns2);
    ok(hr == S_OK, "AddNamespace failed with %08lx\n", hr);

    /* Check the returned structure */
    ok(ns2 != NULL, "ns2 == NULL\n");

    if (ns2 != NULL)
    {
        ok(lstrcmpW(ns2->PreferredPrefix, unPrefix1) == 0, "PreferredPrefix returned by AddNamespace is not as expected (%s)\n", wine_dbgstr_w(ns2->PreferredPrefix));
    }

    WSDFreeLinkedMemory(ns2);

    /* Test with one more conflicting prefix */
    hr = IWSDXMLContext_AddNamespace(context, ns3Uri, prefix1, &ns2);
    ok(hr == S_OK, "AddNamespace failed with %08lx\n", hr);

    /* Check the returned structure */
    ok(ns2 != NULL, "ns2 == NULL\n");

    if (ns2 != NULL)
    {
        ok(lstrcmpW(ns2->PreferredPrefix, L"un2") == 0, "PreferredPrefix returned by AddNamespace is not as expected (%s)\n", wine_dbgstr_w(ns2->PreferredPrefix));
    }

    WSDFreeLinkedMemory(ns2);

    /* Try renaming a prefix */
    hr = IWSDXMLContext_AddNamespace(context, ns3Uri, prefix2, &ns2);
    ok(hr == S_OK, "AddNamespace failed with %08lx\n", hr);

    /* Check the returned structure */
    ok(ns2 != NULL, "ns2 == NULL\n");

    if (ns2 != NULL)
    {
        ok(lstrcmpW(ns2->Uri, ns3Uri) == 0, "Uri returned by AddNamespace is not as expected (%s)\n", wine_dbgstr_w(ns2->Uri));
        ok(lstrcmpW(ns2->PreferredPrefix, prefix2) == 0, "PreferredPrefix returned by AddNamespace is not as expected (%s)\n", wine_dbgstr_w(ns2->PreferredPrefix));
    }

    WSDFreeLinkedMemory(ns2);

    IWSDXMLContext_Release(context);
}

static void XMLContext_AddNameToNamespace_tests(void)
{
    static const WCHAR *ns1Uri = L"http://test.test";
    static const WCHAR *ns2Uri = L"http://wine.rocks";
    static const WCHAR *prefix2 = L"wine";
    static const WCHAR *name1Text = L"Bob";
    static const WCHAR *name2Text = L"Tim";
    IWSDXMLContext *context;
    WSDXML_NAMESPACE *ns2 = NULL;
    WSDXML_NAME *name1 = NULL, *name2 = NULL;
    HRESULT hr;

    hr = WSDXMLCreateContext(&context);
    ok(hr == S_OK, "WSDXMLCreateContext failed with %08lx\n", hr);

    /* Test calling AddNameToNamespace with invalid arguments */
    hr = IWSDXMLContext_AddNameToNamespace(context, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "AddNameToNamespace failed with %08lx\n", hr);

    hr = IWSDXMLContext_AddNameToNamespace(context, ns1Uri, NULL, NULL);
    ok(hr == E_INVALIDARG, "AddNameToNamespace failed with %08lx\n", hr);

    hr = IWSDXMLContext_AddNameToNamespace(context, NULL, name1Text, NULL);
    ok(hr == E_INVALIDARG, "AddNameToNamespace failed with %08lx\n", hr);

    /* Test calling AddNameToNamespace without the ppName parameter */
    hr = IWSDXMLContext_AddNameToNamespace(context, ns1Uri, name1Text, NULL);
    ok(hr == S_OK, "AddNameToNamespace failed with %08lx\n", hr);

    /* Now retrieve the created name */
    hr = IWSDXMLContext_AddNameToNamespace(context, ns1Uri, name1Text, &name1);
    ok(hr == S_OK, "AddNameToNamespace failed with %08lx\n", hr);

    /* Check the returned structure */
    ok(name1 != NULL, "name1 == NULL\n");

    if (name1 != NULL)
    {
        ok(lstrcmpW(name1->LocalName, name1Text) == 0, "LocalName returned by AddNameToNamespace is not as expected (%s)\n", wine_dbgstr_w(name1->LocalName));
        ok(name1->LocalName != name1Text, "LocalName has not been cloned\n");

        ok(name1->Space != NULL, "Space returned by AddNameToNamespace is null\n");
        ok(lstrcmpW(name1->Space->Uri, ns1Uri) == 0, "URI returned by AddNameToNamespace is not as expected (%s)\n", wine_dbgstr_w(name1->Space->Uri));
        ok(lstrcmpW(name1->Space->PreferredPrefix, L"un0") == 0, "PreferredPrefix returned by AddName is not as expected (%s)\n", wine_dbgstr_w(name1->Space->PreferredPrefix));
        ok(name1->Space->Names == NULL, "Names array is not empty\n");
        ok(name1->Space->NamesCount == 0, "NamesCount is not 0 (value = %d)\n", name1->Space->NamesCount);
        ok(name1->Space->Uri != ns1Uri, "URI has not been cloned\n");
    }

    /* Test calling AddNamespace with parameters that are too large */
    hr = IWSDXMLContext_AddNameToNamespace(context, largeText, name1Text, &name2);
    ok(hr == E_INVALIDARG, "AddNameToNamespace failed with %08lx\n", hr);

    hr = IWSDXMLContext_AddNameToNamespace(context, ns1Uri, largeText, &name2);
    ok(hr == E_INVALIDARG, "AddNameToNamespace failed with %08lx\n", hr);

    /* Try creating a namespace explicitly */
    hr = IWSDXMLContext_AddNamespace(context, ns2Uri, prefix2, &ns2);
    ok(hr == S_OK, "AddNamespace failed with %08lx\n", hr);

    /* Now add a name to it */
    hr = IWSDXMLContext_AddNameToNamespace(context, ns2Uri, name2Text, &name2);
    ok(hr == S_OK, "AddNameToNamespace failed with %08lx\n", hr);

    /* Check the returned structure */
    ok(name2 != NULL, "name2 == NULL\n");

    if (name2 != NULL)
    {
        ok(lstrcmpW(name2->LocalName, name2Text) == 0, "LocalName returned by AddNameToNamespace is not as expected (%s)\n", wine_dbgstr_w(name2->LocalName));
        ok(name2->LocalName != name2Text, "LocalName has not been cloned\n");

        ok(name2->Space != NULL, "Space returned by AddNameToNamespace is null\n");
        ok(name2->Space != ns2, "Space returned by AddNameToNamespace is equal to the namespace returned by AddNamespace\n");
        ok(lstrcmpW(name2->Space->Uri, ns2Uri) == 0, "URI returned by AddNameToNamespace is not as expected (%s)\n", wine_dbgstr_w(name2->Space->Uri));
        ok(lstrcmpW(name2->Space->PreferredPrefix, prefix2) == 0, "PreferredPrefix returned by AddNameToNamespace is not as expected (%s)\n", wine_dbgstr_w(name2->Space->PreferredPrefix));
        ok(name2->Space->Names == NULL, "Names array is not empty\n");
        ok(name2->Space->NamesCount == 0, "NamesCount is not 0 (value = %d)\n", name2->Space->NamesCount);
        ok(name2->Space->Uri != ns2Uri, "URI has not been cloned\n");
    }

    WSDFreeLinkedMemory(name1);
    WSDFreeLinkedMemory(name2);
    WSDFreeLinkedMemory(ns2);

    /* Now re-retrieve ns2 */
    hr = IWSDXMLContext_AddNamespace(context, ns2Uri, prefix2, &ns2);
    ok(hr == S_OK, "AddNamespace failed with %08lx\n", hr);

    /* Check the returned structure */
    ok(ns2 != NULL, "ns2 == NULL\n");

    if (ns2 != NULL)
    {
        ok(lstrcmpW(ns2->Uri, ns2Uri) == 0, "URI returned by AddNamespace is not as expected (%s)\n", wine_dbgstr_w(ns2->Uri));

        /* Apparently wsdapi always leaves the namespace names array as empty */
        ok(ns2->Names == NULL, "Names array is not empty\n");
        ok(ns2->NamesCount == 0, "NamesCount is not 0 (value = %d)\n", ns2->NamesCount);
        WSDFreeLinkedMemory(ns2);
    }

    IWSDXMLContext_Release(context);
}

START_TEST(xml)
{
    /* Allocate a large text buffer for use in tests */
    largeText = calloc(1, largeTextSize + sizeof(WCHAR));
    memset(largeText, 'a', largeTextSize);

    BuildAnyForSingleElement_tests();
    AddChild_tests();
    AddSibling_tests();
    GetValueFromAny_tests();

    XMLContext_AddNamespace_tests();
    XMLContext_AddNameToNamespace_tests();

    free(largeText);
}
