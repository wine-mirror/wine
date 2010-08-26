/*
 * Implementation of DirectX File Interfaces
 *
 * Copyright 2004, 2008, 2010 Christian Costa
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

#include "config.h"
#include "wine/debug.h"

#define COBJMACROS

#include "winbase.h"
#include "wingdi.h"

#include "d3dxof_private.h"
#include "dxfile.h"

#include <stdio.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3dxof);

#define MAKEFOUR(a,b,c,d) ((DWORD)a + ((DWORD)b << 8) + ((DWORD)c << 16) + ((DWORD)d << 24))
#define XOFFILE_FORMAT_MAGIC         MAKEFOUR('x','o','f',' ')
#define XOFFILE_FORMAT_VERSION_302   MAKEFOUR('0','3','0','2')
#define XOFFILE_FORMAT_VERSION_303   MAKEFOUR('0','3','0','3')
#define XOFFILE_FORMAT_BINARY        MAKEFOUR('b','i','n',' ')
#define XOFFILE_FORMAT_TEXT          MAKEFOUR('t','x','t',' ')
#define XOFFILE_FORMAT_BINARY_MSZIP  MAKEFOUR('b','z','i','p')
#define XOFFILE_FORMAT_TEXT_MSZIP    MAKEFOUR('t','z','i','p')
#define XOFFILE_FORMAT_COMPRESSED    MAKEFOUR('c','m','p',' ')
#define XOFFILE_FORMAT_FLOAT_BITS_32 MAKEFOUR('0','0','3','2')
#define XOFFILE_FORMAT_FLOAT_BITS_64 MAKEFOUR('0','0','6','4')

static const struct IDirectXFileVtbl IDirectXFile_Vtbl;
static const struct IDirectXFileBinaryVtbl IDirectXFileBinary_Vtbl;
static const struct IDirectXFileDataVtbl IDirectXFileData_Vtbl;
static const struct IDirectXFileDataReferenceVtbl IDirectXFileDataReference_Vtbl;
static const struct IDirectXFileEnumObjectVtbl IDirectXFileEnumObject_Vtbl;
static const struct IDirectXFileObjectVtbl IDirectXFileObject_Vtbl;
static const struct IDirectXFileSaveObjectVtbl IDirectXFileSaveObject_Vtbl;

static HRESULT IDirectXFileDataReferenceImpl_Create(IDirectXFileDataReferenceImpl** ppObj);
static HRESULT IDirectXFileEnumObjectImpl_Create(IDirectXFileEnumObjectImpl** ppObj);
static HRESULT IDirectXFileSaveObjectImpl_Create(IDirectXFileSaveObjectImpl** ppObj);

/* FOURCC to string conversion for debug messages */
static const char *debugstr_fourcc(DWORD fourcc)
{
    if (!fourcc) return "'null'";
    return wine_dbg_sprintf ("\'%c%c%c%c\'",
		(char)(fourcc), (char)(fourcc >> 8),
        (char)(fourcc >> 16), (char)(fourcc >> 24));
}

