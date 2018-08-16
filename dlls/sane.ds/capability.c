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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <math.h>

#include "sane_i.h"
#include "winnls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

#ifndef SANE_VALUE_SCAN_MODE_COLOR
#define SANE_VALUE_SCAN_MODE_COLOR		SANE_I18N("Color")
#endif
#ifndef SANE_VALUE_SCAN_MODE_GRAY
#define SANE_VALUE_SCAN_MODE_GRAY		SANE_I18N("Gray")
#endif
#ifndef SANE_VALUE_SCAN_MODE_LINEART
#define SANE_VALUE_SCAN_MODE_LINEART		SANE_I18N("Lineart")
#endif

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

#ifdef SONAME_LIBSANE
static TW_UINT16 msg_get_range(pTW_CAPABILITY pCapability, TW_UINT16 type,
            TW_UINT32 minval, TW_UINT32 maxval, TW_UINT32 step, TW_UINT32 def,  TW_UINT32 current)
{
    TW_RANGE *range = NULL;

    pCapability->ConType = TWON_RANGE;
    pCapability->hContainer = 0;

    pCapability->hContainer = GlobalAlloc (0, sizeof(*range));
    if (pCapability->hContainer)
        range = GlobalLock(pCapability->hContainer);

    if (! range)
        return TWCC_LOWMEMORY;

    range->ItemType = type;
    range->MinValue = minval;
    range->MaxValue = maxval;
    range->StepSize = step;
    range->DefaultValue = def;
    range->CurrentValue = current;

    GlobalUnlock(pCapability->hContainer);
    return TWCC_SUCCESS;
}
#endif

