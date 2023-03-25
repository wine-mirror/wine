/*
 * HID parsing library
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"

#include <ddk/wdm.h>
#include <ddk/hidpddi.h>

#include "wine/hid.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

/* Flags that are defined in the document
   "Device Class Definition for Human Interface Devices" */
enum
{
    INPUT_DATA_CONST = 0x01, /* Data (0)             | Constant (1)       */
    INPUT_ARRAY_VAR = 0x02,  /* Array (0)            | Variable (1)       */
    INPUT_ABS_REL = 0x04,    /* Absolute (0)         | Relative (1)       */
    INPUT_WRAP = 0x08,       /* No Wrap (0)          | Wrap (1)           */
    INPUT_LINEAR = 0x10,     /* Linear (0)           | Non Linear (1)     */
    INPUT_PREFSTATE = 0x20,  /* Preferred State (0)  | No Preferred (1)   */
    INPUT_NULL = 0x40,       /* No Null position (0) | Null state(1)      */
    INPUT_VOLATILE = 0x80,   /* Non Volatile (0)     | Volatile (1)       */
    INPUT_BITFIELD = 0x100   /* Bit Field (0)        | Buffered Bytes (1) */
};

enum
{
    TAG_TYPE_MAIN = 0x0,
    TAG_TYPE_GLOBAL,
    TAG_TYPE_LOCAL,
    TAG_TYPE_RESERVED,
};

enum
{
    TAG_MAIN_INPUT = 0x08,
    TAG_MAIN_OUTPUT = 0x09,
    TAG_MAIN_FEATURE = 0x0B,
    TAG_MAIN_COLLECTION = 0x0A,
    TAG_MAIN_END_COLLECTION = 0x0C
};

enum
{
    TAG_GLOBAL_USAGE_PAGE = 0x0,
    TAG_GLOBAL_LOGICAL_MINIMUM,
    TAG_GLOBAL_LOGICAL_MAXIMUM,
    TAG_GLOBAL_PHYSICAL_MINIMUM,
    TAG_GLOBAL_PHYSICAL_MAXIMUM,
    TAG_GLOBAL_UNIT_EXPONENT,
    TAG_GLOBAL_UNIT,
    TAG_GLOBAL_REPORT_SIZE,
    TAG_GLOBAL_REPORT_ID,
    TAG_GLOBAL_REPORT_COUNT,
    TAG_GLOBAL_PUSH,
    TAG_GLOBAL_POP
};

enum
{
    TAG_LOCAL_USAGE = 0x0,
    TAG_LOCAL_USAGE_MINIMUM,
    TAG_LOCAL_USAGE_MAXIMUM,
    TAG_LOCAL_DESIGNATOR_INDEX,
    TAG_LOCAL_DESIGNATOR_MINIMUM,
    TAG_LOCAL_DESIGNATOR_MAXIMUM,
    TAG_LOCAL_STRING_INDEX = 0x7,
    TAG_LOCAL_STRING_MINIMUM,
    TAG_LOCAL_STRING_MAXIMUM,
    TAG_LOCAL_DELIMITER
};

static inline const char *debugstr_hid_value_caps( struct hid_value_caps *caps )
{
    if (!caps) return "(null)";
    return wine_dbg_sprintf( "RId %d, Usg %02x:%02x-%02x Dat %02x-%02x, Str %d-%d, Des %d-%d, "
                             "Bits %02lx Flags %#lx, LCol %d LUsg %02x:%02x, BitSz %d, RCnt %d, Unit %lx E%+ld, Log %+ld-%+ld, Phy %+ld-%+ld",
                             caps->report_id, caps->usage_page, caps->usage_min, caps->usage_max, caps->data_index_min, caps->data_index_max,
                             caps->string_min, caps->string_max, caps->designator_min, caps->designator_max, caps->bit_field, caps->flags,
                             caps->link_collection, caps->link_usage_page, caps->link_usage, caps->bit_size, caps->report_count,
                             caps->units, caps->units_exp, caps->logical_min, caps->logical_max, caps->physical_min, caps->physical_max );
}

