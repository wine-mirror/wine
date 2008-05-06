/*
 * assembly parser
 *
 * Copyright 2008 James Hawkins
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winver.h"
#include "wincrypt.h"
#include "dbghelp.h"
#include "ole2.h"
#include "fusion.h"

#include "fusionpriv.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#define MAX_CLR_TABLES  64

typedef struct tagCLRTABLE
{
    DWORD rows;
    DWORD offset;
} CLRTABLE;

struct tagASSEMBLY
{
    LPSTR path;

    HANDLE hfile;
    HANDLE hmap;
    BYTE *data;

    IMAGE_NT_HEADERS32 *nthdr;
    IMAGE_COR20_HEADER *corhdr;

    METADATAHDR *metadatahdr;

    METADATATABLESHDR *tableshdr;
    DWORD numtables;
    DWORD *numrows;
    CLRTABLE tables[MAX_CLR_TABLES];

    BYTE *strings;
    BYTE *blobs;
};

const DWORD COR_TABLE_SIZES[64] =
{
    sizeof(MODULETABLE),
    sizeof(TYPEREFTABLE),
    sizeof(TYPEDEFTABLE),
    0,
    sizeof(FIELDTABLE),
    0,
    sizeof(METHODDEFTABLE),
    0,
    sizeof(PARAMTABLE),
    sizeof(INTERFACEIMPLTABLE),
    sizeof(MEMBERREFTABLE),
    sizeof(CONSTANTTABLE),
    sizeof(CUSTOMATTRIBUTETABLE),
    sizeof(FIELDMARSHALTABLE),
    sizeof(DECLSECURITYTABLE),
    sizeof(CLASSLAYOUTTABLE),
    sizeof(FIELDLAYOUTTABLE),
    sizeof(STANDALONESIGTABLE),
    sizeof(EVENTMAPTABLE),
    0,
    sizeof(EVENTTABLE),
    sizeof(PROPERTYMAPTABLE),
    0,
    sizeof(PROPERTYTABLE),
    sizeof(METHODSEMANTICSTABLE),
    sizeof(METHODIMPLTABLE),
    sizeof(MODULEREFTABLE),
    sizeof(TYPESPECTABLE),
    sizeof(IMPLMAPTABLE),
    sizeof(FIELDRVATABLE),
    0,
    0,
    sizeof(ASSEMBLYTABLE),
    sizeof(ASSEMBLYPROCESSORTABLE),
    sizeof(ASSEMBLYOSTABLE),
    sizeof(ASSEMBLYREFTABLE),
    sizeof(ASSEMBLYREFPROCESSORTABLE),
    sizeof(ASSEMBLYREFOSTABLE),
    sizeof(FILETABLE),
    sizeof(EXPORTEDTYPETABLE),
    sizeof(MANIFESTRESTABLE),
    sizeof(NESTEDCLASSTABLE),
    sizeof(GENERICPARAMTABLE),
    sizeof(METHODSPECTABLE),
    sizeof(GENERICPARAMCONSTRAINTTABLE),
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};

static LPSTR strdupWtoA(LPCWSTR str)
{
    LPSTR ret = NULL;
    DWORD len;

    if (!str)
        return ret;

    len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    ret = HeapAlloc(GetProcessHeap(), 0, len);
    if (ret)
        WideCharToMultiByte(CP_ACP, 0, str, -1, ret, len, NULL, NULL);

    return ret;
}

static DWORD rva_to_offset(IMAGE_NT_HEADERS *nthdrs, DWORD rva)
{
    DWORD offset = rva, limit;
    IMAGE_SECTION_HEADER *img;
    WORD i;

    img = IMAGE_FIRST_SECTION(nthdrs);

    if (rva < img->PointerToRawData)
        return rva;

    for (i = 0; i < nthdrs->FileHeader.NumberOfSections; i++)
    {
        if (img[i].SizeOfRawData)
            limit = img[i].SizeOfRawData;
        else
            limit = img[i].Misc.VirtualSize;

        if (rva >= img[i].VirtualAddress &&
            rva < (img[i].VirtualAddress + limit))
        {
            if (img[i].PointerToRawData != 0)
            {
                offset -= img[i].VirtualAddress;
                offset += img[i].PointerToRawData;
            }

            return offset;
        }
    }

    return 0;
}

static BYTE *GetData(BYTE *pData, ULONG *pLength)
{
    if ((*pData & 0x80) == 0x00)
    {
        *pLength = (*pData & 0x7f);
        return pData + 1;
    }

    if ((*pData & 0xC0) == 0x80)
    {
        *pLength = ((*pData & 0x3f) << 8 | *(pData + 1));
        return pData + 2;
    }

    if ((*pData & 0xE0) == 0xC0)
    {
        *pLength = ((*pData & 0x1f) << 24 | *(pData + 1) << 16 |
                    *(pData + 2) << 8 | *(pData + 3));
        return pData + 4;
    }

    *pLength = (ULONG)-1;
    return 0;
}

static VOID *assembly_data_offset(ASSEMBLY *assembly, ULONG offset)
{
    return (VOID *)&assembly->data[offset];
}

static HRESULT parse_clr_tables(ASSEMBLY *assembly, ULONG offset)
{
    DWORD i, previ, offidx;
    ULONG currofs;

    currofs = offset;
    assembly->tableshdr = (METADATATABLESHDR *)assembly_data_offset(assembly, currofs);
    if (!assembly->tableshdr)
        return E_FAIL;

    currofs += sizeof(METADATATABLESHDR);
    assembly->numrows = (DWORD *)assembly_data_offset(assembly, currofs);
    if (!assembly->numrows)
        return E_FAIL;

    assembly->numtables = 0;
    for (i = 0; i < MAX_CLR_TABLES; i++)
    {
        if ((i < 32 && (assembly->tableshdr->MaskValid.u.LowPart >> i) & 1) ||
            (i >= 32 && (assembly->tableshdr->MaskValid.u.HighPart >> i) & 1))
        {
            assembly->numtables++;
        }
    }

    currofs += assembly->numtables * sizeof(DWORD);
    memset(assembly->tables, -1, MAX_CLR_TABLES * sizeof(CLRTABLE));

    if (assembly->tableshdr->MaskValid.u.LowPart & 1)
    {
        assembly->tables[0].offset = currofs;
        assembly->tables[0].rows = assembly->numrows[0];
    }

    previ = 0;
    offidx = 1;
    for (i = 1; i < MAX_CLR_TABLES; i++)
    {
        if ((i < 32 && (assembly->tableshdr->MaskValid.u.LowPart >> i) & 1) ||
            (i >= 32 && (assembly->tableshdr->MaskValid.u.HighPart >> i) & 1))
        {
            currofs += COR_TABLE_SIZES[previ] * assembly->numrows[offidx - 1];
            assembly->tables[i].offset = currofs;
            assembly->tables[i].rows = assembly->numrows[offidx];
            offidx++;
            previ = i;
        }
    }

    return S_OK;
}

static HRESULT parse_clr_metadata(ASSEMBLY *assembly)
{
    METADATASTREAMHDR *streamhdr;
    ULONG rva, i, ofs;
    LPSTR stream;
    HRESULT hr;
    BYTE *ptr;

    rva = assembly->corhdr->MetaData.VirtualAddress;
    assembly->metadatahdr = ImageRvaToVa(assembly->nthdr, assembly->data,
                                         rva, NULL);
    if (!assembly->metadatahdr)
        return E_FAIL;

    ptr = ImageRvaToVa(assembly->nthdr, assembly->data,
                       rva + sizeof(METADATAHDR), NULL);
    if (!ptr)
        return E_FAIL;

    for (i = 0; i < assembly->metadatahdr->Streams; i++)
    {
        streamhdr = (METADATASTREAMHDR *)ptr;
        ofs = rva_to_offset(assembly->nthdr, rva + streamhdr->Offset);

        ptr += sizeof(METADATASTREAMHDR);
        stream = (LPSTR)ptr;

        if (!lstrcmpA(stream, "#~"))
        {
            hr = parse_clr_tables(assembly, ofs);
            if (FAILED(hr))
                return hr;
        }
        else if (!lstrcmpA(stream, "#Strings") || !lstrcmpA(stream, "Strings"))
            assembly->strings = (BYTE *)assembly_data_offset(assembly, ofs);
        else if (!lstrcmpA(stream, "#Blob"))
            assembly->blobs = (BYTE *)assembly_data_offset(assembly, ofs);

        ptr += lstrlenA(stream);
        while (!*ptr) ptr++;
    }

    return S_OK;
}

static HRESULT parse_pe_header(ASSEMBLY *assembly)
{
    IMAGE_DATA_DIRECTORY *datadirs;

    assembly->nthdr = ImageNtHeader(assembly->data);
    if (!assembly->nthdr)
        return E_FAIL;

    datadirs = assembly->nthdr->OptionalHeader.DataDirectory;
    if (!datadirs)
        return E_FAIL;

    if (!datadirs[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress ||
        !datadirs[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size)
    {
        return E_FAIL;
    }

    assembly->corhdr = ImageRvaToVa(assembly->nthdr, assembly->data,
        datadirs[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress, NULL);
    if (!assembly->corhdr)
        return E_FAIL;

    return S_OK;
}

HRESULT assembly_create(ASSEMBLY **out, LPCWSTR file)
{
    ASSEMBLY *assembly;
    HRESULT hr;

    *out = NULL;

    assembly = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ASSEMBLY));
    if (!assembly)
        return E_OUTOFMEMORY;

    assembly->path = strdupWtoA(file);
    if (!assembly->path)
    {
        hr = E_OUTOFMEMORY;
        goto failed;
    }

    assembly->hfile = CreateFileW(file, GENERIC_READ, FILE_SHARE_READ,
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (assembly->hfile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }

    assembly->hmap = CreateFileMappingW(assembly->hfile, NULL, PAGE_READONLY,
                                        0, 0, NULL);
    if (!assembly->hmap)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }

    assembly->data = MapViewOfFile(assembly->hmap, FILE_MAP_READ, 0, 0, 0);
    if (!assembly->data)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }

    hr = parse_pe_header(assembly);
    if (FAILED(hr)) goto failed;

    hr = parse_clr_metadata(assembly);
    if (FAILED(hr)) goto failed;

    *out = assembly;
    return S_OK;

failed:
    assembly_release( assembly );
    return hr;
}

HRESULT assembly_release(ASSEMBLY *assembly)
{
    if (!assembly)
        return S_OK;

    HeapFree(GetProcessHeap(), 0, assembly->path);
    UnmapViewOfFile(assembly->data);
    CloseHandle(assembly->hmap);
    CloseHandle(assembly->hfile);
    HeapFree(GetProcessHeap(), 0, assembly);

    return S_OK;
}

static LPSTR assembly_dup_str(ASSEMBLY *assembly, WORD index)
{
    return strdup((LPSTR)&assembly->strings[index]);
}

HRESULT assembly_get_name(ASSEMBLY *assembly, LPSTR *name)
{
    ASSEMBLYTABLE *asmtbl;
    ULONG offset;

    offset = assembly->tables[0x20].offset; /* FIXME: add constants */
    if (offset == -1)
        return E_FAIL;

    asmtbl = (ASSEMBLYTABLE *)assembly_data_offset(assembly, offset);
    if (!asmtbl)
        return E_FAIL;

    *name = assembly_dup_str(assembly, asmtbl->Name);
    if (!*name)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT assembly_get_path(ASSEMBLY *assembly, LPSTR *path)
{
    *path = strdup(assembly->path);
    if (!*path)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT assembly_get_version(ASSEMBLY *assembly, LPSTR *version)
{
    LPSTR verdata;
    VS_FIXEDFILEINFO *ffi;
    HRESULT hr = S_OK;
    DWORD size;

    size = GetFileVersionInfoSizeA(assembly->path, NULL);
    if (!size)
        return HRESULT_FROM_WIN32(GetLastError());

    verdata = HeapAlloc(GetProcessHeap(), 0, size);
    if (!verdata)
        return E_OUTOFMEMORY;

    if (!GetFileVersionInfoA(assembly->path, 0, size, verdata))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    if (!VerQueryValueA(verdata, "\\", (LPVOID *)&ffi, &size))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    *version = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
    if (!*version)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    sprintf(*version, "%d.%d.%d.%d", HIWORD(ffi->dwFileVersionMS),
            LOWORD(ffi->dwFileVersionMS), HIWORD(ffi->dwFileVersionLS),
            LOWORD(ffi->dwFileVersionLS));

done:
    HeapFree(GetProcessHeap(), 0, verdata);
    return hr;
}

HRESULT assembly_get_architecture(ASSEMBLY *assembly, DWORD fixme)
{
    /* FIXME */
    return S_OK;
}

static BYTE *assembly_get_blob(ASSEMBLY *assembly, WORD index, ULONG *size)
{
    return GetData(&assembly->blobs[index], size);
}

static void bytes_to_str(BYTE *bytes, DWORD len, LPSTR str)
{
    int i;

    static const char hexval[16] = {
        '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
    };

    for(i = 0; i < len; i++)
    {
        str[i * 2] = hexval[((bytes[i] >> 4) & 0xF)];
        str[i * 2 + 1] = hexval[(bytes[i]) & 0x0F];
    }
}

#define BYTES_PER_TOKEN 8
#define CHARS_PER_BYTE 2
#define TOKEN_LENGTH (BYTES_PER_TOKEN * CHARS_PER_BYTE + 1)

HRESULT assembly_get_pubkey_token(ASSEMBLY *assembly, LPSTR *token)
{
    ASSEMBLYTABLE *asmtbl;
    ULONG i, offset, size;
    BYTE *hashdata;
    HCRYPTPROV crypt;
    HCRYPTHASH hash;
    BYTE *pubkey;
    BYTE tokbytes[BYTES_PER_TOKEN];
    HRESULT hr = E_FAIL;
    LPSTR tok;

    *token = NULL;

    offset = assembly->tables[0x20].offset; /* FIXME: add constants */
    if (offset == -1)
        return E_FAIL;

    asmtbl = (ASSEMBLYTABLE *)assembly_data_offset(assembly, offset);
    if (!asmtbl)
        return E_FAIL;

    pubkey = assembly_get_blob(assembly, asmtbl->PublicKey, &size);

    if (!CryptAcquireContextA(&crypt, NULL, NULL, PROV_RSA_FULL,
                              CRYPT_VERIFYCONTEXT))
        return E_FAIL;

    if (!CryptCreateHash(crypt, CALG_SHA1, 0, 0, &hash))
        return E_FAIL;

    if (!CryptHashData(hash, pubkey, size, 0))
        return E_FAIL;

    size = 0;
    if (!CryptGetHashParam(hash, HP_HASHVAL, NULL, &size, 0))
        return E_FAIL;

    hashdata = HeapAlloc(GetProcessHeap(), 0, size);
    if (!hashdata)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if (!CryptGetHashParam(hash, HP_HASHVAL, hashdata, &size, 0))
        goto done;

    for (i = size - 1; i >= size - 8; i--)
        tokbytes[size - i - 1] = hashdata[i];

    tok = HeapAlloc(GetProcessHeap(), 0, TOKEN_LENGTH);
    if (!tok)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    bytes_to_str(tokbytes, BYTES_PER_TOKEN, tok);
    tok[TOKEN_LENGTH - 1] = '\0';

    *token = tok;
    hr = S_OK;

done:
    HeapFree(GetProcessHeap(), 0, hashdata);
    CryptDestroyHash(hash);
    CryptReleaseContext(crypt, 0);

    return hr;
}
