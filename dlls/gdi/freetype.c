/*
 * FreeType font engine interface
 *
 * Copyright 2001 Huw D M Davies for CodeWeavers.
 *
 * This file contains the WineEng* functions.
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


#include "config.h"

#include "windef.h"
#include "winerror.h"
#include "winreg.h"
#include "wingdi.h"
#include "wine/unicode.h"
#include "wine/port.h"
#include "wine/debug.h"
#include "gdi.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(font);

#ifdef HAVE_FREETYPE

#ifdef HAVE_FREETYPE_FREETYPE_H
#include <freetype/freetype.h>
#endif
#ifdef HAVE_FREETYPE_FTGLYPH_H
#include <freetype/ftglyph.h>
#endif
#ifdef HAVE_FREETYPE_TTTABLES_H
#include <freetype/tttables.h>
#endif
#ifdef HAVE_FREETYPE_FTSNAMES_H
#include <freetype/ftsnames.h>
#else
# ifdef HAVE_FREETYPE_FTNAMES_H
# include <freetype/ftnames.h>
# endif
#endif
#ifdef HAVE_FREETYPE_TTNAMEID_H
#include <freetype/ttnameid.h>
#endif
#ifdef HAVE_FREETYPE_FTOUTLN_H
#include <freetype/ftoutln.h>
#endif
#ifdef HAVE_FREETYPE_INTERNAL_SFNT_H
#include <freetype/internal/sfnt.h>
#endif
#ifdef HAVE_FREETYPE_FTTRIGON_H
#include <freetype/fttrigon.h>
#endif

#ifndef SONAME_LIBFREETYPE
#define SONAME_LIBFREETYPE "libfreetype.so"
#endif

static FT_Library library = 0;
typedef struct
{
    FT_Int major;
    FT_Int minor;
    FT_Int patch;
} FT_Version_t;
static FT_Version_t FT_Version;

static void *ft_handle = NULL;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f = NULL
MAKE_FUNCPTR(FT_Cos);
MAKE_FUNCPTR(FT_Done_Face);
MAKE_FUNCPTR(FT_Get_Char_Index);
MAKE_FUNCPTR(FT_Get_Sfnt_Table);
MAKE_FUNCPTR(FT_Init_FreeType);
MAKE_FUNCPTR(FT_Load_Glyph);
MAKE_FUNCPTR(FT_MulFix);
MAKE_FUNCPTR(FT_New_Face);
MAKE_FUNCPTR(FT_Outline_Get_Bitmap);
MAKE_FUNCPTR(FT_Outline_Transform);
MAKE_FUNCPTR(FT_Outline_Translate);
MAKE_FUNCPTR(FT_Select_Charmap);
MAKE_FUNCPTR(FT_Set_Pixel_Sizes);
MAKE_FUNCPTR(FT_Sin);
MAKE_FUNCPTR(FT_Vector_Rotate);
#undef MAKE_FUNCPTR
static void (*pFT_Library_Version)(FT_Library,FT_Int*,FT_Int*,FT_Int*);

#define GET_BE_WORD(ptr) MAKEWORD( ((BYTE *)(ptr))[1], ((BYTE *)(ptr))[0] )

typedef struct tagFace {
    WCHAR *StyleName;
    char *file;
    FT_Long face_index;
    BOOL Italic;
    BOOL Bold;
    DWORD fsCsb[2]; /* codepage bitfield from FONTSIGNATURE */
    struct tagFace *next;
} Face;

typedef struct tagFamily {
    WCHAR *FamilyName;
    Face *FirstFace;
    struct tagFamily *next;
} Family;

typedef struct {
    GLYPHMETRICS gm;
    INT adv; /* These three hold to widths of the unrotated chars */
    INT lsb;
    INT bbx;
    BOOL init;
} GM;

struct tagGdiFont {
    FT_Face ft_face;
    XFORM xform;
    LPWSTR name;
    int charset;
    BOOL fake_italic;
    BOOL fake_bold;
    INT orientation;
    GM *gm;
    DWORD gmsize;
    HFONT hfont;
    SHORT yMax;
    SHORT yMin;
    OUTLINETEXTMETRICW *potm;
    struct tagGdiFont *next;
};

#define INIT_GM_SIZE 128

static GdiFont GdiFontList = NULL;

static Family *FontList = NULL;

static WCHAR defSerif[] = {'T','i','m','e','s',' ','N','e','w',' ',
			   'R','o','m','a','n','\0'};
static WCHAR defSans[] = {'A','r','i','a','l','\0'};
static WCHAR defFixed[] = {'C','o','u','r','i','e','r',' ','N','e','w','\0'};

static WCHAR defSystem[] = {'A','r','i','a','l','\0'};
static WCHAR SystemW[] = {'S','y','s','t','e','m','\0'};
static WCHAR MSSansSerifW[] = {'M','S',' ','S','a','n','s',' ',
			       'S','e','r','i','f','\0'};
static WCHAR HelvW[] = {'H','e','l','v','\0'};

static WCHAR ArabicW[] = {'A','r','a','b','i','c','\0'};
static WCHAR BalticW[] = {'B','a','l','t','i','c','\0'};
static WCHAR CHINESE_BIG5W[] = {'C','H','I','N','E','S','E','_','B','I','G','5','\0'};
static WCHAR CHINESE_GB2312W[] = {'C','H','I','N','E','S','E','_','G','B','2','3','1','2','\0'};
static WCHAR Central_EuropeanW[] = {'C','e','n','t','r','a','l',' ',
				    'E','u','r','o','p','e','a','n','\0'};
static WCHAR CyrillicW[] = {'C','y','r','i','l','l','i','c','\0'};
static WCHAR GreekW[] = {'G','r','e','e','k','\0'};
static WCHAR HangulW[] = {'H','a','n','g','u','l','\0'};
static WCHAR Hangul_Johab_W[] = {'H','a','n','g','u','l','(','J','o','h','a','b',')','\0'};
static WCHAR HebrewW[] = {'H','e','b','r','e','w','\0'};
static WCHAR JapaneseW[] = {'J','a','p','a','n','e','s','e','\0'};
static WCHAR SymbolW[] = {'S','y','m','b','o','l','\0'};
static WCHAR ThaiW[] = {'T','h','a','i','\0'};
static WCHAR TurkishW[] = {'T','u','r','k','i','s','h','\0'};
static WCHAR VietnameseW[] = {'V','i','e','t','n','a','m','e','s','e','\0'};
static WCHAR WesternW[] = {'W','e','s','t','e','r','n','\0'};

static WCHAR *ElfScriptsW[32] = { /* these are in the order of the fsCsb[0] bits */
    WesternW, /*00*/
    Central_EuropeanW,
    CyrillicW,
    GreekW,
    TurkishW,
    HebrewW,
    ArabicW,
    BalticW,
    VietnameseW, /*08*/
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, /*15*/
    ThaiW,
    JapaneseW,
    CHINESE_GB2312W,
    HangulW,
    CHINESE_BIG5W,
    Hangul_Johab_W,
    NULL, NULL, /*23*/
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    SymbolW /*31*/
};

typedef struct {
  WCHAR *name;
  INT charset;
} NameCs;

typedef struct tagFontSubst {
  NameCs from;
  NameCs to;
  struct tagFontSubst *next;
} FontSubst;

static FontSubst *substlist = NULL;

static BOOL AddFontFileToList(char *file)
{
    FT_Face ft_face;
    TT_OS2 *pOS2;
    WCHAR *FamilyW, *StyleW;
    DWORD len;
    Family *family = FontList;
    Family **insert = &FontList;
    Face **insertface;
    FT_Error err;
    FT_Long face_index = 0, num_faces;
    int i;

    do {
        TRACE("Loading font file %s index %ld\n", debugstr_a(file), face_index);
	if((err = pFT_New_Face(library, file, face_index, &ft_face)) != 0) {
	    WARN("Unable to load font file %s err = %x\n", debugstr_a(file), err);
	    return FALSE;
	}

	if(!FT_IS_SFNT(ft_face)) { /* for now we'll skip everything but TT/OT */
	    pFT_Done_Face(ft_face);
	    return FALSE;
	}
	if(!pFT_Get_Sfnt_Table(ft_face, ft_sfnt_os2) ||
	   !pFT_Get_Sfnt_Table(ft_face, ft_sfnt_hhea)) {
	    TRACE("Font file %s lacks either an OS2 or HHEA table.\n"
		  "Skipping this font.\n", debugstr_a(file));
	    pFT_Done_Face(ft_face);
	    return FALSE;
	}

	len = MultiByteToWideChar(CP_ACP, 0, ft_face->family_name, -1, NULL, 0);
	FamilyW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
	MultiByteToWideChar(CP_ACP, 0, ft_face->family_name, -1, FamilyW, len);

	while(family) {
	    if(!strcmpW(family->FamilyName, FamilyW))
	        break;
	    insert = &family->next;
	    family = family->next;
	}
	if(!family) {
	    family = *insert = HeapAlloc(GetProcessHeap(), 0, sizeof(*family));
	    family->FamilyName = FamilyW;
	    family->FirstFace = NULL;
	    family->next = NULL;
	} else {
	    HeapFree(GetProcessHeap(), 0, FamilyW);
	}

	len = MultiByteToWideChar(CP_ACP, 0, ft_face->style_name, -1, NULL, 0);
	StyleW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
	MultiByteToWideChar(CP_ACP, 0, ft_face->style_name, -1, StyleW, len);

	for(insertface = &family->FirstFace; *insertface;
	    insertface = &(*insertface)->next) {
	    if(!strcmpW((*insertface)->StyleName, StyleW)) {
	        TRACE("Already loaded font %s %s\n", debugstr_w(family->FamilyName),
		    debugstr_w(StyleW));
		HeapFree(GetProcessHeap(), 0, StyleW);
		pFT_Done_Face(ft_face);
		return FALSE;
	    }
	}
	*insertface = HeapAlloc(GetProcessHeap(), 0, sizeof(**insertface));
	(*insertface)->StyleName = StyleW;
	(*insertface)->file = HeapAlloc(GetProcessHeap(),0,strlen(file)+1);
	strcpy((*insertface)->file, file);
	(*insertface)->face_index = face_index;
	(*insertface)->next = NULL;
	(*insertface)->Italic = (ft_face->style_flags & FT_STYLE_FLAG_ITALIC) ? 1 : 0;
	(*insertface)->Bold = (ft_face->style_flags & FT_STYLE_FLAG_BOLD) ? 1 : 0;

	pOS2 = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_os2);
	if(pOS2) {
	    (*insertface)->fsCsb[0] = pOS2->ulCodePageRange1;
	    (*insertface)->fsCsb[1] = pOS2->ulCodePageRange2;
	} else {
	    (*insertface)->fsCsb[0] = (*insertface)->fsCsb[1] = 0;
	}
	TRACE("fsCsb = %08lx %08lx\n", (*insertface)->fsCsb[0], (*insertface)->fsCsb[1]);

	if((*insertface)->fsCsb[0] == 0) { /* let's see if we can find any interesting cmaps */
	    for(i = 0; i < ft_face->num_charmaps &&
		  !(*insertface)->fsCsb[0]; i++) {
	        switch(ft_face->charmaps[i]->encoding) {
		case ft_encoding_unicode:
		    (*insertface)->fsCsb[0] = 1;
		    break;
		case ft_encoding_symbol:
		    (*insertface)->fsCsb[0] = 1L << 31;
		    break;
		default:
		    break;
		}
	    }
	}
	num_faces = ft_face->num_faces;
	pFT_Done_Face(ft_face);
	TRACE("Added font %s %s\n", debugstr_w(family->FamilyName),
	      debugstr_w(StyleW));
    } while(num_faces > ++face_index);
    return TRUE;
}

