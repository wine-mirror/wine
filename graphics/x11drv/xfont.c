/*
 * X11 physical font objects
 *
 * Copyright 1997 Alex Korobka
 *
 * TODO: Mapping algorithm tweaks, FO_SYNTH_... flags (ExtTextOut() will
 *	 have to be changed for that), dynamic font loading (FreeType).
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ts_xlib.h"
#include <X11/Xatom.h>
#include <math.h>
#include "heap.h"
#include "options.h"
#include "x11font.h"
#include "font.h"
#include "debug.h"

#define X_PFONT_MAGIC		(0xFADE0000)
#define X_FMC_MAGIC		(0x0000CAFE)

#define MAX_FONT_FAMILIES	64
#define MAX_LFD_LENGTH		128

#define REMOVE_SUBSETS		1
#define UNMARK_SUBSETS		0

#define DEF_SCALABLE_HEIGHT	24
#define DEF_SCALABLE_DP		240

#define FF_FAMILY       (FF_MODERN | FF_SWISS | FF_ROMAN | FF_DECORATIVE | FF_SCRIPT)

typedef struct __fontAlias
{
  LPSTR			faTypeFace;
  LPSTR			faAlias;
  struct __fontAlias*	next;
} fontAlias;

typedef struct
{
  LPSTR                 fatResource;
  LPSTR                 fatAlias;
} aliasTemplate;

/* Font alias table - these 2 aliases are always present */

static fontAlias __aliasTable[2] = { 
			{ "Helvetica", "Helv", &__aliasTable[1] },
			{ "Times", "Tms Rmn", NULL } 
			};

static fontAlias *aliasTable = __aliasTable;

/* Optional built-in aliases, they are installed only when X
 * cannot supply us with original MS fonts */

static int	     faTemplateNum = 4;
static aliasTemplate faTemplate[4] = {
			  { "-adobe-helvetica-", "MS Sans Serif" },
			  { "-bitstream-charter-", "MS Serif" },
			  { "-adobe-times-", "Times New Roman" },
			  { "-adobe-helvetica-", "Arial" }
			  };

/* Charset translation table, cp125-.. encoded fonts are produced by
 * the fnt2bdf */

static int	 numCPTranslation = 8;
static BYTE	 CPTranslation[] = { EE_CHARSET,	/* cp125-0 */
				     RUSSIAN_CHARSET,	/* cp125-1 */
				     ANSI_CHARSET,	/* cp125-2 */
				     GREEK_CHARSET,	/* cp125-3 */
				     TURKISH_CHARSET,	/* cp125-4 */
				     HEBREW_CHARSET,	/* cp125-5 */
				     ARABIC_CHARSET,	/* cp125-6 */
				     BALTIC_CHARSET	/* cp125-7 */
				   }; 

UINT16			XTextCaps = TC_OP_CHARACTER | TC_OP_STROKE | TC_CP_STROKE |
				    TC_SA_DOUBLE | TC_SA_INTEGER | TC_SA_CONTIN |
			 	    TC_UA_ABLE | TC_SO_ABLE | TC_RA_ABLE;

			/* X11R6 adds TC_SF_X_YINDEP, maybe more... */

static const char*	INIWinePrefix = "/.wine";
static const char*	INIFontMetrics = "/.cachedmetrics";
static const char*	INIFontSection = "fonts";
static const char*	INISubSection = "Alias";
static const char*	INIDefault = "Default";
static const char*	INIDefaultFixed = "DefaultFixed";
static const char*	INIResolution = "Resolution";
static const char*	INIGlobalMetrics = "FontMetrics";

static const char*	LFDSeparator = "*-";
static const char*	localMSEncoding = "cp125-";
static const char*	iso8859Encoding = "iso8859-";
static const char*	iso646Encoding = "iso646.1991-";
static const char*	ansiEncoding = "ansi-";
static unsigned		DefResolution = 0;

static fontResource*	fontList = NULL;
static fontObject*      fontCache = NULL;		/* array */
static int		fontCacheSize = FONTCACHE;
static int		fontLF = -1, fontMRU = -1;	/* last free, most recently used */

#define __PFONT(pFont)     ( fontCache + ((UINT32)(pFont) & 0x0000FFFF) )
#define CHECK_PFONT(pFont) ( (((UINT32)(pFont) & 0xFFFF0000) == X_PFONT_MAGIC) &&\
			     (((UINT32)(pFont) & 0x0000FFFF) < fontCacheSize) )

static INT32 XFONT_IsSubset(fontInfo*, fontInfo*);
static void  XFONT_CheckFIList(fontResource*, fontInfo*, int subset_action);
static void  XFONT_GrowFreeList(int start, int end);

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
   do { font[i++] = tolower(*ptr); } while (*ptr++);
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
 */
static char* LFD_Advance(LPSTR lpFont, UINT16 uParmsNo)
{
  int 	j = 0;
  char*	lpch = lpFont;

  for( ; j < uParmsNo && *lpch; lpch++ ) if( *lpch == LFDSeparator[1] ) j++;
  return lpch;
}

static void LFD_GetWeight( fontInfo* fi, LPSTR lpStr, int j)
{
  if( j == 1 && *lpStr == '0' )
      fi->fi_flags |= FI_POLYWEIGHT;
  else if( j == 4 )
       {
         if( !lstrncmpi32A( "bold", lpStr, 4) )
           fi->df.dfWeight = FW_BOLD; 
	 else if( !lstrncmpi32A( "demi", lpStr, 4) )
	 {
	   fi->fi_flags |= FI_FW_DEMI;
	   fi->df.dfWeight = FW_DEMIBOLD;
	 }
	 else if( !lstrncmpi32A( "book", lpStr, 4) )
	 {
	   fi->fi_flags |= FI_FW_BOOK;
	   fi->df.dfWeight = FW_REGULAR;
	 }
       }
  else if( j == 5 )
       {
         if( !lstrncmpi32A( "light", lpStr, 5) )
	   fi->df.dfWeight = FW_LIGHT;
         else if( !lstrncmpi32A( "black", lpStr, 5) )
	   fi->df.dfWeight = FW_BLACK;
       }
  else if( j == 6 && !lstrncmpi32A( "medium", lpStr, 6) )
      fi->df.dfWeight = FW_REGULAR; 
  else if( j == 8 && !lstrncmpi32A( "demibold", lpStr, 8) )
      fi->df.dfWeight = FW_DEMIBOLD; 
  else
      fi->df.dfWeight = FW_DONTCARE; /* FIXME: try to get something
				      * from the weight property */
}

static int LFD_GetSlant( fontInfo* fi, LPSTR lpStr, int l)
{
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
	return 0;
    }
    return 1;
}

/*************************************************************************
 *           LFD_InitFontInfo
 *
 * Fill in some fields in the fontInfo struct.
 */
