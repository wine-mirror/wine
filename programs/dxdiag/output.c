/*
 * DxDiag file information output
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>

#include "wine/debug.h"
#include "wine/unicode.h"

#include "dxdiag_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxdiag);

static char output_buffer[1024];
static const char crlf[2] = "\r\n";

static BOOL output_text_header(HANDLE hFile, const char *caption)
{
    DWORD len = strlen(caption);
    DWORD total_len = 3 * (len + sizeof(crlf));
    char *ptr = output_buffer;

    assert(total_len <= sizeof(output_buffer));

    memset(ptr, '-', len);
    ptr += len;

    memcpy(ptr, crlf, sizeof(crlf));
    ptr += sizeof(crlf);

    memcpy(ptr, caption, len);
    ptr += len;

    memcpy(ptr, crlf, sizeof(crlf));
    ptr += sizeof(crlf);

    memset(ptr, '-', len);
    ptr += len;

    memcpy(ptr, crlf, sizeof(crlf));
    ptr += sizeof(crlf);

    return WriteFile(hFile, output_buffer, total_len, NULL, NULL);
}

static BOOL output_text_field(HANDLE hFile, const char *field_name, DWORD field_width, const WCHAR *value)
{
    DWORD value_lenW = strlenW(value);
    DWORD value_lenA = WideCharToMultiByte(CP_ACP, 0, value, value_lenW, NULL, 0, NULL, NULL);
    DWORD total_len = field_width + sizeof(": ") - 1 + value_lenA + sizeof(crlf);
    char sprintf_fmt[1 + 10 + 3 + 1];
    char *ptr = output_buffer;

    assert(total_len <= sizeof(output_buffer));

    sprintf(sprintf_fmt, "%%%us: ", field_width);
    ptr += sprintf(ptr, sprintf_fmt, field_name);

    ptr += WideCharToMultiByte(CP_ACP, 0, value, value_lenW, ptr, value_lenA, NULL, NULL);

    memcpy(ptr, crlf, sizeof(crlf));
    ptr += sizeof(crlf);

    return WriteFile(hFile, output_buffer, total_len, NULL, NULL);
}

static BOOL output_crlf(HANDLE hFile)
{
    return WriteFile(hFile, crlf, sizeof(crlf), NULL, NULL);
}

static BOOL output_text_information(struct dxdiag_information *dxdiag_info, const WCHAR *filename)
{
    const struct information_block
    {
        const char *caption;
        size_t field_width;
        struct information_field
        {
            const char *field_name;
            const WCHAR *value;
        } fields[50];
    } output_table[] =
    {
        {"System Information", 19,
            {
                {"Time of this report", dxdiag_info->system_info.szTimeEnglish},
                {"Machine name", dxdiag_info->system_info.szMachineNameEnglish},
                {"Operating System", dxdiag_info->system_info.szOSExLongEnglish},
                {"Language", dxdiag_info->system_info.szLanguagesEnglish},
                {"System Manufacturer", dxdiag_info->system_info.szSystemManufacturerEnglish},
                {"System Model", dxdiag_info->system_info.szSystemManufacturerEnglish},
                {"BIOS", dxdiag_info->system_info.szSystemManufacturerEnglish},
                {"Processor", dxdiag_info->system_info.szProcessorEnglish},
                {"Memory", dxdiag_info->system_info.szPhysicalMemoryEnglish},
                {"Page File", dxdiag_info->system_info.szPageFileEnglish},
                {"Windows Dir", dxdiag_info->system_info.szWindowsDir},
                {"DirectX Version", dxdiag_info->system_info.szDirectXVersionLongEnglish},
                {"DX Setup Parameters", dxdiag_info->system_info.szSetupParamEnglish},
                {"DxDiag Version", dxdiag_info->system_info.szDxDiagVersion},
                {NULL, NULL},
            }
        },
    };

    HANDLE hFile;
    size_t i;

    hFile = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        WINE_ERR("File creation failed, last error %u\n", GetLastError());
        return FALSE;
    }

    for (i = 0; i < sizeof(output_table)/sizeof(output_table[0]); i++)
    {
        const struct information_field *fields = output_table[i].fields;
        unsigned int j;

        output_text_header(hFile, output_table[i].caption);
        for (j = 0; fields[j].field_name; j++)
            output_text_field(hFile, fields[j].field_name, output_table[i].field_width, fields[j].value);
        output_crlf(hFile);
    }

    CloseHandle(hFile);
    return FALSE;
}

static BOOL output_xml_information(struct dxdiag_information *dxdiag_info, const WCHAR *filename)
{
    WINE_FIXME("XML information output is not implemented\n");
    return FALSE;
}

static struct output_backend
{
    const WCHAR filename_ext[5];
    BOOL (*output_handler)(struct dxdiag_information *, const WCHAR *filename);
} output_backends[] =
{
    /* OUTPUT_TEXT */
    {
        {'.','t','x','t',0},
        output_text_information,
    },
    /* OUTPUT_XML */
    {
        {'.','x','m','l',0},
        output_xml_information,
    },
};

const WCHAR *get_output_extension(enum output_type type)
{
    assert(type > OUTPUT_NONE && type <= sizeof(output_backends)/sizeof(output_backends[0]));

    return output_backends[type - 1].filename_ext;
}

BOOL output_dxdiag_information(struct dxdiag_information *dxdiag_info, const WCHAR *filename, enum output_type type)
{
    assert(type > OUTPUT_NONE && type <= sizeof(output_backends)/sizeof(output_backends[0]));

    return output_backends[type - 1].output_handler(dxdiag_info, filename);
}