static void DumpFontList(void)
{
    Family *family;
    Face *face;

    for(family = FontList; family; family = family->next) {
        TRACE("Family: %s\n", debugstr_w(family->FamilyName));
        for(face = family->FirstFace; face; face = face->next) {
	    TRACE("\t%s\n", debugstr_w(face->StyleName));
	}
    }
    return;
}

static void DumpSubstList(void)
{
    FontSubst *psub;

    for(psub = substlist; psub; psub = psub->next)
        if(psub->from.charset != -1 || psub->to.charset != -1)
	    TRACE("%s:%d -> %s:%d\n", debugstr_w(psub->from.name),
	      psub->from.charset, debugstr_w(psub->to.name), psub->to.charset);
	else
	    TRACE("%s -> %s\n", debugstr_w(psub->from.name),
		  debugstr_w(psub->to.name));
    return;
}

static LPWSTR strdupW(LPWSTR p)
{
    LPWSTR ret;
    DWORD len = (strlenW(p) + 1) * sizeof(WCHAR);
    ret = HeapAlloc(GetProcessHeap(), 0, len);
    memcpy(ret, p, len);
    return ret;
}

static void split_subst_info(NameCs *nc, LPSTR str)
{
    CHAR *p = strrchr(str, ',');
    DWORD len;

    nc->charset = -1;
    if(p && *(p+1)) {
        nc->charset = strtol(p+1, NULL, 10);
	*p = '\0';
    }
    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    nc->name = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, -1, nc->name, len);
}

static void LoadSubstList(void)
{
    FontSubst *psub, **ppsub;
    HKEY hkey;
    DWORD valuelen, datalen, i = 0, type, dlen, vlen;
    LPSTR value;
    LPVOID data;

    if(substlist) {
        for(psub = substlist; psub;) {
	    FontSubst *ptmp;
	    HeapFree(GetProcessHeap(), 0, psub->to.name);
	    HeapFree(GetProcessHeap(), 0, psub->from.name);
	    ptmp = psub;
	    psub = psub->next;
	    HeapFree(GetProcessHeap(), 0, ptmp);
	}
	substlist = NULL;
    }

    if(RegOpenKeyA(HKEY_LOCAL_MACHINE,
		   "Software\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes",
		   &hkey) == ERROR_SUCCESS) {

        RegQueryInfoKeyA(hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			 &valuelen, &datalen, NULL, NULL);

	valuelen++; /* returned value doesn't include room for '\0' */
	value = HeapAlloc(GetProcessHeap(), 0, valuelen * sizeof(CHAR));
	data = HeapAlloc(GetProcessHeap(), 0, datalen);

	dlen = datalen;
	vlen = valuelen;
	ppsub = &substlist;
	while(RegEnumValueA(hkey, i++, value, &vlen, NULL, &type, data,
			    &dlen) == ERROR_SUCCESS) {
	    TRACE("Got %s=%s\n", debugstr_a(value), debugstr_a(data));

	    *ppsub = HeapAlloc(GetProcessHeap(), 0, sizeof(**ppsub));
	    (*ppsub)->next = NULL;
	    split_subst_info(&((*ppsub)->from), value);
	    split_subst_info(&((*ppsub)->to), data);

	    /* Win 2000 doesn't allow mapping between different charsets
	       or mapping of DEFAULT_CHARSET */
	    if(((*ppsub)->to.charset != (*ppsub)->from.charset) ||
	       (*ppsub)->to.charset == DEFAULT_CHARSET) {
	        HeapFree(GetProcessHeap(), 0, (*ppsub)->to.name);
		HeapFree(GetProcessHeap(), 0, (*ppsub)->from.name);
		HeapFree(GetProcessHeap(), 0, *ppsub);
                *ppsub = NULL;
	    } else {
	        ppsub = &((*ppsub)->next);
	    }
	    /* reset dlen and vlen */
	    dlen = datalen;
	    vlen = valuelen;
	}
	HeapFree(GetProcessHeap(), 0, data);
	HeapFree(GetProcessHeap(), 0, value);
	RegCloseKey(hkey);
    }
}

static BOOL ReadFontDir(char *dirname)
{
    DIR *dir;
    struct dirent *dent;
    char path[MAX_PATH];

    TRACE("Loading fonts from %s\n", debugstr_a(dirname));

    dir = opendir(dirname);
    if(!dir) {
        ERR("Can't open directory %s\n", debugstr_a(dirname));
	return FALSE;
    }
    while((dent = readdir(dir)) != NULL) {
	struct stat statbuf;

        if(!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
	    continue;

	TRACE("Found %s in %s\n", debugstr_a(dent->d_name), debugstr_a(dirname));

	sprintf(path, "%s/%s", dirname, dent->d_name);

	if(stat(path, &statbuf) == -1)
	{
	    WARN("Can't stat %s\n", debugstr_a(path));
	    continue;
	}
	if(S_ISDIR(statbuf.st_mode))
	    ReadFontDir(path);
	else
	    AddFontFileToList(path);
    }
    closedir(dir);
    return TRUE;
}

/*************************************************************
 *    WineEngAddFontResourceEx
 *
 */
INT WineEngAddFontResourceEx(LPCWSTR file, DWORD flags, PVOID pdv)
{
    if (ft_handle)  /* do it only if we have freetype up and running */
    {
        DWORD len = WideCharToMultiByte(CP_ACP, 0, file, -1, NULL, 0, NULL, NULL);
        LPSTR fileA = HeapAlloc(GetProcessHeap(), 0, len);
        char unixname[MAX_PATH];
        WideCharToMultiByte(CP_ACP, 0, file, -1, fileA, len, NULL, NULL);

        if(flags)
            FIXME("Ignoring flags %lx\n", flags);

        if(wine_get_unix_file_name(fileA, unixname, sizeof(unixname)))
            AddFontFileToList(unixname);
        HeapFree(GetProcessHeap(), 0, fileA);
    }
    return 1;
}

/*************************************************************
 *    WineEngRemoveFontResourceEx
 *
 */
BOOL WineEngRemoveFontResourceEx(LPCWSTR file, DWORD flags, PVOID pdv)
{
    FIXME(":stub\n");
    return TRUE;
}

/*************************************************************
 *    WineEngInit
 *
 * Initialize FreeType library and create a list of available faces
 */
BOOL WineEngInit(void)
{
    HKEY hkey;
    DWORD valuelen, datalen, i = 0, type, dlen, vlen;
    LPSTR value;
    LPVOID data;
    char windowsdir[MAX_PATH];
    char unixname[MAX_PATH];

    TRACE("\n");

    ft_handle = wine_dlopen(SONAME_LIBFREETYPE, RTLD_NOW, NULL, 0);
    if(!ft_handle) {
        WINE_MESSAGE(
      "Wine cannot find the FreeType font library.  To enable Wine to\n"
      "use TrueType fonts please install a version of FreeType greater than\n"
      "or equal to 2.0.5.\n"
      "http://www.freetype.org\n");
	return FALSE;
    }

#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(ft_handle, #f, NULL, 0)) == NULL){WARN("Can't find symbol %s\n", #f); goto sym_not_found;}

    LOAD_FUNCPTR(FT_Cos)
    LOAD_FUNCPTR(FT_Done_Face)
    LOAD_FUNCPTR(FT_Get_Char_Index)
    LOAD_FUNCPTR(FT_Get_Sfnt_Table)
    LOAD_FUNCPTR(FT_Init_FreeType)
    LOAD_FUNCPTR(FT_Load_Glyph)
    LOAD_FUNCPTR(FT_MulFix)
    LOAD_FUNCPTR(FT_New_Face)
    LOAD_FUNCPTR(FT_Outline_Get_Bitmap)
    LOAD_FUNCPTR(FT_Outline_Transform)
    LOAD_FUNCPTR(FT_Outline_Translate)
    LOAD_FUNCPTR(FT_Select_Charmap)
    LOAD_FUNCPTR(FT_Set_Pixel_Sizes)
    LOAD_FUNCPTR(FT_Sin)
    LOAD_FUNCPTR(FT_Vector_Rotate)

#undef LOAD_FUNCPTR
    /* Don't warn if this one is missing */
    pFT_Library_Version = wine_dlsym(ft_handle, "FT_Library_Version", NULL, 0);

      if(!wine_dlsym(ft_handle, "FT_Get_Postscript_Name", NULL, 0) &&
	 !wine_dlsym(ft_handle, "FT_Sqrt64", NULL, 0)) {
	/* try to avoid 2.0.4: >= 2.0.5 has FT_Get_Postscript_Name and
	   <= 2.0.3 has FT_Sqrt64 */
	  goto sym_not_found;
      }

    if(pFT_Init_FreeType(&library) != 0) {
        ERR("Can't init FreeType library\n");
	wine_dlclose(ft_handle, NULL, 0);
        ft_handle = NULL;
	return FALSE;
    }
    FT_Version.major=FT_Version.minor=FT_Version.patch=-1;
    if (pFT_Library_Version)
    {
        pFT_Library_Version(library,&FT_Version.major,&FT_Version.minor,&FT_Version.patch);
    }
    if (FT_Version.major<=0)
    {
        FT_Version.major=2;
        FT_Version.minor=0;
        FT_Version.patch=5;
    }
    TRACE("FreeType version is %d.%d.%d\n",FT_Version.major,FT_Version.minor,FT_Version.patch);

    /* load in the fonts from %WINDOWSDIR%\\Fonts first of all */
    GetWindowsDirectoryA(windowsdir, sizeof(windowsdir));
    strcat(windowsdir, "\\Fonts");
    if(wine_get_unix_file_name(windowsdir, unixname, sizeof(unixname)))
        ReadFontDir(unixname);

    /* now look under HKLM\Software\Microsoft\Windows\CurrentVersion\Fonts
       for any fonts not installed in %WINDOWSDIR%\Fonts.  They will have their
       full path as the entry */
    if(RegOpenKeyA(HKEY_LOCAL_MACHINE,
		   "Software\\Microsoft\\Windows\\CurrentVersion\\Fonts",
		   &hkey) == ERROR_SUCCESS) {
        RegQueryInfoKeyA(hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			 &valuelen, &datalen, NULL, NULL);

	valuelen++; /* returned value doesn't include room for '\0' */
	value = HeapAlloc(GetProcessHeap(), 0, valuelen);
	data = HeapAlloc(GetProcessHeap(), 0, datalen);

	dlen = datalen;
	vlen = valuelen;
	while(RegEnumValueA(hkey, i++, value, &vlen, NULL, &type, data,
			    &dlen) == ERROR_SUCCESS) {
	    if(((LPSTR)data)[1] == ':')
	        if(wine_get_unix_file_name((LPSTR)data, unixname, sizeof(unixname)))
		    AddFontFileToList(unixname);

	    /* reset dlen and vlen */
	    dlen = datalen;
	    vlen = valuelen;
	}
	HeapFree(GetProcessHeap(), 0, data);
	HeapFree(GetProcessHeap(), 0, value);
	RegCloseKey(hkey);
    }


    /* then look in any directories that we've specified in the config file */
    if(RegOpenKeyA(HKEY_LOCAL_MACHINE,
		   "Software\\Wine\\Wine\\Config\\FontDirs",
		   &hkey) == ERROR_SUCCESS) {

        RegQueryInfoKeyA(hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			 &valuelen, &datalen, NULL, NULL);

	valuelen++; /* returned value doesn't include room for '\0' */
	value = HeapAlloc(GetProcessHeap(), 0, valuelen);
	data = HeapAlloc(GetProcessHeap(), 0, datalen);

	dlen = datalen;
	vlen = valuelen;
	i = 0;
	while(RegEnumValueA(hkey, i++, value, &vlen, NULL, &type, data,
			    &dlen) == ERROR_SUCCESS) {
	    TRACE("Got %s=%s\n", value, (LPSTR)data);
	    ReadFontDir((LPSTR)data);
	    /* reset dlen and vlen */
	    dlen = datalen;
	    vlen = valuelen;
	}
	HeapFree(GetProcessHeap(), 0, data);
	HeapFree(GetProcessHeap(), 0, value);
	RegCloseKey(hkey);
    }

    DumpFontList();
    LoadSubstList();
    DumpSubstList();
    return TRUE;
sym_not_found:
    WINE_MESSAGE(
      "Wine cannot find certain functions that it needs inside the FreeType\n"
      "font library.  To enable Wine to use TrueType fonts please upgrade\n"
      "FreeType to at least version 2.0.5.\n"
      "http://www.freetype.org\n");
    wine_dlclose(ft_handle, NULL, 0);
    ft_handle = NULL;
    return FALSE;
}


static LONG calc_ppem_for_height(FT_Face ft_face, LONG height)
{
    TT_OS2 *pOS2;
    LONG ppem;

    pOS2 = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_os2);

    if(height == 0) height = 16;

    /* Calc. height of EM square:
     *
     * For +ve lfHeight we have
     * lfHeight = (winAscent + winDescent) * ppem / units_per_em
     * Re-arranging gives:
     * ppem = units_per_em * lfheight / (winAscent + winDescent)
     *
     * For -ve lfHeight we have
     * |lfHeight| = ppem
     * [i.e. |lfHeight| = (winAscent + winDescent - il) * ppem / units_per_em
     * with il = winAscent + winDescent - units_per_em]
     *
     */

    if(height > 0)
        ppem = ft_face->units_per_EM * height /
	    (pOS2->usWinAscent + pOS2->usWinDescent);
    else
        ppem = -height;

    return ppem;
}

