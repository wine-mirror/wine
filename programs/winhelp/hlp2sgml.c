/*
 * Copyright 1996 Ulrich Schmid
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include "windows.h"
#include "hlpfile.h"

typedef struct
{
  const char *header1;
  const char *header2;
  const char *section;
  const char *first_paragraph;
  const char *newline;
  const char *next_paragraph;
  const char *special_char;
  const char *begin_italic;
  const char *end_italic;
  const char *begin_boldface;
  const char *end_boldface;
  const char *begin_typewriter;
  const char *end_typewriter;
  const char *tail;
} FORMAT;

typedef struct
{
  const char ch;
  const char *subst;
} CHARMAP[];


FORMAT format =
{
  "<!doctype linuxdoc system>\n"
  "<article>\n"
  "<title>\n",

  "\n<author>\n%s\n"
  "<date>\n%s\n",

  "\n<sect>\n",
  "\n<p>\n",
  "\n<newline>\n",
  "\n\n",

  "&%s;",

  "<em>",
  "</em>",
  "<bf>",
  "</bf>",
  "<tt>",
  "</tt>",

  "\n</article>\n"
};

CHARMAP charmap =
  {{'Æ', "AElig"},
   {'Á', "Aacute"},
   {'Â', "Acirc"},
   {'À', "Agrave"},
   {'Ã', "Atilde"},
   {'Ç', "Ccedil"},
   {'É', "Eacute"},
   {'È', "Egrave"},
   {'Ë', "Euml"},
   {'Í', "Iacute"},
   {'Î', "Icirc"},
   {'Ì', "Igrave"},
   {'Ï', "Iuml"},
   {'Ñ', "Ntilde"},
   {'Ó', "Oacute"},
   {'Ô', "Ocirc"},
   {'Ò', "Ograve"},
   {'Ø', "Oslash"},
   {'Ú', "Uacute"},
   {'Ù', "Ugrave"},
   {'Ý', "Yacute"},
   {'á', "aacute"},
   {'â', "acirc"},
   {'æ', "aelig"},
   {'à', "agrave"},
   {'å', "aring"},
   {'ã', "atilde"},
   {'ç', "ccedil"},
   {'é', "eacute"},
   {'ê', "ecirc"},
   {'è', "egrave"},
   {'ë', "euml"},
   {'í', "iacute"},
   {'î', "icirc"},
   {'ì', "igrave"},
   {'ï', "iuml"},
   {'ñ', "ntilde"},
   {'ó', "oacute"},
   {'ÿ', "yuml"},
   {'ô', "ocirc"},
   {'ò', "ograve"},
   {'ø', "oslash"},
   {'õ', "otilde"},
   {'ú', "uacute"},
   {'û', "ucirc"},
   {'ù', "ugrave"},
   {'ý', "yacute"},
   {'<', "lt"},
   {'&', "amp"},
   {'"', "dquot"},
   {'#', "num"},
   {'%', "percnt"},
   {'\'', "quot"},
#if 0
   {'(', "lpar"},
   {')', "rpar"},
   {'*', "ast"},
   {'+', "plus"},
   {',', "comma"},
   {'-', "hyphen"},
   {':', "colon"},
   {';', "semi"},
   {'=', "equals"},
   {'@', "commat"},
   {'[', "lsqb"},
   {']', "rsqb"},
   {'^', "circ"},
   {'_', "lowbar"},
   {'{', "lcub"},
   {'|', "verbar"},
   {'}', "rcub"},
   {'~', "tilde"},
#endif
   {'\\', "bsol"},
   {'$', "dollar"},
   {'Ä', "Auml"},
   {'ä', "auml"},
   {'Ö', "Ouml"},
   {'ö', "ouml"},
   {'Ü', "Uuml"},
   {'ü', "uuml"},
   {'ß', "szlig"},
   {'>', "gt"},
   {'§', "sect"},
   {'¶', "para"},
   {'©', "copy"},
   {'¡', "iexcl"},
   {'¿', "iquest"},
   {'¢', "cent"},
   {'£', "pound"},
   {'×', "times"},
   {'±', "plusmn"},
   {'÷', "divide"},
   {'¬', "not"},
   {'µ', "mu"},
   {0,0}};

/***********************************************************************
 *
 *           print_text
 */

