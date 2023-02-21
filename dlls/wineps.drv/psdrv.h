/*
 *	PostScript driver definitions
 *
 *	Copyright 1998  Huw D M Davies
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

#ifndef __WINE_PSDRV_H
#define __WINE_PSDRV_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winspool.h"

#include "wine/gdi_driver.h"
#include "wine/list.h"

typedef struct {
    INT		    index;
    LPCSTR	    sz;
} GLYPHNAME;

typedef struct {
    LONG	    UV;
    const GLYPHNAME *name;
} UNICODEGLYPH;

typedef struct {
    float	llx, lly, urx, ury;
} AFMBBOX;

typedef struct _tagAFMLIGS {
    char		*successor;
    char		*ligature;
    struct _tagAFMLIGS	*next;
} AFMLIGS;

typedef struct {
    int			C;		/* character */
    LONG     	    	UV;
    float		WX;
    const GLYPHNAME	*N;		/* name */
    AFMBBOX		B;
    const AFMLIGS	*L;		/* Ligatures */
} OLD_AFMMETRICS;

typedef struct {
    INT     	    	C;  	    	    	/* AFM encoding (or -1) */
    LONG    	    	UV; 	    	    	/* Unicode value */
    FLOAT   	    	WX; 	    	    	/* Advance width */
    const GLYPHNAME 	*N; 	    	    	/* Glyph name */
} AFMMETRICS;

typedef struct {
    USHORT    	    	usUnitsPerEm; 	    	/* head:unitsPerEm */
    SHORT   	    	sAscender;  	    	/* hhea:Ascender */
    SHORT   	    	sDescender; 	    	/* hhea:Descender */
    SHORT   	    	sLineGap;   	    	/* hhea:LineGap */
    SHORT   	    	sAvgCharWidth;	    	/* OS/2:xAvgCharWidth */
    SHORT   	    	sTypoAscender;	    	/* OS/2:sTypoAscender */
    SHORT   	    	sTypoDescender;     	/* OS/2:sTypoDescender */
    SHORT   	    	sTypoLineGap;	    	/* OS/2:sTypeLineGap */
    USHORT  	    	usWinAscent;	    	/* OS/2:usWinAscent */
    USHORT  	    	usWinDescent;	    	/* OS/2:usWinDescent */
} WINMETRICS;

typedef struct _tagAFM {
    LPCSTR		FontName;
    LPCSTR		FullName;
    LPCSTR		FamilyName;
    LPCSTR		EncodingScheme;
    LONG		Weight;			/* FW_NORMAL etc. */
    float		ItalicAngle;
    BOOL		IsFixedPitch;
    float		UnderlinePosition;
    float		UnderlineThickness;
    AFMBBOX		FontBBox;
    float		Ascender;
    float		Descender;
    WINMETRICS	    	WinMetrics;
    int			NumofMetrics;
    const AFMMETRICS	*Metrics;
} AFM;

/* Note no 'next' in AFM. Use AFMLISTENTRY as a container. This allows more than
   one list to exist without having to reallocate the entire AFM structure. We
   keep a global list of all afms (PSDRV_AFMFontList) plus a list of available
   fonts for each DC (dc->physDev->Fonts) */

typedef struct _tagAFMLISTENTRY {
    const AFM			*afm;
    struct _tagAFMLISTENTRY	*next;
} AFMLISTENTRY;

typedef struct _tagFONTFAMILY {
    char			*FamilyName; /* family name */
    AFMLISTENTRY		*afmlist;    /* list of afms for this family */
    struct _tagFONTFAMILY	*next;       /* next family */
} FONTFAMILY;

extern FONTFAMILY   *PSDRV_AFMFontList DECLSPEC_HIDDEN;
extern const AFM    *const PSDRV_BuiltinAFMs[] DECLSPEC_HIDDEN;     /* last element is NULL */

typedef struct
{
    struct list          entry;
    char		*Name;
} FONTNAME;

typedef struct {
    float	llx, lly, urx, ury;
} IMAGEABLEAREA;

typedef struct {
    float	x, y;
} PAPERDIMENSION;