static int LFD_InitFontInfo( fontInfo* fi, LPSTR lpstr )
{
   LPSTR	lpch;
   int    	i, j, dec_style_check, scalability;
   UINT16 	tmp[3];

   memset(fi, 0, sizeof(fontInfo) );

/* weight name - */
   lpch = LFD_Advance( lpstr, 1);
   if( !*lpch ) return FALSE;
   j = lpch - lpstr - 1;
   LFD_GetWeight( fi, lpstr, j );

/* slant - */
   lpch = LFD_Advance( lpstr = lpch, 1);
   if( !*lpch ) return FALSE;
   j = lpch - lpstr - 1;
   dec_style_check = LFD_GetSlant( fi, lpstr, j );

/* width name - */
   lpch = LFD_Advance( lpstr = lpch, 1);
   if( !*lpch ) return FALSE;
   if( lstrncmpi32A( "normal", lpstr, 6) )	/* XXX 'narrow', 'condensed', etc... */
       dec_style_check = TRUE;
   else
       fi->fi_flags |= FI_NORMAL;

/* style - */
   lpch = LFD_Advance( lpstr = lpch, 1);
   if( !*lpch ) return FALSE;
   j = lpch - lpstr - 1;
   if( j > 3 )	/* find out is there "sans" or "script" */
   {
	j = 0;
	*(lpch - 1) = '\0';

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
	*(lpch - 1) = LFDSeparator[1];
   }

/* pixel height, decipoint height, and res_x */

   for( i = scalability = 0; i < 3; i++ )
   {
     lpch = LFD_Advance( lpstr = lpch, 1);
     if( !*lpch ) return FALSE;
     if( lpch - lpstr == 1 || lpch - lpstr > 4 ) return FALSE; /* ridiculous */

    *(lpch - 1) = '\0';
     if( !(tmp[i] = atoi(lpstr)) ) scalability++;
    *(lpch - 1) = LFDSeparator[1];
   }
   if( scalability == 3 ) 	/* Type1 */
   {
     fi->fi_flags |= FI_SCALABLE;
     fi->lfd_height = DEF_SCALABLE_HEIGHT; fi->lfd_decipoints = DEF_SCALABLE_DP;
     fi->lfd_resolution = DefResolution;
   }
   else if( scalability == 0 )	/* Bitmap */
   {
     fi->lfd_height = tmp[0]; fi->lfd_decipoints = tmp[1]; 
     fi->lfd_resolution = tmp[2];
   }
   else return FALSE; /* #$%^!!! X11R6 mutant garbage */

/* res_y - skip, spacing - */
   lpstr = LFD_Advance( lpch, 1);
   switch( *lpstr )
   {
     case '\0': return FALSE;

     case 'p': fi->fi_flags |= FI_VARIABLEPITCH;
	       break;
     case 'c': fi->df.dfPitchAndFamily |= FF_MODERN;
	       fi->fi_flags |= FI_FIXEDEX;
	       /* fall through */
     case 'm': fi->fi_flags |= FI_FIXEDPITCH;
	       break;
     default:
	       fi->df.dfPitchAndFamily |= DEFAULT_PITCH | FF_DONTCARE;
   }
   lpstr = LFD_Advance(lpstr, 1);
   if( !*lpstr ) return FALSE;

/* average width - */
   lpch = LFD_Advance( lpstr, 1);
   if( !*lpch ) return FALSE;
   if( lpch - lpstr == 1 || lpch - lpstr > 4 ) return FALSE; /* ridiculous */
  *(lpch - 1) = '\0'; 
   if( !(fi->lfd_width = atoi(lpstr)) && !scalability ) return FALSE;
  *(lpch - 1) = LFDSeparator[1];

/* charset registry, charset encoding - */
   if( strstr(lpch, "jisx") || 
       strstr(lpch, "ksc") || 
       strstr(lpch, "gb2312") ) return FALSE;	/* 2-byte stuff */

   fi->df.dfCharSet = ANSI_CHARSET;
   if( strstr(lpch, iso8859Encoding) )
     fi->fi_flags |= FI_ENC_ISO8859;
   else if( strstr(lpch, iso646Encoding) )
     fi->fi_flags |= FI_ENC_ISO646;
   else if( strstr(lpch, ansiEncoding) )	/* fnt2bdf produces -ansi-0 LFD */
     fi->fi_flags |= FI_ENC_ANSI;
   else						/* ... and -cp125-x */
   {
	fi->df.dfCharSet = OEM_CHARSET;
	if( !strncasecmp(lpch, localMSEncoding, 6) )
	{
	    lpch = LFD_Advance( lpch, 1 );
	    if( lpch && (i = atoi( lpch )) < numCPTranslation )
	    {
		fi->fi_flags |= FI_ENC_MSCODEPAGE;
		fi->df.dfCharSet = CPTranslation[i];
	    }
	}
	else if( strstr(lpch, "fontspecific") )
	    fi->df.dfCharSet = SYMBOL_CHARSET;
   }
   return TRUE;					
}


/*************************************************************************
 *           LFD_ComposeLFD
 */
static BOOL32  LFD_ComposeLFD( fontObject* fo, 
			       INT32 height, LPSTR lpLFD, UINT32 uRelax )
{
   int		h, w, ch, enc_ch, point = 0;
   char*	lpch; 
   const char*  lpEncoding = NULL;
   char         h_string[64], point_string[64];

   lstrcpy32A( lpLFD, fo->fr->resource );

/* add weight */
   switch( fo->fi->df.dfWeight )
   {
	case FW_BOLD:
		strcat( lpLFD, "bold" ); break;
	case FW_REGULAR:
		if( fo->fi->fi_flags & FI_FW_BOOK )
		    strcat( lpLFD, "book" );
		else
		    strcat( lpLFD, "medium" );
		break;
	case FW_DEMIBOLD:
		strcat( lpLFD, "demi" );
		if( !( fo->fi->fi_flags & FI_FW_DEMI) )
		     strcat ( lpLFD, "bold" );
		break;
	case FW_BLACK:
		strcat( lpLFD, "black" ); break;
	case FW_LIGHT:
		strcat( lpLFD, "light" ); break;
	default:
		strcat( lpLFD, "*" );
   }

/* add slant */
   if( fo->fi->df.dfItalic )
       if( fo->fi->fi_flags & FI_OBLIQUE )
	   strcat( lpLFD, "-o" );
       else
	   strcat( lpLFD, "-i" );
   else 
       strcat( lpLFD, (uRelax < 4) ? "-r" : "-*" );

/* add width style and skip serifs */
   if( fo->fi->fi_flags & FI_NORMAL )
       strcat( lpLFD, "-normal-*-");
   else
       strcat( lpLFD, "-*-*-" );

/* add pixelheight, pointheight, and resolution 
 *
 * FIXME: fill in lpXForm and lpPixmap for rotated fonts
 */
   if( fo->fo_flags & FO_SYNTH_HEIGHT ) h = fo->fi->lfd_height;
   else h = (fo->fi->lfd_height * height) / fo->fi->df.dfPixHeight;

   if( XTextCaps & TC_SF_X_YINDEP ) 
	if( fo->lf.lfWidth && !(fo->fo_flags & FO_SYNTH_WIDTH) )
	    point = (fo->fi->lfd_decipoints * fo->lf.lfWidth) / fo->fi->df.dfAvgWidth;
	else 
	if( fo->fi->fi_flags & FI_SCALABLE ) /* adjust h/w ratio */
	    point = h * 72 * 10 / fo->fi->lfd_resolution;

   /* handle rotated fonts */
   if (fo->lf.lfEscapement) {
     /* escapement is in tenths of degrees, theta is in radians */
     double theta = M_PI*fo->lf.lfEscapement/1800.;  
     double h_matrix[4] = {h*cos(theta), h*sin(theta), -h*sin(theta), h*cos(theta)};
     double point_matrix[4] = {point*cos(theta), point*sin(theta), -point*sin(theta), point*cos(theta)};
     char *s;
     sprintf(h_string, "[%+f%+f%+f%+f]", h_matrix[0], h_matrix[1], h_matrix[2], h_matrix[3]);
     sprintf(point_string, "[%+f%+f%+f%+f]", point_matrix[0], point_matrix[1], point_matrix[2], point_matrix[3]);
     while ((s = strchr(h_string, '-'))) *s='~';
     while ((s = strchr(point_string, '-'))) *s='~';
   } else {
     sprintf(h_string, "%d", h);
     sprintf(point_string, "%d", point);
   }


/* spacing and width */

   if( fo->fi->fi_flags & FI_FIXEDPITCH ) 
       w = ( fo->fi->fi_flags & FI_FIXEDEX ) ? 'c' : 'm';
   else 
       w = ( fo->fi->fi_flags & FI_VARIABLEPITCH ) ? 'p' : LFDSeparator[0]; 

/* encoding */

   enc_ch = '*';
   if( fo->fi->df.dfCharSet == ANSI_CHARSET )
   {
	if( fo->fi->fi_flags & FI_ENC_ISO8859 )
	     lpEncoding = iso8859Encoding;
	else if( fo->fi->fi_flags & FI_ENC_ISO646 )
	     lpEncoding = iso646Encoding;
	else if( fo->fi->fi_flags & FI_ENC_MSCODEPAGE )
	{
	     enc_ch = '2';
	     lpEncoding = localMSEncoding;
	}
	else lpEncoding = ansiEncoding;
   } 
   else if( fo->fi->fi_flags & FI_ENC_MSCODEPAGE )
   {
	int i;

	lpEncoding = localMSEncoding;
	for( i = 0; i < numCPTranslation; i++ )
	     if( CPTranslation[i] == fo->fi->df.dfCharSet )
	     {
		enc_ch = '0' + i;
		break;
	     }
   }
   else lpEncoding = LFDSeparator;	/* whatever */

   lpch = lpLFD + lstrlen32A(lpLFD);
   ch = (fo->fi->fi_flags & FI_SCALABLE) ? '0' : LFDSeparator[0];

   switch( uRelax )
   {
       /* RealizeFont() will call us repeatedly with increasing uRelax 
	* until XLoadFont() succeeds. */

       case 0: 
	    if( point )
	    {
	        sprintf( lpch, "%s-%s-%i-%c-%c-*-%s%c", h_string, 
			 point_string, 
			 fo->fi->lfd_resolution, ch, w, lpEncoding, enc_ch );
	        break;
	    }
	    /* fall through */

       case 1: 
	    sprintf( lpch, "%s-*-%i-%c-%c-*-%s%c", h_string, 
			fo->fi->lfd_resolution, ch, w, lpEncoding, enc_ch );
	    break;

       case 2:
	    sprintf( lpch, "%s-*-%i-%c-*-*-%s%c",
			h_string, fo->fi->lfd_resolution, ch, lpEncoding, enc_ch );
	    break;

       case 3:
	    sprintf( lpch, "%i-*-%i-%c-*-*-%s*", fo->fi->lfd_height,
			fo->fi->lfd_resolution, ch, lpEncoding );
	    break;

       default:
	    sprintf( lpch, "%i-*-*-*-*-*-%s*", fo->fi->lfd_height, lpEncoding );
   }

   TRACE(font,"\tLFD: %s\n", lpLFD );
   return TRUE;
}


