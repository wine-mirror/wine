/*
 * Copyright 2018 Józef Kucia
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

#include "interactioncontext.h"
#include "wine/test.h"

static void test_context(void)
{
    HINTERACTIONCONTEXT context;
    HRESULT hr;

    hr = CreateInteractionContext(&context);
    ok(hr == S_OK, "Failed to create context, hr %#x.\n", hr);
    hr = DestroyInteractionContext(context);
    ok(hr == S_OK, "Failed to destroy context, hr %#x.\n", hr);

    hr = CreateInteractionContext(NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    hr = DestroyInteractionContext(NULL);
    ok(hr == E_HANDLE, "Got hr %#x.\n", hr);
}

static void test_properties(void)
{
    HINTERACTIONCONTEXT context;
    UINT32 value;
    HRESULT hr;

    hr = CreateInteractionContext(&context);
    ok(hr == S_OK, "Failed to create context, hr %#x.\n", hr);

    hr = GetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, &value);
    ok(hr == S_OK, "Failed to get property, hr %#x.\n", hr);
    ok(value == TRUE, "Got unexpected value %#x.\n", value);

    hr = SetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, TRUE);
    ok(hr == S_OK, "Failed to set property, hr %#x.\n", hr);
    hr = GetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, &value);
    ok(hr == S_OK, "Failed to get property, hr %#x.\n", hr);
    ok(value == TRUE, "Got unexpected value %#x.\n", value);

    hr = SetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, FALSE);
    ok(hr == S_OK, "Failed to set property, hr %#x.\n", hr);
    hr = GetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, &value);
    ok(hr == S_OK, "Failed to get property, hr %#x.\n", hr);
    ok(value == FALSE, "Got unexpected value %#x.\n", value);

    hr = SetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, 2);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);
    hr = SetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, 3);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = SetPropertyInteractionContext(context, 0xdeadbeef, 0);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);
    hr = GetPropertyInteractionContext(context, 0xdeadbeef, &value);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = GetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = SetPropertyInteractionContext(NULL, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, FALSE);
    ok(hr == E_HANDLE, "Got hr %#x.\n", hr);
    hr = GetPropertyInteractionContext(NULL, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, &value);
    ok(hr == E_HANDLE, "Got hr %#x.\n", hr);

    hr = DestroyInteractionContext(context);
    ok(hr == S_OK, "Failed to destroy context, hr %#x.\n", hr);
}

START_TEST(ninput)
{
    test_context();
    test_properties();
}