static inline const char *debugstr_hid_collection_node( struct hid_collection_node *node )
{
    if (!node) return "(null)";
    return wine_dbg_sprintf( "Usg %02x:%02x, Parent %u, Next %u, NbChild %u, Child %u, Type %02lx",
                             node->usage_page, node->usage, node->parent, node->next_sibling,
                             node->number_of_children, node->first_child, node->collection_type );
}

static void debug_print_preparsed( struct hid_preparsed_data *data )
{
    unsigned int i, end;

    if (TRACE_ON(hid))
    {
        TRACE( "usage %02x:%02x input %u-(%u)-%u, report len %u output %u-(%u)-%u, report len %u "
               "feature %u-(%u)-%u, report len %u collections %u\n", data->usage_page, data->usage,
                data->input_caps_start, data->input_caps_count, data->input_caps_end, data->input_report_byte_length,
                data->output_caps_start, data->output_caps_count, data->output_caps_end, data->output_report_byte_length,
                data->feature_caps_start, data->feature_caps_count, data->feature_caps_end, data->feature_report_byte_length,
                data->number_link_collection_nodes );
        end = data->input_caps_end - data->input_caps_start;
        for (i = 0; i < end; i++) TRACE( "input %d: %s\n", i, debugstr_hid_value_caps( HID_INPUT_VALUE_CAPS( data ) + i ) );
        end = data->output_caps_end - data->output_caps_start;
        for (i = 0; i < end; i++) TRACE( "output %d: %s\n", i, debugstr_hid_value_caps( HID_OUTPUT_VALUE_CAPS( data ) + i ) );
        end = data->feature_caps_end - data->feature_caps_start;
        for (i = 0; i < end; i++) TRACE( "feature %d: %s\n", i, debugstr_hid_value_caps( HID_FEATURE_VALUE_CAPS( data ) + i ) );
        end = data->number_link_collection_nodes;
        for (i = 0; i < end; i++) TRACE( "collection %d: %s\n", i, debugstr_hid_collection_node( HID_COLLECTION_NODES( data ) + i ) );
    }
}

struct hid_parser_state
{
    USAGE usage;
    USAGE usage_page;
    USHORT input_report_byte_length;
    USHORT output_report_byte_length;
    USHORT feature_report_byte_length;
    USHORT number_link_collection_nodes;

    USAGE usages_page[256];
    USAGE usages_min[256];
    USAGE usages_max[256];
    DWORD usages_size;

    struct hid_value_caps items;

    struct hid_value_caps *stack;
    DWORD                  stack_size;
    DWORD                  global_idx;
    DWORD                  collection_idx;

    struct hid_value_caps *collections;
    DWORD                  collections_size;

    struct hid_value_caps *values[3];
    ULONG                  values_size[3];

    ULONG  bit_size[3][256];
    USHORT byte_length[3];
    USHORT caps_count[3];
    USHORT empty_caps[3];
    USHORT data_count[3];
};

static BOOL array_reserve( struct hid_value_caps **array, DWORD *array_size, DWORD index )
{
    if (index < *array_size) return TRUE;
    if ((*array_size = *array_size ? (*array_size * 3 / 2) : 32) <= index) return FALSE;
    if (!(*array = realloc( *array, *array_size * sizeof(**array) ))) return FALSE;
    return TRUE;
}

static void copy_global_items( struct hid_value_caps *dst, const struct hid_value_caps *src )
{
    dst->usage_page = src->usage_page;
    dst->logical_min = src->logical_min;
    dst->logical_max = src->logical_max;
    dst->physical_min = src->physical_min;
    dst->physical_max = src->physical_max;
    dst->units_exp = src->units_exp;
    dst->units = src->units;
    dst->bit_size = src->bit_size;
    dst->report_id = src->report_id;
    dst->report_count = src->report_count;
}

static void copy_collection_items( struct hid_value_caps *dst, const struct hid_value_caps *src )
{
    dst->link_collection = src->link_collection;
    dst->link_usage_page = src->link_usage_page;
    dst->link_usage = src->link_usage;
}

