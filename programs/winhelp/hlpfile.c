/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
 */

#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "windowsx.h"
#include "winhelp.h"

static void Report(LPCSTR str)
{
#if 0
  fprintf(stderr, "%s\n", str);
#endif
}

#define GET_USHORT(buffer, i)\
(((BYTE)((buffer)[(i)]) + 0x100 * (BYTE)((buffer)[(i)+1])))
#define GET_SHORT(buffer, i)\
(((BYTE)((buffer)[(i)]) + 0x100 * (signed char)((buffer)[(i)+1])))
#define GET_UINT(buffer, i)\
GET_USHORT(buffer, i) + 0x10000 * GET_USHORT(buffer, i+2)

static BOOL HLPFILE_DoReadHlpFile(HLPFILE*, LPCSTR);
static BOOL HLPFILE_ReadFileToBuffer(HFILE);
static BOOL HLPFILE_FindSubFile(LPCSTR name, BYTE**, BYTE**);
static VOID HLPFILE_SystemCommands(HLPFILE*);
static BOOL HLPFILE_Uncompress1_Phrases();
static BOOL HLPFILE_Uncompress1_Topic();
static BOOL HLPFILE_GetContext(HLPFILE*);
static BOOL HLPFILE_AddPage(HLPFILE*, BYTE*, BYTE*);
static BOOL HLPFILE_AddParagraph(HLPFILE*, BYTE *, BYTE*);
static UINT HLPFILE_Uncompressed2_Size(BYTE*, BYTE*);
static VOID HLPFILE_Uncompress2(BYTE**, BYTE*, BYTE*);

static HLPFILE *first_hlpfile = 0;
static HGLOBAL  hFileBuffer;
static BYTE    *file_buffer;

static struct
{
  UINT    num;
  BYTE   *buf;
  HGLOBAL hBuffer;
} phrases;

static struct
{
  BYTE    **map;
  BYTE    *end;
  UINT    wMapLen;
  HGLOBAL hMap;
  HGLOBAL hBuffer;
} topic;

static struct
{
  UINT  bDebug;
  UINT  wFont;
  UINT  wIndent;
  UINT  wHSpace;
  UINT  wVSpace;
  UINT  wVBackSpace;
  HLPFILE_LINK link;
} attributes;

/***********************************************************************
 *
 *           HLPFILE_Contents
 */

HLPFILE_PAGE *HLPFILE_Contents(LPCSTR lpszPath)
{
  HLPFILE *hlpfile = HLPFILE_ReadHlpFile(lpszPath);

  if (!hlpfile) return(0);

  return(hlpfile->first_page);
}

/***********************************************************************
 *
 *           HLPFILE_PageByNumber
 */

HLPFILE_PAGE *HLPFILE_PageByNumber(LPCSTR lpszPath, UINT wNum)
{
  HLPFILE_PAGE *page;
  HLPFILE *hlpfile = HLPFILE_ReadHlpFile(lpszPath);

  if (!hlpfile) return(0);

  for (page = hlpfile->first_page; page && wNum; page = page->next) wNum--;

  return page;
}

/***********************************************************************
 *
 *           HLPFILE_HlpFilePageByHash
 */

HLPFILE_PAGE *HLPFILE_PageByHash(LPCSTR lpszPath, LONG lHash)
{
  INT i;
  UINT wNum;
  HLPFILE_PAGE *page;
  HLPFILE *hlpfile = HLPFILE_ReadHlpFile(lpszPath);

  if (!hlpfile) return(0);

  for (i = 0; i < hlpfile->wContextLen; i++)
    if (hlpfile->Context[i].lHash == lHash) break;

  if (i >= hlpfile->wContextLen)
    {
      HLPFILE_FreeHlpFile(hlpfile);
      return(0);
    }

  wNum = hlpfile->Context[i].wPage;
  for (page = hlpfile->first_page; page && wNum; page = page->next) wNum--;

  return page;
}