/* Solaris kludge */
#undef PAGESIZE
typedef struct _tagPAGESIZE {
    struct list         entry;
    char		*Name;
    char		*FullName;
    char		*InvocationString;
    IMAGEABLEAREA	*ImageableArea;
    PAPERDIMENSION	*PaperDimension;
    WORD		WinPage; /*eg DMPAPER_A4. Doesn't really belong here */
} PAGESIZE;


/* For BANDINFO Escape */
typedef struct _BANDINFOSTRUCT
{
    BOOL GraphicsFlag;
    BOOL TextFlag;
    RECT GraphicsRect;
} BANDINFOSTRUCT, *PBANDINFOSTRUCT;

typedef struct
{
    struct list                 entry;
    char			*Feature1;
    char			*Value1;
    char			*Feature2;
    char			*Value2;
} CONSTRAINT;

typedef struct
{
    struct list                 entry;
    const char			*Name;
    const char			*FullName;
    char			*InvocationString;
    WORD			WinBin; /* eg DMBIN_LOWER */
} INPUTSLOT;

typedef enum _RASTERIZEROPTION
  {RO_None, RO_Accept68K, RO_Type42, RO_TrueImage} RASTERIZEROPTION;

typedef struct
{
    struct list                 entry;
    char                        *Name;
    char                        *FullName;
    char                        *InvocationString;
    WORD                        WinDuplex; /* eg DMDUP_SIMPLEX */
} DUPLEX;

typedef struct
{
    struct list entry;
    int    resx, resy;
    char   *InvocationString;
} RESOLUTION;

/* Many Mac OS X based ppd files don't include a *ColorDevice line, so
   we use a tristate here rather than a boolean.  Code that
   cares is expected to treat these as if they were colour. */
typedef enum {
    CD_NotSpecified,
    CD_False,
    CD_True
} COLORDEVICE;

typedef struct {
    char		*NickName;
    int			LanguageLevel;
    COLORDEVICE	        ColorDevice;
    struct list         Resolutions;
    int			DefaultResolution;
    signed int		LandscapeOrientation;
    char		*JCLBegin;
    char		*JCLToPSInterpreter;
    char		*JCLEnd;
    char		*DefaultFont;
    struct list         InstalledFonts;
    struct list         PageSizes;
    PAGESIZE            *DefaultPageSize;
    struct list         Constraints;
    struct list         InputSlots;
    RASTERIZEROPTION    TTRasterizer;
    struct list         Duplexes;
    DUPLEX              *DefaultDuplex;
} PPD;

typedef struct {
    DEVMODEW			dmPublic;
    struct _tagdocprivate {
      int dummy;
    }				dmDocPrivate;
    struct _tagdrvprivate {
      UINT	numInstalledOptions; /* Options at end of struct */
    }				dmDrvPrivate;

/* Now comes:

numInstalledOptions of OPTIONs

*/

} PSDRV_DEVMODE;

typedef struct
{
    struct list             entry;
    WCHAR                   *friendly_name;
    PPD			    *ppd;
    PSDRV_DEVMODE	    *Devmode;
    FONTFAMILY		    *Fonts;
    PPRINTER_ENUM_VALUESA   FontSubTable;
    DWORD		    FontSubTableSize;
} PRINTERINFO;

typedef struct {
    float		r, g, b;
} PSRGB;

typedef struct {
    float		i;
} PSGRAY;


/* def's for PSCOLOR.type */
#define PSCOLOR_GRAY	0
#define PSCOLOR_RGB	1

typedef struct {
    int			type;
    union {
        PSRGB  rgb;
        PSGRAY gray;
    }                   value;
} PSCOLOR;

typedef struct {
    const AFM           *afm;
    float               scale;
    TEXTMETRICW         tm;
} BUILTIN;

typedef struct tagTYPE42 TYPE42;

typedef struct tagTYPE1 TYPE1;

enum downloadtype {
  Type1, Type42
};

