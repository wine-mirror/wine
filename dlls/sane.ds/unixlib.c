/*
 * Unix library interface for SANE
 *
 * Copyright 2000 Shi Quan He
 * Copyright 2021 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <sane/sane.h>
#include <sane/saneopts.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "unixlib.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

static SANE_Handle device_handle;
static BOOL device_started;
static const SANE_Device **device_list;

/* Sane returns device names that are longer than the 32 bytes allowed
   by TWAIN.  However, it colon separates them, and the last bit is
   the most interesting.  So we use the last bit, and add a signature
   to ensure uniqueness */
static void copy_sane_short_name(const char *in, char *out, size_t outsize)
{
    const char *p;
    int  signature = 0;

    if (strlen(in) <= outsize - 1)
    {
        strcpy(out, in);
        return;
    }

    for (p = in; *p; p++)
        signature += *p;

    p = strrchr(in, ':');
    if (!p)
        p = in;
    else
        p++;

    if (strlen(p) > outsize - 7 - 1)
        p += strlen(p) - (outsize - 7 - 1);

    strcpy(out, p);
    sprintf(out + strlen(out), "(%04X)", signature % 0x10000);

}

static void detect_sane_devices(void)
{
    if (device_list && device_list[0]) return;
    TRACE("detecting sane...\n");
    sane_get_devices( &device_list, SANE_FALSE );
}

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
    unsigned int i, size, len = 0;
    WCHAR *p;

    descr->type = map_type( opt->type );
    descr->unit = map_unit( opt->unit );
    descr->constraint_type = map_constraint_type( opt->constraint_type );
    descr->size = opt->size;
    descr->is_active = SANE_OPTION_IS_ACTIVE( opt->cap );
    if (opt->title) len = ntdll_umbstowcs( opt->title, strlen(opt->title),
                                           descr->title, ARRAY_SIZE(descr->title) );
    descr->title[len] = 0;
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
            len = ntdll_umbstowcs( opt->constraint.string_list[i], strlen(opt->constraint.string_list[i]),
                                   p, size );
            p[len++] = 0;
            size -= len;
            p += len;
        }
        *p = 0;
        break;
    default:
        break;
    }
}

static NTSTATUS process_attach( void *args )
{
    SANE_Int version_code;

    sane_init( &version_code, NULL );
    return STATUS_SUCCESS;
}

static NTSTATUS process_detach( void *args )
{
    sane_exit();
    return STATUS_SUCCESS;
}

static NTSTATUS get_identity( void *args )
{
    TW_IDENTITY *id = args;
    static int cur_dev;

    detect_sane_devices();
    if (!device_list[cur_dev]) return STATUS_DEVICE_NOT_CONNECTED;
    id->ProtocolMajor = TWON_PROTOCOLMAJOR;
    id->ProtocolMinor = TWON_PROTOCOLMINOR;
    id->SupportedGroups = DG_CONTROL | DG_IMAGE | DF_DS2;
    copy_sane_short_name(device_list[cur_dev]->name, id->ProductName, sizeof(id->ProductName) - 1);
    lstrcpynA (id->Manufacturer, device_list[cur_dev]->vendor, sizeof(id->Manufacturer) - 1);
    lstrcpynA (id->ProductFamily, device_list[cur_dev]->model, sizeof(id->ProductFamily) - 1);
    cur_dev++;

    if (!device_list[cur_dev] || !device_list[cur_dev]->model	||
        !device_list[cur_dev]->vendor ||
	!device_list[cur_dev]->name)
	cur_dev = 0; /* wrap to begin */

    return STATUS_SUCCESS;
}

