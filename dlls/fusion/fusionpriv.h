/*
 * fusion private definitions
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

#ifndef __WINE_FUSION_PRIVATE__
#define __WINE_FUSION_PRIVATE__

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

typedef struct
{
    ULONG Signature;
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG Reserved;
    ULONG VersionLength;
    BYTE Version[12];
    BYTE Flags;
    WORD Streams;
} METADATAHDR;

#include <pshpack1.h>

typedef struct
{
    DWORD Offset;
    DWORD Size;
} METADATASTREAMHDR;

typedef struct
{
    DWORD Reserved1;
    BYTE MajorVersion;
    BYTE MinorVersion;
    BYTE HeapOffsetSizes;
    BYTE Reserved2;
    LARGE_INTEGER MaskValid;
    LARGE_INTEGER MaskSorted;
} METADATATABLESHDR;

typedef struct
{
    WORD Generation;
    WORD Name;
    WORD Mvid;
    WORD EncId;
    WORD EncBaseId;
} MODULETABLE;

typedef struct
{
    DWORD Flags;
    WORD Name;
    WORD Namespace;
    WORD Extends;
    WORD FieldList;
    WORD MethodList;
} TYPEDEFTABLE;

typedef struct
{
    DWORD ResolutionScope;
    WORD Name;
    WORD Namespace;
} TYPEREFTABLE;

typedef struct
{
    DWORD HashAlgId;
    WORD MajorVersion;
    WORD MinorVersion;
    WORD BuildNumber;
    WORD RevisionNumber;
    DWORD Flags;
    WORD PublicKey;
    WORD Name;
    WORD Culture;
} ASSEMBLYTABLE;

typedef struct
{
    DWORD Offset;
    DWORD Flags;
    WORD Name;
    WORD Implementation;
} MANIFESTRESTABLE;

typedef struct
{
    DWORD ImportLookupTable;
    DWORD DateTimeStamp;
    DWORD ForwarderChain;
    DWORD Name;
    DWORD ImportAddressTable;
    BYTE pad[20];
} IMPORTTABLE;

typedef struct
{
    DWORD HintNameTableRVA;
    BYTE pad[8];
} IMPORTLOOKUPTABLE;

typedef struct
{
    WORD Hint;
    BYTE Name[12];
    BYTE Module[12];
    DWORD Reserved;
    WORD EntryPoint;
    DWORD RVA;
} HINTNAMETABLE;

typedef struct
{
    DWORD PageRVA;
    DWORD Size;
    DWORD Relocation;
} RELOCATION;

typedef struct
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[17];
    VS_FIXEDFILEINFO Value;
} VS_VERSIONINFO;

typedef struct
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[13];
} VARFILEINFO;

typedef struct
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[13];
    DWORD Value;
} VAR;

typedef struct
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[15];
} STRINGFILEINFO;

typedef struct
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[9];
} STRINGTABLE;

typedef struct
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
} STRINGHDR;

typedef struct
{
    DWORD Size;
    DWORD Signature;
    DWORD HeaderVersion;
    DWORD SkipData;
    BYTE Data[168];
} RESOURCE;

#include <poppack.h>

struct tagASSEMBLY;
typedef struct tagASSEMBLY ASSEMBLY;

HRESULT assembly_create(ASSEMBLY **out, LPCWSTR file);
HRESULT assembly_release(ASSEMBLY *assembly);
HRESULT assembly_get_name(ASSEMBLY *assembly, LPSTR *name);
HRESULT assembly_get_path(ASSEMBLY *assembly, LPSTR *path);
HRESULT assembly_get_version(ASSEMBLY *assembly, LPSTR *version);
HRESULT assembly_get_architecture(ASSEMBLY *assembly, DWORD fixme);
HRESULT assembly_get_pubkey_token(ASSEMBLY *assembly, LPSTR *token);

#endif /* __WINE_FUSION_PRIVATE__ */
