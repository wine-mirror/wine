/*
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <assert.h>
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
} CHARMAP_ENTRY;


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

CHARMAP_ENTRY charmap[] =
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

    if (!hlpfile) return 2;

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
        for (; paragraph && !paragraph->u.text.wVSpace; paragraph = paragraph->next)
            print_text(paragraph->u.text.lpszText);
        printf(format.first_paragraph);

        for (; paragraph; paragraph = paragraph->next)
	{
            switch (paragraph->cookie)
            {
            case para_normal_text:
            case para_debug_text:
                /* New line; new paragraph */
                if (paragraph->u.text.wVSpace == 1)
                    printf(format.newline);
                else if (paragraph->u.text.wVSpace > 1)
                    printf(format.next_paragraph);

                if (paragraph->u.text.wFont)
                    printf(format.begin_boldface);

                print_text(paragraph->u.text.lpszText);

                if (paragraph->u.text.wFont)
                    printf(format.end_boldface);
                break;
            case para_bitmap:
            case para_metafile:
                break;
            }
	}
    }

    printf(format.tail);

    return 0;
}

/***********************************************************************
 *
 *           Substitutions for some WINELIB functions
 */

static FILE *file = 0;

HFILE WINAPI OpenFile( LPCSTR path, OFSTRUCT *ofs, UINT mode )
{
    file = *path ? fopen(path, "r") : stdin;
    return file ? (HFILE)1 : HFILE_ERROR;
}

HFILE WINAPI _lclose( HFILE hFile )
{
    fclose(file);
    return 0;
}

LONG WINAPI _hread( HFILE hFile, LPVOID buffer, LONG count )
{
    return fread(buffer, 1, count, file);
}

HANDLE WINAPI GetProcessHeap(void)
{
    return 0;
}

void* WINAPI HeapAlloc( HANDLE heap, DWORD flags, DWORD size )
{
    assert(flags == 0);
    return malloc(size);
}

void* WINAPI HeapReAlloc( HANDLE heap, DWORD flags, void* ptr, DWORD size)
{
    assert(flags == 0);
    return realloc(ptr, size);
}

BOOL WINAPI HeapFree( HGLOBAL handle, DWORD flags, void* ptr )
{
    free(ptr);
    return TRUE;
}

char __wine_dbch_winhelp[] = "\003winhelp";

int wine_dbg_log( int cls, const char *channel, const char *func, const char *format, ... )
{
    return 1;
}

HBITMAP WINAPI CreateDIBitmap(HDC hdc, CONST BITMAPINFOHEADER* bih, DWORD a, CONST void* ptr, CONST BITMAPINFO* bi, UINT c)
{
    return 0;
}

HMETAFILE WINAPI SetMetaFileBitsEx(UINT cbBuffer, CONST BYTE *lpbBuffer)
{
    return 0;
}

BOOL WINAPI DeleteMetaFile(HMETAFILE h)
{
    return 0;
}

HDC WINAPI GetDC(HWND h)
{
    return 0;
}

int WINAPI ReleaseDC(HWND h, HDC hdc)
{
    return 0;
}

BOOL WINAPI DeleteObject(HGDIOBJ h)
{
    return TRUE;
}
/*
 * String functions
 *
 * Copyright 1993 Yngvi Sigurjonsson (yngvi@hafro.is)
 */

INT WINAPI lstrcmp( LPCSTR str1, LPCSTR str2 )
{
    return strcmp( str1, str2 );
}

INT WINAPI lstrcmpi( LPCSTR str1, LPCSTR str2 )
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

INT WINAPI lstrlen( LPCSTR str )
{
    return strlen(str);
}

LPSTR WINAPI lstrcpyA( LPSTR dst, LPCSTR src )
{
    if (!src || !dst) return NULL;
    strcpy( dst, src );
    return dst;
}
