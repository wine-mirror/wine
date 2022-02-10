/*
 * Copyright 2000 Corel Corporation
 * Copyright 2006 Marcus Meissner
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

#include "gphoto2_i.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

static TW_UINT16 GPHOTO2_ICAPXferMech (pTW_CAPABILITY,TW_UINT16);
static TW_UINT16 GPHOTO2_ICAPPixelType (pTW_CAPABILITY,TW_UINT16);
static TW_UINT16 GPHOTO2_ICAPPixelFlavor (pTW_CAPABILITY,TW_UINT16);
static TW_UINT16 GPHOTO2_ICAPBitDepth (pTW_CAPABILITY,TW_UINT16);
static TW_UINT16 GPHOTO2_ICAPUnits (pTW_CAPABILITY,TW_UINT16);

TW_UINT16 GPHOTO2_SaneCapability (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT16 twCC = TWCC_SUCCESS;

    TRACE("capability=%d action=%d\n", pCapability->Cap, action);

    switch (pCapability->Cap)
    {
        case CAP_DEVICEEVENT:
        case CAP_ALARMS:
        case CAP_ALARMVOLUME:
        case ACAP_AUDIOFILEFORMAT:
        case ACAP_XFERMECH:
        case ICAP_AUTOMATICBORDERDETECTION:
        case ICAP_AUTOMATICDESKEW:
        case ICAP_AUTODISCARDBLANKPAGES:
        case ICAP_AUTOMATICROTATE:
        case ICAP_FLIPROTATION:
        case CAP_AUTOMATICCAPTURE:
        case CAP_TIMEBEFOREFIRSTCAPTURE:
        case CAP_TIMEBETWEENCAPTURES:
        case CAP_AUTOSCAN:
        case CAP_CLEARBUFFERS:
        case CAP_MAXBATCHBUFFERS:
        case ICAP_BARCODEDETECTIONENABLED:
        case ICAP_SUPPORTEDBARCODETYPES:
        case ICAP_BARCODEMAXSEARCHPRIORITIES:
        case ICAP_BARCODESEARCHPRIORITIES:
        case ICAP_BARCODESEARCHMODE:
        case ICAP_BARCODEMAXRETRIES:
        case ICAP_BARCODETIMEOUT:
        case CAP_EXTENDEDCAPS:
        case CAP_SUPPORTEDCAPS:
        case ICAP_FILTER:
        case ICAP_GAMMA:
        case ICAP_PLANARCHUNKY:
        case ICAP_BITORDERCODES:
        case ICAP_CCITTKFACTOR:
        case ICAP_JPEGPIXELTYPE:
        /*case ICAP_JPEGQUALITY:*/
        case ICAP_PIXELFLAVORCODES:
        case ICAP_TIMEFILL:
        case CAP_DEVICEONLINE:
        case CAP_DEVICETIMEDATE:
        case CAP_SERIALNUMBER:
        case ICAP_EXPOSURETIME:
        case ICAP_FLASHUSED2:
        case ICAP_IMAGEFILTER:
        case ICAP_LAMPSTATE:
        case ICAP_LIGHTPATH:
        case ICAP_NOISEFILTER:
        case ICAP_OVERSCAN:
        case ICAP_PHYSICALHEIGHT:
        case ICAP_PHYSICALWIDTH:
        case ICAP_ZOOMFACTOR:
        case CAP_PRINTER:
        case CAP_PRINTERENABLED:
        case CAP_PRINTERINDEX:
        case CAP_PRINTERMODE:
        case CAP_PRINTERSTRING:
        case CAP_PRINTERSUFFIX:
        case CAP_AUTHOR:
        case CAP_CAPTION:
        case CAP_TIMEDATE:
        case ICAP_AUTOBRIGHT:
        case ICAP_BRIGHTNESS:
        case ICAP_CONTRAST:
        case ICAP_HIGHLIGHT:
        case ICAP_ORIENTATION:
        case ICAP_ROTATION:
        case ICAP_SHADOW:
        case ICAP_XSCALING:
        case ICAP_YSCALING:
        case ICAP_BITDEPTHREDUCTION:
        case ICAP_BITORDER:
        case ICAP_CUSTHALFTONE:
        case ICAP_HALFTONES:
        case ICAP_THRESHOLD:
        case CAP_LANGUAGE:
        case ICAP_FRAMES:
        case ICAP_MAXFRAMES:
        case ICAP_SUPPORTEDSIZES:
        case CAP_AUTOFEED:
        case CAP_CLEARPAGE:
        case CAP_FEEDERALIGNMENT:
        case CAP_FEEDERENABLED:
        case CAP_FEEDERLOADED:
        case CAP_FEEDERORDER:
        case CAP_FEEDPAGE:
        case CAP_PAPERBINDING:
        case CAP_PAPERDETECTABLE:
        case CAP_REACQUIREALLOWED:
        case CAP_REWINDPAGE:
        case ICAP_PATCHCODEDETECTIONENABLED:
        case ICAP_SUPPORTEDPATCHCODETYPES:
        case ICAP_PATCHCODEMAXSEARCHPRIORITIES:
        case ICAP_PATCHCODESEARCHPRIORITIES:
        case ICAP_PATCHCODESEARCHMODE:
        case ICAP_PATCHCODEMAXRETRIES:
        case ICAP_PATCHCODETIMEOUT:
        case CAP_BATTERYMINUTES:
        case CAP_BATTERYPERCENTAGE:
        case CAP_POWERDOWNTIME:
        case CAP_POWERSUPPLY:
        case ICAP_XNATIVERESOLUTION:
        case ICAP_XRESOLUTION:
        case ICAP_YNATIVERESOLUTION:
        case ICAP_YRESOLUTION:
            twCC = TWCC_CAPUNSUPPORTED;
            break;
        case CAP_XFERCOUNT:
            /* This is a required capability that every source need to
               support but we haven't implemented yet. */
            twCC = TWCC_SUCCESS;
            break;
        /*case ICAP_COMPRESSION:*/
        case ICAP_IMAGEFILEFORMAT:
        case ICAP_TILES:
            twCC = TWCC_CAPUNSUPPORTED;
            break;
        case ICAP_XFERMECH:
            twCC = GPHOTO2_ICAPXferMech (pCapability, action);
            break;
        case ICAP_PIXELTYPE:
            twCC = GPHOTO2_ICAPPixelType (pCapability, action);
	    break;
        case ICAP_PIXELFLAVOR:
            twCC = GPHOTO2_ICAPPixelFlavor (pCapability, action);
	    break;
        case ICAP_BITDEPTH:
            twCC = GPHOTO2_ICAPBitDepth (pCapability, action);
	    break;
        case ICAP_UNITS:
            twCC = GPHOTO2_ICAPUnits (pCapability, action);
	    break;
        case ICAP_UNDEFINEDIMAGESIZE:
        case CAP_CAMERAPREVIEWUI:
        case CAP_ENABLEDSUIONLY:
        case CAP_INDICATORS:
        case CAP_UICONTROLLABLE:
            twCC = TWCC_CAPUNSUPPORTED;
            break;

        case ICAP_COMPRESSION:
            twCC = TWCC_SUCCESS;
	    break;
        default:
            twCC = TWRC_FAILURE;
	    break;
    }
    return twCC;
}