/***********************************************************************
 *
 *           HLPFILE_Hash
 */

LONG HLPFILE_Hash(LPCSTR lpszContext)
{
  LONG lHash = 0;
  CHAR c;
  while((c = *lpszContext++))
    {
      CHAR x = 0;
      if (c >= 'A' && c <= 'Z') x = c - 'A' + 17;
      if (c >= 'a' && c <= 'z') x = c - 'a' + 17;
      if (c >= '1' && c <= '9') x = c - '0';
      if (c == '0') x = 10;
      if (c == '.') x = 12;
      if (c == '_') x = 13;
      if (x) lHash = lHash * 43 + x;
    }
  return lHash;
}

/***********************************************************************
 *
 *           HLPFILE_ReadHlpFile
 */

HLPFILE *HLPFILE_ReadHlpFile(LPCSTR lpszPath)
{
  HGLOBAL   hHlpFile;
  HLPFILE *hlpfile;

  for (hlpfile = first_hlpfile; hlpfile; hlpfile = hlpfile->next)
    if (!lstrcmp(hlpfile->lpszPath, lpszPath))
      {
	hlpfile->wRefCount++;
	return(hlpfile);
      }

  hHlpFile = GlobalAlloc(GMEM_FIXED, sizeof(HLPFILE) + lstrlen(lpszPath) + 1);
  if (!hHlpFile) return(0);

  hlpfile              = GlobalLock(hHlpFile);
  hlpfile->hSelf       = hHlpFile;
  hlpfile->wRefCount   = 1;
  hlpfile->hTitle      = 0;
  hlpfile->hContext    = 0;
  hlpfile->wContextLen = 0;
  hlpfile->first_page  = 0;
  hlpfile->first_macro = 0;
  hlpfile->prev        = 0;
  hlpfile->next        = first_hlpfile;
  first_hlpfile   = hlpfile;
  if (hlpfile->next) hlpfile->next->prev = hlpfile;

  hlpfile->lpszPath   = GlobalLock(hHlpFile);
  hlpfile->lpszPath  += sizeof(HLPFILE);
  strcpy(hlpfile->lpszPath, lpszPath);

  phrases.hBuffer = topic.hBuffer = hFileBuffer = 0;

  if (!HLPFILE_DoReadHlpFile(hlpfile, lpszPath))
    {
      HLPFILE_FreeHlpFile(hlpfile);
      hlpfile = 0;
    }

  if (phrases.hBuffer) GlobalFree(phrases.hBuffer);
  if (topic.hBuffer)   GlobalFree(topic.hBuffer);
  if (topic.hMap)      GlobalFree(topic.hMap);
  if (hFileBuffer)     GlobalFree(hFileBuffer);

  return(hlpfile);
}

/***********************************************************************
 *
 *           HLPFILE_DoReadHlpFile
 */

static BOOL HLPFILE_DoReadHlpFile(HLPFILE *hlpfile, LPCSTR lpszPath)
{
  BOOL     ret;
  HFILE    hFile;
  OFSTRUCT ofs;
  BYTE     *buf;

  hFile=OpenFile(lpszPath, &ofs, OF_READ | OF_SEARCH);
  if (hFile == HFILE_ERROR) return FALSE;

  ret = HLPFILE_ReadFileToBuffer(hFile);
  _lclose(hFile);
  if (!ret) return FALSE;

  HLPFILE_SystemCommands(hlpfile);
  if (!HLPFILE_Uncompress1_Phrases()) return FALSE;
  if (!HLPFILE_Uncompress1_Topic()) return FALSE;

  buf = topic.map[0] + 0xc;
  while(buf + 0xc < topic.end)
    {
      BYTE *end = MIN(buf + GET_UINT(buf, 0), topic.end);
      UINT next, index, offset;

      switch (buf[0x14])
	{
	case 0x02:
	  if (!HLPFILE_AddPage(hlpfile, buf, end)) return(FALSE);
	  break;

	case 0x20:
	  if (!HLPFILE_AddParagraph(hlpfile, buf, end)) return(FALSE);
	  break;

	case 0x23:
	  if (!HLPFILE_AddParagraph(hlpfile, buf, end)) return(FALSE);
	  break;

	default:
	  fprintf(stderr, "buf[0x14] = %x\n", buf[0x14]);
	}

      next   = GET_UINT(buf, 0xc);
      if (next == 0xffffffff) break;

      index  = next >> 14;
      offset = next & 0x3fff;
      if (index > topic.wMapLen) {Report("maplen"); break;}
      buf = topic.map[index] + offset;
    }

  return(HLPFILE_GetContext(hlpfile));
}

