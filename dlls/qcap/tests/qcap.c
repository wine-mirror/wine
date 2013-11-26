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

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(ReceiveConnection);
DEFINE_EXPECT(GetAllocatorRequirements);
DEFINE_EXPECT(NotifyAllocator);
DEFINE_EXPECT(Reconnect);

static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    sprintf(buf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
}

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

typedef enum {
    SOURCE_FILTER,
    SINK_FILTER,
    INTERMEDIATE_FILTER,
    NOT_FILTER
} filter_type;

static const char* debugstr_filter_type(filter_type type)
{
    switch(type) {
    case SOURCE_FILTER:
        return "SOURCE_FILTER";
    case SINK_FILTER:
        return "SINK_FILTER";
    case INTERMEDIATE_FILTER:
        return "INTERMEDIATE_FILTER";
    default:
        return "NOT_FILTER";
    }
}

typedef enum {
    BASEFILTER_ENUMPINS,
    ENUMPINS_NEXT,
    PIN_QUERYDIRECTION,
    PIN_CONNECTEDTO,
    PIN_QUERYPININFO,
    KSPROPERTYSET_GET,
    PIN_ENUMMEDIATYPES,
    ENUMMEDIATYPES_RESET,
    ENUMMEDIATYPES_NEXT,
    GRAPHBUILDER_CONNECT,
    BASEFILTER_GETSTATE,
    BASEFILTER_QUERYINTERFACE,
    END
} call_id;

