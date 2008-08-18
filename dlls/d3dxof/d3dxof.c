/*
 * Implementation of DirectX File Interfaces
 *
 * Copyright 2004, 2008 Christian Costa
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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
#define XOFFILE_FORMAT_VERSION       MAKEFOUR('0','3','0','2')
#define XOFFILE_FORMAT_BINARY        MAKEFOUR('b','i','n',' ')
#define XOFFILE_FORMAT_TEXT          MAKEFOUR('t','x','t',' ')
#define XOFFILE_FORMAT_COMPRESSED    MAKEFOUR('c','m','p',' ')
#define XOFFILE_FORMAT_FLOAT_BITS_32 MAKEFOUR('0','0','3','2')
#define XOFFILE_FORMAT_FLOAT_BITS_64 MAKEFOUR('0','0','6','4')

#define TOKEN_NAME         1
#define TOKEN_STRING       2
#define TOKEN_INTEGER      3
#define TOKEN_GUID         5
#define TOKEN_INTEGER_LIST 6
#define TOKEN_FLOAT_LIST   7
#define TOKEN_OBRACE      10
#define TOKEN_CBRACE      11
#define TOKEN_OPAREN      12
#define TOKEN_CPAREN      13
#define TOKEN_OBRACKET    14
#define TOKEN_CBRACKET    15
#define TOKEN_OANGLE      16
#define TOKEN_CANGLE      17
#define TOKEN_DOT         18
#define TOKEN_COMMA       19
#define TOKEN_SEMICOLON   20
#define TOKEN_TEMPLATE    31
#define TOKEN_WORD        40
#define TOKEN_DWORD       41
#define TOKEN_FLOAT       42
#define TOKEN_DOUBLE      43
#define TOKEN_CHAR        44
#define TOKEN_UCHAR       45
#define TOKEN_SWORD       46
#define TOKEN_SDWORD      47
#define TOKEN_VOID        48
#define TOKEN_LPSTR       49
#define TOKEN_UNICODE     50
#define TOKEN_CSTRING     51
#define TOKEN_ARRAY       52

typedef struct {
  /* Buffer to parse */
  LPBYTE buffer;
  DWORD rem_bytes;
  /* Dump template content */
  char* dump;
  DWORD pos;
  DWORD size;
} parse_buffer;


static const struct IDirectXFileVtbl IDirectXFile_Vtbl;
static const struct IDirectXFileBinaryVtbl IDirectXFileBinary_Vtbl;
static const struct IDirectXFileDataVtbl IDirectXFileData_Vtbl;
static const struct IDirectXFileDataReferenceVtbl IDirectXFileDataReference_Vtbl;
static const struct IDirectXFileEnumObjectVtbl IDirectXFileEnumObject_Vtbl;
static const struct IDirectXFileObjectVtbl IDirectXFileObject_Vtbl;
static const struct IDirectXFileSaveObjectVtbl IDirectXFileSaveObject_Vtbl;

HRESULT IDirectXFileImpl_Create(IUnknown* pUnkOuter, LPVOID* ppObj)
{
    IDirectXFileImpl* object;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileImpl));
    
    object->lpVtbl.lpVtbl = &IDirectXFile_Vtbl;
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
    IClassFactory_AddRef(iface);
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

  FIXME("(%p/%p)->(%p,%x,%p) stub!\n", This, iface, pvSource, dwLoadOptions, ppEnumObj);

  if (dwLoadOptions == DXFILELOAD_FROMFILE)
  {
    HANDLE hFile;
    TRACE("Open source file '%s'\n", (char*)pvSource);
    hFile = CreateFileA((char*)pvSource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
      TRACE("File '%s' not found\n", (char*)pvSource);
      return DXFILEERR_FILENOTFOUND;
    }
    CloseHandle(hFile);
  }
  else
  {
    FIXME("Source type %d is not handled yet\n", dwLoadOptions);
  }

  hr = IDirectXFileEnumObjectImpl_Create(&object);
  if (!SUCCEEDED(hr))
    return hr;

  *ppEnumObj = (LPDIRECTXFILEENUMOBJECT)object;

  return DXFILE_OK;
}

