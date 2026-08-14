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
    ok(hr == S_OK, "Failed to create context, hr %#lx.\n", hr);
    hr = DestroyInteractionContext(context);
    ok(hr == S_OK, "Failed to destroy context, hr %#lx.\n", hr);

    hr = CreateInteractionContext(NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = DestroyInteractionContext(NULL);
    ok(hr == E_HANDLE, "Got hr %#lx.\n", hr);
}

static void test_properties(void)
{
    HINTERACTIONCONTEXT context;
    UINT32 value;
    HRESULT hr;

    hr = CreateInteractionContext(&context);
    ok(hr == S_OK, "Failed to create context, hr %#lx.\n", hr);

    /* Test INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS */
    hr = GetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, &value);
    ok(hr == S_OK, "Failed to get property, hr %#lx.\n", hr);
    ok(value == TRUE, "Got unexpected value %#x.\n", value);

    hr = SetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, TRUE);
    ok(hr == S_OK, "Failed to set property, hr %#lx.\n", hr);
    hr = GetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, &value);
    ok(hr == S_OK, "Failed to get property, hr %#lx.\n", hr);
    ok(value == TRUE, "Got unexpected value %#x.\n", value);

    hr = SetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, FALSE);
    ok(hr == S_OK, "Failed to set property, hr %#lx.\n", hr);
    hr = GetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, &value);
    ok(hr == S_OK, "Failed to get property, hr %#lx.\n", hr);
    ok(value == FALSE, "Got unexpected value %#x.\n", value);

    hr = SetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, 2);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = SetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, 3);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = SetPropertyInteractionContext(context, 0xdeadbeef, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = GetPropertyInteractionContext(context, 0xdeadbeef, &value);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = GetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = SetPropertyInteractionContext(NULL, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, FALSE);
    ok(hr == E_HANDLE, "Got hr %#lx.\n", hr);
    hr = GetPropertyInteractionContext(NULL, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, &value);
    ok(hr == E_HANDLE, "Got hr %#lx.\n", hr);

    /* Test INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK */
    hr = GetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK, &value);
    ok(hr == S_OK, "Failed to get property, hr %#lx.\n", hr);
    ok(value == TRUE, "Got unexpected value %#x.\n", value);

    hr = SetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK, FALSE);
    ok(hr == S_OK, "Failed to set property, hr %#lx.\n", hr);
    hr = GetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK, &value);
    ok(hr == S_OK, "Failed to get property, hr %#lx.\n", hr);
    ok(value == FALSE, "Got unexpected value %#x.\n", value);

    hr = SetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK, TRUE);
    ok(hr == S_OK, "Failed to set property, hr %#lx.\n", hr);
    hr = GetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK, &value);
    ok(hr == S_OK, "Failed to get property, hr %#lx.\n", hr);
    ok(value == TRUE, "Got unexpected value %#x.\n", value);

    hr = SetPropertyInteractionContext(context, INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK, 2);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = DestroyInteractionContext(context);
    ok(hr == S_OK, "Failed to destroy context, hr %#lx.\n", hr);
}