static LONG load_VDMX(GdiFont, LONG);

static FT_Face OpenFontFile(GdiFont font, char *file, FT_Long face_index, LONG height)
{
    FT_Error err;
    FT_Face ft_face;
    LONG ppem;

    err = pFT_New_Face(library, file, face_index, &ft_face);
    if(err) {
        ERR("FT_New_Face rets %d\n", err);
	return 0;
    }

    /* set it here, as load_VDMX needs it */
    font->ft_face = ft_face;

    /* load the VDMX table if we have one */
    ppem = load_VDMX(font, height);
    if(ppem == 0)
        ppem = calc_ppem_for_height(ft_face, height);

    pFT_Set_Pixel_Sizes(ft_face, 0, ppem);

    return ft_face;
}


static int get_nearest_charset(Face *face)
{
  /* Only get here if lfCharSet == DEFAULT_CHARSET or we couldn't find
     a single face with the requested charset.  The idea is to check if
     the selected font supports the current ANSI codepage, if it does
     return the corresponding charset, else return the first charset */

    CHARSETINFO csi;
    int acp = GetACP(), i;
    DWORD fs0;

    if(TranslateCharsetInfo((DWORD*)acp, &csi, TCI_SRCCODEPAGE))
        if(csi.fs.fsCsb[0] & face->fsCsb[0])
	    return csi.ciCharset;

    for(i = 0; i < 32; i++) {
        fs0 = 1L << i;
        if(face->fsCsb[0] & fs0) {
	    if(TranslateCharsetInfo(&fs0, &csi, TCI_SRCFONTSIG))
	        return csi.ciCharset;
	    else
	        FIXME("TCI failing on %lx\n", fs0);
	}
    }

    FIXME("returning DEFAULT_CHARSET face->fsCsb[0] = %08lx file = %s\n",
	  face->fsCsb[0], face->file);
    return DEFAULT_CHARSET;
}

static GdiFont alloc_font(void)
{
    GdiFont ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    ret->gmsize = INIT_GM_SIZE;
    ret->gm = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			ret->gmsize * sizeof(*ret->gm));
    ret->next = NULL;
    ret->potm = NULL;
    ret->xform.eM11 = ret->xform.eM22 = 1.0;
    return ret;
}

static void free_font(GdiFont font)
{
    if (font->ft_face) pFT_Done_Face(font->ft_face);
    if (font->potm) HeapFree(GetProcessHeap(), 0, font->potm);
    if (font->name) HeapFree(GetProcessHeap(), 0, font->name);
    HeapFree(GetProcessHeap(), 0, font->gm);
    HeapFree(GetProcessHeap(), 0, font);
}


/*************************************************************
 * load_VDMX
 *
 * load the vdmx entry for the specified height
 */

#define MS_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (FT_ULong)_x4 << 24 ) |     \
            ( (FT_ULong)_x3 << 16 ) |     \
            ( (FT_ULong)_x2 <<  8 ) |     \
              (FT_ULong)_x1         )

#define MS_VDMX_TAG MS_MAKE_TAG('V', 'D', 'M', 'X')

typedef struct {
    BYTE bCharSet;
    BYTE xRatio;
    BYTE yStartRatio;
    BYTE yEndRatio;
} Ratios;


static LONG load_VDMX(GdiFont font, LONG height)
{
    BYTE hdr[6], tmp[2], group[4];
    BYTE devXRatio, devYRatio;
    USHORT numRecs, numRatios;
    DWORD offset = -1;
    LONG ppem = 0;
    int i, result;

    result = WineEngGetFontData(font, MS_VDMX_TAG, 0, hdr, 6);

    if(result == GDI_ERROR) /* no vdmx table present, use linear scaling */
	return ppem;

    /* FIXME: need the real device aspect ratio */
    devXRatio = 1;
    devYRatio = 1;

    numRecs = GET_BE_WORD(&hdr[2]);
    numRatios = GET_BE_WORD(&hdr[4]);

    TRACE("numRecs = %d numRatios = %d\n", numRecs, numRatios);
    for(i = 0; i < numRatios; i++) {
	Ratios ratio;

	offset = (3 * 2) + (i * sizeof(Ratios));
	WineEngGetFontData(font, MS_VDMX_TAG, offset, &ratio, sizeof(Ratios));
	offset = -1;

	TRACE("Ratios[%d] %d  %d : %d -> %d\n", i, ratio.bCharSet, ratio.xRatio, ratio.yStartRatio, ratio.yEndRatio);

	if(ratio.bCharSet != 1)
	    continue;

	if((ratio.xRatio == 0 &&
	    ratio.yStartRatio == 0 &&
	    ratio.yEndRatio == 0) ||
	   (devXRatio == ratio.xRatio &&
	    devYRatio >= ratio.yStartRatio &&
	    devYRatio <= ratio.yEndRatio))
	    {
		offset = (3 * 2) + (numRatios * 4) + (i * 2);
		WineEngGetFontData(font, MS_VDMX_TAG, offset, tmp, 2);
		offset = GET_BE_WORD(tmp);
		break;
	    }
    }

    if(offset < 0) {
	FIXME("No suitable ratio found\n");
	return ppem;
    }

    if(WineEngGetFontData(font, MS_VDMX_TAG, offset, group, 4) != GDI_ERROR) {
	USHORT recs;
	BYTE startsz, endsz;
	BYTE *vTable;

	recs = GET_BE_WORD(group);
	startsz = group[2];
	endsz = group[3];

	TRACE("recs=%d  startsz=%d  endsz=%d\n", recs, startsz, endsz);

	vTable = HeapAlloc(GetProcessHeap(), 0, recs * 6);
	result = WineEngGetFontData(font, MS_VDMX_TAG, offset + 4, vTable, recs * 6);
	if(result == GDI_ERROR) {
	    FIXME("Failed to retrieve vTable\n");
	    goto end;
	}

	if(height > 0) {
	    for(i = 0; i < recs; i++) {
		SHORT yMax = GET_BE_WORD(&vTable[(i * 6) + 2]);
                SHORT yMin = GET_BE_WORD(&vTable[(i * 6) + 4]);
		ppem = GET_BE_WORD(&vTable[i * 6]);

		if(yMax + -yMin == height) {
		    font->yMax = yMax;
		    font->yMin = yMin;
		    TRACE("ppem %ld found; height=%ld  yMax=%d  yMin=%d\n", ppem, height, font->yMax, font->yMin);
		    break;
		}
		if(yMax + -yMin > height) {
		    if(--i < 0) {
			ppem = 0;
			goto end; /* failed */
		    }
		    font->yMax = GET_BE_WORD(&vTable[(i * 6) + 2]);
		    font->yMin = GET_BE_WORD(&vTable[(i * 6) + 4]);
                    TRACE("ppem %ld found; height=%ld  yMax=%d  yMin=%d\n", ppem, height, font->yMax, font->yMin);
		    break;
		}
	    }
	    if(!font->yMax) {
		ppem = 0;
		TRACE("ppem not found for height %ld\n", height);
	    }
	} else {
	    ppem = -height;
	    if(ppem < startsz || ppem > endsz)
		goto end;

	    for(i = 0; i < recs; i++) {
		USHORT yPelHeight;
		yPelHeight = GET_BE_WORD(&vTable[i * 6]);

		if(yPelHeight > ppem)
		    break; /* failed */

		if(yPelHeight == ppem) {
		    font->yMax = GET_BE_WORD(&vTable[(i * 6) + 2]);
		    font->yMin = GET_BE_WORD(&vTable[(i * 6) + 4]);
		    TRACE("ppem %ld found; yMax=%d  yMin=%d\n", ppem, font->yMax, font->yMin);
		    break;
		}
	    }
	}
	end:
	HeapFree(GetProcessHeap(), 0, vTable);
    }

    return ppem;
}