static HRESULT WINAPI IDirectXFileImpl_CreateSaveObject(IDirectXFile* iface, LPCSTR szFileName, DXFILEFORMAT dwFileFormat, LPDIRECTXFILESAVEOBJECT* ppSaveObj)
{
  IDirectXFileImpl *This = (IDirectXFileImpl *)iface;

  FIXME("(%p/%p)->(%s,%x,%p) stub!\n", This, iface, szFileName, dwFileFormat, ppSaveObj);

  return DXFILEERR_BADVALUE;
}

static BOOL read_bytes(parse_buffer * buf, LPVOID data, DWORD size)
{
  if (buf->rem_bytes < size)
    return FALSE;
  memcpy(data, buf->buffer, size);
  buf->buffer += size;
  buf->rem_bytes -= size;
  return TRUE;
}

static void add_string(parse_buffer * buf, const char * str)
{
  DWORD len = strlen(str);
  if ((buf->pos + len + 1) > buf->size)
  {
    FIXME("Dump buffer to small\n");
    return;
  }
  sprintf(buf->dump + buf->pos, str);
  buf->pos += len;
}

static void dump_TOKEN(WORD token)
{
#define DUMP_TOKEN(t) case t: TRACE(#t "\n"); break
  switch(token)
  {
    DUMP_TOKEN(TOKEN_NAME);
    DUMP_TOKEN(TOKEN_STRING);
    DUMP_TOKEN(TOKEN_INTEGER);
    DUMP_TOKEN(TOKEN_GUID);
    DUMP_TOKEN(TOKEN_INTEGER_LIST);
    DUMP_TOKEN(TOKEN_FLOAT_LIST);
    DUMP_TOKEN(TOKEN_OBRACE);
    DUMP_TOKEN(TOKEN_CBRACE);
    DUMP_TOKEN(TOKEN_OPAREN);
    DUMP_TOKEN(TOKEN_CPAREN);
    DUMP_TOKEN(TOKEN_OBRACKET);
    DUMP_TOKEN(TOKEN_CBRACKET);
    DUMP_TOKEN(TOKEN_OANGLE);
    DUMP_TOKEN(TOKEN_CANGLE);
    DUMP_TOKEN(TOKEN_DOT);
    DUMP_TOKEN(TOKEN_COMMA);
    DUMP_TOKEN(TOKEN_SEMICOLON);
    DUMP_TOKEN(TOKEN_TEMPLATE);
    DUMP_TOKEN(TOKEN_WORD);
    DUMP_TOKEN(TOKEN_DWORD);
    DUMP_TOKEN(TOKEN_FLOAT);
    DUMP_TOKEN(TOKEN_DOUBLE);
    DUMP_TOKEN(TOKEN_CHAR);
    DUMP_TOKEN(TOKEN_UCHAR);
    DUMP_TOKEN(TOKEN_SWORD);
    DUMP_TOKEN(TOKEN_SDWORD);
    DUMP_TOKEN(TOKEN_VOID);
    DUMP_TOKEN(TOKEN_LPSTR);
    DUMP_TOKEN(TOKEN_UNICODE);
    DUMP_TOKEN(TOKEN_CSTRING);
    DUMP_TOKEN(TOKEN_ARRAY);
    default:
      if (0)
        TRACE("Unknown token %d\n", token);
      break;
  }
#undef DUMP_TOKEN
}

static WORD check_TOKEN(parse_buffer * buf)
{
  WORD token;

  if (!read_bytes(buf, &token, 2))
    return 0;
  buf->buffer -= 2;
  buf->rem_bytes += 2;
  if (0)
  {
    TRACE("check: ");
    dump_TOKEN(token);
  }
  return token;
}