HRESULT IDirectXFileImpl_Create(IUnknown* pUnkOuter, LPVOID* ppObj)
{
    IDirectXFileImpl* object;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return DXFILEERR_BADALLOC;
    }

    object->lpVtbl = &IDirectXFile_Vtbl;
    object->ref = 1;

    *ppObj = object;
    
    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectXFileImpl_QueryInterface(IDirectXFile* iface, REFIID riid, void** ppvObject)
{
  IDirectXFileImpl *This = (IDirectXFileImpl *)iface;

  TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_IDirectXFile))
  {
    IUnknown_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

  ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectXFileImpl_AddRef(IDirectXFile* iface)
{
  IDirectXFileImpl *This = (IDirectXFileImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p/%p): AddRef from %d\n", iface, This, ref - 1);

  return ref;
}

static ULONG WINAPI IDirectXFileImpl_Release(IDirectXFile* iface)
{
  IDirectXFileImpl *This = (IDirectXFileImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p/%p): ReleaseRef to %d\n", iface, This, ref);

  if (!ref)
    HeapFree(GetProcessHeap(), 0, This);

  return ref;
}

/*** IDirectXFile methods ***/
static HRESULT WINAPI IDirectXFileImpl_CreateEnumObject(IDirectXFile* iface, LPVOID pvSource, DXFILELOADOPTIONS dwLoadOptions, LPDIRECTXFILEENUMOBJECT* ppEnumObj)
{
  IDirectXFileImpl *This = (IDirectXFileImpl *)iface;
  IDirectXFileEnumObjectImpl* object;
  HRESULT hr;
  DWORD* header;
  HANDLE hFile = INVALID_HANDLE_VALUE;
  HANDLE file_mapping = 0;
  LPBYTE buffer = NULL;
  HGLOBAL resource_data = 0;
  LPBYTE decomp_buffer = NULL;
  DWORD decomp_size = 0;
  LPBYTE file_buffer;
  DWORD file_size;

  LPDXFILELOADMEMORY lpdxflm = NULL;

  TRACE("(%p/%p)->(%p,%x,%p)\n", This, iface, pvSource, dwLoadOptions, ppEnumObj);

  if (!ppEnumObj)
    return DXFILEERR_BADVALUE;

  /* Only lowest 4 bits are relevant in DXFILELOADOPTIONS */
  dwLoadOptions &= 0xF;

  if (dwLoadOptions == DXFILELOAD_FROMFILE)
  {
    TRACE("Open source file '%s'\n", (char*)pvSource);

    hFile = CreateFileA(pvSource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
      TRACE("File '%s' not found\n", (char*)pvSource);
      return DXFILEERR_FILENOTFOUND;
    }

    file_size = GetFileSize(hFile, NULL);

    file_mapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!file_mapping)
    {
      hr = DXFILEERR_BADFILETYPE;
      goto error;
    }

    buffer = MapViewOfFile(file_mapping, FILE_MAP_READ, 0, 0, 0);
    if (!buffer)
    {
      hr = DXFILEERR_BADFILETYPE;
      goto error;
    }
    file_buffer = buffer;
  }
  else if (dwLoadOptions == DXFILELOAD_FROMRESOURCE)
  {
    HRSRC resource_info;
    LPDXFILELOADRESOURCE lpdxflr = pvSource;

    TRACE("Source in resource (module = %p, name = %s, type = %s\n", lpdxflr->hModule, debugstr_a(lpdxflr->lpName), debugstr_a(lpdxflr->lpType));

    resource_info = FindResourceA(lpdxflr->hModule, lpdxflr->lpName, lpdxflr->lpType);
    if (!resource_info)
    {
      hr = DXFILEERR_RESOURCENOTFOUND;
      goto error;
    }

    file_size = SizeofResource(lpdxflr->hModule, resource_info);

    resource_data = LoadResource(lpdxflr->hModule, resource_info);
    if (!resource_data)
    {
      hr = DXFILEERR_BADRESOURCE;
      goto error;
    }

    file_buffer = LockResource(resource_data);
    if (!file_buffer)
    {
      hr = DXFILEERR_BADRESOURCE;
      goto error;
    }
  }
  else if (dwLoadOptions == DXFILELOAD_FROMMEMORY)
  {
    lpdxflm = pvSource;

    TRACE("Source in memory at %p with size %d\n", lpdxflm->lpMemory, lpdxflm->dSize);

    file_buffer = lpdxflm->lpMemory;
    file_size = lpdxflm->dSize;
  }
  else
  {
    FIXME("Source type %d is not handled yet\n", dwLoadOptions);
    hr = DXFILEERR_NOTDONEYET;
    goto error;
  }

  header = (DWORD*)file_buffer;

  if (TRACE_ON(d3dxof))
  {
    char string[17];
    memcpy(string, header, 16);
    string[16] = 0;
    TRACE("header = '%s'\n", string);
  }

  if (file_size < 16)
  {
    hr = DXFILEERR_BADFILETYPE;
    goto error;
  }

  if (header[0] != XOFFILE_FORMAT_MAGIC)
  {
    hr = DXFILEERR_BADFILETYPE;
    goto error;
  }

  if ((header[1] != XOFFILE_FORMAT_VERSION_302) && (header[1] != XOFFILE_FORMAT_VERSION_303))
  {
    hr = DXFILEERR_BADFILEVERSION;
    goto error;
  }

  if ((header[2] != XOFFILE_FORMAT_BINARY) && (header[2] != XOFFILE_FORMAT_TEXT) &&
      (header[2] != XOFFILE_FORMAT_BINARY_MSZIP) && (header[2] != XOFFILE_FORMAT_TEXT_MSZIP))
  {
    WARN("File type %s unknown\n", debugstr_fourcc(header[2]));
    hr = DXFILEERR_BADFILETYPE;
    goto error;
  }

  if ((header[2] == XOFFILE_FORMAT_BINARY_MSZIP) || (header[2] == XOFFILE_FORMAT_TEXT_MSZIP))
  {
    int err;
    DWORD comp_size;

    /*  0-15 -> xfile header, 16-17 -> decompressed size w/ header, 18-19 -> null,
       20-21 -> decompressed size w/o header, 22-23 -> size of MSZIP compressed data,
       24-xx -> compressed MSZIP data */
    decomp_size = ((WORD*)file_buffer)[10];
    comp_size = ((WORD*)file_buffer)[11];

    TRACE("Compressed format %s detected: compressed_size = %x, decompressed_size = %x\n",
        debugstr_fourcc(header[2]), comp_size, decomp_size);

    decomp_buffer = HeapAlloc(GetProcessHeap(), 0, decomp_size);
    if (!decomp_buffer)
    {
        ERR("Out of memory\n");
        hr = DXFILEERR_BADALLOC;
        goto error;
    }
    err = mszip_decompress(comp_size, decomp_size, (char*)file_buffer + 24, (char*)decomp_buffer);
    if (err)
    {
        WARN("Error while decomrpessing mszip archive %d\n", err);
        hr = DXFILEERR_BADALLOC;
        goto error;
    }
  }

  if ((header[3] != XOFFILE_FORMAT_FLOAT_BITS_32) && (header[3] != XOFFILE_FORMAT_FLOAT_BITS_64))
  {
    hr = DXFILEERR_BADFILEFLOATSIZE;
    goto error;
  }

  TRACE("Header is correct\n");

  hr = IDirectXFileEnumObjectImpl_Create(&object);
  if (FAILED(hr))
    goto error;

  object->source = dwLoadOptions;
  object->hFile = hFile;
  object->file_mapping = file_mapping;
  object->buffer = buffer;
  object->decomp_buffer = decomp_buffer;
  object->pDirectXFile = This;
  object->buf.pdxf = This;
  object->buf.txt = (header[2] == XOFFILE_FORMAT_TEXT) || (header[2] == XOFFILE_FORMAT_TEXT_MSZIP);
  object->buf.token_present = FALSE;

  TRACE("File size is %d bytes\n", file_size);

  if (decomp_size)
  {
    /* Use decompressed data */
    object->buf.buffer = decomp_buffer;
    object->buf.rem_bytes = decomp_size;
  }
  else
  {
    /* Go to data after header */
    object->buf.buffer = file_buffer + 16;
    object->buf.rem_bytes = file_size - 16;
  }

  *ppEnumObj = (LPDIRECTXFILEENUMOBJECT)object;

  while (object->buf.rem_bytes && is_template_available(&object->buf))
  {
    if (!parse_template(&object->buf))
    {
      WARN("Template is not correct\n");
      hr = DXFILEERR_BADVALUE;
      goto error;
    }
    else
    {
      TRACE("Template successfully parsed:\n");
      if (TRACE_ON(d3dxof))
        dump_template(This->xtemplates, &This->xtemplates[This->nb_xtemplates - 1]);
    }
  }

  if (TRACE_ON(d3dxof))
  {
    int i;
    TRACE("Registered templates (%d):\n", This->nb_xtemplates);
    for (i = 0; i < This->nb_xtemplates; i++)
      DPRINTF("%s - %s\n", This->xtemplates[i].name, debugstr_guid(&This->xtemplates[i].class_id));
  }

  return DXFILE_OK;

error:
  if (buffer)
    UnmapViewOfFile(buffer);
  if (file_mapping)
    CloseHandle(file_mapping);
  if (hFile != INVALID_HANDLE_VALUE)
    CloseHandle(hFile);
  if (resource_data)
    FreeResource(resource_data);
  HeapFree(GetProcessHeap(), 0, decomp_buffer);
  *ppEnumObj = NULL;

  return hr;
}