/*************************************************************
 * WineEngCreateFontInstance
 *
 */
GdiFont WineEngCreateFontInstance(DC *dc, HFONT hfont)
{
    GdiFont ret;
    Face *face;
    Family *family = NULL;
    BOOL bd, it;
    LOGFONTW lf;
    CHARSETINFO csi;

    if (!GetObjectW( hfont, sizeof(lf), &lf )) return NULL;

    TRACE("%s, h=%ld, it=%d, weight=%ld, PandF=%02x, charset=%d orient %ld escapement %ld\n",
	  debugstr_w(lf.lfFaceName), lf.lfHeight, lf.lfItalic,
	  lf.lfWeight, lf.lfPitchAndFamily, lf.lfCharSet, lf.lfOrientation,
	  lf.lfEscapement);

    /* check the cache first */
    for(ret = GdiFontList; ret; ret = ret->next) {
	if(ret->hfont == hfont && !memcmp(&ret->xform, &dc->xformWorld2Vport, offsetof(XFORM, eDx))) {
	    TRACE("returning cached gdiFont(%p) for hFont %p\n", ret, hfont);
	    return ret;
	}
    }

    if(!FontList) /* No fonts installed */
    {
	TRACE("No fonts installed\n");
	return NULL;
    }

    ret = alloc_font();
    memcpy(&ret->xform, &dc->xformWorld2Vport, sizeof(XFORM));

    /* If lfFaceName is "Symbol" then Windows fixes up lfCharSet to
       SYMBOL_CHARSET so that Symbol gets picked irrespective of the
       original value lfCharSet.  Note this is a special case for
       Symbol and doesn't happen at least for "Wingdings*" */

    if(!strcmpiW(lf.lfFaceName, SymbolW))
        lf.lfCharSet = SYMBOL_CHARSET;

    if(!TranslateCharsetInfo((DWORD*)(INT)lf.lfCharSet, &csi, TCI_SRCCHARSET)) {
        switch(lf.lfCharSet) {
	case DEFAULT_CHARSET:
	    csi.fs.fsCsb[0] = 0;
	    break;
	default:
	    FIXME("Untranslated charset %d\n", lf.lfCharSet);
	    csi.fs.fsCsb[0] = 0;
	    break;
	}
    }

    if(lf.lfFaceName[0] != '\0') {
        FontSubst *psub;
	for(psub = substlist; psub; psub = psub->next)
	    if(!strcmpiW(lf.lfFaceName, psub->from.name) &&
	       (psub->from.charset == -1 ||
		psub->from.charset == lf.lfCharSet))
	      break;
	if(psub) {
	    TRACE("substituting %s -> %s\n", debugstr_w(lf.lfFaceName),
		  debugstr_w(psub->to.name));
	    strcpyW(lf.lfFaceName, psub->to.name);
	}

	/* We want a match on name and charset or just name if
	   charset was DEFAULT_CHARSET.  If the latter then
	   we fixup the returned charset later in get_nearest_charset
	   where we'll either use the charset of the current ansi codepage
	   or if that's unavailable the first charset that the font supports.
	*/
        for(family = FontList; family; family = family->next) {
	    if(!strcmpiW(family->FamilyName, lf.lfFaceName))
	        if((csi.fs.fsCsb[0] & family->FirstFace->fsCsb[0]) || !csi.fs.fsCsb[0])
		    break;
	}

	if(!family) { /* do other aliases here */
	    if(!strcmpiW(lf.lfFaceName, SystemW))
	        strcpyW(lf.lfFaceName, defSystem);
	    else if(!strcmpiW(lf.lfFaceName, MSSansSerifW))
	        strcpyW(lf.lfFaceName, defSans);
	    else if(!strcmpiW(lf.lfFaceName, HelvW))
	        strcpyW(lf.lfFaceName, defSans);
	    else
	        goto not_found;

	    for(family = FontList; family; family = family->next) {
	        if(!strcmpiW(family->FamilyName, lf.lfFaceName))
		  if((csi.fs.fsCsb[0] & family->FirstFace->fsCsb[0]) || !csi.fs.fsCsb[0])
		      break;
	    }
	}
    }

not_found:
    if(!family) {
      /* If requested charset was DEFAULT_CHARSET then try using charset
	 corresponding to the current ansi codepage */
        if(!csi.fs.fsCsb[0]) {
	    INT acp = GetACP();
	    if(!TranslateCharsetInfo((DWORD*)acp, &csi, TCI_SRCCODEPAGE)) {
	        FIXME("TCI failed on codepage %d\n", acp);
		csi.fs.fsCsb[0] = 0;
	    } else
	        lf.lfCharSet = csi.ciCharset;
	}
        if(lf.lfPitchAndFamily & FIXED_PITCH ||
	   lf.lfPitchAndFamily & FF_MODERN)
	  strcpyW(lf.lfFaceName, defFixed);
	else if(lf.lfPitchAndFamily & FF_ROMAN)
	  strcpyW(lf.lfFaceName, defSerif);
	else if(lf.lfPitchAndFamily & FF_SWISS)
	  strcpyW(lf.lfFaceName, defSans);
	else
	  strcpyW(lf.lfFaceName, defSans);
	for(family = FontList; family; family = family->next) {
	    if(!strcmpiW(family->FamilyName, lf.lfFaceName) &&
	       (csi.fs.fsCsb[0] & family->FirstFace->fsCsb[0]))
	        break;
	}
    }

    if(!family) {
        for(family = FontList; family; family = family->next) {
	    if(csi.fs.fsCsb[0] & family->FirstFace->fsCsb[0])
	        break;
	}
    }

    if(!family) {
        family = FontList;
	csi.fs.fsCsb[0] = 0;
	FIXME("just using first face for now\n");
    }

    it = lf.lfItalic ? 1 : 0;
    bd = lf.lfWeight > 550 ? 1 : 0;

    for(face = family->FirstFace; face; face = face->next) {
      if(!(face->Italic ^ it) && !(face->Bold ^ bd))
	break;
    }
    if(!face) {
        face = family->FirstFace;
	if(it && !face->Italic) ret->fake_italic = TRUE;
	if(bd && !face->Bold) ret->fake_bold = TRUE;
    }

    if(csi.fs.fsCsb[0])
        ret->charset = lf.lfCharSet;
    else
        ret->charset = get_nearest_charset(face);

    TRACE("Chosen: %s %s\n", debugstr_w(family->FamilyName),
	  debugstr_w(face->StyleName));

    ret->ft_face = OpenFontFile(ret, face->file, face->face_index,
				lf.lfHeight < 0 ?
				-abs(INTERNAL_YWSTODS(dc,lf.lfHeight)) :
				abs(INTERNAL_YWSTODS(dc, lf.lfHeight)));
    if (!ret->ft_face)
    {
        free_font( ret );
        return 0;
    }

    if(ret->charset == SYMBOL_CHARSET)
        pFT_Select_Charmap(ret->ft_face, ft_encoding_symbol);
    ret->orientation = lf.lfOrientation;
    ret->name = strdupW(family->FamilyName);

    TRACE("caching: gdiFont=%p  hfont=%p\n", ret, hfont);
    ret->hfont = hfont;
    ret->next = GdiFontList;
    GdiFontList = ret;

    return ret;
}

static void DumpGdiFontList(void)
{
    GdiFont gdiFont;

    TRACE("---------- gdiFont Cache ----------\n");
    for(gdiFont = GdiFontList; gdiFont; gdiFont = gdiFont->next) {
	LOGFONTW lf;
        GetObjectW( gdiFont->hfont, sizeof(lf), &lf );
	TRACE("gdiFont=%p  hfont=%p (%s)\n",
	       gdiFont, gdiFont->hfont, debugstr_w(lf.lfFaceName));
    }
}

/*************************************************************
 * WineEngDestroyFontInstance
 *
 * free the gdiFont associated with this handle
 *
 */
BOOL WineEngDestroyFontInstance(HFONT handle)
{
    GdiFont gdiFont;
    GdiFont gdiPrev = NULL;
    BOOL ret = FALSE;

    TRACE("destroying hfont=%p\n", handle);
    if(TRACE_ON(font))
	DumpGdiFontList();

    gdiFont = GdiFontList;
    while(gdiFont) {
	if(gdiFont->hfont == handle) {
	    if(gdiPrev) {
	        gdiPrev->next = gdiFont->next;
		free_font(gdiFont);
		gdiFont = gdiPrev->next;
	    } else {
		GdiFontList = gdiFont->next;
		free_font(gdiFont);
		gdiFont = GdiFontList;
	    }
	    ret = TRUE;
	} else {
	    gdiPrev = gdiFont;
	    gdiFont = gdiFont->next;
	}
    }
    return ret;
}