static TW_BOOL GPHOTO2_OneValueSet32 (pTW_CAPABILITY pCapability, TW_UINT32 value)
{
    pCapability->hContainer = GlobalAlloc (0, sizeof(TW_ONEVALUE));

    TRACE("-> %ld\n", value);

    if (pCapability->hContainer)
    {
        pTW_ONEVALUE pVal = GlobalLock (pCapability->hContainer);
        pVal->ItemType = TWTY_UINT32;
        pVal->Item = value;
        GlobalUnlock (pCapability->hContainer);
        pCapability->ConType = TWON_ONEVALUE;
        return TRUE;
    }
    else
        return FALSE;
}

static TW_BOOL GPHOTO2_OneValueSet16 (pTW_CAPABILITY pCapability, TW_UINT16 value)
{
    pCapability->hContainer = GlobalAlloc (0, sizeof(TW_ONEVALUE));

    TRACE("-> %d\n", value);

    if (pCapability->hContainer)
    {
        pTW_ONEVALUE pVal = GlobalLock (pCapability->hContainer);
        pVal->ItemType = TWTY_UINT16;
        pVal->Item = value;
        GlobalUnlock (pCapability->hContainer);
        pCapability->ConType = TWON_ONEVALUE;
        return TRUE;
    }
    else
        return FALSE;
}

