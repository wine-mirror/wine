/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
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