static WORD parse_TOKEN(parse_buffer * buf)
{
  WORD token;

  if (!read_bytes(buf, &token, 2))
    return 0;

  switch(token)
  {
    case TOKEN_NAME:
    case TOKEN_STRING:
    case TOKEN_INTEGER:
    case TOKEN_GUID:
    case TOKEN_INTEGER_LIST:
    case TOKEN_FLOAT_LIST:
      break;
    case TOKEN_OBRACE:
      add_string(buf, "{ ");
      break;
    case TOKEN_CBRACE:
      add_string(buf, "} ");
      break;
    case TOKEN_OPAREN:
      add_string(buf, "( ");
      break;
    case TOKEN_CPAREN:
      add_string(buf, ") ");
      break;
    case TOKEN_OBRACKET:
      add_string(buf, "[ ");
      break;
    case TOKEN_CBRACKET:
      add_string(buf, "] ");
      break;
    case TOKEN_OANGLE:
      add_string(buf, "< ");
      break;
    case TOKEN_CANGLE:
      add_string(buf, "> ");
      break;
    case TOKEN_DOT:
      add_string(buf, ".");
      break;
    case TOKEN_COMMA:
      add_string(buf, ", ");
      break;
    case TOKEN_SEMICOLON:
      add_string(buf, "; ");
      break;
    case TOKEN_TEMPLATE:
      add_string(buf, "template ");
      break;
    case TOKEN_WORD:
      add_string(buf, "WORD ");
      break;
    case TOKEN_DWORD:
      add_string(buf, "DWORD ");
      break;
    case TOKEN_FLOAT:
      add_string(buf, "FLOAT ");
      break;
    case TOKEN_DOUBLE:
      add_string(buf, "DOUBLE ");
      break;
    case TOKEN_CHAR:
      add_string(buf, "CHAR ");
      break;
    case TOKEN_UCHAR:
      add_string(buf, "UCHAR ");
      break;
    case TOKEN_SWORD:
      add_string(buf, "SWORD ");
      break;
    case TOKEN_SDWORD:
      add_string(buf, "SDWORD ");
      break;
    case TOKEN_VOID:
      add_string(buf, "VOID ");
      break;
    case TOKEN_LPSTR:
      add_string(buf, "LPSTR ");
      break;
    case TOKEN_UNICODE:
      add_string(buf, "UNICODE ");
      break;
    case TOKEN_CSTRING:
      add_string(buf, "CSTRING ");
      break;
    case TOKEN_ARRAY:
      add_string(buf, "array ");
      break;
    default:
      return 0;
  }

  if (0)
    dump_TOKEN(token);

  return token;
}

static inline BOOL is_primitive_type(WORD token)
{
  BOOL ret;
  switch(token)
  {
    case TOKEN_WORD:
    case TOKEN_DWORD:
    case TOKEN_FLOAT:
    case TOKEN_DOUBLE:
    case TOKEN_CHAR:
    case TOKEN_UCHAR:
    case TOKEN_SWORD:
    case TOKEN_SDWORD:
    case TOKEN_LPSTR:
    case TOKEN_UNICODE:
    case TOKEN_CSTRING:
      ret = 1;
      break;
    default:
      ret = 0;
      break;
  }
  return ret;
}

static BOOL parse_name(parse_buffer * buf)
{
  DWORD count;
  static char strname[100];

  if (parse_TOKEN(buf) != TOKEN_NAME)
    return FALSE;
  if (!read_bytes(buf, &count, 4))
    return FALSE;
  if (!read_bytes(buf, strname, count))
    return FALSE;
  strname[count] = 0;
  /*TRACE("name = %s\n", strname);*/
  add_string(buf, strname);
  add_string(buf, " ");

  return TRUE;
}

static BOOL parse_class_id(parse_buffer * buf)
{
  char strguid[38];
  GUID class_id;

  if (parse_TOKEN(buf) != TOKEN_GUID)
    return FALSE;
  if (!read_bytes(buf, &class_id, 16))
    return FALSE;
  sprintf(strguid, "<%08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X>", class_id.Data1, class_id.Data2, class_id.Data3, class_id.Data4[0],
    class_id.Data4[1], class_id.Data4[2], class_id.Data4[3], class_id.Data4[4], class_id.Data4[5], class_id.Data4[6], class_id.Data4[7]);
  /*TRACE("guid = {%s}\n", strguid);*/
  add_string(buf, strguid);

  return TRUE;
}

static BOOL parse_integer(parse_buffer * buf)
{
  DWORD integer;

  if (parse_TOKEN(buf) != TOKEN_INTEGER)
    return FALSE;
  if (!read_bytes(buf, &integer, 4))
    return FALSE;
  /*TRACE("integer = %ld\n", integer);*/
  sprintf(buf->dump+buf->pos, "%d ", integer);
  buf->pos = strlen(buf->dump);

  return TRUE;
}

