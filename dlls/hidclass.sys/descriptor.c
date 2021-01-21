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
#define NONAMELESSUNION
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

struct caps {
    USAGE UsagePage;
    LONG LogicalMin;
    LONG LogicalMax;
    LONG PhysicalMin;
    LONG PhysicalMax;
    ULONG UnitsExp;
    ULONG Units;
    USHORT BitSize;
    UCHAR ReportID;
    USHORT ReportCount;

    BOOLEAN  IsRange;
    BOOLEAN  IsStringRange;
    BOOLEAN  IsDesignatorRange;
    union {
        struct {
            USAGE UsageMin;
            USAGE UsageMax;
            USHORT StringMin;
            USHORT StringMax;
            USHORT DesignatorMin;
            USHORT DesignatorMax;
        } Range;
        struct  {
            USHORT Usage;
            USAGE Reserved1;
            USHORT StringIndex;
            USHORT Reserved2;
            USHORT DesignatorIndex;
            USHORT Reserved3;
        } NotRange;
    } DUMMYUNIONNAME;

    int Delim;
};

struct feature {
    struct list entry;
    struct caps caps;

    HIDP_REPORT_TYPE type;
    BOOLEAN isData;
    BOOLEAN isArray;
    BOOLEAN IsAbsolute;
    BOOLEAN Wrap;
    BOOLEAN Linear;
    BOOLEAN prefState;
    BOOLEAN HasNull;
    BOOLEAN Volatile;
    BOOLEAN BitField;

    unsigned int index;
    struct collection *collection;
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
    struct caps caps;
    unsigned int index;
    unsigned int type;
    struct collection *parent;
    struct list features;
    struct list collections;
};

struct caps_stack {
    struct list entry;
    struct caps caps;
};

static const char* debugstr_usages(struct caps *caps)
{
    if (!caps->IsRange)
        return wine_dbg_sprintf("[0x%x]", caps->u.NotRange.Usage);
    else
        return wine_dbg_sprintf("[0x%x - 0x%x]", caps->u.Range.UsageMin, caps->u.Range.UsageMax);
}

static const char* debugstr_stringindex(struct caps *caps)
{
    if (!caps->IsStringRange)
        return wine_dbg_sprintf("%i", caps->u.NotRange.StringIndex);
    else
        return wine_dbg_sprintf("[%i - %i]", caps->u.Range.StringMin, caps->u.Range.StringMax);
}

static const char* debugstr_designatorindex(struct caps *caps)
{
    if (!caps->IsDesignatorRange)
        return wine_dbg_sprintf("%i", caps->u.NotRange.DesignatorIndex);
    else
        return wine_dbg_sprintf("[%i - %i]", caps->u.Range.DesignatorMin, caps->u.Range.DesignatorMax);
}

static void debugstr_caps(const char* type, struct caps *caps)
{
    if (!caps)
        return;
    TRACE("(%s Caps: UsagePage 0x%x; LogicalMin %i; LogicalMax %i; PhysicalMin %i; "
            "PhysicalMax %i; UnitsExp %i; Units %i; BitSize %i; ReportID %i; ReportCount %i; "
            "Usage %s; StringIndex %s; DesignatorIndex %s; Delim %i;)\n",
    type,
    caps->UsagePage,
    caps->LogicalMin,
    caps->LogicalMax,
    caps->PhysicalMin,
    caps->PhysicalMax,
    caps->UnitsExp,
    caps->Units,
    caps->BitSize,
    caps->ReportID,
    caps->ReportCount,
    debugstr_usages(caps),
    debugstr_stringindex(caps),
    debugstr_designatorindex(caps),
    caps->Delim);
}

static void debug_feature(struct feature *feature)
{
    if (!feature)
        return;
    TRACE("[Feature type %s [%i]; %s; %s; %s; %s; %s; %s; %s; %s; %s]\n",
    feature_string[feature->type],
    feature->index,
    (feature->isData)?"Data":"Const",
    (feature->isArray)?"Array":"Var",
    (feature->IsAbsolute)?"Abs":"Rel",
    (feature->Wrap)?"Wrap":"NoWrap",
    (feature->Linear)?"Linear":"NonLinear",
    (feature->prefState)?"PrefStat":"NoPrefState",
    (feature->HasNull)?"HasNull":"NoNull",
    (feature->Volatile)?"Volatile":"NonVolatile",
    (feature->BitField)?"BitField":"Buffered");

    debugstr_caps("Feature", &feature->caps);
}

