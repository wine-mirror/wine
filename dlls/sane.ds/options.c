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

WINE_DEFAULT_DEBUG_CHANNEL(twain);

static TW_UINT16 sane_status_to_twcc(SANE_Status rc)
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

TW_UINT16 sane_option_get_value( int optno, void *val )
{
    return sane_status_to_twcc( sane_control_option( activeDS.deviceHandle, optno,
                                                    SANE_ACTION_GET_VALUE, val, NULL ));
}

TW_UINT16 sane_option_set_value( int optno, void *val, BOOL *reload )
{
    int status = 0;
    TW_UINT16 rc = sane_status_to_twcc( sane_control_option( activeDS.deviceHandle, optno,
                                                             SANE_ACTION_SET_VALUE, val, &status ));
    if (rc == TWCC_SUCCESS && reload)
        *reload = (status & (SANE_INFO_RELOAD_OPTIONS | SANE_INFO_RELOAD_PARAMS | SANE_INFO_INEXACT)) != 0;
    return rc;
}

static TW_UINT16 sane_find_option(const char *option_name,
        const SANE_Option_Descriptor **opt_p, int *optno, SANE_Value_Type type)
{
    TW_UINT16 rc;
    int optcount;
    const SANE_Option_Descriptor *opt;
    int i;

    /* Debian, in 32_net_backend_standard_fix.dpatch,
     *  forces a frontend (that's us) to reload options
     *  manually by invoking get_option_descriptor. */
    opt = sane_get_option_descriptor(activeDS.deviceHandle, 0);
    if (! opt)
        return TWCC_BUMMER;

    rc = sane_option_get_value( 0, &optcount );
    if (rc != TWCC_SUCCESS) return rc;

    for (i = 1; i < optcount; i++)
    {
        opt = sane_get_option_descriptor(activeDS.deviceHandle, i);
        if (opt && (opt->name && strcmp(opt->name, option_name) == 0) &&
               opt->type == type)
        {
            *opt_p = opt;
            *optno = i;
            return TWCC_SUCCESS;
        }
    }
    return TWCC_CAPUNSUPPORTED;
}

TW_UINT16 sane_option_get_int(const char *option_name, int *val)
{
    int optno;
    const SANE_Option_Descriptor *opt;
    TW_UINT16 rc = sane_find_option(option_name, &opt, &optno, SANE_TYPE_INT);

    if (rc == TWCC_SUCCESS) rc = sane_option_get_value( optno, val );
    return rc;
}

TW_UINT16 sane_option_set_int(const char *option_name, int val, BOOL *needs_reload )
{
    int optno;
    const SANE_Option_Descriptor *opt;
    TW_UINT16 rc = sane_find_option(option_name, &opt, &optno, SANE_TYPE_INT);

    if (rc == TWCC_SUCCESS) rc = sane_option_set_value( optno, &val, needs_reload );
    return rc;
}

TW_UINT16 sane_option_get_bool(const char *option_name, int *val)
{
    int optno;
    const SANE_Option_Descriptor *opt;
    TW_UINT16 rc = sane_find_option(option_name, &opt, &optno, SANE_TYPE_BOOL);

    if (rc == TWCC_SUCCESS) rc = sane_option_get_value( optno, val );
    return rc;
}

TW_UINT16 sane_option_set_bool(const char *option_name, int val )
{
    int optno;
    const SANE_Option_Descriptor *opt;
    TW_UINT16 rc = sane_find_option(option_name, &opt, &optno, SANE_TYPE_BOOL);

    if (rc == TWCC_SUCCESS) rc = sane_option_set_value( optno, &val, NULL );
    return rc;
}

TW_UINT16 sane_option_get_str(const char *option_name, char *val, int len)
{
    int optno;
    const SANE_Option_Descriptor *opt;
    TW_UINT16 rc = sane_find_option(option_name, &opt, &optno, SANE_TYPE_STRING);

    if (rc == TWCC_SUCCESS)
    {
        if (opt->size < len)
            rc = sane_option_get_value( optno, val );
        else
            rc = TWCC_BADVALUE;
    }
    return rc;
}

/* Important:  SANE has the side effect of overwriting val with the returned value */
TW_UINT16 sane_option_set_str(const char *option_name, char *val, BOOL *needs_reload)
{
    int optno;
    const SANE_Option_Descriptor *opt;
    TW_UINT16 rc = sane_find_option(option_name, &opt, &optno, SANE_TYPE_STRING);

    if (rc == TWCC_SUCCESS) rc = sane_option_set_value( optno, &val, needs_reload );
    return rc;
}

TW_UINT16 sane_option_probe_resolution(const char *option_name, int *minval, int *maxval, int *quant)
{
    int optno;
    const SANE_Option_Descriptor *opt;
    TW_UINT16 rc = sane_find_option(option_name, &opt, &optno, SANE_TYPE_INT);

    if (rc != TWCC_SUCCESS) return rc;
    if (opt->constraint_type != SANE_CONSTRAINT_RANGE) return TWCC_CAPUNSUPPORTED;

    *minval = opt->constraint.range->min;
    *maxval = opt->constraint.range->max;
    *quant = opt->constraint.range->quant;

    return rc;
}

