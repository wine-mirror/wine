/*
 * X11 physical font objects
 *
 * Copyright 1997 Alex Korobka
 *
 * TODO: Mapping algorithm tweaks, FO_SYNTH_... flags (ExtTextOut() will
 *	 have to be changed for that), dynamic font loading (FreeType).
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING
#include <X11/Xatom.h>
#include "ts_xlib.h"
#include "x11font.h"
#endif /* !defined(X_DISPLAY_MISSING) */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <assert.h>
#include "winuser.h"
#include "heap.h"
#include "options.h"
#include "font.h"
#include "debugtools.h"
#include "ldt.h"
#include "tweak.h"

DEFAULT_DEBUG_CHANNEL(font)

#ifndef X_DISPLAY_MISSING

#define X_PFONT_MAGIC		(0xFADE0000)
#define X_FMC_MAGIC		(0x0000CAFE)

#define MAX_FONTS	        1024*16
#define MAX_LFD_LENGTH		256
#define TILDE                   '~'
#define HYPHEN                  '-'

#define DEF_POINT_SIZE          8      /* CreateFont(0 .. ) gets this */
#define DEF_SCALABLE_HEIGHT	100    /* pixels */
#define MIN_FONT_SIZE           2      /* Min size in pixels */
#define MAX_FONT_SIZE           1000   /* Max size in pixels */

#define REMOVE_SUBSETS		1
#define UNMARK_SUBSETS		0


#define FF_FAMILY       (FF_MODERN | FF_SWISS | FF_ROMAN | FF_DECORATIVE | FF_SCRIPT)

typedef struct __fontAlias
{
  LPSTR			faTypeFace;
  LPSTR			faAlias;
  struct __fontAlias*	next;
} fontAlias;

static fontAlias *aliasTable = NULL;

UINT16			XTextCaps = TC_OP_CHARACTER | TC_OP_STROKE |
TC_CP_STROKE | TC_CR_ANY |
				    TC_SA_DOUBLE | TC_SA_INTEGER | TC_SA_CONTIN |
			 	    TC_UA_ABLE | TC_SO_ABLE | TC_RA_ABLE;

			/* X11R6 adds TC_SF_X_YINDEP, maybe more... */

static const char*	INIWinePrefix = "/.wine";
static const char*	INIFontMetrics = "/cachedmetrics.";
static const char*	INIFontSection = "fonts";
static const char*	INIAliasSection = "Alias";
static const char*	INIIgnoreSection = "Ignore";
static const char*	INIDefault = "Default";
static const char*	INIDefaultFixed = "DefaultFixed";
static const char*	INIResolution = "Resolution";
static const char*	INIGlobalMetrics = "FontMetrics";
static const char*	INIDefaultSerif = "DefaultSerif";
static const char*	INIDefaultSansSerif = "DefaultSansSerif";


/* FIXME - are there any more Latin charsets ? */
#define IS_LATIN_CHARSET(ch) \
  ((ch)==ANSI_CHARSET ||\
   (ch)==EE_CHARSET ||\
   (ch)==ISO3_CHARSET ||\
   (ch)==ISO4_CHARSET ||\
   (ch)==RUSSIAN_CHARSET ||\
   (ch)==ARABIC_CHARSET ||\
   (ch)==GREEK_CHARSET ||\
   (ch)==HEBREW_CHARSET ||\
   (ch)==TURKISH_CHARSET ||\
   (ch)==BALTIC_CHARSET)

/* suffix-charset mapping tables - must be less than 254 entries long */

typedef struct __sufch
{
  LPSTR		psuffix;
  BYTE		charset;
} SuffixCharset;

static SuffixCharset sufch_ansi[] = {
    {  "0", ANSI_CHARSET },
    { NULL, ANSI_CHARSET }};

static SuffixCharset sufch_iso646[] = {
    { "irv", ANSI_CHARSET },
    { NULL, SYMBOL_CHARSET }};

static SuffixCharset sufch_iso8859[] = {
    {  "1", ANSI_CHARSET },
    {  "2", EE_CHARSET },
    {  "3", ISO3_CHARSET }, 
    {  "4", ISO4_CHARSET }, 
    {  "5", RUSSIAN_CHARSET },
    {  "6", ARABIC_CHARSET },
    {  "7", GREEK_CHARSET },
    {  "8", HEBREW_CHARSET }, 
    {  "9", TURKISH_CHARSET },
    { "10", BALTIC_CHARSET },
    { "11", THAI_CHARSET },
    { "12", SYMBOL_CHARSET },
    { "13", SYMBOL_CHARSET },
    { "14", SYMBOL_CHARSET },
    { "15", ANSI_CHARSET },
    { NULL, SYMBOL_CHARSET }};

static SuffixCharset sufch_microsoft[] = {
    { "cp1250", EE_CHARSET },
    { "cp1251", RUSSIAN_CHARSET },
    { "cp1252", ANSI_CHARSET },
    { "cp1253", GREEK_CHARSET },
    { "cp1254", TURKISH_CHARSET },
    { "cp1255", HEBREW_CHARSET },
    { "cp1256", ARABIC_CHARSET },
    { "cp1257", BALTIC_CHARSET },
    { "fontspecific", SYMBOL_CHARSET },
    { "symbol", SYMBOL_CHARSET },
    {   NULL,   SYMBOL_CHARSET }};

static SuffixCharset sufch_tcvn[] = {
    {  "0", TCVN_CHARSET },
    { NULL, TCVN_CHARSET }};

static SuffixCharset sufch_tis620[] = {
    {  "0", THAI_CHARSET },
    { NULL, THAI_CHARSET }};

static SuffixCharset sufch_viscii[] = {
    {  "1", VISCII_CHARSET },
    { NULL, VISCII_CHARSET }};

static SuffixCharset sufch_windows[] = {
    { "1250", EE_CHARSET },
    { "1251", RUSSIAN_CHARSET },
    { "1252", ANSI_CHARSET },
    { "1253", GREEK_CHARSET },
    { "1254", TURKISH_CHARSET }, 
    { "1255", HEBREW_CHARSET }, 
    { "1256", ARABIC_CHARSET },
    { "1257", BALTIC_CHARSET },
    {  NULL,  BALTIC_CHARSET }}; /* CHECK/FIXME is BALTIC really the default ? */

static SuffixCharset sufch_koi8[] = {
    { "r", RUSSIAN_CHARSET },
    { NULL, RUSSIAN_CHARSET }};

/* Each of these must be matched explicitly */
static SuffixCharset sufch_any[] = {
    { "fontspecific", SYMBOL_CHARSET },
    { NULL, 0 }};


typedef struct __fet
{
  LPSTR		 prefix;
  SuffixCharset* sufch;
  struct __fet*  next;
} fontEncodingTemplate;

/* Note: we can attach additional encoding mappings to the end
 *       of this table at runtime.
 */
static fontEncodingTemplate __fETTable[] = {
			{ "ansi",         sufch_ansi,         &__fETTable[1] },
			{ "ascii",        sufch_ansi,         &__fETTable[2] },
			{ "iso646.1991",  sufch_iso646,       &__fETTable[3] },
			{ "iso8859",      sufch_iso8859,      &__fETTable[4] },
			{ "microsoft",    sufch_microsoft,    &__fETTable[5] },
			{ "tcvn",         sufch_tcvn,         &__fETTable[6] },
			{ "tis620.2533",  sufch_tis620,       &__fETTable[7] },
			{ "viscii1.1",    sufch_viscii,       &__fETTable[8] },
			{ "windows",      sufch_windows,      &__fETTable[9] },
			{ "koi8",         sufch_koi8,         &__fETTable[10] },
			/* NULL prefix matches anything so put it last */
			{   NULL,         sufch_any,          NULL },
};
static fontEncodingTemplate* fETTable = __fETTable;

static int		DefResolution = 0;

static CRITICAL_SECTION crtsc_fonts_X11;

static fontResource*	fontList = NULL;
static fontObject*      fontCache = NULL;		/* array */
static int		fontCacheSize = FONTCACHE;
static int		fontLF = -1, fontMRU = -1;	/* last free, most recently used */

#define __PFONT(pFont)     ( fontCache + ((UINT)(pFont) & 0x0000FFFF) )
#define CHECK_PFONT(pFont) ( (((UINT)(pFont) & 0xFFFF0000) == X_PFONT_MAGIC) &&\
			     (((UINT)(pFont) & 0x0000FFFF) < fontCacheSize) )

static Atom RAW_ASCENT;
static Atom RAW_DESCENT;

/***********************************************************************
 *           Helper macros from X distribution
 */

#define CI_NONEXISTCHAR(cs) (((cs)->width == 0) && \
			     (((cs)->rbearing|(cs)->lbearing| \
			       (cs)->ascent|(cs)->descent) == 0))

#define CI_GET_CHAR_INFO(fs,col,def,cs) \
{ \
    cs = def; \
    if (col >= fs->min_char_or_byte2 && col <= fs->max_char_or_byte2) { \
	if (fs->per_char == NULL) { \
	    cs = &fs->min_bounds; \
	} else { \
	    cs = &fs->per_char[(col - fs->min_char_or_byte2)]; \
	    if (CI_NONEXISTCHAR(cs)) cs = def; \
	} \
    } \
}

#define CI_GET_DEFAULT_INFO(fs,cs) \
  CI_GET_CHAR_INFO(fs, fs->default_char, NULL, cs)

/***********************************************************************
 *           Checksums
 */
static UINT16   __lfCheckSum( LPLOGFONT16 plf )
{
    CHAR        font[LF_FACESIZE];
    UINT16      checksum = 0;
    UINT16      i;

#define ptr ((UINT16*)plf)
   for( i = 0; i < 9; i++ ) checksum ^= *ptr++;
#undef ptr
   i = 0;
#define ptr ((CHAR*)plf)
   do { font[i++] = tolower(*ptr++); } while (( i < LF_FACESIZE) && (*ptr) && (*ptr!=' '));
   for( ptr = font, i >>= 1; i > 0; i-- ) 
#undef ptr
#define ptr ((UINT16*)plf)
        checksum ^= *ptr++;
#undef ptr
   return checksum;
}

static UINT16   __genericCheckSum( const void *ptr, int size )
{
   unsigned int checksum = 0;
   const char *p = (const char *)ptr;
   while (size-- > 0)
     checksum ^= (checksum << 3) + (checksum >> 29) + *p++;

   return checksum & 0xffff;
}

/*************************************************************************
 *           LFD parse/compose routines
 *
 * NB. LFD_Parse will use lpFont for its own ends, so if you want to keep it
 *     make a copy first
 *
 * These functions also do TILDE to HYPHEN conversion
 */
static LFD* LFD_Parse(LPSTR lpFont)
{
    LFD* lfd;
    char *lpch = lpFont, *lfd_fld[LFD_FIELDS], *field_start;
    int i;
    if (*lpch != HYPHEN)
    {
        WARN("font '%s' doesn't begin with '%c'\n", lpFont, HYPHEN);
	return NULL;
    }

    field_start = ++lpch;
    for( i = 0; i < LFD_FIELDS; )
    {
	if (*lpch == HYPHEN)
	{
	    *lpch = '\0';
	    lfd_fld[i] = field_start;
	    i++;
	    field_start = ++lpch;
	}
	else if (!*lpch)
	{
	    lfd_fld[i] = field_start;
	    i++;
	    break;
	}
	else if (*lpch == TILDE)
	{
	    *lpch = HYPHEN;
	    ++lpch;
	}
	else
	    ++lpch;
    }
    /* Fill in the empty fields with NULLS */
    for (; i< LFD_FIELDS; i++)
	lfd_fld[i] = NULL;
    if (*lpch)
	WARN("Extra ignored in font '%s'\n", lpFont);
    
    lfd = HeapAlloc( SystemHeap, 0, sizeof(LFD) );
    if (lfd)
    {
	lfd->foundry = lfd_fld[0];
	lfd->family = lfd_fld[1];
	lfd->weight = lfd_fld[2];
	lfd->slant = lfd_fld[3];
	lfd->set_width = lfd_fld[4];
	lfd->add_style = lfd_fld[5];
	lfd->pixel_size = lfd_fld[6];
	lfd->point_size = lfd_fld[7];
	lfd->resolution_x = lfd_fld[8];
	lfd->resolution_y = lfd_fld[9];
	lfd->spacing = lfd_fld[10];
	lfd->average_width = lfd_fld[11];
	lfd->charset_registry = lfd_fld[12];
	lfd->charset_encoding = lfd_fld[13];
    }
    return lfd;
}


static void LFD_UnParse(LPSTR dp, UINT buf_size, LFD* lfd)
{
    char* lfd_fld[LFD_FIELDS];
    int i;

    if (!buf_size)
	return; /* Dont be silly */

    lfd_fld[0]  = lfd->foundry;
    lfd_fld[1]	= lfd->family;
    lfd_fld[2]	= lfd->weight ;
    lfd_fld[3]	= lfd->slant ;
    lfd_fld[4]  = lfd->set_width ;
    lfd_fld[5]	= lfd->add_style;
    lfd_fld[6]	= lfd->pixel_size;
    lfd_fld[7]	= lfd->point_size;
    lfd_fld[8]	= lfd->resolution_x;
    lfd_fld[9]	= lfd->resolution_y ;
    lfd_fld[10] = lfd->spacing ;
    lfd_fld[11] = lfd->average_width ;
    lfd_fld[12] = lfd->charset_registry ;
    lfd_fld[13] = lfd->charset_encoding ;

    buf_size--; /* Room for the terminator */

    for (i = 0; i < LFD_FIELDS; i++)
    {
	char* sp = lfd_fld[i];
	if (!sp || !buf_size)
	    break;
	
	*dp++ = HYPHEN;
	buf_size--;
	while (buf_size > 0 && *sp)
	{
	    *dp = (*sp == HYPHEN) ? TILDE : *sp;
	    buf_size--;
	    dp++; sp++;
	}
    }
    *dp = '\0';
}