static NTSTATUS open_ds( void *args )
{
    TW_IDENTITY *id = args;
    SANE_Status status;
    int i;

    detect_sane_devices();
    if (!device_list[0])
    {
	ERR("No scanners? We should not get to OpenDS?\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    for (i = 0; device_list[i] && device_list[i]->model; i++)
    {
	TW_STR32 name;

	/* To make string as short as above */
	lstrcpynA(name, device_list[i]->vendor, sizeof(name)-1);
	if (*id->Manufacturer && strcmp(name, id->Manufacturer))
	    continue;
	lstrcpynA(name, device_list[i]->model, sizeof(name)-1);
	if (*id->ProductFamily && strcmp(name, id->ProductFamily))
	    continue;
        copy_sane_short_name(device_list[i]->name, name, sizeof(name) - 1);
	if (*id->ProductName && strcmp(name, id->ProductName))
	    continue;
	break;
    }
    if (!device_list[i]) {
	WARN("Scanner not found.\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    status = sane_open( device_list[i]->name, &device_handle );
    if (status == SANE_STATUS_GOOD) return STATUS_SUCCESS;

    ERR("sane_open(%s): %s\n", device_list[i]->name, sane_strstatus (status));
    return STATUS_DEVICE_NOT_CONNECTED;
}

static NTSTATUS close_ds( void *args )
{
    if (device_handle) sane_close( device_handle );
    device_handle = NULL;
    device_started = FALSE;
    return STATUS_SUCCESS;
}

static NTSTATUS start_device( void *args )
{
    SANE_Status status;

    if (device_started) return STATUS_SUCCESS;
    status = sane_start( device_handle );
    if (status != SANE_STATUS_GOOD)
    {
        TRACE("sane_start returns %s\n", sane_strstatus(status));
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    device_started = TRUE;
    return STATUS_SUCCESS;
}

static NTSTATUS cancel_device( void *args )
{
    if (device_started) sane_cancel( device_handle );
    device_started = FALSE;
    return STATUS_SUCCESS;
}

static NTSTATUS read_data( void *args )
{
    const struct read_data_params *params = args;
    unsigned char *buffer = params->buffer;
    int read_len, remaining = params->len;
    SANE_Status status;

    *params->retlen = 0;
    while (remaining)
    {
        status = sane_read( device_handle, buffer, remaining, &read_len );
        if (status != SANE_STATUS_GOOD) break;
        *params->retlen += read_len;
        buffer += read_len;
        remaining -= read_len;
    }
    if (status == SANE_STATUS_EOF) return TWCC_SUCCESS;
    return sane_status_to_twcc( status );
}

static NTSTATUS get_params( void *args )
{
    struct frame_parameters *params = args;
    SANE_Parameters sane_params;

    if (sane_get_parameters( device_handle, &sane_params )) return STATUS_UNSUCCESSFUL;

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
    return STATUS_SUCCESS;
}

static NTSTATUS option_get_value( void *args )
{
    const struct option_get_value_params *params = args;
    return sane_status_to_twcc( sane_control_option( device_handle, params->optno,
                                                    SANE_ACTION_GET_VALUE, params->val, NULL ));
}

static NTSTATUS option_set_value( void *args )
{
    const struct option_set_value_params *params = args;
    int status = 0;
    TW_UINT16 rc = sane_status_to_twcc( sane_control_option( device_handle, params->optno,
                                                             SANE_ACTION_SET_VALUE, params->val, &status ));
    if (rc == TWCC_SUCCESS && params->reload)
        *params->reload = (status & (SANE_INFO_RELOAD_OPTIONS | SANE_INFO_RELOAD_PARAMS | SANE_INFO_INEXACT)) != 0;
    return rc;
}

static NTSTATUS option_get_descriptor( void *args )
{
    struct option_descriptor *descr = args;
    const SANE_Option_Descriptor *opt = sane_get_option_descriptor( device_handle, descr->optno );

    if (!opt) return STATUS_NO_MORE_ENTRIES;
    map_descr( descr, opt );
    return STATUS_SUCCESS;
}

static NTSTATUS option_find_descriptor( void *args )
{
    const struct option_find_descriptor_params *params = args;
    struct option_descriptor *descr = params->descr;
    const SANE_Option_Descriptor *opt;
    int i;

    for (i = 1; (opt = sane_get_option_descriptor( device_handle, i )) != NULL; i++)
    {
        if (params->type != map_type( opt->type )) continue;
        if (strcmp( params->name, opt->name )) continue;
        descr->optno = i;
        map_descr( descr, opt );
        return STATUS_SUCCESS;
    }
    return STATUS_NO_MORE_ENTRIES;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    process_attach,
    process_detach,
    get_identity,
    open_ds,
    close_ds,
    start_device,
    cancel_device,
    read_data,
    get_params,
    option_get_value,
    option_set_value,
    option_get_descriptor,
    option_find_descriptor,
};

#ifdef _WIN64

typedef ULONG PTR32;

static NTSTATUS wow64_read_data( void *args )
{
    struct
    {
        PTR32 buffer;
        int   len;
        PTR32 retlen;
    } const *params32 = args;

    struct read_data_params params =
    {
        ULongToPtr(params32->buffer),
        params32->len,
        ULongToPtr(params32->retlen)
    };

    return read_data( &params );
}

static NTSTATUS wow64_option_get_value( void *args )
{
    struct
    {
        int   optno;
        PTR32 val;
    } const *params32 = args;

    struct option_get_value_params params =
    {
        params32->optno,
        ULongToPtr(params32->val)
    };

    return option_get_value( &params );
}

static NTSTATUS wow64_option_set_value( void *args )
{
    struct
    {
        int   optno;
        PTR32 val;
        PTR32 reload;
    } const *params32 = args;

    struct option_set_value_params params =
    {
        params32->optno,
        ULongToPtr(params32->val),
        ULongToPtr(params32->reload)
    };

    return option_set_value( &params );
}


static NTSTATUS wow64_option_find_descriptor( void *args )
{
    struct
    {
        PTR32 name;
        int   type;
        PTR32 descr;
    } const *params32 = args;

    struct option_find_descriptor_params params =
    {
        ULongToPtr(params32->name),
        params32->type,
        ULongToPtr(params32->descr)
    };

    return option_find_descriptor( &params );
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    process_attach,
    process_detach,
    get_identity,
    open_ds,
    close_ds,
    start_device,
    cancel_device,
    wow64_read_data,
    get_params,
    wow64_option_get_value,
    wow64_option_set_value,
    option_get_descriptor,
    wow64_option_find_descriptor,
};

#endif  /* _WIN64 */
