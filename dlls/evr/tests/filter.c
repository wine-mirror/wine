/*
 * Copyright 2017 Fabian Maurer
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

#include <stdio.h>

#include "dshow.h"
#include "evr.h"
#include "d3d9.h"
#include "initguid.h"
#include "dxva2api.h"

#include "wine/test.h"

#define QI_SUCCEED(iface, riid, ppv) hr = IUnknown_QueryInterface(iface, &riid, (LPVOID*)&ppv); \
    ok(hr == S_OK, "%s: run %d: IUnknown_QueryInterface returned %x\n", testid, testrun, hr); \
    ok(ppv != NULL, "%s: run %d: Pointer is NULL\n", testid, testrun);

#define QI_FAIL(iface, riid, ppv) hr = IUnknown_QueryInterface(iface, &riid, (LPVOID*)&ppv); \
    ok(hr == E_NOINTERFACE, "%s: run %d: IUnknown_QueryInterface returned %x\n", testid, testrun, hr); \
    ok(ppv == NULL, "%s: run %d: Pointer is %p\n", testid, testrun, ppv);

#define ADDREF_EXPECT(iface, num) if (iface) { \
    refCount = IUnknown_AddRef(iface); \
    ok(refCount == num, "%s: run %d: IUnknown_AddRef should return %d, got %d\n", testid, testrun, num, refCount); \
}

#define RELEASE_EXPECT(iface, num) if (iface) { \
    refCount = IUnknown_Release(iface); \
    ok(refCount == num, "%s: run %d: IUnknown_Release should return %d, got %d\n", testid, testrun, num, refCount); \
}

static void test_aggregation(const CLSID clsid_inner, const IID iid_inner, const char *testid, int testrun)
{
    const CLSID clsid_outer = CLSID_SystemClock;
    const IID iid_outer = IID_IReferenceClock;
    HRESULT hr;
    ULONG refCount;
    IUnknown *unk_outer = NULL;
    IUnknown *unk_inner = NULL;
    IUnknown *unk_inner_fail = NULL;
    IUnknown *unk_outer_test = NULL;
    IUnknown *unk_inner_test = NULL;
    IUnknown *unk_aggregatee = NULL;
    IUnknown *unk_aggregator = NULL;
    IUnknown *unk_test = NULL;

    hr = CoCreateInstance(&clsid_outer, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (LPVOID*)&unk_outer);
    ok(hr == S_OK, "%s: run %d: First CoCreateInstance failed with %x\n", testid, testrun, hr);
    ok(unk_outer != NULL, "%s: run %d: unk_outer is NULL\n", testid, testrun);

    if (!unk_outer)
    {
        skip("%s: run %d: unk_outer is NULL\n", testid, testrun);
        return;
    }

    /* for aggregation, we should only be able to request IUnknown */
    hr = CoCreateInstance(&clsid_inner, unk_outer, CLSCTX_INPROC_SERVER, &iid_inner, (LPVOID*)&unk_inner_fail);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        skip("%s: run %d: Class not registered\n", testid, testrun);
        return;
    }
    ok(hr == E_NOINTERFACE, "%s: run %d: Second CoCreateInstance returned %x\n", testid, testrun, hr);
    ok(unk_inner_fail == NULL, "%s: run %d: unk_inner_fail is not NULL\n", testid, testrun);

    /* aggregation, request IUnknown */
    hr = CoCreateInstance(&clsid_inner, unk_outer, CLSCTX_INPROC_SERVER, &IID_IUnknown, (LPVOID*)&unk_inner);
    todo_wine ok(hr == S_OK, "%s: run %d: Third CoCreateInstance returned %x\n", testid, testrun, hr);
    todo_wine ok(unk_inner != NULL, "%s: run %d: unk_inner is NULL\n", testid, testrun);

    if (!unk_inner)
    {
        skip("%s: run %d: unk_inner is NULL\n", testid, testrun);
        return;
    }

    ADDREF_EXPECT(unk_outer, 2);
    ADDREF_EXPECT(unk_inner, 2);
    RELEASE_EXPECT(unk_outer, 1);
    RELEASE_EXPECT(unk_inner, 1);

    QI_FAIL(unk_outer, iid_inner, unk_aggregatee);
    QI_FAIL(unk_inner, iid_outer, unk_aggregator);

    QI_SUCCEED(unk_outer, iid_outer, unk_aggregator);
    QI_SUCCEED(unk_outer, IID_IUnknown, unk_outer_test);
    QI_SUCCEED(unk_inner, iid_inner, unk_aggregatee);
    QI_SUCCEED(unk_inner, IID_IUnknown, unk_inner_test);

    if (!unk_aggregator || !unk_outer_test || !unk_aggregatee || !unk_inner_test)
    {
        skip("%s: run %d: One of the required interfaces is NULL\n", testid, testrun);
        return;
    }

    ADDREF_EXPECT(unk_aggregator, 5);
    ADDREF_EXPECT(unk_outer_test, 6);
    ADDREF_EXPECT(unk_aggregatee, 7);
    ADDREF_EXPECT(unk_inner_test, 3);
    RELEASE_EXPECT(unk_aggregator, 6);
    RELEASE_EXPECT(unk_outer_test, 5);
    RELEASE_EXPECT(unk_aggregatee, 4);
    RELEASE_EXPECT(unk_inner_test, 2);

    QI_SUCCEED(unk_aggregator, IID_IUnknown, unk_test);
    QI_SUCCEED(unk_outer_test, IID_IUnknown, unk_test);
    QI_SUCCEED(unk_aggregatee, IID_IUnknown, unk_test);
    QI_SUCCEED(unk_inner_test, IID_IUnknown, unk_test);

    QI_FAIL(unk_aggregator, iid_inner, unk_test);
    QI_FAIL(unk_outer_test, iid_inner, unk_test);
    QI_FAIL(unk_aggregatee, iid_inner, unk_test);
    QI_SUCCEED(unk_inner_test, iid_inner, unk_test);

    QI_SUCCEED(unk_aggregator, iid_outer, unk_test);
    QI_SUCCEED(unk_outer_test, iid_outer, unk_test);
    QI_SUCCEED(unk_aggregatee, iid_outer, unk_test);
    QI_FAIL(unk_inner_test, iid_outer, unk_test);

    RELEASE_EXPECT(unk_aggregator, 10);
    RELEASE_EXPECT(unk_outer_test, 9);
    RELEASE_EXPECT(unk_aggregatee, 8);
    RELEASE_EXPECT(unk_inner_test, 2);
    RELEASE_EXPECT(unk_outer, 7);
    RELEASE_EXPECT(unk_inner, 1);

    do
    {
        refCount = IUnknown_Release(unk_inner);
    } while (refCount);

    do
    {
        refCount = IUnknown_Release(unk_outer);
    } while (refCount);
}

static void test_evr_filter_aggregations(void)
{
    const IID * iids[] = {
        &IID_IAMCertifiedOutputProtection, &IID_IAMFilterMiscFlags, &IID_IBaseFilter,
        &IID_IKsPropertySet, &IID_IMediaEventSink, &IID_IMediaSeeking, &IID_IQualityControl,
        &IID_IQualProp, &IID_IEVRFilterConfig, &IID_IMFGetService, &IID_IMFVideoPositionMapper,
        &IID_IMFVideoRenderer, &IID_IQualityControl
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(iids); i++)
    {
        test_aggregation(CLSID_EnhancedVideoRenderer, *iids[i], "filter", i);
    }
}

START_TEST(filter)
{
    CoInitialize(NULL);

    test_evr_filter_aggregations();

    CoUninitialize();
}
