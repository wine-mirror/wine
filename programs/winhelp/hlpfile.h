/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

struct tagHelpFile;

typedef struct
{
  LPCSTR  lpszPath;
  LONG    lHash;
  BOOL    bPopup;

  HGLOBAL hSelf;
} HLPFILE_LINK;

typedef struct tagHlpFileParagraph
{
  LPSTR  lpszText;

  UINT   bDebug;
  UINT   wFont;
  UINT   wIndent;
  UINT   wHSpace;
  UINT   wVSpace;

  HLPFILE_LINK *link;

  struct tagHlpFileParagraph *next;

  HGLOBAL hSelf;
} HLPFILE_PARAGRAPH;

typedef struct tagHlpFilePage
{
  LPSTR          lpszTitle;
  HLPFILE_PARAGRAPH *first_paragraph;

  UINT wNumber;

  struct tagHlpFilePage *next;
  struct tagHlpFileFile *file;

  HGLOBAL hSelf;
} HLPFILE_PAGE;

typedef struct
{
  LONG lHash;
  UINT wPage;
} HLPFILE_CONTEXT;

typedef struct tagHlpFileMacro
{
  LPCSTR lpszMacro;

  HGLOBAL hSelf;
  struct tagHlpFileMacro *next;
} HLPFILE_MACRO;

typedef struct tagHlpFileFile
{
  LPSTR        lpszPath;
  LPSTR        lpszTitle;
  HLPFILE_PAGE    *first_page;
  HLPFILE_MACRO   *first_macro;
  UINT         wContextLen;
  HLPFILE_CONTEXT *Context;

  struct tagHlpFileFile *prev;
  struct tagHlpFileFile *next;

  UINT       wRefCount;

  HGLOBAL    hTitle;
  HGLOBAL    hContext;
  HGLOBAL    hSelf;
} HLPFILE;

HLPFILE      *HLPFILE_ReadHlpFile(LPCSTR lpszPath);
HLPFILE_PAGE *HLPFILE_Contents(LPCSTR lpszPath);
HLPFILE_PAGE *HLPFILE_PageByHash(LPCSTR lpszPath, LONG wNum);
LONG          HLPFILE_Hash(LPCSTR lpszContext);
VOID          HLPFILE_FreeHlpFilePage(HLPFILE_PAGE*);
VOID          HLPFILE_FreeHlpFile(HLPFILE*);

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
