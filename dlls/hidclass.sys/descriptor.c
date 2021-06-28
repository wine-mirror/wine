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


static const char* const feature_string[] =
    { "Input", "Output", "Feature" };

struct feature {
    struct list entry;
    HIDP_VALUE_CAPS caps;

    HIDP_REPORT_TYPE type;
    BOOLEAN isData;
};

static const char* const collection_string[] = {
    "Physical",
    "Application",
    "Logical",
    "Report",
    "Named Array",
    "Usage Switch",
    "Usage Modifier",
};

struct collection {
    struct list entry;
    unsigned int type;
    struct collection *parent;
    struct list features;
    struct list collections;
};

static inline const char *debugstr_hidp_value_caps( HIDP_VALUE_CAPS *caps )
{
    if (!caps) return "(null)";
    return wine_dbg_sprintf( "RId %d, Usg %02x:%02x-%02x Dat %02x-%02x (%d), Str %d-%d (%d), Des %d-%d (%d), "
                             "Bits %02x, Als %d, Abs %d, Nul %d, LCol %d LUsg %02x:%02x, BitSz %d, RCnt %d, "
                             "Unit %x E%+d, Log %+d-%+d, Phy %+d-%+d",
                             caps->ReportID, caps->UsagePage, caps->Range.UsageMin, caps->Range.UsageMax, caps->Range.DataIndexMin, caps->Range.DataIndexMax, caps->IsRange,
                             caps->Range.StringMin, caps->Range.StringMax, caps->IsStringRange, caps->Range.DesignatorMin, caps->Range.DesignatorMax, caps->IsDesignatorRange,
                             caps->BitField, caps->IsAlias, caps->IsAbsolute, caps->HasNull, caps->LinkCollection, caps->LinkUsagePage, caps->LinkUsage, caps->BitSize, caps->ReportCount,
                             caps->Units, caps->UnitsExp, caps->LogicalMin, caps->LogicalMax, caps->PhysicalMin, caps->PhysicalMax );
}

static void copy_hidp_value_caps( HIDP_VALUE_CAPS *out, const struct hid_value_caps *in )
{
    out->UsagePage = in->usage_page;
    out->ReportID = in->report_id;
    out->LinkCollection = in->link_collection;
    out->LinkUsagePage = in->link_usage_page;
    out->LinkUsage = in->link_usage;
    out->BitField = in->bit_field;
    out->IsAlias = FALSE;
    out->IsAbsolute = HID_VALUE_CAPS_IS_ABSOLUTE( in );
    out->HasNull = HID_VALUE_CAPS_HAS_NULL( in );
    out->BitSize = in->bit_size;
    out->ReportCount = in->report_count;
    out->UnitsExp = in->units_exp;
    out->Units = in->units;
    out->LogicalMin = in->logical_min;
    out->LogicalMax = in->logical_max;
    out->PhysicalMin = in->physical_min;
    out->PhysicalMax = in->physical_max;
    if (!(out->IsRange = in->is_range))
        out->NotRange.Usage = in->usage_min;
    else
    {
        out->Range.UsageMin = in->usage_min;
        out->Range.UsageMax = in->usage_max;
    }
    if (!(out->IsStringRange = in->is_string_range))
        out->NotRange.StringIndex = in->string_min;
    else
    {
        out->Range.StringMin = in->string_min;
        out->Range.StringMax = in->string_max;
    }
    if ((out->IsDesignatorRange = in->is_designator_range))
        out->NotRange.DesignatorIndex = in->designator_min;
    else
    {
        out->Range.DesignatorMin = in->designator_min;
        out->Range.DesignatorMax = in->designator_max;
    }
}

static void debug_feature(struct feature *feature)
{
    if (!feature)
        return;
    TRACE( "[Feature type %s %s]\n", feature_string[feature->type], (feature->isData) ? "Data" : "Const" );

    TRACE("Feature %s\n", debugstr_hidp_value_caps(&feature->caps));
}

