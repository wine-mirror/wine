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

#define CLSIDFMT "<%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X>"

typedef struct {
  /* Buffer to parse */
  LPBYTE buffer;
  DWORD rem_bytes;
  /* Misc info */
  BOOL txt;
  BYTE value[100];
  IDirectXFileImpl* pdxf;
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
  DWORD header[4];
  DWORD size;
  HANDLE hFile = INVALID_HANDLE_VALUE;

  FIXME("(%p/%p)->(%p,%x,%p) partial stub!\n", This, iface, pvSource, dwLoadOptions, ppEnumObj);

  if (!ppEnumObj)
    return DXFILEERR_BADVALUE;

  if (dwLoadOptions == DXFILELOAD_FROMFILE)
  {
    TRACE("Open source file '%s'\n", (char*)pvSource);

    hFile = CreateFileA((char*)pvSource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
      TRACE("File '%s' not found\n", (char*)pvSource);
      return DXFILEERR_FILENOTFOUND;
    }

    if (!ReadFile(hFile, header, 16, &size, NULL))
    {
      hr = DXFILEERR_BADVALUE;
      goto error;
    }

    if (size < 16)
    {
      hr = DXFILEERR_BADFILETYPE;
      goto error;
    }

    if (TRACE_ON(d3dxof))
    {
      char string[17];
      memcpy(string, header, 16);
      string[16] = 0;
      TRACE("header = '%s'\n", string);
    }
  }
  else
  {
    FIXME("Source type %d is not handled yet\n", dwLoadOptions);
    hr = DXFILEERR_NOTDONEYET;
    goto error;
  }

  if (header[0] != XOFFILE_FORMAT_MAGIC)
  {
    hr = DXFILEERR_BADFILETYPE;
    goto error;
  }

  if (header[1] != XOFFILE_FORMAT_VERSION)
  {
    hr = DXFILEERR_BADFILEVERSION;
    goto error;
  }

  if ((header[2] != XOFFILE_FORMAT_BINARY) && (header[2] != XOFFILE_FORMAT_TEXT) && (header[2] != XOFFILE_FORMAT_COMPRESSED))
  {
    hr = DXFILEERR_BADFILETYPE;
    goto error;
  }

  if (header[2] == XOFFILE_FORMAT_BINARY)
  {
    FIXME("Binary format not supported yet\n");
    hr = DXFILEERR_NOTDONEYET;
    goto error;
  }

  if (header[2] == XOFFILE_FORMAT_COMPRESSED)
  {
    FIXME("Compressed formats not supported yet");
    hr = DXFILEERR_BADVALUE;
    goto error;
  }

  if ((header[3] != XOFFILE_FORMAT_FLOAT_BITS_32) && (header[3] != XOFFILE_FORMAT_FLOAT_BITS_64))
  {
    hr = DXFILEERR_BADFILEFLOATSIZE;
    goto error;
  }

  TRACE("Header is correct\n");

  hr = IDirectXFileEnumObjectImpl_Create(&object);
  if (!SUCCEEDED(hr))
    goto error;

  object->source = dwLoadOptions;
  object->hFile = hFile;

  *ppEnumObj = (LPDIRECTXFILEENUMOBJECT)object;

  return DXFILE_OK;

error:
  if (hFile != INVALID_HANDLE_VALUE)
    CloseHandle(hFile);
  *ppEnumObj = NULL;

  return hr;
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

static BOOL is_space(char c)
{
  switch (c)
  {
    case 0x00:
    case 0x0D:
    case 0x0A:
    case ' ':
    case '\t':
      return TRUE;
  }
  return FALSE;
}

static BOOL is_operator(char c)
{
  switch(c)
  {
    case '{':
    case '}':
    case '[':
    case ']':
    case '(':
    case ')':
    case '<':
    case '>':
    case ',':
    case ';':
      return TRUE;
  }
  return FALSE;
}

static inline BOOL is_separator(char c)
{
  return is_space(c) || is_operator(c);
}

static WORD get_operator_token(char c)
{
  switch(c)
  {
    case '{':
      return TOKEN_OBRACE;
    case '}':
      return TOKEN_CBRACE;
    case '[':
      return TOKEN_OBRACKET;
    case ']':
      return TOKEN_CBRACKET;
    case '(':
      return TOKEN_OPAREN;
    case ')':
      return TOKEN_CPAREN;
    case '<':
      return TOKEN_OANGLE;
    case '>':
      return TOKEN_CANGLE;
    case ',':
      return TOKEN_COMMA;
    case ';':
      return TOKEN_SEMICOLON;
  }
  return 0;
}

static BOOL is_keyword(parse_buffer* buf, const char* keyword)
{
  DWORD len = strlen(keyword);
  if (!strncmp((char*)buf->buffer, keyword,len) && is_separator(*(buf->buffer+len)))
  {
    buf->buffer += len;
    buf->rem_bytes -= len;
    return TRUE;
  }
  return FALSE;
}

static WORD get_keyword_token(parse_buffer* buf)
{
  if (is_keyword(buf, "template"))
    return TOKEN_TEMPLATE;
  if (is_keyword(buf, "WORD"))
    return TOKEN_WORD;
  if (is_keyword(buf, "DWORD"))
    return TOKEN_DWORD;
  if (is_keyword(buf, "FLOAT"))
    return TOKEN_FLOAT;
  if (is_keyword(buf, "DOUBLE"))
    return TOKEN_DOUBLE;
  if (is_keyword(buf, "CHAR"))
    return TOKEN_CHAR;
  if (is_keyword(buf, "UCHAR"))
    return TOKEN_UCHAR;
  if (is_keyword(buf, "SWORD"))
    return TOKEN_SWORD;
  if (is_keyword(buf, "SDWORD"))
    return TOKEN_SDWORD;
  if (is_keyword(buf, "VOID"))
    return TOKEN_VOID;
  if (is_keyword(buf, "STRING"))
    return TOKEN_LPSTR;
  if (is_keyword(buf, "UNICODE"))
    return TOKEN_UNICODE;
  if (is_keyword(buf, "CSTRING"))
    return TOKEN_CSTRING;
  if (is_keyword(buf, "array"))
    return TOKEN_ARRAY;

  return 0;
}

static BOOL is_guid(parse_buffer* buf)
{
  char tmp[50];
  DWORD pos = 1;
  GUID class_id;
  DWORD tab[10];
  int ret;

  if (*buf->buffer != '<')
    return FALSE;
  tmp[0] = '<';
  while (*(buf->buffer+pos) != '>')
  {
    tmp[pos] = *(buf->buffer+pos);
    pos++;
  }
  tmp[pos++] = '>';
  tmp[pos] = 0;
  if (pos != 38 /* <+36+> */)
  {
    TRACE("Wrong guid %s (%d)\n", tmp, pos);
    return FALSE;
  }
  buf->buffer += pos;
  buf->rem_bytes -= pos;

  ret = sscanf(tmp, CLSIDFMT, &class_id.Data1, tab, tab+1, tab+2, tab+3, tab+4, tab+5, tab+6, tab+7, tab+8, tab+9);
  if (ret != 11)
  {
    TRACE("Wrong guid %s (%d)\n", tmp, pos);
    return FALSE;
  }
  TRACE("Found guid %s (%d)\n", tmp, pos);

  class_id.Data2 = tab[0];
  class_id.Data3 = tab[1];
  class_id.Data4[0] = tab[2];
  class_id.Data4[1] = tab[3];
  class_id.Data4[2] = tab[4];
  class_id.Data4[3] = tab[5];
  class_id.Data4[4] = tab[6];
  class_id.Data4[5] = tab[7];
  class_id.Data4[6] = tab[8];
  class_id.Data4[7] = tab[9];

  *(GUID*)buf->value = class_id;

  return TRUE;
}

static BOOL is_name(parse_buffer* buf)
{
  char tmp[50];
  DWORD pos = 0;
  char c;
  BOOL error = 0;
  while (!is_separator(c = *(buf->buffer+pos)))
  {
    if (!(((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9'))))
      error = 1;
    tmp[pos++] = c;
  }
  tmp[pos] = 0;

  if (error)
  {
    TRACE("Wrong name %s\n", tmp);
    return FALSE;
  }

  buf->buffer += pos;
  buf->rem_bytes -= pos;

  TRACE("Found name %s\n", tmp);
  strcpy((char*)buf->value, tmp);

  return TRUE;
}

static BOOL is_integer(parse_buffer* buf)
{
  char tmp[50];
  DWORD pos = 0;
  char c;
  DWORD integer;

  while (!is_separator(c = *(buf->buffer+pos)))
  {
    if (!((c >= '0') && (c <= '9')))
      return FALSE;
    tmp[pos++] = c;
  }
  tmp[pos] = 0;

  buf->buffer += pos;
  buf->rem_bytes -= pos;

  sscanf(tmp, "%d", &integer);

  TRACE("Found integer %s - %d\n", tmp, integer);

  *(WORD*)buf->value = integer;

  return TRUE;
}

static WORD parse_TOKEN_dbg_opt(parse_buffer * buf, BOOL show_token)
{
  WORD token;

  if (buf->txt)
  {
    while(1)
    {
      char c;
      if (!read_bytes(buf, &c, 1))
        return 0;
      /*TRACE("char = '%c'\n", is_space(c) ? ' ' : c);*/
      if ((c == '#') || (c == '/'))
      {
        /* Handle comment (# or //) */
        if (c == '/')
        {
          if (!read_bytes(buf, &c, 1))
            return 0;
          if (c != '/')
            return 0;
        }
        c = 0;
        while (c != 0x0A)
        {
          if (!read_bytes(buf, &c, 1))
            return 0;
        }
        continue;
      }
      if (is_space(c))
        continue;
      if (is_operator(c) && (c != '<'))
      {
        token = get_operator_token(c);
        break;
      }
      else if (c == '.')
      {
        token = TOKEN_DOT;
        break;
      }
      else
      {
        buf->buffer -= 1;
        buf->rem_bytes += 1;

        if ((token = get_keyword_token(buf)))
          break;

        if (is_guid(buf))
        {
          token = TOKEN_GUID;
          break;
        }
        if (is_integer(buf))
        {
          token = TOKEN_INTEGER;
          break;
        }
        if (is_name(buf))
        {
          token = TOKEN_NAME;
          break;
        }

        FIXME("Unrecognize element\n");
        return 0;
      }
    }
  }
  else
  {
    if (!read_bytes(buf, &token, 2))
      return 0;
  }

  switch(token)
  {
    case TOKEN_NAME:
    case TOKEN_STRING:
    case TOKEN_INTEGER:
    case TOKEN_GUID:
    case TOKEN_INTEGER_LIST:
    case TOKEN_FLOAT_LIST:
    case TOKEN_OBRACE:
    case TOKEN_CBRACE:
    case TOKEN_OPAREN:
    case TOKEN_CPAREN:
    case TOKEN_OBRACKET:
    case TOKEN_CBRACKET:
    case TOKEN_OANGLE:
    case TOKEN_CANGLE:
    case TOKEN_DOT:
    case TOKEN_COMMA:
    case TOKEN_SEMICOLON:
    case TOKEN_TEMPLATE:
    case TOKEN_WORD:
    case TOKEN_DWORD:
    case TOKEN_FLOAT:
    case TOKEN_DOUBLE:
    case TOKEN_CHAR:
    case TOKEN_UCHAR:
    case TOKEN_SWORD:
    case TOKEN_SDWORD:
    case TOKEN_VOID:
    case TOKEN_LPSTR:
    case TOKEN_UNICODE:
    case TOKEN_CSTRING:
    case TOKEN_ARRAY:
      break;
    default:
      return 0;
  }

  if (show_token)
    dump_TOKEN(token);

  return token;
}

static const char* get_primitive_string(WORD token)
{
  switch(token)
  {
    case TOKEN_WORD:
      return "WORD";
    case TOKEN_DWORD:
      return "DWORD";
    case TOKEN_FLOAT:
      return "FLOAT";
    case TOKEN_DOUBLE:
      return "DOUBLE";
    case TOKEN_CHAR:
      return "CHAR";
    case TOKEN_UCHAR:
      return "UCHAR";
    case TOKEN_SWORD:
      return "SWORD";
    case TOKEN_SDWORD:
      return "SDWORD";
    case TOKEN_VOID:
      return "VOID";
    case TOKEN_LPSTR:
      return "STRING";
    case TOKEN_UNICODE:
      return "UNICODE";
    case TOKEN_CSTRING:
      return "CSTRING ";
    default:
      break;
  }
  return NULL;
}

static inline WORD parse_TOKEN(parse_buffer * buf)
{
  return parse_TOKEN_dbg_opt(buf, TRUE);
}

static WORD check_TOKEN(parse_buffer * buf)
{
  WORD token;

  if (buf->txt)
  {
    parse_buffer save = *buf;
    /*TRACE("check: ");*/
    token = parse_TOKEN_dbg_opt(buf, FALSE);
    *buf = save;
    return token;
  }

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
  char strname[100];

  if (parse_TOKEN(buf) != TOKEN_NAME)
    return FALSE;
  if (buf->txt)
    return TRUE;
  if (!read_bytes(buf, &count, 4))
    return FALSE;
  if (!read_bytes(buf, strname, count))
    return FALSE;
  strname[count] = 0;
  /*TRACE("name = %s\n", strname);*/

  strcpy((char*)buf->value, strname);

  return TRUE;
}

static BOOL parse_class_id(parse_buffer * buf)
{
  char strguid[38];
  GUID class_id;

  if (parse_TOKEN(buf) != TOKEN_GUID)
    return FALSE;
  if (buf->txt)
    return TRUE;
  if (!read_bytes(buf, &class_id, 16))
    return FALSE;
  sprintf(strguid, CLSIDFMT, class_id.Data1, class_id.Data2, class_id.Data3, class_id.Data4[0],
    class_id.Data4[1], class_id.Data4[2], class_id.Data4[3], class_id.Data4[4], class_id.Data4[5], class_id.Data4[6], class_id.Data4[7]);
  /*TRACE("guid = {%s}\n", strguid);*/

  *(GUID*)buf->value = class_id;

  return TRUE;
}

static BOOL parse_integer(parse_buffer * buf)
{
  DWORD integer;

  if (parse_TOKEN(buf) != TOKEN_INTEGER)
    return FALSE;
  if (buf->txt)
    return TRUE;
  if (!read_bytes(buf, &integer, 4))
    return FALSE;
  /*TRACE("integer = %ld\n", integer);*/

  *(DWORD*)buf->value = integer;

  return TRUE;
}

static BOOL parse_template_option_info(parse_buffer * buf)
{
  xtemplate* cur_template = &buf->pdxf->xtemplates[buf->pdxf->nb_xtemplates];

  if (check_TOKEN(buf) == TOKEN_DOT)
  {
    parse_TOKEN(buf);
    if (parse_TOKEN(buf) != TOKEN_DOT)
      return FALSE;
    if (parse_TOKEN(buf) != TOKEN_DOT)
      return FALSE;
    cur_template->open = TRUE;
  }
  else
  {
    while (1)
    {
      if (!parse_name(buf))
        return FALSE;
      strcpy(cur_template->childs[cur_template->nb_childs], (char*)buf->value);
      if (check_TOKEN(buf) == TOKEN_GUID)
        if (!parse_class_id(buf))
          return FALSE;
      cur_template->nb_childs++;
      if (check_TOKEN(buf) != TOKEN_COMMA)
        break;
      parse_TOKEN(buf);
    }
    cur_template->open = FALSE;
  }

  return TRUE;
}

static BOOL parse_template_members_list(parse_buffer * buf)
{
  parse_buffer save1;
  int idx_member = 0;
  member* cur_member;

  while (1)
  {
    cur_member = &buf->pdxf->xtemplates[buf->pdxf->nb_xtemplates].members[idx_member];
    save1 = *buf;

    if (check_TOKEN(buf) == TOKEN_NAME)
    {
      if (!parse_name(buf))
        break;
      while (cur_member->idx_template < buf->pdxf->nb_xtemplates)
      {
        if (!strcmp((char*)buf->value, buf->pdxf->xtemplates[cur_member->idx_template].name))
          break;
        cur_member->idx_template++;
      }
      if (cur_member->idx_template == buf->pdxf->nb_xtemplates)
      {
        TRACE("Reference to a nonexistent template '%s'\n", (char*)buf->value);
        return FALSE;
      }
      if (check_TOKEN(buf) == TOKEN_NAME)
        if (!parse_name(buf))
          break;
      if (parse_TOKEN(buf) != TOKEN_SEMICOLON)
        break;
      cur_member->type = TOKEN_NAME;
      strcpy(cur_member->name, (char*)buf->value);
      idx_member++;
    }
    else if (check_TOKEN(buf) == TOKEN_ARRAY)
    {
      parse_buffer save2;
      WORD token;
      int nb_dims = 0;

      parse_TOKEN(buf);
      token = check_TOKEN(buf);
      if (is_primitive_type(token))
      {
        parse_TOKEN(buf);
        cur_member->type = token;
      }
      else
      {
        if (!parse_name(buf))
          break;
        cur_member->type = TOKEN_NAME;
        cur_member->idx_template = 0;
        while (cur_member->idx_template < buf->pdxf->nb_xtemplates)
        {
          if (!strcmp((char*)buf->value, buf->pdxf->xtemplates[cur_member->idx_template].name))
            break;
          cur_member->idx_template++;
        }
        if (cur_member->idx_template == buf->pdxf->nb_xtemplates)
        {
          TRACE("Reference to nonexistent template '%s'\n", (char*)buf->value);
          return FALSE;
        }
      }
      if (!parse_name(buf))
        break;
      strcpy(cur_member->name, (char*)buf->value);
      save2 = *buf;
      while (check_TOKEN(buf) == TOKEN_OBRACKET)
      {
        if (nb_dims)
        {
          FIXME("No support for multi-dimensional array yet\n");
          return FALSE;
        }
        parse_TOKEN(buf);
        if (check_TOKEN(buf) == TOKEN_INTEGER)
        {
          if (!parse_integer(buf))
            break;
          cur_member->dim_fixed[nb_dims] = TRUE;
          cur_member->dim_value[nb_dims] = *(DWORD*)buf->value;
        }
        else
        {
          if (!parse_name(buf))
            break;
          cur_member->dim_fixed[nb_dims] = FALSE;
          /* Hack: Assume array size is specified in previous member */
          cur_member->dim_value[nb_dims] = idx_member - 1;
        }
        if (parse_TOKEN(buf) != TOKEN_CBRACKET)
          break;
        save2 = *buf;
        nb_dims++;
      }
      *buf = save2;
      if (parse_TOKEN(buf) != TOKEN_SEMICOLON)
        break;
      cur_member->nb_dims = nb_dims;
      idx_member++;
    }
    else if (is_primitive_type(check_TOKEN(buf)))
    {
      cur_member->type = check_TOKEN(buf);
      parse_TOKEN(buf);
      if (check_TOKEN(buf) == TOKEN_NAME)
        if (!parse_name(buf))
          break;
      strcpy(cur_member->name, (char*)buf->value);
      if (parse_TOKEN(buf) != TOKEN_SEMICOLON)
        break;
      idx_member++;
    }
    else
      break;
  }

  *buf = save1;
  buf->pdxf->xtemplates[buf->pdxf->nb_xtemplates].nb_members = idx_member;

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
  strcpy(buf->pdxf->xtemplates[buf->pdxf->nb_xtemplates].name, (char*)buf->value);
  if (parse_TOKEN(buf) != TOKEN_OBRACE)
    return FALSE;
  if (!parse_class_id(buf))
    return FALSE;
  buf->pdxf->xtemplates[buf->pdxf->nb_xtemplates].class_id = *(GUID*)buf->value;
  if (!parse_template_parts(buf))
    return FALSE;
  if (parse_TOKEN(buf) != TOKEN_CBRACE)
    return FALSE;
  if (buf->txt)
  {
    /* Go to the next template */
    while (buf->rem_bytes && is_space(*buf->buffer))
    {
      buf->buffer++;
      buf->rem_bytes--;
    }
  }

  TRACE("%d - %s - %s\n", buf->pdxf->nb_xtemplates, buf->pdxf->xtemplates[buf->pdxf->nb_xtemplates].name, debugstr_guid(&buf->pdxf->xtemplates[buf->pdxf->nb_xtemplates].class_id));
  buf->pdxf->nb_xtemplates++;

  return TRUE;
}

static HRESULT WINAPI IDirectXFileImpl_RegisterTemplates(IDirectXFile* iface, LPVOID pvData, DWORD cbSize)
{
  IDirectXFileImpl *This = (IDirectXFileImpl *)iface;
  DWORD token_header;
  parse_buffer buf;

  buf.buffer = (LPBYTE)pvData;
  buf.rem_bytes = cbSize;
  buf.txt = FALSE;
  buf.pdxf = This;

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
    buf.txt = TRUE;
  }

  if (token_header == XOFFILE_FORMAT_COMPRESSED)
  {
    FIXME("Compressed formats not supported yet\n");
    return DXFILEERR_BADVALUE;
  }

  read_bytes(&buf, &token_header, 4);

  if ((token_header != XOFFILE_FORMAT_FLOAT_BITS_32) && (token_header != XOFFILE_FORMAT_FLOAT_BITS_64))
    return DXFILEERR_BADFILEFLOATSIZE;

  TRACE("Header is correct\n");

  while (buf.rem_bytes)
  {
    if (!parse_template(&buf))
    {
      TRACE("Template is not correct\n");
      return DXFILEERR_BADVALUE;
    }
    else
    {
      TRACE("Template successfully parsed:\n");
      if (TRACE_ON(d3dxof))
      {
        int i,j,k;
        GUID* clsid;

        i = This->nb_xtemplates - 1;
        clsid = &This->xtemplates[i].class_id;

        DPRINTF("template %s\n", This->xtemplates[i].name);
        DPRINTF("{\n");
        DPRINTF(CLSIDFMT "\n", clsid->Data1, clsid->Data2, clsid->Data3, clsid->Data4[0],
          clsid->Data4[1], clsid->Data4[2], clsid->Data4[3], clsid->Data4[4], clsid->Data4[5], clsid->Data4[6], clsid->Data4[7]);
        for (j = 0; j < This->xtemplates[i].nb_members; j++)
        {
          if (This->xtemplates[i].members[j].nb_dims)
            DPRINTF("array ");
          if (This->xtemplates[i].members[j].type == TOKEN_NAME)
            DPRINTF("%s ", This->xtemplates[This->xtemplates[i].members[j].idx_template].name);
          else
            DPRINTF("%s ", get_primitive_string(This->xtemplates[i].members[j].type));
          DPRINTF("%s", This->xtemplates[i].members[j].name);
          for (k = 0; k < This->xtemplates[i].members[j].nb_dims; k++)
          {
            if (This->xtemplates[i].members[j].dim_fixed[k])
              DPRINTF("[%d]", This->xtemplates[i].members[j].dim_value[k]);
            else
              DPRINTF("[%s]", This->xtemplates[i].members[This->xtemplates[i].members[j].dim_value[k]].name);
          }
          DPRINTF(";\n");
        }
        if (This->xtemplates[i].open)
          DPRINTF("[...]\n");
        else if (This->xtemplates[i].nb_childs)
        {
          DPRINTF("[%s", This->xtemplates[i].childs[0]);
          for (j = 1; j < This->xtemplates[i].nb_childs; j++)
            DPRINTF(",%s", This->xtemplates[i].childs[j]);
          DPRINTF("]\n");
        }
        DPRINTF("}\n");
      }
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
