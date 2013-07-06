/*
 *    QCAP tests
 *
 * Copyright 2013 Damjan Jovanovic
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

#include "windef.h"
#include "winbase.h"
#define COBJMACROS
#include <dshow.h>
#include <guiddef.h>
#include <devguid.h>
#include <stdio.h>

#include "wine/strmbase.h"
#include "wine/test.h"

static void test_smart_tee_filter(void)
{
    HRESULT hr;
    IBaseFilter *smartTeeFilter = NULL;
    IEnumPins *enumPins = NULL;
    IPin *pin;
    FILTER_INFO filterInfo;
    int pinNumber = 0;

    hr = CoCreateInstance(&CLSID_SmartTee, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void**)&smartTeeFilter);
    todo_wine ok(SUCCEEDED(hr), "couldn't create smart tee filter, hr=%08x\n", hr);
    if (FAILED(hr))
        goto end;

    hr = IBaseFilter_QueryFilterInfo(smartTeeFilter, &filterInfo);
    ok(SUCCEEDED(hr), "QueryFilterInfo failed, hr=%08x\n", hr);
    if (FAILED(hr))
        goto end;

    ok(lstrlenW(filterInfo.achName) == 0,
            "filter's name is meant to be empty but it's %s\n", wine_dbgstr_w(filterInfo.achName));

    hr = IBaseFilter_EnumPins(smartTeeFilter, &enumPins);
    ok(SUCCEEDED(hr), "cannot enum filter pins, hr=%08x\n", hr);
    if (FAILED(hr))
        goto end;

    while (IEnumPins_Next(enumPins, 1, &pin, NULL) == S_OK)
    {
        PIN_INFO pinInfo;
        hr = IPin_QueryPinInfo(pin, &pinInfo);
        ok(SUCCEEDED(hr), "QueryPinInfo failed, hr=%08x\n", hr);
        if (FAILED(hr))
            goto endwhile;

        if (pinNumber == 0)
        {
            static const WCHAR wszInput[] = {'I','n','p','u','t',0};
            ok(pinInfo.dir == PINDIR_INPUT, "pin 0 isn't an input pin\n");
            ok(!lstrcmpW(pinInfo.achName, wszInput), "pin 0 is called %s, not 'Input'\n", wine_dbgstr_w(pinInfo.achName));
        }
        else if (pinNumber == 1)
        {
            static const WCHAR wszCapture[] = {'C','a','p','t','u','r','e',0};
            ok(pinInfo.dir == PINDIR_OUTPUT, "pin 1 isn't an output pin\n");
            ok(!lstrcmpW(pinInfo.achName, wszCapture), "pin 1 is called %s, not 'Capture'\n", wine_dbgstr_w(pinInfo.achName));
        }
        else if (pinNumber == 2)
        {
            static const WCHAR wszPreview[] = {'P','r','e','v','i','e','w',0};
            ok(pinInfo.dir == PINDIR_OUTPUT, "pin 2 isn't an output pin\n");
            ok(!lstrcmpW(pinInfo.achName, wszPreview), "pin 2 is called %s, not 'Preview'\n", wine_dbgstr_w(pinInfo.achName));
        }
        else
            ok(0, "pin %d isn't supposed to exist\n", pinNumber);

    endwhile:
        IPin_Release(pin);
        pinNumber++;
    }

end:
    if (smartTeeFilter)
        IBaseFilter_Release(smartTeeFilter);
    if (enumPins)
        IEnumPins_Release(enumPins);
}

START_TEST(qcap)
{
    if (SUCCEEDED(CoInitialize(NULL)))
    {
        test_smart_tee_filter();
        CoUninitialize();
    }
    else
        skip("CoInitialize failed\n");
}