static HRESULT WINAPI IDirectXFileImpl_CreateSaveObject(IDirectXFile* iface, LPCSTR szFileName, DXFILEFORMAT dwFileFormat, LPDIRECTXFILESAVEOBJECT* ppSaveObj)
{
  IDirectXFileImpl *This = (IDirectXFileImpl *)iface;

  FIXME("(%p/%p)->(%s,%x,%p) partial stub!\n", This, iface, szFileName, dwFileFormat, ppSaveObj);

  if (!szFileName || !ppSaveObj)
    return E_POINTER;

  return IDirectXFileSaveObjectImpl_Create((IDirectXFileSaveObjectImpl**)ppSaveObj);
}

static HRESULT WINAPI IDirectXFileImpl_RegisterTemplates(IDirectXFile* iface, LPVOID pvData, DWORD cbSize)
{
  IDirectXFileImpl *This = (IDirectXFileImpl *)iface;
  DWORD token_header;
  parse_buffer buf;
  LPBYTE decomp_buffer = NULL;
  DWORD decomp_size = 0;

  buf.buffer = pvData;
  buf.rem_bytes = cbSize;
  buf.txt = FALSE;
  buf.token_present = FALSE;
  buf.pdxf = This;

  TRACE("(%p/%p)->(%p,%d)\n", This, iface, pvData, cbSize);

  if (!pvData)
    return DXFILEERR_BADVALUE;

  if (cbSize < 16)
    return DXFILEERR_BADFILETYPE;

  if (TRACE_ON(d3dxof))
  {
    char string[17];
    memcpy(string, pvData, 16);
    string[16] = 0;
    TRACE("header = '%s'\n", string);
  }

  read_bytes(&buf, &token_header, 4);

  if (token_header != XOFFILE_FORMAT_MAGIC)
    return DXFILEERR_BADFILETYPE;

  read_bytes(&buf, &token_header, 4);

  if ((token_header != XOFFILE_FORMAT_VERSION_302) && (token_header != XOFFILE_FORMAT_VERSION_303))
    return DXFILEERR_BADFILEVERSION;

  read_bytes(&buf, &token_header, 4);

  if ((token_header != XOFFILE_FORMAT_BINARY) && (token_header != XOFFILE_FORMAT_TEXT) &&
      (token_header != XOFFILE_FORMAT_BINARY_MSZIP) && (token_header != XOFFILE_FORMAT_TEXT_MSZIP))
  {
    WARN("File type %s unknown\n", debugstr_fourcc(token_header));
    return DXFILEERR_BADFILETYPE;
  }

  if ((token_header == XOFFILE_FORMAT_BINARY_MSZIP) || (token_header == XOFFILE_FORMAT_TEXT_MSZIP))
  {
    int err;
    DWORD comp_size;

    /*  0-15 -> xfile header, 16-17 -> decompressed size w/ header, 18-19 -> null,
       20-21 -> decompressed size w/o header, 22-23 -> size of MSZIP compressed data,
       24-xx -> compressed MSZIP data */
    decomp_size = ((WORD*)pvData)[10];
    comp_size = ((WORD*)pvData)[11];

    TRACE("Compressed format %s detected: compressed_size = %x, decompressed_size = %x\n",
        debugstr_fourcc(token_header), comp_size, decomp_size);

    decomp_buffer = HeapAlloc(GetProcessHeap(), 0, decomp_size);
    if (!decomp_buffer)
    {
        ERR("Out of memory\n");
        return DXFILEERR_BADALLOC;
    }
    err = mszip_decompress(comp_size, decomp_size, (char*)pvData + 24, (char*)decomp_buffer);
    if (err)
    {
        WARN("Error while decomrpessing mszip archive %d\n", err);
        HeapFree(GetProcessHeap(), 0, decomp_buffer);
        return DXFILEERR_BADALLOC;
    }
  }

  if ((token_header == XOFFILE_FORMAT_TEXT) || (token_header == XOFFILE_FORMAT_TEXT_MSZIP))
    buf.txt = TRUE;

  read_bytes(&buf, &token_header, 4);

  if ((token_header != XOFFILE_FORMAT_FLOAT_BITS_32) && (token_header != XOFFILE_FORMAT_FLOAT_BITS_64))
    return DXFILEERR_BADFILEFLOATSIZE;

  TRACE("Header is correct\n");

  if (decomp_size)
  {
    buf.buffer = decomp_buffer;
    buf.rem_bytes = decomp_size;
  }

  while (buf.rem_bytes && is_template_available(&buf))
  {
    if (!parse_template(&buf))
    {
      WARN("Template is not correct\n");
      return DXFILEERR_BADVALUE;
    }
    else
    {
      TRACE("Template successfully parsed:\n");
      if (TRACE_ON(d3dxof))
        dump_template(This->xtemplates, &This->xtemplates[This->nb_xtemplates - 1]);
    }
  }

  if (TRACE_ON(d3dxof))
  {
    int i;
    TRACE("Registered templates (%d):\n", This->nb_xtemplates);
    for (i = 0; i < This->nb_xtemplates; i++)
      DPRINTF("%s - %s\n", This->xtemplates[i].name, debugstr_guid(&This->xtemplates[i].class_id));
  }

  HeapFree(GetProcessHeap(), 0, decomp_buffer);

  return DXFILE_OK;
}

