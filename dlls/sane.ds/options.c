/*
 * Copyright 2009 Jeremy White <jwhite@codeweavers.com> for CodeWeavers
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

#include <stdlib.h>
#include "sane_i.h"
#include "wine/debug.h"

#ifdef SONAME_LIBSANE
static SANE_Status sane_find_option(SANE_Handle h, const char *option_name,
        const SANE_Option_Descriptor **opt_p, int *optno, SANE_Value_Type type)
{
    SANE_Status rc;
    SANE_Int optcount;
    const SANE_Option_Descriptor *opt;
    int i;

    /* Debian, in 32_net_backend_standard_fix.dpatch,
     *  forces a frontend (that's us) to reload options
     *  manually by invoking get_option_descriptor. */
    opt = psane_get_option_descriptor(h, 0);
    if (! opt)
        return SANE_STATUS_EOF;

    rc = psane_control_option(h, 0, SANE_ACTION_GET_VALUE, &optcount, NULL);
    if (rc != SANE_STATUS_GOOD)
        return rc;

    for (i = 1; i < optcount; i++)
    {
        opt = psane_get_option_descriptor(h, i);
        if (opt && (opt->name && strcmp(opt->name, option_name) == 0) &&
               opt->type == type)
        {
            *opt_p = opt;
            *optno = i;
            return SANE_STATUS_GOOD;
        }
    }
    return SANE_STATUS_EOF;
}

SANE_Status sane_option_get_int(SANE_Handle h, const char *option_name, SANE_Int *val)
{
    SANE_Status rc;
    int optno;
    const SANE_Option_Descriptor *opt;

    rc = sane_find_option(h, option_name, &opt, &optno, SANE_TYPE_INT);
    if (rc != SANE_STATUS_GOOD)
        return rc;

    return psane_control_option(h, optno, SANE_ACTION_GET_VALUE, val, NULL);
}

SANE_Status sane_option_set_int(SANE_Handle h, const char *option_name, SANE_Int val, SANE_Int *status)
{
    SANE_Status rc;
    int optno;
    const SANE_Option_Descriptor *opt;

    rc = sane_find_option(h, option_name, &opt, &optno, SANE_TYPE_INT);
    if (rc != SANE_STATUS_GOOD)
        return rc;

    return psane_control_option(h, optno, SANE_ACTION_SET_VALUE, (void *) &val, status);
}

SANE_Status sane_option_get_bool(SANE_Handle h, const char *option_name, SANE_Bool *val, SANE_Int *status)
{
    SANE_Status rc;
    int optno;
    const SANE_Option_Descriptor *opt;

    rc = sane_find_option(h, option_name, &opt, &optno, SANE_TYPE_BOOL);
    if (rc != SANE_STATUS_GOOD)
        return rc;

    return psane_control_option(h, optno, SANE_ACTION_GET_VALUE, (void *) val, status);
}

SANE_Status sane_option_set_bool(SANE_Handle h, const char *option_name, SANE_Bool val, SANE_Int *status)
{
    SANE_Status rc;
    int optno;
    const SANE_Option_Descriptor *opt;

    rc = sane_find_option(h, option_name, &opt, &optno, SANE_TYPE_BOOL);
    if (rc != SANE_STATUS_GOOD)
        return rc;

    return psane_control_option(h, optno, SANE_ACTION_SET_VALUE, (void *) &val, status);
}

SANE_Status sane_option_set_fixed(SANE_Handle h, const char *option_name, SANE_Fixed val, SANE_Int *status)
{
    SANE_Status rc;
    int optno;
    const SANE_Option_Descriptor *opt;

    rc = sane_find_option(h, option_name, &opt, &optno, SANE_TYPE_FIXED);
    if (rc != SANE_STATUS_GOOD)
        return rc;

    return psane_control_option(h, optno, SANE_ACTION_SET_VALUE, (void *) &val, status);
}

SANE_Status sane_option_get_str(SANE_Handle h, const char *option_name, SANE_String val, size_t len, SANE_Int *status)
{
    SANE_Status rc;
    int optno;
    const SANE_Option_Descriptor *opt;

    rc = sane_find_option(h, option_name, &opt, &optno, SANE_TYPE_STRING);
    if (rc != SANE_STATUS_GOOD)
        return rc;

    if (opt->size < len)
        return psane_control_option(h, optno, SANE_ACTION_GET_VALUE, (void *) val, status);
    else
        return SANE_STATUS_NO_MEM;
}

