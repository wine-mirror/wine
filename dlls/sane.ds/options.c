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

#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "sane_i.h"
#include "wine/unicode.h"
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

static int map_type( SANE_Value_Type type )
{
    switch (type)
    {
    case SANE_TYPE_BOOL: return TYPE_BOOL;
    case SANE_TYPE_INT: return TYPE_INT;
    case SANE_TYPE_FIXED: return TYPE_FIXED;
    case SANE_TYPE_STRING: return TYPE_STRING;
    case SANE_TYPE_BUTTON: return TYPE_BUTTON;
    case SANE_TYPE_GROUP: return TYPE_GROUP;
    default: return -1;
    }
}

static int map_unit( SANE_Unit unit )
{
    switch (unit)
    {
    case SANE_UNIT_NONE: return UNIT_NONE;
    case SANE_UNIT_PIXEL: return UNIT_PIXEL;
    case SANE_UNIT_BIT: return UNIT_BIT;
    case SANE_UNIT_MM: return UNIT_MM;
    case SANE_UNIT_DPI: return UNIT_DPI;
    case SANE_UNIT_PERCENT: return UNIT_PERCENT;
    case SANE_UNIT_MICROSECOND: return UNIT_MICROSECOND;
    default: return -1;
    }
}

static int map_constraint_type( SANE_Constraint_Type type )
{
    switch (type)
    {
    case SANE_CONSTRAINT_NONE: return CONSTRAINT_NONE;
    case SANE_CONSTRAINT_RANGE: return CONSTRAINT_RANGE;
    case SANE_CONSTRAINT_WORD_LIST: return CONSTRAINT_WORD_LIST;
    case SANE_CONSTRAINT_STRING_LIST: return CONSTRAINT_STRING_LIST;
    default: return CONSTRAINT_NONE;
    }
}