static void debug_collection(struct collection *collection)
{
    struct feature *fentry;
    struct collection *centry;
    if (TRACE_ON(hid))
    {
        TRACE("START Collection %i <<< %s, parent: %p,  %i features,  %i collections\n",
                collection->index, collection_string[collection->type], collection->parent,
                list_count(&collection->features), list_count(&collection->collections));
        debugstr_caps("Collection", &collection->caps);
        LIST_FOR_EACH_ENTRY(fentry, &collection->features, struct feature, entry)
            debug_feature(fentry);
        LIST_FOR_EACH_ENTRY(centry, &collection->collections, struct collection, entry)
            debug_collection(centry);
        TRACE(">>> END Collection %i\n", collection->index);
    }
}

static void debug_print_button_cap(const CHAR * type, WINE_HID_ELEMENT *wine_element)
{
    if (!wine_element->caps.button.IsRange)
        TRACE("%s Button: 0x%x/0x%04x: ReportId %i, startBit %i/1\n" , type,
            wine_element->caps.button.UsagePage,
            wine_element->caps.button.u.NotRange.Usage,
            wine_element->caps.value.ReportID,
            wine_element->valueStartBit);
    else
        TRACE("%s Button: 0x%x/[0x%04x-0x%04x]: ReportId %i, startBit %i/%i\n" ,type,
               wine_element->caps.button.UsagePage,
               wine_element->caps.button.u.Range.UsageMin,
               wine_element->caps.button.u.Range.UsageMax,
               wine_element->caps.value.ReportID,
               wine_element->valueStartBit,
               wine_element->bitCount);
}

static void debug_print_value_cap(const CHAR * type, WINE_HID_ELEMENT *wine_element)
{
    TRACE("%s Value: 0x%x/0x%x: ReportId %i, IsAbsolute %i, HasNull %i, "
          "Bit Size %i, ReportCount %i, UnitsExp %i, Units %i, "
          "LogicalMin %i, Logical Max %i, PhysicalMin %i, "
          "PhysicalMax %i -- StartBit %i/%i\n", type,
            wine_element->caps.value.UsagePage,
            wine_element->caps.value.u.NotRange.Usage,
            wine_element->caps.value.ReportID,
            wine_element->caps.value.IsAbsolute,
            wine_element->caps.value.HasNull,
            wine_element->caps.value.BitSize,
            wine_element->caps.value.ReportCount,
            wine_element->caps.value.UnitsExp,
            wine_element->caps.value.Units,
            wine_element->caps.value.LogicalMin,
            wine_element->caps.value.LogicalMax,
            wine_element->caps.value.PhysicalMin,
            wine_element->caps.value.PhysicalMax,
            wine_element->valueStartBit,
            wine_element->bitCount);
}

static void debug_print_element(const CHAR* type, WINE_HID_ELEMENT *wine_element)
{
    if (wine_element->ElementType == ButtonElement)
        debug_print_button_cap(type, wine_element);
    else if (wine_element->ElementType == ValueElement)
        debug_print_value_cap(type, wine_element);
    else
        TRACE("%s: UNKNOWN\n", type);
}

static void debug_print_report(const char* type, WINE_HIDP_PREPARSED_DATA *data,
        WINE_HID_REPORT *report)
{
    WINE_HID_ELEMENT *elem = HID_ELEMS(data);
    unsigned int i;
    TRACE("START Report %i <<< %s report : bitSize: %i elementCount: %i\n",
        report->reportID,
        type,
        report->bitSize,
        report->elementCount);
    for (i = 0; i < report->elementCount; i++)
    {
        debug_print_element(type, &elem[report->elementIdx + i]);
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
        for (i = 0; i < end; i++)
        {
            debug_print_report("FEATURE", data, &data->reports[i]);
        }
        TRACE(">>> END Preparsed Data\n");
    }
}

static int getValue(int bsize, int source, BOOL allow_negative)
{
    int mask = 0xff;
    int negative = 0x80;
    int outofrange = 0x100;
    int value;
    unsigned int i;

    if (bsize == 4)
        return source;

    for (i = 1; i < bsize; i++)
    {
        mask = (mask<<8) + 0xff;
        negative = (negative<<8);
        outofrange = (outofrange<<8);
    }
    value = (source&mask);
    if (allow_negative && value&negative)
        value = -1 * (outofrange - value);
    return value;
}