static void reset_local_items( struct hid_parser_state *state )
{
    struct hid_value_caps tmp;
    copy_global_items( &tmp, &state->items );
    copy_collection_items( &tmp, &state->items );
    memset( &state->items, 0, sizeof(state->items) );
    copy_global_items( &state->items, &tmp );
    copy_collection_items( &state->items, &tmp );
    memset( &state->usages_page, 0, sizeof(state->usages_page) );
    memset( &state->usages_min, 0, sizeof(state->usages_min) );
    memset( &state->usages_max, 0, sizeof(state->usages_max) );
    state->usages_size = 0;
}

static BOOL parse_global_push( struct hid_parser_state *state )
{
    if (!array_reserve( &state->stack, &state->stack_size, state->global_idx ))
    {
        ERR( "HID parser stack overflow!\n" );
        return FALSE;
    }

    copy_global_items( state->stack + state->global_idx, &state->items );
    state->global_idx++;
    return TRUE;
}

static BOOL parse_global_pop( struct hid_parser_state *state )
{
    if (!state->global_idx)
    {
        ERR( "HID parser global stack underflow!\n" );
        return FALSE;
    }

    state->global_idx--;
    copy_global_items( &state->items, state->stack + state->global_idx );
    return TRUE;
}

static BOOL parse_local_usage( struct hid_parser_state *state, USAGE usage_page, USAGE usage )
{
    if (!usage_page) usage_page = state->items.usage_page;
    if (state->items.flags & HID_VALUE_CAPS_IS_RANGE) state->usages_size = 0;
    state->usages_page[state->usages_size] = usage_page;
    state->usages_min[state->usages_size] = usage;
    state->usages_max[state->usages_size] = usage;
    state->items.usage_min = usage;
    state->items.usage_max = usage;
    state->items.flags &= ~HID_VALUE_CAPS_IS_RANGE;
    if (state->usages_size++ == 255) ERR( "HID parser usages stack overflow!\n" );
    return state->usages_size <= 255;
}

static void parse_local_usage_min( struct hid_parser_state *state, USAGE usage_page, USAGE usage )
{
    if (!usage_page) usage_page = state->items.usage_page;
    if (!(state->items.flags & HID_VALUE_CAPS_IS_RANGE)) state->usages_max[0] = 0;
    state->usages_page[0] = usage_page;
    state->usages_min[0] = usage;
    state->items.usage_min = usage;
    state->items.flags |= HID_VALUE_CAPS_IS_RANGE;
    state->usages_size = 1;
}

static void parse_local_usage_max( struct hid_parser_state *state, USAGE usage_page, USAGE usage )
{
    if (!usage_page) usage_page = state->items.usage_page;
    if (!(state->items.flags & HID_VALUE_CAPS_IS_RANGE)) state->usages_min[0] = 0;
    state->usages_page[0] = usage_page;
    state->usages_max[0] = usage;
    state->items.usage_max = usage;
    state->items.flags |= HID_VALUE_CAPS_IS_RANGE;
    state->usages_size = 1;
}

static BOOL parse_new_collection( struct hid_parser_state *state )
{
    if (!array_reserve( &state->stack, &state->stack_size, state->collection_idx ))
    {
        ERR( "HID parser stack overflow!\n" );
        return FALSE;
    }

    if (!array_reserve( &state->collections, &state->collections_size, state->number_link_collection_nodes ))
    {
        ERR( "HID parser collections overflow!\n" );
        return FALSE;
    }

    copy_collection_items( state->stack + state->collection_idx, &state->items );
    state->collection_idx++;

    state->items.usage_min = state->usages_min[0];
    state->items.usage_max = state->usages_max[0];

    state->collections[state->number_link_collection_nodes] = state->items;
    state->items.link_collection = state->number_link_collection_nodes;
    state->items.link_usage_page = state->items.usage_page;
    state->items.link_usage = state->items.usage_min;
    if (!state->number_link_collection_nodes)
    {
        state->usage_page = state->items.usage_page;
        state->usage = state->items.usage_min;
    }
    state->number_link_collection_nodes++;

    reset_local_items( state );
    return TRUE;
}

