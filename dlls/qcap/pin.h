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

typedef struct OutputPin
{
	/* inheritance C style! */
	IPinImpl pin;

	IMemInputPin * pMemInputPin;
	HRESULT (* pConnectSpecific)(IPin * iface, IPin * pReceiver, const AM_MEDIA_TYPE * pmt);
	ALLOCATOR_PROPERTIES allocProps;
} OutputPin;

/*** Initializers ***/
HRESULT OutputPin_Init(const PIN_INFO * pPinInfo, const ALLOCATOR_PROPERTIES *props,
                       LPVOID pUserData, QUERYACCEPTPROC pQueryAccept,
                       LPCRITICAL_SECTION pCritSec, OutputPin * pPinImpl);

/* Common */
HRESULT WINAPI IPinImpl_ConnectedTo(IPin * iface, IPin ** ppPin);
HRESULT WINAPI IPinImpl_ConnectionMediaType(IPin * iface, AM_MEDIA_TYPE * pmt);
HRESULT WINAPI IPinImpl_QueryPinInfo(IPin * iface, PIN_INFO * pInfo);
HRESULT WINAPI IPinImpl_QueryDirection(IPin * iface, PIN_DIRECTION * pPinDir);
HRESULT WINAPI IPinImpl_QueryId(IPin * iface, LPWSTR * Id);
HRESULT WINAPI IPinImpl_QueryAccept(IPin * iface, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI IPinImpl_EnumMediaTypes(IPin * iface, IEnumMediaTypes ** ppEnum);
HRESULT WINAPI IPinImpl_QueryInternalConnections(IPin * iface, IPin ** apPin, ULONG * cPin);

/* Output Pin */
HRESULT WINAPI OutputPin_Connect(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI OutputPin_Disconnect(IPin * iface);
HRESULT WINAPI OutputPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);

HRESULT OutputPin_GetDeliveryBuffer(OutputPin * This, IMediaSample ** ppSample, REFERENCE_TIME * tStart, REFERENCE_TIME * tStop, DWORD dwFlags);
HRESULT OutputPin_SendSample(OutputPin * This, IMediaSample * pSample);