static void map_descr( struct option_descriptor *descr, const SANE_Option_Descriptor *opt )
{
    unsigned int i, size;
    WCHAR *p;

    descr->type = map_type( opt->type );
    descr->unit = map_unit( opt->unit );
    descr->constraint_type = map_constraint_type( opt->constraint_type );
    descr->size = opt->size;
    descr->is_active = SANE_OPTION_IS_ACTIVE( opt->cap );
    if (opt->title)
        MultiByteToWideChar( CP_UNIXCP, 0, opt->title, -1, descr->title, ARRAY_SIZE(descr->title) );
    else
        descr->title[0] = 0;
    switch (descr->constraint_type)
    {
    case CONSTRAINT_RANGE:
        descr->constraint.range.min = opt->constraint.range->min;
        descr->constraint.range.max = opt->constraint.range->max;
        descr->constraint.range.quant = opt->constraint.range->quant;
        break;
    case CONSTRAINT_WORD_LIST:
        size = min( opt->constraint.word_list[0], ARRAY_SIZE(descr->constraint.word_list) - 1 );
        descr->constraint.word_list[0] = size;
        for (i = 1; i <= size; i++) descr->constraint.word_list[i] = opt->constraint.word_list[i];
        break;
    case CONSTRAINT_STRING_LIST:
        p = descr->constraint.strings;
        size = ARRAY_SIZE(descr->constraint.strings) - 1;
        for (i = 0; size && opt->constraint.string_list[i]; i++)
        {
            int len = MultiByteToWideChar( CP_UNIXCP, 0, opt->constraint.string_list[i], -1, p, size );
            size -= len;
            p += len;
        }
        *p = 0;
        break;
    default:
        break;
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

TW_UINT16 sane_option_get_descriptor( struct option_descriptor *descr )
{
    const SANE_Option_Descriptor *opt = sane_get_option_descriptor( activeDS.deviceHandle, descr->optno );

    if (!opt) return TWCC_CAPUNSUPPORTED;
    map_descr( descr, opt );
    return TWCC_SUCCESS;
}

static TW_UINT16 sane_find_option( const char *name, int type, struct option_descriptor *descr )
{
    const SANE_Option_Descriptor *opt;
    int i;

    for (i = 1; (opt = sane_get_option_descriptor( activeDS.deviceHandle, i )) != NULL; i++)
    {
        if (type != map_type( opt->type )) continue;
        if (strcmp( name, opt->name )) continue;
        descr->optno = i;
        map_descr( descr, opt );
        return TWCC_SUCCESS;
    }
    return TWCC_CAPUNSUPPORTED;
}

TW_UINT16 sane_option_get_int(const char *option_name, int *val)
{
    struct option_descriptor opt;
    TW_UINT16 rc = sane_find_option(option_name, TYPE_INT, &opt);

    if (rc == TWCC_SUCCESS) rc = sane_option_get_value( opt.optno, val );
    return rc;
}

TW_UINT16 sane_option_set_int(const char *option_name, int val, BOOL *needs_reload )
{
    struct option_descriptor opt;
    TW_UINT16 rc = sane_find_option(option_name, TYPE_INT, &opt);

    if (rc == TWCC_SUCCESS) rc = sane_option_set_value( opt.optno, &val, needs_reload );
    return rc;
}

TW_UINT16 sane_option_get_bool(const char *option_name, int *val)
{
    struct option_descriptor opt;
    TW_UINT16 rc = sane_find_option(option_name, TYPE_BOOL, &opt);

    if (rc == TWCC_SUCCESS) rc = sane_option_get_value( opt.optno, val );
    return rc;
}

TW_UINT16 sane_option_set_bool(const char *option_name, int val )
{
    struct option_descriptor opt;
    TW_UINT16 rc = sane_find_option(option_name, TYPE_BOOL, &opt);

    if (rc == TWCC_SUCCESS) rc = sane_option_set_value( opt.optno, &val, NULL );
    return rc;
}

TW_UINT16 sane_option_get_str(const char *option_name, char *val, int len)
{
    struct option_descriptor opt;
    TW_UINT16 rc = sane_find_option(option_name, TYPE_STRING, &opt);

    if (rc == TWCC_SUCCESS)
    {
        if (opt.size < len)
            rc = sane_option_get_value( opt.optno, val );
        else
            rc = TWCC_BADVALUE;
    }
    return rc;
}

/* Important:  SANE has the side effect of overwriting val with the returned value */
TW_UINT16 sane_option_set_str(const char *option_name, char *val, BOOL *needs_reload)
{
    struct option_descriptor opt;
    TW_UINT16 rc = sane_find_option(option_name, TYPE_STRING, &opt);

    if (rc == TWCC_SUCCESS) rc = sane_option_set_value( opt.optno, &val, needs_reload );
    return rc;
}

TW_UINT16 sane_option_probe_resolution(const char *option_name, int *minval, int *maxval, int *quant)
{
    struct option_descriptor opt;
    TW_UINT16 rc = sane_find_option(option_name, TYPE_INT, &opt);

    if (rc != TWCC_SUCCESS) return rc;
    if (opt.constraint_type != CONSTRAINT_RANGE) return TWCC_CAPUNSUPPORTED;

    *minval = opt.constraint.range.min;
    *maxval = opt.constraint.range.max;
    *quant  = opt.constraint.range.quant;
    return rc;
}

TW_UINT16 sane_option_probe_mode(TW_UINT16 *current, TW_UINT32 *choices, int *count)
{
    WCHAR *p;
    char buffer[256];
    struct option_descriptor opt;
    TW_UINT16 rc = sane_find_option("mode", TYPE_STRING, &opt);

    if (rc != TWCC_SUCCESS) return rc;
    if (opt.size > sizeof(buffer)) return TWCC_BADVALUE;
    rc = sane_option_get_value( opt.optno, buffer );
    if (rc != TWCC_SUCCESS) return rc;

    if (!strcmp( buffer, "Lineart" )) *current = TWPT_BW;
    else if (!strcmp( buffer, "Color" )) *current = TWPT_RGB;
    else if (!strncmp( buffer, "Gray", 4 )) *current = TWPT_GRAY;

    *count = 0;
    if (opt.constraint_type == CONSTRAINT_STRING_LIST)
    {
        for (p = opt.constraint.strings; *p; p += strlenW(p) + 1)
        {
            static const WCHAR lineartW[] = {'L','i','n','e','a','r','t',0};
            static const WCHAR colorW[] = {'C','o','l','o','r',0};
            static const WCHAR grayW[] = {'G','r','a','y',0};
            if (!strcmpW( p, lineartW )) choices[(*count)++] = TWPT_BW;
            else if (!strcmpW( p, colorW )) choices[(*count)++] = TWPT_RGB;
            else if (!strncmpW( p, grayW, 4 )) choices[(*count)++] = TWPT_GRAY;
        }
    }
    return rc;
}

TW_UINT16 sane_option_get_scan_area( int *tlx, int *tly, int *brx, int *bry )
{
    TW_UINT16 rc;
    struct option_descriptor opt;

    if ((rc = sane_find_option( "tl-x", TYPE_FIXED, &opt )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_get_value( opt.optno, tlx )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_find_option( "tl-y", TYPE_FIXED, &opt )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_get_value( opt.optno, tly )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_find_option( "br-x", TYPE_FIXED, &opt )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_get_value( opt.optno, brx )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_find_option( "br-y", TYPE_FIXED, &opt )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_get_value( opt.optno, bry )) != TWCC_SUCCESS) return rc;
    if (opt.unit != UNIT_MM) FIXME( "unsupported unit %u\n", opt.unit );
    return rc;
}

TW_UINT16 sane_option_get_max_scan_area( int *tlx, int *tly, int *brx, int *bry )
{
    TW_UINT16 rc;
    struct option_descriptor opt;

    if ((rc = sane_find_option( "tl-x", TYPE_FIXED, &opt )) != TWCC_SUCCESS) return rc;
    *tlx = opt.constraint.range.min;
    if ((rc = sane_find_option( "tl-y", TYPE_FIXED, &opt )) != TWCC_SUCCESS) return rc;
    *tly = opt.constraint.range.min;
    if ((rc = sane_find_option( "br-x", TYPE_FIXED, &opt )) != TWCC_SUCCESS) return rc;
    *brx = opt.constraint.range.max;
    if ((rc = sane_find_option( "br-y", TYPE_FIXED, &opt )) != TWCC_SUCCESS) return rc;
    *bry = opt.constraint.range.max;
    if (opt.unit != UNIT_MM) FIXME( "unsupported unit %u\n", opt.unit );
    return rc;
}

TW_UINT16 sane_option_set_scan_area( int tlx, int tly, int brx, int bry, BOOL *reload )
{
    TW_UINT16 rc;
    struct option_descriptor opt;

    if ((rc = sane_find_option( "tl-x", TYPE_FIXED, &opt )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_set_value( opt.optno, &tlx, reload )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_find_option( "tl-y", TYPE_FIXED, &opt )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_set_value( opt.optno, &tly, reload )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_find_option( "br-x", TYPE_FIXED, &opt )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_set_value( opt.optno, &brx, reload )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_find_option( "br-y", TYPE_FIXED, &opt )) != TWCC_SUCCESS) return rc;
    if ((rc = sane_option_set_value( opt.optno, &bry, reload )) != TWCC_SUCCESS) return rc;
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