static BOOL parse_end_collection( struct hid_parser_state *state )
{
    if (!state->collection_idx)
    {
        ERR( "HID parser collection stack underflow!\n" );
        return FALSE;
    }

    state->collection_idx--;
    copy_collection_items( &state->items, state->stack + state->collection_idx );
    reset_local_items( state );
    return TRUE;
}

static void add_new_value_caps( struct hid_parser_state *state, struct hid_value_caps *values,
                                LONG i, ULONG start_bit )
{
    DWORD count, usages_size = max( 1, state->usages_size );

    state->items.start_byte = start_bit / 8;
    state->items.start_bit = start_bit % 8;
    state->items.total_bits = state->items.report_count * state->items.bit_size;
    state->items.end_byte = (start_bit + state->items.total_bits + 7) / 8;
    state->items.usage_page = state->usages_page[usages_size - 1 - i];
    state->items.usage_min = state->usages_min[usages_size - 1 - i];
    state->items.usage_max = state->usages_max[usages_size - 1 - i];
    if (!state->items.usage_max && !state->items.usage_min) count = -1;
    else count = state->items.usage_max - state->items.usage_min;
    state->items.data_index_min = state->items.data_index_max + 1;
    state->items.data_index_max = state->items.data_index_min + count;
    values[i] = state->items;

    if (values[i].flags & HID_VALUE_CAPS_IS_BUTTON)
    {
        if (!HID_VALUE_CAPS_IS_ARRAY( values + i )) values[i].logical_min = 0;
        else values[i].logical_min = values[i].logical_max;
        values[i].logical_max = 0;
        values[i].physical_min = 0;
        values[i].physical_max = 0;
    }
}

static BOOL parse_new_value_caps( struct hid_parser_state *state, HIDP_REPORT_TYPE type )
{
    struct hid_value_caps *values;
    USAGE usage_page = state->items.usage_page;
    USHORT report_count = state->items.report_count;
    DWORD i, usages_size = max( 1, state->usages_size );
    USHORT *byte_length = &state->byte_length[type];
    ULONG start_bit, *bit_size = &state->bit_size[type][state->items.report_id];
    BOOL is_array;

    if (!*bit_size) *bit_size = 8;
    *bit_size += state->items.bit_size * state->items.report_count;
    *byte_length = max( *byte_length, (*bit_size + 7) / 8 );
    start_bit = *bit_size;

    if (!state->items.report_count)
    {
        state->empty_caps[type] += usages_size;
        reset_local_items( state );
        return TRUE;
    }

    if (!array_reserve( &state->values[type], &state->values_size[type],
                        state->caps_count[type] + usages_size ))
    {
        ERR( "HID parser values overflow!\n" );
        return FALSE;
    }
    values = state->values[type] + state->caps_count[type];

    if (!(is_array = HID_VALUE_CAPS_IS_ARRAY( &state->items ))) state->items.report_count -= usages_size - 1;
    else start_bit -= state->items.report_count * state->items.bit_size;

    if (!(state->items.bit_field & INPUT_ABS_REL)) state->items.flags |= HID_VALUE_CAPS_IS_ABSOLUTE;
    if (state->items.bit_field & INPUT_DATA_CONST) state->items.flags |= HID_VALUE_CAPS_IS_CONSTANT;
    if (state->items.bit_size == 1 || is_array) state->items.flags |= HID_VALUE_CAPS_IS_BUTTON;

    if (is_array) state->items.null_value = state->items.logical_min;
    else if (!(state->items.bit_field & INPUT_NULL)) state->items.null_value = 0;
    else state->items.null_value = 1;

    state->items.data_index_max = state->data_count[type] - 1;
    for (i = 0; i < usages_size; ++i)
    {
        if (!is_array) start_bit -= state->items.report_count * state->items.bit_size;
        else if (i) state->items.flags |= HID_VALUE_CAPS_ARRAY_HAS_MORE;
        else state->items.flags &= ~HID_VALUE_CAPS_ARRAY_HAS_MORE;
        add_new_value_caps( state, values, is_array ? usages_size - i - 1 : i, start_bit );
        if (!is_array) state->items.report_count = 1;
    }
    state->caps_count[type] += usages_size;
    state->data_count[type] = state->items.data_index_max + 1;

    state->items.usage_page = usage_page;
    state->items.report_count = report_count;
    reset_local_items( state );
    return TRUE;
}