static BOOL parse_template_option_info(parse_buffer * buf)
{
  if (check_TOKEN(buf) == TOKEN_DOT)
  {
    parse_TOKEN(buf);
    if (parse_TOKEN(buf) != TOKEN_DOT)
      return FALSE;
    if (parse_TOKEN(buf) != TOKEN_DOT)
      return FALSE;
    sprintf(buf->dump+buf->pos, " ");
    buf->pos = strlen(buf->dump);
  }
  else
  {
    while (1)
    {
      if (!parse_name(buf))
        return FALSE;
      if (check_TOKEN(buf) == TOKEN_GUID)
        if (!parse_class_id(buf))
          return FALSE;
      if (check_TOKEN(buf) != TOKEN_COMMA)
        break;
      parse_TOKEN(buf);
    }
  }
  return TRUE;
}

static BOOL parse_template_members_list(parse_buffer * buf)
{
  parse_buffer save1;
  while (1)
  {
    save1 = *buf;
    if (check_TOKEN(buf) == TOKEN_NAME)
    {
      if (!parse_name(buf))
        break;
      if (check_TOKEN(buf) == TOKEN_NAME)
        if (!parse_name(buf))
          break;
      if (parse_TOKEN(buf) != TOKEN_SEMICOLON)
        break;
    }
    else if (check_TOKEN(buf) == TOKEN_ARRAY)
    {
      parse_buffer save2;
      WORD token;
      parse_TOKEN(buf);
      token = check_TOKEN(buf);
      if (is_primitive_type(token))
      {
        parse_TOKEN(buf);
      }
      else
      {
        if (!parse_name(buf))
          break;
      }
      if (!parse_name(buf))
        break;
      save2 = *buf;
      while (check_TOKEN(buf) == TOKEN_OBRACKET)
      {
        parse_TOKEN(buf);
        if (check_TOKEN(buf) == TOKEN_INTEGER)
        {
          if (!parse_integer(buf))
            break;
        }
        else
        {
          if (!parse_name(buf))
            break;
        }
        if (parse_TOKEN(buf) != TOKEN_CBRACKET)
          break;
        save2 = *buf;
      }
      *buf = save2;
      if (parse_TOKEN(buf) != TOKEN_SEMICOLON)
        break;
    }
    else if (is_primitive_type(check_TOKEN(buf)))
    {
      parse_TOKEN(buf);
      if (check_TOKEN(buf) == TOKEN_NAME)
        if (!parse_name(buf))
          break;
      if (parse_TOKEN(buf) != TOKEN_SEMICOLON)
        break;
    }
    else
      break;
    add_string(buf, "\n");
  }
  *buf = save1;
  return TRUE;
}

static BOOL parse_template_parts(parse_buffer * buf)
{
  if (check_TOKEN(buf) == TOKEN_OBRACKET)
  {
    parse_TOKEN(buf);
    if (!parse_template_option_info(buf))
      return FALSE;
    if (parse_TOKEN(buf) != TOKEN_CBRACKET)
      return FALSE;
    add_string(buf, "\n");
  }
  else
  {
    if (!parse_template_members_list(buf))
      return FALSE;
    if (check_TOKEN(buf) == TOKEN_OBRACKET)
    {
      parse_TOKEN(buf);
      if (!parse_template_option_info(buf))
        return FALSE;
      if (parse_TOKEN(buf) != TOKEN_CBRACKET)
       return FALSE;
      add_string(buf, "\n");
    }
  }

  return TRUE;
}

static BOOL parse_template(parse_buffer * buf)
{
  if (parse_TOKEN(buf) != TOKEN_TEMPLATE)
    return FALSE;
  if (!parse_name(buf))
    return FALSE;
  add_string(buf, "\n");
  if (parse_TOKEN(buf) != TOKEN_OBRACE)
    return FALSE;
  add_string(buf, "\n");
  if (!parse_class_id(buf))
    return FALSE;
  add_string(buf, "\n");
  if (!parse_template_parts(buf))
    return FALSE;
  if (parse_TOKEN(buf) != TOKEN_CBRACE)
    return FALSE;
  add_string(buf, "\n\n");
  return TRUE;
}