static const struct {
    call_id call_id;
    filter_type filter_type;
    BOOL wine_missing;
    BOOL wine_extra;
    BOOL optional; /* fails on wine if missing */
    BOOL broken;
} renderstream_cat_media[] = {
    {BASEFILTER_QUERYINTERFACE, SOURCE_FILTER},
    {BASEFILTER_ENUMPINS, SOURCE_FILTER},
    {ENUMPINS_NEXT, SOURCE_FILTER},
    {PIN_QUERYDIRECTION, SOURCE_FILTER},
    {PIN_CONNECTEDTO, SOURCE_FILTER},
    {PIN_QUERYPININFO, SOURCE_FILTER, TRUE},
    {KSPROPERTYSET_GET, SOURCE_FILTER},
    {PIN_ENUMMEDIATYPES, SOURCE_FILTER},
    {ENUMMEDIATYPES_RESET, SOURCE_FILTER},
    {ENUMMEDIATYPES_NEXT, SOURCE_FILTER},
    {BASEFILTER_QUERYINTERFACE, SOURCE_FILTER},
    {BASEFILTER_ENUMPINS, SOURCE_FILTER},
    {ENUMPINS_NEXT, SOURCE_FILTER},
    {PIN_QUERYDIRECTION, SOURCE_FILTER},
    {PIN_CONNECTEDTO, SOURCE_FILTER},
    {PIN_QUERYPININFO, SOURCE_FILTER, TRUE},
    {KSPROPERTYSET_GET, SOURCE_FILTER},
    {ENUMPINS_NEXT, SOURCE_FILTER},
    {BASEFILTER_QUERYINTERFACE, SOURCE_FILTER, TRUE},
    {BASEFILTER_ENUMPINS, SOURCE_FILTER, TRUE},
    {ENUMPINS_NEXT, SOURCE_FILTER, TRUE},
    {PIN_QUERYDIRECTION, SOURCE_FILTER, TRUE},
    {PIN_CONNECTEDTO, SOURCE_FILTER, TRUE},
    {PIN_QUERYPININFO, SOURCE_FILTER, TRUE},
    {KSPROPERTYSET_GET, SOURCE_FILTER, TRUE},
    {ENUMPINS_NEXT, SOURCE_FILTER, TRUE},
    {BASEFILTER_QUERYINTERFACE, SOURCE_FILTER, TRUE},
    {BASEFILTER_ENUMPINS, SOURCE_FILTER, TRUE},
    {ENUMPINS_NEXT, SOURCE_FILTER, TRUE},
    {PIN_QUERYDIRECTION, SOURCE_FILTER, TRUE},
    {PIN_CONNECTEDTO, SOURCE_FILTER, TRUE},
    {PIN_QUERYPININFO, SOURCE_FILTER, TRUE},
    {KSPROPERTYSET_GET, SOURCE_FILTER, TRUE},
    {ENUMPINS_NEXT, SOURCE_FILTER, TRUE},
    {PIN_ENUMMEDIATYPES, SOURCE_FILTER, TRUE, FALSE, TRUE},
    {ENUMMEDIATYPES_NEXT, SOURCE_FILTER, TRUE, FALSE, TRUE},
    {BASEFILTER_QUERYINTERFACE, SINK_FILTER, FALSE, TRUE},
    {BASEFILTER_ENUMPINS, SINK_FILTER},
    {ENUMPINS_NEXT, SINK_FILTER},
    {PIN_QUERYDIRECTION, SINK_FILTER},
    {PIN_CONNECTEDTO, SINK_FILTER},
    {GRAPHBUILDER_CONNECT, NOT_FILTER},
    {BASEFILTER_GETSTATE, SOURCE_FILTER, TRUE, FALSE, TRUE},
    {BASEFILTER_ENUMPINS, SOURCE_FILTER, FALSE, FALSE, FALSE, TRUE},
    {ENUMPINS_NEXT, SOURCE_FILTER, FALSE, FALSE, FALSE, TRUE},
    {PIN_QUERYDIRECTION, SOURCE_FILTER, FALSE, FALSE, FALSE, TRUE},
    {PIN_CONNECTEDTO, SOURCE_FILTER, FALSE, FALSE, FALSE, TRUE},
    {ENUMPINS_NEXT, SOURCE_FILTER, FALSE, FALSE, FALSE, TRUE},
    {END, NOT_FILTER}
}, renderstream_intermediate[] = {
    {BASEFILTER_QUERYINTERFACE, SOURCE_FILTER},
    {BASEFILTER_ENUMPINS, SOURCE_FILTER},
    {ENUMPINS_NEXT, SOURCE_FILTER},
    {PIN_QUERYDIRECTION, SOURCE_FILTER},
    {PIN_CONNECTEDTO, SOURCE_FILTER},
    {PIN_QUERYPININFO, SOURCE_FILTER, TRUE},
    {KSPROPERTYSET_GET, SOURCE_FILTER},
    {PIN_ENUMMEDIATYPES, SOURCE_FILTER},
    {ENUMMEDIATYPES_RESET, SOURCE_FILTER},
    {ENUMMEDIATYPES_NEXT, SOURCE_FILTER},
    {BASEFILTER_QUERYINTERFACE, SOURCE_FILTER},
    {BASEFILTER_ENUMPINS, SOURCE_FILTER},
    {ENUMPINS_NEXT, SOURCE_FILTER},
    {PIN_QUERYDIRECTION, SOURCE_FILTER},
    {PIN_CONNECTEDTO, SOURCE_FILTER},
    {PIN_QUERYPININFO, SOURCE_FILTER, TRUE},
    {KSPROPERTYSET_GET, SOURCE_FILTER},
    {ENUMPINS_NEXT, SOURCE_FILTER},
    {BASEFILTER_QUERYINTERFACE, SOURCE_FILTER, TRUE},
    {BASEFILTER_ENUMPINS, SOURCE_FILTER, TRUE},
    {ENUMPINS_NEXT, SOURCE_FILTER, TRUE},
    {PIN_QUERYDIRECTION, SOURCE_FILTER, TRUE},
    {PIN_CONNECTEDTO, SOURCE_FILTER, TRUE},
    {PIN_QUERYPININFO, SOURCE_FILTER, TRUE},
    {KSPROPERTYSET_GET, SOURCE_FILTER, TRUE},
    {ENUMPINS_NEXT, SOURCE_FILTER, TRUE},
    {BASEFILTER_QUERYINTERFACE, SOURCE_FILTER, TRUE},
    {BASEFILTER_ENUMPINS, SOURCE_FILTER, TRUE},
    {ENUMPINS_NEXT, SOURCE_FILTER, TRUE},
    {PIN_QUERYDIRECTION, SOURCE_FILTER, TRUE},
    {PIN_CONNECTEDTO, SOURCE_FILTER, TRUE},
    {PIN_QUERYPININFO, SOURCE_FILTER, TRUE},
    {KSPROPERTYSET_GET, SOURCE_FILTER, TRUE},
    {ENUMPINS_NEXT, SOURCE_FILTER, TRUE},
    {PIN_ENUMMEDIATYPES, SOURCE_FILTER, TRUE, FALSE, TRUE},
    {ENUMMEDIATYPES_NEXT, SOURCE_FILTER, TRUE, FALSE, TRUE},
    {BASEFILTER_QUERYINTERFACE, SINK_FILTER, FALSE, TRUE},
    {BASEFILTER_ENUMPINS, SINK_FILTER},
    {ENUMPINS_NEXT, SINK_FILTER},
    {PIN_QUERYDIRECTION, SINK_FILTER},
    {PIN_CONNECTEDTO, SINK_FILTER},
    {BASEFILTER_QUERYINTERFACE, INTERMEDIATE_FILTER, FALSE, TRUE},
    {BASEFILTER_ENUMPINS, INTERMEDIATE_FILTER},
    {ENUMPINS_NEXT, INTERMEDIATE_FILTER},
    {PIN_QUERYDIRECTION, INTERMEDIATE_FILTER},
    {PIN_CONNECTEDTO, INTERMEDIATE_FILTER},
    {ENUMPINS_NEXT, INTERMEDIATE_FILTER},
    {PIN_QUERYDIRECTION, INTERMEDIATE_FILTER},
    {PIN_CONNECTEDTO, INTERMEDIATE_FILTER},
    {GRAPHBUILDER_CONNECT, NOT_FILTER},
    {BASEFILTER_QUERYINTERFACE, INTERMEDIATE_FILTER, FALSE, TRUE},
    {BASEFILTER_ENUMPINS, INTERMEDIATE_FILTER},
    {ENUMPINS_NEXT, INTERMEDIATE_FILTER},
    {PIN_QUERYDIRECTION, INTERMEDIATE_FILTER},
    {PIN_CONNECTEDTO, INTERMEDIATE_FILTER},
    {GRAPHBUILDER_CONNECT, NOT_FILTER},
    {BASEFILTER_GETSTATE, SOURCE_FILTER, TRUE, FALSE, TRUE},
    {BASEFILTER_ENUMPINS, SOURCE_FILTER, FALSE, FALSE, FALSE, TRUE},
    {ENUMPINS_NEXT, SOURCE_FILTER, FALSE, FALSE, FALSE, TRUE},
    {PIN_QUERYDIRECTION, SOURCE_FILTER, FALSE, FALSE, FALSE, TRUE},
    {PIN_CONNECTEDTO, SOURCE_FILTER, FALSE, FALSE, FALSE, TRUE},
    {ENUMPINS_NEXT, SOURCE_FILTER, FALSE, FALSE, FALSE, TRUE},
    {END, NOT_FILTER}
}, *current_calls_list;
int call_no;