static const IDirectXFileVtbl IDirectXFile_Vtbl =
{
  IDirectXFileImpl_QueryInterface,
  IDirectXFileImpl_AddRef,
  IDirectXFileImpl_Release,
  IDirectXFileImpl_CreateEnumObject,
  IDirectXFileImpl_CreateSaveObject,
  IDirectXFileImpl_RegisterTemplates
};

static HRESULT IDirectXFileBinaryImpl_Create(IDirectXFileBinaryImpl** ppObj)
{
    IDirectXFileBinaryImpl* object;

    TRACE("(%p)\n", ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileBinaryImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return DXFILEERR_BADALLOC;
    }

    object->lpVtbl = &IDirectXFileBinary_Vtbl;
    object->ref = 1;

    *ppObj = object;

    return DXFILE_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectXFileBinaryImpl_QueryInterface(IDirectXFileBinary* iface, REFIID riid, void** ppvObject)
{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;

  TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_IDirectXFileObject)
      || IsEqualGUID(riid, &IID_IDirectXFileBinary))
  {
    IUnknown_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

  /* Do not print an error for interfaces that can be queried to retrieve the type of the object */
  if (!IsEqualGUID(riid, &IID_IDirectXFileData)
      && !IsEqualGUID(riid, &IID_IDirectXFileDataReference))
    ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);

  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectXFileBinaryImpl_AddRef(IDirectXFileBinary* iface)
{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p/%p): AddRef from %d\n", iface, This, ref - 1);

  return ref;
}

static ULONG WINAPI IDirectXFileBinaryImpl_Release(IDirectXFileBinary* iface)
{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p/%p): ReleaseRef to %d\n", iface, This, ref);

  if (!ref)
    HeapFree(GetProcessHeap(), 0, This);

  return ref;
}

/*** IDirectXFileObject methods ***/
static HRESULT WINAPI IDirectXFileBinaryImpl_GetName(IDirectXFileBinary* iface, LPSTR pstrNameBuf, LPDWORD pdwBufLen)

{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;

  FIXME("(%p/%p)->(%p,%p) stub!\n", This, iface, pstrNameBuf, pdwBufLen); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileBinaryImpl_GetId(IDirectXFileBinary* iface, LPGUID pGuid)
{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pGuid); 

  return DXFILEERR_BADVALUE;
}

/*** IDirectXFileBinary methods ***/
static HRESULT WINAPI IDirectXFileBinaryImpl_GetSize(IDirectXFileBinary* iface, DWORD* pcbSize)
{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pcbSize); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileBinaryImpl_GetMimeType(IDirectXFileBinary* iface, LPCSTR* pszMimeType)
{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pszMimeType);

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileBinaryImpl_Read(IDirectXFileBinary* iface, LPVOID pvData, DWORD cbSize, LPDWORD pcbRead)
{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;

  FIXME("(%p/%p)->(%p, %d, %p) stub!\n", This, iface, pvData, cbSize, pcbRead);

  return DXFILEERR_BADVALUE;
}