static void free_parser_state( struct hid_parser_state *state )
{
    if (state->global_idx) ERR( "%lu unpopped device caps on the stack\n", state->global_idx );
    if (state->collection_idx) ERR( "%lu unpopped device collection on the stack\n", state->collection_idx );
    free( state->stack );
    free( state->collections );
    free( state->values[HidP_Input] );
    free( state->values[HidP_Output] );
    free( state->values[HidP_Feature] );
    free( state );
}

static struct hid_preparsed_data *build_preparsed_data( struct hid_parser_state *state, POOL_TYPE pool_type )
{
    struct hid_collection_node *nodes;
    struct hid_preparsed_data *data;
    struct hid_value_caps *caps;
    DWORD i, size, caps_size;

    caps_size = state->caps_count[HidP_Input] + state->caps_count[HidP_Output] +
                state->caps_count[HidP_Feature];
    caps_size += state->empty_caps[HidP_Input] + state->empty_caps[HidP_Output] +
                 state->empty_caps[HidP_Feature];
    caps_size *= sizeof(struct hid_value_caps);

    size = caps_size + FIELD_OFFSET(struct hid_preparsed_data, value_caps[0]) +
           state->number_link_collection_nodes * sizeof(struct hid_collection_node);
    if (!(data = ExAllocatePool( pool_type, size ))) return NULL;
    memset( data, 0, size );

    memcpy( data->magic, "HidP KDR", 8 );
    data->usage = state->usage;
    data->usage_page = state->usage_page;
    data->input_caps_start = 0;
    data->input_caps_count = state->caps_count[HidP_Input] + state->empty_caps[HidP_Input];
    data->input_caps_end = data->input_caps_start + state->caps_count[HidP_Input];
    data->input_report_byte_length = state->byte_length[HidP_Input];
    data->output_caps_start = data->input_caps_end;
    data->output_caps_count = state->caps_count[HidP_Output] + state->empty_caps[HidP_Output];
    data->output_caps_end = data->output_caps_start + state->caps_count[HidP_Output];
    data->output_report_byte_length = state->byte_length[HidP_Output];
    data->feature_caps_start = data->output_caps_end;
    data->feature_caps_count = state->caps_count[HidP_Feature] + state->empty_caps[HidP_Feature];
    data->feature_caps_end = data->feature_caps_start + state->caps_count[HidP_Feature];
    data->feature_report_byte_length = state->byte_length[HidP_Feature];
    data->caps_size = caps_size;
    data->number_link_collection_nodes = state->number_link_collection_nodes;

    caps = HID_INPUT_VALUE_CAPS( data );
    memcpy( caps, state->values[0], state->caps_count[HidP_Input] * sizeof(*caps) );
    caps = HID_OUTPUT_VALUE_CAPS( data );
    memcpy( caps, state->values[1], state->caps_count[HidP_Output] * sizeof(*caps) );
    caps = HID_FEATURE_VALUE_CAPS( data );
    memcpy( caps, state->values[2], state->caps_count[HidP_Feature] * sizeof(*caps) );

    nodes = HID_COLLECTION_NODES( data );
    for (i = 0; i < data->number_link_collection_nodes; ++i)
    {
        nodes[i].usage_page = state->collections[i].usage_page;
        nodes[i].usage = state->collections[i].usage_min;
        nodes[i].parent = state->collections[i].link_collection;
        nodes[i].collection_type = state->collections[i].bit_field;
        nodes[i].first_child = 0;
        nodes[i].next_sibling = 0;
        nodes[i].number_of_children = 0;

        if (i > 0)
        {
            nodes[i].next_sibling = nodes[nodes[i].parent].first_child;
            nodes[nodes[i].parent].first_child = i;
            nodes[nodes[i].parent].number_of_children++;
        }
    }

    return data;
}