static void parse_io_feature(unsigned int bSize, int itemVal, int bTag,
                             unsigned int *feature_index,
                             struct feature *feature)
{
    if (bSize == 0)
    {
        return;
    }
    else
    {
        feature->isData = ((itemVal & INPUT_DATA_CONST) == 0);
        feature->isArray =  ((itemVal & INPUT_ARRAY_VAR) == 0);
        feature->IsAbsolute = ((itemVal & INPUT_ABS_REL) == 0);
        feature->Wrap = ((itemVal & INPUT_WRAP) != 0);
        feature->Linear = ((itemVal & INPUT_LINEAR) == 0);
        feature->prefState = ((itemVal & INPUT_PREFSTATE) == 0);
        feature->HasNull = ((itemVal & INPUT_NULL) != 0);

        if (bTag != TAG_MAIN_INPUT)
        {
            feature->Volatile = ((itemVal & INPUT_VOLATILE) != 0);
        }
        if (bSize > 1)
        {
            feature->BitField = ((itemVal & INPUT_BITFIELD) == 0);
        }
        feature->index = *feature_index;
        *feature_index = *feature_index + 1;
    }
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

static void new_caps(struct caps *caps)
{
    caps->IsRange = 0;
    caps->IsStringRange = 0;
    caps->IsDesignatorRange = 0;
    caps->u.NotRange.Usage = 0;
}

static int parse_descriptor(BYTE *descriptor, unsigned int index, unsigned int length,
                            unsigned int *feature_index, unsigned int *collection_index,
                            struct collection *collection, struct caps *caps,
                            struct list *stack)
{
    int usages_top = 0;
    USAGE usages[256];
    unsigned int i;