static void LFD_GetWeight( fontInfo* fi, LPCSTR lpStr)
{
    int j = lstrlenA(lpStr);
    if( j == 1 && *lpStr == '0') 
	fi->fi_flags |= FI_POLYWEIGHT;
    else if( j == 4 )
    {
	if( !strcasecmp( "bold", lpStr) )
	    fi->df.dfWeight = FW_BOLD; 
	else if( !strcasecmp( "demi", lpStr) )
	{
	    fi->fi_flags |= FI_FW_DEMI;
	    fi->df.dfWeight = FW_DEMIBOLD;
	}
	else if( !strcasecmp( "book", lpStr) )
	{
	    fi->fi_flags |= FI_FW_BOOK;
	    fi->df.dfWeight = FW_REGULAR;
	}
    }
    else if( j == 5 )
    {
	if( !strcasecmp( "light", lpStr) )
	    fi->df.dfWeight = FW_LIGHT;
	else if( !strcasecmp( "black", lpStr) )
	    fi->df.dfWeight = FW_BLACK;
    }
    else if( j == 6 && !strcasecmp( "medium", lpStr) )
	fi->df.dfWeight = FW_REGULAR; 
    else if( j == 8 && !strcasecmp( "demibold", lpStr) )
	fi->df.dfWeight = FW_DEMIBOLD; 
    else
	fi->df.dfWeight = FW_DONTCARE; /* FIXME: try to get something
					* from the weight property */
}

static BOOL LFD_GetSlant( fontInfo* fi, LPCSTR lpStr)
{
    int l = lstrlenA(lpStr);
    if( l == 1 )
    {
	switch( tolower( *lpStr ) )
	{
	    case '0':  fi->fi_flags |= FI_POLYSLANT;	/* haven't seen this one yet */
	    default:
	    case 'r':  fi->df.dfItalic = 0;
		       break;
	    case 'o':
		       fi->fi_flags |= FI_OBLIQUE;
	    case 'i':  fi->df.dfItalic = 1;
		       break;
	}
	return FALSE;
    }
    return TRUE;
}

static void LFD_GetStyle( fontInfo* fi, LPCSTR lpstr, int dec_style_check)
{
    int j = lstrlenA(lpstr);
    if( j > 3 )	/* find out is there "sans" or "script" */
    {
	j = 0;
	
	if( strstr(lpstr, "sans") ) 
	{ 
	    fi->df.dfPitchAndFamily |= FF_SWISS; 
	    j = 1; 
	}
	if( strstr(lpstr, "script") ) 
	{ 
	    fi->df.dfPitchAndFamily |= FF_SCRIPT; 
	    j = 1; 
	}
	if( !j && dec_style_check ) 
	    fi->df.dfPitchAndFamily |= FF_DECORATIVE;
   }
}

/*************************************************************************
 *           LFD_InitFontInfo
 *
 * INIT ONLY
 *
 * Fill in some fields in the fontInfo struct.
 */
static int LFD_InitFontInfo( fontInfo* fi, const LFD* lfd, LPCSTR fullname )
{
   int    	i, j, dec_style_check, scalability;
   fontEncodingTemplate* boba;
   const char* ridiculous = "font '%s' has ridiculous %s\n";
   char* lpstr;

   if (!lfd->charset_registry)
   {
       WARN("font '%s' does not have enough fields\n", fullname);
       return FALSE;
   }

   memset(fi, 0, sizeof(fontInfo) );

/* weight name - */
   LFD_GetWeight( fi, lfd->weight);

/* slant - */
   dec_style_check = LFD_GetSlant( fi, lfd->slant);

/* width name - */
   lpstr = lfd->set_width;
   if( strcasecmp( "normal", lpstr) )	/* XXX 'narrow', 'condensed', etc... */
       dec_style_check = TRUE;
   else
       fi->fi_flags |= FI_NORMAL;

/* style - */
   LFD_GetStyle(fi, lfd->add_style, dec_style_check);

/* pixel & decipoint height, and res_x & y */

   scalability = 0;

   j = strlen(lfd->pixel_size);
   if( j == 0 || j > 3 )
   {
       WARN(ridiculous, fullname, "pixel_size");
       return FALSE;
   }
   if( !(fi->lfd_height = atoi(lfd->pixel_size)) )
       scalability++;

   j = strlen(lfd->point_size);
   if( j == 0 || j > 3 )
   {
       WARN(ridiculous, fullname, "point_size");
       return FALSE;
   }
   if( !(atoi(lfd->point_size)) )
       scalability++;

   j = strlen(lfd->resolution_x);
   if( j == 0 || j > 3 )
   {
       WARN(ridiculous, fullname, "resolution_x");
       return FALSE;
   }
   if( !(fi->lfd_resolution = atoi(lfd->resolution_x)) )
       scalability++;

   j = strlen(lfd->resolution_y);
   if( j == 0 || j > 3 )
   {
       WARN(ridiculous, fullname, "resolution_y");
       return FALSE;
   }
   if( !(atoi(lfd->resolution_y)) )
       scalability++;

   /* Check scalability */
   switch (scalability)
   {
   case 0: /* Bitmap */
       break;
   case 4: /* Scalable */
       fi->fi_flags |= FI_SCALABLE;
       break;
   case 2:
       /* #$%^!!! X11R6 mutant garbage (scalable bitmap) */
       TRACE("Skipping scalable bitmap '%s'\n", fullname);
       return FALSE;
   default:
       WARN("Font '%s' has weird scalability\n", fullname);
       return FALSE;
   }

/* spacing - */
   lpstr = lfd->spacing;
   switch( *lpstr )
   {
     case '\0':
	 WARN("font '%s' has no spacing\n", fullname);
	 return FALSE;

     case 'p': fi->fi_flags |= FI_VARIABLEPITCH;
	       break;
     case 'c': fi->df.dfPitchAndFamily |= FF_MODERN;
	       fi->fi_flags |= FI_FIXEDEX;
	       /* fall through */
     case 'm': fi->fi_flags |= FI_FIXEDPITCH;
	       break;
     default:
               /* Of course this line does nothing */
	       fi->df.dfPitchAndFamily |= DEFAULT_PITCH | FF_DONTCARE;
   }

/* average width - */
   lpstr = lfd->average_width;
   j = strlen(lpstr);
   if( j == 0 || j > 3 )
   {
       WARN(ridiculous, fullname, "average_width");
       return FALSE;
   }

   if( !(atoi(lpstr)) && !scalability )
   {
       WARN("font '%s' has average_width 0 but is not scalable!!\n", fullname);
       return FALSE;
   }

/* charset registry, charset encoding - */
   lpstr = lfd->charset_registry;
   if( strstr(lpstr, "jisx") || 
       strstr(lpstr, "ksc") || 
       strstr(lpstr, "gb2312") ||
       strstr(lpstr, "big5") ||
       strstr(lpstr, "unicode") )
   {
       TRACE(" 2-byte fonts like '%s' are not supported\n", fullname);
       return FALSE;
   }

   fi->df.dfCharSet = ANSI_CHARSET;

   for( i = 0, boba = fETTable; boba; boba = boba->next, i++ )
   {
       if (!boba->prefix || !strcasecmp(lpstr, boba->prefix))
       {
	   if (lfd->charset_encoding)
	   {
	       for( j = 0; boba->sufch[j].psuffix; j++ )
	       {
		   if( !strcasecmp(lfd->charset_encoding, boba->sufch[j].psuffix ))
		   {
		       fi->df.dfCharSet = boba->sufch[j].charset;
		       goto done;
		   }
	       }
	       if (boba->prefix)
	       {
		   WARN("font '%s' has unknown character encoding '%s'\n",
			fullname, lfd->charset_encoding);
		   fi->df.dfCharSet = boba->sufch[j].charset;
		   j = 254;
		   goto done;
	       }
	   }
	   else if (boba->prefix)
	   {
	       for( j = 0; boba->sufch[j].psuffix; j++ )
		   ;
	       fi->df.dfCharSet = boba->sufch[j].charset;
	       j = 255;
	       goto done;
	   }
       }
   }
   WARN("font '%s' has unknown character set '%s'\n", fullname, lpstr);
   return FALSE;

done:
   /* i - index into fETTable
    * j - index into suffix array for fETTable[i]
    *     except:
    *     254 - unknown suffix
    *     255 - no suffix at all.
    */
   fi->fi_encoding = 256 * (UINT16)i + (UINT16)j;

   return TRUE;					
}


/*************************************************************************
 *           LFD_AngleMatrix
 *
 * make a matrix suitable for LFD based on theta radians
 */
static void LFD_AngleMatrix( char* buffer, int h, double theta)
{
    double matrix[4];
    matrix[0] = h*cos(theta);
    matrix[1] = h*sin(theta);
    matrix[2] = -h*sin(theta);
    matrix[3] = h*cos(theta);
    sprintf(buffer, "[%+f%+f%+f%+f]", matrix[0], matrix[1], matrix[2], matrix[3]);
}

/*************************************************************************
 *           LFD_ComposeLFD
 *
 * Note: uRelax is a treatment not a cure. Font mapping algorithm
 *       should be bulletproof enough to allow us to avoid hacks like
 *	 this despite LFD being so braindead.
 */
static BOOL LFD_ComposeLFD( const fontObject* fo, 
			    INT height, LPSTR lpLFD, UINT uRelax )
{
   int		i, h;
   char         *any = "*";
   char         h_string[64], resx_string[64], resy_string[64];
   LFD          aLFD;

/* Get the worst case over with first */

/* RealizeFont() will call us repeatedly with increasing uRelax 
 * until XLoadFont() succeeds. 
 * to avoid an infinite loop; these will always match
 */
   if (uRelax >= 5)
   {
       if (uRelax == 5)
	   sprintf( lpLFD, "-*-*-*-*-*-*-*-*-*-*-*-*-iso8859-1" );
       else
	   sprintf( lpLFD, "-*-*-*-*-*-*-*-*-*-*-*-*-*-*" );
       return TRUE;
   }

/* read foundry + family from fo */
   aLFD.foundry = fo->fr->resource->foundry;
   aLFD.family  = fo->fr->resource->family;

/* add weight */
   switch( fo->fi->df.dfWeight )
   {
        case FW_BOLD:
		aLFD.weight = "bold"; break;
	case FW_REGULAR:
		if( fo->fi->fi_flags & FI_FW_BOOK )
		    aLFD.weight = "book";
		else
		    aLFD.weight = "medium";
		break;
	case FW_DEMIBOLD:
		aLFD.weight = "demi";
		if( !( fo->fi->fi_flags & FI_FW_DEMI) )
		     aLFD.weight = "bold";
		break;
	case FW_BLACK:
		aLFD.weight = "black"; break;
	case FW_LIGHT:
		aLFD.weight = "light"; break;
	default:
		aLFD.weight = any;
   }

/* add slant */
   if( fo->fi->df.dfItalic )
       if( fo->fi->fi_flags & FI_OBLIQUE )
	   aLFD.slant = "o";
       else
	   aLFD.slant = "i";
   else 
       aLFD.slant = (uRelax < 1) ? "r" : any;

/* add width */
   if( fo->fi->fi_flags & FI_NORMAL )
       aLFD.set_width = "normal";
   else
       aLFD.set_width = any;

/* ignore style */
   aLFD.add_style = any;

/* add pixelheight 
 *
 * FIXME: fill in lpXForm and lpPixmap for rotated fonts
 */
   if( fo->fo_flags & FO_SYNTH_HEIGHT )
       h = fo->fi->lfd_height;
   else
   {
       h = (fo->fi->lfd_height * height + (fo->fi->df.dfPixHeight>>1));
       h /= fo->fi->df.dfPixHeight;
   } 
   if (h < MIN_FONT_SIZE) /* Resist rounding down to 0 */
       h = MIN_FONT_SIZE;
   else if (h > MAX_FONT_SIZE) /* Sanity check as huge fonts can lock up the font server */
   {
       WARN("Huge font size %d pixels has been reduced to %d\n", h, MAX_FONT_SIZE);
       h = MAX_FONT_SIZE;
   }
       
   if (uRelax <= 2)
       /* handle rotated fonts */
       if (fo->lf.lfEscapement) {
	   /* escapement is in tenths of degrees, theta is in radians */
	   double theta = M_PI*fo->lf.lfEscapement/1800.;  
	   LFD_AngleMatrix(h_string, h, theta);
       }
       else
       {
	   sprintf(h_string, "%d", h);
       }
   else
       sprintf(h_string, "%d", fo->fi->lfd_height);

   aLFD.pixel_size = h_string;
   aLFD.point_size = any;

/* resolution_x,y, average width */
   /* FOX ME - Why do some font servers ignore average width ?
    * so that you have to mess around with res_y
    */
   aLFD.average_width = any;
   if (uRelax <= 3)
   {
       sprintf(resx_string, "%d", fo->fi->lfd_resolution);
       aLFD.resolution_x = resx_string;

       strcpy(resy_string, resx_string);
       if( uRelax == 0  && XTextCaps & TC_SF_X_YINDEP ) 
       {
	   if( fo->lf.lfWidth && !(fo->fo_flags & FO_SYNTH_WIDTH))
	   {
	       int resy = 0.5 + fo->fi->lfd_resolution *	
		   (fo->fi->df.dfAvgWidth * height) /
		   (fo->lf.lfWidth * fo->fi->df.dfPixHeight) ;
	       sprintf(resy_string,  "%d", resy);
	   }
	   else
	       ; /* FIXME - synth width */
       }	   
       aLFD.resolution_y = resy_string;
   }
   else
   {
       aLFD.resolution_x = aLFD.resolution_y = any;
   }

/* spacing */
   {
       char* w;

       if( fo->fi->fi_flags & FI_FIXEDPITCH ) 
	   w = ( fo->fi->fi_flags & FI_FIXEDEX ) ? "c" : "m";
       else 
	   w = ( fo->fi->fi_flags & FI_VARIABLEPITCH ) ? "p" : any; 
       
       aLFD.spacing = (uRelax <= 1) ? w : any;
   }

/* encoding */

   if (uRelax <= 4)
   {
       fontEncodingTemplate* boba;
       
       i = fo->fi->fi_encoding >> 8;
       for( boba = fETTable; i; i--, boba = boba->next );
       
       aLFD.charset_registry = boba->prefix ? boba->prefix : any;
       
       i = fo->fi->fi_encoding & 255;
       switch( i )
       {
       default:
	   aLFD.charset_encoding = boba->sufch[i].psuffix;
	   break;
	   
       case 254:
	   aLFD.charset_encoding = any;
	   break;
	   
       case 255: /* no suffix - it ends eg "-ascii" */
	   aLFD.charset_encoding = NULL;
	   break;
       }
   }
   else
   {
       aLFD.charset_registry = any;
       aLFD.charset_encoding = any;
   }
    
   LFD_UnParse(lpLFD, MAX_LFD_LENGTH, &aLFD);

   TRACE("\tLFD(uRelax=%d): %s\n", uRelax, lpLFD );
   return TRUE;
}