static TW_BOOL GPHOTO2_EnumSet16 (pTW_CAPABILITY pCapability, int nrofvalues,
                                  const TW_UINT16 *values, int current, int def)
{
    pTW_ENUMERATION pVal;
    pCapability->hContainer = GlobalAlloc (0, sizeof(TW_ENUMERATION) + nrofvalues * sizeof(TW_UINT16));

    if (!pCapability->hContainer)
	return FALSE;

    pVal = GlobalLock (pCapability->hContainer);
    pVal->ItemType = TWTY_UINT16;
    pVal->NumItems = nrofvalues;
    memcpy(pVal->ItemList, values, sizeof(TW_UINT16)*nrofvalues);
    pVal->CurrentIndex = current;
    pVal->DefaultIndex = def;
    pCapability->ConType = TWON_ENUMERATION;
    GlobalUnlock (pCapability->hContainer);
    return TRUE;
}

static TW_BOOL GPHOTO2_EnumGet16 (pTW_CAPABILITY pCapability, int *nrofvalues, TW_UINT16 **values)
{
    pTW_ENUMERATION pVal = GlobalLock (pCapability->hContainer);

    if (!pVal)
	return FALSE;
    *nrofvalues = pVal->NumItems;
    *values = HeapAlloc( GetProcessHeap(), 0, sizeof(TW_UINT16)*pVal->NumItems);
    memcpy (*values, pVal->ItemList, sizeof(TW_UINT16)*(*nrofvalues));
    FIXME("Current Index %ld, Default Index %ld\n", pVal->CurrentIndex, pVal->DefaultIndex);
    GlobalUnlock (pCapability->hContainer);
    return TRUE;
}

static TW_BOOL GPHOTO2_OneValueGet32 (pTW_CAPABILITY pCapability, TW_UINT32 *pValue)
{
    pTW_ONEVALUE pVal = GlobalLock (pCapability->hContainer);

    if (pVal)
    {
        *pValue = pVal->Item;
        GlobalUnlock (pCapability->hContainer);
        return TRUE;
    }
    else
        return FALSE;
}

static TW_BOOL GPHOTO2_OneValueGet16 (pTW_CAPABILITY pCapability, TW_UINT16 *pValue)
{
    pTW_ONEVALUE pVal = GlobalLock (pCapability->hContainer);

    if (pVal)
    {
        *pValue = pVal->Item;
        GlobalUnlock (pCapability->hContainer);
        return TRUE;
    }
    else
        return FALSE;
}

/* ICAP_XFERMECH */
static TW_UINT16 GPHOTO2_ICAPXferMech (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TRACE("ICAP_XFERMECH, action %d\n", action);

    switch (action)
    {
        case MSG_GET:
            if (!GPHOTO2_OneValueSet32 (pCapability, activeDS.capXferMech))
                return TWCC_LOWMEMORY;
	    return TWCC_SUCCESS;
        case MSG_SET:
            if (pCapability->ConType == TWON_ONEVALUE)
            {
		TW_UINT32 xfermechtemp = 0;

                if (!GPHOTO2_OneValueGet32 (pCapability, &xfermechtemp))
                    return TWCC_LOWMEMORY;
		activeDS.capXferMech = xfermechtemp;
                TRACE("xfermech is %ld\n", xfermechtemp);
		return TWCC_SUCCESS;
            }
            else if (pCapability->ConType == TWON_ENUMERATION)
            {

            }
	    FIXME("GET FAILED\n");
            break;
        case MSG_GETCURRENT:
            if (!GPHOTO2_OneValueSet32 (pCapability, activeDS.capXferMech))
                return TWCC_LOWMEMORY;
            break;
        case MSG_GETDEFAULT:
            if (!GPHOTO2_OneValueSet32 (pCapability, TWSX_NATIVE))
                return TWCC_LOWMEMORY;
            break;
        case MSG_RESET:
            activeDS.capXferMech = TWSX_NATIVE;
            break;
    }
    return TWCC_SUCCESS;
}