static void GetEnumStructs(Face *face, LPENUMLOGFONTEXW pelf,
			   NEWTEXTMETRICEXW *pntm, LPDWORD ptype)
{
    OUTLINETEXTMETRICW *potm;
    UINT size;
    GdiFont font = alloc_font();

    if (!(font->ft_face = OpenFontFile(font, face->file, face->face_index, 100)))
    {
        free_font(font);
        return;
    }

    memset(&pelf->elfLogFont, 0, sizeof(LOGFONTW));

    size = WineEngGetOutlineTextMetrics(font, 0, NULL);
    potm = HeapAlloc(GetProcessHeap(), 0, size);
    WineEngGetOutlineTextMetrics(font, size, potm);

#define TM potm->otmTextMetrics

    pntm->ntmTm.tmHeight = pelf->elfLogFont.lfHeight = TM.tmHeight;
    pntm->ntmTm.tmAscent = TM.tmAscent;
    pntm->ntmTm.tmDescent = TM.tmDescent;
    pntm->ntmTm.tmInternalLeading = TM.tmInternalLeading;
    pntm->ntmTm.tmExternalLeading = TM.tmExternalLeading;
    pntm->ntmTm.tmAveCharWidth = pelf->elfLogFont.lfWidth = TM.tmAveCharWidth;
    pntm->ntmTm.tmMaxCharWidth = TM.tmMaxCharWidth;
    pntm->ntmTm.tmWeight = pelf->elfLogFont.lfWeight = TM.tmWeight;
    pntm->ntmTm.tmOverhang = TM.tmOverhang;
    pntm->ntmTm.tmDigitizedAspectX = TM.tmDigitizedAspectX;
    pntm->ntmTm.tmDigitizedAspectY = TM.tmDigitizedAspectY;
    pntm->ntmTm.tmFirstChar = TM.tmFirstChar;
    pntm->ntmTm.tmLastChar = TM.tmLastChar;
    pntm->ntmTm.tmDefaultChar = TM.tmDefaultChar;
    pntm->ntmTm.tmBreakChar = TM.tmBreakChar;
    pntm->ntmTm.tmItalic = pelf->elfLogFont.lfItalic = TM.tmItalic;
    pntm->ntmTm.tmUnderlined = pelf->elfLogFont.lfUnderline = TM.tmUnderlined;
    pntm->ntmTm.tmStruckOut = pelf->elfLogFont.lfStrikeOut = TM.tmStruckOut;
    pntm->ntmTm.tmPitchAndFamily = TM.tmPitchAndFamily;
    pelf->elfLogFont.lfPitchAndFamily = (TM.tmPitchAndFamily & 0xf1) + 1;
    pntm->ntmTm.tmCharSet = pelf->elfLogFont.lfCharSet = TM.tmCharSet;
    pelf->elfLogFont.lfOutPrecision = OUT_STROKE_PRECIS;
    pelf->elfLogFont.lfClipPrecision = CLIP_STROKE_PRECIS;
    pelf->elfLogFont.lfQuality = DRAFT_QUALITY;

    pntm->ntmTm.ntmFlags = TM.tmItalic ? NTM_ITALIC : 0;
    if(TM.tmWeight > 550) pntm->ntmTm.ntmFlags |= NTM_BOLD;
    if(pntm->ntmTm.ntmFlags == 0) pntm->ntmTm.ntmFlags = NTM_REGULAR;

    pntm->ntmTm.ntmSizeEM = potm->otmEMSquare;
    pntm->ntmTm.ntmCellHeight = 0;
    pntm->ntmTm.ntmAvgWidth = 0;

    *ptype = TM.tmPitchAndFamily & TMPF_TRUETYPE ? TRUETYPE_FONTTYPE : 0;
    if(!(TM.tmPitchAndFamily & TMPF_VECTOR))
        *ptype |= RASTER_FONTTYPE;

#undef TM
    memset(&pntm->ntmFontSig, 0, sizeof(FONTSIGNATURE));

    strncpyW(pelf->elfLogFont.lfFaceName,
	     (WCHAR*)((char*)potm + (ptrdiff_t)potm->otmpFamilyName),
	     LF_FACESIZE);
    strncpyW(pelf->elfFullName,
	     (WCHAR*)((char*)potm + (ptrdiff_t)potm->otmpFaceName),
	     LF_FULLFACESIZE);
    strncpyW(pelf->elfStyle,
	     (WCHAR*)((char*)potm + (ptrdiff_t)potm->otmpStyleName),
	     LF_FACESIZE);
    pelf->elfScript[0] = '\0'; /* This will get set in WineEngEnumFonts */

    HeapFree(GetProcessHeap(), 0, potm);
    free_font(font);
    return;
}

/*************************************************************
 * WineEngEnumFonts
 *
 */
DWORD WineEngEnumFonts(LPLOGFONTW plf, DEVICEFONTENUMPROC proc,
		       LPARAM lparam)
{
    Family *family;
    Face *face;
    ENUMLOGFONTEXW elf;
    NEWTEXTMETRICEXW ntm;
    DWORD type, ret = 1;
    FONTSIGNATURE fs;
    CHARSETINFO csi;
    int i;

    TRACE("facename = %s charset %d\n", debugstr_w(plf->lfFaceName), plf->lfCharSet);

    if(plf->lfFaceName[0]) {
        for(family = FontList; family; family = family->next) {
	    if(!strcmpiW(plf->lfFaceName, family->FamilyName)) {
	        for(face = family->FirstFace; face; face = face->next) {
		    GetEnumStructs(face, &elf, &ntm, &type);
		    for(i = 0; i < 32; i++) {
		        if(face->fsCsb[0] & (1L << i)) {
			    fs.fsCsb[0] = 1L << i;
			    fs.fsCsb[1] = 0;
			    if(!TranslateCharsetInfo(fs.fsCsb, &csi,
						     TCI_SRCFONTSIG))
			        csi.ciCharset = DEFAULT_CHARSET;
			    if(i == 31) csi.ciCharset = SYMBOL_CHARSET;
			    if(csi.ciCharset != DEFAULT_CHARSET) {
			        elf.elfLogFont.lfCharSet =
				  ntm.ntmTm.tmCharSet = csi.ciCharset;
				if(ElfScriptsW[i])
				    strcpyW(elf.elfScript, ElfScriptsW[i]);
				else
				    FIXME("Unknown elfscript for bit %d\n", i);
				TRACE("enuming face %s full %s style %s charset %d type %ld script %s it %d weight %ld ntmflags %08lx\n",
				      debugstr_w(elf.elfLogFont.lfFaceName),
				      debugstr_w(elf.elfFullName), debugstr_w(elf.elfStyle),
				      csi.ciCharset, type, debugstr_w(elf.elfScript),
				      elf.elfLogFont.lfItalic, elf.elfLogFont.lfWeight,
				      ntm.ntmTm.ntmFlags);
				ret = proc(&elf, &ntm, type, lparam);
				if(!ret) goto end;
			    }
			}
		    }
		}
	    }
	}
    } else {
        for(family = FontList; family; family = family->next) {
	    GetEnumStructs(family->FirstFace, &elf, &ntm, &type);
	    for(i = 0; i < 32; i++) {
	        if(family->FirstFace->fsCsb[0] & (1L << i)) {
		    fs.fsCsb[0] = 1L << i;
		    fs.fsCsb[1] = 0;
		    if(!TranslateCharsetInfo(fs.fsCsb, &csi,
					     TCI_SRCFONTSIG))
		        csi.ciCharset = DEFAULT_CHARSET;
		    if(i == 31) csi.ciCharset = SYMBOL_CHARSET;
		    if(csi.ciCharset != DEFAULT_CHARSET) {
		        elf.elfLogFont.lfCharSet = ntm.ntmTm.tmCharSet =
			  csi.ciCharset;
			  if(ElfScriptsW[i])
			      strcpyW(elf.elfScript, ElfScriptsW[i]);
			  else
			      FIXME("Unknown elfscript for bit %d\n", i);
			TRACE("enuming face %s full %s style %s charset = %d type %ld script %s it %d weight %ld ntmflags %08lx\n",
			      debugstr_w(elf.elfLogFont.lfFaceName),
			      debugstr_w(elf.elfFullName), debugstr_w(elf.elfStyle),
			      csi.ciCharset, type, debugstr_w(elf.elfScript),
			      elf.elfLogFont.lfItalic, elf.elfLogFont.lfWeight,
			      ntm.ntmTm.ntmFlags);
			ret = proc(&elf, &ntm, type, lparam);
			if(!ret) goto end;
		    }
		}
	    }
	}
    }
end:
    return ret;
}

static void FTVectorToPOINTFX(FT_Vector *vec, POINTFX *pt)
{
    pt->x.value = vec->x >> 6;
    pt->x.fract = (vec->x & 0x3f) << 10;
    pt->x.fract |= ((pt->x.fract >> 6) | (pt->x.fract >> 12));
    pt->y.value = vec->y >> 6;
    pt->y.fract = (vec->y & 0x3f) << 10;
    pt->y.fract |= ((pt->y.fract >> 6) | (pt->y.fract >> 12));
    return;
}

static FT_UInt get_glyph_index(GdiFont font, UINT glyph)
{
    if(font->charset == SYMBOL_CHARSET && glyph < 0x100)
        glyph = glyph + 0xf000;
    return pFT_Get_Char_Index(font->ft_face, glyph);
}

/*************************************************************
 * WineEngGetGlyphIndices
 *
 * FIXME: add support for GGI_MARK_NONEXISTING_GLYPHS
 */
DWORD WineEngGetGlyphIndices(GdiFont font, LPCWSTR lpstr, INT count,
				LPWORD pgi, DWORD flags)
{
    INT i;

    for(i = 0; i < count; i++)
        pgi[i] = get_glyph_index(font, lpstr[i]);

    return count;
}

/*************************************************************
 * WineEngGetGlyphOutline
 *
 * Behaves in exactly the same way as the win32 api GetGlyphOutline
 * except that the first parameter is the HWINEENGFONT of the font in
 * question rather than an HDC.
 *
 */