static HRESULT WINAPI IDirectXFileImpl_RegisterTemplates(IDirectXFile* iface, LPVOID pvData, DWORD cbSize)
{
  IDirectXFileImpl *This = (IDirectXFileImpl *)iface;
  DWORD token_header;
  parse_buffer buf;

  buf.buffer = (LPBYTE)pvData;
  buf.rem_bytes = cbSize;
  buf.dump = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 500);
  buf.size = 500;

  FIXME("(%p/%p)->(%p,%d) partial stub!\n", This, iface, pvData, cbSize);

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

  if (token_header != XOFFILE_FORMAT_VERSION)
    return DXFILEERR_BADFILEVERSION;

  read_bytes(&buf, &token_header, 4);

  if ((token_header != XOFFILE_FORMAT_BINARY) && (token_header != XOFFILE_FORMAT_TEXT) && (token_header != XOFFILE_FORMAT_COMPRESSED))
    return DXFILEERR_BADFILETYPE;

  if (token_header == XOFFILE_FORMAT_TEXT)
  {
    FIXME("Text format not supported yet");
    return DXFILEERR_BADVALUE;
  }

  if (token_header == XOFFILE_FORMAT_COMPRESSED)
  {
    FIXME("Compressed formats not supported yet");
    return DXFILEERR_BADVALUE;
  }

  read_bytes(&buf, &token_header, 4);

  if ((token_header != XOFFILE_FORMAT_FLOAT_BITS_32) && (token_header != XOFFILE_FORMAT_FLOAT_BITS_64))
    return DXFILEERR_BADFILEFLOATSIZE;

  while (buf.rem_bytes)
  {
    buf.pos = 0;
    if (!parse_template(&buf))
    {
      TRACE("Template is not correct\n");
      return DXFILEERR_BADVALUE;
    }
    else
      TRACE("Template successfully parsed:\n");
      if (TRACE_ON(d3dxof))
        DPRINTF(buf.dump);
  }

  HeapFree(GetProcessHeap(), 0, buf.dump);

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

HRESULT IDirectXFileBinaryImpl_Create(IDirectXFileBinaryImpl** ppObj)
{
    IDirectXFileBinaryImpl* object;

    TRACE("(%p)\n", ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileBinaryImpl));
    
    object->lpVtbl.lpVtbl = &IDirectXFileBinary_Vtbl;
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
    IClassFactory_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

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

HRESULT IDirectXFileDataImpl_Create(IDirectXFileDataImpl** ppObj)
{
    IDirectXFileDataImpl* object;

    TRACE("(%p)\n", ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileDataImpl));
    
    object->lpVtbl.lpVtbl = &IDirectXFileData_Vtbl;
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
    IClassFactory_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

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
    HeapFree(GetProcessHeap(), 0, This);

  return ref;
}

/*** IDirectXFileObject methods ***/
static HRESULT WINAPI IDirectXFileDataImpl_GetName(IDirectXFileData* iface, LPSTR pstrNameBuf, LPDWORD pdwBufLen)

{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%p,%p) stub!\n", This, iface, pstrNameBuf, pdwBufLen); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataImpl_GetId(IDirectXFileData* iface, LPGUID pGuid)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pGuid); 

  return DXFILEERR_BADVALUE;
}

/*** IDirectXFileData methods ***/
static HRESULT WINAPI IDirectXFileDataImpl_GetData(IDirectXFileData* iface, LPCSTR szMember, DWORD* pcbSize, void** ppvData)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%s,%p,%p) stub!\n", This, iface, szMember, pcbSize, ppvData); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataImpl_GetType(IDirectXFileData* iface, const GUID** pguid)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pguid); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataImpl_GetNextObject(IDirectXFileData* iface, LPDIRECTXFILEOBJECT* ppChildObj)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, ppChildObj); 

  return DXFILEERR_BADVALUE;
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

HRESULT IDirectXFileDataReferenceImpl_Create(IDirectXFileDataReferenceImpl** ppObj)
{
    IDirectXFileDataReferenceImpl* object;

    TRACE("(%p)\n", ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileDataReferenceImpl));
    
    object->lpVtbl.lpVtbl = &IDirectXFileDataReference_Vtbl;
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
    IClassFactory_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

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

  FIXME("(%p/%p)->(%p,%p) stub!\n", This, iface, pstrNameBuf, pdwBufLen); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataReferenceImpl_GetId(IDirectXFileDataReference* iface, LPGUID pGuid)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pGuid); 

  return DXFILEERR_BADVALUE;
}

