/*
 * Copyright 2000 Corel Corporation
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "twain.h"
#include "sane_i.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

static TW_UINT16 SANE_ICAPXferMech (pTW_CAPABILITY pCapability, TW_UINT16 action);
static TW_UINT16 TWAIN_GetSupportedCaps(pTW_CAPABILITY pCapability);

TW_UINT16 SANE_SaneCapability (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT16 twCC = TWCC_SUCCESS;

    TRACE("capability=%d action=%d\n", pCapability->Cap, action);

    switch (pCapability->Cap)
    {
        case CAP_SUPPORTEDCAPS:
            if (action == MSG_GET)
                twCC = TWAIN_GetSupportedCaps(pCapability);
            else
                twCC = TWCC_BADVALUE;
            break;

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
        case ICAP_FILTER:
        case ICAP_GAMMA:
        case ICAP_PLANARCHUNKY:
        case ICAP_BITORDERCODES:
        case ICAP_CCITTKFACTOR:
        case ICAP_COMPRESSION:
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
        case ICAP_UNITS:
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
        case ICAP_BITDEPTH:
        case ICAP_BITDEPTHREDUCTION:
        case ICAP_BITORDER:
        case ICAP_CUSTHALFTONE:
        case ICAP_HALFTONES:
        case ICAP_PIXELFLAVOR:
        case ICAP_PIXELTYPE:
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
            /* This is a required capability that every source needs to
               support but we haven't implemented it yet. */
            twCC = TWCC_SUCCESS;
            break;
        /*case ICAP_COMPRESSION:*/
        case ICAP_IMAGEFILEFORMAT:
        case ICAP_TILES:
            twCC = TWCC_CAPUNSUPPORTED;
            break;
        case ICAP_XFERMECH:
            twCC = SANE_ICAPXferMech (pCapability, action);
            break;
        case ICAP_UNDEFINEDIMAGESIZE:
        case CAP_CAMERAPREVIEWUI:
        case CAP_ENABLEDSUIONLY:
        case CAP_INDICATORS:
        case CAP_UICONTROLLABLE:
            twCC = TWCC_CAPUNSUPPORTED;
            break;
        default:
            twCC = TWRC_FAILURE;

    }

    return twCC;
}


static TW_UINT16 get_onevalue(pTW_CAPABILITY pCapability, TW_UINT16 *type, TW_UINT32 *value)
{
    if (pCapability->hContainer)
    {
        pTW_ONEVALUE pVal = GlobalLock (pCapability->hContainer);
        if (pVal)
        {
            *value = pVal->Item;
            if (type)
                *type = pVal->ItemType;
            GlobalUnlock (pCapability->hContainer);
            return TWCC_SUCCESS;
        }
    }
    return TWCC_BUMMER;
}


static TW_UINT16 set_onevalue(pTW_CAPABILITY pCapability, TW_UINT16 type, TW_UINT32 value)
{
    pCapability->hContainer = GlobalAlloc (0, sizeof(TW_ONEVALUE));

    if (pCapability->hContainer)
    {
        pTW_ONEVALUE pVal = GlobalLock (pCapability->hContainer);
        if (pVal)
        {
            pCapability->ConType = TWON_ONEVALUE;
            pVal->ItemType = type;
            pVal->Item = value;
            GlobalUnlock (pCapability->hContainer);
            return TWCC_SUCCESS;
        }
    }
   return TWCC_LOWMEMORY;
}

static TW_UINT16 msg_set(pTW_CAPABILITY pCapability, TW_UINT32 *val)
{
    if (pCapability->ConType == TWON_ONEVALUE)
        return get_onevalue(pCapability, NULL, val);

    FIXME("Partial Stub:  MSG_SET only supports TW_ONEVALUE\n");
    return TWCC_BADCAP;
}