DWORD WineEngGetGlyphOutline(GdiFont font, UINT glyph, UINT format,
			     LPGLYPHMETRICS lpgm, DWORD buflen, LPVOID buf,
			     const MAT2* lpmat)
{
    FT_Face ft_face = font->ft_face;
    FT_UInt glyph_index;
    DWORD width, height, pitch, needed = 0;
    FT_Bitmap ft_bitmap;
    FT_Error err;
    INT left, right, top = 0, bottom = 0;
    FT_Angle angle = 0;
    FT_Int load_flags = FT_LOAD_DEFAULT;

    TRACE("%p, %04x, %08x, %p, %08lx, %p, %p\n", font, glyph, format, lpgm,
	  buflen, buf, lpmat);

    if(format & GGO_GLYPH_INDEX) {
        glyph_index = glyph;
	format &= ~GGO_GLYPH_INDEX;
    } else
        glyph_index = get_glyph_index(font, glyph);

    if(glyph_index >= font->gmsize) {
        font->gmsize = (glyph_index / INIT_GM_SIZE + 1) * INIT_GM_SIZE;
	font->gm = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, font->gm,
			       font->gmsize * sizeof(*font->gm));
    } else {
        if(format == GGO_METRICS && font->gm[glyph_index].init) {
	    memcpy(lpgm, &font->gm[glyph_index].gm, sizeof(*lpgm));
	    return 1; /* FIXME */
	}
    }

    if(font->orientation || (format != GGO_METRICS && format != GGO_BITMAP))
        load_flags |= FT_LOAD_NO_BITMAP;

    err = pFT_Load_Glyph(ft_face, glyph_index, load_flags);

    if(err) {
        FIXME("FT_Load_Glyph on index %x returns %d\n", glyph_index, err);
	return GDI_ERROR;
    }

    left = ft_face->glyph->metrics.horiBearingX & -64;
    right = ((ft_face->glyph->metrics.horiBearingX +
		  ft_face->glyph->metrics.width) + 63) & -64;

    font->gm[glyph_index].adv = (ft_face->glyph->metrics.horiAdvance + 63) >> 6;
    font->gm[glyph_index].lsb = left >> 6;
    font->gm[glyph_index].bbx = (right - left) >> 6;

    if(font->orientation == 0) {
	top = (ft_face->glyph->metrics.horiBearingY + 63) & -64;
	bottom = (ft_face->glyph->metrics.horiBearingY -
		  ft_face->glyph->metrics.height) & -64;
	lpgm->gmCellIncX = font->gm[glyph_index].adv;
	lpgm->gmCellIncY = 0;
    } else {
        INT xc, yc;
	FT_Vector vec;
	angle = font->orientation / 10 << 16;
	angle |= ((font->orientation % 10) * (1 << 16)) / 10;
	TRACE("angle %ld\n", angle >> 16);
	for(xc = 0; xc < 2; xc++) {
	    for(yc = 0; yc < 2; yc++) {
	        vec.x = ft_face->glyph->metrics.horiBearingX +
		  xc * ft_face->glyph->metrics.width;
		vec.y = ft_face->glyph->metrics.horiBearingY -
		  yc * ft_face->glyph->metrics.height;
		TRACE("Vec %ld,%ld\n", vec.x, vec.y);
		pFT_Vector_Rotate(&vec, angle);
		if(xc == 0 && yc == 0) {
		    left = right = vec.x;
		    top = bottom = vec.y;
		} else {
		    if(vec.x < left) left = vec.x;
		    else if(vec.x > right) right = vec.x;
		    if(vec.y < bottom) bottom = vec.y;
		    else if(vec.y > top) top = vec.y;
		}
	    }
	}
	left = left & -64;
	right = (right + 63) & -64;
	bottom = bottom & -64;
	top = (top + 63) & -64;

	TRACE("transformed box: (%d,%d - %d,%d)\n", left, top, right, bottom);
	vec.x = ft_face->glyph->metrics.horiAdvance;
	vec.y = 0;
	pFT_Vector_Rotate(&vec, angle);
	lpgm->gmCellIncX = (vec.x+63) >> 6;
	lpgm->gmCellIncY = -(vec.y+63) >> 6;
    }
    lpgm->gmBlackBoxX = (right - left) >> 6;
    lpgm->gmBlackBoxY = (top - bottom) >> 6;
    lpgm->gmptGlyphOrigin.x = left >> 6;
    lpgm->gmptGlyphOrigin.y = top >> 6;

    memcpy(&font->gm[glyph_index].gm, lpgm, sizeof(*lpgm));
    font->gm[glyph_index].init = TRUE;

    if(format == GGO_METRICS)
        return 1; /* FIXME */

    if(ft_face->glyph->format != ft_glyph_format_outline && format != GGO_BITMAP) {
        FIXME("loaded a bitmap\n");
	return GDI_ERROR;
    }

    switch(format) {
    case GGO_BITMAP:
        width = lpgm->gmBlackBoxX;
	height = lpgm->gmBlackBoxY;
	pitch = (width + 31) / 32 * 4;
        needed = pitch * height;

	if(!buf || !buflen) break;

	switch(ft_face->glyph->format) {
	case ft_glyph_format_bitmap:
	  {
	    BYTE *src = ft_face->glyph->bitmap.buffer, *dst = buf;
	    INT w = (ft_face->glyph->bitmap.width + 7) >> 3;
	    INT h = ft_face->glyph->bitmap.rows;
	    while(h--) {
	        memcpy(dst, src, w);
		src += ft_face->glyph->bitmap.pitch;
		dst += pitch;
	    }
	    break;
	  }

	case ft_glyph_format_outline:
	    ft_bitmap.width = width;
	    ft_bitmap.rows = height;
	    ft_bitmap.pitch = pitch;
	    ft_bitmap.pixel_mode = ft_pixel_mode_mono;
	    ft_bitmap.buffer = buf;

	    if(font->orientation) {
	        FT_Matrix matrix;
		matrix.xx = matrix.yy = pFT_Cos(angle);
		matrix.xy = -pFT_Sin(angle);
		matrix.yx = -matrix.xy;

		pFT_Outline_Transform(&ft_face->glyph->outline, &matrix);
	    }

	    pFT_Outline_Translate(&ft_face->glyph->outline, -left, -bottom );

	    /* Note: FreeType will only set 'black' bits for us. */
	    memset(buf, 0, needed);
	    pFT_Outline_Get_Bitmap(library, &ft_face->glyph->outline, &ft_bitmap);
	    break;

	default:
	    FIXME("loaded glyph format %x\n", ft_face->glyph->format);
	    return GDI_ERROR;
	}
	break;

    case GGO_GRAY2_BITMAP:
    case GGO_GRAY4_BITMAP:
    case GGO_GRAY8_BITMAP:
    case WINE_GGO_GRAY16_BITMAP:
      {
	int mult, row, col;
	BYTE *start, *ptr;

        width = lpgm->gmBlackBoxX;
	height = lpgm->gmBlackBoxY;
	pitch = (width + 3) / 4 * 4;
	needed = pitch * height;

	if(!buf || !buflen) break;
	ft_bitmap.width = width;
	ft_bitmap.rows = height;
	ft_bitmap.pitch = pitch;
	ft_bitmap.pixel_mode = ft_pixel_mode_grays;
	ft_bitmap.buffer = buf;

	if(font->orientation) {
	    FT_Matrix matrix;
	    matrix.xx = matrix.yy = pFT_Cos(angle);
	    matrix.xy = -pFT_Sin(angle);
	    matrix.yx = -matrix.xy;
	    pFT_Outline_Transform(&ft_face->glyph->outline, &matrix);
	}

	pFT_Outline_Translate(&ft_face->glyph->outline, -left, -bottom );

	pFT_Outline_Get_Bitmap(library, &ft_face->glyph->outline, &ft_bitmap);

	if(format == GGO_GRAY2_BITMAP)
	    mult = 5;
	else if(format == GGO_GRAY4_BITMAP)
	    mult = 17;
	else if(format == GGO_GRAY8_BITMAP)
	    mult = 65;
	else if(format == WINE_GGO_GRAY16_BITMAP)
	    break;
	else {
	    assert(0);
	    break;
	}

	start = buf;
	for(row = 0; row < height; row++) {
	    ptr = start;
	    for(col = 0; col < width; col++, ptr++) {
	        *ptr = (*(unsigned int*)ptr * mult + 128) / 256;
	    }
	    start += pitch;
	}
	break;
      }

    case GGO_NATIVE:
      {
	int contour, point = 0, first_pt;
	FT_Outline *outline = &ft_face->glyph->outline;
	TTPOLYGONHEADER *pph;
	TTPOLYCURVE *ppc;
	DWORD pph_start, cpfx, type;

	if(buflen == 0) buf = NULL;

        for(contour = 0; contour < outline->n_contours; contour++) {
	    pph_start = needed;
	    pph = (TTPOLYGONHEADER *)((char *)buf + needed);
	    first_pt = point;
	    if(buf) {
	        pph->dwType = TT_POLYGON_TYPE;
		FTVectorToPOINTFX(&outline->points[point], &pph->pfxStart);
	    }
	    needed += sizeof(*pph);
	    point++;
	    while(point <= outline->contours[contour]) {
	        ppc = (TTPOLYCURVE *)((char *)buf + needed);
		type = (outline->tags[point] & FT_Curve_Tag_On) ?
		  TT_PRIM_LINE : TT_PRIM_QSPLINE;
		cpfx = 0;
		do {
		    if(buf)
		        FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
		    cpfx++;
		    point++;
		} while(point <= outline->contours[contour] &&
			(outline->tags[point] & FT_Curve_Tag_On) ==
			(outline->tags[point-1] & FT_Curve_Tag_On));
		/* At the end of a contour Windows adds the start point, but
		   only for Beziers */
		if(point > outline->contours[contour] &&
		   !(outline->tags[point-1] & FT_Curve_Tag_On)) {
		    if(buf)
		        FTVectorToPOINTFX(&outline->points[first_pt], &ppc->apfx[cpfx]);
		    cpfx++;
		} else if(point <= outline->contours[contour] &&
			  outline->tags[point] & FT_Curve_Tag_On) {
		  /* add closing pt for bezier */
		    if(buf)
		        FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
		    cpfx++;
		    point++;
		}
		if(buf) {
		    ppc->wType = type;
		    ppc->cpfx = cpfx;
		}
		needed += sizeof(*ppc) + (cpfx - 1) * sizeof(POINTFX);
	    }
	    if(buf)
	        pph->cb = needed - pph_start;
	}
	break;
      }
    case GGO_BEZIER:
      {
	/* Convert the quadratic Beziers to cubic Beziers.
	   The parametric eqn for a cubic Bezier is, from PLRM:
	   r(t) = at^3 + bt^2 + ct + r0
	   with the control points:
	   r1 = r0 + c/3
	   r2 = r1 + (c + b)/3
	   r3 = r0 + c + b + a

	   A quadratic Beizer has the form:
	   p(t) = (1-t)^2 p0 + 2(1-t)t p1 + t^2 p2

	   So equating powers of t leads to:
	   r1 = 2/3 p1 + 1/3 p0
	   r2 = 2/3 p1 + 1/3 p2
	   and of course r0 = p0, r3 = p2
	*/

	int contour, point = 0, first_pt;
	FT_Outline *outline = &ft_face->glyph->outline;
	TTPOLYGONHEADER *pph;
	TTPOLYCURVE *ppc;
	DWORD pph_start, cpfx, type;
	FT_Vector cubic_control[4];
	if(buflen == 0) buf = NULL;

        for(contour = 0; contour < outline->n_contours; contour++) {
	    pph_start = needed;
	    pph = (TTPOLYGONHEADER *)((char *)buf + needed);
	    first_pt = point;
	    if(buf) {
	        pph->dwType = TT_POLYGON_TYPE;
		FTVectorToPOINTFX(&outline->points[point], &pph->pfxStart);
	    }
	    needed += sizeof(*pph);
	    point++;
	    while(point <= outline->contours[contour]) {
	        ppc = (TTPOLYCURVE *)((char *)buf + needed);
		type = (outline->tags[point] & FT_Curve_Tag_On) ?
		  TT_PRIM_LINE : TT_PRIM_CSPLINE;
		cpfx = 0;
		do {
		    if(type == TT_PRIM_LINE) {
		        if(buf)
			    FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
			cpfx++;
			point++;
		    } else {
		      /* Unlike QSPLINEs, CSPLINEs always have their endpoint
			 so cpfx = 3n */

		      /* FIXME: Possible optimization in endpoint calculation
			 if there are two consecutive curves */
		        cubic_control[0] = outline->points[point-1];
		        if(!(outline->tags[point-1] & FT_Curve_Tag_On)) {
			    cubic_control[0].x += outline->points[point].x + 1;
			    cubic_control[0].y += outline->points[point].y + 1;
			    cubic_control[0].x >>= 1;
			    cubic_control[0].y >>= 1;
			}
			if(point+1 > outline->contours[contour])
 			    cubic_control[3] = outline->points[first_pt];
			else {
			    cubic_control[3] = outline->points[point+1];
			    if(!(outline->tags[point+1] & FT_Curve_Tag_On)) {
			        cubic_control[3].x += outline->points[point].x + 1;
				cubic_control[3].y += outline->points[point].y + 1;
				cubic_control[3].x >>= 1;
				cubic_control[3].y >>= 1;
			    }
			}
			/* r1 = 1/3 p0 + 2/3 p1
			   r2 = 1/3 p2 + 2/3 p1 */
		        cubic_control[1].x = (2 * outline->points[point].x + 1) / 3;
			cubic_control[1].y = (2 * outline->points[point].y + 1) / 3;
			cubic_control[2] = cubic_control[1];
			cubic_control[1].x += (cubic_control[0].x + 1) / 3;
			cubic_control[1].y += (cubic_control[0].y + 1) / 3;
			cubic_control[2].x += (cubic_control[3].x + 1) / 3;
			cubic_control[2].y += (cubic_control[3].y + 1) / 3;
		        if(buf) {
			    FTVectorToPOINTFX(&cubic_control[1], &ppc->apfx[cpfx]);
			    FTVectorToPOINTFX(&cubic_control[2], &ppc->apfx[cpfx+1]);
			    FTVectorToPOINTFX(&cubic_control[3], &ppc->apfx[cpfx+2]);
			}
			cpfx += 3;
			point++;
		    }
		} while(point <= outline->contours[contour] &&
			(outline->tags[point] & FT_Curve_Tag_On) ==
			(outline->tags[point-1] & FT_Curve_Tag_On));
		/* At the end of a contour Windows adds the start point,
		   but only for Beziers and we've already done that.
		*/
		if(point <= outline->contours[contour] &&
		   outline->tags[point] & FT_Curve_Tag_On) {
		  /* This is the closing pt of a bezier, but we've already
		     added it, so just inc point and carry on */
		    point++;
		}
		if(buf) {
		    ppc->wType = type;
		    ppc->cpfx = cpfx;
		}
		needed += sizeof(*ppc) + (cpfx - 1) * sizeof(POINTFX);
	    }
	    if(buf)
	        pph->cb = needed - pph_start;
	}
	break;
      }

    default:
        FIXME("Unsupported format %d\n", format);
	return GDI_ERROR;
    }
    return needed;
}