static const IDirectXFileBinaryVtbl IDirectXFileBinary_Vtbl =
{
    IDirectXFileBinaryImpl_QueryInterface,
    IDirectXFileBinaryImpl_AddRef,
    IDirectXFileBinaryImpl_Release,
    IDirectXFileBinaryImpl_GetName,
    IDirectXFileBinaryImpl_GetId,
    IDirectXFileBinaryImpl_GetSize,
    IDirectXFileBinaryImpl_GetMimeType,
    IDirectXFileBinaryImpl_Read
};

static HRESULT IDirectXFileDataImpl_Create(IDirectXFileDataImpl** ppObj)
{
    IDirectXFileDataImpl* object;

    TRACE("(%p)\n", ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileDataImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return DXFILEERR_BADALLOC;
    }

    object->lpVtbl = &IDirectXFileData_Vtbl;
    object->ref = 1;

    *ppObj = object;
    
    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectXFileDataImpl_QueryInterface(IDirectXFileData* iface, REFIID riid, void** ppvObject)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_IDirectXFileObject)
      || IsEqualGUID(riid, &IID_IDirectXFileData))
  {
    IUnknown_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

  /* Do not print an error for interfaces that can be queried to retrieve the type of the object */
  if (!IsEqualGUID(riid, &IID_IDirectXFileBinary)
      && !IsEqualGUID(riid, &IID_IDirectXFileDataReference))
    ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);

  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectXFileDataImpl_AddRef(IDirectXFileData* iface)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p/%p): AddRef from %d\n", iface, This, ref - 1);

  return ref;
}

static ULONG WINAPI IDirectXFileDataImpl_Release(IDirectXFileData* iface)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p/%p): ReleaseRef to %d\n", iface, This, ref);

  if (!ref)
  {
    if (!This->level && !This->from_ref)
    {
      HeapFree(GetProcessHeap(), 0, This->pstrings);
      HeapFree(GetProcessHeap(), 0, This->pobj->pdata);
      HeapFree(GetProcessHeap(), 0, This->pobj);
    }
    HeapFree(GetProcessHeap(), 0, This);
  }

  return ref;
}

/*** IDirectXFileObject methods ***/
static HRESULT WINAPI IDirectXFileDataImpl_GetName(IDirectXFileData* iface, LPSTR pstrNameBuf, LPDWORD pdwBufLen)

{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  TRACE("(%p/%p)->(%p,%p)\n", This, iface, pstrNameBuf, pdwBufLen);

  if (!pstrNameBuf)
    return DXFILEERR_BADVALUE;

  strcpy(pstrNameBuf, This->pobj->name);

  return DXFILE_OK;
}

static HRESULT WINAPI IDirectXFileDataImpl_GetId(IDirectXFileData* iface, LPGUID pGuid)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  TRACE("(%p/%p)->(%p)\n", This, iface, pGuid);

  if (!pGuid)
    return DXFILEERR_BADVALUE;

  memcpy(pGuid, &This->pobj->class_id, 16);

  return DXFILE_OK;
}

/*** IDirectXFileData methods ***/
static HRESULT WINAPI IDirectXFileDataImpl_GetData(IDirectXFileData* iface, LPCSTR szMember, DWORD* pcbSize, void** ppvData)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  TRACE("(%p/%p)->(%s,%p,%p)\n", This, iface, szMember, pcbSize, ppvData);

  if (!pcbSize || !ppvData)
    return DXFILEERR_BADVALUE;

  if (szMember)
  {
    FIXME("Specifying a member is not supported yet!\n");
    return DXFILEERR_BADVALUE;
  }

  *pcbSize = This->pobj->size;
  *ppvData = This->pobj->root->pdata + This->pobj->pos_data;

  return DXFILE_OK;
}

static HRESULT WINAPI IDirectXFileDataImpl_GetType(IDirectXFileData* iface, const GUID** pguid)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;
  static GUID guid;

  TRACE("(%p/%p)->(%p)\n", This, iface, pguid);

  if (!pguid)
    return DXFILEERR_BADVALUE;

  memcpy(&guid, &This->pobj->type, 16);
  *pguid = &guid;

  return DXFILE_OK;
}

static HRESULT WINAPI IDirectXFileDataImpl_GetNextObject(IDirectXFileData* iface, LPDIRECTXFILEOBJECT* ppChildObj)
{
  HRESULT hr;
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  TRACE("(%p/%p)->(%p)\n", This, iface, ppChildObj);

  if (This->cur_enum_object >= This->pobj->nb_childs)
    return DXFILEERR_NOMOREOBJECTS;

  if (This->from_ref && (This->level >= 1))
  {
    /* Only 2 levels can be enumerated if the object is obtained from a reference */
    return DXFILEERR_NOMOREOBJECTS;
  }

  if (This->pobj->childs[This->cur_enum_object]->binary)
  {
    IDirectXFileBinaryImpl *object;

    hr = IDirectXFileBinaryImpl_Create(&object);
    if (FAILED(hr))
      return hr;

    *ppChildObj = (LPDIRECTXFILEOBJECT)object;
  }
  else if (This->pobj->childs[This->cur_enum_object]->ptarget)
  {
    IDirectXFileDataReferenceImpl *object;

    hr = IDirectXFileDataReferenceImpl_Create(&object);
    if (FAILED(hr))
      return hr;

    object->ptarget = This->pobj->childs[This->cur_enum_object++]->ptarget;

    *ppChildObj = (LPDIRECTXFILEOBJECT)object;
  }
  else
  {
    IDirectXFileDataImpl *object;

    hr = IDirectXFileDataImpl_Create(&object);
    if (FAILED(hr))
      return hr;

    object->pobj = This->pobj->childs[This->cur_enum_object++];
    object->cur_enum_object = 0;
    object->from_ref = This->from_ref;
    object->level = This->level + 1;

    *ppChildObj = (LPDIRECTXFILEOBJECT)object;
  }

  return DXFILE_OK;
}