struct hid_preparsed_data *parse_descriptor( BYTE *descriptor, unsigned int length, POOL_TYPE pool_type )
{
    struct hid_preparsed_data *data = NULL;
    struct hid_parser_state *state;
    UINT32 size, value;
    INT32 signed_value;
    BYTE *ptr, *end;
    int i;

    if (TRACE_ON(hid))
    {
        TRACE( "descriptor %p, length %u:\n", descriptor, length );
        for (i = 0; i < length;)
        {
            TRACE( "%08x ", i );
            do { TRACE( " %02x", descriptor[i] ); } while (++i % 16 && i < length);
            TRACE( "\n" );
        }
    }

    if (!(state = calloc( 1, sizeof(*state) ))) return NULL;

    for (ptr = descriptor, end = descriptor + length; ptr != end; ptr += size + 1)
    {
        size = (*ptr & 0x03);
        if (size == 3) size = 4;
        if (ptr + size > end)
        {
            ERR( "Need %d bytes to read item value\n", size );
            goto done;
        }

        if (size == 0) signed_value = value = 0;
        else if (size == 1) signed_value = (INT8)(value = *(UINT8 *)(ptr + 1));
        else if (size == 2) signed_value = (INT16)(value = *(UINT16 *)(ptr + 1));
        else if (size == 4) signed_value = (INT32)(value = *(UINT32 *)(ptr + 1));
        else
        {
            ERR( "Unexpected item value size %d.\n", size );
            goto done;
        }

        state->items.bit_field = value;

#define SHORT_ITEM( tag, type ) (((tag) << 4) | ((type) << 2))
        switch (*ptr & SHORT_ITEM( 0xf, 0x3 ))
        {
        case SHORT_ITEM( TAG_MAIN_INPUT, TAG_TYPE_MAIN ):
            if (!parse_new_value_caps( state, HidP_Input )) goto done;
            break;
        case SHORT_ITEM( TAG_MAIN_OUTPUT, TAG_TYPE_MAIN ):
            if (!parse_new_value_caps( state, HidP_Output )) goto done;
            break;
        case SHORT_ITEM( TAG_MAIN_FEATURE, TAG_TYPE_MAIN ):
            if (!parse_new_value_caps( state, HidP_Feature )) goto done;
            break;
        case SHORT_ITEM( TAG_MAIN_COLLECTION, TAG_TYPE_MAIN ):
            if (!parse_new_collection( state )) goto done;
            break;
        case SHORT_ITEM( TAG_MAIN_END_COLLECTION, TAG_TYPE_MAIN ):
            if (!parse_end_collection( state )) goto done;
            break;

        case SHORT_ITEM( TAG_GLOBAL_USAGE_PAGE, TAG_TYPE_GLOBAL ):
            state->items.usage_page = value;
            break;
        case SHORT_ITEM( TAG_GLOBAL_LOGICAL_MINIMUM, TAG_TYPE_GLOBAL ):
            state->items.logical_min = signed_value;
            break;
        case SHORT_ITEM( TAG_GLOBAL_LOGICAL_MAXIMUM, TAG_TYPE_GLOBAL ):
            state->items.logical_max = signed_value;
            break;
        case SHORT_ITEM( TAG_GLOBAL_PHYSICAL_MINIMUM, TAG_TYPE_GLOBAL ):
            state->items.physical_min = signed_value;
            break;
        case SHORT_ITEM( TAG_GLOBAL_PHYSICAL_MAXIMUM, TAG_TYPE_GLOBAL ):
            state->items.physical_max = signed_value;
            break;
        case SHORT_ITEM( TAG_GLOBAL_UNIT_EXPONENT, TAG_TYPE_GLOBAL ):
            state->items.units_exp = signed_value;
            break;
        case SHORT_ITEM( TAG_GLOBAL_UNIT, TAG_TYPE_GLOBAL ):
            state->items.units = signed_value;
            break;
        case SHORT_ITEM( TAG_GLOBAL_REPORT_SIZE, TAG_TYPE_GLOBAL ):
            state->items.bit_size = value;
            break;
        case SHORT_ITEM( TAG_GLOBAL_REPORT_ID, TAG_TYPE_GLOBAL ):
            state->items.report_id = value;
            break;
        case SHORT_ITEM( TAG_GLOBAL_REPORT_COUNT, TAG_TYPE_GLOBAL ):
            state->items.report_count = value;
            break;
        case SHORT_ITEM( TAG_GLOBAL_PUSH, TAG_TYPE_GLOBAL ):
            if (!parse_global_push( state )) goto done;
            break;
        case SHORT_ITEM( TAG_GLOBAL_POP, TAG_TYPE_GLOBAL ):
            if (!parse_global_pop( state )) goto done;
            break;

        case SHORT_ITEM( TAG_LOCAL_USAGE, TAG_TYPE_LOCAL ):
            if (!parse_local_usage( state, value >> 16, value & 0xffff )) goto done;
            break;
        case SHORT_ITEM( TAG_LOCAL_USAGE_MINIMUM, TAG_TYPE_LOCAL ):
            parse_local_usage_min( state, value >> 16, value & 0xffff );
            break;
        case SHORT_ITEM( TAG_LOCAL_USAGE_MAXIMUM, TAG_TYPE_LOCAL ):
            parse_local_usage_max( state, value >> 16, value & 0xffff );
            break;
        case SHORT_ITEM( TAG_LOCAL_DESIGNATOR_INDEX, TAG_TYPE_LOCAL ):
            state->items.designator_min = state->items.designator_max = value;
            state->items.flags &= ~HID_VALUE_CAPS_IS_DESIGNATOR_RANGE;
            break;
        case SHORT_ITEM( TAG_LOCAL_DESIGNATOR_MINIMUM, TAG_TYPE_LOCAL ):
            state->items.designator_min = value;
            state->items.flags |= HID_VALUE_CAPS_IS_DESIGNATOR_RANGE;
            break;
        case SHORT_ITEM( TAG_LOCAL_DESIGNATOR_MAXIMUM, TAG_TYPE_LOCAL ):
            state->items.designator_max = value;
            state->items.flags |= HID_VALUE_CAPS_IS_DESIGNATOR_RANGE;
            break;
        case SHORT_ITEM( TAG_LOCAL_STRING_INDEX, TAG_TYPE_LOCAL ):
            state->items.string_min = state->items.string_max = value;
            state->items.flags &= ~HID_VALUE_CAPS_IS_STRING_RANGE;
            break;
        case SHORT_ITEM( TAG_LOCAL_STRING_MINIMUM, TAG_TYPE_LOCAL ):
            state->items.string_min = value;
            state->items.flags |= HID_VALUE_CAPS_IS_STRING_RANGE;
            break;
        case SHORT_ITEM( TAG_LOCAL_STRING_MAXIMUM, TAG_TYPE_LOCAL ):
            state->items.string_max = value;
            state->items.flags |= HID_VALUE_CAPS_IS_STRING_RANGE;
            break;
        case SHORT_ITEM( TAG_LOCAL_DELIMITER, TAG_TYPE_LOCAL ):
            FIXME( "delimiter %d not implemented!\n", value );
            goto done;

        default:
            FIXME( "item type %x not implemented!\n", *ptr );
            break;
        }
#undef SHORT_ITEM
    }