/* Important:  SANE has the side effect of overwriting val with the returned value */
SANE_Status sane_option_set_str(SANE_Handle h, const char *option_name, SANE_String val, SANE_Int *status)
{
    SANE_Status rc;
    int optno;
    const SANE_Option_Descriptor *opt;

    rc = sane_find_option(h, option_name, &opt, &optno, SANE_TYPE_STRING);
    if (rc != SANE_STATUS_GOOD)
        return rc;

    return psane_control_option(h, optno, SANE_ACTION_SET_VALUE, (void *) val, status);
}

SANE_Status sane_option_probe_resolution(SANE_Handle h, const char *option_name, SANE_Int *minval, SANE_Int *maxval, SANE_Int *quant)
{
    SANE_Status rc;
    int optno;
    const SANE_Option_Descriptor *opt;

    rc = sane_find_option(h, option_name, &opt, &optno, SANE_TYPE_INT);
    if (rc != SANE_STATUS_GOOD)
        return rc;

    if (opt->constraint_type != SANE_CONSTRAINT_RANGE)
        return SANE_STATUS_UNSUPPORTED;

    *minval = opt->constraint.range->min;
    *maxval = opt->constraint.range->max;
    *quant = opt->constraint.range->quant;

    return rc;
}

SANE_Status sane_option_probe_mode(SANE_Handle h, SANE_String_Const **choices, char *current, int current_size)
{
    SANE_Status rc;
    int optno;
    const SANE_Option_Descriptor *opt;
    rc = sane_find_option(h, "mode", &opt, &optno, SANE_TYPE_STRING);
    if (rc != SANE_STATUS_GOOD)
        return rc;

    if (choices && opt->constraint_type == SANE_CONSTRAINT_STRING_LIST)
        *choices = (SANE_String_Const *) opt->constraint.string_list;

    if (opt->size < current_size)
        return psane_control_option(h, optno, SANE_ACTION_GET_VALUE, current, NULL);
    else
        return SANE_STATUS_NO_MEM;

}

SANE_Status sane_option_probe_scan_area(SANE_Handle h, const char *option_name, SANE_Fixed *val,
                                        SANE_Unit *unit, SANE_Fixed *min, SANE_Fixed *max, SANE_Fixed *quant)
{
    SANE_Status rc;
    int optno;
    const SANE_Option_Descriptor *opt;

    rc = sane_find_option(h, option_name, &opt, &optno, SANE_TYPE_FIXED);
    if (rc != SANE_STATUS_GOOD)
        return rc;

    if (unit)
        *unit = opt->unit;
    if (min)
        *min = opt->constraint.range->min;
    if (max)
        *max = opt->constraint.range->max;
    if (quant)
        *quant = opt->constraint.range->quant;

    if (val)
        rc = psane_control_option(h, optno, SANE_ACTION_GET_VALUE, val, NULL);

    return rc;
}

TW_UINT16 sane_status_to_twcc(SANE_Status rc)
{
    switch (rc)
    {
        case SANE_STATUS_GOOD:
            return TWCC_SUCCESS;
        case SANE_STATUS_UNSUPPORTED:
            return TWCC_CAPUNSUPPORTED;
        case SANE_STATUS_JAMMED:
            return TWCC_PAPERJAM;
        case SANE_STATUS_NO_MEM:
            return TWCC_LOWMEMORY;
        case SANE_STATUS_ACCESS_DENIED:
            return TWCC_DENIED;

        case SANE_STATUS_IO_ERROR:
        case SANE_STATUS_NO_DOCS:
        case SANE_STATUS_COVER_OPEN:
        case SANE_STATUS_EOF:
        case SANE_STATUS_INVAL:
        case SANE_STATUS_CANCELLED:
        case SANE_STATUS_DEVICE_BUSY:
        default:
            return TWCC_BUMMER;
    }
}
static void convert_double_fix32(double d, TW_FIX32 *fix32)
{
    TW_INT32 value = (TW_INT32) (d * 65536.0 + 0.5);
    fix32->Whole = value >> 16;
    fix32->Frac = value & 0x0000ffffL;
}

BOOL convert_sane_res_to_twain(double sane_res, SANE_Unit unit, TW_FIX32 *twain_res, TW_UINT16 twtype)
{
    if (unit != SANE_UNIT_MM)
        return FALSE;

    if (twtype != TWUN_INCHES)
        return FALSE;

    convert_double_fix32((sane_res / 10.0) / 2.54, twain_res);

    return TRUE;
}
#endif
