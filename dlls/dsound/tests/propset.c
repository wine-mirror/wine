/*
 * Unit tests for CLSID_DirectSoundPrivate property set functions (used by dxdiag)
 *
 * Copyright (c) 2003 Robert Reif
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include <windows.h>

#include <math.h>
#include <stdlib.h>

#include "wine/test.h"
#include "dsound.h"
#include "dsconf.h"

typedef HRESULT  (CALLBACK * MYPROC)(REFCLSID, REFIID, LPVOID FAR*);

BOOL CALLBACK callback(PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA data, LPVOID context)
{
	trace("found device:\n");
	trace("\tType: %s\n", 
		data->Type == DIRECTSOUNDDEVICE_TYPE_EMULATED ? "Emulated" :
		data->Type == DIRECTSOUNDDEVICE_TYPE_VXD ? "VxD" :
        	data->Type == DIRECTSOUNDDEVICE_TYPE_WDM ? "WDM" : "Unknown");
	trace("\tDataFlow: %s\n",
        	data->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER ? "Render" :
        	data->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE ? "Capture" : "Unknown");
	trace("\tDeviceId: {%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
		data->DeviceId.Data1,data->DeviceId.Data2,data->DeviceId.Data3,
		data->DeviceId.Data4[0],data->DeviceId.Data4[1],data->DeviceId.Data4[2],data->DeviceId.Data4[3],
		data->DeviceId.Data4[4],data->DeviceId.Data4[5],data->DeviceId.Data4[6],data->DeviceId.Data4[7]);
	trace("\tDescription: %s\n", data->Description);
	trace("\tModule: %s\n", data->Module);
	trace("\tInterface: %s\n", data->Interface);
	trace("\tWaveDeviceId: %ld\n", data->WaveDeviceId);

	return TRUE;
}

static void propset_tests()
{
	HMODULE	hDsound;
	HRESULT hr;
	IClassFactory	* pcf;
	IKsPropertySet	* pps;
	MYPROC fProc;
	ULONG	support;

	hDsound = LoadLibrary("dsound.dll");
	ok(hDsound!=0,"LoadLibrary(dsound.dll) failed\n");
	if (hDsound==0)
		return;

	fProc = (MYPROC)GetProcAddress(hDsound, "DllGetClassObject");

	/* try direct sound first */
	hr = (fProc)(&CLSID_DirectSound, &IID_IClassFactory, (void **)0);
	ok(hr==DSERR_INVALIDPARAM,"DllGetClassObject(CLSID_DirectSound, IID_IClassFactory) should have failed: 0x%lx\n",hr);

	hr = (fProc)(&CLSID_DirectSound, &IID_IClassFactory, (void **)(&pcf));
	ok(pcf!=0, "DllGetClassObject(CLSID_DirectSound, IID_IClassFactory) failed: 0x%lx\n",hr);
	if (pcf==0)
		goto error;

	/* direct sound doesn't have an IKsPropertySet */
	hr = pcf->lpVtbl->CreateInstance(pcf, NULL, &IID_IKsPropertySet, (void **)0);
	ok(hr==DSERR_INVALIDPARAM, "CreateInstance(IID_IKsPropertySet) should have failed: 0x%lx\n",hr);

	hr = pcf->lpVtbl->CreateInstance(pcf, NULL, &IID_IKsPropertySet, (void **)(&pps));
	ok(hr==E_NOINTERFACE, "CreateInstance(IID_IKsPropertySet) should have failed: 0x%lx\n",hr);

	/* and the direct sound 8 version */
	hr = (fProc)(&CLSID_DirectSound8, &IID_IClassFactory, (void **)(&pcf));
	ok(pcf!=0, "DllGetClassObject(CLSID_DirectSound8, IID_IClassFactory) failed: 0x%lx\n",hr);
	if (pcf==0)
		goto error;

	/* direct sound 8 doesn't have an IKsPropertySet */
	hr = pcf->lpVtbl->CreateInstance(pcf, NULL, &IID_IKsPropertySet, (void **)(&pps));
	ok(hr==E_NOINTERFACE, "CreateInstance(IID_IKsPropertySet) should have failed: 0x%lx\n",hr);

	/* try direct sound capture next */
	hr = (fProc)(&CLSID_DirectSoundCapture, &IID_IClassFactory, (void **)(&pcf));
	ok(pcf!=0, "DllGetClassObject(CLSID_DirectSoundCapture, IID_IClassFactory) failed: 0x%lx\n",hr);
	if (pcf==0)
		goto error;

	/* direct sound capture doesn't have an IKsPropertySet */
	hr = pcf->lpVtbl->CreateInstance(pcf, NULL, &IID_IKsPropertySet, (void **)(&pps));
	ok(hr==E_NOINTERFACE, "CreateInstance(IID_IKsPropertySet) should have failed: 0x%lx\n",hr);

	/* and the direct sound capture 8 version */
	hr = (fProc)(&CLSID_DirectSoundCapture8, &IID_IClassFactory, (void **)(&pcf));
	ok(pcf!=0, "DllGetClassObject(CLSID_DirectSoundCapture8, IID_IClassFactory) failed: 0x%lx\n",hr);
	if (pcf==0)
		goto error;

	/* direct sound capture 8 doesn't have an IKsPropertySet */
	hr = pcf->lpVtbl->CreateInstance(pcf, NULL, &IID_IKsPropertySet, (void **)(&pps));
	ok(hr==E_NOINTERFACE, "CreateInstance(IID_IKsPropertySet) should have failed: 0x%lx\n",hr);

	/* try direct sound full duplex next */
	hr = (fProc)(&CLSID_DirectSoundFullDuplex, &IID_IClassFactory, (void **)(&pcf));
	ok(pcf!=0, "DllGetClassObject(CLSID_DirectSoundFullDuplex, IID_IClassFactory) failed: 0x%lx\n",hr);
	if (pcf==0)
		goto error;

	/* direct sound full duplex doesn't have an IKsPropertySet */
	hr = pcf->lpVtbl->CreateInstance(pcf, NULL, &IID_IKsPropertySet, (void **)(&pps));
	ok(hr==E_NOINTERFACE, "CreateInstance(IID_IKsPropertySet) should have failed: 0x%lx\n",hr);

	/* try direct sound private last */
	hr = (fProc)(&CLSID_DirectSoundPrivate, &IID_IClassFactory, (void **)(&pcf));
	ok(pcf!=0, "DllGetClassObject(CLSID_DirectSoundPrivate, IID_IClassFactory) failed: 0x%lx\n",hr);
	if (pcf==0)
		goto error;

	/* direct sound private does have an IKsPropertySet */
	hr = pcf->lpVtbl->CreateInstance(pcf, NULL, &IID_IKsPropertySet, (void **)(&pps));
	ok(hr==DS_OK, "CreateInstance(IID_IKsPropertySet) failed: 0x%lx\n",hr);
	if (hr!=DS_OK)
		goto error;

	hr = pps->lpVtbl->QuerySupport(pps, &DSPROPSETID_DirectSoundDevice, DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION, &support);
	ok(hr==DS_OK, "QuerySupport(DSPROPSETID_DirectSoundDevice, DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION) failed: 0x%lx\n",hr);
	if (hr!=DS_OK)
		goto error;

	ok (support & KSPROPERTY_SUPPORT_GET, "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION: support = 0x%lx\n",support);
	ok (!(support & KSPROPERTY_SUPPORT_SET), "Shouldn't be able to set DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION: support = 0x%lx\n",support);

	hr = pps->lpVtbl->QuerySupport(pps, &DSPROPSETID_DirectSoundDevice, DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING, &support);
	ok(hr==DS_OK, "QuerySupport(DSPROPSETID_DirectSoundDevice, DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING) failed: 0x%lx\n",hr);
	if (hr!=DS_OK)
		goto error;

	ok (support & KSPROPERTY_SUPPORT_GET, "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING: support = 0x%lx\n",support);
	ok (!(support & KSPROPERTY_SUPPORT_SET), "Shouldn't be able to set DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING: support = 0x%lx\n",support);

	hr = pps->lpVtbl->QuerySupport(pps, &DSPROPSETID_DirectSoundDevice, DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE, &support);
	ok(hr==DS_OK, "QuerySupport(DSPROPSETID_DirectSoundDevice, DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE) failed: 0x%lx\n",hr);
	if (hr!=DS_OK)
		goto error;

	ok (support & KSPROPERTY_SUPPORT_GET, "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE: support = 0x%lx\n",support);
	ok (!(support & KSPROPERTY_SUPPORT_SET), "Shouldn't be able to set DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE: support = 0x%lx\n",support);

	if (support & KSPROPERTY_SUPPORT_GET)
	{
		DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_DATA data;
		ULONG bytes;

		data.Callback = callback;
		data.Context = 0;

		hr = pps->lpVtbl->Get(pps, &DSPROPSETID_DirectSoundDevice, 
					  DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE,
					  NULL,
					  0,
					  &data, 
					  sizeof(data), 
					  &bytes);
		ok(hr==DS_OK, "Couldn't enumerate: 0x%lx\n",hr);
	}

error:
	FreeLibrary(hDsound);
}

START_TEST(propset)
{
    propset_tests();
}
