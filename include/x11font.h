/*
 * X11 physical font definitions
 *
 * Copyright 1997 Alex Korobka
 */

#ifndef __WINE_X11FONT_H
#define __WINE_X11FONT_H

#include "gdi.h"

#pragma pack(1)

/* this is a part of the font resource header, should 
 * make it easier to implement dynamic softfont loading */

typedef struct
{
    INT16	dfType;
    INT16	dfPoints;
    INT16	dfVertRes;
    INT16	dfHorizRes;
    INT16	dfAscent;
    INT16	dfInternalLeading;
    INT16	dfExternalLeading;
    CHAR	dfItalic;
    CHAR	dfUnderline;
    CHAR	dfStrikeOut;
    INT16	dfWeight;
    CHAR	dfCharSet;
    INT16	dfPixWidth;
    INT16	dfPixHeight;
    CHAR	dfPitchAndFamily;
    INT16	dfAvgWidth;
    INT16	dfMaxWidth;
    CHAR	dfFirstChar;
    CHAR	dfLastChar;
    CHAR	dfDefaultChar;
    CHAR	dfBreakChar;
    INT16	dfWidthBytes;
    LPCSTR	dfDevice;
    LPCSTR	dfFace;
} IFONTINFO16, *LPIFONTINFO16;

#pragma pack(4)

/* internal flags */

#define FI_POLYWEIGHT	    0x0001
#define FI_POLYSLANT 	    0x0002
#define FI_OBLIQUE	    0x0004
#define FI_SCALABLE  	    0x0008
#define FI_FW_BOOK	    0x0010
#define FI_FW_DEMI	    0x0020
#define FI_VARIABLEPITCH    0x0040
#define FI_FIXEDPITCH       0x0080

#define FI_ENC_ISO8859	    0x0100	/* iso8859-* 	*/
#define FI_ENC_ISO646	    0x0200	/* iso646* 	*/
#define FI_ENC_ANSI	    0x0400	/* ansi-0 	*/
#define FI_ENC_MSCODEPAGE   0x0800	/* cp125-* 	*/

#define FI_FIXEDEX	    0x1000
#define FI_NORMAL	    0x2000
#define FI_SUBSET 	    0x4000
#define FI_TRUETYPE	    0x8000

typedef struct tagFontInfo
{
    struct tagFontInfo*		next;
    UINT16			fi_flags;

 /* LFD parameters can be quite different from the actual metrics */

    UINT16			lfd_height;
    UINT16			lfd_width;
    UINT16			lfd_decipoints;
    UINT16			lfd_resolution;

    IFONTINFO16			df;
} fontInfo;

/* Font resource list for EnumFont() purposes */

#define FR_SOFTFONT         0x1000              /* - .FON or .FOT file */
#define FR_SOFTRESOURCE     0x2000              /* - resource handle */
#define FR_REMOVED          0x4000              /* delayed remove */
#define FR_NAMESET          0x8000

typedef struct tagFontResource
{
  struct tagFontResource*	next;
  UINT16			fr_flags;
  UINT16			count;
  fontInfo*			fi;
  char*                         resource;
  CHAR				lfFaceName[LF_FACESIZE];
} fontResource;

#define FO_RESOURCE_MASK	0x000F
#define FO_SYSTEM		0x0001		/* resident in cache */
#define FO_SOFTFONT		0x0002		/* installed at runtime */
#define FO_SHARED		0x0004		/* MITSHM */
#define FO_REMOVED		0x0008		/* remove when count falls to 0 */

#define FO_MATCH_MASK		0x00F0
#define FO_MATCH_NORASTER	0x0010
#define FO_MATCH_PAF		0x0020
#define FO_MATCH_XYINDEP	0x0040

#define FO_SYNTH_MASK		0xFF00
#define FO_SYNTH_HEIGHT   	0x2000
#define FO_SYNTH_WIDTH		0x4000
#define FO_SYNTH_ROTATE 	0x8000
#define FO_SYNTH_BOLD		0x0100
#define FO_SYNTH_ITALIC		0x0200
#define FO_SYNTH_UNDERLINE	0x0400
#define FO_SYNTH_STRIKEOUT	0x0800

/* Realized screen font */

typedef struct 
{
  XFontStruct*          fs;			/* text metrics */
  fontResource*         fr;			/* font family */
  fontInfo*		fi;			/* font instance info */
  LPMAT2*		lpXForm;		/* optional transformation matrix */
  Pixmap*               lpPixmap;		/* optional character bitmasks for synth fonts */

  INT16			foInternalLeading;
  INT16			foAvgCharWidth;
  UINT16		fo_flags;

  /* font cache housekeeping */

  UINT16                count;
  UINT16                lru;
  UINT16                lfchecksum;
  LOGFONT16             lf;
} fontObject;

typedef struct
{
  fontResource*		pfr;
  fontInfo*		pfi;
  UINT16		height;
  UINT16		flags;
  LPLOGFONT16		plf;
} fontMatch;

typedef struct
{
  LPLOGFONT16		lpLogFontParam;
  FONTENUMPROC16	lpEnumFunc;
  LPARAM		lpData;

  LPNEWTEXTMETRICEX16	lpTextMetric;
  LPENUMLOGFONTEX16	lpLogFont;
  SEGPTR		segTextMetric;
  SEGPTR		segLogFont;
} fontEnum16;

typedef struct
{
  LPLOGFONT32W		lpLogFontParam;
  FONTENUMPROC32W	lpEnumFunc;
  LPARAM		lpData;

  LPNEWTEXTMETRICEX32W  lpTextMetric;
  LPENUMLOGFONTEX32W    lpLogFont;
  DWORD			dwFlags;
} fontEnum32;

extern fontObject* XFONT_GetFontObject( X_PHYSFONT pFont );
extern XFontStruct* XFONT_GetFontStruct( X_PHYSFONT pFont );
extern LPIFONTINFO16 XFONT_GetFontInfo( X_PHYSFONT pFont );

#endif __WINE_X11FONT_H 

