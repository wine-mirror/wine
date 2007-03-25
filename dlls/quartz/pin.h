/*
 * IPin function declarations to allow inheritance
 *
 * Copyright 2003 Robert Shearman
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

/* This function will process incoming samples to the pin.
 * Any return value valid in IMemInputPin::Receive is allowed here
 */
typedef HRESULT (* SAMPLEPROC)(LPVOID userdata, IMediaSample * pSample);

/* This function will determine whether a type is supported or not.
 * It is allowed to return any error value (within reason), as opposed
 * to IPin::QueryAccept which is only allowed to return S_OK or S_FALSE.
 */
typedef HRESULT (* QUERYACCEPTPROC)(LPVOID userdata, const AM_MEDIA_TYPE * pmt);

/* This function is called prior to finalizing a connection with
 * another pin and can be used to get things from the other pin
 * like IMemInput interfaces.
 */
typedef HRESULT (* PRECONNECTPROC)(IPin * iface, IPin * pConnectPin);

typedef struct IPinImpl
{
	const struct IPinVtbl * lpVtbl;
	LONG refCount;
	LPCRITICAL_SECTION pCritSec;
	PIN_INFO pinInfo;
	IPin * pConnectedTo;
	AM_MEDIA_TYPE mtCurrent;
	ENUMMEDIADETAILS enumMediaDetails;
	QUERYACCEPTPROC fnQueryAccept;
	LPVOID pUserData;
} IPinImpl;

typedef struct InputPin
{
	/* inheritance C style! */
	IPinImpl pin;

	const IMemInputPinVtbl * lpVtblMemInput;
	IMemAllocator * pAllocator;
	SAMPLEPROC fnSampleProc;
	REFERENCE_TIME tStart;
	REFERENCE_TIME tStop;
	double dRate;
} InputPin;

typedef struct OutputPin
{
	/* inheritance C style! */
	IPinImpl pin;

	IMemInputPin * pMemInputPin;
	HRESULT (* pConnectSpecific)(IPin * iface, IPin * pReceiver, const AM_MEDIA_TYPE * pmt);
	ALLOCATOR_PROPERTIES allocProps;
} OutputPin;

typedef struct PullPin
{
	/* inheritance C style! */
	IPinImpl pin;

	IAsyncReader * pReader;
	IMemAllocator * pAlloc;
	SAMPLEPROC fnSampleProc;
	PRECONNECTPROC fnPreConnect;
	HANDLE hThread;
	HANDLE hEventStateChanged;
	REFERENCE_TIME rtStart;
	REFERENCE_TIME rtStop;
	REFERENCE_TIME rtCurrent;
} PullPin;

/*** Initializers ***/
HRESULT InputPin_Init(const PIN_INFO * pPinInfo, SAMPLEPROC pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, InputPin * pPinImpl);
HRESULT OutputPin_Init(const PIN_INFO * pPinInfo, ALLOCATOR_PROPERTIES *props, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, OutputPin * pPinImpl);
HRESULT PullPin_Init(const PIN_INFO * pPinInfo, SAMPLEPROC pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, PullPin * pPinImpl);

/*** Constructors ***/
HRESULT InputPin_Construct(const PIN_INFO * pPinInfo, SAMPLEPROC pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, IPin ** ppPin);
HRESULT OutputPin_Construct(const PIN_INFO * pPinInfo, ALLOCATOR_PROPERTIES *props, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, IPin ** ppPin);
HRESULT PullPin_Construct(const PIN_INFO * pPinInfo, SAMPLEPROC pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, IPin ** ppPin);

/**************************/
/*** Pin Implementation ***/

