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

#define COBJMACROS
#include <initguid.h>
#include <windows.h>
#include <msxml2.h>
#include <assert.h>
#include <stdio.h>

#include "wine/debug.h"
#include "wine/unicode.h"

#include "dxdiag_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxdiag);

static char output_buffer[1024];
static const char crlf[2] = "\r\n";

static const WCHAR DxDiag[] = {'D','x','D','i','a','g',0};

static const WCHAR SystemInformation[] = {'S','y','s','t','e','m','I','n','f','o','r','m','a','t','i','o','n',0};
static const WCHAR Time[] = {'T','i','m','e',0};
static const WCHAR MachineName[] = {'M','a','c','h','i','n','e','N','a','m','e',0};
static const WCHAR OperatingSystem[] = {'O','p','e','r','a','t','i','n','g','S','y','s','t','e','m',0};
static const WCHAR Language[] = {'L','a','n','g','u','a','g','e',0};
static const WCHAR SystemManufacturer[] = {'S','y','s','t','e','m','M','a','n','u','f','a','c','t','u','r','e','r',0};
static const WCHAR SystemModel[] = {'S','y','s','t','e','m','M','o','d','e','l',0};
static const WCHAR BIOS[] = {'B','I','O','S',0};
static const WCHAR Processor[] = {'P','r','o','c','e','s','s','o','r',0};
static const WCHAR Memory[] = {'M','e','m','o','r','y',0};
static const WCHAR PageFile[] = {'P','a','g','e','F','i','l','e',0};
static const WCHAR WindowsDir[] = {'W','i','n','d','o','w','s','D','i','r',0};
static const WCHAR DirectXVersion[] = {'D','i','r','e','c','t','X','V','e','r','s','i','o','n',0};
static const WCHAR DXSetupParameters[] = {'D','X','S','e','t','u','p','P','a','r','a','m','e','t','e','r','s',0};
static const WCHAR DxDiagVersion[] = {'D','x','D','i','a','g','V','e','r','s','i','o','n',0};
static const WCHAR DxDiagUnicode[] = {'D','x','D','i','a','g','U','n','i','c','o','d','e',0};
static const WCHAR DxDiag64Bit[] = {'D','x','D','i','a','g','6','4','B','i','t',0};

struct text_information_field
{
    const char *field_name;
    const WCHAR *value;
};

struct xml_information_field
{
    const WCHAR *tag_name;
    const WCHAR *value;
};

static BOOL output_text_header(HANDLE hFile, const char *caption)
{
    DWORD len = strlen(caption);
    DWORD total_len = 3 * (len + sizeof(crlf));
    char *ptr = output_buffer;
    DWORD bytes_written;

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

    return WriteFile(hFile, output_buffer, total_len, &bytes_written, NULL);
}

static BOOL output_text_field(HANDLE hFile, const char *field_name, DWORD field_width, const WCHAR *value)
{
    DWORD value_lenW = strlenW(value);
    DWORD value_lenA = WideCharToMultiByte(CP_ACP, 0, value, value_lenW, NULL, 0, NULL, NULL);
    DWORD total_len = field_width + sizeof(": ") - 1 + value_lenA + sizeof(crlf);
    char sprintf_fmt[1 + 10 + 3 + 1];
    char *ptr = output_buffer;
    DWORD bytes_written;

    assert(total_len <= sizeof(output_buffer));

    sprintf(sprintf_fmt, "%%%us: ", field_width);
    ptr += sprintf(ptr, sprintf_fmt, field_name);

    ptr += WideCharToMultiByte(CP_ACP, 0, value, value_lenW, ptr, value_lenA, NULL, NULL);
    memcpy(ptr, crlf, sizeof(crlf));

    return WriteFile(hFile, output_buffer, total_len, &bytes_written, NULL);
}

static BOOL output_crlf(HANDLE hFile)
{
    DWORD bytes_written;
    return WriteFile(hFile, crlf, sizeof(crlf), &bytes_written, NULL);
}