typedef struct _tagDOWNLOAD {
  enum downloadtype type;
  union {
    TYPE1  *Type1;
    TYPE42 *Type42;
  } typeinfo;
  char *ps_name;
  struct _tagDOWNLOAD *next;
} DOWNLOAD;

enum fontloc {
  Builtin, Download
};

typedef struct
{
    INT xx, xy, yx, yy;
} matrix;

enum fontset { UNSET = 0, HORIZONTAL_SET, VERTICAL_SET };

typedef struct {
    enum fontloc        fontloc;
    union {
        BUILTIN  Builtin;
        DOWNLOAD *Download;
    }                   fontinfo;

    matrix              size;
    PSCOLOR             color;
    enum fontset        set;    /* Have we done a setfont yet */

  /* These are needed by PSDRV_ExtTextOut */
    int                 escapement;
    int                 underlineThickness;
    int                 underlinePosition;
    int                 strikeoutThickness;
    int                 strikeoutPosition;

} PSFONT;

typedef struct {
    PSCOLOR              color;
    BOOL                 set;
    struct brush_pattern pattern;
} PSBRUSH;

#define MAX_DASHLEN 16

typedef struct {
    INT                 style;
    INT                 width;
    BYTE                join;
    BYTE                endcap;
    DWORD               dash[MAX_DASHLEN];
    DWORD               dash_len;
    PSCOLOR		color;
    BOOL		set;
} PSPEN;

enum passthrough
{
    passthrough_none,
    passthrough_active,
    passthrough_had_rect,  /* See the comment in PSDRV_Rectangle */
};

typedef struct {
    DWORD		id;             /* Job id */
    HANDLE              hprinter;       /* Printer handle */
    LPWSTR              doc_name;       /* Document Name */
    BOOL		banding;        /* Have we received a NEXTBAND */
    BOOL		OutOfPage;      /* Page header not sent yet */
    INT			PageNo;
    BOOL                quiet;          /* Don't actually output anything */
    enum passthrough    passthrough_state;
} JOB;

typedef struct
{
    struct gdi_physdev  dev;
    PSFONT		font;		/* Current PS font */
    DOWNLOAD            *downloaded_fonts;
    PSPEN		pen;
    PSBRUSH		brush;
    PSCOLOR		bkColor;
    PSCOLOR		inkColor;	/* Last colour set */
    JOB			job;
    PSDRV_DEVMODE	*Devmode;
    PRINTERINFO		*pi;
    SIZE                PageSize;      /* Physical page size in device units */
    RECT                ImageableArea; /* Imageable area in device units */
                                       /* NB both PageSize and ImageableArea
					  are not rotated in landscape mode,
					  so PageSize.cx is generally
					  < PageSize.cy */
    int                 horzRes;       /* device caps */
    int                 vertRes;
    int                 horzSize;
    int                 vertSize;
    int                 logPixelsX;
    int                 logPixelsY;

    int                 pathdepth;
} PSDRV_PDEVICE;

static inline PSDRV_PDEVICE *get_psdrv_dev( PHYSDEV dev )
{
    return (PSDRV_PDEVICE *)dev;
}

/*
 *  Every glyph name in the Adobe Glyph List and the 35 core PostScript fonts
 */

extern const INT    PSDRV_AGLGlyphNamesSize DECLSPEC_HIDDEN;
extern GLYPHNAME    PSDRV_AGLGlyphNames[] DECLSPEC_HIDDEN;


/*
 *  The AGL encoding vector
 */

extern const INT    	    PSDRV_AGLbyNameSize DECLSPEC_HIDDEN; /* sorted by name */
extern const UNICODEGLYPH   PSDRV_AGLbyName[] DECLSPEC_HIDDEN;	 /*  duplicates omitted  */

extern const INT    	    PSDRV_AGLbyUVSize DECLSPEC_HIDDEN;	 /* sorted by UV */
extern const UNICODEGLYPH   PSDRV_AGLbyUV[] DECLSPEC_HIDDEN;	 /*  duplicates included */

extern HINSTANCE PSDRV_hInstance DECLSPEC_HIDDEN;
extern HANDLE PSDRV_Heap DECLSPEC_HIDDEN;
extern char *PSDRV_ANSIVector[256] DECLSPEC_HIDDEN;