/*** IDirectXFileDataReference ***/
static HRESULT WINAPI IDirectXFileDataReferenceImpl_Resolve(IDirectXFileDataReference* iface, LPDIRECTXFILEDATA* ppDataObj)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, ppDataObj); 

  return DXFILEERR_BADVALUE;
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

HRESULT IDirectXFileEnumObjectImpl_Create(IDirectXFileEnumObjectImpl** ppObj)
{
    IDirectXFileEnumObjectImpl* object;

    TRACE("(%p)\n", ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileEnumObjectImpl));
    
    object->lpVtbl.lpVtbl = &IDirectXFileEnumObject_Vtbl;
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
    IClassFactory_AddRef(iface);
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
    HeapFree(GetProcessHeap(), 0, This);

  return ref;
}

/*** IDirectXFileEnumObject methods ***/
static HRESULT WINAPI IDirectXFileEnumObjectImpl_GetNextDataObject(IDirectXFileEnumObject* iface, LPDIRECTXFILEDATA* ppDataObj)
{
  IDirectXFileEnumObjectImpl *This = (IDirectXFileEnumObjectImpl *)iface;
  IDirectXFileDataImpl* object;
  HRESULT hr;
  
  FIXME("(%p/%p)->(%p) stub!\n", This, iface, ppDataObj); 

  hr = IDirectXFileDataImpl_Create(&object);
  if (!SUCCEEDED(hr))
    return hr;

  *ppDataObj = (LPDIRECTXFILEDATA)object;

  return DXFILE_OK;
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

HRESULT IDirectXFileObjectImpl_Create(IDirectXFileObjectImpl** ppObj)
{
    IDirectXFileObjectImpl* object;

    TRACE("(%p)\n", ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileObjectImpl));
    
    object->lpVtbl.lpVtbl = &IDirectXFileObject_Vtbl;
    object->ref = 1;

    *ppObj = object;
    
    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectXFileObjectImpl_QueryInterface(IDirectXFileObject* iface, REFIID riid, void** ppvObject)
{
  IDirectXFileObjectImpl *This = (IDirectXFileObjectImpl *)iface;

  TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_IDirectXFileObject))
  {
    IClassFactory_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

  ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectXFileObjectImpl_AddRef(IDirectXFileObject* iface)
{
  IDirectXFileObjectImpl *This = (IDirectXFileObjectImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p/%p): AddRef from %d\n", iface, This, ref - 1);

  return ref;
}

static ULONG WINAPI IDirectXFileObjectImpl_Release(IDirectXFileObject* iface)
{
  IDirectXFileObjectImpl *This = (IDirectXFileObjectImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p/%p): ReleaseRef to %d\n", iface, This, ref);

  if (!ref)
    HeapFree(GetProcessHeap(), 0, This);

  return ref;
}

/*** IDirectXFileObject methods ***/
static HRESULT WINAPI IDirectXFileObjectImpl_GetName(IDirectXFileObject* iface, LPSTR pstrNameBuf, LPDWORD pdwBufLen)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;

  FIXME("(%p/%p)->(%p,%p) stub!\n", This, iface, pstrNameBuf, pdwBufLen); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileObjectImpl_GetId(IDirectXFileObject* iface, LPGUID pGuid)
{
  IDirectXFileObjectImpl *This = (IDirectXFileObjectImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pGuid); 

  return DXFILEERR_BADVALUE;
}

static const IDirectXFileObjectVtbl IDirectXFileObject_Vtbl =
{
    IDirectXFileObjectImpl_QueryInterface,
    IDirectXFileObjectImpl_AddRef,
    IDirectXFileObjectImpl_Release,
    IDirectXFileObjectImpl_GetName,
    IDirectXFileObjectImpl_GetId
};

HRESULT IDirectXFileSaveObjectImpl_Create(IDirectXFileSaveObjectImpl** ppObj)
{
    IDirectXFileSaveObjectImpl* object;

    TRACE("(%p)\n", ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileSaveObjectImpl));
    
    object->lpVtbl.lpVtbl = &IDirectXFileSaveObject_Vtbl;
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
    IClassFactory_AddRef(iface);
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