static void debug_collection(struct collection *collection)
{
    struct feature *fentry;
    struct collection *centry;
    if (TRACE_ON(hid))
    {
        TRACE( "START Collection <<< %s, parent: %p,  %i features,  %i collections\n",
               collection_string[collection->type], collection->parent,
               list_count( &collection->features ), list_count( &collection->collections ) );
        LIST_FOR_EACH_ENTRY(fentry, &collection->features, struct feature, entry)
            debug_feature(fentry);
        LIST_FOR_EACH_ENTRY(centry, &collection->collections, struct collection, entry)
            debug_collection(centry);
        TRACE( ">>> END Collection\n" );
    }
}

static void debug_print_report(const char* type, WINE_HIDP_PREPARSED_DATA *data,
        WINE_HID_REPORT *report)
{
    WINE_HID_ELEMENT *elems = HID_ELEMS(data);
    unsigned int i;
    TRACE("START Report %i <<< %s report : bitSize: %i elementCount: %i\n",
        report->reportID,
        type,
        report->bitSize,
        report->elementCount);
    for (i = 0; i < report->elementCount; i++)
    {
        WINE_HID_ELEMENT *elem = elems + report->elementIdx + i;
        TRACE("%s: %s, StartBit %d, BitCount %d\n", type, debugstr_hidp_value_caps(&elem->caps), elem->valueStartBit, elem->bitCount);
    }
    TRACE(">>> END Report %i\n",report->reportID);
}