static TW_UINT16 msg_get_enum(pTW_CAPABILITY pCapability, const TW_UINT32 *values, int value_count,
                              TW_UINT16 type, TW_UINT32 current, TW_UINT32 default_value)
{
    TW_ENUMERATION *enumv = NULL;
    TW_UINT32 *p32;
    TW_UINT16 *p16;
    int i;

    pCapability->ConType = TWON_ENUMERATION;
    pCapability->hContainer = 0;

    if (type == TWTY_INT16 || type == TWTY_UINT16)
        pCapability->hContainer = GlobalAlloc (0, FIELD_OFFSET( TW_ENUMERATION, ItemList[value_count * sizeof(TW_UINT16)]));

    if (type == TWTY_INT32 || type == TWTY_UINT32)
        pCapability->hContainer = GlobalAlloc (0, FIELD_OFFSET( TW_ENUMERATION, ItemList[value_count * sizeof(TW_UINT32)]));

    if (pCapability->hContainer)
        enumv = GlobalLock(pCapability->hContainer);

    if (! enumv)
        return TWCC_LOWMEMORY;

    enumv->ItemType = type;
    enumv->NumItems = value_count;

    p16 = (TW_UINT16 *) enumv->ItemList;
    p32 = (TW_UINT32 *) enumv->ItemList;
    for (i = 0; i < value_count; i++)
    {
        if (values[i] == current)
            enumv->CurrentIndex = i;
        if (values[i] == default_value)
            enumv->DefaultIndex = i;
        if (type == TWTY_INT16 || type == TWTY_UINT16)
            p16[i] = values[i];
        if (type == TWTY_INT32 || type == TWTY_UINT32)
            p32[i] = values[i];
    }

    GlobalUnlock(pCapability->hContainer);
    return TWCC_SUCCESS;
}

static TW_UINT16 TWAIN_GetSupportedCaps(pTW_CAPABILITY pCapability)
{
    TW_ARRAY *a;
    static const UINT16 supported_caps[] = { CAP_SUPPORTEDCAPS, ICAP_XFERMECH };

    pCapability->hContainer = GlobalAlloc (0, FIELD_OFFSET( TW_ARRAY, ItemList[sizeof(supported_caps)] ));
    pCapability->ConType = TWON_ARRAY;

    if (pCapability->hContainer)
    {
        UINT16 *u;
        int i;
        a = GlobalLock (pCapability->hContainer);
        a->ItemType = TWTY_UINT16;
        a->NumItems = sizeof(supported_caps) / sizeof(supported_caps[0]);
        u = (UINT16 *) a->ItemList;
        for (i = 0; i < a->NumItems; i++)
            u[i] = supported_caps[i];
        GlobalUnlock (pCapability->hContainer);
        return TWCC_SUCCESS;
    }
    else
        return TWCC_LOWMEMORY;
}


/* ICAP_XFERMECH */
static TW_UINT16 SANE_ICAPXferMech (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    static const TW_UINT32 possible_values[] = { TWSX_NATIVE, TWSX_MEMORY };
    TW_UINT32 val;
    TW_UINT16 twCC = TWCC_BADCAP;

    TRACE("ICAP_XFERMECH\n");

    switch (action)
    {
        case MSG_QUERYSUPPORT:
            twCC = set_onevalue(pCapability, TWTY_INT32,
                    TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET );
            break;

        case MSG_GET:
            twCC = msg_get_enum(pCapability, possible_values, sizeof(possible_values) / sizeof(possible_values[0]),
                    TWTY_UINT16, activeDS.capXferMech, TWSX_NATIVE);
            break;

        case MSG_SET:
            twCC = msg_set(pCapability, &val);
            if (twCC == TWCC_SUCCESS)
            {
               activeDS.capXferMech = (TW_UINT16) val;
               FIXME("Partial Stub:  XFERMECH set to %d, but ignored\n", val);
            }
            break;

        case MSG_GETDEFAULT:
            twCC = set_onevalue(pCapability, TWTY_UINT16, TWSX_NATIVE);
            break;

        case MSG_RESET:
            activeDS.capXferMech = TWSX_NATIVE;
            /* .. fall through intentional .. */

        case MSG_GETCURRENT:
            twCC = set_onevalue(pCapability, TWTY_UINT16, activeDS.capXferMech);
            FIXME("Partial Stub:  XFERMECH of %d not actually used\n", activeDS.capXferMech);
            break;
    }
    return twCC;
}