static void check_calls_list(const char *func, call_id id, filter_type type)
{
    if(!current_calls_list)
        return;

    while(current_calls_list[call_no].wine_missing || current_calls_list[call_no].wine_extra ||
         current_calls_list[call_no].optional || current_calls_list[call_no].broken) {
        if(current_calls_list[call_no].wine_missing) {
            todo_wine ok((current_calls_list[call_no].call_id == id && current_calls_list[call_no].filter_type == type) ||
                    broken(current_calls_list[call_no].optional && (current_calls_list[call_no].call_id != id ||
                            current_calls_list[call_no].filter_type != type)),
                    "missing call, got %s(%d), expected %d (%d)\n", func, id, current_calls_list[call_no].call_id, call_no);

            if(current_calls_list[call_no].call_id != id || current_calls_list[call_no].filter_type != type)
                call_no++;
            else
                break;
        }else if(current_calls_list[call_no].wine_extra) {
            todo_wine ok(current_calls_list[call_no].call_id != id || current_calls_list[call_no].filter_type != type,
                    "extra call, got %s(%d) (%d)\n", func, id, call_no);

            if(current_calls_list[call_no].call_id == id && current_calls_list[call_no].filter_type == type) {
                call_no++;
                return;
            }
            call_no++;
        }else if(current_calls_list[call_no].optional) {
            ok((current_calls_list[call_no].call_id == id && current_calls_list[call_no].filter_type == type) ||
                    broken(current_calls_list[call_no].call_id != id || current_calls_list[call_no].filter_type != type),
                    "unexpected call: %s on %s (%d)\n", func, debugstr_filter_type(type), call_no);

            if(current_calls_list[call_no].call_id != id || current_calls_list[call_no].filter_type != type)
                call_no++;
            else
                break;
        }else if(current_calls_list[call_no].broken) {
            ok(broken(current_calls_list[call_no].call_id == id && current_calls_list[call_no].filter_type == type) ||
                    (current_calls_list[call_no].call_id != id || current_calls_list[call_no].filter_type != type),
                    "unexpected call: %s on %s (%d)\n", func, debugstr_filter_type(type), call_no);

            if(current_calls_list[call_no].call_id == id && current_calls_list[call_no].filter_type == type)
                break;
            call_no++;
        }
    }

    ok(current_calls_list[call_no].call_id == id, "unexpected call: %s on %s (%d)\n",
            func, debugstr_filter_type(type), call_no);
    if(current_calls_list[call_no].call_id != id)
        return;

    ok(current_calls_list[call_no].filter_type == type, "unexpected call: %s on %s (%d)\n",
            func, debugstr_filter_type(type), call_no);
    if(current_calls_list[call_no].filter_type != type)
        return;

    call_no++;
}