/***********************************************************************
 *		X Font Resources
 *
 * font info		- http://www.microsoft.com/kb/articles/q65/1/23.htm
 * Windows font metrics	- http://www.microsoft.com/kb/articles/q32/6/67.htm
 */
static void XFONT_GetLeading( const LPIFONTINFO16 pFI, const XFontStruct* x_fs, 
			      INT16* pIL, INT16* pEL, const XFONTTRANS *XFT )
{
    unsigned long height;
    unsigned min = (unsigned char)pFI->dfFirstChar;
    BOOL bIsLatin = IS_LATIN_CHARSET(pFI->dfCharSet);

    if( pEL ) *pEL = 0;

    if(XFT) {
        Atom RAW_CAP_HEIGHT = TSXInternAtom(display, "RAW_CAP_HEIGHT", TRUE);
	if(TSXGetFontProperty((XFontStruct*)x_fs, RAW_CAP_HEIGHT, &height))
	    *pIL = XFT->ascent - 
                            (INT)(XFT->pixelsize / 1000.0 * height);
	else
	    *pIL = 0;
	return;
    }
       
    if( TSXGetFontProperty((XFontStruct*)x_fs, XA_CAP_HEIGHT, &height) == FALSE )
    {
        if( x_fs->per_char )
	    if( bIsLatin )
		    height = x_fs->per_char['X' - min].ascent;
	    else
		if (x_fs->ascent >= x_fs->max_bounds.ascent)
		    height = x_fs->max_bounds.ascent;
		else
		{
		    height = x_fs->ascent;
		    if( pEL )
			*pEL = x_fs->max_bounds.ascent - height;
		}
	else 
	    height = x_fs->min_bounds.ascent;
    }

    *pIL = x_fs->ascent - height;
}

/***********************************************************************
 *           XFONT_CharWidth
 */
static int XFONT_CharWidth(const XFontStruct* x_fs,
			   const XFONTTRANS *XFT, int ch)
{   
    if(!XFT)
	return x_fs->per_char[ch].width;
    else
	return x_fs->per_char[ch].attributes * XFT->pixelsize / 1000.0;
}

/***********************************************************************
 *           XFONT_GetAvgCharWidth
 */
static INT XFONT_GetAvgCharWidth( LPIFONTINFO16 pFI, const XFontStruct* x_fs,
				    const XFONTTRANS *XFT)
{
    unsigned min = (unsigned char)pFI->dfFirstChar;
    unsigned max = (unsigned char)pFI->dfLastChar;

    INT avg;

    if( x_fs->per_char )
    {
	int  width = 0, chars = 0, j;
	if( IS_LATIN_CHARSET(pFI->dfCharSet))
	{
	    /* FIXME - should use a weighted average */
	    for( j = 0; j < 26; j++ )
		width += XFONT_CharWidth(x_fs, XFT, 'a' + j - min) +
		         XFONT_CharWidth(x_fs, XFT, 'A' + j - min);	    
	    chars = 52;
	}
	else /* unweighted average of everything */
	{
	    for( j = 0,  max -= min; j <= max; j++ )
		if( !CI_NONEXISTCHAR(x_fs->per_char + j) )
		{
		    width += XFONT_CharWidth(x_fs, XFT, j);
		    chars++;
		}
	}
	avg = (width + (chars>>1))/ chars;
    }
    else /* uniform width */
	avg = x_fs->min_bounds.width;

    return avg;
}

/***********************************************************************
 *           XFONT_GetMaxCharWidth
 */
static INT XFONT_GetMaxCharWidth(const XFontStruct* xfs, const XFONTTRANS *XFT)
{
    unsigned min = (unsigned char)xfs->min_char_or_byte2;
    unsigned max = (unsigned char)xfs->max_char_or_byte2;
    int  maxwidth, j;

    if(!XFT || !xfs->per_char)
        return abs(xfs->max_bounds.width);

    for( j = 0, maxwidth = 0, max -= min; j <= max; j++ )
	if( !CI_NONEXISTCHAR(xfs->per_char + j) )
	    if(maxwidth < xfs->per_char[j].attributes)
		maxwidth = xfs->per_char[j].attributes;
    
    maxwidth *= XFT->pixelsize / 1000.0;
    return maxwidth;
}

/***********************************************************************
 *              XFONT_SetFontMetric
 *
 * INIT ONLY
 *
 * Initializes IFONTINFO16.
 */
static void XFONT_SetFontMetric(fontInfo* fi, const fontResource* fr, XFontStruct* xfs)
{
    unsigned min, max;
    fi->df.dfFirstChar = (BYTE)(min = xfs->min_char_or_byte2);
    fi->df.dfLastChar = (BYTE)(max = xfs->max_char_or_byte2);

    fi->df.dfDefaultChar = (BYTE)xfs->default_char;
    fi->df.dfBreakChar = (BYTE)(( ' ' < min || ' ' > max) ? xfs->default_char: ' ');

    fi->df.dfPixHeight = (INT16)((fi->df.dfAscent = (INT16)xfs->ascent) + xfs->descent);
    fi->df.dfPixWidth = (xfs->per_char) ? 0 : xfs->min_bounds.width;

    XFONT_GetLeading( &fi->df, xfs, &fi->df.dfInternalLeading, &fi->df.dfExternalLeading, NULL );
    fi->df.dfAvgWidth = (INT16)XFONT_GetAvgCharWidth(&fi->df, xfs, NULL );
    fi->df.dfMaxWidth = (INT16)XFONT_GetMaxCharWidth(xfs, NULL);

    if( xfs->min_bounds.width != xfs->max_bounds.width )
        fi->df.dfPitchAndFamily |= TMPF_FIXED_PITCH; /* au contraire! */
    if( fi->fi_flags & FI_SCALABLE ) 
    {
	fi->df.dfType = DEVICE_FONTTYPE;
        fi->df.dfPitchAndFamily |= TMPF_DEVICE;
    } 
    else if( fi->fi_flags & FI_TRUETYPE )
	fi->df.dfType = TRUETYPE_FONTTYPE;
    else
	fi->df.dfType = RASTER_FONTTYPE;

    fi->df.dfFace = fr->lfFaceName;
}

/***********************************************************************
 *              XFONT_GetTextMetrics
 *
 * GetTextMetrics() back end.
 */
static void XFONT_GetTextMetrics( const fontObject* pfo, const LPTEXTMETRICA pTM )
{
    LPIFONTINFO16 pdf = &pfo->fi->df;

    if( ! pfo->lpX11Trans ) {
      pTM->tmAscent = pfo->fs->ascent;
      pTM->tmDescent = pfo->fs->descent;
    } else {
      pTM->tmAscent = pfo->lpX11Trans->ascent;
      pTM->tmDescent = pfo->lpX11Trans->descent;
    }

    pTM->tmAscent *= pfo->rescale;
    pTM->tmDescent *= pfo->rescale;

    pTM->tmHeight = pTM->tmAscent + pTM->tmDescent;

    pTM->tmAveCharWidth = pfo->foAvgCharWidth * pfo->rescale;
    pTM->tmMaxCharWidth = pfo->foMaxCharWidth * pfo->rescale;

    pTM->tmInternalLeading = pfo->foInternalLeading * pfo->rescale;
    pTM->tmExternalLeading = pdf->dfExternalLeading * pfo->rescale;

    pTM->tmStruckOut = (pfo->fo_flags & FO_SYNTH_STRIKEOUT )
			? 1 : pdf->dfStrikeOut;
    pTM->tmUnderlined = (pfo->fo_flags & FO_SYNTH_UNDERLINE )
			? 1 : pdf->dfUnderline;

    pTM->tmOverhang = 0;
    if( pfo->fo_flags & FO_SYNTH_ITALIC ) 
    {
	pTM->tmOverhang += pTM->tmHeight/3;
	pTM->tmItalic = 1;
    } else 
	pTM->tmItalic = pdf->dfItalic;

    pTM->tmWeight = pdf->dfWeight;
    if( pfo->fo_flags & FO_SYNTH_BOLD ) 
    {
	pTM->tmOverhang++; 
	pTM->tmWeight += 100;
    } 

    pTM->tmFirstChar = pdf->dfFirstChar;
    pTM->tmLastChar = pdf->dfLastChar;
    pTM->tmDefaultChar = pdf->dfDefaultChar;
    pTM->tmBreakChar = pdf->dfBreakChar;

    pTM->tmCharSet = pdf->dfCharSet;
    pTM->tmPitchAndFamily = pdf->dfPitchAndFamily;

    pTM->tmDigitizedAspectX = pdf->dfHorizRes;
    pTM->tmDigitizedAspectY = pdf->dfVertRes;
}

/***********************************************************************
 *              XFONT_GetFontMetric
 *
 * Retrieve font metric info (enumeration).
 */
static UINT XFONT_GetFontMetric( const fontInfo* pfi, const LPENUMLOGFONTEX16 pLF,
                                                  const LPNEWTEXTMETRIC16 pTM )
{
    memset( pLF, 0, sizeof(*pLF) );
    memset( pTM, 0, sizeof(*pTM) );

#define plf ((LPLOGFONT16)pLF)
    plf->lfHeight    = pTM->tmHeight       = pfi->df.dfPixHeight;
    plf->lfWidth     = pTM->tmAveCharWidth = pfi->df.dfAvgWidth;
    plf->lfWeight    = pTM->tmWeight       = pfi->df.dfWeight;
    plf->lfItalic    = pTM->tmItalic       = pfi->df.dfItalic;
    plf->lfUnderline = pTM->tmUnderlined   = pfi->df.dfUnderline;
    plf->lfStrikeOut = pTM->tmStruckOut    = pfi->df.dfStrikeOut;
    plf->lfCharSet   = pTM->tmCharSet      = pfi->df.dfCharSet;

    /* convert pitch values */

    pTM->tmPitchAndFamily = pfi->df.dfPitchAndFamily;
    plf->lfPitchAndFamily = (pfi->df.dfPitchAndFamily & 0xF1) + 1;

    lstrcpynA( plf->lfFaceName, pfi->df.dfFace, LF_FACESIZE );
#undef plf

    /* FIXME: fill in rest of plF values
    lstrcpynA(plF->elfFullName, , LF_FULLFACESIZE);
    lstrcpynA(plF->elfStyle, , LF_FACESIZE);
    lstrcpynA(plF->elfScript, , LF_FACESIZE);
    */

    pTM->tmAscent = pfi->df.dfAscent;
    pTM->tmDescent = pTM->tmHeight - pTM->tmAscent;
    pTM->tmInternalLeading = pfi->df.dfInternalLeading;
    pTM->tmMaxCharWidth = pfi->df.dfMaxWidth;
    pTM->tmDigitizedAspectX = pfi->df.dfHorizRes;
    pTM->tmDigitizedAspectY = pfi->df.dfVertRes;

    pTM->tmFirstChar = pfi->df.dfFirstChar;
    pTM->tmLastChar = pfi->df.dfLastChar;
    pTM->tmDefaultChar = pfi->df.dfDefaultChar;
    pTM->tmBreakChar = pfi->df.dfBreakChar;

    /* return font type */

    return pfi->df.dfType;
}


/***********************************************************************
 *           XFONT_FixupFlags
 *
 * INIT ONLY
 * 
 * dfPitchAndFamily flags for some common typefaces.
 */
