/*
 * Unit tests for CLSID_DirectSoundPrivate property set functions
 * (used by dxdiag)
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS
#include <windows.h>

#include "wine/test.h"
#include "mmsystem.h"
#include "dsound.h"
#include "dsconf.h"

#include "dsound_test.h"

#ifndef DSBCAPS_CTRLDEFAULT
#define DSBCAPS_CTRLDEFAULT \
        DSBCAPS_CTRLFREQUENCY|DSBCAPS_CTRLPAN|DSBCAPS_CTRLVOLUME
#endif

#include "initguid.h"

DEFINE_GUID(DSPROPSETID_VoiceManager,
            0x62A69BAE,0xDF9D,0x11D1,0x99,0xA6,0x00,0xC0,0x4F,0xC9,0x9D,0x46);
DEFINE_GUID(DSPROPSETID_EAX20_ListenerProperties,
            0x306a6a8,0xb224,0x11d2,0x99,0xe5,0x0,0x0,0xe8,0xd8,0xc7,0x22);
DEFINE_GUID(DSPROPSETID_EAX20_BufferProperties,
            0x306a6a7,0xb224,0x11d2,0x99,0xe5,0x0,0x0,0xe8,0xd8,0xc7,0x22);
DEFINE_GUID(DSPROPSETID_I3DL2_ListenerProperties,
            0xDA0F0520,0x300A,0x11D3,0x8A,0x2B,0x00,0x60,0x97,0x0D,0xB0,0x11);
DEFINE_GUID(DSPROPSETID_I3DL2_BufferProperties,
            0xDA0F0521,0x300A,0x11D3,0x8A,0x2B,0x00,0x60,0x97,0x0D,0xB0,0x11);
DEFINE_GUID(DSPROPSETID_ZOOMFX_BufferProperties,
            0xCD5368E0,0x3450,0x11D3,0x8B,0x6E,0x00,0x10,0x5A,0x9B,0x7B,0xBC);

static HRESULT (WINAPI *pDllGetClassObject)(REFCLSID,REFIID,LPVOID*)=NULL;

static BOOL CALLBACK callback(PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA data,
                       LPVOID context)
{
    trace("  found device:\n");
    trace("    Type: %s\n",
          data->Type == DIRECTSOUNDDEVICE_TYPE_EMULATED ? "Emulated" :
          data->Type == DIRECTSOUNDDEVICE_TYPE_VXD ? "VxD" :
          data->Type == DIRECTSOUNDDEVICE_TYPE_WDM ? "WDM" : "Unknown");
    trace("    DataFlow: %s\n",
          data->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER ? "Render" :
          data->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE ?
          "Capture" : "Unknown");
    trace("    DeviceId: %s\n", wine_dbgstr_guid(&data->DeviceId));
    trace("    Description: %s\n", data->Description);
    trace("    Module: %s\n", data->Module);
    trace("    Interface: %s\n", data->Interface);
    trace("    WaveDeviceId: %ld\n", data->WaveDeviceId);

    return TRUE;
}

static BOOL CALLBACK callback1(PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA data,
                        LPVOID context)
{
    char descriptionA[0x100];
    char moduleA[MAX_PATH];

    trace("  found device:\n");
    trace("    Type: %s\n",
          data->Type == DIRECTSOUNDDEVICE_TYPE_EMULATED ? "Emulated" :
          data->Type == DIRECTSOUNDDEVICE_TYPE_VXD ? "VxD" :
          data->Type == DIRECTSOUNDDEVICE_TYPE_WDM ? "WDM" : "Unknown");
    trace("    DataFlow: %s\n",
          data->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER ? "Render" :
          data->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE ?
          "Capture" : "Unknown");
    trace("    DeviceId: %s\n", wine_dbgstr_guid(&data->DeviceId));
    trace("    DescriptionA: %s\n", data->DescriptionA);
    WideCharToMultiByte(CP_ACP, 0, data->DescriptionW, -1, descriptionA, sizeof(descriptionA), NULL, NULL);
    trace("    DescriptionW: %s\n", descriptionA);
    trace("    ModuleA: %s\n", data->ModuleA);
    WideCharToMultiByte(CP_ACP, 0, data->ModuleW, -1, moduleA, sizeof(moduleA), NULL, NULL);
    trace("    ModuleW: %s\n", moduleA);
    trace("    WaveDeviceId: %ld\n", data->WaveDeviceId);

    return TRUE;
}

static BOOL CALLBACK callbackA(PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA data,
                        LPVOID context)
{
    trace("  found device:\n");
    trace("    Type: %s\n",
          data->Type == DIRECTSOUNDDEVICE_TYPE_EMULATED ? "Emulated" :
          data->Type == DIRECTSOUNDDEVICE_TYPE_VXD ? "VxD" :
          data->Type == DIRECTSOUNDDEVICE_TYPE_WDM ? "WDM" : "Unknown");
    trace("    DataFlow: %s\n",
          data->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER ? "Render" :
          data->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE ?
          "Capture" : "Unknown");
    trace("    DeviceId: %s\n", wine_dbgstr_guid(&data->DeviceId));
    trace("    Description: %s\n", data->Description);
    trace("    Module: %s\n", data->Module);
    trace("    Interface: %s\n", data->Interface);
    trace("    WaveDeviceId: %ld\n", data->WaveDeviceId);

    return TRUE;
}

static BOOL CALLBACK callbackW(PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA data,
                        LPVOID context)
{
    char descriptionA[0x100];
    char moduleA[MAX_PATH];
    char interfaceA[MAX_PATH];

    trace("found device:\n");
    trace("\tType: %s\n",
          data->Type == DIRECTSOUNDDEVICE_TYPE_EMULATED ? "Emulated" :
          data->Type == DIRECTSOUNDDEVICE_TYPE_VXD ? "VxD" :
          data->Type == DIRECTSOUNDDEVICE_TYPE_WDM ? "WDM" : "Unknown");
    trace("\tDataFlow: %s\n",
          data->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER ? "Render" :
          data->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE ?
          "Capture" : "Unknown");
    trace("\tDeviceId: %s\n", wine_dbgstr_guid(&data->DeviceId));
    WideCharToMultiByte(CP_ACP, 0, data->Description, -1, descriptionA, sizeof(descriptionA), NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, data->Module, -1, moduleA, sizeof(moduleA), NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, data->Interface, -1, interfaceA, sizeof(interfaceA), NULL, NULL);
    trace("\tDescription: %s\n", descriptionA);
    trace("\tModule: %s\n", moduleA);
    trace("\tInterface: %s\n", interfaceA);
    trace("\tWaveDeviceId: %ld\n", data->WaveDeviceId);

    return TRUE;
}

static void propset_private_tests(void)
{
    HRESULT rc;
    IClassFactory * pcf;
    IKsPropertySet * pps;
    ULONG support;

    /* try direct sound first */
    /* DSOUND: Error: Invalid interface buffer */
    rc = (pDllGetClassObject)(&CLSID_DirectSound, &IID_IClassFactory, NULL);
    ok(rc==DSERR_INVALIDPARAM,"DllGetClassObject(CLSID_DirectSound, "
       "IID_IClassFactory) should have returned DSERR_INVALIDPARAM, "
       "returned: %08lx\n",rc);

    rc = (pDllGetClassObject)(&CLSID_DirectSound, &IID_IDirectSound, (void **)(&pcf));
    ok(rc==E_NOINTERFACE,"DllGetClassObject(CLSID_DirectSound, "
       "IID_IDirectSound) should have returned E_NOINTERFACE, "
       "returned: %08lx\n",rc);

    rc = (pDllGetClassObject)(&CLSID_DirectSound, &IID_IUnknown, (void **)(&pcf));
    ok(rc==DS_OK,"DllGetClassObject(CLSID_DirectSound, "
       "IID_IUnknown) failed: %08lx\n",rc);

    rc = (pDllGetClassObject)(&CLSID_DirectSound, &IID_IClassFactory, (void **)(&pcf));
    ok(pcf!=0, "DllGetClassObject(CLSID_DirectSound, IID_IClassFactory) "
       "failed: %08lx\n",rc);
    if (pcf==0)
        return;

    /* direct sound doesn't have an IKsPropertySet */
    /* DSOUND: Error: Invalid interface buffer */
    rc = IClassFactory_CreateInstance(pcf, NULL, &IID_IKsPropertySet,
                                     NULL);
    ok(rc==DSERR_INVALIDPARAM, "CreateInstance(IID_IKsPropertySet) should have "
       "returned DSERR_INVALIDPARAM, returned: %08lx\n",rc);

    rc = IClassFactory_CreateInstance(pcf, NULL, &IID_IKsPropertySet,
                                     (void **)(&pps));
    ok(rc==E_NOINTERFACE, "CreateInstance(IID_IKsPropertySet) should have "
       "returned E_NOINTERFACE, returned: %08lx\n",rc);

    /* and the direct sound 8 version */
    rc = pDllGetClassObject(&CLSID_DirectSound8, &IID_IClassFactory, (void **)&pcf);
    ok(pcf!=0, "DllGetClassObject(CLSID_DirectSound8, IID_IClassFactory) "
       "failed: %08lx\n",rc);
    if (pcf==0)
        return;

    /* direct sound 8 doesn't have an IKsPropertySet */
    rc = IClassFactory_CreateInstance(pcf, NULL, &IID_IKsPropertySet,
                                     (void **)(&pps));
    ok(rc==E_NOINTERFACE, "CreateInstance(IID_IKsPropertySet) should have "
       "returned E_NOINTERFACE, returned: %08lx\n",rc);

    /* try direct sound capture next */
    rc = pDllGetClassObject(&CLSID_DirectSoundCapture, &IID_IClassFactory, (void **)&pcf);
    ok(pcf!=0, "DllGetClassObject(CLSID_DirectSoundCapture, IID_IClassFactory) "
       "failed: %08lx\n",rc);
    if (pcf==0)
        return;

    /* direct sound capture doesn't have an IKsPropertySet */
    rc = IClassFactory_CreateInstance(pcf, NULL, &IID_IKsPropertySet,
                                     (void **)(&pps));
    ok(rc==E_NOINTERFACE, "CreateInstance(IID_IKsPropertySet) should have "
       "returned E_NOINTERFACE,returned: %08lx\n",rc);

    /* and the direct sound capture 8 version */
    rc = pDllGetClassObject(&CLSID_DirectSoundCapture8, &IID_IClassFactory, (void **)&pcf);
    ok(pcf!=0, "DllGetClassObject(CLSID_DirectSoundCapture8, "
       "IID_IClassFactory) failed: %08lx\n",rc);
    if (pcf==0)
        return;

    /* direct sound capture 8 doesn't have an IKsPropertySet */
    rc = IClassFactory_CreateInstance(pcf, NULL, &IID_IKsPropertySet,
                                     (void **)(&pps));
    ok(rc==E_NOINTERFACE, "CreateInstance(IID_IKsPropertySet) should have "
       "returned E_NOINTERFACE, returned: %08lx\n",rc);

    /* try direct sound full duplex next */
    rc = (pDllGetClassObject)(&CLSID_DirectSoundFullDuplex, &IID_IClassFactory,
                 (void **)(&pcf));
    ok(pcf!=0, "DllGetClassObject(CLSID_DirectSoundFullDuplex, "
       "IID_IClassFactory) failed: %08lx\n",rc);
    if (pcf==0)
        return;

    /* direct sound full duplex doesn't have an IKsPropertySet */
    rc = IClassFactory_CreateInstance(pcf, NULL, &IID_IKsPropertySet,
                                     (void **)(&pps));
    ok(rc==E_NOINTERFACE, "CreateInstance(IID_IKsPropertySet) should have "
       "returned NOINTERFACE, returned: %08lx\n",rc);

    /* try direct sound private last */
    rc = (pDllGetClassObject)(&CLSID_DirectSoundPrivate, &IID_IClassFactory,
                 (void **)(&pcf));

    /* some early versions of Direct Sound do not have this */
    if (rc==CLASS_E_CLASSNOTAVAILABLE)
        return;

    /* direct sound private does have an IKsPropertySet */
    rc = IClassFactory_CreateInstance(pcf, NULL, &IID_IKsPropertySet,
                                     (void **)(&pps));
    ok(rc==DS_OK, "CreateInstance(IID_IKsPropertySet) failed: %08lx\n",
       rc);
    if (rc!=DS_OK)
        return;

    /* test generic DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION */
    rc = IKsPropertySet_QuerySupport(pps, &DSPROPSETID_DirectSoundDevice,
                                   DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION,
                                   &support);
    ok(rc==DS_OK||rc==E_INVALIDARG,
       "QuerySupport(DSPROPSETID_DirectSoundDevice, "
       "DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION) failed: %08lx\n",
       rc);
    if (rc!=DS_OK) {
        if (rc==E_INVALIDARG)
            trace("  Not Supported\n");
        return;
    }

    ok(support & KSPROPERTY_SUPPORT_GET,
       "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION: "
       "support = 0x%lx\n",support);
    ok(!(support & KSPROPERTY_SUPPORT_SET),
       "Shouldn't be able to set DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION: "
       "support = 0x%lx\n",support);

    /* test DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1 */
    rc = IKsPropertySet_QuerySupport(pps, &DSPROPSETID_DirectSoundDevice,
                                   DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1,
                                   &support);
    ok(rc==DS_OK||rc==E_INVALIDARG,
       "QuerySupport(DSPROPSETID_DirectSoundDevice, "
       "DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1) failed: %08lx\n",
       rc);
    if (rc!=DS_OK) {
        if (rc==E_INVALIDARG)
            trace("  Not Supported\n");
        return;
    }

    ok(support & KSPROPERTY_SUPPORT_GET,
       "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1: "
       "support = 0x%lx\n",support);
    ok(!(support & KSPROPERTY_SUPPORT_SET),
       "Shouldn't be able to set DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1: "
       "support = 0x%lx\n",support);

    /* test DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A */
    rc = IKsPropertySet_QuerySupport(pps, &DSPROPSETID_DirectSoundDevice,
                                   DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A,
                                   &support);
    ok(rc==DS_OK||rc==E_INVALIDARG,
       "QuerySupport(DSPROPSETID_DirectSoundDevice, "
       "DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A) failed: %08lx\n",
       rc);
    if (rc!=DS_OK) {
        if (rc==E_INVALIDARG)
            trace("  Not Supported\n");
        return;
    }

    ok(support & KSPROPERTY_SUPPORT_GET,
       "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A: "
       "support = 0x%lx\n",support);
    ok(!(support & KSPROPERTY_SUPPORT_SET),
       "Shouldn't be able to set DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A: "
       "support = 0x%lx\n",support);

    /* test DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W */
    rc = IKsPropertySet_QuerySupport(pps, &DSPROPSETID_DirectSoundDevice,
                                   DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W,
                                   &support);
    ok(rc==DS_OK||rc==E_INVALIDARG,
       "QuerySupport(DSPROPSETID_DirectSoundDevice, "
       "DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W) failed: %08lx\n",
       rc);
    if (rc!=DS_OK) {
        if (rc==E_INVALIDARG)
            trace("  Not Supported\n");
        return;
    }

    ok(support & KSPROPERTY_SUPPORT_GET,
       "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W: "
       "support = 0x%lx\n",support);
    ok(!(support & KSPROPERTY_SUPPORT_SET),
       "Shouldn't be able to set DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W: "
       "support = 0x%lx\n",support);

    /* test generic DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING */
    rc = IKsPropertySet_QuerySupport(pps, &DSPROPSETID_DirectSoundDevice,
        DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING, &support);
    ok(rc==DS_OK, "QuerySupport(DSPROPSETID_DirectSoundDevice, "
       "DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING) failed: %08lx\n",
       rc);
    if (rc!=DS_OK)
        return;

    ok(support & KSPROPERTY_SUPPORT_GET,
       "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING: "
       "support = 0x%lx\n",support);
    ok(!(support & KSPROPERTY_SUPPORT_SET), "Shouldn't be able to set "
       "DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING: support = "
       "0x%lx\n",support);

    /* test DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A */
    rc = IKsPropertySet_QuerySupport(pps, &DSPROPSETID_DirectSoundDevice,
        DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A, &support);
    ok(rc==DS_OK, "QuerySupport(DSPROPSETID_DirectSoundDevice, "
       "DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A) failed: %08lx\n",
       rc);
    if (rc!=DS_OK)
        return;

    ok(support & KSPROPERTY_SUPPORT_GET,
       "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A: "
       "support = 0x%lx\n",support);
    ok(!(support & KSPROPERTY_SUPPORT_SET), "Shouldn't be able to set "
       "DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A: support = "
       "0x%lx\n",support);

    /* test DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W */
    rc = IKsPropertySet_QuerySupport(pps, &DSPROPSETID_DirectSoundDevice,
        DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W, &support);
    ok(rc==DS_OK, "QuerySupport(DSPROPSETID_DirectSoundDevice, "
       "DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W) failed: %08lx\n",
       rc);
    if (rc!=DS_OK)
        return;

    ok(support & KSPROPERTY_SUPPORT_GET,
       "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W: "
       "support = 0x%lx\n",support);
    ok(!(support & KSPROPERTY_SUPPORT_SET), "Shouldn't be able to set "
       "DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W: support = "
       "0x%lx\n",support);

    /* test generic DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE */
    trace("*** Testing DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE ***\n");
    rc = IKsPropertySet_QuerySupport(pps, &DSPROPSETID_DirectSoundDevice,
                                   DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE,
                                   &support);
    ok(rc==DS_OK, "QuerySupport(DSPROPSETID_DirectSoundDevice, "
       "DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE) failed: %08lx\n",
       rc);
    if (rc!=DS_OK)
        return;

    ok(support & KSPROPERTY_SUPPORT_GET,
       "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE: "
       "support = 0x%lx\n",support);
    ok(!(support & KSPROPERTY_SUPPORT_SET),"Shouldn't be able to set "
       "DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE: support = 0x%lx\n",support);

    if (support & KSPROPERTY_SUPPORT_GET) {
        DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_DATA data;
        ULONG bytes;

        data.Callback = callback;
        data.Context = 0;

        rc = IKsPropertySet_Get(pps, &DSPROPSETID_DirectSoundDevice,
                              DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE,
                              NULL, 0, &data, sizeof(data), &bytes);
        ok(rc==DS_OK, "Couldn't enumerate: 0x%lx\n",rc);
    }

    /* test DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1 */
    trace("*** Testing DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1 ***\n");
    rc = IKsPropertySet_QuerySupport(pps, &DSPROPSETID_DirectSoundDevice,
                                   DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1,
                                   &support);
    ok(rc==DS_OK, "QuerySupport(DSPROPSETID_DirectSoundDevice, "
       "DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1) failed: %08lx\n",
       rc);
    if (rc!=DS_OK)
        return;

    ok(support & KSPROPERTY_SUPPORT_GET,
       "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1: "
       "support = 0x%lx\n",support);
    ok(!(support & KSPROPERTY_SUPPORT_SET),"Shouldn't be able to set "
       "DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1: support = 0x%lx\n",support);

    if (support & KSPROPERTY_SUPPORT_GET) {
        DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1_DATA data;
        ULONG bytes;

        data.Callback = callback1;
        data.Context = 0;

        rc = IKsPropertySet_Get(pps, &DSPROPSETID_DirectSoundDevice,
                              DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1,
                              NULL, 0, &data, sizeof(data), &bytes);
        ok(rc==DS_OK, "Couldn't enumerate: 0x%lx\n",rc);
    }

    /* test DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A */
    trace("*** Testing DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A ***\n");
    rc = IKsPropertySet_QuerySupport(pps, &DSPROPSETID_DirectSoundDevice,
                                   DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A,
                                   &support);
    ok(rc==DS_OK, "QuerySupport(DSPROPSETID_DirectSoundDevice, "
       "DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A) failed: %08lx\n",
       rc);
    if (rc!=DS_OK)
        return;

    ok(support & KSPROPERTY_SUPPORT_GET,
       "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A: "
       "support = 0x%lx\n",support);
    ok(!(support & KSPROPERTY_SUPPORT_SET),"Shouldn't be able to set "
       "DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A: support = 0x%lx\n",support);

    if (support & KSPROPERTY_SUPPORT_GET) {
        DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A_DATA data;
        ULONG bytes;

        data.Callback = callbackA;
        data.Context = 0;

        rc = IKsPropertySet_Get(pps, &DSPROPSETID_DirectSoundDevice,
                              DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A,
                              NULL, 0, &data, sizeof(data), &bytes);
        ok(rc==DS_OK, "Couldn't enumerate: 0x%lx\n",rc);
    }

    /* test DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W */
    trace("*** Testing DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W ***\n");
    rc = IKsPropertySet_QuerySupport(pps, &DSPROPSETID_DirectSoundDevice,
                                   DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W,
                                   &support);
    ok(rc==DS_OK, "QuerySupport(DSPROPSETID_DirectSoundDevice, "
       "DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W) failed: %08lx\n",
       rc);
    if (rc!=DS_OK)
        return;

    ok(support & KSPROPERTY_SUPPORT_GET,
       "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W: "
       "support = 0x%lx\n",support);
    ok(!(support & KSPROPERTY_SUPPORT_SET),"Shouldn't be able to set "
       "DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W: support = 0x%lx\n",support);

    if (support & KSPROPERTY_SUPPORT_GET) {
        DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W_DATA data;
        ULONG bytes;

        data.Callback = callbackW;
        data.Context = 0;

        rc = IKsPropertySet_Get(pps, &DSPROPSETID_DirectSoundDevice,
                              DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W,
                              NULL, 0, &data, sizeof(data), &bytes);
        ok(rc==DS_OK, "Couldn't enumerate: 0x%lx\n",rc);
    }
    IKsPropertySet_Release(pps);
}