static inline void fill_system_text_output_table(struct dxdiag_information *dxdiag_info, struct text_information_field *fields)
{
    fields[0].field_name = "Time of this report";
    fields[0].value = dxdiag_info->system_info.szTimeEnglish;
    fields[1].field_name = "Machine name";
    fields[1].value = dxdiag_info->system_info.szMachineNameEnglish;
    fields[2].field_name = "Operating System";
    fields[2].value = dxdiag_info->system_info.szOSExLongEnglish;
    fields[3].field_name = "Language";
    fields[3].value = dxdiag_info->system_info.szLanguagesEnglish;
    fields[4].field_name = "System Manufacturer";
    fields[4].value = dxdiag_info->system_info.szSystemManufacturerEnglish;
    fields[5].field_name = "System Model";
    fields[5].value = dxdiag_info->system_info.szSystemModelEnglish;
    fields[6].field_name = "BIOS";
    fields[6].value = dxdiag_info->system_info.szBIOSEnglish;
    fields[7].field_name = "Processor";
    fields[7].value = dxdiag_info->system_info.szProcessorEnglish;
    fields[8].field_name = "Memory";
    fields[8].value = dxdiag_info->system_info.szPhysicalMemoryEnglish;
    fields[9].field_name = "Page File";
    fields[9].value = dxdiag_info->system_info.szPageFileEnglish;
    fields[10].field_name = "Windows Dir";
    fields[10].value = dxdiag_info->system_info.szWindowsDir;
    fields[11].field_name = "DirectX Version";
    fields[11].value = dxdiag_info->system_info.szDirectXVersionLongEnglish;
    fields[12].field_name = "DX Setup Parameters";
    fields[12].value = dxdiag_info->system_info.szSetupParamEnglish;
    fields[13].field_name = "DxDiag Version";
    fields[13].value = dxdiag_info->system_info.szDxDiagVersion;
}

static BOOL output_text_information(struct dxdiag_information *dxdiag_info, const WCHAR *filename)
{
    struct information_block
    {
        const char *caption;
        const size_t field_width;
        struct text_information_field fields[50];
    } output_table[] =
    {
        {"System Information", 19},
    };

    HANDLE hFile;
    size_t i;

    fill_system_text_output_table(dxdiag_info, output_table[0].fields);

    hFile = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        WINE_ERR("File creation failed, last error %u\n", GetLastError());
        return FALSE;
    }

    for (i = 0; i < ARRAY_SIZE(output_table); i++)
    {
        const struct text_information_field *fields = output_table[i].fields;
        unsigned int j;

        output_text_header(hFile, output_table[i].caption);
        for (j = 0; fields[j].field_name; j++)
            output_text_field(hFile, fields[j].field_name, output_table[i].field_width, fields[j].value);
        output_crlf(hFile);
    }

    CloseHandle(hFile);
    return FALSE;
}

static IXMLDOMElement *xml_create_element(IXMLDOMDocument *xmldoc, const WCHAR *name)
{
    BSTR bstr = SysAllocString(name);
    IXMLDOMElement *ret;
    HRESULT hr;

    if (!bstr)
        return NULL;

    hr = IXMLDOMDocument_createElement(xmldoc, bstr, &ret);
    SysFreeString(bstr);

    return SUCCEEDED(hr) ? ret : NULL;
}

static HRESULT xml_put_element_text(IXMLDOMElement *element, const WCHAR *text)
{
    BSTR bstr = SysAllocString(text);
    HRESULT hr;

    if (!bstr)
        return E_OUTOFMEMORY;

    hr = IXMLDOMElement_put_text(element, bstr);
    SysFreeString(bstr);

    return hr;
}

static HRESULT save_xml_document(IXMLDOMDocument *xmldoc, const WCHAR *filename)
{
    BSTR bstr = SysAllocString(filename);
    VARIANT destVar;
    HRESULT hr;

    if (!bstr)
        return E_OUTOFMEMORY;

    V_VT(&destVar) = VT_BSTR;
    V_BSTR(&destVar) = bstr;

    hr = IXMLDOMDocument_save(xmldoc, destVar);
    VariantClear(&destVar);

    return hr;
}

