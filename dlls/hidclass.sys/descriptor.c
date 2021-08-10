/*
 * HID descriptor parsing
 *
 * Copyright (C) 2015 Aric Stewart
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
#include <stdio.h>
#include "hid.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

/* Flags that are defined in the document
   "Device Class Definition for Human Interface Devices" */
enum {
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

enum {
    TAG_TYPE_MAIN = 0x0,
    TAG_TYPE_GLOBAL,
    TAG_TYPE_LOCAL,
    TAG_TYPE_RESERVED,
};

enum {
    TAG_MAIN_INPUT = 0x08,
    TAG_MAIN_OUTPUT = 0x09,
    TAG_MAIN_FEATURE = 0x0B,
    TAG_MAIN_COLLECTION = 0x0A,
    TAG_MAIN_END_COLLECTION = 0x0C
};

enum {
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

enum {
    TAG_LOCAL_USAGE = 0x0,
    TAG_LOCAL_USAGE_MINIMUM,
    TAG_LOCAL_USAGE_MAXIMUM,
    TAG_LOCAL_DESIGNATOR_INDEX,
    TAG_LOCAL_DESIGNATOR_MINIMUM,
    TAG_LOCAL_DESIGNATOR_MAXIMUM,
    TAG_LOCAL_STRING_INDEX,
    TAG_LOCAL_STRING_MINIMUM,
    TAG_LOCAL_STRING_MAXIMUM,
    TAG_LOCAL_DELIMITER
};

static inline const char *debugstr_hid_value_caps( struct hid_value_caps *caps )
{
    if (!caps) return "(null)";
    return wine_dbg_sprintf( "RId %d, Usg %02x:%02x-%02x Dat %02x-%02x (%d), Str %d-%d (%d), Des %d-%d (%d), "
                             "Bits %02x, LCol %d LUsg %02x:%02x, BitSz %d, RCnt %d, Unit %x E%+d, Log %+d-%+d, Phy %+d-%+d",
                             caps->report_id, caps->usage_page, caps->usage_min, caps->usage_max, caps->data_index_min, caps->data_index_max, caps->is_range,
                             caps->string_min, caps->string_max, caps->is_string_range, caps->designator_min, caps->designator_max, caps->is_designator_range,
                             caps->bit_field, caps->link_collection, caps->link_usage_page, caps->link_usage, caps->bit_size, caps->report_count,
                             caps->units, caps->units_exp, caps->logical_min, caps->logical_max, caps->physical_min, caps->physical_max );
}

static void debug_print_preparsed( struct hid_preparsed_data *data )
{
    unsigned int i, end;
    if (TRACE_ON(hid))
    {
        TRACE( "START PREPARSED Data <<< Usage: %i, UsagePage: %i, "
               "InputReportByteLength: %i, tOutputReportByteLength: %i, "
               "FeatureReportByteLength: %i, NumberLinkCollectionNodes: %i, "
               "NumberInputButtonCaps: %i, NumberInputValueCaps: %i, "
               "NumberInputDataIndices: %i, NumberOutputButtonCaps: %i, "
               "NumberOutputValueCaps: %i, NumberOutputDataIndices: %i, "
               "NumberFeatureButtonCaps: %i, NumberFeatureValueCaps: %i, "
               "NumberFeatureDataIndices: %i\n",
               data->caps.Usage, data->caps.UsagePage, data->caps.InputReportByteLength,
               data->caps.OutputReportByteLength, data->caps.FeatureReportByteLength,
               data->caps.NumberLinkCollectionNodes, data->caps.NumberInputButtonCaps,
               data->caps.NumberInputValueCaps, data->caps.NumberInputDataIndices,
               data->caps.NumberOutputButtonCaps, data->caps.NumberOutputValueCaps,
               data->caps.NumberOutputDataIndices, data->caps.NumberFeatureButtonCaps,
               data->caps.NumberFeatureValueCaps, data->caps.NumberFeatureDataIndices );
        end = data->value_caps_count[HidP_Input];
        for (i = 0; i < end; i++) TRACE( "INPUT: %s\n", debugstr_hid_value_caps( HID_INPUT_VALUE_CAPS( data ) + i ) );
        end = data->value_caps_count[HidP_Output];
        for (i = 0; i < end; i++) TRACE( "OUTPUT: %s\n", debugstr_hid_value_caps( HID_OUTPUT_VALUE_CAPS( data ) + i ) );
        end = data->value_caps_count[HidP_Feature];
        for (i = 0; i < end; i++) TRACE( "FEATURE: %s\n", debugstr_hid_value_caps( HID_FEATURE_VALUE_CAPS( data ) + i ) );
        end = data->caps.NumberLinkCollectionNodes;
        for (i = 0; i < end; i++) TRACE( "COLLECTION: %s\n", debugstr_hid_value_caps( HID_COLLECTION_VALUE_CAPS( data ) + i ) );
        TRACE(">>> END Preparsed Data\n");
    }
}

struct hid_parser_state
{
    HIDP_CAPS caps;

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