static void debug_print_preparsed(WINE_HIDP_PREPARSED_DATA *data)
{
    unsigned int i, end;
    if (TRACE_ON(hid))
    {
        TRACE("START PREPARSED Data <<< dwSize: %i Usage: %i, UsagePage: %i, "
                "InputReportByteLength: %i, tOutputReportByteLength: %i, "
                "FeatureReportByteLength: %i, NumberLinkCollectionNodes: %i, "
                "NumberInputButtonCaps: %i, NumberInputValueCaps: %i, "
                "NumberInputDataIndices: %i, NumberOutputButtonCaps: %i, "
                "NumberOutputValueCaps: %i, NumberOutputDataIndices: %i, "
                "NumberFeatureButtonCaps: %i, NumberFeatureValueCaps: %i, "
                "NumberFeatureDataIndices: %i, reportCount[HidP_Input]: %i, "
                "reportCount[HidP_Output]: %i, reportCount[HidP_Feature]: %i, "
                "elementOffset: %i\n",
        data->dwSize,
        data->caps.Usage,
        data->caps.UsagePage,
        data->caps.InputReportByteLength,
        data->caps.OutputReportByteLength,
        data->caps.FeatureReportByteLength,
        data->caps.NumberLinkCollectionNodes,
        data->caps.NumberInputButtonCaps,
        data->caps.NumberInputValueCaps,
        data->caps.NumberInputDataIndices,
        data->caps.NumberOutputButtonCaps,
        data->caps.NumberOutputValueCaps,
        data->caps.NumberOutputDataIndices,
        data->caps.NumberFeatureButtonCaps,
        data->caps.NumberFeatureValueCaps,
        data->caps.NumberFeatureDataIndices,
        data->reportCount[HidP_Input],
        data->reportCount[HidP_Output],
        data->reportCount[HidP_Feature],
        data->elementOffset);

        end = data->reportCount[HidP_Input];
        for (i = 0; i < end; i++)
        {
            debug_print_report("INPUT", data, &data->reports[i]);
        }
        end += data->reportCount[HidP_Output];
        for (; i < end; i++)
        {
            debug_print_report("OUTPUT", data, &data->reports[i]);
        }
        end += data->reportCount[HidP_Feature];
        for (; i < end; i++)
        {
            debug_print_report("FEATURE", data, &data->reports[i]);
        }
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

static BOOL parse_new_value_caps( struct hid_parser_state *state, HIDP_REPORT_TYPE type, struct collection *collection )
{
    struct hid_value_caps *value;
    USAGE usage_page = state->items.usage_page;
    DWORD usages_size = max(1, state->usages_size);
    USHORT *byte_size = state->byte_size[type];
    USHORT *value_idx = state->value_idx[type];
    USHORT *data_idx = state->data_idx[type];
    ULONG *bit_size = &state->bit_size[type][state->items.report_id];
    struct feature *feature;
    int j;

    for (j = 0; j < state->items.report_count; j++)
    {
        if (!(feature = calloc( 1, sizeof(*feature) ))) return -1;
        list_add_tail( &collection->features, &feature->entry );
        feature->type = type;
        feature->isData = ((state->items.bit_field & INPUT_DATA_CONST) == 0);
        copy_hidp_value_caps( &feature->caps, &state->items );
        if (j < state->usages_size) feature->caps.NotRange.Usage = state->usages_min[j];
        feature->caps.ReportCount = 1;
        if (j + 1 >= state->usages_size)
        {
            feature->caps.ReportCount += state->items.report_count - (j + 1);
            break;
        }
    }

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

    state->items.report_count -= usages_size - 1;
    while (usages_size--)
    {
        state->items.start_bit -= state->items.report_count * state->items.bit_size;
        state->items.usage_page = state->usages_page[usages_size];
        state->items.usage_min = state->usages_min[usages_size];
        state->items.usage_max = state->usages_max[usages_size];
        state->items.data_index_min = *data_idx;
        state->items.data_index_max = *data_idx + state->items.usage_max - state->items.usage_min;
        if (state->items.usage_max || state->items.usage_min) *data_idx = state->items.data_index_max + 1;
        *value++ = state->items;
        *value_idx += 1;
        state->items.report_count = 1;
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

static void parse_collection(unsigned int bSize, int itemVal,
                             struct collection *collection)
{
    if (bSize)
    {
        collection->type = itemVal;

        if (itemVal >= 0x07 && itemVal <= 0x7F) {
            ERR(" (Reserved 0x%x )\n", itemVal);
        }
        else if (itemVal >= 0x80 && itemVal <= 0xFF) {
            ERR(" (Vendor Defined 0x%x )\n", itemVal);
        }
    }
}

static int parse_descriptor( BYTE *descriptor, unsigned int index, unsigned int length,
                             struct collection *collection, struct hid_parser_state *state )
{
    int i;
    UINT32 value;
    INT32 signed_value;

    for (i = index; i < length;)
    {
        BYTE item = descriptor[i++];
        int size = item & 0x03;

        if (size == 3) size = 4;
        if (length - i < size)
        {
            ERR("Need %d bytes to read item value\n", size);
            return -1;
        }

        if (size == 0) signed_value = value = 0;
        else if (size == 1) signed_value = (INT8)(value = *(UINT8 *)(descriptor + i));
        else if (size == 2) signed_value = (INT16)(value = *(UINT16 *)(descriptor + i));
        else if (size == 4) signed_value = (INT32)(value = *(UINT32 *)(descriptor + i));
        else
        {
            ERR("Unexpected item value size %d.\n", size);
            return -1;
        }
        i += size;

        state->items.bit_field = value;

#define SHORT_ITEM(tag,type) (((tag)<<4)|((type)<<2))
        switch (item & SHORT_ITEM(0xf,0x3))
        {
        case SHORT_ITEM(TAG_MAIN_INPUT, TAG_TYPE_MAIN):
            if (!parse_new_value_caps( state, HidP_Input, collection )) return -1;
            break;
        case SHORT_ITEM(TAG_MAIN_OUTPUT, TAG_TYPE_MAIN):
            if (!parse_new_value_caps( state, HidP_Output, collection )) return -1;
            break;
        case SHORT_ITEM(TAG_MAIN_FEATURE, TAG_TYPE_MAIN):
            if (!parse_new_value_caps( state, HidP_Feature, collection )) return -1;
            break;
        case SHORT_ITEM(TAG_MAIN_COLLECTION, TAG_TYPE_MAIN):
        {
            struct collection *subcollection;
            if (!(subcollection = calloc(1, sizeof(struct collection)))) return -1;
            list_add_tail(&collection->collections, &subcollection->entry);
            subcollection->parent = collection;
            /* Only set our collection once...
               We do not properly handle composite devices yet. */
            list_init(&subcollection->features);
            list_init(&subcollection->collections);
            parse_collection(size, value, subcollection);
            if (!parse_new_collection( state )) return -1;

            if ((i = parse_descriptor( descriptor, i, length, subcollection, state )) < 0) return i;
            continue;
        }
        case SHORT_ITEM(TAG_MAIN_END_COLLECTION, TAG_TYPE_MAIN):
            if (!parse_end_collection( state )) return -1;
            return i;

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
            if (!parse_global_push( state )) return -1;
            break;
        case SHORT_ITEM(TAG_GLOBAL_POP, TAG_TYPE_GLOBAL):
            if (!parse_global_pop( state )) return -1;
            break;

        case SHORT_ITEM(TAG_LOCAL_USAGE, TAG_TYPE_LOCAL):
            if (!parse_local_usage( state, value >> 16, value & 0xffff )) return -1;
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
            return -1;

        default:
            FIXME("item type %x not implemented!\n", item);
            return -1;
        }
#undef SHORT_ITEM
    }
    return i;
}

static void build_elements(WINE_HID_REPORT *wine_report, WINE_HID_ELEMENT *elems,
        struct feature* feature, USHORT *data_index)
{
    WINE_HID_ELEMENT *wine_element = elems + wine_report->elementIdx + wine_report->elementCount;
    ULONG index_count;

    if (!feature->isData)
    {
        wine_report->bitSize += feature->caps.BitSize * feature->caps.ReportCount;
        return;
    }

    wine_element->valueStartBit = wine_report->bitSize;

    wine_element->bitCount = (feature->caps.BitSize * feature->caps.ReportCount);
    wine_report->bitSize += wine_element->bitCount;

    wine_element->caps = feature->caps;

    if (wine_element->caps.IsRange)
    {
        if (wine_element->caps.BitSize == 1) index_count = wine_element->bitCount - 1;
        else index_count = wine_element->caps.Range.UsageMax - wine_element->caps.Range.UsageMin;
        wine_element->caps.Range.DataIndexMin = *data_index;
        wine_element->caps.Range.DataIndexMax = *data_index + index_count;
        *data_index = *data_index + index_count + 1;
    }
    else
    {
        wine_element->caps.NotRange.DataIndex = *data_index;
        wine_element->caps.NotRange.Reserved4 = *data_index;
        *data_index = *data_index + 1;
    }

    wine_report->elementCount++;
}

static void count_elements(struct feature* feature, USHORT *buttons, USHORT *values)
{
    if (!feature->isData)
        return;

    if (feature->caps.BitSize == 1)
        (*buttons)++;
    else
        (*values)++;
}

struct preparse_ctx
{
    int report_count[3];
    int elem_count;
    int report_elem_count[3][256];

    int elem_alloc;
    BOOL report_created[3][256];
};

static void create_preparse_ctx(const struct collection *base, struct preparse_ctx *ctx)
{
    struct feature *f;
    struct collection *c;

    LIST_FOR_EACH_ENTRY(f, &base->features, struct feature, entry)
    {
        ctx->elem_count++;
        ctx->report_elem_count[f->type][f->caps.ReportID]++;
        if (ctx->report_elem_count[f->type][f->caps.ReportID] != 1)
            continue;
        ctx->report_count[f->type]++;
    }

    LIST_FOR_EACH_ENTRY(c, &base->collections, struct collection, entry)
        create_preparse_ctx(c, ctx);
}

static void preparse_collection(const struct collection *root, const struct collection *base,
        WINE_HIDP_PREPARSED_DATA *data, struct preparse_ctx *ctx)
{
    WINE_HID_ELEMENT *elem = HID_ELEMS(data);
    struct feature *f;
    struct collection *c;

    LIST_FOR_EACH_ENTRY(f, &base->features, struct feature, entry)
    {
        WINE_HID_REPORT *report;

        if (!ctx->report_created[f->type][f->caps.ReportID])
        {
            ctx->report_created[f->type][f->caps.ReportID] = TRUE;
            data->reportIdx[f->type][f->caps.ReportID] = data->reportCount[f->type]++;
            if (f->type > 0) data->reportIdx[f->type][f->caps.ReportID] += ctx->report_count[0];
            if (f->type > 1) data->reportIdx[f->type][f->caps.ReportID] += ctx->report_count[1];

            report = &data->reports[data->reportIdx[f->type][f->caps.ReportID]];
            report->reportID = f->caps.ReportID;
            /* Room for the reportID */
            report->bitSize = 8;
            report->elementIdx = ctx->elem_alloc;
            ctx->elem_alloc += ctx->report_elem_count[f->type][f->caps.ReportID];
        }

        report = &data->reports[data->reportIdx[f->type][f->caps.ReportID]];
        switch (f->type)
        {
            case HidP_Input:
                build_elements(report, elem, f, &data->caps.NumberInputDataIndices);
                count_elements(f, &data->caps.NumberInputButtonCaps, &data->caps.NumberInputValueCaps);
                break;
            case HidP_Output:
                build_elements(report, elem, f, &data->caps.NumberOutputDataIndices);
                count_elements(f, &data->caps.NumberOutputButtonCaps, &data->caps.NumberOutputValueCaps);
                break;
            case HidP_Feature:
                build_elements(report, elem, f, &data->caps.NumberFeatureDataIndices);
                count_elements(f, &data->caps.NumberFeatureButtonCaps, &data->caps.NumberFeatureValueCaps);
                break;
        }
    }

    LIST_FOR_EACH_ENTRY(c, &base->collections, struct collection, entry)
        preparse_collection(root, c, data, ctx);
}

static WINE_HIDP_PREPARSED_DATA *build_preparsed_data( struct collection *base_collection,
                                                       struct hid_parser_state *state )
{
    WINE_HIDP_PREPARSED_DATA *data;
    struct hid_value_caps *caps;
    unsigned int report_count;
    unsigned int size;
    DWORD i, button, filler, caps_len, caps_off;

    struct preparse_ctx ctx;
    unsigned int element_off;

    memset(&ctx, 0, sizeof(ctx));
    create_preparse_ctx(base_collection, &ctx);

    report_count = ctx.report_count[HidP_Input] + ctx.report_count[HidP_Output]
        + ctx.report_count[HidP_Feature];
    element_off = FIELD_OFFSET(WINE_HIDP_PREPARSED_DATA, reports[report_count]);
    size = element_off + (ctx.elem_count * sizeof(WINE_HID_ELEMENT));

    caps_len = state->caps.NumberInputValueCaps + state->caps.NumberOutputValueCaps +
               state->caps.NumberFeatureValueCaps + state->caps.NumberLinkCollectionNodes;
    caps_off = size;
    size += caps_len * sizeof(*caps);

    if (!(data = calloc(1, size))) return NULL;
    data->magic = HID_MAGIC;
    data->dwSize = size;
    data->caps = state->caps;
    data->new_caps = state->caps;
    data->elementOffset = element_off;

    data->value_caps_offset = caps_off;
    data->value_caps_count[HidP_Input] = state->caps.NumberInputValueCaps;
    data->value_caps_count[HidP_Output] = state->caps.NumberOutputValueCaps;
    data->value_caps_count[HidP_Feature] = state->caps.NumberFeatureValueCaps;

    data->caps.NumberInputValueCaps = data->caps.NumberInputButtonCaps = data->caps.NumberInputDataIndices = 0;
    data->caps.NumberOutputValueCaps = data->caps.NumberOutputButtonCaps = data->caps.NumberOutputDataIndices = 0;
    data->caps.NumberFeatureValueCaps = data->caps.NumberFeatureButtonCaps = data->caps.NumberFeatureDataIndices = 0;
    preparse_collection(base_collection, base_collection, data, &ctx);

    /* fixup value vs button vs filler counts */

    caps = HID_INPUT_VALUE_CAPS( data );
    memcpy( caps, state->values[0], data->new_caps.NumberInputValueCaps * sizeof(*caps) );
    for (i = 0, button = 0, filler = 0; i < data->new_caps.NumberInputValueCaps; ++i)
    {
        if (!caps[i].usage_min && !caps[i].usage_max) filler++;
        else if (HID_VALUE_CAPS_IS_BUTTON( caps + i )) button++;
    }
    data->new_caps.NumberInputButtonCaps = button;
    data->new_caps.NumberInputValueCaps -= filler + button;

    caps = HID_OUTPUT_VALUE_CAPS( data );
    memcpy( caps, state->values[1], data->new_caps.NumberOutputValueCaps * sizeof(*caps) );
    for (i = 0, button = 0, filler = 0; i < data->new_caps.NumberOutputValueCaps; ++i)
    {
        if (!caps[i].usage_min && !caps[i].usage_max) filler++;
        else if (HID_VALUE_CAPS_IS_BUTTON( caps + i )) button++;
    }
    caps += data->new_caps.NumberOutputValueCaps;
    data->new_caps.NumberOutputButtonCaps = button;
    data->new_caps.NumberOutputValueCaps -= filler + button;

    caps = HID_FEATURE_VALUE_CAPS( data );
    memcpy( caps, state->values[2], data->new_caps.NumberFeatureValueCaps * sizeof(*caps) );
    for (i = 0, button = 0, filler = 0; i < data->new_caps.NumberFeatureValueCaps; ++i)
    {
        if (!caps[i].usage_min && !caps[i].usage_max) filler++;
        else if (HID_VALUE_CAPS_IS_BUTTON( caps + i )) button++;
    }
    caps += data->new_caps.NumberFeatureValueCaps;
    data->new_caps.NumberFeatureButtonCaps = button;
    data->new_caps.NumberFeatureValueCaps -= filler + button;

    caps = HID_COLLECTION_VALUE_CAPS( data );
    memcpy( caps, state->collections, data->new_caps.NumberLinkCollectionNodes * sizeof(*caps) );

    return data;
}

static void free_collection(struct collection *collection)
{
    struct feature *fentry, *fnext;
    struct collection *centry, *cnext;
    LIST_FOR_EACH_ENTRY_SAFE(centry, cnext, &collection->collections, struct collection, entry)
    {
        list_remove(&centry->entry);
        free_collection(centry);
    }
    LIST_FOR_EACH_ENTRY_SAFE(fentry, fnext, &collection->features, struct feature, entry)
    {
        list_remove(&fentry->entry);
        free(fentry);
    }
    free(collection);
}

WINE_HIDP_PREPARSED_DATA* ParseDescriptor(BYTE *descriptor, unsigned int length)
{
    WINE_HIDP_PREPARSED_DATA *data = NULL;
    struct hid_parser_state *state;
    struct collection *base;
    int i;

    if (TRACE_ON(hid))
    {
        TRACE("descriptor %p, length %u:\n", descriptor, length);
        for (i = 0; i < length;)
        {
            TRACE("%08x ", i);
            do { TRACE(" %02x", descriptor[i]); } while (++i % 16 && i < length);
            TRACE("\n");
        }
    }

    if (!(state = calloc( 1, sizeof(*state) ))) return NULL;
    if (!(base = calloc( 1, sizeof(*base) )))
    {
        free( state );
        return NULL;
    }
    list_init(&base->features);
    list_init(&base->collections);
    init_parser_state( state );

    if (parse_descriptor( descriptor, 0, length, base, state ) < 0)
    {
        free_collection(base);
        free_parser_state( state );
        return NULL;
    }

    debug_collection(base);

    if ((data = build_preparsed_data( base, state )))
        debug_print_preparsed(data);
    free_collection(base);

    free_parser_state( state );
    return data;
}