static inline void fill_system_xml_output_table(struct dxdiag_information *dxdiag_info, struct xml_information_field *fields)
{
    static const WCHAR zeroW[] = {'0',0};
    static const WCHAR oneW[] = {'1',0};

    fields[0].tag_name = Time;
    fields[0].value = dxdiag_info->system_info.szTimeEnglish;
    fields[1].tag_name = MachineName;
    fields[1].value = dxdiag_info->system_info.szMachineNameEnglish;
    fields[2].tag_name = OperatingSystem;
    fields[2].value = dxdiag_info->system_info.szOSExLongEnglish;
    fields[3].tag_name = Language;
    fields[3].value = dxdiag_info->system_info.szLanguagesEnglish;
    fields[4].tag_name = SystemManufacturer;
    fields[4].value = dxdiag_info->system_info.szSystemManufacturerEnglish;
    fields[5].tag_name = SystemModel;
    fields[5].value = dxdiag_info->system_info.szSystemModelEnglish;
    fields[6].tag_name = BIOS;
    fields[6].value = dxdiag_info->system_info.szBIOSEnglish;
    fields[7].tag_name = Processor;
    fields[7].value = dxdiag_info->system_info.szProcessorEnglish;
    fields[8].tag_name = Memory;
    fields[8].value = dxdiag_info->system_info.szPhysicalMemoryEnglish;
    fields[9].tag_name = PageFile;
    fields[9].value = dxdiag_info->system_info.szPageFileEnglish;
    fields[10].tag_name = WindowsDir;
    fields[10].value = dxdiag_info->system_info.szWindowsDir;
    fields[11].tag_name = DirectXVersion;
    fields[11].value = dxdiag_info->system_info.szDirectXVersionLongEnglish;
    fields[12].tag_name = DXSetupParameters;
    fields[12].value = dxdiag_info->system_info.szSetupParamEnglish;
    fields[13].tag_name = DxDiagVersion;
    fields[13].value = dxdiag_info->system_info.szDxDiagVersion;
    fields[14].tag_name = DxDiagUnicode;
    fields[14].value = oneW;
    fields[15].tag_name = DxDiag64Bit;
    fields[15].value = dxdiag_info->system_info.win64 ? oneW : zeroW;
}

static BOOL output_xml_information(struct dxdiag_information *dxdiag_info, const WCHAR *filename)
{
    struct information_block
    {
        const WCHAR *tag_name;
        struct xml_information_field fields[50];
    } output_table[] =
    {
        {SystemInformation},
    };

    IXMLDOMDocument *xmldoc = NULL;
    IXMLDOMElement *dxdiag_element = NULL;
    HRESULT hr;
    size_t i;

    fill_system_xml_output_table(dxdiag_info, output_table[0].fields);

    hr = CoCreateInstance(&CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IXMLDOMDocument, (void **)&xmldoc);
    if (FAILED(hr))
    {
        WINE_ERR("IXMLDOMDocument instance creation failed with 0x%08x\n", hr);
        goto error;
    }

    if (!(dxdiag_element = xml_create_element(xmldoc, DxDiag)))
        goto error;

    hr = IXMLDOMDocument_appendChild(xmldoc, (IXMLDOMNode *)dxdiag_element, NULL);
    if (FAILED(hr))
        goto error;

    for (i = 0; i < ARRAY_SIZE(output_table); i++)
    {
        IXMLDOMElement *info_element = xml_create_element(xmldoc, output_table[i].tag_name);
        const struct xml_information_field *fields = output_table[i].fields;
        unsigned int j = 0;

        if (!info_element)
            goto error;

        hr = IXMLDOMElement_appendChild(dxdiag_element, (IXMLDOMNode *)info_element, NULL);
        if (FAILED(hr))
        {
            IXMLDOMElement_Release(info_element);
            goto error;
        }

        for (j = 0; fields[j].tag_name; j++)
        {
            IXMLDOMElement *field_element = xml_create_element(xmldoc, fields[j].tag_name);

            if (!field_element)
            {
                IXMLDOMElement_Release(info_element);
                goto error;
            }

            hr = xml_put_element_text(field_element, fields[j].value);
            if (FAILED(hr))
            {
                IXMLDOMElement_Release(field_element);
                IXMLDOMElement_Release(info_element);
                goto error;
            }

            hr = IXMLDOMElement_appendChild(info_element, (IXMLDOMNode *)field_element, NULL);
            if (FAILED(hr))
            {
                IXMLDOMElement_Release(field_element);
                IXMLDOMElement_Release(info_element);
                goto error;
            }

            IXMLDOMElement_Release(field_element);
        }

        IXMLDOMElement_Release(info_element);
    }

    hr = save_xml_document(xmldoc, filename);
    if (FAILED(hr))
        goto error;

    IXMLDOMElement_Release(dxdiag_element);
    IXMLDOMDocument_Release(xmldoc);
    return TRUE;
error:
    if (dxdiag_element) IXMLDOMElement_Release(dxdiag_element);
    if (xmldoc) IXMLDOMDocument_Release(xmldoc);
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
    assert(type > OUTPUT_NONE && type <= ARRAY_SIZE(output_backends));

    return output_backends[type - 1].filename_ext;
}

BOOL output_dxdiag_information(struct dxdiag_information *dxdiag_info, const WCHAR *filename, enum output_type type)
{
    assert(type > OUTPUT_NONE && type <= ARRAY_SIZE(output_backends));

    return output_backends[type - 1].output_handler(dxdiag_info, filename);
}