/***********************************************************************
 *		X Font Resources
 *
 * font info		- http://www.microsoft.com/kb/articles/q65/1/23.htm
 * Windows font metrics	- http://www.microsoft.com/kb/articles/q32/6/67.htm
 */
static BOOL32 XFONT_GetLeading( LPIFONTINFO16 pFI, XFontStruct* x_fs, INT32* pIL, INT32* pEL )
{
    unsigned long height;
    unsigned min = (unsigned char)pFI->dfFirstChar;
    unsigned max = (unsigned char)pFI->dfLastChar;
    BOOL32 bHaveCapHeight = (pFI->dfCharSet == ANSI_CHARSET && 'X' >= min && 'X' <= max );

    if( pEL ) *pEL = 0;
    if( TSXGetFontProperty(x_fs, XA_CAP_HEIGHT, &height) == False )
    {
	if( x_fs->per_char )
	    if( bHaveCapHeight )
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
    return (bHaveCapHeight && x_fs->per_char);
}

static INT32 XFONT_GetAvgCharWidth( LPIFONTINFO16 pFI, XFontStruct* x_fs)
{
    unsigned min = (unsigned char)pFI->dfFirstChar;
    unsigned max = (unsigned char)pFI->dfLastChar;

    if( x_fs->per_char )
    {
	int  width, chars, j;
	for( j = 0, width = 0, chars = 0, max -= min; j <= max; j++ )
            if( !CI_NONEXISTCHAR(x_fs->per_char + j) )
            {
                width += x_fs->per_char[j].width;
                chars++;
            }
	return (width / chars);
    }
    /* uniform width */
    return x_fs->min_bounds.width;
}

/***********************************************************************
 *              XFONT_SetFontMetric
 *
 * Initializes IFONTINFO16. dfHorizRes and dfVertRes must be already set.
 */
static void XFONT_SetFontMetric(fontInfo* fi, fontResource* fr, XFontStruct* xfs)
{
    unsigned	 min, max;
    INT32	 el, il;

    fi->df.dfFirstChar = (BYTE)(min = xfs->min_char_or_byte2);
    fi->df.dfLastChar = (BYTE)(max = xfs->max_char_or_byte2);

    fi->df.dfDefaultChar = (BYTE)xfs->default_char;
    fi->df.dfBreakChar = (BYTE)(( ' ' < min || ' ' > max) ? xfs->default_char: ' ');

    fi->df.dfPixHeight = (INT16)((fi->df.dfAscent = (INT16)xfs->ascent) + xfs->descent);
    fi->df.dfPixWidth = (xfs->per_char) ? 0 : xfs->min_bounds.width;
    fi->df.dfMaxWidth = (INT16)abs(xfs->max_bounds.width);

    if( XFONT_GetLeading( &fi->df, xfs, &il, &el ) )
        fi->df.dfAvgWidth = (INT16)xfs->per_char['X' - min].width;
    else
        fi->df.dfAvgWidth = (INT16)XFONT_GetAvgCharWidth( &fi->df, xfs);

    fi->df.dfInternalLeading = (INT16)il;
    fi->df.dfExternalLeading = (INT16)el;

    fi->df.dfPoints = (INT16)(((INT32)(fi->df.dfPixHeight - 
	       fi->df.dfInternalLeading) * 72 + (fi->df.dfVertRes >> 1)) / fi->df.dfVertRes);

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
 *              XFONT_GetTextMetric
 */
static void XFONT_GetTextMetric( fontObject* pfo, LPTEXTMETRIC32A pTM )
{
    LPIFONTINFO16 pdf = &pfo->fi->df;

    pTM->tmAscent = pfo->fs->ascent;
    pTM->tmDescent = pfo->fs->descent;
    pTM->tmHeight = pTM->tmAscent + pTM->tmDescent;

    pTM->tmAveCharWidth = pfo->foAvgCharWidth;
    pTM->tmMaxCharWidth = abs(pfo->fs->max_bounds.width);

    pTM->tmInternalLeading = pfo->foInternalLeading;
    pTM->tmExternalLeading = pdf->dfExternalLeading;

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

    *(INT32*)&pTM->tmFirstChar = *(INT32*)&pdf->dfFirstChar;

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
static UINT32 XFONT_GetFontMetric( fontInfo* pfi, LPENUMLOGFONTEX16 pLF,
                                                  LPNEWTEXTMETRIC16 pTM )
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

    lstrcpyn32A( plf->lfFaceName, pfi->df.dfFace, LF_FACESIZE );
#undef plf

    pTM->tmAscent = pfi->df.dfAscent;
    pTM->tmDescent = pTM->tmHeight - pTM->tmAscent;
    pTM->tmInternalLeading = pfi->df.dfInternalLeading;
    pTM->tmMaxCharWidth = pfi->df.dfMaxWidth;
    pTM->tmDigitizedAspectX = pfi->df.dfHorizRes;
    pTM->tmDigitizedAspectY = pfi->df.dfVertRes;

    *(INT32*)&pTM->tmFirstChar = *(INT32*)&pfi->df.dfFirstChar;

    /* return font type */

    return pfi->df.dfType;
}


/***********************************************************************
 *           XFONT_FixupFlags
 * 
 * dfPitchAndFamily flags for some common typefaces.
 */
static BYTE XFONT_FixupFlags( LPCSTR lfFaceName )
{
   switch( lfFaceName[0] )
   {
        case 'h':
        case 'H': if(!lstrcmpi32A(lfFaceName, "Helvetica") )
                    return FF_SWISS;
                  break;
        case 'c':
        case 'C': if(!lstrcmpi32A(lfFaceName, "Courier") ||
		     !lstrcmpi32A(lfFaceName, "Charter") )
                    return FF_ROMAN;
                  break;
        case 'p':
        case 'P': if( !lstrcmpi32A(lfFaceName,"Palatino") )
                    return FF_ROMAN;
                  break;
        case 't':
        case 'T': if(!lstrncmpi32A(lfFaceName, "Times", 5) )
                    return FF_ROMAN;
                  break;
        case 'u':
        case 'U': if(!lstrcmpi32A(lfFaceName, "Utopia") )
                    return FF_ROMAN;
                  break;
        case 'z':
        case 'Z': if(!lstrcmpi32A(lfFaceName, "Zapf Dingbats") )
                    return FF_DECORATIVE;
   }
   return 0;
}


/***********************************************************************
 *           XFONT_CheckResourceName
 */
static BOOL32 XFONT_CheckResourceName( LPSTR resource, LPCSTR name, INT32 n )
{
    resource = LFD_Advance( resource, 2 );
    if( resource )
	return (!lstrncmpi32A( resource, name, n ));
    return FALSE;
}


/***********************************************************************
 *           XFONT_WindowsNames
 *
 * Build generic Windows aliases for X font names.
 *
 * -misc-fixed- -> "Fixed"
 * -sony-fixed- -> "Sony Fixed", etc...
 */
static void XFONT_WindowsNames( char* buffer )
{
    fontResource* fr, *pfr;
    char* 	lpstr, *lpch;
    int		i, up;
    BYTE	bFamilyStyle;
    const char*	relocTable[] = { INIDefaultFixed, INIDefault, NULL };

    for( fr = fontList; fr ; fr = fr->next )
    {
	if( fr->fr_flags & FR_NAMESET ) continue;     /* skip already assigned */

	lpstr = LFD_Advance(fr->resource, 2);
	i = LFD_Advance( lpstr, 1 ) - lpstr;

	for( pfr = fontList; pfr != fr ; pfr = pfr->next )
	    if( pfr->fr_flags & FR_NAMESET )
		if( XFONT_CheckResourceName( pfr->resource, lpstr, i ) )
		    break;

	if( pfr != fr )  /* prepend vendor name */
	    lpstr = fr->resource + 1;

	for( i = 0, up = 1, lpch = fr->lfFaceName; *lpstr && i < 32;
					        lpch++, lpstr++, i++ )
	{
	    if( *lpstr == LFDSeparator[1] || *lpstr == ' ' ) 
	    { 
		*lpch = ' '; 
		up = 1; continue; 
	    }
            else if( isalpha(*lpstr) && up )
	    { 
		*lpch = toupper(*lpstr); 
		up = 0; continue; 
	    }
	    *lpch = *lpstr;
	}
	while (*(lpch - 1) == ' ') *(--lpch) = '\0';

	if( (bFamilyStyle = XFONT_FixupFlags( fr->lfFaceName )) )
	{
	    fontInfo* fi;
	    for( fi = fr->fi ; fi ; fi = fi->next )
		fi->df.dfPitchAndFamily |= bFamilyStyle;
	}

	TRACE(font,"typeface \'%s\'\n", fr->lfFaceName);

	fr->fr_flags |= FR_NAMESET;
    }

    for( up = 0; relocTable[up]; up++ )
	if( PROFILE_GetWineIniString( INIFontSection, relocTable[up], "", buffer, 128 ) )
	{
	    while( *buffer && isspace(*buffer) ) buffer++;
	    for( fr = NULL, pfr = fontList; pfr; pfr = pfr->next )
	    {
		i = lstrlen32A( pfr->resource );
		if( !lstrncmpi32A( pfr->resource, buffer, i) )
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
	}
}

/***********************************************************************
 *           XFONT_CreateAlias
 */
static fontAlias* XFONT_CreateAlias( LPCSTR lpTypeFace, LPCSTR lpAlias )
{
    int j;
    fontAlias* pfa = aliasTable;

    while( 1 ) 
    {
	/* check if we already got one */
	if( !lstrcmpi32A( pfa->faTypeFace, lpAlias ) )
	{
	    TRACE(font,"\tredundant alias '%s' -> '%s'\n", 
		  lpAlias, lpTypeFace );
	    return NULL;
	} 
	if( pfa->next ) pfa = pfa->next;
	else break;
    }

    j = lstrlen32A(lpTypeFace) + 1;
    pfa->next = HeapAlloc( SystemHeap, 0, sizeof(fontAlias) +
					  j + lstrlen32A(lpAlias) + 1 );
    if((pfa = pfa->next))
    {
	pfa->next = NULL;
	pfa->faTypeFace = (LPSTR)(pfa + 1);
	lstrcpy32A( pfa->faTypeFace, lpTypeFace );
        pfa->faAlias = pfa->faTypeFace + j;
        lstrcpy32A( pfa->faAlias, lpAlias );

        TRACE(font, "\tadded alias '%s' for %s\n", lpAlias, lpTypeFace );

	return pfa;
    }
    return NULL;
}

/***********************************************************************
 *           XFONT_LoadAliases
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
static void XFONT_LoadAliases( char** buffer, int buf_size )
{
    char* lpResource, *lpAlias;
    char  subsection[32];
    int	  i = 0, j = 0;
    BOOL32 bHaveAlias = TRUE, bSubst = FALSE;

    if( buf_size < 128 )
	*buffer = HeapReAlloc(SystemHeap, 0, *buffer, 256 );
    do
    {
	if( j < faTemplateNum )
	{
	    /* built-in templates first */

	    lpResource = faTemplate[j].fatResource;
	    lpAlias = faTemplate[j].fatAlias;
	    j++;
	}
	else
	{
	    /* then WINE.CONF */

	    wsprintf32A( subsection, "%s%i", INISubSection, i++ );

	    if( (bHaveAlias = PROFILE_GetWineIniString( INIFontSection, 
					subsection, "", *buffer, 128 )) )
	    {
		lpAlias = *buffer;
		while( isspace(*lpAlias) ) lpAlias++;
		lpResource = PROFILE_GetStringItem( lpAlias );
		bSubst = (PROFILE_GetStringItem( lpResource )) ? TRUE : FALSE;
	    }
	}

	if( bHaveAlias )
	{
	    int length;

	    length = lstrlen32A( lpAlias );
	    if( lpResource && length )
	    {
		fontResource* fr, *frMatch = NULL;

		for (fr = fontList; fr ; fr = fr->next)
		{
		    if( !lstrcmpi32A( fr->resource, lpResource ) ) frMatch = fr;
		    if( XFONT_CheckResourceName( fr->resource, lpAlias, length ) )
		    {
			/* alias is not needed since the real font is present */
			frMatch = NULL; break;
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
			    if(!lstrcmp32A(lpAlias, pfa->faAlias))
			    {
				if(prev)
				    prev->next = pfa->next;
				else
				    aliasTable = pfa->next;
			     }

	/* Update any references to the substituted font in aliasTable */
			    if(!lstrcmp32A(frMatch->lfFaceName,
							pfa->faTypeFace))
				pfa->faTypeFace = HEAP_strdupA( SystemHeap, 0,
							 lpAlias );
			    prev = pfa;
			}
						
                        TRACE(font, "\tsubstituted '%s' with %s\n",
						frMatch->lfFaceName, lpAlias );

			lstrcpyn32A( frMatch->lfFaceName, lpAlias, LF_FACESIZE );
			frMatch->fr_flags |= FR_NAMESET;
		    }
		    else
		    {
			/* create new entry in the alias table */
			XFONT_CreateAlias( frMatch->lfFaceName, lpAlias );
		    }
		}
	    }
	    else ERR(font, " malformed font alias '%s'\n", *buffer );
	}
	else break;
    } while(TRUE);
}

