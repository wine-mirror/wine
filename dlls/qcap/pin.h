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

typedef struct OutputPin
{
	/* inheritance C style! */
	BasePin pin;

	IMemInputPin * pMemInputPin;
	HRESULT (* pConnectSpecific)(IPin * iface, IPin * pReceiver, const AM_MEDIA_TYPE * pmt);
	ALLOCATOR_PROPERTIES allocProps;
} OutputPin;

/*** Initializers ***/
HRESULT OutputPin_Init(const PIN_INFO * pPinInfo, const ALLOCATOR_PROPERTIES *props,
                       LPCRITICAL_SECTION pCritSec, OutputPin * pPinImpl);

/* Output Pin */
HRESULT WINAPI OutputPin_Connect(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI OutputPin_Disconnect(IPin * iface);
HRESULT WINAPI OutputPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);

HRESULT OutputPin_GetDeliveryBuffer(OutputPin * This, IMediaSample ** ppSample, REFERENCE_TIME * tStart, REFERENCE_TIME * tStop, DWORD dwFlags);
HRESULT OutputPin_SendSample(OutputPin * This, IMediaSample * pSample);