/***********************************************************************
 *
 *           HLPFILE_AddPage
 */

static BOOL HLPFILE_AddPage(HLPFILE *hlpfile, BYTE *buf, BYTE *end)
{
  HGLOBAL   hPage;
  HLPFILE_PAGE *page, **pageptr;
  BYTE      *title;
  UINT      titlesize;

  for (pageptr = &hlpfile->first_page; *pageptr; pageptr = &(*pageptr)->next)
    /* Nothing */; 

  if (buf + 0x31 > end) {Report("page1"); return(FALSE);};
  title = buf + GET_UINT(buf, 0x10);
  if (title > end) {Report("page2"); return(FALSE);};

  titlesize = HLPFILE_Uncompressed2_Size(title, end);
  hPage = GlobalAlloc(GMEM_FIXED, sizeof(HLPFILE_PAGE) + titlesize);
  if (!hPage) return FALSE;
  page = *pageptr = GlobalLock(hPage);
  pageptr               = &page->next;
  page->hSelf           = hPage;
  page->file            = hlpfile;
  page->next            = 0;
  page->first_paragraph = 0;

  page->lpszTitle = GlobalLock(hPage);
  page->lpszTitle += sizeof(HLPFILE_PAGE);
  HLPFILE_Uncompress2(&title, end, page->lpszTitle);

  page->wNumber = GET_UINT(buf, 0x21);

  attributes.bDebug        = 0;
  attributes.wFont         = 0;
  attributes.wVSpace       = 0;
  attributes.wVBackSpace   = 0;
  attributes.wHSpace       = 0;
  attributes.wIndent       = 0;
  attributes.link.lpszPath = 0;

  return TRUE;
}

/***********************************************************************
 *
 *           HLPFILE_AddParagraph
 */

