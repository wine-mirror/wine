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

#include <stdarg.h>
#include <stdio.h>
#include <math.h>

#include "sane_i.h"
#include "winnls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

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

static TW_UINT16 msg_get_array(pTW_CAPABILITY pCapability, TW_UINT16 type, const int *values, int value_count)
{
    TW_ARRAY *a;
    TW_UINT32 *p32;
    TW_UINT16 *p16;

    pCapability->hContainer = 0;
    pCapability->ConType = TWON_ARRAY;

    if (type == TWTY_INT16 || type == TWTY_UINT16)
        pCapability->hContainer = GlobalAlloc (0, FIELD_OFFSET( TW_ARRAY, ItemList[value_count * sizeof(TW_UINT16)]));
    else if (type == TWTY_INT32 || type == TWTY_UINT32)
        pCapability->hContainer = GlobalAlloc (0, FIELD_OFFSET( TW_ARRAY, ItemList[value_count * sizeof(TW_UINT32)]));

    if (pCapability->hContainer)
    {
        TW_UINT32 i;
        a = GlobalLock (pCapability->hContainer);
        a->ItemType = type;
        a->NumItems = value_count;
        p16 = (TW_UINT16 *) a->ItemList;
        p32 = (TW_UINT32 *) a->ItemList;
        for (i = 0; i < a->NumItems; i++)
        {
            if (type == TWTY_INT16 || type == TWTY_UINT16)
                p16[i] = values[i];
            else if (type == TWTY_INT32 || type == TWTY_UINT32)
                p32[i] = values[i];
        }
        GlobalUnlock (pCapability->hContainer);
        return TWCC_SUCCESS;
    }
    else
        return TWCC_LOWMEMORY;
}