    if ((data = build_preparsed_data( state, pool_type ))) debug_print_preparsed( data );

done:
    free_parser_state( state );
    return data;
}

NTSTATUS WINAPI HidP_GetCollectionDescription( PHIDP_REPORT_DESCRIPTOR report_desc, ULONG report_desc_len,
                                               POOL_TYPE pool_type, HIDP_DEVICE_DESC *device_desc )
{
    ULONG i, len, report_count = 0, input_len[256] = {0}, output_len[256] = {0}, feature_len[256] = {0};
    struct hid_value_caps *caps, *caps_end;
    struct hid_preparsed_data *preparsed;

    TRACE( "report_desc %p, report_desc_len %lu, pool_type %u, device_desc %p.\n",
            report_desc, report_desc_len, pool_type, device_desc );

    memset( device_desc, 0, sizeof(*device_desc) );

    if (!(preparsed = parse_descriptor( report_desc, report_desc_len, pool_type )))
        return HIDP_STATUS_INTERNAL_ERROR;

    if (!(device_desc->CollectionDesc = ExAllocatePool( pool_type, sizeof(*device_desc->CollectionDesc) )))
    {
        free( preparsed );
        return STATUS_NO_MEMORY;
    }

    len = preparsed->caps_size + FIELD_OFFSET(struct hid_preparsed_data, value_caps[0]) +
          preparsed->number_link_collection_nodes * sizeof(struct hid_collection_node);

    device_desc->CollectionDescLength = 1;
    device_desc->CollectionDesc[0].UsagePage = preparsed->usage_page;
    device_desc->CollectionDesc[0].Usage = preparsed->usage;
    device_desc->CollectionDesc[0].CollectionNumber = 1;
    device_desc->CollectionDesc[0].InputLength = preparsed->input_report_byte_length;
    device_desc->CollectionDesc[0].OutputLength = preparsed->output_report_byte_length;
    device_desc->CollectionDesc[0].FeatureLength = preparsed->feature_report_byte_length;
    device_desc->CollectionDesc[0].PreparsedDataLength = len;
    device_desc->CollectionDesc[0].PreparsedData = (PHIDP_PREPARSED_DATA)preparsed;

    caps = HID_INPUT_VALUE_CAPS( preparsed );
    caps_end = caps + preparsed->input_caps_end - preparsed->input_caps_start;
    for (; caps != caps_end; ++caps)
    {
        len = caps->start_byte * 8 + caps->start_bit + caps->bit_size * caps->report_count;
        if (!input_len[caps->report_id]) report_count++;
        input_len[caps->report_id] = max(input_len[caps->report_id], len);
    }

    caps = HID_OUTPUT_VALUE_CAPS( preparsed );
    caps_end = caps + preparsed->output_caps_end - preparsed->output_caps_start;
    for (; caps != caps_end; ++caps)
    {
        len = caps->start_byte * 8 + caps->start_bit + caps->bit_size * caps->report_count;
        if (!input_len[caps->report_id] && !output_len[caps->report_id]) report_count++;
        output_len[caps->report_id] = max(output_len[caps->report_id], len);
    }

    caps = HID_FEATURE_VALUE_CAPS( preparsed );
    caps_end = caps + preparsed->feature_caps_end - preparsed->feature_caps_start;
    for (; caps != caps_end; ++caps)
    {
        len = caps->start_byte * 8 + caps->start_bit + caps->bit_size * caps->report_count;
        if (!input_len[caps->report_id] && !output_len[caps->report_id] && !feature_len[caps->report_id]) report_count++;
        feature_len[caps->report_id] = max(feature_len[caps->report_id], len);
    }

    if (!(device_desc->ReportIDs = ExAllocatePool( pool_type, sizeof(*device_desc->ReportIDs) * report_count )))
    {
        free( preparsed );
        ExFreePool( device_desc->CollectionDesc );
        return STATUS_NO_MEMORY;
    }

    for (i = 0, report_count = 0; i < 256; ++i)
    {
        if (!input_len[i] && !output_len[i] && !feature_len[i]) continue;
        device_desc->ReportIDs[report_count].ReportID = i;
        device_desc->ReportIDs[report_count].CollectionNumber = 1;
        device_desc->ReportIDs[report_count].InputLength = (input_len[i] + 7) / 8;
        device_desc->ReportIDs[report_count].OutputLength = (output_len[i] + 7) / 8;
        device_desc->ReportIDs[report_count].FeatureLength = (feature_len[i] + 7) / 8;
        report_count++;
    }
    device_desc->ReportIDsLength = report_count;

    return HIDP_STATUS_SUCCESS;
}

void WINAPI HidP_FreeCollectionDescription( HIDP_DEVICE_DESC *device_desc )
{
    TRACE( "device_desc %p.\n", device_desc );

    ExFreePool( device_desc->CollectionDesc );
    ExFreePool( device_desc->ReportIDs );
}