static TW_UINT16 TWAIN_GetSupportedCaps(pTW_CAPABILITY pCapability)
{
    TW_ARRAY *a;
    static const UINT16 supported_caps[] = { CAP_SUPPORTEDCAPS, CAP_XFERCOUNT, CAP_UICONTROLLABLE,
                    CAP_AUTOFEED, CAP_FEEDERENABLED,
                    ICAP_XFERMECH, ICAP_PIXELTYPE, ICAP_UNITS, ICAP_BITDEPTH, ICAP_COMPRESSION, ICAP_PIXELFLAVOR,
                    ICAP_XRESOLUTION, ICAP_YRESOLUTION, ICAP_PHYSICALHEIGHT, ICAP_PHYSICALWIDTH, ICAP_SUPPORTEDSIZES };

    pCapability->hContainer = GlobalAlloc (0, FIELD_OFFSET( TW_ARRAY, ItemList[sizeof(supported_caps)] ));
    pCapability->ConType = TWON_ARRAY;

    if (pCapability->hContainer)
    {
        UINT16 *u;
        TW_UINT32 i;
        a = GlobalLock (pCapability->hContainer);
        a->ItemType = TWTY_UINT16;
        a->NumItems = ARRAY_SIZE(supported_caps);
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
            twCC = msg_get_enum(pCapability, possible_values, ARRAY_SIZE(possible_values),
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


/* CAP_XFERCOUNT */
static TW_UINT16 SANE_CAPXferCount (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT32 val;
    TW_UINT16 twCC = TWCC_BADCAP;

    TRACE("CAP_XFERCOUNT\n");

    switch (action)
    {
        case MSG_QUERYSUPPORT:
            twCC = set_onevalue(pCapability, TWTY_INT32,
                    TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET );
            break;

        case MSG_GET:
            twCC = set_onevalue(pCapability, TWTY_INT16, -1);
            FIXME("Partial Stub:  Reporting only support for transfer all\n");
            break;

        case MSG_SET:
            twCC = msg_set(pCapability, &val);
            if (twCC == TWCC_SUCCESS)
               FIXME("Partial Stub:  XFERCOUNT set to %d, but ignored\n", val);
            break;

        case MSG_GETDEFAULT:
            twCC = set_onevalue(pCapability, TWTY_INT16, -1);
            break;

        case MSG_RESET:
            /* .. fall through intentional .. */

        case MSG_GETCURRENT:
            twCC = set_onevalue(pCapability, TWTY_INT16, -1);
            break;
    }
    return twCC;
}

#ifdef SONAME_LIBSANE
static BOOL pixeltype_to_sane_mode(TW_UINT16 pixeltype, SANE_String mode, int len)
{
    SANE_String_Const m = NULL;
    switch (pixeltype)
    {
        case TWPT_GRAY:
            m = SANE_VALUE_SCAN_MODE_GRAY;
            break;
        case TWPT_RGB:
            m = SANE_VALUE_SCAN_MODE_COLOR;
            break;
        case TWPT_BW:
            m = SANE_VALUE_SCAN_MODE_LINEART;
            break;
    }
    if (! m)
        return FALSE;
    if (strlen(m) >= len)
        return FALSE;
    strcpy(mode, m);
    return TRUE;
}
static BOOL sane_mode_to_pixeltype(SANE_String_Const mode, TW_UINT16 *pixeltype)
{
    if (strcmp(mode, SANE_VALUE_SCAN_MODE_LINEART) == 0)
        *pixeltype = TWPT_BW;
    else if (memcmp(mode, SANE_VALUE_SCAN_MODE_GRAY, strlen(SANE_VALUE_SCAN_MODE_GRAY)) == 0)
        *pixeltype = TWPT_GRAY;
    else if (strcmp(mode, SANE_VALUE_SCAN_MODE_COLOR) == 0)
        *pixeltype = TWPT_RGB;
    else
        return FALSE;

    return TRUE;
}
#endif

/* ICAP_PIXELTYPE */
static TW_UINT16 SANE_ICAPPixelType (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT16 twCC = TWCC_BADCAP;
#ifdef SONAME_LIBSANE
    TW_UINT32 possible_values[3];
    int possible_value_count;
    TW_UINT32 val;
    SANE_Status rc;
    SANE_Int status;
    SANE_String_Const *choices;
    char current_mode[64];
    TW_UINT16 current_pixeltype = TWPT_BW;
    SANE_Char mode[64];

    TRACE("ICAP_PIXELTYPE\n");

    rc = sane_option_probe_mode(activeDS.deviceHandle, &choices, current_mode, sizeof(current_mode));
    if (rc != SANE_STATUS_GOOD)
    {
        ERR("Unable to retrieve mode from sane, ICAP_PIXELTYPE unsupported\n");
        return twCC;
    }

    sane_mode_to_pixeltype(current_mode, &current_pixeltype);

    /* Sane does not support a concept of a default mode, so we simply cache
     *   the first mode we find */
    if (! activeDS.PixelTypeSet)
    {
        activeDS.PixelTypeSet = TRUE;
        activeDS.defaultPixelType = current_pixeltype;
    }

    switch (action)
    {
        case MSG_QUERYSUPPORT:
            twCC = set_onevalue(pCapability, TWTY_INT32,
                    TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET );
            break;

        case MSG_GET:
            for (possible_value_count = 0; choices && *choices && possible_value_count < 3; choices++)
            {
                TW_UINT16 pix;
                if (sane_mode_to_pixeltype(*choices, &pix))
                    possible_values[possible_value_count++] = pix;
            }
            twCC = msg_get_enum(pCapability, possible_values, possible_value_count,
                    TWTY_UINT16, current_pixeltype, activeDS.defaultPixelType);
            break;

        case MSG_SET:
            twCC = msg_set(pCapability, &val);
            if (twCC == TWCC_SUCCESS)
            {
                TRACE("Setting pixeltype to %d\n", val);
                if (! pixeltype_to_sane_mode(val, mode, sizeof(mode)))
                    return TWCC_BADVALUE;

                status = 0;
                rc = sane_option_set_str(activeDS.deviceHandle, "mode", mode, &status);
                /* Some SANE devices use 'Grayscale' instead of the standard 'Gray' */
                if (rc == SANE_STATUS_INVAL && strcmp(mode, SANE_VALUE_SCAN_MODE_GRAY) == 0)
                {
                    strcpy(mode, "Grayscale");
                    rc = sane_option_set_str(activeDS.deviceHandle, "mode", mode, &status);
                }
                if (rc != SANE_STATUS_GOOD)
                    return sane_status_to_twcc(rc);
                if (status & SANE_INFO_RELOAD_PARAMS)
                    psane_get_parameters (activeDS.deviceHandle, &activeDS.sane_param);
            }
            break;

        case MSG_GETDEFAULT:
            twCC = set_onevalue(pCapability, TWTY_UINT16, activeDS.defaultPixelType);
            break;

        case MSG_RESET:
            current_pixeltype = activeDS.defaultPixelType;
            if (! pixeltype_to_sane_mode(current_pixeltype, mode, sizeof(mode)))
                return TWCC_BADVALUE;

            status = 0;
            rc = sane_option_set_str(activeDS.deviceHandle, "mode", mode, &status);
            /* Some SANE devices use 'Grayscale' instead of the standard 'Gray' */
            if (rc == SANE_STATUS_INVAL && strcmp(mode, SANE_VALUE_SCAN_MODE_GRAY) == 0)
            {
                strcpy(mode, "Grayscale");
                rc = sane_option_set_str(activeDS.deviceHandle, "mode", mode, &status);
            }
            if (rc != SANE_STATUS_GOOD)
                return sane_status_to_twcc(rc);
            if (status & SANE_INFO_RELOAD_PARAMS)
                psane_get_parameters (activeDS.deviceHandle, &activeDS.sane_param);

            /* .. fall through intentional .. */

        case MSG_GETCURRENT:
            twCC = set_onevalue(pCapability, TWTY_UINT16, current_pixeltype);
            TRACE("Returning current pixeltype of %d\n", current_pixeltype);
            break;
    }

#endif
    return twCC;
}

/* ICAP_UNITS */
static TW_UINT16 SANE_ICAPUnits (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT32 val;
    TW_UINT16 twCC = TWCC_BADCAP;

    TRACE("ICAP_UNITS\n");

    switch (action)
    {
        case MSG_QUERYSUPPORT:
            twCC = set_onevalue(pCapability, TWTY_INT32,
                    TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET );
            break;

        case MSG_GET:
            twCC = set_onevalue(pCapability, TWTY_UINT16, TWUN_INCHES);
            break;

        case MSG_SET:
            twCC = msg_set(pCapability, &val);
            if (twCC == TWCC_SUCCESS)
            {
                if  (val != TWUN_INCHES)
                {
                    ERR("Sane supports only SANE_UNIT_DPI\n");
                    twCC = TWCC_BADVALUE;
                }
            }
            break;

        case MSG_GETDEFAULT:
        case MSG_RESET:
            /* .. fall through intentional .. */

        case MSG_GETCURRENT:
            twCC = set_onevalue(pCapability, TWTY_UINT16, TWUN_INCHES);
            break;
    }

    return twCC;
}

/* ICAP_BITDEPTH */
static TW_UINT16 SANE_ICAPBitDepth(pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT16 twCC = TWCC_BADCAP;
#ifdef SONAME_LIBSANE
    TW_UINT32 possible_values[1];

    TRACE("ICAP_BITDEPTH\n");

    possible_values[0] = activeDS.sane_param.depth;

    switch (action)
    {
        case MSG_QUERYSUPPORT:
            twCC = set_onevalue(pCapability, TWTY_INT32,
                    TWQC_GET | TWQC_GETDEFAULT | TWQC_GETCURRENT  );
            break;

        case MSG_GET:
            twCC = msg_get_enum(pCapability, possible_values, ARRAY_SIZE(possible_values),
                    TWTY_UINT16, activeDS.sane_param.depth, activeDS.sane_param.depth);
            break;

        case MSG_GETDEFAULT:
            /* .. Fall through intentional .. */

        case MSG_GETCURRENT:
            TRACE("Returning current bitdepth of %d\n", activeDS.sane_param.depth);
            twCC = set_onevalue(pCapability, TWTY_UINT16, activeDS.sane_param.depth);
            break;
    }
#endif
    return twCC;
}

/* CAP_UICONTROLLABLE */
static TW_UINT16 SANE_CAPUiControllable(pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT16 twCC = TWCC_BADCAP;

    TRACE("CAP_UICONTROLLABLE\n");

    switch (action)
    {
        case MSG_QUERYSUPPORT:
            twCC = set_onevalue(pCapability, TWTY_INT32, TWQC_GET);
            break;

        case MSG_GET:
            twCC = set_onevalue(pCapability, TWTY_BOOL, TRUE);
            break;

    }
    return twCC;
}

/* ICAP_COMPRESSION */
static TW_UINT16 SANE_ICAPCompression (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    static const TW_UINT32 possible_values[] = { TWCP_NONE };
    TW_UINT32 val;
    TW_UINT16 twCC = TWCC_BADCAP;

    TRACE("ICAP_COMPRESSION\n");

    switch (action)
    {
        case MSG_QUERYSUPPORT:
            twCC = set_onevalue(pCapability, TWTY_INT32,
                    TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET );
            break;

        case MSG_GET:
            twCC = msg_get_enum(pCapability, possible_values, ARRAY_SIZE(possible_values),
                    TWTY_UINT16, TWCP_NONE, TWCP_NONE);
            FIXME("Partial stub:  We don't attempt to support compression\n");
            break;

        case MSG_SET:
            twCC = msg_set(pCapability, &val);
            if (twCC == TWCC_SUCCESS)
               FIXME("Partial Stub:  COMPRESSION set to %d, but ignored\n", val);
            break;

        case MSG_GETDEFAULT:
            twCC = set_onevalue(pCapability, TWTY_UINT16, TWCP_NONE);
            break;

        case MSG_RESET:
            /* .. fall through intentional .. */

        case MSG_GETCURRENT:
            twCC = set_onevalue(pCapability, TWTY_UINT16, TWCP_NONE);
            break;
    }
    return twCC;
}

/* ICAP_XRESOLUTION, ICAP_YRESOLUTION  */
static TW_UINT16 SANE_ICAPResolution (pTW_CAPABILITY pCapability, TW_UINT16 action,  TW_UINT16 cap)
{
    TW_UINT16 twCC = TWCC_BADCAP;
#ifdef SONAME_LIBSANE
    TW_UINT32 val;
    SANE_Int current_resolution;
    TW_FIX32 *default_res;
    const char *best_option_name;
    SANE_Int minval, maxval, quantval;
    SANE_Status sane_rc;
    SANE_Int set_status;

    TRACE("ICAP_%cRESOLUTION\n", cap == ICAP_XRESOLUTION ? 'X' : 'Y');

    /* Some scanners support 'x-resolution', most seem to just support 'resolution' */
    if (cap == ICAP_XRESOLUTION)
    {
        best_option_name = "x-resolution";
        default_res = &activeDS.defaultXResolution;
    }
    else
    {
        best_option_name = "y-resolution";
        default_res = &activeDS.defaultYResolution;
    }
    if (sane_option_get_int(activeDS.deviceHandle, best_option_name, &current_resolution) != SANE_STATUS_GOOD)
    {
        best_option_name = "resolution";
        if (sane_option_get_int(activeDS.deviceHandle, best_option_name, &current_resolution) != SANE_STATUS_GOOD)
            return TWCC_BADCAP;
    }

    /* Sane does not support a concept of 'default' resolution, so we have to
     *   cache the resolution the very first time we load the scanner, and use that
     *   as the default */
    if (cap == ICAP_XRESOLUTION && ! activeDS.XResolutionSet)
    {
        default_res->Whole = current_resolution;
        default_res->Frac = 0;
        activeDS.XResolutionSet = TRUE;
    }

    if (cap == ICAP_YRESOLUTION && ! activeDS.YResolutionSet)
    {
        default_res->Whole = current_resolution;
        default_res->Frac = 0;
        activeDS.YResolutionSet = TRUE;
    }

    switch (action)
    {
        case MSG_QUERYSUPPORT:
            twCC = set_onevalue(pCapability, TWTY_INT32,
                    TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET );
            break;

        case MSG_GET:
            sane_rc = sane_option_probe_resolution(activeDS.deviceHandle, best_option_name, &minval, &maxval, &quantval);
            if (sane_rc != SANE_STATUS_GOOD)
                twCC = TWCC_BADCAP;
            else
                twCC = msg_get_range(pCapability, TWTY_FIX32,
                                minval, maxval, quantval == 0 ? 1 : quantval, default_res->Whole, current_resolution);
            break;

        case MSG_SET:
            twCC = msg_set(pCapability, &val);
            if (twCC == TWCC_SUCCESS)
            {
                TW_FIX32 f32;
                memcpy(&f32, &val, sizeof(f32));
                sane_rc = sane_option_set_int(activeDS.deviceHandle, best_option_name, f32.Whole, &set_status);
                if (sane_rc != SANE_STATUS_GOOD)
                {
                    FIXME("Status of %d not expected or handled\n", sane_rc);
                    twCC = TWCC_BADCAP;
                }
                else if (set_status == SANE_INFO_INEXACT)
                    twCC = TWCC_CHECKSTATUS;
            }
            break;

        case MSG_GETDEFAULT:
            twCC = set_onevalue(pCapability, TWTY_FIX32, default_res->Whole);
            break;

        case MSG_RESET:
            sane_rc = sane_option_set_int(activeDS.deviceHandle, best_option_name, default_res->Whole, NULL);
            if (sane_rc != SANE_STATUS_GOOD)
                return TWCC_BADCAP;

            /* .. fall through intentional .. */

        case MSG_GETCURRENT:
            twCC = set_onevalue(pCapability, TWTY_FIX32, current_resolution);
            break;
    }
#endif
    return twCC;
}

/* ICAP_PHYSICALHEIGHT, ICAP_PHYSICALWIDTH */
static TW_UINT16 SANE_ICAPPhysical (pTW_CAPABILITY pCapability, TW_UINT16 action,  TW_UINT16 cap)
{
    TW_UINT16 twCC = TWCC_BADCAP;
#ifdef SONAME_LIBSANE
    TW_FIX32 res;
    char option_name[64];
    SANE_Fixed lower, upper;
    SANE_Unit lowerunit, upperunit;
    SANE_Status status;

    TRACE("ICAP_PHYSICAL%s\n", cap == ICAP_PHYSICALHEIGHT? "HEIGHT" : "WIDTH");

    sprintf(option_name, "tl-%c", cap == ICAP_PHYSICALHEIGHT ? 'y' : 'x');
    status = sane_option_probe_scan_area(activeDS.deviceHandle, option_name, NULL, &lowerunit, &lower, NULL, NULL);
    if (status != SANE_STATUS_GOOD)
        return sane_status_to_twcc(status);

    sprintf(option_name, "br-%c", cap == ICAP_PHYSICALHEIGHT ? 'y' : 'x');
    status = sane_option_probe_scan_area(activeDS.deviceHandle, option_name, NULL, &upperunit, NULL, &upper, NULL);
    if (status != SANE_STATUS_GOOD)
        return sane_status_to_twcc(status);

    if (upperunit != lowerunit)
        return TWCC_BADCAP;

    if (! convert_sane_res_to_twain(SANE_UNFIX(upper) - SANE_UNFIX(lower), upperunit, &res, TWUN_INCHES))
        return TWCC_BADCAP;

    switch (action)
    {
        case MSG_QUERYSUPPORT:
            twCC = set_onevalue(pCapability, TWTY_INT32,
                    TWQC_GET | TWQC_GETDEFAULT | TWQC_GETCURRENT );
            break;

        case MSG_GET:
        case MSG_GETDEFAULT:

            /* .. fall through intentional .. */

        case MSG_GETCURRENT:
            twCC = set_onevalue(pCapability, TWTY_FIX32, res.Whole | (res.Frac << 16));
            break;
    }
#endif
    return twCC;
}

/* ICAP_PIXELFLAVOR */
static TW_UINT16 SANE_ICAPPixelFlavor (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT16 twCC = TWCC_BADCAP;
#ifdef SONAME_LIBSANE
    static const TW_UINT32 possible_values[] = { TWPF_CHOCOLATE, TWPF_VANILLA };
    TW_UINT32 val;
    TW_UINT32 flavor = activeDS.sane_param.depth == 1 ? TWPF_VANILLA : TWPF_CHOCOLATE;

    TRACE("ICAP_PIXELFLAVOR\n");

    switch (action)
    {
        case MSG_QUERYSUPPORT:
            twCC = set_onevalue(pCapability, TWTY_INT32,
                    TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET );
            break;

        case MSG_GET:
            twCC = msg_get_enum(pCapability, possible_values, ARRAY_SIZE(possible_values),
                    TWTY_UINT16, flavor, flavor);
            break;

        case MSG_SET:
            twCC = msg_set(pCapability, &val);
            if (twCC == TWCC_SUCCESS)
            {
               FIXME("Stub:  PIXELFLAVOR set to %d, but ignored\n", val);
            }
            break;

        case MSG_GETDEFAULT:
            twCC = set_onevalue(pCapability, TWTY_UINT16, flavor);
            break;

        case MSG_RESET:
            /* .. fall through intentional .. */

        case MSG_GETCURRENT:
            twCC = set_onevalue(pCapability, TWTY_UINT16, flavor);
            break;
    }
#endif
    return twCC;
}

#ifdef SONAME_LIBSANE
static TW_UINT16 get_width_height(double *width, double *height, BOOL max)
{
    SANE_Status status;

    SANE_Fixed tlx_current, tlx_min, tlx_max;
    SANE_Fixed tly_current, tly_min, tly_max;
    SANE_Fixed brx_current, brx_min, brx_max;
    SANE_Fixed bry_current, bry_min, bry_max;

    status = sane_option_probe_scan_area(activeDS.deviceHandle, "tl-x", &tlx_current, NULL, &tlx_min, &tlx_max, NULL);
    if (status != SANE_STATUS_GOOD)
        return sane_status_to_twcc(status);

    status = sane_option_probe_scan_area(activeDS.deviceHandle, "tl-y", &tly_current, NULL, &tly_min, &tly_max, NULL);
    if (status != SANE_STATUS_GOOD)
        return sane_status_to_twcc(status);

    status = sane_option_probe_scan_area(activeDS.deviceHandle, "br-x", &brx_current, NULL, &brx_min, &brx_max, NULL);
    if (status != SANE_STATUS_GOOD)
        return sane_status_to_twcc(status);

    status = sane_option_probe_scan_area(activeDS.deviceHandle, "br-y", &bry_current, NULL, &bry_min, &bry_max, NULL);
    if (status != SANE_STATUS_GOOD)
        return sane_status_to_twcc(status);

    if (max)
        *width = SANE_UNFIX(brx_max) - SANE_UNFIX(tlx_min);
    else
        *width = SANE_UNFIX(brx_current) - SANE_UNFIX(tlx_current);

    if (max)
        *height = SANE_UNFIX(bry_max) - SANE_UNFIX(tly_min);
    else
        *height = SANE_UNFIX(bry_current) - SANE_UNFIX(tly_current);

    return(TWCC_SUCCESS);
}

static TW_UINT16 set_one_coord(const char *name, double coord)
{
    SANE_Status status;
    status = sane_option_set_fixed(activeDS.deviceHandle, name, SANE_FIX(coord), NULL);
    if (status != SANE_STATUS_GOOD)
        return sane_status_to_twcc(status);
    return TWCC_SUCCESS;
}

static TW_UINT16 set_width_height(double width, double height)
{
    TW_UINT16 rc = TWCC_SUCCESS;
    rc = set_one_coord("tl-x", 0);
    if (rc != TWCC_SUCCESS)
        return rc;
    rc = set_one_coord("br-x", width);
    if (rc != TWCC_SUCCESS)
        return rc;
    rc = set_one_coord("tl-y", 0);
    if (rc != TWCC_SUCCESS)
        return rc;
    rc = set_one_coord("br-y", height);

    return rc;
}

typedef struct
{
    TW_UINT32 size;
    double x;
    double y;
} supported_size_t;

static const supported_size_t supported_sizes[] =
{
    { TWSS_NONE,        0,      0       },
    { TWSS_A4,          210,    297     },
    { TWSS_JISB5,       182,    257     },
    { TWSS_USLETTER,    215.9,  279.4   },
    { TWSS_USLEGAL,     215.9,  355.6   },
    { TWSS_A5,          148,    210     },
    { TWSS_B4,          250,    353     },
    { TWSS_B6,          125,    176     },
    { TWSS_USLEDGER,    215.9,  431.8   },
    { TWSS_USEXECUTIVE, 184.15, 266.7   },
    { TWSS_A3,          297,    420     },
};

static TW_UINT16 get_default_paper_size(const supported_size_t *s, int n)
{
    DWORD paper;
    int rc;
    int defsize = -1;
    double width, height;
    int i;
    rc = GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IPAPERSIZE | LOCALE_RETURN_NUMBER, (void *) &paper, sizeof(paper));
    if (rc > 0)
        switch (paper)
        {
            case 1:
                defsize = TWSS_USLETTER;
                break;
            case 5:
                defsize = TWSS_USLEGAL;
                break;
            case 8:
                defsize = TWSS_A3;
                break;
            case 9:
                defsize = TWSS_A4;
                break;
        }

    if (defsize == -1)
        return TWSS_NONE;

    if (get_width_height(&width, &height, TRUE) != TWCC_SUCCESS)
        return TWSS_NONE;

    for (i = 0; i < n; i++)
        if (s[i].size == defsize)
        {
            /* Sane's use of integers to store floats is a hair lossy; deal with it */
            if (s[i].x > (width + .01) || s[i].y > (height + 0.01))
                return TWSS_NONE;
            else
                return s[i].size;
        }

    return TWSS_NONE;
}

static TW_UINT16 get_current_paper_size(const supported_size_t *s, int n)
{
    int i;
    double width, height;
    double xdelta, ydelta;

    if (get_width_height(&width, &height, FALSE) != TWCC_SUCCESS)
        return TWSS_NONE;

    for (i = 0; i < n; i++)
    {
        /* Sane's use of integers to store floats results
         * in a very small error; cope with that */
        xdelta = s[i].x - width;
        ydelta = s[i].y - height;
        if (xdelta < 0.01 && xdelta > -0.01 &&
            ydelta < 0.01 && ydelta > -0.01)
            return s[i].size;
    }

    return TWSS_NONE;
}
#endif

/* ICAP_SUPPORTEDSIZES */
static TW_UINT16 SANE_ICAPSupportedSizes (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT16 twCC = TWCC_BADCAP;
#ifdef SONAME_LIBSANE

    static TW_UINT32 possible_values[ARRAY_SIZE(supported_sizes)];
    unsigned int i;
    TW_UINT32 val;
    TW_UINT16 default_size = get_default_paper_size(supported_sizes, ARRAY_SIZE(supported_sizes));
    TW_UINT16 current_size = get_current_paper_size(supported_sizes, ARRAY_SIZE(supported_sizes));

    TRACE("ICAP_SUPPORTEDSIZES\n");

    switch (action)
    {
        case MSG_QUERYSUPPORT:
            twCC = set_onevalue(pCapability, TWTY_INT32,
                    TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET );
            break;

        case MSG_GET:
            for (i = 0; i < ARRAY_SIZE(supported_sizes); i++)
                possible_values[i] = supported_sizes[i].size;
            twCC = msg_get_enum(pCapability, possible_values, ARRAY_SIZE(possible_values),
                    TWTY_UINT16, current_size, default_size);
            WARN("Partial Stub:  our supported size selection is a bit thin.\n");
            break;

        case MSG_SET:
            twCC = msg_set(pCapability, &val);
            if (twCC == TWCC_SUCCESS)
                for (i = 1; i < ARRAY_SIZE(supported_sizes); i++)
                    if (supported_sizes[i].size == val)
                        return set_width_height(supported_sizes[i].x, supported_sizes[i].y);

            ERR("Unsupported size %d\n", val);
            twCC = TWCC_BADCAP;
            break;

        case MSG_GETDEFAULT:
            twCC = set_onevalue(pCapability, TWTY_UINT16, default_size);
            break;

        case MSG_RESET:
            twCC = TWCC_BADCAP;
            for (i = 1; i < ARRAY_SIZE(supported_sizes); i++)
                if (supported_sizes[i].size == default_size)
                {
                    twCC = set_width_height(supported_sizes[i].x, supported_sizes[i].y);
                    break;
                }
            if (twCC != TWCC_SUCCESS)
                return twCC;

            /* .. fall through intentional .. */

        case MSG_GETCURRENT:
            twCC = set_onevalue(pCapability, TWTY_UINT16, current_size);
            break;
    }

#endif
    return twCC;
}

/* CAP_AUTOFEED */
static TW_UINT16 SANE_CAPAutofeed (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT16 twCC = TWCC_BADCAP;
#ifdef SONAME_LIBSANE
    TW_UINT32 val;
    SANE_Bool autofeed;
    SANE_Status status;

    TRACE("CAP_AUTOFEED\n");

    if (sane_option_get_bool(activeDS.deviceHandle, "batch-scan", &autofeed, NULL) != SANE_STATUS_GOOD)
        return TWCC_BADCAP;

    switch (action)
    {
        case MSG_QUERYSUPPORT:
            twCC = set_onevalue(pCapability, TWTY_INT32,
                    TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET );
            break;

        case MSG_GET:
            twCC = set_onevalue(pCapability, TWTY_BOOL, autofeed);
            break;

        case MSG_SET:
            twCC = msg_set(pCapability, &val);
            if (twCC == TWCC_SUCCESS)
            {
                if (val)
                    autofeed = SANE_TRUE;
                else
                    autofeed = SANE_FALSE;

                status = sane_option_set_bool(activeDS.deviceHandle, "batch-scan", autofeed, NULL);
                if (status != SANE_STATUS_GOOD)
                {
                    ERR("Error %s: Could not set batch-scan to %d\n", psane_strstatus(status), autofeed);
                    return sane_status_to_twcc(status);
                }
            }
            break;

        case MSG_GETDEFAULT:
            twCC = set_onevalue(pCapability, TWTY_BOOL, SANE_TRUE);
            break;

        case MSG_RESET:
            autofeed = SANE_TRUE;
            status = sane_option_set_bool(activeDS.deviceHandle, "batch-scan", autofeed, NULL);
            if (status != SANE_STATUS_GOOD)
            {
                ERR("Error %s: Could not reset batch-scan to SANE_TRUE\n", psane_strstatus(status));
                return sane_status_to_twcc(status);
            }
            /* .. fall through intentional .. */

        case MSG_GETCURRENT:
            twCC = set_onevalue(pCapability, TWTY_BOOL, autofeed);
            break;
    }
#endif
    return twCC;
}

/* CAP_FEEDERENABLED */
static TW_UINT16 SANE_CAPFeederEnabled (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT16 twCC = TWCC_BADCAP;
#ifdef SONAME_LIBSANE
    TW_UINT32 val;
    TW_BOOL enabled;
    SANE_Status status;
    SANE_Char source[64];

    TRACE("CAP_FEEDERENABLED\n");

    if (sane_option_get_str(activeDS.deviceHandle, SANE_NAME_SCAN_SOURCE, source, sizeof(source), NULL) != SANE_STATUS_GOOD)
        return TWCC_BADCAP;

    if (strcmp(source, "Auto") == 0 || strcmp(source, "ADF") == 0)
        enabled = TRUE;
    else
        enabled = FALSE;

    switch (action)
    {
        case MSG_QUERYSUPPORT:
            twCC = set_onevalue(pCapability, TWTY_INT32,
                    TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET );
            break;

        case MSG_GET:
            twCC = set_onevalue(pCapability, TWTY_BOOL, enabled);
            break;

        case MSG_SET:
            twCC = msg_set(pCapability, &val);
            if (twCC == TWCC_SUCCESS)
            {
                strcpy(source, "ADF");
                status = sane_option_set_str(activeDS.deviceHandle, SANE_NAME_SCAN_SOURCE, source, NULL);
                if (status != SANE_STATUS_GOOD)
                {
                    strcpy(source, "Auto");
                    status = sane_option_set_str(activeDS.deviceHandle, SANE_NAME_SCAN_SOURCE, source, NULL);
                }
                if (status != SANE_STATUS_GOOD)
                {
                    ERR("Error %s: Could not set source to either ADF or Auto\n", psane_strstatus(status));
                    return sane_status_to_twcc(status);
                }
            }
            break;

        case MSG_GETDEFAULT:
            twCC = set_onevalue(pCapability, TWTY_BOOL, TRUE);
            break;

        case MSG_RESET:
            strcpy(source, "Auto");
            if (sane_option_set_str(activeDS.deviceHandle, SANE_NAME_SCAN_SOURCE, source, NULL) == SANE_STATUS_GOOD)
                enabled = TRUE;
            /* .. fall through intentional .. */

        case MSG_GETCURRENT:
            twCC = set_onevalue(pCapability, TWTY_BOOL, enabled);
            break;
    }
#endif
    return twCC;
}



TW_UINT16 SANE_SaneCapability (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT16 twCC = TWCC_CAPUNSUPPORTED;

    TRACE("capability=%d action=%d\n", pCapability->Cap, action);

    switch (pCapability->Cap)
    {
        case CAP_SUPPORTEDCAPS:
            if (action == MSG_GET)
                twCC = TWAIN_GetSupportedCaps(pCapability);
            else
                twCC = TWCC_BADVALUE;
            break;

        case CAP_XFERCOUNT:
            twCC = SANE_CAPXferCount (pCapability, action);
            break;

        case CAP_UICONTROLLABLE:
            twCC = SANE_CAPUiControllable (pCapability, action);
            break;

        case CAP_AUTOFEED:
            twCC = SANE_CAPAutofeed (pCapability, action);
            break;

        case CAP_FEEDERENABLED:
            twCC = SANE_CAPFeederEnabled (pCapability, action);
            break;

        case ICAP_PIXELTYPE:
            twCC = SANE_ICAPPixelType (pCapability, action);
            break;

        case ICAP_UNITS:
            twCC = SANE_ICAPUnits (pCapability, action);
            break;

        case ICAP_BITDEPTH:
            twCC = SANE_ICAPBitDepth(pCapability, action);
            break;

        case ICAP_XFERMECH:
            twCC = SANE_ICAPXferMech (pCapability, action);
            break;

        case ICAP_PIXELFLAVOR:
            twCC = SANE_ICAPPixelFlavor (pCapability, action);
            break;

        case ICAP_COMPRESSION:
            twCC = SANE_ICAPCompression(pCapability, action);
            break;

        case ICAP_XRESOLUTION:
            twCC = SANE_ICAPResolution(pCapability, action, pCapability->Cap);
            break;

        case ICAP_YRESOLUTION:
            twCC = SANE_ICAPResolution(pCapability, action, pCapability->Cap);
            break;

        case ICAP_PHYSICALHEIGHT:
            twCC = SANE_ICAPPhysical(pCapability, action, pCapability->Cap);
            break;

        case ICAP_PHYSICALWIDTH:
            twCC = SANE_ICAPPhysical(pCapability, action, pCapability->Cap);
            break;

        case ICAP_SUPPORTEDSIZES:
            twCC = SANE_ICAPSupportedSizes (pCapability, action);
            break;

        case ICAP_PLANARCHUNKY:
            FIXME("ICAP_PLANARCHUNKY not implemented\n");
            break;

        case ICAP_BITORDER:
            FIXME("ICAP_BITORDER not implemented\n");
            break;

    }

    /* Twain specifies that you should return a 0 in response to QUERYSUPPORT,
     *   even if you don't formally support the capability */
    if (twCC == TWCC_CAPUNSUPPORTED && action == MSG_QUERYSUPPORT)
        twCC = set_onevalue(pCapability, 0, TWTY_INT32);

    if (twCC == TWCC_CAPUNSUPPORTED)
        TRACE("capability 0x%x/action=%d being reported as unsupported\n", pCapability->Cap, action);

    return twCC;
}

#ifdef SONAME_LIBSANE
TW_UINT16 SANE_SaneSetDefaults (void)
{
    TW_CAPABILITY cap;

    memset(&cap, 0, sizeof(cap));
    cap.Cap = CAP_AUTOFEED;
    cap.ConType = TWON_DONTCARE16;

    if (SANE_SaneCapability(&cap, MSG_RESET) == TWCC_SUCCESS)
        GlobalFree(cap.hContainer);

    memset(&cap, 0, sizeof(cap));
    cap.Cap = CAP_FEEDERENABLED;
    cap.ConType = TWON_DONTCARE16;

    if (SANE_SaneCapability(&cap, MSG_RESET) == TWCC_SUCCESS)
        GlobalFree(cap.hContainer);

    memset(&cap, 0, sizeof(cap));
    cap.Cap = ICAP_SUPPORTEDSIZES;
    cap.ConType = TWON_DONTCARE16;

    if (SANE_SaneCapability(&cap, MSG_RESET) == TWCC_SUCCESS)
        GlobalFree(cap.hContainer);

   return TWCC_SUCCESS;
}
#endif