static TW_UINT16 TWAIN_GetSupportedCaps(pTW_CAPABILITY pCapability)
{
    static const int supported_caps[] = { CAP_SUPPORTEDCAPS, CAP_XFERCOUNT, CAP_UICONTROLLABLE,
                    CAP_AUTOFEED, CAP_FEEDERENABLED,
                    ICAP_XFERMECH, ICAP_PIXELTYPE, ICAP_UNITS, ICAP_BITDEPTH, ICAP_COMPRESSION, ICAP_PIXELFLAVOR,
                    ICAP_XRESOLUTION, ICAP_YRESOLUTION, ICAP_PHYSICALHEIGHT, ICAP_PHYSICALWIDTH, ICAP_SUPPORTEDSIZES };

    return msg_get_array(pCapability, TWTY_UINT16, supported_caps, ARRAY_SIZE(supported_caps));
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
               FIXME("Partial Stub:  XFERMECH set to %ld, but ignored\n", val);
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
               FIXME("Partial Stub:  XFERCOUNT set to %ld, but ignored\n", val);
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

static TW_UINT16 set_option_value(const char* option_name, char* option_value)
{
    TW_UINT16 twCC = TWCC_BADVALUE;
    BOOL reload = FALSE;
    if (*option_value)
    {
        twCC = sane_option_set_str(option_name, option_value, &reload);
        if (twCC == TWCC_SUCCESS) {
            if (reload) get_sane_params(&activeDS.frame_params);
        }
    }
    return twCC;
}

static TW_UINT16 find_value_pos(const char* value, const char* values, TW_UINT16 buf_len, TW_UINT16 buf_count)
{
    TW_UINT16 index;
    for (index=0; index<buf_count; ++index)
    {
        if (!strncmp(value, values, buf_len))
            return index;
        values += buf_len;
    }
    return buf_count;
}

/* ICAP_PIXELTYPE */
static TW_UINT16 SANE_ICAPPixelType (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT16 twCC;
    TW_UINT32 possible_values[3];
    int possible_value_count;
    TW_UINT32 val;
    TW_UINT16 current_pixeltype = TWPT_BW;
    enum { buf_len = 64, buf_count = 3 };
    char color_modes[buf_len * buf_count] = {'\0'}, current_mode[buf_len];
    /* most of the values are taken from https://gitlab.gnome.org/GNOME/simple-scan/-/blob/master/src/scanner.vala */
    static const WCHAR* bw[] = {L"Lineart", L"LineArt", L"Black & White", L"Binary", L"Thresholded",
        L"1-bit Black & White", L"Black and White - Line Art", L"Black and White - Halftone", L"Monochrome", L"bw", 0};
    static const WCHAR* gray[] = {L"Gray", L"Grayscale", L"True Gray", L"8-bit Grayscale", L"Grayscale - 256 Levels",
        L"gray", 0};
    static const WCHAR* rgb[] = {L"Color", L"24bit Color[Fast]", L"24bit Color", L"24 bit Color",
        L"Color - 16 Million Colors", L"color", 0};
    /* TWPT_BW = 0, TWPT_GRAY = 1, TWPT_RGB = 2 */
    static const WCHAR* const* filter[] = {bw, gray, rgb, 0};

    TRACE("ICAP_PIXELTYPE\n");
    twCC = sane_option_probe_str("mode", filter, color_modes, buf_len);
    if (twCC != TWCC_SUCCESS)
    {
        ERR("Unable to retrieve modes from sane, ICAP_PIXELTYPE unsupported\n");
        return twCC;
    }

    twCC = sane_option_get_str("mode", current_mode, sizeof(current_mode));
    if (twCC != TWCC_SUCCESS)
    {
        ERR("Unable to retrieve current mode from sane, ICAP_PIXELTYPE unsupported\n");
        return twCC;
    }

    /* Map current mode name to TWPT_BW (0), TWPT_GRAY (1) or TWPT_RGB (2) */
    current_pixeltype = find_value_pos(current_mode, color_modes, buf_len, buf_count);
    if (current_pixeltype == buf_count)
    {
        ERR("Wrong current mode value, ICAP_PIXELTYPE unsupported\n");
        twCC = TWCC_BADVALUE;
        return twCC;
    }
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
            possible_value_count = 0;
            for (val=TWPT_BW; val<=TWPT_RGB; ++val)
            {
                if (*(color_modes + val * buf_len))
                    possible_values[possible_value_count++] = val;
            }
            twCC = msg_get_enum(pCapability, possible_values, possible_value_count,
                    TWTY_UINT16, current_pixeltype, activeDS.defaultPixelType);
            break;

        case MSG_SET:
            twCC = msg_set(pCapability, &val);
            if ((twCC == TWCC_SUCCESS) && (val < buf_count))
            {
                char *output = color_modes + val * buf_len;
                TRACE("Setting pixeltype to %lu: %s\n", val, output);
                twCC = set_option_value("mode", output);
                if (twCC != TWCC_SUCCESS)
                    ERR("Unable to set pixeltype to %lu: %s\n", val, output);
            }
            else
                twCC = TWCC_BADVALUE;
            break;

        case MSG_GETDEFAULT:
            twCC = set_onevalue(pCapability, TWTY_UINT16, activeDS.defaultPixelType);
            break;

        case MSG_RESET:
            twCC = set_option_value("mode", color_modes + activeDS.defaultPixelType * buf_len);
            if (twCC == TWCC_SUCCESS)
                current_pixeltype = activeDS.defaultPixelType;
            else
                ERR("Unable to reset color mode\n");

            /* .. fall through intentional .. */

        case MSG_GETCURRENT:
            twCC = set_onevalue(pCapability, TWTY_UINT16, current_pixeltype);
            TRACE("Returning current pixeltype of %d\n", current_pixeltype);
            break;
    }

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
    TW_UINT32 possible_values[1];

    TRACE("ICAP_BITDEPTH\n");

    possible_values[0] = activeDS.frame_params.depth;

    switch (action)
    {
        case MSG_QUERYSUPPORT:
            twCC = set_onevalue(pCapability, TWTY_INT32,
                    TWQC_GET | TWQC_GETDEFAULT | TWQC_GETCURRENT  );
            break;

        case MSG_GET:
            twCC = msg_get_enum(pCapability, possible_values, ARRAY_SIZE(possible_values),
                    TWTY_UINT16, activeDS.frame_params.depth, activeDS.frame_params.depth);
            break;

        case MSG_GETDEFAULT:
            /* .. Fall through intentional .. */

        case MSG_GETCURRENT:
            TRACE("Returning current bitdepth of %d\n", activeDS.frame_params.depth);
            twCC = set_onevalue(pCapability, TWTY_UINT16, activeDS.frame_params.depth);
            break;
    }
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
               FIXME("Partial Stub:  COMPRESSION set to %ld, but ignored\n", val);
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
    TW_UINT32 val;
    int current_resolution;
    TW_FIX32 *default_res;
    const char *best_option_name;
    struct option_descriptor opt;

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
    if (sane_option_get_int(best_option_name, &current_resolution) != TWCC_SUCCESS)
    {
        best_option_name = "resolution";
        if (sane_option_get_int(best_option_name, &current_resolution) != TWCC_SUCCESS)
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
            twCC = sane_option_probe_resolution(best_option_name, &opt);
            if (twCC == TWCC_SUCCESS)
            {
                if (opt.constraint_type == CONSTRAINT_RANGE)
                    twCC = msg_get_range(pCapability, TWTY_FIX32,
                            opt.constraint.range.min, opt.constraint.range.max,
                            opt.constraint.range.quant == 0 ? 1 : opt.constraint.range.quant,
                            default_res->Whole, current_resolution);
                else if (opt.constraint_type == CONSTRAINT_WORD_LIST)
                    twCC = msg_get_array(pCapability, TWTY_UINT32, &(opt.constraint.word_list[1]),
                            opt.constraint.word_list[0]);
                else
                    twCC = TWCC_CAPUNSUPPORTED;
            }
            break;

        case MSG_SET:
            twCC = msg_set(pCapability, &val);
            if (twCC == TWCC_SUCCESS)
            {
                TW_FIX32 f32;
                BOOL reload = FALSE;
                memcpy(&f32, &val, sizeof(f32));
                twCC = sane_option_set_int(best_option_name, f32.Whole, &reload);
                if (reload) twCC = TWCC_CHECKSTATUS;
            }
            break;

        case MSG_GETDEFAULT:
            twCC = set_onevalue(pCapability, TWTY_FIX32, default_res->Whole);
            break;

        case MSG_RESET:
            twCC = sane_option_set_int(best_option_name, default_res->Whole, NULL);
            if (twCC != TWCC_SUCCESS) return twCC;

            /* .. fall through intentional .. */

        case MSG_GETCURRENT:
            twCC = set_onevalue(pCapability, TWTY_FIX32, current_resolution);
            break;
    }
    return twCC;
}