static BOOL HLPFILE_AddParagraph(HLPFILE *hlpfile, BYTE *buf, BYTE *end)
{
  HGLOBAL            hParagraph;
  HLPFILE_PAGE      *page;
  HLPFILE_PARAGRAPH *paragraph, **paragraphptr;
  UINT               textsize;
  BYTE              *format, *text;
  BOOL               format_header = TRUE;
  BOOL               format_end = FALSE;
  UINT               mask, i;

  if (!hlpfile->first_page) {Report("paragraph1"); return(FALSE);};

  for (page = hlpfile->first_page; page->next; page = page->next) /* Nothing */;
  for (paragraphptr = &page->first_paragraph; *paragraphptr;
       paragraphptr = &(*paragraphptr)->next) /* Nothing */; 

  if (buf + 0x19 > end) {Report("paragraph2"); return(FALSE);};

  if (buf[0x14] == 0x02) return TRUE;

  text   = buf + GET_UINT(buf, 0x10);

  switch (buf[0x14])
    {
    case 0x20:
      format = buf + 0x18;
      while (*format) format++;
      format += 4;
      break;

    case 0x23:
      format = buf + 0x2b;
      if (buf[0x17] & 1) format++;
      break;

    default:
      Report("paragraph3");
      return FALSE;
    }

  while (text < end)
    {
      if (format_header)
	{
	  format_header = FALSE;

	  mask = GET_USHORT(format, 0);
	  mask &= 0x3ff;
	  format += 2;

	  for (i = 0; i < 10; i++, mask = mask >> 1)
	    {
	      if (mask & 1)
		{
		  BOOL twoargs = FALSE;
		  CHAR prefix0 = ' ';
		  CHAR prefix1 = '*';

		  if (i == 9 && !twoargs)
		    {
		      switch (*format++)
			{
			default:
			  prefix0 = prefix1 = '?';
			  break;

			case 0x82:
			  prefix0 = prefix1 = 'x';
			  break;

			case 0x84:
			  prefix0 = prefix1 = 'X';
			  twoargs = TRUE;
			}
		    }

		  if (*format & 1)
		    switch(*format)
		      {
		      default:
			format += 2;
			break;
		      }
		  else
		    switch(*format)
		      {

		      default:
			format++;
			break;

		      case 0x08:
			format += 3;
			break;
		      }

		  if (twoargs) format += (*format & 1) ? 2 : 1;
		}
	    }
	}

      for (; !format_header && text < end && format < end && !*text; text++)
	{
	  switch(*format)
	    {
	    case 0x80:
	      attributes.wFont = GET_USHORT(format, 1);
	      format += 3;
	      break;

	    case 0x81:
	      attributes.wVSpace++;
	      format += 1;
	      break;

	    case 0x82:
	      attributes.wVSpace += 2 - attributes.wVBackSpace;
	      attributes.wVBackSpace = 0;
	      attributes.wIndent = 0;
	      format += 1;
	      break;

	    case 0x83:
	      attributes.wIndent++;
	      format += 1;
	      break;

	    case 0x84:
	      format += 3;
	      break;

	    case 0x86:
	    case 0x87:
	    case 0x88:
	      format += 9;
	      break;

	    case 0x89:
	      attributes.wVBackSpace++;
	      format += 1;
	      break;

	    case 0xa9:
	      format += 2;
	      break;

	    case 0xe2:
	    case 0xe3:
	      attributes.link.lpszPath = hlpfile->lpszPath;
	      attributes.link.lHash    = GET_UINT(format, 1);
	      attributes.link.bPopup   = !(*format & 1);
	      format += 5;
	      break;

	    case 0xea:
	      attributes.link.lpszPath = format + 8;
	      attributes.link.lHash    = GET_UINT(format, 4);
	      attributes.link.bPopup   = !(*format & 1);
	      format += 3 + GET_USHORT(format, 1);
	      break;

	    case 0xff:
	      if (buf[0x14] != 0x23 || GET_USHORT(format, 1) == 0xffff)
		{
		  if (format_end) Report("format_end");
		  format_end = TRUE;
		  break;
		}
	      else
		{
		  format_header = TRUE;
		  format += 10;
		  break;
		}

	    default:
	      Report("format");
	      format++;
	    }
	}

      if (text > end || format > end) {Report("paragraph_end"); return(FALSE);};
      if (text == end && !format_end) Report("text_end");

      if (text == end) break;

      textsize = HLPFILE_Uncompressed2_Size(text, end);
      hParagraph = GlobalAlloc(GMEM_FIXED, sizeof(HLPFILE_PARAGRAPH) + textsize);
      if (!hParagraph) return FALSE;
      paragraph = *paragraphptr = GlobalLock(hParagraph);
      paragraphptr = &paragraph->next;
      paragraph->hSelf    = hParagraph;
      paragraph->next     = 0;
      paragraph->link     = 0;

      paragraph->lpszText = GlobalLock(hParagraph);
      paragraph->lpszText += sizeof(HLPFILE_PARAGRAPH);
      HLPFILE_Uncompress2(&text, end, paragraph->lpszText);

      paragraph->bDebug      = attributes.bDebug;
      paragraph->wFont       = attributes.wFont;
      paragraph->wVSpace     = attributes.wVSpace;
      paragraph->wHSpace     = attributes.wHSpace;
      paragraph->wIndent     = attributes.wIndent;
      if (attributes.link.lpszPath)
	{
	  LPSTR ptr;
	  HGLOBAL handle = GlobalAlloc(GMEM_FIXED, sizeof(HLPFILE_LINK) +
					   strlen(attributes.link.lpszPath) + 1);
	  if (!handle) return FALSE;
	  paragraph->link = GlobalLock(handle);
	  paragraph->link->hSelf = handle;

	  ptr = GlobalLock(handle);
	  ptr += sizeof(HLPFILE_LINK);
	  lstrcpy(ptr, (LPSTR) attributes.link.lpszPath);

	  paragraph->link->lpszPath = ptr;
	  paragraph->link->lHash    = attributes.link.lHash;
	  paragraph->link->bPopup   = attributes.link.bPopup;
	}

      attributes.bDebug        = 0;
      attributes.wVSpace       = 0;
      attributes.wHSpace       = 0;
      attributes.link.lpszPath = 0;
    }

  return TRUE;
}