static BYTE XFONT_FixupFlags( LPCSTR lfFaceName )
{
   switch( lfFaceName[0] )
   {
        case 'a':
        case 'A': if(!strncasecmp(lfFaceName, "Arial", 5) )
                    return FF_SWISS;
                  break;
        case 'h':
        case 'H': if(!strcasecmp(lfFaceName, "Helvetica") )
                    return FF_SWISS;
                  break;
        case 'c':
        case 'C': if(!strncasecmp(lfFaceName, "Courier", 7))
	            return FF_MODERN;
	
	          if (!strcasecmp(lfFaceName, "Charter") )
		      return FF_ROMAN;
		  break;
        case 'p':
        case 'P': if( !strcasecmp(lfFaceName,"Palatino") )
                    return FF_ROMAN;
                  break;
        case 't':
        case 'T': if(!strncasecmp(lfFaceName, "Times", 5) )
                    return FF_ROMAN;
                  break;
        case 'u':
        case 'U': if(!strcasecmp(lfFaceName, "Utopia") )
                    return FF_ROMAN;
                  break;
        case 'z':
        case 'Z': if(!strcasecmp(lfFaceName, "Zapf Dingbats") )
                    return FF_DECORATIVE;
   }
   return 0;
}

/***********************************************************************
 *           XFONT_SameFoundryAndFamily
 *
 * INIT ONLY
 */
static BOOL XFONT_SameFoundryAndFamily( const LFD* lfd1, const LFD* lfd2 )
{
    return ( !strcasecmp( lfd1->foundry, lfd2->foundry ) && 
	     !strcasecmp( lfd1->family,  lfd2->family ) );
}

/***********************************************************************
 *           XFONT_InitialCapitals
 *
 * INIT ONLY
 *
 * Upper case first letters of words & remove multiple spaces
 */
static void XFONT_InitialCapitals(LPSTR lpch)
{
    int i;
    BOOL up;
    char* lpstr = lpch;

    for( i = 0, up = TRUE; *lpch; lpch++, i++ )
    {
	if( isspace(*lpch) ) 
	{
	    if (!up)  /* Not already got one */
	    {
		*lpstr++ = ' ';
		up = TRUE;
	    }
	}
	else if( isalpha(*lpch) && up )
	{ 
	    *lpstr++ = toupper(*lpch); 
	    up = FALSE;
	}
	else 
	{
	    *lpstr++ = *lpch;
	    up = FALSE;
	}
    }

    /* Remove possible trailing space */
    if (up && i > 0)
	--lpstr;
    *lpstr = '\0';
}


/***********************************************************************
 *           XFONT_WindowsNames
 *
 * INIT ONLY
 *
 * Build generic Windows aliases for X font names.
 *
 * -misc-fixed- -> "Fixed"
 * -sony-fixed- -> "Sony Fixed", etc...
 */
static void XFONT_WindowsNames(void)
{
    fontResource* fr;

    for( fr = fontList; fr ; fr = fr->next )
    {
	fontResource* pfr;
	char* 	      lpch;

	if( fr->fr_flags & FR_NAMESET ) continue;     /* skip already assigned */

	for( pfr = fontList; pfr != fr ; pfr = pfr->next )
	    if( pfr->fr_flags & FR_NAMESET )
	    {
		if (!strcasecmp( pfr->resource->family, fr->resource->family))
		    break;
	    }

	lpch = fr->lfFaceName;
	wsnprintfA( fr->lfFaceName, sizeof(fr->lfFaceName), "%s %s",
					  /* prepend vendor name */
					  (pfr==fr) ? "" : fr->resource->foundry,
					  fr->resource->family);
	XFONT_InitialCapitals(fr->lfFaceName);
	{
	    BYTE bFamilyStyle = XFONT_FixupFlags( fr->lfFaceName );
	    if( bFamilyStyle)
	    {
		fontInfo* fi;
		for( fi = fr->fi ; fi ; fi = fi->next )
		    fi->df.dfPitchAndFamily |= bFamilyStyle;
	    }
	}
	    
	TRACE("typeface '%s'\n", fr->lfFaceName);
	
	fr->fr_flags |= FR_NAMESET;
    }
}

/***********************************************************************
 *           XFONT_LoadDefaultLFD
 *
 * Move lfd to the head of fontList to make it more likely to be matched
 */
static void XFONT_LoadDefaultLFD(LFD* lfd, LPCSTR fonttype)
{
    {
	fontResource *fr, *pfr;
	for( fr = NULL, pfr = fontList; pfr; pfr = pfr->next )
	{
	    if( XFONT_SameFoundryAndFamily(pfr->resource, lfd) )
	    {
		if( fr )
		{
		    fr->next = pfr->next;
		    pfr->next = fontList;
		    fontList = pfr;
		}
		break;
	    }
	    fr = pfr;
	}
	if (!pfr)
	    WARN("Default %sfont '-%s-%s-' not available\n", fonttype, 
		 lfd->foundry, lfd->family);
    }
}

/***********************************************************************
 *           XFONT_LoadDefault
 */
static void XFONT_LoadDefault(LPCSTR ini, LPCSTR fonttype)
{
    char buffer[MAX_LFD_LENGTH];

    if( PROFILE_GetWineIniString( INIFontSection, ini, "", buffer, sizeof buffer ) )
    {
	if (*buffer)
	{
	    LFD* lfd;
	    char* pch = buffer;
	    while( *pch && isspace(*pch) ) pch++;
	    
	    TRACE("Using '%s' as default %sfont\n", pch, fonttype);
	    lfd = LFD_Parse(pch);
	    if (lfd && lfd->foundry && lfd->family)
		XFONT_LoadDefaultLFD(lfd, fonttype);
	    else
		WARN("Ini section [%s]%s is malformed\n", INIFontSection, ini);
	    HeapFree(SystemHeap, 0, lfd);
	}
    }   
}

/***********************************************************************
 *           XFONT_LoadDefaults
 */
static void XFONT_LoadDefaults(void)
{
    XFONT_LoadDefault(INIDefaultFixed, "fixed ");
    XFONT_LoadDefault(INIDefault, "");
}

/***********************************************************************
 *           XFONT_CreateAlias
 */
static fontAlias* XFONT_CreateAlias( LPCSTR lpTypeFace, LPCSTR lpAlias )
{
    int j;
    fontAlias *pfa, *prev = NULL;

    for(pfa = aliasTable; pfa; pfa = pfa->next)
    {
	/* check if we already got one */
	if( !strcasecmp( pfa->faTypeFace, lpAlias ) )
	{
	    TRACE("redundant alias '%s' -> '%s'\n", 
		  lpAlias, lpTypeFace );
	    return NULL;
	}
	prev = pfa;
    }

    j = lstrlenA(lpTypeFace) + 1;
    pfa = HeapAlloc( SystemHeap, 0, sizeof(fontAlias) +
			       j + lstrlenA(lpAlias) + 1 );
    if (pfa)
    {
        if (!prev)
	    aliasTable = pfa;
	else
	    prev->next = pfa;
    
	pfa->next = NULL;
	pfa->faTypeFace = (LPSTR)(pfa + 1);
	lstrcpyA( pfa->faTypeFace, lpTypeFace );
	pfa->faAlias = pfa->faTypeFace + j;
	lstrcpyA( pfa->faAlias, lpAlias );

        TRACE("added alias '%s' for %s\n", lpAlias, lpTypeFace );

	return pfa;
    }
    return NULL;
}


/***********************************************************************
 *           XFONT_LoadAlias
 */
static void XFONT_LoadAlias(const LFD* lfd, LPCSTR lpAlias, BOOL bSubst)
{
    fontResource *fr, *frMatch = NULL;
    if (!lfd->foundry || ! lfd->family)
    {
	WARN("Malformed font resource for alias '%s'\n", lpAlias);
	return;
    }
    for (fr = fontList; fr ; fr = fr->next)
    {
        if(!strcasecmp(fr->resource->family, lpAlias))
	{
	    /* alias is not needed since the real font is present */
	    TRACE("Ignoring font alias '%s' as it is already available as a real font\n", lpAlias);
	    return;
	}
	if( XFONT_SameFoundryAndFamily( fr->resource, lfd ) )
	{
	    frMatch = fr;
	    break;
	}
    }

    if( frMatch )
    {
        if( bSubst )
	{
	    fontAlias *pfa, *prev = NULL;

	    for(pfa = aliasTable; pfa; pfa = pfa->next)
	    {
	        /* Remove lpAlias from aliasTable - we should free the old entry */
	        if(!strcmp(lpAlias, pfa->faAlias))
		{
		    if(prev)
		        prev->next = pfa->next;
		    else
		        aliasTable = pfa->next;
		}

		/* Update any references to the substituted font in aliasTable */
		if(!strcmp(frMatch->lfFaceName, pfa->faTypeFace))
		    pfa->faTypeFace = HEAP_strdupA( SystemHeap, 0, lpAlias );
		prev = pfa;
	    }
						
	    TRACE("\tsubstituted '%s' with %s\n", frMatch->lfFaceName, lpAlias );
		
	    lstrcpynA( frMatch->lfFaceName, lpAlias, LF_FACESIZE );
	    frMatch->fr_flags |= FR_NAMESET;
	}
	else
	{
	    /* create new entry in the alias table */
	    XFONT_CreateAlias( frMatch->lfFaceName, lpAlias );
	}
    }
    else
    {
	WARN("Font alias '-%s-%s-' is not available\n", lfd->foundry, lfd->family);
    }
} 

/***********************************************************************
 *           XFONT_LoadAliases
 *
 * INIT ONLY 
 *
 * Create font aliases for some standard windows fonts using users
 * default choice of (sans-)serif fonts
 *
 * Read user-defined aliases from wine.conf. Format is as follows
 *
 * Alias# = [Windows font name],[LFD font name], <substitute original name>
 *
 * Example:
 *   Alias0 = Arial, -adobe-helvetica- 
 *   Alias1 = Times New Roman, -bitstream-courier-, 1
 *   ...
 *
 * Note that from 081797 and on we have built-in alias templates that take
 * care of the necessary Windows typefaces.
 */
typedef struct
{
  LPSTR                 fatResource;
  LPSTR                 fatAlias;
} aliasTemplate;

static void XFONT_LoadAliases(void)
{
    char *lpResource;
    char buffer[MAX_LFD_LENGTH];
    int i = 0;
    LFD* lfd;

    /* built-ins first */
    PROFILE_GetWineIniString( INIFontSection, INIDefaultSerif,
				"-bitstream-charter-", buffer, sizeof buffer );
    TRACE("Using '%s' as default serif font\n", buffer);
    lfd = LFD_Parse(buffer);
    /* NB XFONT_InitialCapitals should not change these standard aliases */
    if (lfd)
    {
	XFONT_LoadAlias( lfd, "Tms Roman", FALSE);
	XFONT_LoadAlias( lfd, "MS Serif", FALSE);
	XFONT_LoadAlias( lfd, "Times New Roman", FALSE);

	XFONT_LoadDefaultLFD( lfd, "serif ");
	HeapFree(SystemHeap, 0, lfd);
    }
	
    PROFILE_GetWineIniString( INIFontSection, INIDefaultSansSerif,
				"-adobe-helvetica-", buffer, sizeof buffer);
    TRACE("Using '%s' as default sans serif font\n", buffer);
    lfd = LFD_Parse(buffer);
    if (lfd)
    {
	XFONT_LoadAlias( lfd, "Helv", FALSE);
	XFONT_LoadAlias( lfd, "MS Sans Serif", FALSE);
	XFONT_LoadAlias( lfd, "Arial", FALSE);

	XFONT_LoadDefaultLFD( lfd, "sans serif ");
	HeapFree(SystemHeap, 0, lfd);
    }

    /* then user specified aliases */
    do
    {
        BOOL bHaveAlias, bSubst;
	char subsection[32];
        wsnprintfA( subsection, sizeof subsection, "%s%i", INIAliasSection, i++ );

	bHaveAlias = PROFILE_GetWineIniString( INIFontSection, 
						subsection, "", buffer, sizeof buffer);
	if (!bHaveAlias)
	    break;

	XFONT_InitialCapitals(buffer);
	lpResource = PROFILE_GetStringItem( buffer );
	bSubst = (PROFILE_GetStringItem( lpResource )) ? TRUE : FALSE;
	if( lpResource && *lpResource )
	{
	    lfd = LFD_Parse(lpResource);
	    if (lfd)
	    {
		XFONT_LoadAlias(lfd, buffer, bSubst);
		HeapFree(SystemHeap, 0, lfd);
	    }
	}
	else
	    WARN("malformed font alias '%s'\n", buffer );
    }
    while(TRUE);
}

/***********************************************************************
 *           XFONT_UnAlias
 *
 * Convert an (potential) alias into a real windows name
 *
 */
static LPCSTR XFONT_UnAlias(char* font)
{
    if (font[0])
    {
	fontAlias* fa;
	XFONT_InitialCapitals(font); /* to remove extra white space */
    
	for( fa = aliasTable; fa; fa = fa->next )
	    /* use case insensitive matching to handle eg "MS Sans Serif" */
	    if( !strcasecmp( fa->faAlias, font ) ) 
	    {
		TRACE("found alias '%s'->%s'\n", font, fa->faTypeFace );
		strcpy(font, fa->faTypeFace);
		return fa->faAlias;
		break;
	    }
    }
    return NULL;
}

/***********************************************************************
 *           XFONT_RemoveFontResource
 *
 * Caller should check if the font resource is in use. If it is it should
 * set FR_REMOVED flag to delay removal until the resource is not in use
 * anymore.
 */
void XFONT_RemoveFontResource( fontResource** ppfr )
{
    fontResource* pfr = *ppfr;
#if 0
    fontInfo* pfi;

    /* FIXME - if fonts were read from a cache, these HeapFrees will fail */
    while( pfr->fi )
    {
	pfi = pfr->fi->next;
	HeapFree( SystemHeap, 0, pfr->fi );
	pfr->fi = pfi;
    }
    HeapFree( SystemHeap, 0, pfr );
#endif
    *ppfr = pfr->next;
}