/* ICAP_PHYSICALHEIGHT, ICAP_PHYSICALWIDTH */
static TW_UINT16 SANE_ICAPPhysical (pTW_CAPABILITY pCapability, TW_UINT16 action,  TW_UINT16 cap)
{
    TW_UINT16 twCC;
    TW_FIX32 res;
    int tlx, tly, brx, bry;

    TRACE("ICAP_PHYSICAL%s\n", cap == ICAP_PHYSICALHEIGHT? "HEIGHT" : "WIDTH");

    twCC = sane_option_get_max_scan_area( &tlx, &tly, &brx, &bry );
    if (twCC != TWCC_SUCCESS) return twCC;

    res = convert_sane_res_to_twain( (cap == ICAP_PHYSICALHEIGHT) ? bry - tly : brx - tlx );

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
    return twCC;
}

/* ICAP_PIXELFLAVOR */
static TW_UINT16 SANE_ICAPPixelFlavor (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT16 twCC = TWCC_BADCAP;
    static const TW_UINT32 possible_values[] = { TWPF_CHOCOLATE, TWPF_VANILLA };
    TW_UINT32 val;
    TW_UINT32 flavor = activeDS.frame_params.depth == 1 ? TWPF_VANILLA : TWPF_CHOCOLATE;

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
               FIXME("Stub:  PIXELFLAVOR set to %ld, but ignored\n", val);
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
    return twCC;
}

static TW_UINT16 get_width_height(double *width, double *height, BOOL max)
{
    TW_UINT16 rc;
    int tlx, tly, brx, bry;

    if (max) rc = sane_option_get_max_scan_area( &tlx, &tly, &brx, &bry );
    else rc = sane_option_get_scan_area( &tlx, &tly, &brx, &bry );

    if (rc == TWCC_SUCCESS)
    {
        *width = (brx - tlx) / 65536.0;
        *height = (bry - tly) / 65536.0;
    }
    return rc;
}