    for (i = index; i < length;)
    {
        BYTE b0 = descriptor[i++];
        int bSize = b0 & 0x03;
        int bType = (b0 >> 2) & 0x03;
        int bTag = (b0 >> 4) & 0x0F;

        bSize = (bSize == 3) ? 4 : bSize;
        if (bType == TAG_TYPE_RESERVED && bTag == 0x0F && bSize == 2 &&
            i + 2 < length)
        {
            /* Long data items: Should be unused */
            ERR("Long Data Item, should be unused\n");
        }
        else
        {
            int bSizeActual = 0;
            int itemVal = 0;
            unsigned int j;

            for (j = 0; j < bSize; j++)
            {
                if (i + j < length)
                {
                    itemVal += descriptor[i + j] << (8 * j);
                    bSizeActual++;
                }
            }
            TRACE(" 0x%x[%i], type %i , tag %i, size %i, val %i\n",b0,i-1,bType, bTag, bSize, itemVal );

            if (bType == TAG_TYPE_MAIN)
            {
                struct feature *feature;
                switch(bTag)
                {
                    case TAG_MAIN_INPUT:
                    case TAG_MAIN_OUTPUT:
                    case TAG_MAIN_FEATURE:
                        for (j = 0; j < caps->ReportCount; j++)
                        {
                            feature = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*feature));
                            list_add_tail(&collection->features, &feature->entry);
                            if (bTag == TAG_MAIN_INPUT)
                                feature->type = HidP_Input;
                            else if (bTag == TAG_MAIN_OUTPUT)
                                feature->type = HidP_Output;
                            else
                                feature->type = HidP_Feature;
                            parse_io_feature(bSize, itemVal, bTag, feature_index, feature);
                            if (j < usages_top)
                                caps->u.NotRange.Usage = usages[j];
                            feature->caps = *caps;
                            feature->caps.ReportCount = 1;
                            feature->collection = collection;
                            if (j+1 >= usages_top)
                            {
                                feature->caps.ReportCount += caps->ReportCount - (j + 1);
                                break;
                            }
                        }
                        usages_top = 0;
                        new_caps(caps);
                        break;
                    case TAG_MAIN_COLLECTION:
                    {
                        struct collection *subcollection = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct collection));
                        list_add_tail(&collection->collections, &subcollection->entry);
                        subcollection->parent = collection;
                        /* Only set our collection once...
                           We do not properly handle composite devices yet. */
                        if (usages_top)
                        {
                            caps->u.NotRange.Usage = usages[usages_top-1];
                            usages_top = 0;
                        }
                        if (*collection_index == 0)
                            collection->caps = *caps;
                        subcollection->caps = *caps;
                        subcollection->index = *collection_index;
                        *collection_index = *collection_index + 1;
                        list_init(&subcollection->features);
                        list_init(&subcollection->collections);
                        new_caps(caps);

                        parse_collection(bSize, itemVal, subcollection);

                        i = parse_descriptor(descriptor, i+1, length, feature_index, collection_index, subcollection, caps, stack);
                        continue;
                    }
                    case TAG_MAIN_END_COLLECTION:
                        return i;
                    default:
                        ERR("Unknown (bTag: 0x%x, bType: 0x%x)\n", bTag, bType);
                }
            }
            else if (bType == TAG_TYPE_GLOBAL)
            {
                switch(bTag)
                {
                    case TAG_GLOBAL_USAGE_PAGE:
                        caps->UsagePage = getValue(bSize, itemVal, FALSE);
                        break;
                    case TAG_GLOBAL_LOGICAL_MINIMUM:
                        caps->LogicalMin = getValue(bSize, itemVal, TRUE);
                        break;
                    case TAG_GLOBAL_LOGICAL_MAXIMUM:
                        caps->LogicalMax = getValue(bSize, itemVal, TRUE);
                        break;
                    case TAG_GLOBAL_PHYSICAL_MINIMUM:
                        caps->PhysicalMin = getValue(bSize, itemVal, TRUE);
                        break;
                    case TAG_GLOBAL_PHYSICAL_MAXIMUM:
                        caps->PhysicalMax = getValue(bSize, itemVal, TRUE);
                        break;
                    case TAG_GLOBAL_UNIT_EXPONENT:
                        caps->UnitsExp = getValue(bSize, itemVal, TRUE);
                        break;
                    case TAG_GLOBAL_UNIT:
                        caps->Units = getValue(bSize, itemVal, TRUE);
                        break;
                    case TAG_GLOBAL_REPORT_SIZE:
                        caps->BitSize = getValue(bSize, itemVal, FALSE);
                        break;
                    case TAG_GLOBAL_REPORT_ID:
                        caps->ReportID = getValue(bSize, itemVal, FALSE);
                        break;
                    case TAG_GLOBAL_REPORT_COUNT:
                        caps->ReportCount = getValue(bSize, itemVal, FALSE);
                        break;
                    case TAG_GLOBAL_PUSH:
                    {
                        struct caps_stack *saved = HeapAlloc(GetProcessHeap(), 0, sizeof(*saved));
                        saved->caps = *caps;
                        TRACE("Push\n");
                        list_add_tail(stack, &saved->entry);
                        break;
                    }
                    case TAG_GLOBAL_POP:
                    {
                        struct list *tail;
                        struct caps_stack *saved;
                        TRACE("Pop\n");
                        tail = list_tail(stack);
                        if (tail)
                        {
                            saved = LIST_ENTRY(tail, struct caps_stack, entry);
                            *caps = saved->caps;
                            list_remove(tail);
                            HeapFree(GetProcessHeap(), 0, saved);
                        }
                        else
                            ERR("Pop but no stack!\n");
                        break;
                    }
                    default:
                        ERR("Unknown (bTag: 0x%x, bType: 0x%x)\n", bTag, bType);
                }
            }
            else if (bType == TAG_TYPE_LOCAL)
            {
                switch(bTag)
                {
                    case TAG_LOCAL_USAGE:
                        if (usages_top == sizeof(usages))
                            ERR("More than 256 individual usages defined\n");
                        else
                        {
                            usages[usages_top++] = getValue(bSize, itemVal, FALSE);
                            caps->IsRange = FALSE;
                        }
                        break;
                    case TAG_LOCAL_USAGE_MINIMUM:
                        caps->u.Range.UsageMin = getValue(bSize, itemVal, FALSE);
                        caps->IsRange = TRUE;
                        break;
                    case TAG_LOCAL_USAGE_MAXIMUM:
                        caps->u.Range.UsageMax = getValue(bSize, itemVal, FALSE);
                        caps->IsRange = TRUE;
                        break;
                    case TAG_LOCAL_DESIGNATOR_INDEX:
                        caps->u.NotRange.DesignatorIndex = getValue(bSize, itemVal, FALSE);
                        caps->IsDesignatorRange = FALSE;
                        break;
                    case TAG_LOCAL_DESIGNATOR_MINIMUM:
                        caps->u.Range.DesignatorMin = getValue(bSize, itemVal, FALSE);
                        caps->IsDesignatorRange = TRUE;
                        break;
                    case TAG_LOCAL_DESIGNATOR_MAXIMUM:
                        caps->u.Range.DesignatorMax = getValue(bSize, itemVal, FALSE);
                        caps->IsDesignatorRange = TRUE;
                        break;
                    case TAG_LOCAL_STRING_INDEX:
                        caps->u.NotRange.StringIndex = getValue(bSize, itemVal, FALSE);
                        caps->IsStringRange = FALSE;
                        break;
                    case TAG_LOCAL_STRING_MINIMUM:
                        caps->u.Range.StringMin = getValue(bSize, itemVal, FALSE);
                        caps->IsStringRange = TRUE;
                        break;
                    case TAG_LOCAL_STRING_MAXIMUM:
                        caps->u.Range.StringMax = getValue(bSize, itemVal, FALSE);
                        caps->IsStringRange = TRUE;
                        break;
                    case TAG_LOCAL_DELIMITER:
                        caps->Delim = getValue(bSize, itemVal, FALSE);
                        break;
                    default:
                        ERR("Unknown (bTag: 0x%x, bType: 0x%x)\n", bTag, bType);
                }
            }
            else
                ERR("Unknown (bTag: 0x%x, bType: 0x%x)\n", bTag, bType);

            i += bSize;
        }
    }
    return i;
}