static void test_configuration(void)
{
    HINTERACTIONCONTEXT context;
    HRESULT hr;

    static const INTERACTION_CONTEXT_CONFIGURATION test_config[] =
    {
        {
            INTERACTION_ID_MANIPULATION,
            INTERACTION_CONFIGURATION_FLAG_MANIPULATION |
            INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_X |
            INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_Y |
            INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING |
            INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_INERTIA |
            INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING_INERTIA
        },
        {
            INTERACTION_ID_TAP,
            INTERACTION_CONFIGURATION_FLAG_TAP
        },
    };
    INTERACTION_CONTEXT_CONFIGURATION input_config, output_config[ARRAY_SIZE(test_config)] = {0};

    hr = CreateInteractionContext(&context);
    ok(hr == S_OK, "Failed to create context, hr %#lx.\n", hr);

    /* Check GetInteractionConfigurationInteractionContext() parameters */
    hr = GetInteractionConfigurationInteractionContext(NULL, 1, output_config);
    ok(hr == E_HANDLE, "Got hr %#lx.\n", hr);

    hr = GetInteractionConfigurationInteractionContext(context, 0, output_config);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = GetInteractionConfigurationInteractionContext(context, 1, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = GetInteractionConfigurationInteractionContext(context, 1, output_config);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    output_config->interactionId = INTERACTION_ID_NONE;
    hr = GetInteractionConfigurationInteractionContext(context, 1, output_config);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    output_config->interactionId = INTERACTION_ID_CROSS_SLIDE + 1;
    hr = GetInteractionConfigurationInteractionContext(context, 1, output_config);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    /* Check GetInteractionConfigurationInteractionContext() default config */
    for (INTERACTION_ID id = INTERACTION_ID_MANIPULATION; id <= INTERACTION_ID_CROSS_SLIDE; id++)
    {
        winetest_push_context("id %d", id);

        output_config[0].interactionId = id;
        hr = GetInteractionConfigurationInteractionContext(context, 1, output_config);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(output_config[0].enable == INTERACTION_CONFIGURATION_FLAG_NONE,
                "Got unexpected flags %#x.\n", output_config[0].enable);

        winetest_pop_context();
    }

    /* Check SetInteractionConfigurationInteractionContext() parameters */
    hr = SetInteractionConfigurationInteractionContext(NULL, 0, NULL);
    ok(hr == E_HANDLE, "Got hr %#lx.\n", hr);
    hr = SetInteractionConfigurationInteractionContext(context, 0, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = SetInteractionConfigurationInteractionContext(context, 1, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    input_config.interactionId = INTERACTION_ID_NONE;
    input_config.enable = INTERACTION_CONFIGURATION_FLAG_NONE;
    hr = SetInteractionConfigurationInteractionContext(context, 1, &input_config);
    ok(hr == E_INVALIDARG, "Failed to set configuration, hr %#lx.\n", hr);

    input_config.interactionId = INTERACTION_ID_CROSS_SLIDE + 1;
    input_config.enable = INTERACTION_CONFIGURATION_FLAG_NONE;
    hr = SetInteractionConfigurationInteractionContext(context, 1, &input_config);
    ok(hr == E_INVALIDARG, "Failed to set configuration, hr %#lx.\n", hr);

    input_config.interactionId = INTERACTION_ID_MANIPULATION;
    input_config.enable = INTERACTION_CONFIGURATION_FLAG_MAX;
    hr = SetInteractionConfigurationInteractionContext(context, 1, &input_config);
    ok(hr == S_OK, "Failed to set configuration, hr %#lx.\n", hr);

    output_config[0].interactionId = INTERACTION_ID_MANIPULATION;
    hr = GetInteractionConfigurationInteractionContext(context, 1, output_config);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(output_config[0].enable == INTERACTION_CONFIGURATION_FLAG_MAX,
            "Got unexpected flags %#x.\n", output_config[0].enable);

    /* Check normal SetInteractionConfigurationInteractionContext() calls */
    hr = SetInteractionConfigurationInteractionContext(context, ARRAY_SIZE(test_config), test_config);
    ok(hr == S_OK, "Failed to set configuration, hr %#lx.\n", hr);

    /* Check normal GetInteractionConfigurationInteractionContext() calls */
    output_config[0].interactionId = INTERACTION_ID_MANIPULATION;
    hr = GetInteractionConfigurationInteractionContext(context, 1, output_config);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(output_config[0].enable == test_config[0].enable,
            "Got unexpected flags %#x.\n", output_config[0].enable);

    output_config[0].interactionId = INTERACTION_ID_TAP;
    output_config[1].interactionId = INTERACTION_ID_MANIPULATION;
    hr = GetInteractionConfigurationInteractionContext(context, 2, output_config);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(output_config[0].enable == test_config[1].enable,
            "Got unexpected flags %#x.\n", output_config[0].enable);
    ok(output_config[1].enable == test_config[0].enable,
            "Got unexpected flags %#x.\n", output_config[1].enable);

    output_config[0].interactionId = INTERACTION_ID_DRAG;
    hr = GetInteractionConfigurationInteractionContext(context, 1, output_config);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(output_config[0].enable == INTERACTION_CONFIGURATION_FLAG_NONE,
            "Got unexpected flags %#x.\n", output_config[0].enable);

    hr = DestroyInteractionContext(context);
    ok(hr == S_OK, "Failed to destroy context, hr %#lx.\n", hr);
}

static void test_BufferPointerPacketsInteractionContext(void)
{
    POINTER_INFO pointer_info = {0};
    HINTERACTIONCONTEXT context;
    HRESULT hr;

    hr = CreateInteractionContext(&context);
    ok(hr == S_OK, "Failed to create context, hr %#lx.\n", hr);

    hr = BufferPointerPacketsInteractionContext(NULL, 1, &pointer_info);
    ok(hr == E_HANDLE, "Got unexpected hr %#lx.\n", hr);

    hr = BufferPointerPacketsInteractionContext(context, 0, &pointer_info);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = BufferPointerPacketsInteractionContext(context, 1, NULL);
    ok(hr == E_POINTER, "Got unexpected hr %#lx.\n", hr);

    hr = BufferPointerPacketsInteractionContext(context, 1, &pointer_info);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = DestroyInteractionContext(context);
    ok(hr == S_OK, "Failed to destroy context, hr %#lx.\n", hr);
}

static void test_GetStateInteractionContext(void)
{
    POINTER_INFO pointer_info = {0};
    HINTERACTIONCONTEXT context;
    INTERACTION_STATE state;
    HRESULT hr;

    hr = CreateInteractionContext(&context);
    ok(hr == S_OK, "Failed to create context, hr %#lx.\n", hr);

    hr = GetStateInteractionContext(NULL, &pointer_info, &state);
    ok(hr == E_HANDLE, "Got unexpected hr %#lx.\n", hr);

    state = INTERACTION_STATE_MAX;
    hr = GetStateInteractionContext(context, 0, &state);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(state == INTERACTION_STATE_IDLE, "Got unexpected state %d.\n", state);

    hr = GetStateInteractionContext(context, &pointer_info, NULL);
    ok(hr == E_POINTER, "Got unexpected hr %#lx.\n", hr);

    state = INTERACTION_STATE_MAX;
    hr = GetStateInteractionContext(context, &pointer_info, &state);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(state == INTERACTION_STATE_IDLE, "Got unexpected state %d.\n", state);

    hr = DestroyInteractionContext(context);
    ok(hr == S_OK, "Failed to destroy context, hr %#lx.\n", hr);
}

static void test_ProcessBufferedPacketsInteractionContext(void)
{
    HINTERACTIONCONTEXT context;
    HRESULT hr;

    hr = CreateInteractionContext(&context);
    ok(hr == S_OK, "Failed to create context, hr %#lx.\n", hr);

    hr = ProcessBufferedPacketsInteractionContext(NULL);
    ok(hr == E_HANDLE, "Got unexpected hr %#lx.\n", hr);

    hr = ProcessBufferedPacketsInteractionContext(context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = DestroyInteractionContext(context);
    ok(hr == S_OK, "Failed to destroy context, hr %#lx.\n", hr);
}

static void test_ordinal_2502(void)
{
    static HRESULT (WINAPI *pOrdinal2502)(void *, void *, void *);
    HINTERACTIONCONTEXT context;
    HMODULE module;
    HRESULT hr;

    module = GetModuleHandleA("ninput");
    pOrdinal2502 = (void *)GetProcAddress(module, (LPCSTR)2502);
    ok(!!pOrdinal2502, "Failed to retrieve the function at ordinal 2502.\n");

    hr = pOrdinal2502(NULL, NULL, NULL);
    ok(hr == E_HANDLE, "Got unexpected hr %#lx.\n", hr);

    hr = CreateInteractionContext(&context);
    ok(hr == S_OK, "Failed to create context, hr %#lx.\n", hr);

    hr = pOrdinal2502(context, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = DestroyInteractionContext(context);
    ok(hr == S_OK, "Failed to destroy context, hr %#lx.\n", hr);
}

START_TEST(ninput)
{
    test_context();
    test_properties();
    test_configuration();
    test_BufferPointerPacketsInteractionContext();
    test_GetStateInteractionContext();
    test_ProcessBufferedPacketsInteractionContext();
    test_ordinal_2502();
}