static TW_UINT16 set_width_height(double width, double height)
{
    return sane_option_set_scan_area( 0, 0, width * 65536, height * 65536, NULL );
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

/* ICAP_SUPPORTEDSIZES */
static TW_UINT16 SANE_ICAPSupportedSizes (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT16 twCC = TWCC_BADCAP;

    static TW_UINT32 possible_values[ARRAY_SIZE(supported_sizes)];
    unsigned int i;
    TW_UINT32 val;
    double max_width, max_height;
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
            /* TWAIN: For devices that support physical dimensions TWSS_NONE indicates that the maximum
               image size supported by the device is to be used. */
            if (val == TWSS_NONE && get_width_height(&max_width, &max_height, TRUE) == TWCC_SUCCESS)
                return set_width_height(max_width, max_height);
            ERR("Unsupported size %ld\n", val);
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

    return twCC;
}

/* CAP_AUTOFEED */
static TW_UINT16 SANE_CAPAutofeed (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT16 twCC = TWCC_BADCAP;
    TW_UINT32 val;
    BOOL autofeed;

    TRACE("CAP_AUTOFEED\n");

    if (sane_option_get_bool("batch-scan", &autofeed) != TWCC_SUCCESS)
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
                twCC = sane_option_set_bool("batch-scan", !!val);
            break;

        case MSG_GETDEFAULT:
            twCC = set_onevalue(pCapability, TWTY_BOOL, TRUE);
            break;

        case MSG_RESET:
            autofeed = TRUE;
            twCC = sane_option_set_bool("batch-scan", autofeed);
            if (twCC != TWCC_SUCCESS) break;
            /* .. fall through intentional .. */

        case MSG_GETCURRENT:
            twCC = set_onevalue(pCapability, TWTY_BOOL, autofeed);
            break;
    }
    return twCC;
}

/* CAP_FEEDERENABLED */
static TW_UINT16 SANE_CAPFeederEnabled (pTW_CAPABILITY pCapability, TW_UINT16 action)
{
    TW_UINT16 twCC = TWCC_BADCAP;
    TW_UINT32 val;
    TW_BOOL enabled;
    enum { buf_len = 64, buf_count = 2 };
    char paper_sources[buf_len * buf_count] = {'\0'}, current_source[buf_len], *output;
    /* most of the values are taken from https://gitlab.gnome.org/GNOME/simple-scan/-/blob/master/src/scanner.vala */
    static const WCHAR* flatbed[] = {L"Flatbed", L"FlatBed", L"Platen", L"Normal", L"Document Table", 0};
    static const WCHAR* autofeeder[] = {L"Auto", L"ADF", L"ADF Front", L"ADF Back", L"adf",
        L"Automatic Document Feeder", L"Automatic Document Feeder(centrally aligned)",
        L"Automatic Document Feeder(center aligned)", L"Automatic Document Feeder(left aligned)",
        L"ADF Simplex" L"DP", 0};
    static const WCHAR* const* filter[] = {flatbed, autofeeder, 0};

    TRACE("CAP_FEEDERENABLED\n");
    twCC = sane_option_probe_str("source", filter, paper_sources, buf_len);
    if (twCC != TWCC_SUCCESS)
    {
        ERR("Unable to retrieve paper sources from sane, CAP_FEEDERENABLED unsupported\n");
        return twCC;
    }

    twCC = sane_option_get_str("source", current_source, sizeof(current_source));
    if (twCC != TWCC_SUCCESS)
    {
        ERR("Unable to retrieve current paper source from sane, CAP_FEEDERENABLED unsupported\n");
        return twCC;
    }

    val = find_value_pos(current_source, paper_sources, buf_len, buf_count);
    if (val == buf_count)
    {
        ERR("Wrong current paper source value, CAP_FEEDERENABLED unsupported\n");
        twCC = TWCC_BADVALUE;
        return twCC;
    }
    enabled = val > 0;

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
            if ((twCC == TWCC_SUCCESS) && (val < buf_count))
            {
                output = paper_sources + val * buf_len;
                TRACE("Setting paper source to %lu: %s\n", val, output);
                twCC = set_option_value("source", output);
                if (twCC != TWCC_SUCCESS)
                    ERR("Unable to set paper source to %lu: %s\n", val, output);
            }
            else
                twCC = TWCC_BADVALUE;
            break;

        case MSG_GETDEFAULT:
            twCC = set_onevalue(pCapability, TWTY_BOOL, TRUE);
            break;

        case MSG_RESET:
            val = *(paper_sources + buf_len) ? 1 : 0; // set to Auto/ADF if it's supported
            output = paper_sources + val * buf_len;
            TRACE("Resetting paper source to %lu: %s\n", val, output);
            twCC = set_option_value("source", output);
            if (twCC != TWCC_SUCCESS)
                ERR("Unable to reset paper source to %lu: %s\n", val, output);
            else
                enabled = val > 0;
            /* .. fall through intentional .. */

        case MSG_GETCURRENT:
            twCC = set_onevalue(pCapability, TWTY_BOOL, enabled);
            break;
    }
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