static void build_elements(WINE_HID_REPORT *wine_report, WINE_HID_ELEMENT *elems,
        struct feature* feature, USHORT *data_index)
{
    WINE_HID_ELEMENT *wine_element = elems + wine_report->elementIdx + wine_report->elementCount;

    if (!feature->isData)
    {
        wine_report->bitSize += feature->caps.BitSize * feature->caps.ReportCount;
        return;
    }

    wine_element->valueStartBit = wine_report->bitSize;

    wine_element->bitCount = (feature->caps.BitSize * feature->caps.ReportCount);
    wine_report->bitSize += wine_element->bitCount;

    if (feature->caps.BitSize == 1)
    {
        wine_element->ElementType = ButtonElement;
        wine_element->caps.button.UsagePage = feature->caps.UsagePage;
        wine_element->caps.button.ReportID = feature->caps.ReportID;
        wine_element->caps.button.BitField = feature->BitField;
        wine_element->caps.button.LinkCollection = feature->collection->index;
        wine_element->caps.button.LinkUsage = feature->collection->caps.u.NotRange.Usage;
        wine_element->caps.button.LinkUsagePage = feature->collection->caps.UsagePage;
        wine_element->caps.button.IsRange = feature->caps.IsRange;
        wine_element->caps.button.IsStringRange = feature->caps.IsStringRange;
        wine_element->caps.button.IsDesignatorRange = feature->caps.IsDesignatorRange;
        wine_element->caps.button.IsAbsolute = feature->IsAbsolute;
        if (wine_element->caps.button.IsRange)
        {
            wine_element->caps.button.u.Range.UsageMin = feature->caps.u.Range.UsageMin;
            wine_element->caps.button.u.Range.UsageMax = feature->caps.u.Range.UsageMax;
            wine_element->caps.button.u.Range.StringMin = feature->caps.u.Range.StringMin;
            wine_element->caps.button.u.Range.StringMax = feature->caps.u.Range.StringMax;
            wine_element->caps.button.u.Range.DesignatorMin = feature->caps.u.Range.DesignatorMin;
            wine_element->caps.button.u.Range.DesignatorMax = feature->caps.u.Range.DesignatorMax;
            wine_element->caps.button.u.Range.DataIndexMin = *data_index;
            wine_element->caps.button.u.Range.DataIndexMax = *data_index + wine_element->bitCount - 1;
            *data_index = *data_index + wine_element->bitCount;
        }
        else
        {
            wine_element->caps.button.u.NotRange.Usage = feature->caps.u.NotRange.Usage;
            wine_element->caps.button.u.NotRange.Reserved1 = feature->caps.u.NotRange.Usage;
            wine_element->caps.button.u.NotRange.StringIndex = feature->caps.u.NotRange.StringIndex;
            wine_element->caps.button.u.NotRange.Reserved2 = feature->caps.u.NotRange.StringIndex;
            wine_element->caps.button.u.NotRange.DesignatorIndex = feature->caps.u.NotRange.DesignatorIndex;
            wine_element->caps.button.u.NotRange.Reserved3 = feature->caps.u.NotRange.DesignatorIndex;
            wine_element->caps.button.u.NotRange.DataIndex = *data_index;
            wine_element->caps.button.u.NotRange.Reserved4 = *data_index;
            *data_index = *data_index + 1;
        }
    }
    else
    {
        wine_element->ElementType = ValueElement;
        wine_element->caps.value.UsagePage = feature->caps.UsagePage;
        wine_element->caps.value.ReportID = feature->caps.ReportID;
        wine_element->caps.value.BitField = feature->BitField;
        wine_element->caps.value.LinkCollection = feature->collection->index;
        wine_element->caps.value.LinkUsage = feature->collection->caps.u.NotRange.Usage;
        wine_element->caps.value.LinkUsagePage = feature->collection->caps.UsagePage;
        wine_element->caps.value.IsRange = feature->caps.IsRange;
        wine_element->caps.value.IsStringRange = feature->caps.IsStringRange;
        wine_element->caps.value.IsDesignatorRange = feature->caps.IsDesignatorRange;
        wine_element->caps.value.IsAbsolute = feature->IsAbsolute;
        wine_element->caps.value.HasNull = feature->HasNull;
        wine_element->caps.value.BitSize = feature->caps.BitSize;
        wine_element->caps.value.ReportCount = feature->caps.ReportCount;
        wine_element->caps.value.UnitsExp = feature->caps.UnitsExp;
        wine_element->caps.value.Units = feature->caps.Units;
        wine_element->caps.value.LogicalMin = feature->caps.LogicalMin;
        wine_element->caps.value.LogicalMax = feature->caps.LogicalMax;
        wine_element->caps.value.PhysicalMin = feature->caps.PhysicalMin;
        wine_element->caps.value.PhysicalMax = feature->caps.PhysicalMax;
        if (wine_element->caps.value.IsRange)
        {
            wine_element->caps.value.u.Range.UsageMin = feature->caps.u.Range.UsageMin;
            wine_element->caps.value.u.Range.UsageMax = feature->caps.u.Range.UsageMax;
            wine_element->caps.value.u.Range.StringMin = feature->caps.u.Range.StringMin;
            wine_element->caps.value.u.Range.StringMax = feature->caps.u.Range.StringMax;
            wine_element->caps.value.u.Range.DesignatorMin = feature->caps.u.Range.DesignatorMin;
            wine_element->caps.value.u.Range.DesignatorMax = feature->caps.u.Range.DesignatorMax;
            wine_element->caps.value.u.Range.DataIndexMin = *data_index;
            wine_element->caps.value.u.Range.DataIndexMax = *data_index +
                (wine_element->caps.value.u.Range.UsageMax -
                 wine_element->caps.value.u.Range.UsageMin);
            *data_index = *data_index +
                (wine_element->caps.value.u.Range.UsageMax -
                 wine_element->caps.value.u.Range.UsageMin) + 1;
        }
        else
        {
            wine_element->caps.value.u.NotRange.Usage = feature->caps.u.NotRange.Usage;
            wine_element->caps.value.u.NotRange.Reserved1 = feature->caps.u.NotRange.Usage;
            wine_element->caps.value.u.NotRange.StringIndex = feature->caps.u.NotRange.StringIndex;
            wine_element->caps.value.u.NotRange.Reserved2 = feature->caps.u.NotRange.StringIndex;
            wine_element->caps.value.u.NotRange.DesignatorIndex = feature->caps.u.NotRange.DesignatorIndex;
            wine_element->caps.value.u.NotRange.Reserved3 = feature->caps.u.NotRange.DesignatorIndex;
            wine_element->caps.value.u.NotRange.DataIndex = *data_index;
            wine_element->caps.value.u.NotRange.Reserved4 = *data_index;
            *data_index = *data_index + 1;
        }
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
    WINE_HID_LINK_COLLECTION_NODE *nodes = HID_NODES(data);
    struct feature *f;
    struct collection *c;
    struct list *entry;

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
                data->caps.InputReportByteLength =
                    max(data->caps.InputReportByteLength, (report->bitSize + 7) / 8);
                break;
            case HidP_Output:
                build_elements(report, elem, f, &data->caps.NumberOutputDataIndices);
                count_elements(f, &data->caps.NumberOutputButtonCaps, &data->caps.NumberOutputValueCaps);
                data->caps.OutputReportByteLength =
                    max(data->caps.OutputReportByteLength, (report->bitSize + 7) / 8);
                break;
            case HidP_Feature:
                build_elements(report, elem, f, &data->caps.NumberFeatureDataIndices);
                count_elements(f, &data->caps.NumberFeatureButtonCaps, &data->caps.NumberFeatureValueCaps);
                data->caps.FeatureReportByteLength =
                    max(data->caps.FeatureReportByteLength, (report->bitSize + 7) / 8);
                break;
        }
    }

    if (root != base)
    {
        nodes[base->index].LinkUsagePage = base->caps.UsagePage;
        nodes[base->index].LinkUsage = base->caps.u.NotRange.Usage;
        nodes[base->index].Parent = base->parent == root ? 0 : base->parent->index;
        nodes[base->index].CollectionType = base->type;
        nodes[base->index].IsAlias = 0;

        if ((entry = list_head(&base->collections)))
            nodes[base->index].FirstChild = LIST_ENTRY(entry, struct collection, entry)->index;
    }

    LIST_FOR_EACH_ENTRY(c, &base->collections, struct collection, entry)
    {
        preparse_collection(root, c, data, ctx);

        if ((entry = list_next(&base->collections, &c->entry)))
            nodes[c->index].NextSibling = LIST_ENTRY(entry, struct collection, entry)->index;
        if (root != base) nodes[base->index].NumberOfChildren++;
    }
}