/*************************************************************
 * WineEngGetTextMetrics
 *
 */
BOOL WineEngGetTextMetrics(GdiFont font, LPTEXTMETRICW ptm)
{
    if(!font->potm) {
        if(!WineEngGetOutlineTextMetrics(font, 0, NULL))
	    return FALSE;
    }
    if(!font->potm) return FALSE;
    memcpy(ptm, &font->potm->otmTextMetrics, sizeof(*ptm));
    return TRUE;
}


/*************************************************************
 * WineEngGetOutlineTextMetrics
 *
 */
UINT WineEngGetOutlineTextMetrics(GdiFont font, UINT cbSize,
				  OUTLINETEXTMETRICW *potm)
{
    FT_Face ft_face = font->ft_face;
    UINT needed, lenfam, lensty, ret;
    TT_OS2 *pOS2;
    TT_HoriHeader *pHori;
    TT_Postscript *pPost;
    FT_Fixed x_scale, y_scale;
    WCHAR *family_nameW, *style_nameW;
    WCHAR spaceW[] = {' ', '\0'};
    char *cp;

    TRACE("font=%p\n", font);

    if(font->potm) {
        if(cbSize >= font->potm->otmSize)
	    memcpy(potm, font->potm, font->potm->otmSize);
	return font->potm->otmSize;
    }

    needed = sizeof(*potm);

    lenfam = MultiByteToWideChar(CP_ACP, 0, ft_face->family_name, -1, NULL, 0)
      * sizeof(WCHAR);
    family_nameW = HeapAlloc(GetProcessHeap(), 0, lenfam);
    MultiByteToWideChar(CP_ACP, 0, ft_face->family_name, -1,
			family_nameW, lenfam);

    lensty = MultiByteToWideChar(CP_ACP, 0, ft_face->style_name, -1, NULL, 0)
      * sizeof(WCHAR);
    style_nameW = HeapAlloc(GetProcessHeap(), 0, lensty);
    MultiByteToWideChar(CP_ACP, 0, ft_face->style_name, -1,
			style_nameW, lensty);

    /* These names should be read from the TT name table */

    /* length of otmpFamilyName */
    needed += lenfam;

    /* length of otmpFaceName */
    if(!strcasecmp(ft_face->style_name, "regular")) {
      needed += lenfam; /* just the family name */
    } else {
      needed += lenfam + lensty; /* family + " " + style */
    }

    /* length of otmpStyleName */
    needed += lensty;

    /* length of otmpFullName */
    needed += lenfam + lensty;


    x_scale = ft_face->size->metrics.x_scale;
    y_scale = ft_face->size->metrics.y_scale;

    pOS2 = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_os2);
    if(!pOS2) {
        FIXME("Can't find OS/2 table - not TT font?\n");
	ret = 0;
	goto end;
    }

    pHori = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_hhea);
    if(!pHori) {
        FIXME("Can't find HHEA table - not TT font?\n");
	ret = 0;
	goto end;
    }

    pPost = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_post); /* we can live with this failing */

    TRACE("OS/2 winA = %d winD = %d typoA = %d typoD = %d typoLG = %d FT_Face a = %d, d = %d, h = %d: HORZ a = %d, d = %d lg = %d maxY = %ld minY = %ld\n",
	  pOS2->usWinAscent, pOS2->usWinDescent,
	  pOS2->sTypoAscender, pOS2->sTypoDescender, pOS2->sTypoLineGap,
	  ft_face->ascender, ft_face->descender, ft_face->height,
	  pHori->Ascender, pHori->Descender, pHori->Line_Gap,
	  ft_face->bbox.yMax, ft_face->bbox.yMin);

    font->potm = HeapAlloc(GetProcessHeap(), 0, needed);
    font->potm->otmSize = needed;

#define TM font->potm->otmTextMetrics

    if(font->yMax) {
	TM.tmAscent = font->yMax;
	TM.tmDescent = -font->yMin;
	TM.tmInternalLeading = (TM.tmAscent + TM.tmDescent) - ft_face->size->metrics.y_ppem;
    } else {
	TM.tmAscent = (pFT_MulFix(pOS2->usWinAscent, y_scale) + 32) >> 6;
	TM.tmDescent = (pFT_MulFix(pOS2->usWinDescent, y_scale) + 32) >> 6;
	TM.tmInternalLeading = (pFT_MulFix(pOS2->usWinAscent + pOS2->usWinDescent
					    - ft_face->units_per_EM, y_scale) + 32) >> 6;
    }

    TM.tmHeight = TM.tmAscent + TM.tmDescent;

    /* MSDN says:
     el = MAX(0, LineGap - ((WinAscent + WinDescent) - (Ascender - Descender)))
    */
    TM.tmExternalLeading = max(0, (pFT_MulFix(pHori->Line_Gap -
       		 ((pOS2->usWinAscent + pOS2->usWinDescent) -
		  (pHori->Ascender - pHori->Descender)), y_scale) + 32) >> 6);

    TM.tmAveCharWidth = (pFT_MulFix(pOS2->xAvgCharWidth, x_scale) + 32) >> 6;
    TM.tmMaxCharWidth = (pFT_MulFix(ft_face->bbox.xMax - ft_face->bbox.xMin, x_scale) + 32) >> 6;
    TM.tmWeight = font->fake_bold ? FW_BOLD : pOS2->usWeightClass;
    TM.tmOverhang = 0;
    TM.tmDigitizedAspectX = 300;
    TM.tmDigitizedAspectY = 300;
    TM.tmFirstChar = pOS2->usFirstCharIndex;
    TM.tmLastChar = pOS2->usLastCharIndex;
    TM.tmDefaultChar = pOS2->usDefaultChar;
    TM.tmBreakChar = pOS2->usBreakChar ? pOS2->usBreakChar : ' ';
    TM.tmItalic = font->fake_italic ? 255 : ((ft_face->style_flags & FT_STYLE_FLAG_ITALIC) ? 255 : 0);
    TM.tmUnderlined = 0; /* entry in OS2 table */
    TM.tmStruckOut = 0; /* entry in OS2 table */

    /* Yes TPMF_FIXED_PITCH is correct; braindead api */
    if(!FT_IS_FIXED_WIDTH(ft_face))
        TM.tmPitchAndFamily = TMPF_FIXED_PITCH;
    else
        TM.tmPitchAndFamily = 0;

    switch(pOS2->panose[PAN_FAMILYTYPE_INDEX]) {
    case PAN_FAMILY_SCRIPT:
        TM.tmPitchAndFamily |= FF_SCRIPT;
	break;
    case PAN_FAMILY_DECORATIVE:
    case PAN_FAMILY_PICTORIAL:
        TM.tmPitchAndFamily |= FF_DECORATIVE;
	break;
    case PAN_FAMILY_TEXT_DISPLAY:
        if(TM.tmPitchAndFamily == 0) /* fixed */
	    TM.tmPitchAndFamily = FF_MODERN;
	else {
	    switch(pOS2->panose[PAN_SERIFSTYLE_INDEX]) {
	    case PAN_SERIF_NORMAL_SANS:
	    case PAN_SERIF_OBTUSE_SANS:
	    case PAN_SERIF_PERP_SANS:
	        TM.tmPitchAndFamily |= FF_SWISS;
		break;
	    default:
	        TM.tmPitchAndFamily |= FF_ROMAN;
	    }
	}
	break;
    default:
        TM.tmPitchAndFamily |= FF_DONTCARE;
    }

    if(FT_IS_SCALABLE(ft_face))
        TM.tmPitchAndFamily |= TMPF_VECTOR;
    if(FT_IS_SFNT(ft_face))
        TM.tmPitchAndFamily |= TMPF_TRUETYPE;

    TM.tmCharSet = font->charset;