/***********************************************************************
 *           XFONT_LoadIgnores
 *
 * INIT ONLY 
 *
 * Removes specified fonts from the font table to prevent Wine from
 * using it.
 *
 * Ignore# = [LFD font name]
 *
 * Example:
 *   Ignore0 = -misc-nil-
 *   Ignore1 = -sun-open look glyph-
 *   ...
 *
 */
static void XFONT_LoadIgnore(char* lfdname)
{
    fontResource** ppfr;

    LFD* lfd = LFD_Parse(lfdname);
    if (lfd && lfd->foundry && lfd->family)
    {
	for( ppfr = &fontList; *ppfr ; ppfr = &((*ppfr)->next))
	{
	    if( XFONT_SameFoundryAndFamily( (*ppfr)->resource, lfd) )
	    {
		TRACE("Ignoring '-%s-%s-'\n",
		      (*ppfr)->resource->foundry, (*ppfr)->resource->family  );
			
		XFONT_RemoveFontResource( ppfr );
		break;
	    }
	}
    }
    else
	WARN("Malformed font resource\n");
    
    HeapFree(SystemHeap, 0, lfd);
}

static void XFONT_LoadIgnores(void)
{
    int i = 0;
    char  subsection[32];
    char buffer[MAX_LFD_LENGTH];

    /* Standard one that noone wants */
    strcpy(buffer, "-misc-nil-");
    XFONT_LoadIgnore(buffer);

    /* Others from INI file */
    do
    {
	wsprintfA( subsection, "%s%i", INIIgnoreSection, i++ );

	if( PROFILE_GetWineIniString( INIFontSection,
				      subsection, "", buffer, sizeof buffer) )
	{
	    char* pch = buffer;
	    while( *pch && isspace(*pch) ) pch++;
	    XFONT_LoadIgnore(pch);
	}
	else 
	    break;
    } while(TRUE);
}


/***********************************************************************
 *           XFONT_UserMetricsCache
 * 
 * Returns expanded name for the cachedmetrics file.
 * Now it also appends the current value of the $DISPLAY varaible.
 */
static char* XFONT_UserMetricsCache( char* buffer, int* buf_size )
{
    char* pchDisplay, *home;

    pchDisplay = getenv( "DISPLAY" );
    if( !pchDisplay ) pchDisplay = "0";

    if ((home = getenv( "HOME" )) != NULL)
    {
	int i = strlen( home ) + strlen( INIWinePrefix ) + 
		strlen( INIFontMetrics ) + strlen( pchDisplay ) + 2;
	if( i > *buf_size ) 
	    buffer = (char*) HeapReAlloc( SystemHeap, 0, buffer, *buf_size = i );
	strcpy( buffer, home );
	strcat( buffer, INIWinePrefix );
	strcat( buffer, INIFontMetrics );
	strcat( buffer, pchDisplay );
    } else buffer[0] = '\0';
    return buffer;
}


/***********************************************************************
 *           X Font Matching
 *
 * Compare two fonts (only parameters set by the XFONT_InitFontInfo()).
 */
static INT XFONT_IsSubset(const fontInfo* match, const fontInfo* fi)
{
  INT           m;

  /* 0 - keep both, 1 - keep match, -1 - keep fi */

  /* Compare dfItalic, Underline, Strikeout, Weight, Charset */
  m = (BYTE*)&fi->df.dfPixWidth - (BYTE*)&fi->df.dfItalic;
  if( memcmp(&match->df.dfItalic, &fi->df.dfItalic, m )) return 0;

  if( (!((fi->fi_flags & FI_SCALABLE) + (match->fi_flags & FI_SCALABLE))
       && fi->lfd_height != match->lfd_height) ||
      (!((fi->fi_flags & FI_POLYWEIGHT) + (match->fi_flags & FI_POLYWEIGHT))
       && fi->df.dfWeight != match->df.dfWeight) ) return 0;

  m = (int)(match->fi_flags & (FI_POLYWEIGHT | FI_SCALABLE)) -
      (int)(fi->fi_flags & (FI_SCALABLE | FI_POLYWEIGHT));

  if( m == (FI_POLYWEIGHT - FI_SCALABLE) ||
      m == (FI_SCALABLE - FI_POLYWEIGHT) ) return 0;	/* keep both */
  else if( m >= 0 ) return 1;	/* 'match' is better */

  return -1;			/* 'fi' is better */
}

/***********************************************************************
 *            XFONT_CheckFIList
 *
 * REMOVE_SUBSETS - attach new fi and purge subsets
 * UNMARK_SUBSETS - remove subset flags from all fi entries
 */
static void XFONT_CheckFIList( fontResource* fr, fontInfo* fi, int action)
{
  int		i = 0;
  fontInfo*     pfi, *prev;

  for( prev = NULL, pfi = fr->fi; pfi; )
  {
    if( action == REMOVE_SUBSETS )
    {
        if( pfi->fi_flags & FI_SUBSET )
        {
	    fontInfo* subset = pfi;

	    i++;
	    fr->fi_count--;
            if( prev ) prev->next = pfi = pfi->next;
            else fr->fi = pfi = pfi->next;
	    HeapFree( SystemHeap, 0, subset );
	    continue;
        }
    }
    else pfi->fi_flags &= ~FI_SUBSET;

    prev = pfi; 
    pfi = pfi->next;
  }

  if( action == REMOVE_SUBSETS )	/* also add the superset */
  {
    if( fi->fi_flags & FI_SCALABLE )
    {
        fi->next = fr->fi;
        fr->fi = fi;
    }
    else if( prev ) prev->next = fi; else fr->fi = fi;
    fr->fi_count++;
  }

  if( i ) TRACE("\t    purged %i subsets [%i]\n", i , fr->fi_count);
}

/***********************************************************************
 *            XFONT_FindFIList
 */
static fontResource* XFONT_FindFIList( fontResource* pfr, const char* pTypeFace )
{
  while( pfr )
  {
    if( !strcasecmp( pfr->lfFaceName, pTypeFace ) ) break;
    pfr = pfr->next;
  }
  /* Give the app back the font name it asked for. Encarta checks this. */
  if (pfr) strcpy(pfr->lfFaceName,pTypeFace);
  return pfr;
}

/***********************************************************************
 *           XFONT_FixupPointSize
 */
static void XFONT_FixupPointSize(fontInfo* fi)
{
#define df (fi->df)
    df.dfHorizRes = df.dfVertRes = fi->lfd_resolution;
    df.dfPoints = (INT16)
	(((INT)(df.dfPixHeight - df.dfInternalLeading) * 72 + (df.dfVertRes >> 1)) /
	 df.dfVertRes );
#undef df
}

/***********************************************************************
 *           XFONT_BuildMetrics
 * 
 * Build font metrics from X font
 */
static int XFONT_BuildMetrics(char** x_pattern, int res, unsigned x_checksum, int x_count)
{
    int		  i;
    fontInfo*	  fi = NULL;
    int           n_ff = 0;

    MESSAGE("Building font metrics. This may take some time...\n");
    for( i = 0; i < x_count; i++ )
    {
	char*         typeface;
	LFD*          lfd;
	fontResource* fr, *pfr;
	int           j;
	char          buffer[MAX_LFD_LENGTH];
	char*	      lpstr;
	XFontStruct*  x_fs;
	fontInfo*    pfi;

	typeface = HEAP_strdupA(SystemHeap, 0, x_pattern[i]);
	if (!typeface)
	    break;

	lfd = LFD_Parse(typeface);
	if (!lfd)
	{
	    HeapFree(SystemHeap, 0, typeface);
	    continue;
	}

	/* find a family to insert into */
	  
	for( pfr = NULL, fr = fontList; fr; fr = fr->next )
	{
	    if( XFONT_SameFoundryAndFamily(fr->resource, lfd))
		break;
	    pfr = fr;
	}  
	
	if( !fi ) fi = (fontInfo*) HeapAlloc(SystemHeap, 0, sizeof(fontInfo));
	
	if( !LFD_InitFontInfo( fi, lfd, x_pattern[i]) )
	    goto nextfont;
	    
	if( !fr ) /* add new family */
	{
	    n_ff++;
	    fr = (fontResource*) HeapAlloc(SystemHeap, 0, sizeof(fontResource)); 
	    if (fr)
	    {
		memset(fr, 0, sizeof(fontResource));
	      
		fr->resource = (LFD*) HeapAlloc(SystemHeap, 0, sizeof(LFD)); 
		memset(fr->resource, 0, sizeof(LFD));
	      
		TRACE("family: -%s-%s-\n", lfd->foundry, lfd->family );
		fr->resource->foundry = HEAP_strdupA(SystemHeap, 0, lfd->foundry);
		fr->resource->family = HEAP_strdupA(SystemHeap, 0, lfd->family);
		fr->resource->weight = "";

		if( pfr ) pfr->next = fr;
		else fontList = fr;
	    }
	    else
		WARN("Not enough memory for a new font family\n");
	}

	/* check if we already have something better than "fi" */
	
	for( pfi = fr->fi, j = 0; pfi && j <= 0; pfi = pfi->next ) 
	    if( (j = XFONT_IsSubset( pfi, fi )) < 0 ) 
		pfi->fi_flags |= FI_SUBSET; /* superseded by "fi" */
	if( j > 0 ) goto nextfont;
	
	/* add new font instance "fi" to the "fr" font resource */
	
	if( fi->fi_flags & FI_SCALABLE )
	{
	    LFD lfd1;
	    char pxl_string[4], res_string[4]; 
	    /* set scalable font height to get an basis for extrapolation */

	    fi->lfd_height = DEF_SCALABLE_HEIGHT;
	    fi->lfd_resolution = res;

	    sprintf(pxl_string, "%d", fi->lfd_height);
	    sprintf(res_string, "%d", fi->lfd_resolution);
	   
	    lfd1 = *lfd;
	    lfd1.pixel_size = pxl_string;
	    lfd1.point_size = "*";
	    lfd1.resolution_x = res_string;
	    lfd1.resolution_y = res_string;

	    LFD_UnParse(buffer, sizeof buffer, &lfd1);
	    
	    lpstr = buffer;
	}
	else lpstr = x_pattern[i];

	if( (x_fs = TSXLoadQueryFont(display, lpstr)) )
	{	      
	    XFONT_SetFontMetric( fi, fr, x_fs );
	    TSXFreeFont( display, x_fs );

	    XFONT_FixupPointSize(fi);

	    TRACE("\t[% 2ipt] '%s'\n", fi->df.dfPoints, x_pattern[i] );
	      
	    XFONT_CheckFIList( fr, fi, REMOVE_SUBSETS );
	    fi = NULL;	/* preventing reuse */
	}
	else
	{
	    ERR("failed to load %s\n", lpstr );

	    XFONT_CheckFIList( fr, fi, UNMARK_SUBSETS );
	}
    nextfont:
	HeapFree(SystemHeap, 0, lfd);
	HeapFree(SystemHeap, 0, typeface);
    }
    if( fi ) HeapFree(SystemHeap, 0, fi);

    return n_ff;
}

/***********************************************************************
 *           XFONT_ReadCachedMetrics
 *
 * INIT ONLY
 */
static BOOL XFONT_ReadCachedMetrics( int fd, int res, unsigned x_checksum, int x_count )
{
    if( fd >= 0 )
    {
	unsigned u;
	int i, j;

	/* read checksums */
	read( fd, &u, sizeof(unsigned) );
	read( fd, &i, sizeof(int) );

	if( u == x_checksum && i == x_count )
	{
	    off_t length, offset = 3 * sizeof(int);

            /* read total size */
	    read( fd, &i, sizeof(int) );
	    length = lseek( fd, 0, SEEK_END );

	    if( length == (i + offset) )
	    {
		lseek( fd, offset, SEEK_SET );
		fontList = (fontResource*)HeapAlloc( SystemHeap, 0, i);
		if( fontList )
		{
		    fontResource* 	pfr = fontList;
		    fontInfo* 		pfi = NULL;

		    TRACE("Reading cached font metrics:\n");

		    read( fd, fontList, i); /* read all metrics at once */
		    while( offset < length )
		    {
			offset += sizeof(fontResource) + sizeof(fontInfo);
			pfr->fi = pfi = (fontInfo*)(pfr + 1);
			j = 1;
			while( TRUE )
			{
			   if( offset > length ||
			      (int)(pfi->next) != j++ ) goto fail;

			   pfi->df.dfFace = pfr->lfFaceName;
			   if( pfi->fi_flags & FI_SCALABLE )
			   {
			       /* we can pretend we got this font for any resolution */
			       pfi->lfd_resolution = res;
			       XFONT_FixupPointSize(pfi);
			   }
			   pfi->next = pfi + 1;

			   if( j > pfr->fi_count ) break;

			   pfi = pfi->next;
			   offset += sizeof(fontInfo);
			}
			pfi->next = NULL;
			if( pfr->next )
			{
			    pfr->next = (fontResource*)(pfi + 1);
			    pfr = pfr->next;
			} 
			else break;
		    }
		    if( pfr->next == NULL &&
			*(int*)(pfi + 1) == X_FMC_MAGIC )
		    {
			/* read LFD stubs */
			char* lpch = (char*)((int*)(pfi + 1) + 1);
			offset += sizeof(int);
			for( pfr = fontList; pfr; pfr = pfr->next )
			{
			    size_t len = strlen(lpch) + 1;
			    TRACE("\t%s, %i instances\n", lpch, pfr->fi_count );
			    pfr->resource = LFD_Parse(lpch);
			    lpch += len;
			    offset += len;
			    if (offset > length)
			        goto fail;
			}
			close( fd );
			return TRUE;
		    }
		}
	    }
	}
fail:
	if( fontList ) HeapFree( SystemHeap, 0, fontList );
	fontList = NULL;
	close( fd );
    }
    return FALSE;
}