static WINE_HIDP_PREPARSED_DATA* build_PreparseData(struct collection *base_collection, unsigned int node_count)
{
    WINE_HIDP_PREPARSED_DATA *data;
    unsigned int report_count;
    unsigned int size;

    struct preparse_ctx ctx;
    unsigned int element_off;
    unsigned int nodes_offset;

    memset(&ctx, 0, sizeof(ctx));
    create_preparse_ctx(base_collection, &ctx);

    report_count = ctx.report_count[HidP_Input] + ctx.report_count[HidP_Output]
        + ctx.report_count[HidP_Feature];
    element_off = FIELD_OFFSET(WINE_HIDP_PREPARSED_DATA, reports[report_count]);
    size = element_off + (ctx.elem_count * sizeof(WINE_HID_ELEMENT));

    nodes_offset = size;
    size += node_count * sizeof(WINE_HID_LINK_COLLECTION_NODE);

    data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    data->magic = HID_MAGIC;
    data->dwSize = size;
    data->caps.Usage = base_collection->caps.u.NotRange.Usage;
    data->caps.UsagePage = base_collection->caps.UsagePage;
    data->caps.NumberLinkCollectionNodes = node_count;
    data->elementOffset = element_off;
    data->nodesOffset = nodes_offset;

    preparse_collection(base_collection, base_collection, data, &ctx);
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
        HeapFree(GetProcessHeap(), 0, fentry);
    }
    HeapFree(GetProcessHeap(), 0, collection);
}