/***********************************************************************
 *
 *           HLPFILE_ReadFileToBuffer
 */

static BOOL HLPFILE_ReadFileToBuffer(HFILE hFile)
{
  BYTE  header[16], dummy[1];
  UINT  size;

  if (_hread(hFile, header, 16) != 16) {Report("header"); return(FALSE);};

  size = GET_UINT(header, 12);
  hFileBuffer = GlobalAlloc(GMEM_FIXED, size + 1);
  if (!hFileBuffer) return FALSE;
  file_buffer = GlobalLock(hFileBuffer);

  memcpy(file_buffer, header, 16);
  if (_hread(hFile, file_buffer + 16, size - 16) != size - 16)
    {Report("filesize1"); return(FALSE);};

  if (_hread(hFile, dummy, 1) != 0) Report("filesize2"); 

  file_buffer[size] = '0';

  return TRUE;
}

/***********************************************************************
 *
 *           HLPFILE_FindSubFile
 */

static BOOL HLPFILE_FindSubFile(LPCSTR name, BYTE **subbuf, BYTE **subend)
{
  BYTE *root = file_buffer + GET_UINT(file_buffer,  4);
  BYTE *end  = file_buffer + GET_UINT(file_buffer, 12);
  BYTE *ptr  = root + 0x37;

  while (ptr < end && ptr[0] == 0x7c)
    {
      BYTE *fname = ptr + 1;
      ptr += strlen(ptr) + 1;
      if (!lstrcmpi(fname, name))
	{
	  *subbuf = file_buffer + GET_UINT(ptr, 0);
	  *subend = *subbuf + GET_UINT(*subbuf, 0);
	  if (file_buffer > *subbuf || *subbuf > *subend || *subend >= end)
	    {
	      Report("subfile");
	      return FALSE;
	    }
	  return TRUE;
	}
      else ptr += 4;
    }
  return FALSE;
}

/***********************************************************************
 *
 *           HLPFILE_SystemCommands
 */