/***********************************************************************
 *           XFONT_WriteCachedMetrics
 *
 * INIT ONLY
 */
static BOOL XFONT_WriteCachedMetrics( int fd, unsigned x_checksum, int x_count, int n_ff )
{
    fontResource* pfr;
    fontInfo* pfi;

    if( fd >= 0 )
    {
        int  i, j, k;
	char buffer[MAX_LFD_LENGTH];

        /* font metrics file:
         *
         * +0000 x_checksum
         * +0004 x_count
         * +0008 total size to load
         * +000C prepackaged font metrics
	 * ...
	 * +...x 	X_FMC_MAGIC
	 * +...x + 4 	LFD stubs
         */

	write( fd, &x_checksum, sizeof(unsigned) );
	write( fd, &x_count, sizeof(int) );

	for( j = i = 0, pfr = fontList; pfr; pfr = pfr->next ) 
	{
	    LFD_UnParse(buffer, sizeof buffer, pfr->resource);
	    i += strlen( buffer) + 1;
	    j += pfr->fi_count;
	}
        i += n_ff * sizeof(fontResource) + j * sizeof(fontInfo) + sizeof(int);
	write( fd, &i, sizeof(int) );

	TRACE("Writing font cache:\n");

	for( pfr = fontList; pfr; pfr = pfr->next )
	{
	    fontInfo fi;

	    TRACE("\t-%s-%s-, %i instances\n", pfr->resource->foundry, pfr->resource->family, pfr->fi_count );

	    i = write( fd, pfr, sizeof(fontResource) );
	    if( i == sizeof(fontResource) ) 
	    {
		for( k = 1, pfi = pfr->fi; pfi; pfi = pfi->next )
		{
		    fi = *pfi;

		    fi.df.dfFace = NULL;
		    fi.next = (fontInfo*)k;	/* loader checks this */

		    j = write( fd, &fi, sizeof(fi) );
		    k++;
		}
	        if( j == sizeof(fontInfo) ) continue;
	    }
	    break;
	}
        if( i == sizeof(fontResource) && j == sizeof(fontInfo) )
	{
	    i = j = X_FMC_MAGIC;
	    write( fd, &i, sizeof(int) );
	    for( pfr = fontList; pfr && i == j; pfr = pfr->next )
	    {
		LFD_UnParse(buffer, sizeof buffer, pfr->resource);
	        i = strlen( buffer ) + 1;
		j = write( fd, buffer, i );
	    }
	}
	close( fd );
	return ( i == j );
    }
    return FALSE;
}

/***********************************************************************
 *           XFONT_CheckIniSection
 *
 * INIT ONLY
 *
 *   Examines wine.conf for old/invalid font entries and recommend changes to
 *   the user.
 *
 *   Revision history
 *        05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 */
static void XFONT_CheckIniCallback(char const *, char const *, void *);

static char const  *fontmsgprologue =
"Wine warning:\n"
"   The following entries in the [fonts] section of the wine.conf file are\n"
"   obsolete or invalid:\n";

static char const  *fontmsgepilogue =
"   These entries should be eliminated or updated.\n"
"   See the documentation/fonts file for more information.\n";

static int XFONT_CheckIniSection(void)
{
    int  found = 0;

    PROFILE_EnumerateWineIniSection("Fonts", &XFONT_CheckIniCallback,
                    (void *)&found);
    if(found)
	MESSAGE(fontmsgepilogue);

    return 1;
}

static void  XFONT_CheckIniCallback(
    char const  *key,
    char const  *value,
    void  *found)
{
    /* Ignore any keys that start with potential comment characters "'", '#',
       or ';'. */
    if(key[0] == '\'' || key[0] == '#' || key[0] == ';' || key[0] == '\0')
	return;

    /* Make sure this is a valid key */
    if((strncasecmp(key, INIAliasSection, 5) == 0) ||
       (strncasecmp(key, INIIgnoreSection, 6) == 0) ||
       (strcasecmp( key, INIDefault) == 0) ||
       (strcasecmp( key, INIDefaultFixed) == 0) ||
       (strcasecmp( key, INIGlobalMetrics) == 0) ||
       (strcasecmp( key, INIResolution) == 0) ||
       (strcasecmp( key, INIDefaultSerif) == 0) ||
       (strcasecmp( key, INIDefaultSansSerif) ==0) )
    {
	/* Valid key; make sure the value doesn't contain a wildcard */
	if(strchr(value, '*')) {
	    if(*(int *)found == 0) {
		MESSAGE(fontmsgprologue);
		++*(int *)found;
	    }
	    MESSAGE("     %s=%s [no wildcards allowed]\n", key, value);
	}
    }
    else {
	/* Not a valid key */
	if(*(int *)found == 0) {
	    MESSAGE(fontmsgprologue);
	    ++*(int *)found;
	}
	
	MESSAGE("     %s=%s [obsolete]\n", key, value);
    }

    return;
}

/***********************************************************************
 *           XFONT_GetPointResolution()
 *
 * INIT ONLY
 *
 * Here we initialize DefResolution which is used in the
 * XFONT_Match() penalty function. We also load the point
 * resolution value (higher values result in larger fonts).
 */
static int XFONT_GetPointResolution( DeviceCaps* pDevCaps )
{
    int i, j, point_resolution, num = 3; 
    int allowed_xfont_resolutions[3] = { 72, 75, 100 };
    int best = 0, best_diff = 65536;

    point_resolution = PROFILE_GetWineIniInt( INIFontSection, INIResolution, 0 );
    if( !point_resolution )
	point_resolution = pDevCaps->logPixelsY;
    else
	pDevCaps->logPixelsX = pDevCaps->logPixelsY = point_resolution;


    /* FIXME We can only really guess at a best DefResolution
     * - this should be configurable
     */
    for( i = best = 0; i < num; i++ )
    {
	j = abs( point_resolution - allowed_xfont_resolutions[i] );
	if( j < best_diff )
	{
	    best = i;
	    best_diff = j;
	}
    }
    DefResolution = allowed_xfont_resolutions[best];

    /* FIXME - do win95,nt40,... do this as well ? */
    if (TWEAK_WineLook == WIN98_LOOK)
    {
	/* Lie about the screen size, so that eg MM_LOMETRIC becomes MM_logical_LOMETRIC */
	int denom;
	denom = pDevCaps->logPixelsX * 10;
	pDevCaps->horzSize = (pDevCaps->horzRes * 254 + (denom>>1)) / denom;
	denom = pDevCaps->logPixelsY * 10;
	pDevCaps->vertSize = (pDevCaps->vertRes * 254 + (denom>>1)) / denom;
    }

    return point_resolution;
}


/***********************************************************************
 *            XFONT_Match
 *
 * Compute the matching score between the logical font and the device font.
 *
 * contributions from highest to lowest:
 *      charset
 *      fixed pitch
 *      height
 *      family flags (only when the facename is not present)
 *      width
 *      weight, italics, underlines, strikeouts
 *
 * NOTE: you can experiment with different penalty weights to see what happens.
 * http://premium.microsoft.com/msdn/library/techart/f365/f36b/f37b/d38b/sa8bf.htm
 */
static UINT XFONT_Match( fontMatch* pfm )
{
   fontInfo*    pfi = pfm->pfi;         /* device font to match */
   LPLOGFONT16  plf = pfm->plf;         /* wanted logical font */
   UINT       penalty = 0;
   BOOL       bR6 = pfm->flags & FO_MATCH_XYINDEP;    /* from TextCaps */
   BOOL       bScale = pfi->fi_flags & FI_SCALABLE;
   int d = 0, height;

   TRACE("\t[ %-2ipt h=%-3i w=%-3i %s%s]\n", pfi->df.dfPoints,
		 pfi->df.dfPixHeight, pfi->df.dfAvgWidth,
		(pfi->df.dfWeight > FW_NORMAL) ? "Bold " : "Normal ",
		(pfi->df.dfItalic) ? "Italic" : "" );

   pfm->flags &= FO_MATCH_MASK;

/* Charset */
   if( plf->lfCharSet == DEFAULT_CHARSET )
   {
       if( (pfi->df.dfCharSet != ANSI_CHARSET) && (pfi->df.dfCharSet != DEFAULT_CHARSET) ) 
	    penalty += 0x200;
   }
   else if (plf->lfCharSet != pfi->df.dfCharSet) penalty += 0x200;

/* Height */
   height = -1;
   {
       if( plf->lfHeight > 0 )
       {
	   int h = pfi->df.dfPixHeight;
	   d = h - plf->lfHeight;
	   height = plf->lfHeight;
       }
       else
       {
	   int h = pfi->df.dfPixHeight - pfi->df.dfInternalLeading;
	   if (h)
	   {
	       d = h + plf->lfHeight;
	       height = (-plf->lfHeight * pfi->df.dfPixHeight) / h;
	   }
	   else
	   {
	       ERR("PixHeight == InternalLeading\n");
	       penalty += 0x1000; /* dont want this */
	   }
       }
   }

   if( height == 0 )
       pfm->height = 1; /* Very small */
   else if( d )
   {
       if( bScale )
	   pfm->height = height;
       else if( (plf->lfQuality != PROOF_QUALITY) && bR6 )
       {
	   if( d > 0 ) 	/* do not shrink raster fonts */
	   {
	       pfm->height = pfi->df.dfPixHeight;
	       penalty += (pfi->df.dfPixHeight - height) * 0x4;
	   }
	   else 	/* expand only in integer multiples */
	   {
	       pfm->height = height - height%pfi->df.dfPixHeight;
	       penalty += (height - pfm->height + 1) * height / pfi->df.dfPixHeight;
	   }
       }
       else /* can't be scaled at all */
       {
	   if( plf->lfQuality != PROOF_QUALITY) pfm->flags |= FO_SYNTH_HEIGHT;
	   pfm->height = pfi->df.dfPixHeight;
	   penalty += (d > 0)? d * 0x8 : -d * 0x10;
       }
   }
   else
       pfm->height = pfi->df.dfPixHeight;

/* Pitch and Family */
   if( pfm->flags & FO_MATCH_PAF ) {
       int family = plf->lfPitchAndFamily & FF_FAMILY;

       /* TMPF_FIXED_PITCH means exactly the opposite */
       if( plf->lfPitchAndFamily & FIXED_PITCH ) {
	   if( pfi->df.dfPitchAndFamily & TMPF_FIXED_PITCH ) penalty += 0x100;
       } else /* Variable is the default */
	   if( !(pfi->df.dfPitchAndFamily & TMPF_FIXED_PITCH) ) penalty += 0x2;

       if (family != FF_DONTCARE && family != (pfi->df.dfPitchAndFamily & FF_FAMILY) )
	   penalty += 0x10;
   }

/* Width */
   if( plf->lfWidth )
   {
       int h;
       if( bR6 || bScale ) h = 0; 
       else
       {
	   /* FIXME: not complete */

	   pfm->flags |= FO_SYNTH_WIDTH;
	   h = abs(plf->lfWidth - (pfm->height * pfi->df.dfAvgWidth)/pfi->df.dfPixHeight);
       }
       penalty += h * ( d ) ? 0x2 : 0x1 ;
   }
   else if( !(pfi->fi_flags & FI_NORMAL) ) penalty++;

/* Weight */
   if( plf->lfWeight != FW_DONTCARE )
   {
       penalty += abs(plf->lfWeight - pfi->df.dfWeight) / 40;
       if( plf->lfWeight > pfi->df.dfWeight ) pfm->flags |= FO_SYNTH_BOLD;
   } else if( pfi->df.dfWeight >= FW_BOLD ) penalty++;	/* choose normal by default */

/* Italic */
   if( plf->lfItalic != pfi->df.dfItalic )
   {
       penalty += 0x4;
       pfm->flags |= FO_SYNTH_ITALIC;
   }
/* Underline */
   if( plf->lfUnderline ) pfm->flags |= FO_SYNTH_UNDERLINE;

/* Strikeout */   
   if( plf->lfStrikeOut ) pfm->flags |= FO_SYNTH_STRIKEOUT;


   if( penalty && !bScale && pfi->lfd_resolution != DefResolution ) 
       penalty++;

   TRACE("  returning %i\n", penalty );

   return penalty;
}

/***********************************************************************
 *            XFONT_MatchFIList
 *
 * Scan a particular font resource for the best match.
 */
static UINT XFONT_MatchFIList( fontMatch* pfm )
{
  BOOL        skipRaster = (pfm->flags & FO_MATCH_NORASTER);
  UINT        current_score, score = (UINT)(-1);
  fontMatch   fm = *pfm;

  for( fm.pfi = pfm->pfr->fi; fm.pfi && score; fm.pfi = fm.pfi->next)
  {
     if( skipRaster && !(fm.pfi->fi_flags & FI_SCALABLE) )
         continue;

     current_score = XFONT_Match( &fm );
     if( score > current_score )
     {
	*pfm = fm;
        score = current_score;
     }
  }
  return score;
}

/***********************************************************************
 *          XFONT_MatchDeviceFont
 *
 * Scan font resource tree.
 *
 */