static HRESULT WINAPI GraphBuilder_QueryInterface(
        IGraphBuilder *iface, REFIID riid, void **ppv)
{
    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IFilterGraph)
            || IsEqualIID(riid, &IID_IGraphBuilder))
    {
        *ppv = iface;
        return S_OK;
    }

    ok(IsEqualIID(riid, &IID_IMediaEvent) || IsEqualIID(riid, &IID_IMediaEventSink),
            "QueryInterface(%s)\n", debugstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI GraphBuilder_AddRef(IGraphBuilder *iface)
{
    return 2;
}

static ULONG WINAPI GraphBuilder_Release(IGraphBuilder *iface)
{
    return 1;
}

static HRESULT WINAPI GraphBuilder_AddFilter(IGraphBuilder *iface,
        IBaseFilter *pFilter, LPCWSTR pName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_RemoveFilter(
        IGraphBuilder *iface, IBaseFilter *pFilter)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_EnumFilters(
        IGraphBuilder *iface, IEnumFilters **ppEnum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_FindFilterByName(IGraphBuilder *iface,
        LPCWSTR pName, IBaseFilter **ppFilter)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_ConnectDirect(IGraphBuilder *iface,
        IPin *ppinOut, IPin *ppinIn, const AM_MEDIA_TYPE *pmt)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_Reconnect(IGraphBuilder *iface, IPin *ppin)
{
    CHECK_EXPECT(Reconnect);
    return S_OK;
}

static HRESULT WINAPI GraphBuilder_Disconnect(IGraphBuilder *iface, IPin *ppin)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_SetDefaultSyncSource(IGraphBuilder *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_Connect(IGraphBuilder *iface, IPin *ppinOut, IPin *ppinIn)
{
    check_calls_list("GraphBuilder_Connect", GRAPHBUILDER_CONNECT, NOT_FILTER);
    return S_OK;
}

static HRESULT WINAPI GraphBuilder_Render(IGraphBuilder *iface, IPin *ppinOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_RenderFile(IGraphBuilder *iface,
        LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_AddSourceFilter(IGraphBuilder *iface, LPCWSTR lpcwstrFileName,
        LPCWSTR lpcwstrFilterName, IBaseFilter **ppFilter)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_SetLogFile(IGraphBuilder *iface, DWORD_PTR hFile)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_Abort(IGraphBuilder *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_ShouldOperationContinue(IGraphBuilder *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IGraphBuilderVtbl GraphBuilder_vtbl = {
    GraphBuilder_QueryInterface,
    GraphBuilder_AddRef,
    GraphBuilder_Release,
    GraphBuilder_AddFilter,
    GraphBuilder_RemoveFilter,
    GraphBuilder_EnumFilters,
    GraphBuilder_FindFilterByName,
    GraphBuilder_ConnectDirect,
    GraphBuilder_Reconnect,
    GraphBuilder_Disconnect,
    GraphBuilder_SetDefaultSyncSource,
    GraphBuilder_Connect,
    GraphBuilder_Render,
    GraphBuilder_RenderFile,
    GraphBuilder_AddSourceFilter,
    GraphBuilder_SetLogFile,
    GraphBuilder_Abort,
    GraphBuilder_ShouldOperationContinue
};

static IGraphBuilder GraphBuilder = {&GraphBuilder_vtbl};

typedef struct {
    IBaseFilter IBaseFilter_iface;
    IEnumPins IEnumPins_iface;
    IPin IPin_iface;
    IKsPropertySet IKsPropertySet_iface;
    IMemInputPin IMemInputPin_iface;
    IEnumMediaTypes IEnumMediaTypes_iface;

    PIN_DIRECTION dir;
    filter_type filter_type;

    int enum_pins_pos;
    int enum_media_types_pos;
} test_filter;

static test_filter* impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, test_filter, IBaseFilter_iface);
}

static HRESULT WINAPI BaseFilter_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppv)
{
    test_filter *This = impl_from_IBaseFilter(iface);

    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPersist)
            || IsEqualIID(riid, &IID_IMediaFilter) || IsEqualIID(riid, &IID_IBaseFilter)) {
        *ppv = iface;
        return S_OK;
    }

    check_calls_list("BaseFilter_QueryInterface", BASEFILTER_QUERYINTERFACE, This->filter_type);
    ok(IsEqualIID(riid, &IID_IPin), "riid = %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI BaseFilter_AddRef(IBaseFilter *iface)
{
    return 2;
}

static ULONG WINAPI BaseFilter_Release(IBaseFilter *iface)
{
    return 1;
}

static HRESULT WINAPI BaseFilter_GetClassID(IBaseFilter *iface, CLSID *pClassID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_Stop(IBaseFilter *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_Pause(IBaseFilter *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_GetState(IBaseFilter *iface,
        DWORD dwMilliSecsTimeout, FILTER_STATE *State)
{
    test_filter *This = impl_from_IBaseFilter(iface);
    check_calls_list("BaseFilter_GetState", BASEFILTER_GETSTATE, This->filter_type);
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_SetSyncSource(
        IBaseFilter *iface, IReferenceClock *pClock)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_GetSyncSource(
        IBaseFilter *iface, IReferenceClock **pClock)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_EnumPins(
        IBaseFilter *iface, IEnumPins **ppEnum)
{
    test_filter *This = impl_from_IBaseFilter(iface);
    check_calls_list("BaseFilter_EnumPins", BASEFILTER_ENUMPINS, This->filter_type);

    *ppEnum = &This->IEnumPins_iface;
    This->enum_pins_pos = 0;
    return S_OK;
}

static HRESULT WINAPI BaseFilter_FindPin(IBaseFilter *iface,
        LPCWSTR Id, IPin **ppPin)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_QueryFilterInfo(IBaseFilter *iface, FILTER_INFO *pInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_JoinFilterGraph(IBaseFilter *iface,
        IFilterGraph *pGraph, LPCWSTR pName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_QueryVendorInfo(IBaseFilter *iface, LPWSTR *pVendorInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IBaseFilterVtbl BaseFilterVtbl = {
    BaseFilter_QueryInterface,
    BaseFilter_AddRef,
    BaseFilter_Release,
    BaseFilter_GetClassID,
    BaseFilter_Stop,
    BaseFilter_Pause,
    BaseFilter_Run,
    BaseFilter_GetState,
    BaseFilter_SetSyncSource,
    BaseFilter_GetSyncSource,
    BaseFilter_EnumPins,
    BaseFilter_FindPin,
    BaseFilter_QueryFilterInfo,
    BaseFilter_JoinFilterGraph,
    BaseFilter_QueryVendorInfo
};

static test_filter* impl_from_IEnumPins(IEnumPins *iface)
{
    return CONTAINING_RECORD(iface, test_filter, IEnumPins_iface);
}

static HRESULT WINAPI EnumPins_QueryInterface(IEnumPins *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI EnumPins_AddRef(IEnumPins *iface)
{
    return 2;
}

static ULONG WINAPI EnumPins_Release(IEnumPins *iface)
{
    return 1;
}

static HRESULT WINAPI EnumPins_Next(IEnumPins *iface,
        ULONG cPins, IPin **ppPins, ULONG *pcFetched)
{
    test_filter *This = impl_from_IEnumPins(iface);
    check_calls_list("EnumPins_Next", ENUMPINS_NEXT, This->filter_type);

    ok(cPins == 1, "cPins = %d\n", cPins);
    ok(ppPins != NULL, "ppPins == NULL\n");
    ok(pcFetched != NULL, "pcFetched == NULL\n");

    if(This->enum_pins_pos++ < (This->filter_type == INTERMEDIATE_FILTER ? 2 : 1)) {
        *ppPins = &This->IPin_iface;
        *pcFetched = 1;
        return S_OK;
    }
    *pcFetched = 0;
    return S_FALSE;
}

static HRESULT WINAPI EnumPins_Skip(IEnumPins *iface, ULONG cPins)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumPins_Reset(IEnumPins *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumPins_Clone(IEnumPins *iface, IEnumPins **ppEnum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IEnumPinsVtbl EnumPinsVtbl = {
    EnumPins_QueryInterface,
    EnumPins_AddRef,
    EnumPins_Release,
    EnumPins_Next,
    EnumPins_Skip,
    EnumPins_Reset,
    EnumPins_Clone
};

static test_filter* impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, test_filter, IPin_iface);
}

static HRESULT WINAPI Pin_QueryInterface(IPin *iface, REFIID riid, void **ppv)
{
    test_filter *This = impl_from_IPin(iface);

    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPin)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualIID(riid, &IID_IKsPropertySet)) {
        *ppv = &This->IKsPropertySet_iface;
        return S_OK;
    }

    if(IsEqualIID(riid, &IID_IMemInputPin)) {
        *ppv = &This->IMemInputPin_iface;
        return S_OK;
    }

    ok(0, "unexpected call: %s\n", debugstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Pin_AddRef(IPin *iface)
{
    return 2;
}

static ULONG WINAPI Pin_Release(IPin *iface)
{
    return 1;
}

static HRESULT WINAPI Pin_Connect(IPin *iface, IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_ReceiveConnection(IPin *iface,
        IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    CHECK_EXPECT(ReceiveConnection);

    ok(IsEqualIID(&pmt->majortype, &MEDIATYPE_Stream), "majortype = %s\n",
            debugstr_guid(&pmt->majortype));
    ok(IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_Avi), "subtype = %s\n",
            debugstr_guid(&pmt->subtype));
    ok(pmt->bFixedSizeSamples, "bFixedSizeSamples = %x\n", pmt->bFixedSizeSamples);
    ok(!pmt->bTemporalCompression, "bTemporalCompression = %x\n", pmt->bTemporalCompression);
    ok(pmt->lSampleSize == 1, "lSampleSize = %d\n", pmt->lSampleSize);
    ok(IsEqualIID(&pmt->formattype, &GUID_NULL), "formattype = %s\n",
            debugstr_guid(&pmt->formattype));
    ok(!pmt->pUnk, "pUnk = %p\n", pmt->pUnk);
    ok(!pmt->cbFormat, "cbFormat = %d\n", pmt->cbFormat);
    ok(!pmt->pbFormat, "pbFormat = %p\n", pmt->pbFormat);
    return S_OK;
}

static HRESULT WINAPI Pin_Disconnect(IPin *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_ConnectedTo(IPin *iface, IPin **pPin)
{
    test_filter *This = impl_from_IPin(iface);
    check_calls_list("Pin_ConnectedTo", PIN_CONNECTEDTO, This->filter_type);

    *pPin = NULL;
    return S_OK;
}

static HRESULT WINAPI Pin_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *pmt)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_QueryPinInfo(IPin *iface, PIN_INFO *pInfo)
{
    test_filter *This = impl_from_IPin(iface);
    check_calls_list("Pin_QueryPinInfo", PIN_QUERYPININFO, This->filter_type);
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_QueryDirection(IPin *iface, PIN_DIRECTION *pPinDir)
{
    test_filter *This = impl_from_IPin(iface);
    check_calls_list("Pin_QueryDirection", PIN_QUERYDIRECTION, This->filter_type);

    *pPinDir = This->dir;
    if(This->filter_type==INTERMEDIATE_FILTER && This->enum_pins_pos==2)
        *pPinDir = PINDIR_INPUT;
    return S_OK;
}

static HRESULT WINAPI Pin_QueryId(IPin *iface, LPWSTR *Id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *pmt)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    test_filter *This = impl_from_IPin(iface);
    check_calls_list("Pin_EnumMediaTypes", PIN_ENUMMEDIATYPES, This->filter_type);

    ok(ppEnum != NULL, "ppEnum == NULL\n");
    *ppEnum = &This->IEnumMediaTypes_iface;
    return S_OK;
}

static HRESULT WINAPI Pin_QueryInternalConnections(IPin *iface, IPin **apPin, ULONG *nPin)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_EndOfStream(IPin *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_BeginFlush(IPin *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_EndFlush(IPin *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_NewSegment(IPin *iface, REFERENCE_TIME tStart,
        REFERENCE_TIME tStop, double dRate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IPinVtbl PinVtbl = {
    Pin_QueryInterface,
    Pin_AddRef,
    Pin_Release,
    Pin_Connect,
    Pin_ReceiveConnection,
    Pin_Disconnect,
    Pin_ConnectedTo,
    Pin_ConnectionMediaType,
    Pin_QueryPinInfo,
    Pin_QueryDirection,
    Pin_QueryId,
    Pin_QueryAccept,
    Pin_EnumMediaTypes,
    Pin_QueryInternalConnections,
    Pin_EndOfStream,
    Pin_BeginFlush,
    Pin_EndFlush,
    Pin_NewSegment
};

static test_filter* impl_from_IKsPropertySet(IKsPropertySet *iface)
{
    return CONTAINING_RECORD(iface, test_filter, IKsPropertySet_iface);
}

static HRESULT WINAPI KsPropertySet_QueryInterface(IKsPropertySet *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI KsPropertySet_AddRef(IKsPropertySet *iface)
{
    return 2;
}

static ULONG WINAPI KsPropertySet_Release(IKsPropertySet *iface)
{
    return 1;
}

static HRESULT WINAPI KsPropertySet_Set(IKsPropertySet *iface, REFGUID guidPropSet, DWORD dwPropID,
        LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI KsPropertySet_Get(IKsPropertySet *iface, REFGUID guidPropSet, DWORD dwPropID,
        LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned)
{
    test_filter *This = impl_from_IKsPropertySet(iface);
    check_calls_list("KsPropertySet_Get", KSPROPERTYSET_GET, This->filter_type);

    ok(IsEqualIID(guidPropSet, &AMPROPSETID_Pin), "guidPropSet = %s\n", debugstr_guid(guidPropSet));
    ok(dwPropID == 0, "dwPropID = %d\n", dwPropID);
    ok(pInstanceData == NULL, "pInstanceData != NULL\n");
    ok(cbInstanceData == 0, "cbInstanceData != 0\n");
    ok(cbPropData == sizeof(GUID), "cbPropData = %d\n", cbPropData);
    *pcbReturned = sizeof(GUID);
    memcpy(pPropData, &PIN_CATEGORY_EDS, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI KsPropertySet_QuerySupported(IKsPropertySet *iface,
        REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IKsPropertySetVtbl KsPropertySetVtbl = {
    KsPropertySet_QueryInterface,
    KsPropertySet_AddRef,
    KsPropertySet_Release,
    KsPropertySet_Set,
    KsPropertySet_Get,
    KsPropertySet_QuerySupported
};

static HRESULT WINAPI MemInputPin_QueryInterface(IMemInputPin *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI MemInputPin_AddRef(IMemInputPin *iface)
{
    return 2;
}

static ULONG WINAPI MemInputPin_Release(IMemInputPin *iface)
{
    return 1;
}

static HRESULT WINAPI MemInputPin_GetAllocator(IMemInputPin *iface, IMemAllocator **ppAllocator)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MemInputPin_NotifyAllocator(IMemInputPin *iface,
        IMemAllocator *pAllocator, BOOL bReadOnly)
{
    ALLOCATOR_PROPERTIES ap;
    HRESULT hr;

    CHECK_EXPECT(NotifyAllocator);

    ok(pAllocator != NULL, "pAllocator = %p\n", pAllocator);
    ok(bReadOnly, "bReadOnly = %x\n", bReadOnly);

    hr = IMemAllocator_GetProperties(pAllocator, &ap);
    ok(hr == S_OK, "GetProperties returned %x\n", hr);
    ok(ap.cBuffers == 32, "cBuffers = %d\n", ap.cBuffers);
    ok(ap.cbBuffer == 0, "cbBuffer = %d\n", ap.cbBuffer);
    ok(ap.cbAlign == 1, "cbAlign = %d\n", ap.cbAlign);
    ok(ap.cbPrefix == 0, "cbPrefix = %d\n", ap.cbPrefix);
    return S_OK;
}

static HRESULT WINAPI MemInputPin_GetAllocatorRequirements(
        IMemInputPin *iface, ALLOCATOR_PROPERTIES *pProps)
{
    CHECK_EXPECT(GetAllocatorRequirements);
    return E_NOTIMPL;
}

static HRESULT WINAPI MemInputPin_Receive(IMemInputPin *iface, IMediaSample *pSample)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MemInputPin_ReceiveMultiple(IMemInputPin *iface,
        IMediaSample **pSamples, LONG nSamples, LONG *nSamplesProcessed)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MemInputPin_ReceiveCanBlock(IMemInputPin *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IMemInputPinVtbl MemInputPinVtbl = {
    MemInputPin_QueryInterface,
    MemInputPin_AddRef,
    MemInputPin_Release,
    MemInputPin_GetAllocator,
    MemInputPin_NotifyAllocator,
    MemInputPin_GetAllocatorRequirements,
    MemInputPin_Receive,
    MemInputPin_ReceiveMultiple,
    MemInputPin_ReceiveCanBlock
};

static test_filter* impl_from_IEnumMediaTypes(IEnumMediaTypes *iface)
{
    return CONTAINING_RECORD(iface, test_filter, IEnumMediaTypes_iface);
}

static HRESULT WINAPI EnumMediaTypes_QueryInterface(IEnumMediaTypes *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI EnumMediaTypes_AddRef(IEnumMediaTypes *iface)
{
    return 2;
}

static ULONG WINAPI EnumMediaTypes_Release(IEnumMediaTypes *iface)
{
    return 1;
}

static HRESULT WINAPI EnumMediaTypes_Next(IEnumMediaTypes *iface, ULONG cMediaTypes,
        AM_MEDIA_TYPE **ppMediaTypes, ULONG *pcFetched)
{
    test_filter *This = impl_from_IEnumMediaTypes(iface);
    check_calls_list("EnumMediaTypes_Next", ENUMMEDIATYPES_NEXT, This->filter_type);

    ok(cMediaTypes == 1, "cMediaTypes = %d\n", cMediaTypes);
    ok(ppMediaTypes != NULL, "ppMediaTypes == NULL\n");
    ok(pcFetched != NULL, "pcFetched == NULL\n");

    if(!This->enum_media_types_pos++) {
        ppMediaTypes[0] = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        memset(ppMediaTypes[0], 0, sizeof(AM_MEDIA_TYPE));
        ppMediaTypes[0]->majortype = MEDIATYPE_Video;
        *pcFetched = 1;
        return S_OK;
    }

    *pcFetched = 0;
    return S_FALSE;
}

static HRESULT WINAPI EnumMediaTypes_Skip(IEnumMediaTypes *iface, ULONG cMediaTypes)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumMediaTypes_Reset(IEnumMediaTypes *iface)
{
    test_filter *This = impl_from_IEnumMediaTypes(iface);
    check_calls_list("EnumMediaTypes_Reset", ENUMMEDIATYPES_RESET, This->filter_type);

    This->enum_media_types_pos = 0;
    return S_OK;
}

static HRESULT WINAPI EnumMediaTypes_Clone(IEnumMediaTypes *iface, IEnumMediaTypes **ppEnum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IEnumMediaTypesVtbl EnumMediaTypesVtbl = {
    EnumMediaTypes_QueryInterface,
    EnumMediaTypes_AddRef,
    EnumMediaTypes_Release,
    EnumMediaTypes_Next,
    EnumMediaTypes_Skip,
    EnumMediaTypes_Reset,
    EnumMediaTypes_Clone
};

static void init_test_filter(test_filter *This, PIN_DIRECTION dir, filter_type type)
{
    memset(This, 0, sizeof(*This));
    This->IBaseFilter_iface.lpVtbl = &BaseFilterVtbl;
    This->IEnumPins_iface.lpVtbl = &EnumPinsVtbl;
    This->IPin_iface.lpVtbl = &PinVtbl;
    This->IKsPropertySet_iface.lpVtbl = &KsPropertySetVtbl;
    This->IMemInputPin_iface.lpVtbl = &MemInputPinVtbl;
    This->IEnumMediaTypes_iface.lpVtbl = &EnumMediaTypesVtbl;

    This->dir = dir;
    This->filter_type = type;
}

static void test_CaptureGraphBuilder_RenderStream(void)
{
    test_filter source_filter, sink_filter, intermediate_filter;
    ICaptureGraphBuilder2 *cgb;
    HRESULT hr;

    init_test_filter(&source_filter, PINDIR_OUTPUT, SOURCE_FILTER);
    init_test_filter(&sink_filter, PINDIR_INPUT, SINK_FILTER);
    init_test_filter(&intermediate_filter, PINDIR_OUTPUT, INTERMEDIATE_FILTER);

    hr = CoCreateInstance(&CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER,
            &IID_ICaptureGraphBuilder2, (void**)&cgb);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG),
            "couldn't create CaptureGraphBuilder, hr = %08x\n", hr);
    if(hr != S_OK) {
        win_skip("CaptureGraphBuilder is not registered\n");
        return;
    }

    hr = ICaptureGraphBuilder2_SetFiltergraph(cgb, &GraphBuilder);
    ok(hr == S_OK, "SetFiltergraph failed: %08x\n", hr);

    trace("RenderStream with category and mediatype test\n");
    current_calls_list = renderstream_cat_media;
    call_no = 0;
    hr = ICaptureGraphBuilder2_RenderStream(cgb, &PIN_CATEGORY_EDS,
            &MEDIATYPE_Video, (IUnknown*)&source_filter.IBaseFilter_iface,
            NULL, &sink_filter.IBaseFilter_iface);
    ok(hr == S_OK, "RenderStream failed: %08x\n", hr);
    check_calls_list("test_CaptureGraphBuilder_RenderStream", END, NOT_FILTER);

    trace("RenderStream with intermediate filter\n");
    current_calls_list = renderstream_intermediate;
    call_no = 0;
    hr = ICaptureGraphBuilder2_RenderStream(cgb, &PIN_CATEGORY_EDS,
            &MEDIATYPE_Video, (IUnknown*)&source_filter.IBaseFilter_iface,
            &intermediate_filter.IBaseFilter_iface, &sink_filter.IBaseFilter_iface);
    ok(hr == S_OK, "RenderStream failed: %08x\n", hr);
    check_calls_list("test_CaptureGraphBuilder_RenderStream", END, NOT_FILTER);

    ICaptureGraphBuilder2_Release(cgb);
}

static void test_AviMux_QueryInterface(void)
{
    IUnknown *avimux, *unk;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_AviDest, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&avimux);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG),
            "couldn't create AVI Mux filter, hr = %08x\n", hr);
    if(hr != S_OK) {
        win_skip("AVI Mux filter is not registered\n");
        return;
    }

    hr = IUnknown_QueryInterface(avimux, &IID_IBaseFilter, (void**)&unk);
    ok(hr == S_OK, "QueryInterface(IID_IBaseFilter) failed: %x\n", hr);
    IUnknown_Release(unk);

    hr = IUnknown_QueryInterface(avimux, &IID_IConfigAviMux, (void**)&unk);
    ok(hr == S_OK, "QueryInterface(IID_IConfigAviMux) failed: %x\n", hr);
    IUnknown_Release(unk);

    hr = IUnknown_QueryInterface(avimux, &IID_IConfigInterleaving, (void**)&unk);
    ok(hr == S_OK, "QueryInterface(IID_IConfigInterleaving) failed: %x\n", hr);
    IUnknown_Release(unk);

    hr = IUnknown_QueryInterface(avimux, &IID_IMediaSeeking, (void**)&unk);
    ok(hr == S_OK, "QueryInterface(IID_IMediaSeeking) failed: %x\n", hr);
    IUnknown_Release(unk);

    hr = IUnknown_QueryInterface(avimux, &IID_IPersistMediaPropertyBag, (void**)&unk);
    ok(hr == S_OK, "QueryInterface(IID_IPersistMediaPropertyBag) failed: %x\n", hr);
    IUnknown_Release(unk);

    hr = IUnknown_QueryInterface(avimux, &IID_ISpecifyPropertyPages, (void**)&unk);
    ok(hr == S_OK, "QueryInterface(IID_ISpecifyPropertyPages) failed: %x\n", hr);
    IUnknown_Release(unk);

    IUnknown_Release(avimux);
}

static void test_AviMux(void)
{
    test_filter source_filter, sink_filter;
    VIDEOINFOHEADER videoinfoheader;
    IPin *avimux_in, *avimux_out, *pin;
    AM_MEDIA_TYPE source_media_type;
    AM_MEDIA_TYPE *media_type;
    PIN_DIRECTION dir;
    IBaseFilter *avimux;
    IEnumPins *ep;
    IEnumMediaTypes *emt;
    HRESULT hr;

    init_test_filter(&source_filter, PINDIR_OUTPUT, SOURCE_FILTER);
    init_test_filter(&sink_filter, PINDIR_INPUT, SINK_FILTER);

    hr = CoCreateInstance(&CLSID_AviDest, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void**)&avimux);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG),
            "couldn't create AVI Mux filter, hr = %08x\n", hr);
    if(hr != S_OK) {
        win_skip("AVI Mux filter is not registered\n");
        return;
    }

    hr = IBaseFilter_EnumPins(avimux, &ep);
    ok(hr == S_OK, "EnumPins returned %x\n", hr);

    hr = IEnumPins_Next(ep, 1, &avimux_out, NULL);
    ok(hr == S_OK, "Next returned %x\n", hr);
    hr = IPin_QueryDirection(avimux_out, &dir);
    ok(hr == S_OK, "QueryDirection returned %x\n", hr);
    ok(dir == PINDIR_OUTPUT, "dir = %d\n", dir);

    hr = IEnumPins_Next(ep, 1, &avimux_in, NULL);
    ok(hr == S_OK, "Next returned %x\n", hr);
    hr = IPin_QueryDirection(avimux_in, &dir);
    ok(hr == S_OK, "QueryDirection returned %x\n", hr);
    ok(dir == PINDIR_INPUT, "dir = %d\n", dir);
    IEnumPins_Release(ep);

    hr = IPin_EnumMediaTypes(avimux_out, &emt);
    ok(hr == S_OK, "EnumMediaTypes returned %x\n", hr);
    hr = IEnumMediaTypes_Next(emt, 1, &media_type, NULL);
    ok(hr == S_OK, "Next returned %x\n", hr);
    ok(IsEqualIID(&media_type->majortype, &MEDIATYPE_Stream), "majortype = %s\n",
            debugstr_guid(&media_type->majortype));
    ok(IsEqualIID(&media_type->subtype, &MEDIASUBTYPE_Avi), "subtype = %s\n",
            debugstr_guid(&media_type->subtype));
    ok(media_type->bFixedSizeSamples, "bFixedSizeSamples = %x\n", media_type->bFixedSizeSamples);
    ok(!media_type->bTemporalCompression, "bTemporalCompression = %x\n", media_type->bTemporalCompression);
    ok(media_type->lSampleSize == 1, "lSampleSize = %d\n", media_type->lSampleSize);
    ok(IsEqualIID(&media_type->formattype, &GUID_NULL), "formattype = %s\n",
            debugstr_guid(&media_type->formattype));
    ok(!media_type->pUnk, "pUnk = %p\n", media_type->pUnk);
    ok(!media_type->cbFormat, "cbFormat = %d\n", media_type->cbFormat);
    ok(!media_type->pbFormat, "pbFormat = %p\n", media_type->pbFormat);
    CoTaskMemFree(media_type);
    hr = IEnumMediaTypes_Next(emt, 1, &media_type, NULL);
    ok(hr == S_FALSE, "Next returned %x\n", hr);
    IEnumMediaTypes_Release(emt);

    hr = IPin_EnumMediaTypes(avimux_in, &emt);
    ok(hr == S_OK, "EnumMediaTypes returned %x\n", hr);
    hr = IEnumMediaTypes_Reset(emt);
    ok(hr == S_OK, "Reset returned %x\n", hr);
    hr = IEnumMediaTypes_Next(emt, 1, &media_type, NULL);
    ok(hr == S_FALSE, "Next returned %x\n", hr);
    IEnumMediaTypes_Release(emt);

    hr = IPin_ReceiveConnection(avimux_in, &source_filter.IPin_iface, NULL);
    ok(hr == E_POINTER, "ReceiveConnection returned %x\n", hr);

    current_calls_list = NULL;
    memset(&source_media_type, 0, sizeof(AM_MEDIA_TYPE));
    memset(&videoinfoheader, 0, sizeof(VIDEOINFOHEADER));
    source_media_type.majortype = MEDIATYPE_Video;
    source_media_type.subtype = MEDIASUBTYPE_RGB32;
    source_media_type.formattype = FORMAT_VideoInfo;
    source_media_type.bFixedSizeSamples = TRUE;
    source_media_type.lSampleSize = 40000;
    source_media_type.cbFormat = sizeof(VIDEOINFOHEADER);
    source_media_type.pbFormat = (BYTE*)&videoinfoheader;
    videoinfoheader.AvgTimePerFrame = 333333;
    videoinfoheader.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    videoinfoheader.bmiHeader.biWidth = 100;
    videoinfoheader.bmiHeader.biHeight = 100;
    videoinfoheader.bmiHeader.biPlanes = 1;
    videoinfoheader.bmiHeader.biBitCount = 32;
    videoinfoheader.bmiHeader.biSizeImage = 40000;
    videoinfoheader.bmiHeader.biClrImportant = 256;
    hr = IPin_ReceiveConnection(avimux_in, &source_filter.IPin_iface, &source_media_type);
    ok(hr == S_OK, "ReceiveConnection returned %x\n", hr);

    hr = IPin_ConnectedTo(avimux_in, &pin);
    ok(hr == S_OK, "ConnectedTo returned %x\n", hr);
    ok(pin == &source_filter.IPin_iface, "incorrect pin: %p, expected %p\n",
            pin, &source_filter.IPin_iface);

    hr = IPin_Connect(avimux_out, &source_filter.IPin_iface, NULL);
    todo_wine ok(hr == VFW_E_INVALID_DIRECTION, "Connect returned %x\n", hr);

    hr = IBaseFilter_JoinFilterGraph(avimux, (IFilterGraph*)&GraphBuilder, NULL);
    ok(hr == S_OK, "JoinFilterGraph returned %x\n", hr);

    SET_EXPECT(ReceiveConnection);
    SET_EXPECT(GetAllocatorRequirements);
    SET_EXPECT(NotifyAllocator);
    SET_EXPECT(Reconnect);
    hr = IPin_Connect(avimux_out, &sink_filter.IPin_iface, NULL);
    ok(hr == S_OK, "Connect returned %x\n", hr);
    CHECK_CALLED(ReceiveConnection);
    CHECK_CALLED(GetAllocatorRequirements);
    CHECK_CALLED(NotifyAllocator);
    CHECK_CALLED(Reconnect);

    hr = IPin_Disconnect(avimux_out);
    ok(hr == S_OK, "Disconnect returned %x\n", hr);

    IPin_Release(avimux_in);
    IPin_Release(avimux_out);
    IBaseFilter_Release(avimux);
}

START_TEST(qcap)
{
    if (SUCCEEDED(CoInitialize(NULL)))
    {
        test_smart_tee_filter();
        test_CaptureGraphBuilder_RenderStream();
        test_AviMux_QueryInterface();
        test_AviMux();
        CoUninitialize();
    }
    else
        skip("CoInitialize failed\n");
}