static unsigned driver_count = 0;

static BOOL WINAPI dsenum_callback(LPGUID lpGuid, LPCSTR lpcstrDescription,
                                   LPCSTR lpcstrModule, LPVOID lpContext)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL,secondary=NULL;
    DSBUFFERDESC bufdesc;
    WAVEFORMATEX wfx;
    int ref;

    trace("*** Testing %s - %s ***\n",lpcstrDescription,lpcstrModule);
    driver_count++;

    rc = DirectSoundCreate(lpGuid, &dso, NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCreate() failed: %08lx\n",rc);
    if (rc!=DS_OK) {
        if (rc==DSERR_NODRIVER)
            trace("  No Driver\n");
        else if (rc == DSERR_ALLOCATED)
            trace("  Already In Use\n");
        else if (rc == E_FAIL)
            trace("  No Device\n");
        goto EXIT;
    }

    /* We must call SetCooperativeLevel before calling CreateSoundBuffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %08lx\n",
       rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* Testing 3D buffers */
    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_LOCHARDWARE|DSBCAPS_CTRL3D;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok((rc==DS_OK&&primary!=NULL)
       || broken(rc==DSERR_INVALIDPARAM),
       "IDirectSound_CreateSoundBuffer() failed to "
       "create a hardware 3D primary buffer: %08lx\n",rc);
    if(rc==DSERR_INVALIDPARAM) {
       skip("broken driver\n");
       goto EXIT;
    }
    if (rc==DS_OK&&primary!=NULL) {
        ZeroMemory(&wfx, sizeof(wfx));
        wfx.wFormatTag=WAVE_FORMAT_PCM;
        wfx.nChannels=1;
        wfx.wBitsPerSample=16;
        wfx.nSamplesPerSec=44100;
        wfx.nBlockAlign=wfx.nChannels*wfx.wBitsPerSample/8;
        wfx.nAvgBytesPerSec=wfx.nSamplesPerSec*wfx.nBlockAlign;
        ZeroMemory(&bufdesc, sizeof(bufdesc));
        bufdesc.dwSize=sizeof(bufdesc);
        bufdesc.dwFlags=DSBCAPS_CTRLDEFAULT|DSBCAPS_GETCURRENTPOSITION2;
        bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec;
        bufdesc.lpwfxFormat=&wfx;
        trace("  Testing a secondary buffer at %ldx%dx%d\n",
            wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels);
        rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
        ok((rc==DS_OK && secondary!=NULL) || broken(rc == DSERR_CONTROLUNAVAIL), /* vmware drivers on w2k */
           "IDirectSound_CreateSoundBuffer() failed to create a secondary buffer: %08lx\n",rc);
        if (rc==DS_OK&&secondary!=NULL) {
            IKsPropertySet * pPropertySet=NULL;
            rc=IDirectSoundBuffer_QueryInterface(secondary,
                                                 &IID_IKsPropertySet,
                                                 (void **)&pPropertySet);
            /* it's not an error for this to fail */
            if(rc==DS_OK) {
                ULONG ulTypeSupport;
                trace("  Supports property sets\n");
                /* it's not an error for these to fail */
                rc=IKsPropertySet_QuerySupport(pPropertySet,
                                               &DSPROPSETID_VoiceManager,
                                               0,&ulTypeSupport);
                if((rc==DS_OK)&&(ulTypeSupport&(KSPROPERTY_SUPPORT_GET|
                                                KSPROPERTY_SUPPORT_SET)))
                    trace("    DSPROPSETID_VoiceManager supported\n");
                else
                    trace("    DSPROPSETID_VoiceManager not supported\n");
                rc=IKsPropertySet_QuerySupport(pPropertySet,
                    &DSPROPSETID_EAX20_ListenerProperties,0,&ulTypeSupport);
                if((rc==DS_OK)&&(ulTypeSupport&(KSPROPERTY_SUPPORT_GET|
                    KSPROPERTY_SUPPORT_SET)))
                    trace("    DSPROPSETID_EAX20_ListenerProperties "
                          "supported\n");
                else
                    trace("    DSPROPSETID_EAX20_ListenerProperties not "
                          "supported\n");
                rc=IKsPropertySet_QuerySupport(pPropertySet,
                    &DSPROPSETID_EAX20_BufferProperties,0,&ulTypeSupport);
                if((rc==DS_OK)&&(ulTypeSupport&(KSPROPERTY_SUPPORT_GET|
                    KSPROPERTY_SUPPORT_SET)))
                    trace("    DSPROPSETID_EAX20_BufferProperties supported\n");
                else
                    trace("    DSPROPSETID_EAX20_BufferProperties not "
                          "supported\n");
                rc=IKsPropertySet_QuerySupport(pPropertySet,
                    &DSPROPSETID_I3DL2_ListenerProperties,0,&ulTypeSupport);
                if((rc==DS_OK)&&(ulTypeSupport&(KSPROPERTY_SUPPORT_GET|
                    KSPROPERTY_SUPPORT_SET)))
                    trace("    DSPROPSETID_I3DL2_ListenerProperties "
                          "supported\n");
                else
                    trace("    DSPROPSETID_I3DL2_ListenerProperties not "
                          "supported\n");
                rc=IKsPropertySet_QuerySupport(pPropertySet,
                    &DSPROPSETID_I3DL2_BufferProperties,0,&ulTypeSupport);
                if((rc==DS_OK)&&(ulTypeSupport&(KSPROPERTY_SUPPORT_GET|
                    KSPROPERTY_SUPPORT_SET)))
                    trace("    DSPROPSETID_I3DL2_BufferProperties supported\n");
                else
                    trace("    DSPROPSETID_I3DL2_BufferProperties not "
                          "supported\n");
                rc=IKsPropertySet_QuerySupport(pPropertySet,
                    &DSPROPSETID_ZOOMFX_BufferProperties,0,&ulTypeSupport);
                if((rc==DS_OK)&&(ulTypeSupport&(KSPROPERTY_SUPPORT_GET|
                    KSPROPERTY_SUPPORT_SET)))
                    trace("    DSPROPSETID_ZOOMFX_BufferProperties "
                          "supported\n");
                else
                    trace("    DSPROPSETID_ZOOMFX_BufferProperties not "
                          "supported\n");
                ref=IKsPropertySet_Release(pPropertySet);
                /* try a few common ones */
                ok(ref==0,"IKsPropertySet_Release() secondary has %d "
                   "references, should have 0\n",ref);
            } else
                trace("  Doesn't support property sets\n");

            ref=IDirectSoundBuffer_Release(secondary);
            ok(ref==0,"IDirectSoundBuffer_Release() secondary has %d "
               "references, should have 0\n",ref);
        }

        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

EXIT:
    if (dso!=NULL) {
        ref=IDirectSound_Release(dso);
        ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",
           ref);
    }
    return TRUE;
}

static void propset_buffer_tests(void)
{
    HRESULT rc;
    rc = DirectSoundEnumerateA(dsenum_callback, NULL);
    ok(rc==DS_OK,"DirectSoundEnumerateA() failed: %08lx\n",rc);
    trace("tested %u DirectSound drivers\n", driver_count);
}

START_TEST(propset)
{
    CoInitialize(NULL);

    pDllGetClassObject = (void *)GetProcAddress(GetModuleHandleA("dsound"), "DllGetClassObject");

    propset_private_tests();
    propset_buffer_tests();

    CoUninitialize();
}