extern INPUTSLOT *find_slot( PPD *ppd, const PSDRV_DEVMODE *dm ) DECLSPEC_HIDDEN;
extern PAGESIZE *find_pagesize( PPD *ppd, const PSDRV_DEVMODE *dm ) DECLSPEC_HIDDEN;
extern DUPLEX *find_duplex( PPD *ppd, const PSDRV_DEVMODE *dm ) DECLSPEC_HIDDEN;

/* GDI driver functions */
extern BOOL CDECL PSDRV_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                             INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                               INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom) DECLSPEC_HIDDEN;
extern INT CDECL PSDRV_EndDoc( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern INT CDECL PSDRV_EndPage( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_EnumFonts( PHYSDEV dev, LPLOGFONTW plf, FONTENUMPROCW proc, LPARAM lp ) DECLSPEC_HIDDEN;
extern INT CDECL PSDRV_ExtEscape( PHYSDEV dev, INT nEscape, INT cbInput, LPCVOID in_data,
                            INT cbOutput, LPVOID out_data ) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags,
                                    const RECT *lprect, LPCWSTR str, UINT count, const INT *lpDx ) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_FillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_GetCharWidth(PHYSDEV dev, UINT first, UINT count, const WCHAR *chars, INT *buffer) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_GetTextExtentExPoint(PHYSDEV dev, LPCWSTR str, INT count, LPINT alpDx) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_GetTextMetrics(PHYSDEV dev, TEXTMETRICW *metrics) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_LineTo(PHYSDEV dev, INT x, INT y) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_PaintRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_PatBlt(PHYSDEV dev, struct bitblt_coords *dst, DWORD dwRop) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                             INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_PolyBezier( PHYSDEV dev, const POINT *pts, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_PolyBezierTo( PHYSDEV dev, const POINT *pts, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_PolyPolygon( PHYSDEV dev, const POINT* pts, const INT* counts, UINT polygons ) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_PolyPolyline( PHYSDEV dev, const POINT* pts, const DWORD* counts, DWORD polylines ) DECLSPEC_HIDDEN;
extern DWORD CDECL PSDRV_PutImage( PHYSDEV dev, HRGN clip, BITMAPINFO *info,
                                   const struct gdi_image_bits *bits, struct bitblt_coords *src,
                                   struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_Rectangle( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_RoundRect( PHYSDEV dev, INT left, INT top, INT right,
                                   INT bottom, INT ell_width, INT ell_height ) DECLSPEC_HIDDEN;
extern HBRUSH CDECL PSDRV_SelectBrush( PHYSDEV dev, HBRUSH hbrush, const struct brush_pattern *pattern ) DECLSPEC_HIDDEN;
extern HFONT CDECL PSDRV_SelectFont( PHYSDEV dev, HFONT hfont, UINT *aa_flags ) DECLSPEC_HIDDEN;
extern HPEN CDECL PSDRV_SelectPen( PHYSDEV dev, HPEN hpen, const struct brush_pattern *pattern ) DECLSPEC_HIDDEN;
extern COLORREF CDECL PSDRV_SetBkColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF CDECL PSDRV_SetDCBrushColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF CDECL PSDRV_SetDCPenColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF CDECL PSDRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF CDECL PSDRV_SetTextColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern INT CDECL PSDRV_StartDoc( PHYSDEV dev, const DOCINFOW *doc ) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_StrokeAndFillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL PSDRV_StrokePath( PHYSDEV dev ) DECLSPEC_HIDDEN;

extern void PSDRV_MergeDevmodes(PSDRV_DEVMODE *dm1, const PSDRV_DEVMODE *dm2,
			 PRINTERINFO *pi) DECLSPEC_HIDDEN;
extern BOOL PSDRV_GetFontMetrics(void) DECLSPEC_HIDDEN;
extern PPD *PSDRV_ParsePPD(const WCHAR *fname, HANDLE printer) DECLSPEC_HIDDEN;
extern PRINTERINFO *PSDRV_FindPrinterInfo(LPCWSTR name) DECLSPEC_HIDDEN;
extern const AFM *PSDRV_FindAFMinList(FONTFAMILY *head, LPCSTR name) DECLSPEC_HIDDEN;
extern BOOL PSDRV_AddAFMtoList(FONTFAMILY **head, const AFM *afm,
	BOOL *p_added) DECLSPEC_HIDDEN;
extern void PSDRV_FreeAFMList( FONTFAMILY *head ) DECLSPEC_HIDDEN;

extern INT PSDRV_XWStoDS( PHYSDEV dev, INT width ) DECLSPEC_HIDDEN;

extern BOOL PSDRV_Brush(PHYSDEV dev, BOOL EO) DECLSPEC_HIDDEN;
extern BOOL PSDRV_SetFont( PHYSDEV dev, BOOL vertical ) DECLSPEC_HIDDEN;
extern BOOL PSDRV_SetPen( PHYSDEV dev ) DECLSPEC_HIDDEN;

extern void PSDRV_AddClip( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern void PSDRV_SetClip( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern void PSDRV_ResetClip( PHYSDEV dev ) DECLSPEC_HIDDEN;

extern BOOL PSDRV_CopyColor(PSCOLOR *col1, PSCOLOR *col2) DECLSPEC_HIDDEN;
extern void PSDRV_CreateColor( PHYSDEV dev, PSCOLOR *pscolor,
		     COLORREF wincolor ) DECLSPEC_HIDDEN;
extern PSRGB rgb_to_grayscale_scale( void ) DECLSPEC_HIDDEN;
extern char PSDRV_UnicodeToANSI(int u) DECLSPEC_HIDDEN;

extern INT PSDRV_WriteHeader( PHYSDEV dev, LPCWSTR title ) DECLSPEC_HIDDEN;
extern INT PSDRV_WriteFooter( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern INT PSDRV_WriteNewPage( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern INT PSDRV_WriteEndPage( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteMoveTo(PHYSDEV dev, INT x, INT y) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteLineTo(PHYSDEV dev, INT x, INT y) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteStroke(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteRectangle(PHYSDEV dev, INT x, INT y, INT width, INT height) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteRRectangle(PHYSDEV dev, INT x, INT y, INT width, INT height) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteSetFont(PHYSDEV dev, const char *name, matrix size, INT escapement,
                               BOOL fake_italic) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteGlyphShow(PHYSDEV dev, LPCSTR g_name) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteSetPen(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteArc(PHYSDEV dev, INT x, INT y, INT w, INT h,
			     double ang1, double ang2) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteCurveTo(PHYSDEV dev, POINT pts[3]) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteSetColor(PHYSDEV dev, PSCOLOR *color) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteSetBrush(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteFill(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteEOFill(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteGSave(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteGRestore(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteNewPath(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteClosePath(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteClip(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteRectClip(PHYSDEV dev, INT x, INT y, INT w, INT h) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteRectClip2(PHYSDEV dev, CHAR *pszArrayName) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteEOClip(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteHatch(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteRotate(PHYSDEV dev, float ang) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteIndexColorSpaceBegin(PHYSDEV dev, int size) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteIndexColorSpaceEnd(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteRGBQUAD(PHYSDEV dev, const RGBQUAD *rgb, int number) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteImage(PHYSDEV dev, WORD depth, BOOL grayscale, INT xDst, INT yDst,
			     INT widthDst, INT heightDst, INT widthSrc,
			     INT heightSrc, BOOL mask, BOOL top_down) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteBytes(PHYSDEV dev, const BYTE *bytes, DWORD number) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteData(PHYSDEV dev, const BYTE *byte, DWORD number) DECLSPEC_HIDDEN;
extern DWORD PSDRV_WriteSpool(PHYSDEV dev, LPCSTR lpData, DWORD cch) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteDIBPatternDict(PHYSDEV dev, const BITMAPINFO *bmi, BYTE *bits, UINT usage) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteArrayPut(PHYSDEV dev, CHAR *pszArrayName, INT nIndex, LONG lCoord) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteArrayDef(PHYSDEV dev, CHAR *pszArrayName, INT nSize) DECLSPEC_HIDDEN;

extern INT CDECL PSDRV_StartPage( PHYSDEV dev ) DECLSPEC_HIDDEN;

INT PSDRV_GlyphListInit(void) DECLSPEC_HIDDEN;
const GLYPHNAME *PSDRV_GlyphName(LPCSTR szName) DECLSPEC_HIDDEN;
VOID PSDRV_IndexGlyphList(void) DECLSPEC_HIDDEN;
BOOL PSDRV_GetType1Metrics(void) DECLSPEC_HIDDEN;
const AFMMETRICS *PSDRV_UVMetrics(LONG UV, const AFM *afm) DECLSPEC_HIDDEN;
SHORT PSDRV_CalcAvgCharWidth(const AFM *afm) DECLSPEC_HIDDEN;

extern BOOL PSDRV_SelectBuiltinFont(PHYSDEV dev, HFONT hfont,
				    LOGFONTW *plf, LPSTR FaceName) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteSetBuiltinFont(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteBuiltinGlyphShow(PHYSDEV dev, LPCWSTR str, INT count) DECLSPEC_HIDDEN;

extern BOOL PSDRV_SelectDownloadFont(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteSetDownloadFont(PHYSDEV dev, BOOL vertical) DECLSPEC_HIDDEN;
extern BOOL PSDRV_WriteDownloadGlyphShow(PHYSDEV dev, const WORD *glyphs, UINT count) DECLSPEC_HIDDEN;
extern BOOL PSDRV_EmptyDownloadList(PHYSDEV dev, BOOL write_undef) DECLSPEC_HIDDEN;

extern DWORD write_spool( PHYSDEV dev, const void *data, DWORD num ) DECLSPEC_HIDDEN;

#define MAX_G_NAME 31 /* max length of PS glyph name */
extern void get_glyph_name(HDC hdc, WORD index, char *name) DECLSPEC_HIDDEN;

extern TYPE1 *T1_download_header(PHYSDEV dev, char *ps_name,
                                 RECT *bbox, UINT emsize) DECLSPEC_HIDDEN;
extern BOOL T1_download_glyph(PHYSDEV dev, DOWNLOAD *pdl,
			      DWORD index, char *glyph_name) DECLSPEC_HIDDEN;
extern void T1_free(TYPE1 *t1) DECLSPEC_HIDDEN;

extern TYPE42 *T42_download_header(PHYSDEV dev, char *ps_name,
                                   RECT *bbox, UINT emsize) DECLSPEC_HIDDEN;
extern BOOL T42_download_glyph(PHYSDEV dev, DOWNLOAD *pdl,
			       DWORD index, char *glyph_name) DECLSPEC_HIDDEN;
extern void T42_free(TYPE42 *t42) DECLSPEC_HIDDEN;

extern DWORD RLE_encode(BYTE *in_buf, DWORD len, BYTE *out_buf) DECLSPEC_HIDDEN;
extern DWORD ASCII85_encode(BYTE *in_buf, DWORD len, BYTE *out_buf) DECLSPEC_HIDDEN;

extern void passthrough_enter(PHYSDEV dev) DECLSPEC_HIDDEN;
extern void passthrough_leave(PHYSDEV dev) DECLSPEC_HIDDEN;

#define push_lc_numeric(x) do {					\
	const char *tmplocale = setlocale(LC_NUMERIC,NULL);	\
	setlocale(LC_NUMERIC,x);

#define pop_lc_numeric()					\
	setlocale(LC_NUMERIC,tmplocale);			\
} while (0)

static inline WCHAR *strdupW( const WCHAR *str )
{
    int size;
    WCHAR *ret;

    if (!str) return NULL;
    size = (lstrlenW( str ) + 1) * sizeof(WCHAR);
    ret = HeapAlloc( GetProcessHeap(), 0, size );
    if (ret) memcpy( ret, str, size );
    return ret;
}

#endif