WINE_HIDP_PREPARSED_DATA* ParseDescriptor(BYTE *descriptor, unsigned int length)
{
    WINE_HIDP_PREPARSED_DATA *data = NULL;
    struct collection *base;
    struct caps caps;

    struct list caps_stack;

    unsigned int feature_count = 0;
    unsigned int cidx;

    if (TRACE_ON(hid))
    {
        TRACE("Descriptor[%i]: ", length);
        for (cidx = 0; cidx < length; cidx++)
        {
            TRACE("%x ",descriptor[cidx]);
            if ((cidx+1) % 80 == 0)
                TRACE("\n");
        }
        TRACE("\n");
    }

    list_init(&caps_stack);

    base = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*base));
    base->index = 1;
    list_init(&base->features);
    list_init(&base->collections);
    memset(&caps, 0, sizeof(caps));

    cidx = 0;
    parse_descriptor(descriptor, 0, length, &feature_count, &cidx, base, &caps, &caps_stack);

    debug_collection(base);

    if (!list_empty(&caps_stack))
    {
        struct caps_stack *entry, *cursor;
        ERR("%i unpopped device caps on the stack\n", list_count(&caps_stack));
        LIST_FOR_EACH_ENTRY_SAFE(entry, cursor, &caps_stack, struct caps_stack, entry)
        {
            list_remove(&entry->entry);
            HeapFree(GetProcessHeap(), 0, entry);
        }
    }

    data = build_PreparseData(base, cidx);
    debug_print_preparsed(data);
    free_collection(base);

    return data;
}