/* ICAP_PIXELTYPE */
static TW_UINT16 GPHOTO2_ICAPPixelType (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TRACE("Action %d\n", action);

    switch (action)
    {
        case MSG_GET:
            if ((pCapability->ConType == TWON_DONTCARE16) ||
		(pCapability->ConType == TWON_ONEVALUE)
	    ) {
                if (!GPHOTO2_OneValueSet16 (pCapability, activeDS.pixeltype))
                    return TWCC_LOWMEMORY;
    	        return TWCC_SUCCESS;
	    }
	    FIXME("Unknown container type %x in MSG_GET\n", pCapability->ConType);
	    return TWCC_CAPBADOPERATION;
        case MSG_SET:
            if (pCapability->ConType == TWON_ONEVALUE)
            {
		TW_UINT16 pixeltype = 0;

                if (!GPHOTO2_OneValueGet16 (pCapability, &pixeltype))
                    return TWCC_LOWMEMORY;
		activeDS.pixeltype = pixeltype;
		FIXME("pixeltype changed to %d!\n", pixeltype);
    		return TWCC_SUCCESS;
            }
	    FIXME("set not done\n");
            if (pCapability->ConType == TWON_ENUMERATION) {
		TW_UINT16	*values = NULL;
		int i, nrofvalues = 0;
		
		if (!GPHOTO2_EnumGet16 (pCapability, &nrofvalues, &values))
		    return TWCC_LOWMEMORY;
		for (i=0;i<nrofvalues;i++)
		    FIXME("SET PixelType %d:%d\n", i, values[i]);
		HeapFree (GetProcessHeap(), 0, values);
	    }
            break;
        case MSG_GETCURRENT:
            if (!GPHOTO2_OneValueSet16 (pCapability, activeDS.pixeltype)) {
	    	FIXME("Failed one value set in GETCURRENT, contype %d!\n", pCapability->ConType);
                return TWCC_LOWMEMORY;
	    }
            break;
        case MSG_GETDEFAULT:
            if (!GPHOTO2_OneValueSet16 (pCapability, TWPT_RGB)) {
	    	FIXME("Failed onevalue set in GETDEFAULT!\n");
                return TWCC_LOWMEMORY;
	    }
            break;
        case MSG_RESET:
            activeDS.pixeltype = TWPT_RGB;
            break;
    }
    return TWCC_SUCCESS;
}

/* ICAP_PIXELFLAVOR */
static TW_UINT16 GPHOTO2_ICAPPixelFlavor (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TRACE("Action %d\n", action);

    switch (action)
    {
        case MSG_GET:
            if ((pCapability->ConType == TWON_DONTCARE16)	||
		(pCapability->ConType == TWON_ONEVALUE)
	    ) {
		if (!GPHOTO2_OneValueSet16 (pCapability, TWPF_CHOCOLATE))
		    return TWCC_LOWMEMORY;
		return TWCC_SUCCESS;
	    }
	    if (!pCapability->ConType) {
		TW_UINT16 arr[2];
		arr[0] = TWPF_CHOCOLATE;
		arr[1] = TWPF_VANILLA;

		if (!GPHOTO2_EnumSet16 (pCapability, 2, arr, 1, 1))
                    return TWCC_LOWMEMORY;
		return TWCC_SUCCESS;
	    }
	    FIXME("MSG_GET container type %x unhandled\n", pCapability->ConType);
            return TWCC_BADVALUE;
        case MSG_SET:
            if (pCapability->ConType == TWON_ONEVALUE)
            {
		TW_UINT16 pixelflavor = 0;

                if (!GPHOTO2_OneValueGet16 (pCapability, &pixelflavor))
                    return TWCC_LOWMEMORY;
		activeDS.pixelflavor = pixelflavor;
		FIXME("pixelflavor is %d\n", pixelflavor);
            }
            break;
        case MSG_GETCURRENT:
            if (!GPHOTO2_OneValueSet16 (pCapability, activeDS.pixelflavor))
                return TWCC_LOWMEMORY;
            break;
        case MSG_GETDEFAULT:
            if (!GPHOTO2_OneValueSet16 (pCapability, TWPF_CHOCOLATE))
                return TWCC_LOWMEMORY;
            break;
        case MSG_RESET:
            break;
    }
    return TWCC_SUCCESS;
}