/***********************************************************************
 *           XFONT_UserMetricsCache
 * 
 * Returns expanded name for the ~/.wine/.cachedmetrics file.
 */
static char* XFONT_UserMetricsCache( char* buffer, int* buf_size )
{
    struct passwd* pwd;

    pwd = getpwuid(getuid());
    if( pwd && pwd->pw_dir )
    {
	int i = lstrlen32A( pwd->pw_dir ) + lstrlen32A( INIWinePrefix ) + 
					    lstrlen32A( INIFontMetrics ) + 2;
	if( i > *buf_size ) 
	    buffer = (char*) HeapReAlloc( SystemHeap, 0, buffer, *buf_size = i );
	lstrcpy32A( buffer, pwd->pw_dir );
	strcat( buffer, INIWinePrefix );
	strcat( buffer, INIFontMetrics );
    } else buffer[0] = '\0';
    return buffer;
}


/***********************************************************************
 *           XFONT_ReadCachedMetrics
 */
static BOOL32 XFONT_ReadCachedMetrics( int fd, int res, unsigned x_checksum, int x_count )
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

		    TRACE(font,"Reading cached font metrics:\n");

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
			   pfi->df.dfHorizRes = pfi->df.dfVertRes = res;
			   pfi->df.dfPoints = (INT16)(((INT32)(pfi->df.dfPixHeight -
				pfi->df.dfInternalLeading) * 72 + (res >> 1)) / res );
			   pfi->next = pfi + 1;

			   if( j > pfr->count ) break;

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
			    TRACE(font,"\t%s, %i instances\n", lpch, pfr->count );
			    pfr->resource = lpch;
			    while( TRUE )
			    { 
				if( ++offset > length ) goto fail;
				if( !*lpch++ ) break;
			    }
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
 */