static HRESULT WINAPI IDirectXFileDataImpl_AddDataObject(IDirectXFileData* iface, LPDIRECTXFILEDATA pDataObj)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pDataObj); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataImpl_AddDataReference(IDirectXFileData* iface, LPCSTR szRef, const GUID* pguidRef)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%s,%p) stub!\n", This, iface, szRef, pguidRef); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataImpl_AddBinaryObject(IDirectXFileData* iface, LPCSTR szName, const GUID* pguid, LPCSTR szMimeType, LPVOID pvData, DWORD cbSize)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%s,%p,%s,%p,%d) stub!\n", This, iface, szName, pguid, szMimeType, pvData, cbSize);

  return DXFILEERR_BADVALUE;
}

static const IDirectXFileDataVtbl IDirectXFileData_Vtbl =
{
    IDirectXFileDataImpl_QueryInterface,
    IDirectXFileDataImpl_AddRef,
    IDirectXFileDataImpl_Release,
    IDirectXFileDataImpl_GetName,
    IDirectXFileDataImpl_GetId,
    IDirectXFileDataImpl_GetData,
    IDirectXFileDataImpl_GetType,
    IDirectXFileDataImpl_GetNextObject,
    IDirectXFileDataImpl_AddDataObject,
    IDirectXFileDataImpl_AddDataReference,
    IDirectXFileDataImpl_AddBinaryObject
};

static HRESULT IDirectXFileDataReferenceImpl_Create(IDirectXFileDataReferenceImpl** ppObj)
{
    IDirectXFileDataReferenceImpl* object;

    TRACE("(%p)\n", ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileDataReferenceImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return DXFILEERR_BADALLOC;
    }
    
    object->lpVtbl = &IDirectXFileDataReference_Vtbl;
    object->ref = 1;

    *ppObj = object;
    
    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectXFileDataReferenceImpl_QueryInterface(IDirectXFileDataReference* iface, REFIID riid, void** ppvObject)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;

  TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_IDirectXFileObject)
      || IsEqualGUID(riid, &IID_IDirectXFileDataReference))
  {
    IUnknown_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

  /* Do not print an error for interfaces that can be queried to retrieve the type of the object */
  if (!IsEqualGUID(riid, &IID_IDirectXFileData)
      && !IsEqualGUID(riid, &IID_IDirectXFileBinary))
    ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);

  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectXFileDataReferenceImpl_AddRef(IDirectXFileDataReference* iface)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p/%p): AddRef from %d\n", iface, This, ref - 1);

  return ref;
}

static ULONG WINAPI IDirectXFileDataReferenceImpl_Release(IDirectXFileDataReference* iface)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p/%p): ReleaseRef to %d\n", iface, This, ref);

  if (!ref)
    HeapFree(GetProcessHeap(), 0, This);

  return ref;
}

/*** IDirectXFileObject methods ***/
static HRESULT WINAPI IDirectXFileDataReferenceImpl_GetName(IDirectXFileDataReference* iface, LPSTR pstrNameBuf, LPDWORD pdwBufLen)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;

  TRACE("(%p/%p)->(%p,%p)\n", This, iface, pstrNameBuf, pdwBufLen);

  if (!pstrNameBuf)
    return DXFILEERR_BADVALUE;

  strcpy(pstrNameBuf, This->ptarget->name);

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataReferenceImpl_GetId(IDirectXFileDataReference* iface, LPGUID pGuid)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;

  TRACE("(%p/%p)->(%p)\n", This, iface, pGuid);

  if (!pGuid)
    return DXFILEERR_BADVALUE;

  memcpy(pGuid, &This->ptarget->class_id, 16);

  return DXFILE_OK;
}

/*** IDirectXFileDataReference ***/
static HRESULT WINAPI IDirectXFileDataReferenceImpl_Resolve(IDirectXFileDataReference* iface, LPDIRECTXFILEDATA* ppDataObj)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;
  IDirectXFileDataImpl *object;
  HRESULT hr;

  TRACE("(%p/%p)->(%p)\n", This, iface, ppDataObj);

  if (!ppDataObj)
    return DXFILEERR_BADVALUE;

  hr = IDirectXFileDataImpl_Create(&object);
  if (FAILED(hr))
    return hr;

  object->pobj = This->ptarget;
  object->cur_enum_object = 0;
  object->level = 0;
  object->from_ref = TRUE;

  *ppDataObj = (LPDIRECTXFILEDATA)object;

  return DXFILE_OK;
}