    ULONG   bit_size[3][256];
    USHORT *byte_size[3]; /* pointers to caps */
    USHORT *value_idx[3]; /* pointers to caps */
    USHORT *data_idx[3]; /* pointers to caps */
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
    if (state->items.is_range) state->usages_size = 0;
    state->usages_page[state->usages_size] = usage_page;
    state->usages_min[state->usages_size] = usage;
    state->usages_max[state->usages_size] = usage;
    state->items.usage_min = usage;
    state->items.usage_max = usage;
    state->items.is_range = FALSE;
    if (state->usages_size++ == 255) ERR( "HID parser usages stack overflow!\n" );
    return state->usages_size <= 255;
}

static void parse_local_usage_min( struct hid_parser_state *state, USAGE usage_page, USAGE usage )
{
    if (!usage_page) usage_page = state->items.usage_page;
    if (!state->items.is_range) state->usages_max[0] = 0;
    state->usages_page[0] = usage_page;
    state->usages_min[0] = usage;
    state->items.usage_min = usage;
    state->items.is_range = TRUE;
    state->usages_size = 1;
}

static void parse_local_usage_max( struct hid_parser_state *state, USAGE usage_page, USAGE usage )
{
    if (!usage_page) usage_page = state->items.usage_page;
    if (!state->items.is_range) state->usages_min[0] = 0;
    state->usages_page[0] = usage_page;
    state->usages_max[0] = usage;
    state->items.usage_max = usage;
    state->items.is_range = TRUE;
    state->usages_size = 1;
}

static BOOL parse_new_collection( struct hid_parser_state *state )
{
    if (!array_reserve( &state->stack, &state->stack_size, state->collection_idx ))
    {
        ERR( "HID parser stack overflow!\n" );
        return FALSE;
    }

    if (!array_reserve( &state->collections, &state->collections_size, state->caps.NumberLinkCollectionNodes ))
    {
        ERR( "HID parser collections overflow!\n" );
        return FALSE;
    }

    copy_collection_items( state->stack + state->collection_idx, &state->items );
    state->collection_idx++;

    state->items.usage_min = state->usages_min[0];
    state->items.usage_max = state->usages_max[0];

    state->collections[state->caps.NumberLinkCollectionNodes] = state->items;
    state->items.link_collection = state->caps.NumberLinkCollectionNodes;
    state->items.link_usage_page = state->items.usage_page;
    state->items.link_usage = state->items.usage_min;
    if (!state->caps.NumberLinkCollectionNodes)
    {
        state->caps.UsagePage = state->items.usage_page;
        state->caps.Usage = state->items.usage_min;
    }
    state->caps.NumberLinkCollectionNodes++;

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

static BOOL parse_new_value_caps( struct hid_parser_state *state, HIDP_REPORT_TYPE type )
{
    struct hid_value_caps *value;
    USAGE usage_page = state->items.usage_page;
    DWORD usages_size = max(1, state->usages_size);
    USHORT *byte_size = state->byte_size[type];
    USHORT *value_idx = state->value_idx[type];
    USHORT *data_idx = state->data_idx[type];
    ULONG *bit_size = &state->bit_size[type][state->items.report_id];
    BOOL is_array;

    if (!*bit_size) *bit_size = 8;
    *bit_size += state->items.bit_size * state->items.report_count;
    *byte_size = max( *byte_size, (*bit_size + 7) / 8 );
    state->items.start_bit = *bit_size;

    if (!state->items.report_count)
    {
        reset_local_items( state );
        return TRUE;
    }

    if (!array_reserve( &state->values[type], &state->values_size[type], *value_idx + usages_size ))
    {
        ERR( "HID parser values overflow!\n" );
        return FALSE;
    }
    value = state->values[type] + *value_idx;

    state->items.start_index = 0;
    if (!(is_array = HID_VALUE_CAPS_IS_ARRAY( &state->items ))) state->items.report_count -= usages_size - 1;
    else state->items.start_bit -= state->items.report_count * state->items.bit_size;

    while (usages_size--)
    {
        if (!is_array) state->items.start_bit -= state->items.report_count * state->items.bit_size;
        else state->items.start_index += 1;
        state->items.usage_page = state->usages_page[usages_size];
        state->items.usage_min = state->usages_min[usages_size];
        state->items.usage_max = state->usages_max[usages_size];
        state->items.data_index_min = *data_idx;
        state->items.data_index_max = *data_idx + state->items.usage_max - state->items.usage_min;
        if (state->items.usage_max || state->items.usage_min) *data_idx = state->items.data_index_max + 1;
        *value++ = state->items;
        *value_idx += 1;
        if (!is_array) state->items.report_count = 1;
    }

    state->items.usage_page = usage_page;
    reset_local_items( state );
    return TRUE;
}

static void init_parser_state( struct hid_parser_state *state )
{
    memset( state, 0, sizeof(*state) );
    state->byte_size[HidP_Input] = &state->caps.InputReportByteLength;
    state->byte_size[HidP_Output] = &state->caps.OutputReportByteLength;
    state->byte_size[HidP_Feature] = &state->caps.FeatureReportByteLength;

    state->value_idx[HidP_Input] = &state->caps.NumberInputValueCaps;
    state->value_idx[HidP_Output] = &state->caps.NumberOutputValueCaps;
    state->value_idx[HidP_Feature] = &state->caps.NumberFeatureValueCaps;

    state->data_idx[HidP_Input] = &state->caps.NumberInputDataIndices;
    state->data_idx[HidP_Output] = &state->caps.NumberOutputDataIndices;
    state->data_idx[HidP_Feature] = &state->caps.NumberFeatureDataIndices;
}

static void free_parser_state( struct hid_parser_state *state )
{
    if (state->global_idx) ERR( "%u unpopped device caps on the stack\n", state->global_idx );
    if (state->collection_idx) ERR( "%u unpopped device collection on the stack\n", state->collection_idx );
    free( state->stack );
    free( state->collections );
    free( state->values[HidP_Input] );
    free( state->values[HidP_Output] );
    free( state->values[HidP_Feature] );
    free( state );
}

static struct hid_preparsed_data *build_preparsed_data( struct hid_parser_state *state )
{
    struct hid_preparsed_data *data;
    struct hid_value_caps *caps;
    DWORD i, button, filler, caps_len, size;

    caps_len = state->caps.NumberInputValueCaps + state->caps.NumberOutputValueCaps +
               state->caps.NumberFeatureValueCaps + state->caps.NumberLinkCollectionNodes;
    size = FIELD_OFFSET( struct hid_preparsed_data, value_caps[caps_len] );

    if (!(data = calloc( 1, size ))) return NULL;
    data->magic = HID_MAGIC;
    data->size = size;
    data->caps = state->caps;
    data->value_caps_count[HidP_Input] = state->caps.NumberInputValueCaps;
    data->value_caps_count[HidP_Output] = state->caps.NumberOutputValueCaps;
    data->value_caps_count[HidP_Feature] = state->caps.NumberFeatureValueCaps;

    /* fixup value vs button vs filler counts */

    caps = HID_INPUT_VALUE_CAPS( data );
    memcpy( caps, state->values[0], data->caps.NumberInputValueCaps * sizeof(*caps) );
    for (i = 0, button = 0, filler = 0; i < data->caps.NumberInputValueCaps; ++i)
    {
        if (!caps[i].usage_min && !caps[i].usage_max) filler++;
        else if (HID_VALUE_CAPS_IS_BUTTON( caps + i )) button++;
    }
    data->caps.NumberInputButtonCaps = button;
    data->caps.NumberInputValueCaps -= filler + button;

    caps = HID_OUTPUT_VALUE_CAPS( data );
    memcpy( caps, state->values[1], data->caps.NumberOutputValueCaps * sizeof(*caps) );
    for (i = 0, button = 0, filler = 0; i < data->caps.NumberOutputValueCaps; ++i)
    {
        if (!caps[i].usage_min && !caps[i].usage_max) filler++;
        else if (HID_VALUE_CAPS_IS_BUTTON( caps + i )) button++;
    }
    caps += data->caps.NumberOutputValueCaps;
    data->caps.NumberOutputButtonCaps = button;
    data->caps.NumberOutputValueCaps -= filler + button;

    caps = HID_FEATURE_VALUE_CAPS( data );
    memcpy( caps, state->values[2], data->caps.NumberFeatureValueCaps * sizeof(*caps) );
    for (i = 0, button = 0, filler = 0; i < data->caps.NumberFeatureValueCaps; ++i)
    {
        if (!caps[i].usage_min && !caps[i].usage_max) filler++;
        else if (HID_VALUE_CAPS_IS_BUTTON( caps + i )) button++;
    }
    caps += data->caps.NumberFeatureValueCaps;
    data->caps.NumberFeatureButtonCaps = button;
    data->caps.NumberFeatureValueCaps -= filler + button;

    caps = HID_COLLECTION_VALUE_CAPS( data );
    memcpy( caps, state->collections, data->caps.NumberLinkCollectionNodes * sizeof(*caps) );

    return data;
}

struct hid_preparsed_data *parse_descriptor( BYTE *descriptor, unsigned int length )
{
    struct hid_preparsed_data *data = NULL;
    struct hid_parser_state *state;
    UINT32 size, value;
    INT32 signed_value;
    BYTE *ptr, *end;
    int i;

    if (TRACE_ON( hid ))
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
    init_parser_state( state );

    for (ptr = descriptor, end = descriptor + length; ptr != end; ptr += size + 1)
    {
        size = (*ptr & 0x03);
        if (size == 3) size = 4;
        if (ptr + size > end)
        {
            ERR("Need %d bytes to read item value\n", size);
            goto done;
        }

        if (size == 0) signed_value = value = 0;
        else if (size == 1) signed_value = (INT8)(value = *(UINT8 *)(ptr + 1));
        else if (size == 2) signed_value = (INT16)(value = *(UINT16 *)(ptr + 1));
        else if (size == 4) signed_value = (INT32)(value = *(UINT32 *)(ptr + 1));
        else
        {
            ERR("Unexpected item value size %d.\n", size);
            goto done;
        }

        state->items.bit_field = value;

#define SHORT_ITEM(tag,type) (((tag)<<4)|((type)<<2))
        switch (*ptr & SHORT_ITEM(0xf,0x3))
        {
        case SHORT_ITEM(TAG_MAIN_INPUT, TAG_TYPE_MAIN):
            if (!parse_new_value_caps( state, HidP_Input )) goto done;
            break;
        case SHORT_ITEM(TAG_MAIN_OUTPUT, TAG_TYPE_MAIN):
            if (!parse_new_value_caps( state, HidP_Output )) goto done;
            break;
        case SHORT_ITEM(TAG_MAIN_FEATURE, TAG_TYPE_MAIN):
            if (!parse_new_value_caps( state, HidP_Feature )) goto done;
            break;
        case SHORT_ITEM(TAG_MAIN_COLLECTION, TAG_TYPE_MAIN):
            if (!parse_new_collection( state )) goto done;
            break;
        case SHORT_ITEM(TAG_MAIN_END_COLLECTION, TAG_TYPE_MAIN):
            if (!parse_end_collection( state )) goto done;
            break;

        case SHORT_ITEM(TAG_GLOBAL_USAGE_PAGE, TAG_TYPE_GLOBAL):
            state->items.usage_page = value;
            break;
        case SHORT_ITEM(TAG_GLOBAL_LOGICAL_MINIMUM, TAG_TYPE_GLOBAL):
            state->items.logical_min = signed_value;
            break;
        case SHORT_ITEM(TAG_GLOBAL_LOGICAL_MAXIMUM, TAG_TYPE_GLOBAL):
            state->items.logical_max = signed_value;
            break;
        case SHORT_ITEM(TAG_GLOBAL_PHYSICAL_MINIMUM, TAG_TYPE_GLOBAL):
            state->items.physical_min = signed_value;
            break;
        case SHORT_ITEM(TAG_GLOBAL_PHYSICAL_MAXIMUM, TAG_TYPE_GLOBAL):
            state->items.physical_max = signed_value;
            break;
        case SHORT_ITEM(TAG_GLOBAL_UNIT_EXPONENT, TAG_TYPE_GLOBAL):
            state->items.units_exp = signed_value;
            break;
        case SHORT_ITEM(TAG_GLOBAL_UNIT, TAG_TYPE_GLOBAL):
            state->items.units = signed_value;
            break;
        case SHORT_ITEM(TAG_GLOBAL_REPORT_SIZE, TAG_TYPE_GLOBAL):
            state->items.bit_size = value;
            break;
        case SHORT_ITEM(TAG_GLOBAL_REPORT_ID, TAG_TYPE_GLOBAL):
            state->items.report_id = value;
            break;
        case SHORT_ITEM(TAG_GLOBAL_REPORT_COUNT, TAG_TYPE_GLOBAL):
            state->items.report_count = value;
            break;
        case SHORT_ITEM(TAG_GLOBAL_PUSH, TAG_TYPE_GLOBAL):
            if (!parse_global_push( state )) goto done;
            break;
        case SHORT_ITEM(TAG_GLOBAL_POP, TAG_TYPE_GLOBAL):
            if (!parse_global_pop( state )) goto done;
            break;

        case SHORT_ITEM(TAG_LOCAL_USAGE, TAG_TYPE_LOCAL):
            if (!parse_local_usage( state, value >> 16, value & 0xffff )) goto done;
            break;
        case SHORT_ITEM(TAG_LOCAL_USAGE_MINIMUM, TAG_TYPE_LOCAL):
            parse_local_usage_min( state, value >> 16, value & 0xffff );
            break;
        case SHORT_ITEM(TAG_LOCAL_USAGE_MAXIMUM, TAG_TYPE_LOCAL):
            parse_local_usage_max( state, value >> 16, value & 0xffff );
            break;
        case SHORT_ITEM(TAG_LOCAL_DESIGNATOR_INDEX, TAG_TYPE_LOCAL):
            state->items.designator_min = state->items.designator_max = value;
            state->items.is_designator_range = FALSE;
            break;
        case SHORT_ITEM(TAG_LOCAL_DESIGNATOR_MINIMUM, TAG_TYPE_LOCAL):
            state->items.designator_min = value;
            state->items.is_designator_range = TRUE;
            break;
        case SHORT_ITEM(TAG_LOCAL_DESIGNATOR_MAXIMUM, TAG_TYPE_LOCAL):
            state->items.designator_max = value;
            state->items.is_designator_range = TRUE;
            break;
        case SHORT_ITEM(TAG_LOCAL_STRING_INDEX, TAG_TYPE_LOCAL):
            state->items.string_min = state->items.string_max = value;
            state->items.is_string_range = FALSE;
            break;
        case SHORT_ITEM(TAG_LOCAL_STRING_MINIMUM, TAG_TYPE_LOCAL):
            state->items.string_min = value;
            state->items.is_string_range = TRUE;
            break;
        case SHORT_ITEM(TAG_LOCAL_STRING_MAXIMUM, TAG_TYPE_LOCAL):
            state->items.string_max = value;
            state->items.is_string_range = TRUE;
            break;
        case SHORT_ITEM(TAG_LOCAL_DELIMITER, TAG_TYPE_LOCAL):
            FIXME("delimiter %d not implemented!\n", value);
            goto done;

        default:
            FIXME( "item type %x not implemented!\n", *ptr );
            goto done;
        }
#undef SHORT_ITEM
    }

    if ((data = build_preparsed_data( state ))) debug_print_preparsed( data );

done:
    free_parser_state( state );
    return data;
}