static BOOL32 XFONT_WriteCachedMetrics( int fd, unsigned x_checksum, int x_count, int n_ff )
{
    fontResource* pfr;
    fontInfo* pfi;

    if( fd >= 0 )
    {
        int  i, j, k;

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
	    i += lstrlen32A( pfr->resource ) + 1;
	    j += pfr->count;
	}
        i += n_ff * sizeof(fontResource) + j * sizeof(fontInfo) + sizeof(int);
	write( fd, &i, sizeof(int) );

	TRACE(font,"Writing font cache:\n");

	for( pfr = fontList; pfr; pfr = pfr->next )
	{
	    fontInfo fi;

	    TRACE(font,"\t%s, %i instances\n", pfr->resource, pfr->count );

	    i = write( fd, pfr, sizeof(fontResource) );
	    if( i == sizeof(fontResource) ) 
	    {
		for( k = 1, pfi = pfr->fi; pfi; pfi = pfi->next )
		{
		    memcpy( &fi, pfi, sizeof(fi) );

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
	        i = lstrlen32A( pfr->resource ) + 1;
		j = write( fd, pfr->resource, i );
	    }
	}
	close( fd );
	return ( i == j );
    }
    return TRUE;
}

/***********************************************************************
 *           XFONT_CheckIniSection
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

static int XFONT_CheckIniSection()
{
    int  found = 0;

    PROFILE_EnumerateWineIniSection("Fonts", &XFONT_CheckIniCallback,
                    (void *)&found);
    if(found)
    fprintf(stderr, fontmsgepilogue);

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
    if((strncasecmp(key, INISubSection, 5) == 0) ||
       (strcasecmp( key, INIDefault) == 0) ||
       (strcasecmp( key, INIDefaultFixed) == 0) ||
       (strcasecmp( key, INIGlobalMetrics) == 0) ||
       (strcasecmp( key, INIResolution) == 0) ) 
    {
	/* Valid key; make sure the value doesn't contain a wildcard */
	if(strchr(value, '*')) {
	    if(*(int *)found == 0) {
		fprintf(stderr, fontmsgprologue);
		++*(int *)found;
	    }

	    fprintf(stderr, "     %s=%s [no wildcards allowed]\n", key, value);
	}
    }
    else {
	/* Not a valid key */
	if(*(int *)found == 0) {
	    fprintf(stderr, fontmsgprologue);
	    ++*(int *)found;
    }

	fprintf(stderr, "     %s=%s [obsolete]\n", key, value);
    }

    return;
}

/***********************************************************************
 *           XFONT_GetPointResolution()
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

    DefResolution = point_resolution = PROFILE_GetWineIniInt( INIFontSection, INIResolution, 0 );
    if( !DefResolution ) DefResolution = point_resolution = pDevCaps->logPixelsY;
    else pDevCaps->logPixelsX = pDevCaps->logPixelsY = DefResolution;

    for( i = best = 0; i < num; i++ )
    {
	j = abs( DefResolution - allowed_xfont_resolutions[i] );
	if( j < best_diff )
	{
	    best = i;
		best_diff = j;
	}
    }
    DefResolution = allowed_xfont_resolutions[best];
    return point_resolution;
}

/***********************************************************************
 *           X11DRV_FONT_Init
 *
 * Initialize font resource list and allocate font cache.
 */