static VOID HLPFILE_SystemCommands(HLPFILE* hlpfile)
{
  BYTE *buf, *ptr, *end;
  HGLOBAL handle;
  HLPFILE_MACRO *macro, **m;
  LPSTR p;

  hlpfile->lpszTitle = "";

  if (!HLPFILE_FindSubFile("SYSTEM", &buf, &end)) return;

  for (ptr = buf + 0x15; ptr + 4 <= end; ptr += GET_USHORT(ptr, 2) + 4)
    {
      switch (GET_USHORT(ptr, 0))
	{
	case 1:
	  if (hlpfile->hTitle) {Report("title"); break;}
	  hlpfile->hTitle = GlobalAlloc(GMEM_FIXED, strlen(ptr + 4) + 1);
	  if (!hlpfile->hTitle) return;
	  hlpfile->lpszTitle = GlobalLock(hlpfile->hTitle);
	  lstrcpy(hlpfile->lpszTitle, ptr + 4);
	  break;

	case 2:
	  if (GET_USHORT(ptr, 2) != 1 || ptr[4] != 0) Report("system2");
	  break;

	case 3:
	  if (GET_USHORT(ptr, 2) != 4 || GET_UINT(ptr, 4) != 0) Report("system3");
	  break;

	case 4:
	  handle = GlobalAlloc(GMEM_FIXED, sizeof(HLPFILE_MACRO) + lstrlen(ptr + 4) + 1);
	  if (!handle) break;
	  macro = GlobalLock(handle);
	  macro->hSelf = handle;
	  p  = GlobalLock(handle);
	  p += sizeof(HLPFILE_MACRO);
	  lstrcpy(p, (LPSTR) ptr + 4);
	  macro->lpszMacro = p;
	  macro->next = 0;
	  for (m = &hlpfile->first_macro; *m; m = &(*m)->next);
	  *m = macro;
	  break;

	default:
	  Report("system");
	}
    }
}

/***********************************************************************
 *
 *           HLPFILE_Uncompressed1_Size
 */

static INT HLPFILE_Uncompressed1_Size(BYTE *ptr, BYTE *end)
{
  INT  i, newsize = 0;

  while (ptr < end)
    {
      INT mask=*ptr++;
      for (i = 0; i < 8 && ptr < end; i++, mask = mask >> 1)
	{
	  if (mask & 1)
	    {
	      INT code = GET_USHORT(ptr, 0);
	      INT len  = 3 + (code >> 12);
	      newsize += len;
	      ptr     += 2;
	    }
	  else newsize++, ptr++;
	}
    }

  return(newsize);
}

/***********************************************************************
 *
 *           HLPFILE_Uncompress1
 */

static BYTE *HLPFILE_Uncompress1(BYTE *ptr, BYTE *end, BYTE *newptr)
{
  INT i;

  while (ptr < end)
    {
      INT mask=*ptr++;
      for (i = 0; i < 8 && ptr < end; i++, mask = mask >> 1)
	{
	  if (mask & 1)
	    {
	      INT code   = GET_USHORT(ptr, 0);
	      INT len    = 3 + (code >> 12);
	      INT offset = code & 0xfff;
	      hmemcpy16(newptr, newptr - offset - 1, len);
	      newptr += len;
	      ptr    += 2;
	    }
	  else *newptr++ = *ptr++;
	}
    }

  return(newptr);
}

/***********************************************************************
 *
 *           HLPFILE_Uncompress1_Phrases
 */

static BOOL HLPFILE_Uncompress1_Phrases()
{
  UINT i, num, newsize;
  BYTE *buf, *end, *newbuf;

  if (!HLPFILE_FindSubFile("Phrases", &buf, &end)) {Report("phrases0"); return FALSE;}

  num = phrases.num = GET_USHORT(buf, 9);
  if (buf + 2 * num + 0x13 >= end) {Report("uncompress1a"); return(FALSE);};

  newsize  = 2 * num + 2;
  newsize += HLPFILE_Uncompressed1_Size(buf + 0x13 + 2 * num, end);
  phrases.hBuffer = GlobalAlloc(GMEM_FIXED, newsize);
  if (!phrases.hBuffer) return FALSE;
  newbuf = phrases.buf = GlobalLock(phrases.hBuffer);

  hmemcpy16(newbuf, buf + 0x11, 2 * num + 2);
  HLPFILE_Uncompress1(buf + 0x13 + 2 * num, end, newbuf + 2 * num + 2);

  for (i = 0; i < num; i++)
    {
      INT i0 = GET_USHORT(newbuf, 2 * i);
      INT i1 = GET_USHORT(newbuf, 2 * i + 2);
      if (i1 < i0 || i1 > newsize) {Report("uncompress1b"); return(FALSE);};
    }
  return TRUE;
}