static void XFONT_MatchDeviceFont( fontResource* start, fontMatch* pfm)
{
    fontMatch           fm = *pfm;
    UINT          current_score, score = (UINT)(-1);
    fontResource**	ppfr;
    
    TRACE("(%u) '%s' h=%i weight=%i %s\n",
	  pfm->plf->lfCharSet, pfm->plf->lfFaceName, pfm->plf->lfHeight, 
	  pfm->plf->lfWeight, (pfm->plf->lfItalic) ? "Italic" : "" );

    pfm->pfi = NULL;
    if( fm.plf->lfFaceName[0] ) 
    {
        fm.pfr = XFONT_FindFIList( start, fm.plf->lfFaceName);
	if( fm.pfr ) /* match family */
	{
	    TRACE("found facename '%s'\n", fm.pfr->lfFaceName );
	    
	    if( fm.pfr->fr_flags & FR_REMOVED )
		fm.pfr = 0;
	    else
	    {
		XFONT_MatchFIList( &fm );
		*pfm = fm;
		if (pfm->pfi)
		    return;
	    }
	}
    }

    /* match all available fonts */

    fm.flags |= FO_MATCH_PAF;
    for( ppfr = &fontList; *ppfr && score; ppfr = &(*ppfr)->next )
    {
	if( (*ppfr)->fr_flags & FR_REMOVED )
	{
	    if( (*ppfr)->fo_count == 0 )
		XFONT_RemoveFontResource( ppfr );
	    continue;
	}
	
	fm.pfr = *ppfr;
	
	TRACE("%s\n", fm.pfr->lfFaceName );
	    
	current_score = XFONT_MatchFIList( &fm );
	if( current_score < score )
	{
	    score = current_score;
	    *pfm = fm;
	}
    }
}


/***********************************************************************
 *           X Font Cache
 */
static void XFONT_GrowFreeList(int start, int end)
{
   /* add all entries from 'start' up to and including 'end' */

   memset( fontCache + start, 0, (end - start + 1) * sizeof(fontObject) );

   fontCache[end].lru = fontLF;
   fontCache[end].count = -1;
   fontLF = start;
   while( start < end )
   {
	fontCache[start].count = -1;
	fontCache[start].lru = start + 1;
	start++;
   } 
}

static fontObject* XFONT_LookupCachedFont( const LPLOGFONT16 plf, UINT16* checksum )
{
    UINT16	cs = __lfCheckSum( plf );
    int		i = fontMRU, prev = -1;
   
   *checksum = cs;
    while( i >= 0 )
    {
	if( fontCache[i].lfchecksum == cs &&
	  !(fontCache[i].fo_flags & FO_REMOVED) )
	{
	    /* FIXME: something more intelligent here ? */

	    if( !memcmp( plf, &fontCache[i].lf,
			 sizeof(LOGFONT16) - LF_FACESIZE ) &&
		!strcmp( plf->lfFaceName, fontCache[i].lf.lfFaceName) )
	    {
		/* remove temporarily from the lru list */
		if( prev >= 0 )
		    fontCache[prev].lru = fontCache[i].lru;
		else 
		    fontMRU = (INT16)fontCache[i].lru;
		return (fontCache + i);
	    }
	}
	prev = i;
	i = (INT16)fontCache[i].lru;
    }
    return NULL;
}

static fontObject* XFONT_GetCacheEntry(void)
{
    int		i;

    if( fontLF == -1 )
    {
	int	prev_i, prev_j, j;

	TRACE("font cache is full\n");

	/* lookup the least recently used font */

	for( prev_i = prev_j = j = -1, i = fontMRU; i >= 0; i = (INT16)fontCache[i].lru )
	{
	    if( fontCache[i].count <= 0 &&
	      !(fontCache[i].fo_flags & FO_SYSTEM) )
	    {
		prev_j = prev_i;
		j = i;
	    }
	    prev_i = i;
	}

	if( j >= 0 )	/* unload font */
	{
	    /* detach from the lru list */

	    TRACE("\tfreeing entry %i\n", j );

	    fontCache[j].fr->fo_count--;

	    if( prev_j >= 0 )
		fontCache[prev_j].lru = fontCache[j].lru;
	    else fontMRU = (INT16)fontCache[j].lru;

	    /* FIXME: lpXForm, lpPixmap */
	    if(fontCache[j].lpX11Trans)
	        HeapFree( SystemHeap, 0, fontCache[j].lpX11Trans );

	    TSXFreeFont( display, fontCache[j].fs );

	    memset( fontCache + j, 0, sizeof(fontObject) );
	    return (fontCache + j);
	}
	else		/* expand cache */
	{
	    fontObject*	newCache;

	    prev_i = fontCacheSize + FONTCACHE;

	    TRACE("\tgrowing font cache from %i to %i\n", fontCacheSize, prev_i );

	    if( (newCache = (fontObject*)HeapReAlloc(SystemHeap, 0,  
						     fontCache, prev_i)) )
	    {
		i = fontCacheSize;
		fontCacheSize  = prev_i;
		fontCache = newCache;
		XFONT_GrowFreeList( i, fontCacheSize - 1);
	    } 
	    else return NULL;
	}
    }

    /* detach from the free list */

    i = fontLF;
    fontLF = (INT16)fontCache[i].lru;
    fontCache[i].count = 0;
    return (fontCache + i);
}

static int XFONT_ReleaseCacheEntry(const fontObject* pfo)
{
    UINT	u = (UINT)(pfo - fontCache);

    if( u < fontCacheSize ) return (--fontCache[u].count);
    return -1;
}

/***********************************************************************
 *           X11DRV_FONT_Init
 *
 * Initialize font resource list and allocate font cache.
 */
BOOL X11DRV_FONT_Init( DeviceCaps* pDevCaps )
{
  char**    x_pattern;
  unsigned  x_checksum;
  int       i,res, x_count, fd, buf_size;
  char      *buffer;

  XFONT_CheckIniSection();

  res = XFONT_GetPointResolution( pDevCaps );
      
  x_pattern = TSXListFonts(display, "*", MAX_FONTS, &x_count );

  TRACE("Font Mapper: initializing %i fonts [logical dpi=%i, default dpi=%i]\n", 
				    x_count, res, DefResolution);
  if (x_count == MAX_FONTS)      
      MESSAGE("There may be more fonts available - try increasing the value of MAX_FONTS\n");

  for( i = x_checksum = 0; i < x_count; i++ )
  {
     int j;
#if 0
     printf("%i\t: %s\n", i, x_pattern[i] );
#endif

     j = strlen( x_pattern[i] );
     if( j ) x_checksum ^= __genericCheckSum( x_pattern[i], j );
  }
  x_checksum |= X_PFONT_MAGIC;
  buf_size = 128;
  buffer = HeapAlloc( SystemHeap, 0, buf_size );

  /* deal with systemwide font metrics cache */

  if( PROFILE_GetWineIniString( INIFontSection, INIGlobalMetrics, "", buffer, buf_size ) )
  {
      fd = open( buffer, O_RDONLY );
      XFONT_ReadCachedMetrics(fd, DefResolution, x_checksum, x_count);
  }
  if (fontList == NULL)
  {
      /* try per-user */
      buffer = XFONT_UserMetricsCache( buffer, &buf_size );
      if( buffer[0] )
      {
	  fd = open( buffer, O_RDONLY );
	  XFONT_ReadCachedMetrics(fd, DefResolution, x_checksum, x_count);
      }
  }

  if( fontList == NULL )	/* build metrics from scratch */
  {
      int n_ff = XFONT_BuildMetrics(x_pattern, DefResolution, x_checksum, x_count);
      if( buffer[0] )	 /* update cached metrics */
      {
	  fd = open( buffer, O_CREAT | O_TRUNC | O_RDWR, 0644 ); /* -rw-r--r-- */
	  if( XFONT_WriteCachedMetrics( fd, x_checksum, x_count, n_ff ) == FALSE )
	  {
	      WARN("Unable to write to fontcache '%s'\n", buffer);
	      if( fd >= 0) remove( buffer );	/* couldn't write entire file */
	  }
      }
  }

  TSXFreeFontNames(x_pattern);

  /* check if we're dealing with X11 R6 server */
  {
      XFontStruct*  x_fs;
      strcpy(buffer, "-*-*-*-*-normal-*-[12 0 0 12]-*-72-*-*-*-iso8859-1");
      if( (x_fs = TSXLoadQueryFont(display, buffer)) )
      {
	  XTextCaps |= TC_SF_X_YINDEP;
	  TSXFreeFont(display, x_fs);
      }
  }
  HeapFree(SystemHeap, 0, buffer);

  XFONT_WindowsNames();
  XFONT_LoadAliases();
  XFONT_LoadDefaults();
  XFONT_LoadIgnores();

  InitializeCriticalSection( &crtsc_fonts_X11 );
  MakeCriticalSectionGlobal( &crtsc_fonts_X11 );

  /* fontList initialization is over, allocate X font cache */

  fontCache = (fontObject*) HeapAlloc(SystemHeap, 0, fontCacheSize * sizeof(fontObject));
  XFONT_GrowFreeList(0, fontCacheSize - 1);

  TRACE("done!\n");

  /* update text caps parameter */

  pDevCaps->textCaps = XTextCaps;

  RAW_ASCENT  = TSXInternAtom(display, "RAW_ASCENT", TRUE);
  RAW_DESCENT = TSXInternAtom(display, "RAW_DESCENT", TRUE);
  
  return TRUE;
}

/**********************************************************************
 *	XFONT_SetX11Trans
 */
static BOOL XFONT_SetX11Trans( fontObject *pfo )
{
  char *fontName;
  Atom nameAtom;
  LFD* lfd;

  TSXGetFontProperty( pfo->fs, XA_FONT, &nameAtom );
  fontName = TSXGetAtomName( display, nameAtom );
  lfd = LFD_Parse(fontName);
  if (!lfd)
  {
      TSXFree(fontName);
      return FALSE;
  }

  if (lfd->pixel_size[0] != '[') {
      HeapFree(SystemHeap, 0, lfd);
      TSXFree(fontName);
      return FALSE;
  }

#define PX pfo->lpX11Trans

  sscanf(lfd->pixel_size, "[%f%f%f%f]", &PX->a, &PX->b, &PX->c, &PX->d);
  TSXFree(fontName);
  HeapFree(SystemHeap, 0, lfd);

  TSXGetFontProperty( pfo->fs, RAW_ASCENT, &PX->RAW_ASCENT );
  TSXGetFontProperty( pfo->fs, RAW_DESCENT, &PX->RAW_DESCENT );

  PX->pixelsize = hypot(PX->a, PX->b);
  PX->ascent = PX->pixelsize / 1000.0 * PX->RAW_ASCENT;
  PX->descent = PX->pixelsize / 1000.0 * PX->RAW_DESCENT;

  TRACE("[%f %f %f %f] RA = %ld RD = %ld\n",
	PX->a, PX->b, PX->c, PX->d,
	PX->RAW_ASCENT, PX->RAW_DESCENT);

#undef PX
  return TRUE;
}

/***********************************************************************
 *           X Device Font Objects
 */
static X_PHYSFONT XFONT_RealizeFont( const LPLOGFONT16 plf, LPCSTR* faceMatched)
{
    UINT16	checksum;
    INT         index = 0;
    fontObject* pfo;

    pfo = XFONT_LookupCachedFont( plf, &checksum );
    if( !pfo )
    {
	fontMatch	fm;
	INT		i;

	fm.pfr = NULL;
	fm.pfi = NULL;
	fm.height = 0;
	fm.flags = 0;
	fm.plf = plf;

	if( XTextCaps & TC_SF_X_YINDEP ) fm.flags = FO_MATCH_XYINDEP;

	/* allocate new font cache entry */

	if( (pfo = XFONT_GetCacheEntry()) )
	{
	    /* initialize entry and load font */
	    char lpLFD[MAX_LFD_LENGTH];
	    UINT uRelaxLevel = 0;

	    if(abs(plf->lfHeight) > MAX_FONT_SIZE) {
		ERR(
  "plf->lfHeight = %d, Creating a 100 pixel font and rescaling metrics \n", 
		    plf->lfHeight);
		pfo->rescale = fabs(plf->lfHeight / 100.0);
		if(plf->lfHeight > 0) plf->lfHeight = 100;
		else plf->lfHeight = -100;
	    } else
		pfo->rescale = 1.0;
	    
	    XFONT_MatchDeviceFont( fontList, &fm );
	    pfo->fr = fm.pfr;
	    pfo->fi = fm.pfi;
	    pfo->fr->fo_count++;
	    pfo->fo_flags = fm.flags & ~FO_MATCH_MASK;

	    pfo->lf = *plf;
	    pfo->lfchecksum = checksum;

	    do
	    {
		LFD_ComposeLFD( pfo, fm.height, lpLFD, uRelaxLevel++ );
		if( (pfo->fs = TSXLoadQueryFont( display, lpLFD )) ) break;
	    } while( uRelaxLevel );

		
	    if(pfo->lf.lfEscapement != 0) {
		pfo->lpX11Trans = HeapAlloc(SystemHeap, 0, sizeof(XFONTTRANS));
		if(!XFONT_SetX11Trans( pfo )) {
		    HeapFree(SystemHeap, 0, pfo->lpX11Trans);
		    pfo->lpX11Trans = NULL;
		}
	    }
	    XFONT_GetLeading( &pfo->fi->df, pfo->fs,
			      &pfo->foInternalLeading, NULL, pfo->lpX11Trans );
	    pfo->foAvgCharWidth = (INT16)XFONT_GetAvgCharWidth(&pfo->fi->df, pfo->fs, pfo->lpX11Trans );
	    pfo->foMaxCharWidth = (INT16)XFONT_GetMaxCharWidth(pfo->fs, pfo->lpX11Trans);
	    
	    /* FIXME: If we've got a soft font or
	     * there are FO_SYNTH_... flags for the
	     * non PROOF_QUALITY request, the engine
	     * should rasterize characters into mono
	     * pixmaps and store them in the pfo->lpPixmap
	     * array (pfo->fs should be updated as well).
	     * array (pfo->fs should be updated as well).
	     * X11DRV_ExtTextOut() must be heavily modified 
	     * to support pixmap blitting and FO_SYNTH_...
	     * styles.
	     */

	    pfo->lpPixmap = NULL;
	}
	
	if( !pfo ) /* couldn't get a new entry, get one of the cached fonts */
	{
	    UINT		current_score, score = (UINT)(-1);

	    i = index = fontMRU; 
	    fm.flags |= FO_MATCH_PAF;
            do
            {
		pfo = fontCache + i;
		fm.pfr = pfo->fr; fm.pfi = pfo->fi;

                current_score = XFONT_Match( &fm );
                if( current_score < score ) index = i;

	        i =  pfo->lru;
            } while( i >= 0 );
	    pfo = fontCache + index;
	    goto END;
	}
    }

    /* attach at the head of the lru list */
    pfo->lru = fontMRU;
    index = fontMRU = (pfo - fontCache);
 
END:
    pfo->count++;

    TRACE("physfont %i\n", index);
    *faceMatched = pfo->fi->df.dfFace;

    return (X_PHYSFONT)(X_PFONT_MAGIC | index);
}