BOOL32 X11DRV_FONT_Init( DeviceCaps* pDevCaps )
{
  XFontStruct*	x_fs;
  fontResource* fr, *pfr;
  fontInfo*	fi, *pfi;
  unsigned	x_checksum;
  int		i, j, res, x_count, fd = -1, buf_size = 0;
  char*		lpstr, *lpch, *lpmetrics, *buffer;
  char**	x_pattern;

  XFONT_CheckIniSection();

  res = XFONT_GetPointResolution( pDevCaps );
      
  x_pattern = TSXListFonts(display, "*", MAX_FONT_FAMILIES * 16, &x_count );

  TRACE(font,"Font Mapper: initializing %i fonts [LPY=%i, XDR=%i, DR=%i]\n", 
				    x_count, pDevCaps->logPixelsY, DefResolution, res);
  for( i = x_checksum = 0; i < x_count; i++ )
  {
#if 0
     printf("%i\t: %s\n", i, x_pattern[i] );
#endif

     j = lstrlen32A( x_pattern[i] );
     if( j ) x_checksum ^= __genericCheckSum( x_pattern[i], j );
  }
  x_checksum |= X_PFONT_MAGIC;

  buf_size = 128;
  buffer = HeapAlloc( SystemHeap, 0, buf_size );
  lpmetrics = NULL;

  /* deal with systemwide font metrics cache */

  if( PROFILE_GetWineIniString( INIFontSection, INIGlobalMetrics, "", buffer, 128 ) )
      fd = open( buffer, O_RDONLY );

  if( XFONT_ReadCachedMetrics(fd, res, x_checksum, x_count) == FALSE )
  {
      /* try per-user */
      buffer = XFONT_UserMetricsCache( buffer, &buf_size );
      if( buffer[0] )
      {
	  fd = open( buffer, O_RDONLY );
	  if( XFONT_ReadCachedMetrics(fd, res, x_checksum, x_count) == FALSE )
	      lpmetrics = HEAP_strdupA( SystemHeap, 0, buffer ); /* update later on */
      }
  }

  fi = NULL;
  if( fontList == NULL )	/* build metrics from scratch */
  {
     int n_ff;
     char* typeface;

     for( i = n_ff = 0; i < x_count; i++ )
     {
	typeface = lpch = x_pattern[i];

	lpch = LFD_Advance(typeface, 3); /* extra '-' in the beginning */
	if( !*lpch ) continue;

	lpstr = lpch; 
	j = lpch - typeface;	/* resource name length */

	/* find a family to insert into */

	for( pfr = NULL, fr = fontList; fr; fr = fr->next )
	{
	   if( !lstrncmpi32A(fr->resource, typeface, j) && 
	       lstrlen32A(fr->resource) == j ) break;
	   pfr = fr;
	}  

	if( !fi ) fi = (fontInfo*) HeapAlloc(SystemHeap, 0, sizeof(fontInfo));

	if( !fr ) /* add new family */
	{
	   if( n_ff >= MAX_FONT_FAMILIES ) break;
	   if( !LFD_InitFontInfo( fi, lpstr) ) continue;

	   n_ff++;
	   fr = (fontResource*) HeapAlloc(SystemHeap, 0, sizeof(fontResource)); 
	   memset(fr, 0, sizeof(fontResource));
	   fr->resource = (char*) HeapAlloc(SystemHeap, 0, j + 1 );
	   lstrcpyn32A( fr->resource, typeface, j + 1 );

	   TRACE(font,"    family: %s\n", fr->resource );

	   if( pfr ) pfr->next = fr;
	   else fontList = fr;
	}
	else if( !LFD_InitFontInfo( fi, lpstr) ) continue;

	/* check if we already have something better than "fi" */

	for( pfi = fr->fi, j = 0; pfi && j <= 0; pfi = pfi->next ) 
           if( (j = XFONT_IsSubset( pfi, fi )) < 0 ) 
	       pfi->fi_flags |= FI_SUBSET; /* superseded by "fi" */
	if( j > 0 ) continue;

	/* add new font instance "fi" to the "fr" font resource */

	if( fi->fi_flags & FI_SCALABLE )
	{
	    /* set scalable font height to 24 to get an origin for extrapolation */

	   j = lstrlen32A(typeface);  j += 0x10;
	   if( j > buf_size ) 
	       buffer = (char*)HeapReAlloc( SystemHeap, 0, buffer, buf_size = j );

	   lpch = LFD_Advance(typeface, 7);
	   memcpy( buffer, typeface, (j = lpch - typeface) );
	   lpch = LFD_Advance(lpch, 4);
	   sprintf( buffer + j, "%d-%d-%d-*-%c-*-", fi->lfd_height,
                            fi->lfd_decipoints, fi->lfd_resolution,
                                        (*lpch == '-')?'*':*lpch );
	   lpch = LFD_Advance(lpch, 2);
	   strcat( lpstr = buffer, lpch);
	}
	else lpstr = typeface;

	if( (x_fs = TSXLoadQueryFont(display, lpstr)) )
	{
	   fi->df.dfHorizRes = fi->df.dfVertRes = res;

	   XFONT_SetFontMetric( fi, fr, x_fs );
           TSXFreeFont( display, x_fs );

	   TRACE(font,"\t[% 2ipt] '%s'\n", fi->df.dfPoints, typeface );

	   XFONT_CheckFIList( fr, fi, REMOVE_SUBSETS );
	   fi = NULL;	/* preventing reuse */
        }
        else
        {
           ERR(font, "failed to load %s\n", lpstr );

           XFONT_CheckFIList( fr, fi, UNMARK_SUBSETS );
        }
     }

     if( lpmetrics )	 /* update cached metrics */
     {
	 fd = open( lpmetrics, O_CREAT | O_TRUNC | O_RDWR, 0644 ); /* -rw-r--r-- */
	 if( XFONT_WriteCachedMetrics( fd, x_checksum, x_count, n_ff ) == FALSE )
	     if( fd ) remove( lpmetrics );	/* couldn't write entire file */
	 HeapFree( SystemHeap, 0, lpmetrics );
     }
  }

  if( fi ) HeapFree(SystemHeap, 0, fi);
  TSXFreeFontNames(x_pattern);

  /* check if we're dealing with X11 R6 server */

  lstrcpy32A(buffer, "-*-*-*-*-normal-*-[12 0 0 12]-*-72-*-*-*-iso8859-1");
  if( (x_fs = TSXLoadQueryFont(display, buffer)) )
  {
       XTextCaps |= TC_SF_X_YINDEP;
       TSXFreeFont(display, x_fs);
  }

  XFONT_WindowsNames( buffer );
  XFONT_LoadAliases( &buffer, buf_size );
  HeapFree(SystemHeap, 0, buffer);


  /* fontList initialization is over, allocate X font cache */

  fontCache = (fontObject*) HeapAlloc(SystemHeap, 0, fontCacheSize * sizeof(fontObject));
  XFONT_GrowFreeList(0, fontCacheSize - 1);

  TRACE(font,"done!\n");

  /* update text caps parameter */

  pDevCaps->textCaps = XTextCaps;
  return TRUE;
}


/***********************************************************************
 *           X Font Matching
 *
 * Compare two fonts (only parameters set by the XFONT_InitFontInfo()).
 */
static INT32 XFONT_IsSubset(fontInfo* match, fontInfo* fi)
{
  INT32           m;

  /* 0 - keep both, 1 - keep match, -1 - keep fi */

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
static UINT32 XFONT_Match( fontMatch* pfm )
{
   fontInfo*    pfi = pfm->pfi;         /* device font to match */
   LPLOGFONT16  plf = pfm->plf;         /* wanted logical font */
   UINT32       penalty = 0;
   BOOL32       bR6 = pfm->flags & FO_MATCH_XYINDEP;    /* from TextCaps */
   BOOL32       bScale = pfi->fi_flags & FI_SCALABLE;
   INT32        d, h;

   TRACE(font,"\t[ %-2ipt h=%-3i w=%-3i %s%s]\n", pfi->df.dfPoints,
		 pfi->df.dfPixHeight, pfi->df.dfAvgWidth,
		(pfi->df.dfWeight > 400) ? "Bold " : "Normal ",
		(pfi->df.dfItalic) ? "Italic" : "" );

   pfm->flags = 0;

   if( plf->lfCharSet == DEFAULT_CHARSET )
   {
       if( (pfi->df.dfCharSet!= ANSI_CHARSET) && (pfi->df.dfCharSet!=DEFAULT_CHARSET) ) 
	    penalty += 0x200;
   }
   else if (plf->lfCharSet != pfi->df.dfCharSet) penalty += 0x200;

   /* TMPF_FIXED_PITCH means exactly the opposite */

   if( plf->lfPitchAndFamily & FIXED_PITCH ) 
   {
      if( pfi->df.dfPitchAndFamily & TMPF_FIXED_PITCH ) penalty += 0x100;
   }
   else if( !(pfi->df.dfPitchAndFamily & TMPF_FIXED_PITCH) ) penalty += 0x2;

   if( plf->lfHeight > 0 )
       d = (h = pfi->df.dfPixHeight) - plf->lfHeight;
   else if( plf->lfHeight < -1 )
       d = (h = pfi->df.dfPoints) + plf->lfHeight;
   else d = h = 0;	

   if( d && plf->lfHeight )
   {
       UINT16	height = ( plf->lfHeight > 0 ) ? plf->lfHeight
			 : ((-plf->lfHeight * pfi->df.dfPixHeight) / h);
       if( bScale ) pfm->height = height;
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
   } else pfm->height = pfi->df.dfPixHeight;

   if((pfm->flags & FO_MATCH_PAF) &&
      (plf->lfPitchAndFamily & FF_FAMILY) != (pfi->df.dfPitchAndFamily & FF_FAMILY) )
       penalty += 0x10;

   if( plf->lfWidth )
   {
       if( bR6 && bScale ) h = 0; 
       else
       {
	   /* FIXME: not complete */

	   pfm->flags |= FO_SYNTH_WIDTH;
	   h = abs(plf->lfWidth - (pfm->height * pfi->df.dfAvgWidth)/pfi->df.dfPixHeight);
       }
       penalty += h * ( d ) ? 0x2 : 0x1 ;
   }
   else if( !(pfi->fi_flags & FI_NORMAL) ) penalty++;

   if( plf->lfWeight != FW_DONTCARE )
   {
       penalty += abs(plf->lfWeight - pfi->df.dfWeight) / 40;
       if( plf->lfWeight > pfi->df.dfWeight ) pfm->flags |= FO_SYNTH_BOLD;
   } else if( pfi->df.dfWeight >= FW_BOLD ) penalty++;	/* choose normal by default */

   if( plf->lfItalic != pfi->df.dfItalic )
   {
       penalty += 0x4;
       pfm->flags |= FO_SYNTH_ITALIC;
   }

   if( plf->lfUnderline ) pfm->flags |= FO_SYNTH_UNDERLINE;
   if( plf->lfStrikeOut ) pfm->flags |= FO_SYNTH_STRIKEOUT;

   if( penalty && pfi->lfd_resolution != DefResolution ) 
       penalty++;

   TRACE(font,"  returning %i\n", penalty );

   return penalty;
}

/***********************************************************************
 *            XFONT_MatchFIList
 *
 * Scan a particular font resource for the best match.
 */
static UINT32 XFONT_MatchFIList( fontMatch* pfm )
{
  BOOL32        skipRaster = (pfm->flags & FO_MATCH_NORASTER);
  UINT32        current_score, score = (UINT32)(-1);
  UINT16        origflags = pfm->flags; /* Preserve FO_MATCH_XYINDEP */
  fontMatch     fm = *pfm;

  for( fm.pfi = pfm->pfr->fi; fm.pfi && score; fm.pfi = fm.pfi->next,
      fm.flags = origflags )
  {
     if( skipRaster && !(fm.pfi->fi_flags & FI_SCALABLE) )
         continue;

     current_score = XFONT_Match( &fm );
     if( score > current_score )
     {
        memcpy( pfm, &fm, sizeof(fontMatch) );
        score = current_score;
     }
  }
  return score;
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
	    fr->count--;
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
    fr->count++;
  }

  if( i ) TRACE(font,"\t    purged %i subsets [%i]\n", i , fr->count);
}