/***********************************************************************
 *
 *           HLPFILE_Uncompress1_Topic
 */

static BOOL HLPFILE_Uncompress1_Topic()
{
  BYTE *buf, *ptr, *end, *newptr;
  INT  i, newsize = 0;

  if (!HLPFILE_FindSubFile("TOPIC", &buf, &end)) {Report("topic0"); return FALSE;}

  buf += 9;
  topic.wMapLen = (end - buf - 1) / 0x1000 + 1;

  for (i = 0; i < topic.wMapLen; i++)
    {
      ptr = buf + i * 0x1000;

      /* I don't know why, it's necessary for printman.hlp */
      if (ptr + 0x44 > end) ptr = end - 0x44;

      newsize += HLPFILE_Uncompressed1_Size(ptr + 0xc, MIN(end, ptr + 0x1000));
    }

  topic.hMap    = GlobalAlloc(GMEM_FIXED, topic.wMapLen * sizeof(topic.map[0]));
  topic.hBuffer = GlobalAlloc(GMEM_FIXED, newsize);
  if (!topic.hMap || !topic.hBuffer) return FALSE;
  topic.map = GlobalLock(topic.hMap);
  newptr = GlobalLock(topic.hBuffer);
  topic.end = newptr + newsize;

  for (i = 0; i < topic.wMapLen; i++)
    {
      ptr = buf + i * 0x1000;
      if (ptr + 0x44 > end) ptr = end - 0x44;

      topic.map[i] = newptr - 0xc;
      newptr = HLPFILE_Uncompress1(ptr + 0xc, MIN(end, ptr + 0x1000), newptr);
    }

  return TRUE;
}

/***********************************************************************
 *
 *           HLPFILE_Uncompressed2_Size
 */

static UINT HLPFILE_Uncompressed2_Size(BYTE *ptr, BYTE *end)
{
  UINT wSize   = 0;

  while (ptr < end && *ptr)
    {
      if (*ptr >= 0x20)
	wSize++, ptr++;
      else
	{
	  BYTE *phptr, *phend;
	  UINT code  = 0x100 * ptr[0] + ptr[1];
	  UINT index = (code - 0x100) / 2;
	  BOOL space = code & 1;

	  if (index < phrases.num)
	    {
	      phptr = phrases.buf + GET_USHORT(phrases.buf, 2 * index);
	      phend = phrases.buf + GET_USHORT(phrases.buf, 2 * index + 2);

	      if (phend < phptr) Report("uncompress2a");

	      wSize += phend - phptr;
	      if (space) wSize++;
	    }
	  else Report("uncompress2b");

	  ptr += 2;
	}
    }

  return(wSize + 1);
}

/***********************************************************************
 *
 *           HLPFILE_Uncompress2
 */

static VOID HLPFILE_Uncompress2(BYTE **pptr, BYTE *end, BYTE *newptr)
{
  BYTE *ptr    = *pptr;

  while (ptr < end && *ptr)
    {
      if (*ptr >= 0x20)
	*newptr++ = *ptr++;
      else
	{
	  BYTE *phptr, *phend;
	  UINT code  = 0x100 * ptr[0] + ptr[1];
	  UINT index = (code - 0x100) / 2;
	  BOOL space = code & 1;

	  phptr = phrases.buf + GET_USHORT(phrases.buf, 2 * index);
	  phend = phrases.buf + GET_USHORT(phrases.buf, 2 * index + 2);

	  hmemcpy16(newptr, phptr, phend - phptr);
	  newptr += phend - phptr;
	  if (space) *newptr++ = ' ';

	  ptr += 2;
	}
    }
  *newptr  = '\0';
  *pptr    = ptr;
}

/***********************************************************************
 *
 *           HLPFILE_GetContext
 */