static const IDirectXFileDataReferenceVtbl IDirectXFileDataReference_Vtbl =
{
    IDirectXFileDataReferenceImpl_QueryInterface,
    IDirectXFileDataReferenceImpl_AddRef,
    IDirectXFileDataReferenceImpl_Release,
    IDirectXFileDataReferenceImpl_GetName,
    IDirectXFileDataReferenceImpl_GetId,
    IDirectXFileDataReferenceImpl_Resolve
};

static HRESULT IDirectXFileEnumObjectImpl_Create(IDirectXFileEnumObjectImpl** ppObj)
{
    IDirectXFileEnumObjectImpl* object;

    TRACE("(%p)\n", ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileEnumObjectImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return DXFILEERR_BADALLOC;
    }
    
    object->lpVtbl = &IDirectXFileEnumObject_Vtbl;
    object->ref = 1;

    *ppObj = object;
    
    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectXFileEnumObjectImpl_QueryInterface(IDirectXFileEnumObject* iface, REFIID riid, void** ppvObject)
{
  IDirectXFileEnumObjectImpl *This = (IDirectXFileEnumObjectImpl *)iface;

  TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_IDirectXFileEnumObject))
  {
    IUnknown_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

  ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectXFileEnumObjectImpl_AddRef(IDirectXFileEnumObject* iface)
{
  IDirectXFileEnumObjectImpl *This = (IDirectXFileEnumObjectImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p/%p): AddRef from %d\n", iface, This, ref - 1);

  return ref;
}

static ULONG WINAPI IDirectXFileEnumObjectImpl_Release(IDirectXFileEnumObject* iface)
{
  IDirectXFileEnumObjectImpl *This = (IDirectXFileEnumObjectImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p/%p): ReleaseRef to %d\n", iface, This, ref);

  if (!ref)
  {
    int i;
    for (i = 0; i < This->nb_xobjects; i++)
      IDirectXFileData_Release(This->pRefObjects[i]);
    if (This->source == DXFILELOAD_FROMFILE)
    {
      UnmapViewOfFile(This->buffer);
      CloseHandle(This->file_mapping);
      CloseHandle(This->hFile);
    }
    else if (This->source == DXFILELOAD_FROMRESOURCE)
      FreeResource(This->resource_data);
    HeapFree(GetProcessHeap(), 0, This->decomp_buffer);
    HeapFree(GetProcessHeap(), 0, This);
  }

  return ref;
}

/*** IDirectXFileEnumObject methods ***/
static HRESULT WINAPI IDirectXFileEnumObjectImpl_GetNextDataObject(IDirectXFileEnumObject* iface, LPDIRECTXFILEDATA* ppDataObj)
{
  IDirectXFileEnumObjectImpl *This = (IDirectXFileEnumObjectImpl *)iface;
  IDirectXFileDataImpl* object;
  HRESULT hr;
  LPBYTE pstrings = NULL;

  TRACE("(%p/%p)->(%p)\n", This, iface, ppDataObj);

  if (This->nb_xobjects >= MAX_OBJECTS)
  {
    ERR("Too many objects\n");
    return DXFILEERR_NOMOREOBJECTS;
  }

  /* Check if there are templates defined before the object */
  while (This->buf.rem_bytes && is_template_available(&This->buf))
  {
    if (!parse_template(&This->buf))
    {
      WARN("Template is not correct\n");
      hr = DXFILEERR_BADVALUE;
      goto error;
    }
    else
    {
      TRACE("Template successfully parsed:\n");
      if (TRACE_ON(d3dxof))
        dump_template(This->pDirectXFile->xtemplates, &This->pDirectXFile->xtemplates[This->pDirectXFile->nb_xtemplates - 1]);
    }
  }

  if (!This->buf.rem_bytes)
    return DXFILEERR_NOMOREOBJECTS;

  hr = IDirectXFileDataImpl_Create(&object);
  if (FAILED(hr))
    return hr;

  This->buf.pxo_globals = This->xobjects;
  This->buf.nb_pxo_globals = This->nb_xobjects;
  This->buf.level = 0;

  This->buf.pxo_tab = HeapAlloc(GetProcessHeap(), 0, sizeof(xobject)*MAX_SUBOBJECTS);
  if (!This->buf.pxo_tab)
  {
    ERR("Out of memory\n");
    hr = DXFILEERR_BADALLOC;
    goto error;
  }
  This->buf.pxo = This->xobjects[This->nb_xobjects] = This->buf.pxo_tab;

  This->buf.pxo->pdata = This->buf.pdata = NULL;
  This->buf.capacity = 0;
  This->buf.cur_pos_data = 0;
  This->buf.pxo->nb_subobjects = 1;

  pstrings = HeapAlloc(GetProcessHeap(), 0, MAX_STRINGS_BUFFER);
  if (!pstrings)
  {
    ERR("Out of memory\n");
    hr = DXFILEERR_BADALLOC;
    goto error;
  }
  This->buf.cur_pstrings = This->buf.pstrings = object->pstrings = pstrings;

  if (!parse_object(&This->buf))
  {
    WARN("Object is not correct\n");
    hr = DXFILEERR_PARSEERROR;
    goto error;
  }

  object->pstrings = pstrings;
  object->pobj = This->buf.pxo;
  object->cur_enum_object = 0;
  object->level = 0;
  object->from_ref = FALSE;

  *ppDataObj = (LPDIRECTXFILEDATA)object;

  /* Get a reference to created object */
  This->pRefObjects[This->nb_xobjects] = (LPDIRECTXFILEDATA)object;
  IDirectXFileData_AddRef(This->pRefObjects[This->nb_xobjects]);

  This->nb_xobjects++;

  return DXFILE_OK;

error:

  HeapFree(GetProcessHeap(), 0, This->buf.pxo_tab);
  HeapFree(GetProcessHeap(), 0, This->buf.pdata);
  HeapFree(GetProcessHeap(), 0, pstrings);

  return hr;
}

static HRESULT WINAPI IDirectXFileEnumObjectImpl_GetDataObjectById(IDirectXFileEnumObject* iface, REFGUID rguid, LPDIRECTXFILEDATA* ppDataObj)
{
  IDirectXFileEnumObjectImpl *This = (IDirectXFileEnumObjectImpl *)iface;

  FIXME("(%p/%p)->(%p,%p) stub!\n", This, iface, rguid, ppDataObj); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileEnumObjectImpl_GetDataObjectByName(IDirectXFileEnumObject* iface, LPCSTR szName, LPDIRECTXFILEDATA* ppDataObj)
{
  IDirectXFileEnumObjectImpl *This = (IDirectXFileEnumObjectImpl *)iface;

  FIXME("(%p/%p)->(%s,%p) stub!\n", This, iface, szName, ppDataObj); 

  return DXFILEERR_BADVALUE;
}

static const IDirectXFileEnumObjectVtbl IDirectXFileEnumObject_Vtbl =
{
    IDirectXFileEnumObjectImpl_QueryInterface,
    IDirectXFileEnumObjectImpl_AddRef,
    IDirectXFileEnumObjectImpl_Release,
    IDirectXFileEnumObjectImpl_GetNextDataObject,
    IDirectXFileEnumObjectImpl_GetDataObjectById,
    IDirectXFileEnumObjectImpl_GetDataObjectByName
};

static HRESULT IDirectXFileSaveObjectImpl_Create(IDirectXFileSaveObjectImpl** ppObj)
{
    IDirectXFileSaveObjectImpl* object;

    TRACE("(%p)\n", ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileSaveObjectImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return DXFILEERR_BADALLOC;
    }

    object->lpVtbl = &IDirectXFileSaveObject_Vtbl;
    object->ref = 1;

    *ppObj = object;

    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectXFileSaveObjectImpl_QueryInterface(IDirectXFileSaveObject* iface, REFIID riid, void** ppvObject)
{
  IDirectXFileSaveObjectImpl *This = (IDirectXFileSaveObjectImpl *)iface;

  TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_IDirectXFileSaveObject))
  {
    IUnknown_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

  ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectXFileSaveObjectImpl_AddRef(IDirectXFileSaveObject* iface)
{
  IDirectXFileSaveObjectImpl *This = (IDirectXFileSaveObjectImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p/%p): AddRef from %d\n", iface, This, ref - 1);

  return ref;
}

static ULONG WINAPI IDirectXFileSaveObjectImpl_Release(IDirectXFileSaveObject* iface)
{
  IDirectXFileSaveObjectImpl *This = (IDirectXFileSaveObjectImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p/%p): ReleaseRef to %d\n", iface, This, ref);

  if (!ref)
    HeapFree(GetProcessHeap(), 0, This);

  return ref;
}

static HRESULT WINAPI IDirectXFileSaveObjectImpl_SaveTemplates(IDirectXFileSaveObject* iface, DWORD cTemplates, const GUID** ppguidTemplates)
{
  IDirectXFileSaveObjectImpl *This = (IDirectXFileSaveObjectImpl *)iface;

  FIXME("(%p/%p)->(%d,%p) stub!\n", This, iface, cTemplates, ppguidTemplates);

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileSaveObjectImpl_CreateDataObject(IDirectXFileSaveObject* iface, REFGUID rguidTemplate, LPCSTR szName, const GUID* pguid, DWORD cbSize, LPVOID pvData, LPDIRECTXFILEDATA* ppDataObj)
{
  IDirectXFileSaveObjectImpl *This = (IDirectXFileSaveObjectImpl *)iface;

  FIXME("(%p/%p)->(%p,%s,%p,%d,%p,%p) stub!\n", This, iface, rguidTemplate, szName, pguid, cbSize, pvData, ppDataObj);

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileSaveObjectImpl_SaveData(IDirectXFileSaveObject* iface, LPDIRECTXFILEDATA ppDataObj)
{
  IDirectXFileSaveObjectImpl *This = (IDirectXFileSaveObjectImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, ppDataObj); 

  return DXFILEERR_BADVALUE;
}

static const IDirectXFileSaveObjectVtbl IDirectXFileSaveObject_Vtbl =
{
    IDirectXFileSaveObjectImpl_QueryInterface,
    IDirectXFileSaveObjectImpl_AddRef,
    IDirectXFileSaveObjectImpl_Release,
    IDirectXFileSaveObjectImpl_SaveTemplates,
    IDirectXFileSaveObjectImpl_CreateDataObject,
    IDirectXFileSaveObjectImpl_SaveData
};