/***********************************************************************
 *           XFONT_GetFontObject
 */
fontObject* XFONT_GetFontObject( X_PHYSFONT pFont )
{
    if( CHECK_PFONT(pFont) ) return __PFONT(pFont);
    return NULL;
}

/***********************************************************************
 *           XFONT_GetFontStruct
 */
XFontStruct* XFONT_GetFontStruct( X_PHYSFONT pFont )
{
    if( CHECK_PFONT(pFont) ) return __PFONT(pFont)->fs;
    return NULL;
}

/***********************************************************************
 *           XFONT_GetFontInfo
 */
LPIFONTINFO16 XFONT_GetFontInfo( X_PHYSFONT pFont )
{
    if( CHECK_PFONT(pFont) ) return &(__PFONT(pFont)->fi->df);
    return NULL;
}



/* X11DRV Interface ****************************************************
 *                                                                     *
 * Exposed via the dc->funcs dispatch table.                           *
 *                                                                     *
 ***********************************************************************/
/***********************************************************************
 *           X11DRV_FONT_SelectObject
 */
HFONT X11DRV_FONT_SelectObject( DC* dc, HFONT hfont, FONTOBJ* font )
{
    HFONT hPrevFont = 0;
    LOGFONT16 lf;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    EnterCriticalSection( &crtsc_fonts_X11 );

    if( CHECK_PFONT(physDev->font) ) 
        XFONT_ReleaseCacheEntry( __PFONT(physDev->font) );

    lf = font->logfont;

    /* Make sure we don't change the sign when converting to device coords */
    /* FIXME - check that the other drivers do this correctly */
    if (lf.lfWidth)
    {
	int vpt = abs(dc->vportExtX);
	int wnd = abs(dc->wndExtX);
	lf.lfWidth = (abs(lf.lfWidth) * vpt + (wnd>>1))/wnd;
	if (lf.lfWidth == 0)
	    lf.lfWidth = 1; /* Minimum width */
    }
    if (lf.lfHeight)
    {
	int vpt = abs(dc->vportExtY);
	int wnd = abs(dc->wndExtY);
	if (lf.lfHeight > 0)
	    lf.lfHeight = (lf.lfHeight * vpt + (wnd>>1))/wnd;
	else
	    lf.lfHeight = (lf.lfHeight * vpt - (wnd>>1))/wnd;

	if (lf.lfHeight == 0)
	    lf.lfHeight = MIN_FONT_SIZE;
    }
    else
	lf.lfHeight = -(DEF_POINT_SIZE * dc->w.devCaps->logPixelsY + (72>>1)) / 72;
    
    {
	/* Fixup aliases before passing to RealizeFont */
        /* alias = Windows name in the alias table */
	LPCSTR alias = XFONT_UnAlias( lf.lfFaceName );
	LPCSTR faceMatched;

	TRACE("hfont=%04x\n", hfont); /* to connect with the trace from RealizeFont */
	physDev->font = XFONT_RealizeFont( &lf, &faceMatched );

	/* set face to the requested facename if it matched 
	 * so that GetTextFace can get the correct face name
	 */
	if (alias && !strcmp(faceMatched, lf.lfFaceName))
	    strcpy( font->logfont.lfFaceName, alias );
	else
	    strcpy( font->logfont.lfFaceName, faceMatched );
    }

    hPrevFont = dc->w.hFont;
    dc->w.hFont = hfont;

    LeaveCriticalSection( &crtsc_fonts_X11 );

    return hPrevFont;
}


/***********************************************************************
 *
 *           X11DRV_EnumDeviceFonts
 */
BOOL	X11DRV_EnumDeviceFonts( DC* dc, LPLOGFONT16 plf, 
				        DEVICEFONTENUMPROC proc, LPARAM lp )
{
    ENUMLOGFONTEX16	lf;
    NEWTEXTMETRIC16	tm;
    fontResource*	pfr = fontList;
    BOOL	  	b, bRet = 0;

    if( plf->lfFaceName[0] )
    {
	/* enum all entries in this resource */
	pfr = XFONT_FindFIList( pfr, plf->lfFaceName );
	if( pfr )
	{
	    fontInfo*	pfi;
	    for( pfi = pfr->fi; pfi; pfi = pfi->next )
	    {
		/* Note: XFONT_GetFontMetric() will have to
		   release the crit section, font list will
		   have to be retraversed on return */

		if( (b = (*proc)( &lf, &tm, 
			XFONT_GetFontMetric( pfi, &lf, &tm ), lp )) )
		     bRet = b;
		else break;
	    }
	}
    }
    else /* enum first entry in each resource */
	for( ; pfr ; pfr = pfr->next )
	{
            if(pfr->fi)
            {
		if( (b = (*proc)( &lf, &tm, 
			XFONT_GetFontMetric( pfr->fi, &lf, &tm ), lp )) )
		     bRet = b;
		else break;
            }
	}
    return bRet;
}


/***********************************************************************
 *           X11DRV_GetTextExtentPoint
 */
BOOL X11DRV_GetTextExtentPoint( DC *dc, LPCWSTR str, INT count,
                                  LPSIZE size )
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    fontObject* pfo = XFONT_GetFontObject( physDev->font );

    TRACE("%s %d\n", debugstr_wn(str,count), count);
    if( pfo ) {
        if( !pfo->lpX11Trans ) {
	    int dir, ascent, descent, i;
	    XCharStruct info;
	    XChar2b *p = HeapAlloc( GetProcessHeap(), 0,
				    count * sizeof(XChar2b) );
	    for(i = 0; i < count; i++) {
	        p[i].byte1 = str[i] >> 8;
		p[i].byte2 = str[i] & 0xff;
	    }
	    TSXTextExtents16( pfo->fs, p, count, &dir, &ascent, &descent, &info );
	    size->cx = abs((info.width + dc->w.breakRem + count * 
			    dc->w.charExtra) * dc->wndExtX / dc->vportExtX);
	    size->cy = abs((pfo->fs->ascent + pfo->fs->descent) * 
			   dc->wndExtY / dc->vportExtY);
	    HeapFree( GetProcessHeap(), 0, p );
	} else {
	    INT i;
	    float x = 0.0, y = 0.0;
	    for(i = 0; i < count; i++) {
	        x += pfo->fs->per_char ? 
	   pfo->fs->per_char[str[i] - pfo->fs->min_char_or_byte2].attributes : 
	   pfo->fs->min_bounds.attributes;
	    }
	    y = pfo->lpX11Trans->RAW_ASCENT + pfo->lpX11Trans->RAW_DESCENT;
	    TRACE("x = %f y = %f\n", x, y);
	    x *= pfo->lpX11Trans->pixelsize / 1000.0;
	    y *= pfo->lpX11Trans->pixelsize / 1000.0; 
	    size->cx = fabs((x + dc->w.breakRem + count * dc->w.charExtra) *
			     dc->wndExtX / dc->vportExtX);
	    size->cy = fabs(y * dc->wndExtY / dc->vportExtY);
	}
	size->cx *= pfo->rescale;
	size->cy *= pfo->rescale;
	return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           X11DRV_GetTextMetrics
 */
BOOL X11DRV_GetTextMetrics(DC *dc, TEXTMETRICA *metrics)
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    if( CHECK_PFONT(physDev->font) )
    {
	fontObject* pfo = __PFONT(physDev->font);
	XFONT_GetTextMetrics( pfo, metrics );

	return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           X11DRV_GetCharWidth
 */
BOOL X11DRV_GetCharWidth( DC *dc, UINT firstChar, UINT lastChar,
                            LPINT buffer )
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    fontObject* pfo = XFONT_GetFontObject( physDev->font );

    if( pfo )
    {
	int i;

	if (pfo->fs->per_char == NULL)
	    for (i = firstChar; i <= lastChar; i++)
  	        if(pfo->lpX11Trans)
		    *buffer++ = pfo->fs->min_bounds.attributes *
		      pfo->lpX11Trans->pixelsize / 1000.0 * pfo->rescale;
		else
		    *buffer++ = pfo->fs->min_bounds.width * pfo->rescale;
	else
	{
	    XCharStruct *cs, *def;
	    static XCharStruct	__null_char = { 0, 0, 0, 0, 0, 0 };

	    CI_GET_CHAR_INFO(pfo->fs, pfo->fs->default_char, &__null_char,
			     def);

	    for (i = firstChar; i <= lastChar; i++)
	    {
		if (i >= pfo->fs->min_char_or_byte2 && 
		    i <= pfo->fs->max_char_or_byte2)
		{
		    cs = &pfo->fs->per_char[(i - pfo->fs->min_char_or_byte2)]; 
		    if (CI_NONEXISTCHAR(cs)) cs = def; 
  		} else cs = def;
		if(pfo->lpX11Trans)
		    *buffer++ = MAX(cs->attributes, 0) *
		      pfo->lpX11Trans->pixelsize / 1000.0 * pfo->rescale;
		else
		    *buffer++ = MAX(cs->width, 0 ) * pfo->rescale;
	    }
	}

	return TRUE;
    }
    return FALSE;
}

#endif /* !defined(X_DISPLAY_MISSING) */

/***********************************************************************
 *								       *
 *           Font Resource API					       *
 *								       *
 ***********************************************************************/
/***********************************************************************
 *           AddFontResource16    (GDI.119)
 *
 *  Can be either .FON, or .FNT, or .TTF, or .FOT font file.
 *
 *  FIXME: Load header and find the best-matching font in the fontList;
 * 	   fixup dfPoints if all metrics are identical, otherwise create
 *	   new fontAlias. When soft font support is ready this will
 *	   simply create a new fontResource ('filename' will go into
 *	   the pfr->resource field) with FR_SOFTFONT/FR_SOFTRESOURCE 
 *	   flag set. 
 */
INT16 WINAPI AddFontResource16( LPCSTR filename )
{
    return AddFontResourceA( filename );
}


/***********************************************************************
 *           AddFontResource32A    (GDI32.2)
 */
INT WINAPI AddFontResourceA( LPCSTR str )
{
    FIXME("(%s): stub\n", debugres_a(str));
    return 1;
}


/***********************************************************************
 *           AddFontResource32W    (GDI32.4)
 */
INT WINAPI AddFontResourceW( LPCWSTR str )
{
    FIXME("(%s): stub\n", debugres_w(str) );
    return 1;
}

/***********************************************************************
 *           RemoveFontResource16    (GDI.136)
 */
BOOL16 WINAPI RemoveFontResource16( SEGPTR str )
{
    FIXME("(%s): stub\n",	debugres_a(PTR_SEG_TO_LIN(str)));
    return TRUE;
}


/***********************************************************************
 *           RemoveFontResource32A    (GDI32.284)
 */
BOOL WINAPI RemoveFontResourceA( LPCSTR str )
{
/*  This is how it should look like */
/*
    fontResource** ppfr;
    BOOL32 retVal = FALSE;

    EnterCriticalSection( &crtsc_fonts_X11 );
    for( ppfr = &fontList; *ppfr; ppfr = &(*ppfr)->next )
	 if( !strcasecmp( (*ppfr)->lfFaceName, str ) )
	 {
	     if(((*ppfr)->fr_flags & (FR_SOFTFONT | FR_SOFTRESOURCE)) &&
		 (*ppfr)->hOwnerProcess == GetCurrentProcess() )
	     {
		 if( (*ppfr)->fo_count )
		     (*ppfr)->fr_flags |= FR_REMOVED;
		 else
		     XFONT_RemoveFontResource( ppfr );
	     }
	     retVal = TRUE;
	 }
    LeaveCriticalSection( &crtsc_fonts_X11 );
    return retVal;
 */
    FIXME("(%s): stub\n", debugres_a(str));
    return TRUE;
}


/***********************************************************************
 *           RemoveFontResource32W    (GDI32.286)
 */
BOOL WINAPI RemoveFontResourceW( LPCWSTR str )
{
    FIXME("(%s): stub\n", debugres_w(str) );
    return TRUE;
}