TW_UINT16 sane_option_probe_mode(const char * const **choices, char *current, int current_size)
{
    int optno;
    const SANE_Option_Descriptor *opt;
    TW_UINT16 rc = sane_find_option("mode", &opt, &optno, SANE_TYPE_STRING);

    if (rc != TWCC_SUCCESS) return rc;
    if (choices && opt->constraint_type == SANE_CONSTRAINT_STRING_LIST)
        *choices = opt->constraint.string_list;

    if (opt->size < current_size)
        return sane_option_get_value( optno, current );
    else
        return TWCC_BADVALUE;

}

TW_UINT16 sane_option_get_scan_area( int *tlx, int *tly, int *brx, int *bry )
{
    TW_UINT16 rc;
    int optno;
    const SANE_Option_Descriptor *opt;

    if ((rc = sane_find_option( "tl-x", &opt, &optno, SANE_TYPE_FIXED )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_get_value( optno, tlx )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_find_option( "tl-y", &opt, &optno, SANE_TYPE_FIXED )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_get_value( optno, tly )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_find_option( "br-x", &opt, &optno, SANE_TYPE_FIXED )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_get_value( optno, brx )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_find_option( "br-y", &opt, &optno, SANE_TYPE_FIXED )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_get_value( optno, bry )) != TWCC_SUCCESS) return rc;
    if (opt->unit != SANE_UNIT_MM) FIXME( "unsupported unit %u\n", opt->unit );
    return rc;
}

TW_UINT16 sane_option_get_max_scan_area( int *tlx, int *tly, int *brx, int *bry )
{
    TW_UINT16 rc;
    int optno;
    const SANE_Option_Descriptor *opt;

    if ((rc = sane_find_option( "tl-x", &opt, &optno, SANE_TYPE_FIXED )) != TWCC_SUCCESS) return rc;
    *tlx = opt->constraint.range->min;
    if ((rc = sane_find_option( "tl-y", &opt, &optno, SANE_TYPE_FIXED )) != TWCC_SUCCESS) return rc;
    *tly = opt->constraint.range->min;
    if ((rc = sane_find_option( "br-x", &opt, &optno, SANE_TYPE_FIXED )) != TWCC_SUCCESS) return rc;
    *brx = opt->constraint.range->max;
    if ((rc = sane_find_option( "br-y", &opt, &optno, SANE_TYPE_FIXED )) != TWCC_SUCCESS) return rc;
    *bry = opt->constraint.range->max;
    if (opt->unit != SANE_UNIT_MM) FIXME( "unsupported unit %u\n", opt->unit );
    return rc;
}

TW_UINT16 sane_option_set_scan_area( int tlx, int tly, int brx, int bry, BOOL *reload )
{
    TW_UINT16 rc;
    int optno;
    const SANE_Option_Descriptor *opt;

    if ((rc = sane_find_option( "tl-x", &opt, &optno, SANE_TYPE_FIXED )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_set_value( optno, &tlx, reload )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_find_option( "tl-y", &opt, &optno, SANE_TYPE_FIXED )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_set_value( optno, &tly, reload )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_find_option( "br-x", &opt, &optno, SANE_TYPE_FIXED )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_set_value( optno, &brx, reload )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_find_option( "br-y", &opt, &optno, SANE_TYPE_FIXED )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_set_value( optno, &bry, reload )) != TWCC_SUCCESS) return rc;
    return rc;
}

TW_FIX32 convert_sane_res_to_twain(int res)
{
    TW_FIX32 value;
    res = MulDiv( res, 10, 254 );  /* mm -> inch */
    value.Whole = res / 65536;
    value.Frac  = res & 0xffff;
    return value;
}

TW_UINT16 get_sane_params( struct frame_parameters *params )
{
    SANE_Parameters sane_params;
    TW_UINT16 rc = sane_status_to_twcc( sane_get_parameters( activeDS.deviceHandle, &sane_params ));

    if (rc != TWCC_SUCCESS) return rc;
    switch (sane_params.format)
    {
    case SANE_FRAME_GRAY:
        params->format = FMT_GRAY;
        break;
    case SANE_FRAME_RGB:
        params->format = FMT_RGB;
        break;
    default:
        ERR("Unhandled source frame format %i\n", sane_params.format);
        params->format = FMT_OTHER;
        break;
    }
    params->last_frame = sane_params.last_frame;
    params->bytes_per_line = sane_params.bytes_per_line;
    params->pixels_per_line = sane_params.pixels_per_line;
    params->lines = sane_params.lines;
    params->depth = sane_params.depth;
    return TWCC_SUCCESS;
}