static void print_text(const char *p)
{
  int i;

  for (; *p; p++)
    {
      for (i = 0; charmap[i].ch; i++)
	if (*p == charmap[i].ch)
	  {
	    printf(format.special_char, charmap[i].subst);
	    break;
	  }
      if (!charmap[i].ch)
	printf("%c", *p);
    }
}

/***********************************************************************
 *
 *           main
 */

int main(int argc, char **argv)
{
  HLPFILE   *hlpfile;
  HLPFILE_PAGE *page;
  HLPFILE_PARAGRAPH *paragraph;
  time_t t;
  char date[50];
  char *filename;

  hlpfile = HLPFILE_ReadHlpFile(argc > 1 ? argv[1] : "");

  if (!hlpfile) return(2);

  time(&t);
  strftime(date, sizeof(date), "%x", localtime(&t));
  filename = strrchr(hlpfile->lpszPath, '/');
  if (filename) filename++;
  else filename = hlpfile->lpszPath;

  /* Header */
  printf(format.header1);
  print_text(hlpfile->lpszTitle);
  printf(format.header2, filename, date);

  for (page = hlpfile->first_page; page; page = page->next)
    {
      paragraph = page->first_paragraph;
      if (!paragraph) continue;

      /* Section */
      printf(format.section);
      for (; paragraph && !paragraph->wVSpace; paragraph = paragraph->next)
	print_text(paragraph->lpszText);
      printf(format.first_paragraph);

      for (; paragraph; paragraph = paragraph->next)
	{
	  /* New line; new paragraph */
	  if (paragraph->wVSpace == 1)
	    printf(format.newline);
	  else if (paragraph->wVSpace > 1)
	    printf(format.next_paragraph);

	  if (paragraph->wFont)
	    printf(format.begin_boldface);

	  print_text(paragraph->lpszText);

	  if (paragraph->wFont)
	    printf(format.end_boldface);
	}
    }

  printf(format.tail);

  return(0);
}

/***********************************************************************
 *
 *           Substitutions for some WINELIB functions
 */

HFILE OpenFile( LPCSTR path, OFSTRUCT *ofs, UINT mode )
{
  FILE *file;

  if (!*path) return (HFILE) stdin;

  file = fopen(path, "r");
  if (!file) return HFILE_ERROR;
  return (HFILE) file;
}

HFILE _lclose( HFILE hFile )
{
  fclose((FILE*) hFile);
  return 0;
}

LONG _hread( HFILE hFile, SEGPTR buffer, LONG count )
{
  return fread(buffer, 1, count, (FILE*) hFile);
}

HGLOBAL GlobalAlloc( WORD flags, DWORD size )
{
  return (HGLOBAL) malloc(size);
}

LPVOID GlobalLock( HGLOBAL handle )
{
  return (LPVOID) handle;
}

HGLOBAL GlobalFree( HGLOBAL handle )
{
  free((VOID*) handle);
  return(0);
}

/*
 * String functions
 *
 * Copyright 1993 Yngvi Sigurjonsson (yngvi@hafro.is)
 */

INT lstrcmp(LPCSTR str1,LPCSTR str2)
{
  return strcmp( str1, str2 );
}

INT lstrcmpi( LPCSTR str1, LPCSTR str2 )
{
  INT res;

  while (*str1)
    {
      if ((res = toupper(*str1) - toupper(*str2)) != 0) return res;
      str1++;
      str2++;
    }
  return toupper(*str1) - toupper(*str2);
}

INT lstrlen(LPCSTR str)
{
  return strlen(str);
}

SEGPTR lstrcpy( SEGPTR target, SEGPTR source )
{
  strcpy( (char *)target, (char *)source );
  return target;
}

void hmemcpy(LPVOID hpvDest, LPCVOID hpvSource, LONG cbCopy)
{
  memcpy(hpvDest, hpvSource, cbCopy);
}

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
