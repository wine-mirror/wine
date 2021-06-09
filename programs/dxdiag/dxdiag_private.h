/*
 * Private definitions for the DirectX Diagnostic Tool
 *
 * Copyright 2011 Andrew Nguyen
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

#include <windef.h>
#include <winuser.h>

/* Resource definitions. */
#define MAX_STRING_LEN          1024

#define STRING_DXDIAG_TOOL      101
#define STRING_USAGE            102

/* Information collection definitions. */
struct system_information
{
    WCHAR *szTimeEnglish;
    WCHAR *szTimeLocalized;
    WCHAR *szMachineNameEnglish;
    WCHAR *szOSExLongEnglish;
    WCHAR *szOSExLocalized;
    WCHAR *szLanguagesEnglish;
    WCHAR *szLanguagesLocalized;
    WCHAR *szSystemManufacturerEnglish;
    WCHAR *szSystemModelEnglish;
    WCHAR *szBIOSEnglish;
    WCHAR *szProcessorEnglish;
    WCHAR *szPhysicalMemoryEnglish;
    WCHAR *szPageFileEnglish;
    WCHAR *szPageFileLocalized;
    WCHAR *szWindowsDir;
    WCHAR *szDirectXVersionLongEnglish;
    WCHAR *szSetupParamEnglish;
    WCHAR *szDxDiagVersion;
    BOOL win64;
};

struct dxdiag_information
{
    struct system_information system_info;
};

struct dxdiag_information *collect_dxdiag_information(BOOL whql_check);
void free_dxdiag_information(struct dxdiag_information *dxdiag_info);

/* Output backend definitions. */
enum output_type
{
    OUTPUT_NONE,
    OUTPUT_TEXT,
    OUTPUT_XML,
};

static inline const char *debugstr_output_type(enum output_type type)
{
    switch (type)
    {
    case OUTPUT_NONE:
        return "(none)";
    case OUTPUT_TEXT:
        return "Plain-text output";
    case OUTPUT_XML:
        return "XML output";
    default:
        return "(unknown)";
    }
}

const WCHAR *get_output_extension(enum output_type type);
BOOL output_dxdiag_information(struct dxdiag_information *dxdiag_info, const WCHAR *filename, enum output_type type);