/* ICAP_BITDEPTH */
static TW_UINT16 GPHOTO2_ICAPBitDepth (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TRACE("Action %d\n", action);

    switch (action)
    {
        case MSG_GET:
            if ((pCapability->ConType == TWON_DONTCARE16) ||
		(pCapability->ConType == TWON_ONEVALUE)
	    ) {
		if (!GPHOTO2_OneValueSet16 (pCapability, 24))
		    return TWCC_LOWMEMORY;
		return TWCC_SUCCESS;
	    }
	    FIXME("MSG_GET container type %x unhandled\n", pCapability->ConType);
            return TWCC_SUCCESS;
        case MSG_SET:
            if (pCapability->ConType == TWON_ONEVALUE) {
		TW_UINT16 bitdepth = 0;

                if (!GPHOTO2_OneValueGet16 (pCapability, &bitdepth))
                    return TWCC_LOWMEMORY;
		if (bitdepth != 24)
		    return TWCC_BADVALUE;
		return TWCC_SUCCESS;
            }
            if (pCapability->ConType == TWON_ENUMERATION)
	    {
		int i, nrofvalues = 0;
		TW_UINT16 *values = NULL;

                if (!GPHOTO2_EnumGet16 (pCapability, &nrofvalues, &values))
                    return TWCC_LOWMEMORY;
		for (i=0;i<nrofvalues;i++)
		    FIXME("SET: enum element %d = %d\n", i, values[i]);
		HeapFree (GetProcessHeap(), 0, values);
		return TWCC_SUCCESS;
	    }
	    FIXME("Unhandled container type %d in MSG_SET\n", pCapability->ConType);
            break;
        case MSG_GETCURRENT:
            if (!GPHOTO2_OneValueSet16 (pCapability, 24))
                return TWCC_LOWMEMORY;
            break;
        case MSG_GETDEFAULT:
            if (!GPHOTO2_OneValueSet16 (pCapability, 24))
                return TWCC_LOWMEMORY;
            break;
        case MSG_RESET:
            break;
    }
    return TWCC_SUCCESS;
}

/* ICAP_UNITS */
static TW_UINT16 GPHOTO2_ICAPUnits (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TRACE("Action %d\n", action);

    switch (action)
    {
        case MSG_GET:
            if ((pCapability->ConType == TWON_DONTCARE16) ||
		(pCapability->ConType == TWON_ONEVALUE)
	    ) {
		if (!GPHOTO2_OneValueSet16 (pCapability, TWUN_PIXELS))
		    return TWCC_LOWMEMORY;
		return TWCC_SUCCESS;
	    }
	    FIXME("MSG_GET container type %x unhandled\n", pCapability->ConType);
            return TWCC_SUCCESS;
        case MSG_SET:
            if (pCapability->ConType == TWON_ONEVALUE) {
		TW_UINT16 units = 0;

                if (!GPHOTO2_OneValueGet16 (pCapability, &units))
                    return TWCC_LOWMEMORY;
		FIXME("SET to type %d, stub.\n", units);
		return TWCC_SUCCESS;
            }
            if (pCapability->ConType == TWON_ENUMERATION)
	    {
		int i, nrofvalues = 0;
		TW_UINT16 *values = NULL;

                if (!GPHOTO2_EnumGet16 (pCapability, &nrofvalues, &values))
                    return TWCC_LOWMEMORY;
		for (i=0;i<nrofvalues;i++)
		    FIXME("SET: enum element %d = %d\n", i, values[i]);
		HeapFree (GetProcessHeap(), 0, values);
		return TWCC_SUCCESS;
	    }
	    FIXME("Unhandled container type %d in MSG_SET\n", pCapability->ConType);
            break;
        case MSG_GETCURRENT:
            if (!GPHOTO2_OneValueSet16 (pCapability, TWUN_INCHES))
                return TWCC_LOWMEMORY;
            break;
        case MSG_GETDEFAULT:
            if (!GPHOTO2_OneValueSet16 (pCapability, TWUN_PIXELS))
                return TWCC_LOWMEMORY;
            break;
        case MSG_RESET:
            break;
    }
    return TWCC_SUCCESS;
}
