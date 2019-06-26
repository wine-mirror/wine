/*
 * Parser declarations
 *
 * Copyright 2005 Christian Costa
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

typedef struct ParserImpl ParserImpl;

typedef HRESULT (*PFN_PROCESS_SAMPLE) (LPVOID iface, IMediaSample * pSample, DWORD_PTR cookie);
typedef HRESULT (*PFN_QUERY_ACCEPT) (LPVOID iface, const AM_MEDIA_TYPE * pmt);
typedef HRESULT (*PFN_PRE_CONNECT) (IPin * iface, IPin * pConnectPin, ALLOCATOR_PROPERTIES *prop);
typedef HRESULT (*PFN_CLEANUP) (LPVOID iface);
typedef HRESULT (*PFN_DISCONNECT) (LPVOID iface);

typedef struct Parser_OutputPin
{
    BaseOutputPin pin;

    AM_MEDIA_TYPE * pmt;
    LONGLONG dwSamplesProcessed;
    ALLOCATOR_PROPERTIES allocProps;

    IMemAllocator *alloc;
    BOOL readonly;
} Parser_OutputPin;

struct ParserImpl
{
    BaseFilter filter;

    PFN_DISCONNECT fnDisconnect;

    PullPin *pInputPin;
    Parser_OutputPin **sources;
    ULONG cStreams;
    SourceSeeking sourceSeeking;
};

extern HRESULT Parser_AddPin(ParserImpl * This, const PIN_INFO * piOutput, ALLOCATOR_PROPERTIES * props, const AM_MEDIA_TYPE * amt);

HRESULT Parser_Create(ParserImpl *parser, const IBaseFilterVtbl *vtbl, IUnknown *outer,
        const CLSID *clsid, const BaseFilterFuncTable *func_table, const WCHAR *sink_name,
        PFN_PROCESS_SAMPLE, PFN_QUERY_ACCEPT, PFN_PRE_CONNECT, PFN_CLEANUP, PFN_DISCONNECT,
        REQUESTPROC, STOPPROCESSPROC, SourceSeeking_ChangeStop,
        SourceSeeking_ChangeStart, SourceSeeking_ChangeRate) DECLSPEC_HIDDEN;

/* Override the _Release function and call this when releasing */
extern void Parser_Destroy(ParserImpl *This);

extern HRESULT WINAPI Parser_Stop(IBaseFilter * iface);
extern HRESULT WINAPI Parser_Pause(IBaseFilter * iface);
extern HRESULT WINAPI Parser_Run(IBaseFilter * iface, REFERENCE_TIME tStart);
extern HRESULT WINAPI Parser_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState);
extern HRESULT WINAPI Parser_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock);

IPin *parser_get_pin(BaseFilter *iface, unsigned int index) DECLSPEC_HIDDEN;

/* COM helpers */
static inline Parser_OutputPin *unsafe_impl_Parser_OutputPin_from_IPin( IPin *iface )
{
    return CONTAINING_RECORD(iface, Parser_OutputPin, pin.pin.IPin_iface);
}