static BOOL HLPFILE_GetContext(HLPFILE *hlpfile)
{
  UINT i, j, clen, tlen;
  BYTE *cbuf, *cptr, *cend, *tbuf, *tptr, *tend;

  if (!HLPFILE_FindSubFile("CONTEXT", &cbuf, &cend)) {Report("context0"); return FALSE;}
  if (cbuf + 0x37 > cend) {Report("context1"); return(FALSE);};
  clen = GET_UINT(cbuf, 0x2b);
  if (cbuf + 0x37 + 8 * hlpfile->wContextLen > cend) {Report("context2"); return(FALSE);};

  if (!HLPFILE_FindSubFile("TTLBTREE", &tbuf, &tend)) {Report("ttlb0"); return FALSE;}
  if (tbuf + 0x37 > tend) {Report("ttlb1"); return(FALSE);};
  tlen = GET_UINT(tbuf, 0x2b);

  hlpfile->hContext = GlobalAlloc(GMEM_FIXED, clen * sizeof(HLPFILE_CONTEXT));
  if (!hlpfile->hContext) return FALSE;
  hlpfile->Context = GlobalLock(hlpfile->hContext);
  hlpfile->wContextLen = clen;

  cptr = cbuf + 0x37;
  for (i = 0; i < clen; i++, cptr += 8)
    {
      tptr = tbuf + 0x37;
      for (j = 0; j < tlen; j++, tptr += 5 + strlen(tptr + 4))
	{
	  if (tptr + 4 >= tend) {Report("ttlb2"); return(FALSE);};
	  if (GET_UINT(tptr, 0) == GET_UINT(cptr, 4)) break;
	}
      if (j >= tlen)
	{
	  Report("ttlb3");
	  j = 0;
	}
      hlpfile->Context[i].lHash = GET_UINT(cptr, 0);
      hlpfile->Context[i].wPage = j;
    }

  return TRUE;
}

/***********************************************************************
 *
 *           HLPFILE_DeleteParagraph
 */

static VOID HLPFILE_DeleteParagraph(HLPFILE_PARAGRAPH* paragraph)
{
  if (!paragraph) return;

  if (paragraph->link) GlobalFree(paragraph->link->hSelf);

  HLPFILE_DeleteParagraph(paragraph->next);
  GlobalFree(paragraph->hSelf);
}

/***********************************************************************
 *
 *           DeletePage
 */

static VOID HLPFILE_DeletePage(HLPFILE_PAGE* page)
{
  if (!page) return;

  HLPFILE_DeletePage(page->next);
  HLPFILE_DeleteParagraph(page->first_paragraph);
  GlobalFree(page->hSelf);
}

/***********************************************************************
 *
 *           DeleteMacro
 */

static VOID HLPFILE_DeleteMacro(HLPFILE_MACRO* macro)
{
  if (!macro) return;

  HLPFILE_DeleteMacro(macro->next);
  GlobalFree(macro->hSelf);
}

/***********************************************************************
 *
 *           HLPFILE_FreeHlpFile
 */

VOID HLPFILE_FreeHlpFile(HLPFILE* hlpfile)
{
  if (!hlpfile) return;
  if (--hlpfile->wRefCount) return;

  if (hlpfile->next) hlpfile->next->prev = hlpfile->prev;
  if (hlpfile->prev) hlpfile->prev->next = hlpfile->next;
  else first_hlpfile = 0;

  HLPFILE_DeletePage(hlpfile->first_page);
  HLPFILE_DeleteMacro(hlpfile->first_macro);
  if (hlpfile->hContext) GlobalFree(hlpfile->hContext);
  if (hlpfile->hTitle) GlobalFree(hlpfile->hTitle);
  GlobalFree(hlpfile->hSelf);
}

/***********************************************************************
 *
 *           FreeHlpFilePage
 */

VOID HLPFILE_FreeHlpFilePage(HLPFILE_PAGE* page)
{
  if (!page) return;
  HLPFILE_FreeHlpFile(page->file);
}

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