#undef TM

    font->potm->otmFiller = 0;
    memcpy(&font->potm->otmPanoseNumber, pOS2->panose, PANOSE_COUNT);
    font->potm->otmfsSelection = pOS2->fsSelection;
    font->potm->otmfsType = pOS2->fsType;
    font->potm->otmsCharSlopeRise = pHori->caret_Slope_Rise;
    font->potm->otmsCharSlopeRun = pHori->caret_Slope_Run;
    font->potm->otmItalicAngle = 0; /* POST table */
    font->potm->otmEMSquare = ft_face->units_per_EM;
    font->potm->otmAscent = (pFT_MulFix(pOS2->sTypoAscender, y_scale) + 32) >> 6;
    font->potm->otmDescent = (pFT_MulFix(pOS2->sTypoDescender, y_scale) + 32) >> 6;
    font->potm->otmLineGap = (pFT_MulFix(pOS2->sTypoLineGap, y_scale) + 32) >> 6;
    font->potm->otmsCapEmHeight = (pFT_MulFix(pOS2->sCapHeight, y_scale) + 32) >> 6;
    font->potm->otmsXHeight = (pFT_MulFix(pOS2->sxHeight, y_scale) + 32) >> 6;
    font->potm->otmrcFontBox.left = ft_face->bbox.xMin;
    font->potm->otmrcFontBox.right = ft_face->bbox.xMax;
    font->potm->otmrcFontBox.top = ft_face->bbox.yMin;
    font->potm->otmrcFontBox.bottom = ft_face->bbox.yMax;
    font->potm->otmMacAscent = 0; /* where do these come from ? */
    font->potm->otmMacDescent = 0;
    font->potm->otmMacLineGap = 0;
    font->potm->otmusMinimumPPEM = 0; /* TT Header */
    font->potm->otmptSubscriptSize.x = (pFT_MulFix(pOS2->ySubscriptXSize, x_scale) + 32) >> 6;
    font->potm->otmptSubscriptSize.y = (pFT_MulFix(pOS2->ySubscriptYSize, y_scale) + 32) >> 6;
    font->potm->otmptSubscriptOffset.x = (pFT_MulFix(pOS2->ySubscriptXOffset, x_scale) + 32) >> 6;
    font->potm->otmptSubscriptOffset.y = (pFT_MulFix(pOS2->ySubscriptYOffset, y_scale) + 32) >> 6;
    font->potm->otmptSuperscriptSize.x = (pFT_MulFix(pOS2->ySuperscriptXSize, x_scale) + 32) >> 6;
    font->potm->otmptSuperscriptSize.y = (pFT_MulFix(pOS2->ySuperscriptYSize, y_scale) + 32) >> 6;
    font->potm->otmptSuperscriptOffset.x = (pFT_MulFix(pOS2->ySuperscriptXOffset, x_scale) + 32) >> 6;
    font->potm->otmptSuperscriptOffset.y = (pFT_MulFix(pOS2->ySuperscriptYOffset, y_scale) + 32) >> 6;
    font->potm->otmsStrikeoutSize = (pFT_MulFix(pOS2->yStrikeoutSize, y_scale) + 32) >> 6;
    font->potm->otmsStrikeoutPosition = (pFT_MulFix(pOS2->yStrikeoutPosition, y_scale) + 32) >> 6;
    if(!pPost) {
        font->potm->otmsUnderscoreSize = 0;
	font->potm->otmsUnderscorePosition = 0;
    } else {
        font->potm->otmsUnderscoreSize = (pFT_MulFix(pPost->underlineThickness, y_scale) + 32) >> 6;
	font->potm->otmsUnderscorePosition = (pFT_MulFix(pPost->underlinePosition, y_scale) + 32) >> 6;
    }

    /* otmp* members should clearly have type ptrdiff_t, but M$ knows best */
    cp = (char*)font->potm + sizeof(*font->potm);
    font->potm->otmpFamilyName = (LPSTR)(cp - (char*)font->potm);
    strcpyW((WCHAR*)cp, family_nameW);
    cp += lenfam;
    font->potm->otmpStyleName = (LPSTR)(cp - (char*)font->potm);
    strcpyW((WCHAR*)cp, style_nameW);
    cp += lensty;
    font->potm->otmpFaceName = (LPSTR)(cp - (char*)font->potm);
    strcpyW((WCHAR*)cp, family_nameW);
    if(strcasecmp(ft_face->style_name, "regular")) {
        strcatW((WCHAR*)cp, spaceW);
	strcatW((WCHAR*)cp, style_nameW);
	cp += lenfam + lensty;
    } else
        cp += lenfam;
    font->potm->otmpFullName = (LPSTR)(cp - (char*)font->potm);
    strcpyW((WCHAR*)cp, family_nameW);
    strcatW((WCHAR*)cp, spaceW);
    strcatW((WCHAR*)cp, style_nameW);
    ret = needed;

    if(needed <= cbSize)
        memcpy(potm, font->potm, font->potm->otmSize);

end:
    HeapFree(GetProcessHeap(), 0, style_nameW);
    HeapFree(GetProcessHeap(), 0, family_nameW);

    return ret;
}


/*************************************************************
 * WineEngGetCharWidth
 *
 */
BOOL WineEngGetCharWidth(GdiFont font, UINT firstChar, UINT lastChar,
			 LPINT buffer)
{
    UINT c;
    GLYPHMETRICS gm;
    FT_UInt glyph_index;

    TRACE("%p, %d, %d, %p\n", font, firstChar, lastChar, buffer);

    for(c = firstChar; c <= lastChar; c++) {
        WineEngGetGlyphOutline(font, c, GGO_METRICS, &gm, 0, NULL, NULL);
	glyph_index = get_glyph_index(font, c);
	buffer[c - firstChar] = font->gm[glyph_index].adv;
    }
    return TRUE;
}

/*************************************************************
 * WineEngGetTextExtentPoint
 *
 */
BOOL WineEngGetTextExtentPoint(GdiFont font, LPCWSTR wstr, INT count,
			       LPSIZE size)
{
    INT idx;
    GLYPHMETRICS gm;
    TEXTMETRICW tm;
    FT_UInt glyph_index;

    TRACE("%p, %s, %d, %p\n", font, debugstr_wn(wstr, count), count,
	  size);

    size->cx = 0;
    WineEngGetTextMetrics(font, &tm);
    size->cy = tm.tmHeight;

   for(idx = 0; idx < count; idx++) {
        WineEngGetGlyphOutline(font, wstr[idx], GGO_METRICS, &gm, 0, NULL,
			       NULL);
	glyph_index = get_glyph_index(font, wstr[idx]);
	size->cx += font->gm[glyph_index].adv;
    }
    TRACE("return %ld,%ld\n", size->cx, size->cy);
    return TRUE;
}

/*************************************************************
 * WineEngGetTextExtentPointI
 *
 */
BOOL WineEngGetTextExtentPointI(GdiFont font, const WORD *indices, INT count,
				LPSIZE size)
{
    INT idx;
    GLYPHMETRICS gm;
    TEXTMETRICW tm;

    TRACE("%p, %p, %d, %p\n", font, indices, count, size);

    size->cx = 0;
    WineEngGetTextMetrics(font, &tm);
    size->cy = tm.tmHeight;

   for(idx = 0; idx < count; idx++) {
        WineEngGetGlyphOutline(font, indices[idx],
			       GGO_METRICS | GGO_GLYPH_INDEX, &gm, 0, NULL,
			       NULL);
	size->cx += font->gm[indices[idx]].adv;
    }
    TRACE("return %ld,%ld\n", size->cx, size->cy);
    return TRUE;
}

/*************************************************************
 * WineEngGetFontData
 *
 */
DWORD WineEngGetFontData(GdiFont font, DWORD table, DWORD offset, LPVOID buf,
			 DWORD cbData)
{
    FT_Face ft_face = font->ft_face;
    TT_Face tt_face;
    SFNT_Interface *sfnt;
    DWORD len;
    FT_Error err;

    TRACE("font=%p, table=%08lx, offset=%08lx, buf=%p, cbData=%lx\n",
	font, table, offset, buf, cbData);

    if(!FT_IS_SFNT(ft_face))
        return GDI_ERROR;

    tt_face = (TT_Face) ft_face;
    if (FT_Version.major==2 && FT_Version.minor==0)
    {
        /* 2.0.x */
        sfnt = *(SFNT_Interface**)((char*)tt_face + 528);
    }
    else
    {
        /* A field was added in the middle of the structure in 2.1.x */
        sfnt = *(SFNT_Interface**)((char*)tt_face + 532);
    }

    if(!buf || !cbData)
        len = 0;
    else
        len = cbData;

    if(table) { /* MS tags differ in endidness from FT ones */
        table = table >> 24 | table << 24 |
	  (table >> 8 & 0xff00) | (table << 8 & 0xff0000);
    }

    err = sfnt->load_any(tt_face, table, offset, buf, &len);
    if(err) {
        TRACE("Can't find table %08lx.\n", table);
	return GDI_ERROR;
    }
    return len;
}

/*************************************************************
 * WineEngGetTextFace
 *
 */
INT WineEngGetTextFace(GdiFont font, INT count, LPWSTR str)
{
    if(str) {
        lstrcpynW(str, font->name, count);
	return strlenW(font->name);
    } else
        return strlenW(font->name) + 1;
}


#else /* HAVE_FREETYPE */

BOOL WineEngInit(void)
{
    return FALSE;
}
GdiFont WineEngCreateFontInstance(DC *dc, HFONT hfont)
{
    return NULL;
}
BOOL WineEngDestroyFontInstance(HFONT hfont)
{
    return FALSE;
}

DWORD WineEngEnumFonts(LPLOGFONTW plf, DEVICEFONTENUMPROC proc, LPARAM lparam)
{
    return 1;
}

DWORD WineEngGetGlyphIndices(GdiFont font, LPCWSTR lpstr, INT count,
				LPWORD pgi, DWORD flags)
{
    return GDI_ERROR;
}

DWORD WineEngGetGlyphOutline(GdiFont font, UINT glyph, UINT format,
			     LPGLYPHMETRICS lpgm, DWORD buflen, LPVOID buf,
			     const MAT2* lpmat)
{
    ERR("called but we don't have FreeType\n");
    return GDI_ERROR;
}

BOOL WineEngGetTextMetrics(GdiFont font, LPTEXTMETRICW ptm)
{
    ERR("called but we don't have FreeType\n");
    return FALSE;
}

UINT WineEngGetOutlineTextMetrics(GdiFont font, UINT cbSize,
				  OUTLINETEXTMETRICW *potm)
{
    ERR("called but we don't have FreeType\n");
    return 0;
}

BOOL WineEngGetCharWidth(GdiFont font, UINT firstChar, UINT lastChar,
			 LPINT buffer)
{
    ERR("called but we don't have FreeType\n");
    return FALSE;
}

BOOL WineEngGetTextExtentPoint(GdiFont font, LPCWSTR wstr, INT count,
			       LPSIZE size)
{
    ERR("called but we don't have FreeType\n");
    return FALSE;
}

BOOL WineEngGetTextExtentPointI(GdiFont font, const WORD *indices, INT count,
				LPSIZE size)
{
    ERR("called but we don't have FreeType\n");
    return FALSE;
}

DWORD WineEngGetFontData(GdiFont font, DWORD table, DWORD offset, LPVOID buf,
			 DWORD cbData)
{
    ERR("called but we don't have FreeType\n");
    return GDI_ERROR;
}

INT WineEngGetTextFace(GdiFont font, INT count, LPWSTR str)
{
    ERR("called but we don't have FreeType\n");
    return 0;
}

INT WineEngAddFontResourceEx(LPCWSTR file, DWORD flags, PVOID pdv)
{
    FIXME(":stub\n");
    return 1;
}

INT WineEngRemoveFontResourceEx(LPCWSTR file, DWORD flags, PVOID pdv)
{
    FIXME(":stub\n");
    return TRUE;
}
#endif /* HAVE_FREETYPE */