/***********************************************************************
 *            XFONT_FindFIList
 */
static fontResource* XFONT_FindFIList( fontResource* pfr, const char* pTypeFace )
{
  while( pfr )
  {
    if( !lstrcmpi32A( pfr->lfFaceName, pTypeFace ) ) break;
    pfr = pfr->next;
  }
  return pfr;
}

/***********************************************************************
 *          XFONT_MatchDeviceFont
 *
 * Scan font resource tree.
 */
static BOOL32 XFONT_MatchDeviceFont( fontResource* start, fontMatch* pfm )
{
    fontMatch           fm = *pfm;

    pfm->pfi = NULL;
    if( fm.plf->lfFaceName[0] ) 
    {
	fontAlias* fa;
	LPSTR str = NULL;

	for( fa = aliasTable; fa; fa = fa->next )
	     if( !lstrcmpi32A( fa->faAlias, fm.plf->lfFaceName ) ) 
	     {
		str = fa->faTypeFace;
		break;
	     }
        fm.pfr = XFONT_FindFIList( start, str ? str : fm.plf->lfFaceName );
    }

    if( fm.pfr )        /* match family */
    {
	TRACE(font, "%s\n", fm.pfr->lfFaceName );

        XFONT_MatchFIList( &fm );
        *pfm = fm;
    }

    if( !pfm->pfi )      /* match all available fonts */
    {
        UINT32          current_score, score = (UINT32)(-1);

        fm.flags |= FO_MATCH_PAF;
        for( start = fontList; start && score; start = start->next )
        {
            fm.pfr = start;
	    TRACE(font, "%s\n", fm.pfr->lfFaceName );

            current_score = XFONT_MatchFIList( &fm );
            if( current_score < score )
            {
                score = current_score;
                *pfm = fm;
            }
        }
    }
    return TRUE;
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

static fontObject* XFONT_LookupCachedFont( LPLOGFONT16 plf, UINT16* checksum )
{
    UINT16	cs = __lfCheckSum( plf );
    int		i = fontMRU, prev = -1;
   
   *checksum = cs;
    while( i >= 0 )
    {
	if( fontCache[i].lfchecksum == cs &&
	  !(fontCache[i].fo_flags & FO_REMOVED) )
	{
	    /* FIXME: something more intelligent here */

	    if( !memcmp( plf, &fontCache[i].lf,
			 sizeof(LOGFONT16) - LF_FACESIZE ) &&
		!lstrncmpi32A( plf->lfFaceName, fontCache[i].lf.lfFaceName, 
							    LF_FACESIZE ) )
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

static fontObject* XFONT_GetCacheEntry()
{
    int		i;

    if( fontLF == -1 )
    {
	int	prev_i, prev_j, j;

	TRACE(font,"font cache is full\n");

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

	    TRACE(font,"\tfreeing entry %i\n", j );

	    if( prev_j >= 0 )
		fontCache[prev_j].lru = fontCache[j].lru;
	    else fontMRU = (INT16)fontCache[j].lru;

	    /* FIXME: lpXForm, lpPixmap */
	    TSXFreeFont( display, fontCache[j].fs );

	    memset( fontCache + j, 0, sizeof(fontObject) );
	    return (fontCache + j);
	}
	else		/* expand cache */
	{
	    fontObject*	newCache;

	    prev_i = fontCacheSize + FONTCACHE;

	    TRACE(font,"\tgrowing font cache from %i to %i\n", fontCacheSize, prev_i );

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

static int XFONT_ReleaseCacheEntry(fontObject* pfo)
{
    UINT32	u = (UINT32)(pfo - fontCache);

    if( u < fontCacheSize ) return (--fontCache[u].count);
    return -1;
}

/***********************************************************************
 *           X Device Font Objects
 */
static X_PHYSFONT XFONT_RealizeFont( LPLOGFONT16 plf )
{
    UINT16	checksum;
    fontObject* pfo = XFONT_LookupCachedFont( plf, &checksum );

    if( !pfo )
    {
	fontMatch	fm = { NULL, NULL, 0, 0, plf};
	INT32		i, index;

	if( XTextCaps & TC_SF_X_YINDEP ) fm.flags = FO_MATCH_XYINDEP;

	/* allocate new font cache entry */

	if( (pfo = XFONT_GetCacheEntry()) )
	{
	    LPSTR	lpLFD = HeapAlloc( GetProcessHeap(), 0, MAX_LFD_LENGTH );
	    
	    if( lpLFD ) /* initialize entry and load font */
	    {
		UINT32	uRelaxLevel = 0;

		TRACE(font,"(%u) '%s' h=%i weight=%i %s\n",
			     plf->lfCharSet, plf->lfFaceName, plf->lfHeight, 
			     plf->lfWeight, (plf->lfItalic) ? "Italic" : "" );

		XFONT_MatchDeviceFont( fontList, &fm );

		pfo->fr = fm.pfr;
		pfo->fi = fm.pfi;
		pfo->fo_flags = fm.flags & ~FO_MATCH_MASK;

		memcpy( &pfo->lf, plf, sizeof(LOGFONT16) );
		pfo->lfchecksum = checksum;

		do
		{
		    LFD_ComposeLFD( pfo, fm.height, lpLFD, uRelaxLevel++ );
		    if( (pfo->fs = TSXLoadQueryFont( display, lpLFD )) ) break;
		} while( uRelaxLevel );

		if( XFONT_GetLeading( &pfo->fi->df, pfo->fs, &i, NULL ) )
		    pfo->foAvgCharWidth = (INT16)pfo->fs->per_char['X' - pfo->fs->min_char_or_byte2].width;
		else
		    pfo->foAvgCharWidth = (INT16)XFONT_GetAvgCharWidth( &pfo->fi->df, pfo->fs );
		pfo->foInternalLeading = (INT16)i;

		/* FIXME: If we've got a soft font or
		 * there are FO_SYNTH_... flags for the
		 * non PROOF_QUALITY request, the engine
		 * should rasterize characters into mono
		 * pixmaps and store them in the pfo->lpPixmap
		 * array (pfo->fs should be updated as well).
		 * X11DRV_ExtTextOut() must be heavily modified 
		 * to support pixmap blitting and FO_SYNTH_...
		 * styles.
		 */

		pfo->lpXForm = NULL;
		pfo->lpPixmap = NULL;

		HeapFree( GetProcessHeap(), 0, lpLFD );
	    } 
	    else /* attach back to the free list */
	    {
		pfo->count = -1;
		pfo->lru = fontLF;
		fontLF = (pfo - fontCache);
		pfo = NULL;
	    }
	}

	if( !pfo ) /* couldn't get a new entry, get one of the cached fonts */
	{
	    UINT32		current_score, score = (UINT32)(-1);

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
	    pfo->count++;
	    return (X_PHYSFONT)(X_PFONT_MAGIC | index);
	}
    }
 
    /* attach at the head of the lru list */

    pfo->count++;
    pfo->lru = fontMRU;
    fontMRU = (pfo - fontCache);

    TRACE(font,"physfont %i\n", fontMRU);

    return (X_PHYSFONT)(X_PFONT_MAGIC | fontMRU);
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
HFONT32 X11DRV_FONT_SelectObject( DC* dc, HFONT32 hfont, FONTOBJ* font )
{
    HFONT32 hPrevFont = 0;
    LOGFONT16 lf;

    if( CHECK_PFONT(dc->u.x.font) ) 
        XFONT_ReleaseCacheEntry( __PFONT(dc->u.x.font) );

    /* FIXME: do we need to pass anything back from here? */
    memcpy(&lf,&font->logfont,sizeof(lf));
    lf.lfWidth  = font->logfont.lfWidth * dc->vportExtX/dc->wndExtX;
    lf.lfHeight = font->logfont.lfHeight* dc->vportExtY/dc->wndExtY;

    dc->u.x.font = XFONT_RealizeFont( &lf );
    hPrevFont = dc->w.hFont;
    dc->w.hFont = hfont;

    return hPrevFont;
}


/***********************************************************************
 *
 *           X11DRV_EnumDeviceFonts
 */
BOOL32	X11DRV_EnumDeviceFonts( DC* dc, LPLOGFONT16 plf, 
				        DEVICEFONTENUMPROC proc, LPARAM lp )
{
    ENUMLOGFONTEX16	lf;
    NEWTEXTMETRIC16	tm;
    fontResource* 	pfr = fontList;
    BOOL32	  	b, bRet = 0;

    if( plf->lfFaceName[0] )
    {
	pfr = XFONT_FindFIList( pfr, plf->lfFaceName );
	if( pfr )
	{
	    fontInfo*	pfi;
	    for( pfi = pfr->fi; pfi; pfi = pfi->next )
		if( (b = (*proc)( (LPENUMLOGFONT16)&lf, &tm, 
			XFONT_GetFontMetric( pfi, &lf, &tm ), lp )) )
		     bRet = b;
		else break;
	}
    }
    else
	for( ; pfr ; pfr = pfr->next )
	    if( (b = (*proc)( (LPENUMLOGFONT16)&lf, &tm, 
		       XFONT_GetFontMetric( pfr->fi, &lf, &tm ), lp )) )
		 bRet = b;
	    else break;

    return bRet;
}


/***********************************************************************
 *           X11DRV_GetTextExtentPoint
 */
BOOL32 X11DRV_GetTextExtentPoint( DC *dc, LPCSTR str, INT32 count,
                                  LPSIZE32 size )
{
    XFontStruct* pfs = XFONT_GetFontStruct( dc->u.x.font );
    if( pfs )
    {
	int dir, ascent, descent;
	XCharStruct info;

	TSXTextExtents( pfs, str, count, &dir, &ascent, &descent, &info );
	size->cx = abs((info.width + dc->w.breakRem + count * dc->w.charExtra)
						* dc->wndExtX / dc->vportExtX);
	size->cy = abs((pfs->ascent + pfs->descent) * dc->wndExtY / dc->vportExtY);

	return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           X11DRV_GetTextMetrics
 */
BOOL32 X11DRV_GetTextMetrics(DC *dc, TEXTMETRIC32A *metrics)
{
    if( CHECK_PFONT(dc->u.x.font) )
    {
	fontObject* pfo = __PFONT(dc->u.x.font);
	XFONT_GetTextMetric( pfo, metrics );

	return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           X11DRV_GetCharWidth
 */
BOOL32 X11DRV_GetCharWidth( DC *dc, UINT32 firstChar, UINT32 lastChar,
                            LPINT32 buffer )
{
    XFontStruct* xfs = XFONT_GetFontStruct( dc->u.x.font );

    if( xfs )
    {
	int i;

	if (xfs->per_char == NULL)
	    for (i = firstChar; i <= lastChar; i++)
		*buffer++ = xfs->min_bounds.width;
	else
	{
	    XCharStruct *cs, *def;
	    static XCharStruct	__null_char = { 0, 0, 0, 0, 0, 0 };

	    CI_GET_CHAR_INFO(xfs, xfs->default_char, &__null_char, def);

	    for (i = firstChar; i <= lastChar; i++)
	    {
		if (i >= xfs->min_char_or_byte2 && i <= xfs->max_char_or_byte2)
		{
		    cs = &xfs->per_char[(i - xfs->min_char_or_byte2)]; 
		    if (CI_NONEXISTCHAR(cs)) cs = def; 
  		} else cs = def;
		*buffer++ = MAX(cs->width, 0 );
	    }
	}

	return TRUE;
    }
    return FALSE;
}

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
    return AddFontResource32A( filename );
}


/***********************************************************************
 *           AddFontResource32A    (GDI32.2)
 */
INT32 WINAPI AddFontResource32A( LPCSTR str )
{
    FIXME(font, "(%s): stub\n", debugres_a(str));
    return 1;
}


/***********************************************************************
 *           AddFontResource32W    (GDI32.4)
 */
INT32 WINAPI AddFontResource32W( LPCWSTR str )
{
    FIXME(font, "(%s): stub\n", debugres_w(str) );
    return 1;
}

/***********************************************************************
 *           RemoveFontResource16    (GDI.136)
 */
BOOL16 WINAPI RemoveFontResource16( SEGPTR str )
{
    FIXME(font, "(%s): stub\n",	debugres_a(PTR_SEG_TO_LIN(str)));
    return TRUE;
}


/***********************************************************************
 *           RemoveFontResource32A    (GDI32.284)
 */
BOOL32 WINAPI RemoveFontResource32A( LPCSTR str )
{
    FIXME(font, "(%s): stub\n", debugres_a(str));
    return TRUE;
}


/***********************************************************************
 *           RemoveFontResource32W    (GDI32.286)
 */
BOOL32 WINAPI RemoveFontResource32W( LPCWSTR str )
{
    FIXME(font, "(%s): stub\n", debugres_w(str) );
    return TRUE;
}