/* Common */
ULONG   WINAPI IPinImpl_AddRef(IPin * iface);
HRESULT WINAPI IPinImpl_Disconnect(IPin * iface);
HRESULT WINAPI IPinImpl_ConnectedTo(IPin * iface, IPin ** ppPin);
HRESULT WINAPI IPinImpl_ConnectionMediaType(IPin * iface, AM_MEDIA_TYPE * pmt);
HRESULT WINAPI IPinImpl_QueryPinInfo(IPin * iface, PIN_INFO * pInfo);
HRESULT WINAPI IPinImpl_QueryDirection(IPin * iface, PIN_DIRECTION * pPinDir);
HRESULT WINAPI IPinImpl_QueryId(IPin * iface, LPWSTR * Id);
HRESULT WINAPI IPinImpl_QueryAccept(IPin * iface, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI IPinImpl_EnumMediaTypes(IPin * iface, IEnumMediaTypes ** ppEnum);
HRESULT WINAPI IPinImpl_QueryInternalConnections(IPin * iface, IPin ** apPin, ULONG * cPin);

/* Input Pin */
HRESULT WINAPI InputPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv);
ULONG   WINAPI InputPin_Release(IPin * iface);
HRESULT WINAPI InputPin_Connect(IPin * iface, IPin * pConnector, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI InputPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI InputPin_EndOfStream(IPin * iface);
HRESULT WINAPI InputPin_BeginFlush(IPin * iface);
HRESULT WINAPI InputPin_EndFlush(IPin * iface);
HRESULT WINAPI InputPin_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

/* Output Pin */
HRESULT WINAPI OutputPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv);
ULONG   WINAPI OutputPin_Release(IPin * iface);
HRESULT WINAPI OutputPin_Connect(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI OutputPin_Disconnect(IPin * iface);
HRESULT WINAPI OutputPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI OutputPin_EndOfStream(IPin * iface);
HRESULT WINAPI OutputPin_BeginFlush(IPin * iface);
HRESULT WINAPI OutputPin_EndFlush(IPin * iface);
HRESULT WINAPI OutputPin_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

HRESULT OutputPin_CommitAllocator(OutputPin * This);
HRESULT OutputPin_GetDeliveryBuffer(OutputPin * This, IMediaSample ** ppSample, REFERENCE_TIME * tStart, REFERENCE_TIME * tStop, DWORD dwFlags);
HRESULT OutputPin_SendSample(OutputPin * This, IMediaSample * pSample);
HRESULT OutputPin_DeliverDisconnect(OutputPin * This);
HRESULT OutputPin_DeliverNewSegment(OutputPin * This, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

/**********************************/
/*** MemInputPin Implementation ***/

HRESULT WINAPI MemInputPin_QueryInterface(IMemInputPin * iface, REFIID riid, LPVOID * ppv);
ULONG   WINAPI MemInputPin_AddRef(IMemInputPin * iface);
ULONG   WINAPI MemInputPin_Release(IMemInputPin * iface);
HRESULT WINAPI MemInputPin_GetAllocator(IMemInputPin * iface, IMemAllocator ** ppAllocator);
HRESULT WINAPI MemInputPin_NotifyAllocator(IMemInputPin * iface, IMemAllocator * pAllocator, BOOL bReadOnly);
HRESULT WINAPI MemInputPin_GetAllocatorRequirements(IMemInputPin * iface, ALLOCATOR_PROPERTIES * pProps);
HRESULT WINAPI MemInputPin_Receive(IMemInputPin * iface, IMediaSample * pSample);
HRESULT WINAPI MemInputPin_ReceiveMultiple(IMemInputPin * iface, IMediaSample ** pSamples, long nSamples, long *nSamplesProcessed);
HRESULT WINAPI MemInputPin_ReceiveCanBlock(IMemInputPin * iface);

/* Pull Pin */
HRESULT WINAPI PullPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI PullPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv);
ULONG   WINAPI PullPin_Release(IPin * iface);
HRESULT PullPin_InitProcessing(PullPin * This);
HRESULT PullPin_StartProcessing(PullPin * This);
HRESULT PullPin_StopProcessing(PullPin * This);
HRESULT PullPin_PauseProcessing(PullPin * This);
HRESULT PullPin_Seek(PullPin * This, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
HRESULT WINAPI PullPin_EndOfStream(IPin * iface);
HRESULT WINAPI PullPin_BeginFlush(IPin * iface);
HRESULT WINAPI PullPin_EndFlush(IPin * iface);
HRESULT WINAPI PullPin_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
HRESULT PullPin_WaitForStateChange(PullPin * This, DWORD dwMilliseconds);
