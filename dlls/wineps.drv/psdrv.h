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

/* Note no 'next' in AFM. Use AFMLISTENTRY as a container. This allow more than
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

extern FONTFAMILY   *PSDRV_AFMFontList;
extern const AFM    *const PSDRV_BuiltinAFMs[];     /* last element is NULL */

typedef struct _tagFONTNAME {
    char		*Name;
    struct _tagFONTNAME *next;
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

typedef struct _tagOPTIONENTRY {
    char			*Name;		/* eg "True" */
    char			*FullName;	/* eg "Installed" */
    char			*InvocationString; /* Often NULL */
    struct _tagOPTIONENTRY	*next;
} OPTIONENTRY;

typedef struct _tagOPTION { /* Treat bool as a special case of pickone */
    char			*OptionName;	/* eg "*Option1" */
    char			*FullName;	/* eg "Envelope Feeder" */
    char			*DefaultOption; /* eg "False" */
    OPTIONENTRY			*Options;
    struct _tagOPTION		*next;
} OPTION;

typedef struct _tagCONSTRAINT {
    char			*Feature1;
    char			*Value1;
    char			*Feature2;
    char			*Value2;
    struct _tagCONSTRAINT	*next;
} CONSTRAINT;

typedef struct _tagINPUTSLOT {
    const char			*Name;
    const char			*FullName;
    char			*InvocationString;
    WORD			WinBin; /* eg DMBIN_LOWER */
    struct _tagINPUTSLOT	*next;
} INPUTSLOT;

typedef enum _RASTERIZEROPTION
  {RO_None, RO_Accept68K, RO_Type42, RO_TrueImage} RASTERIZEROPTION;

typedef struct _tagDUPLEX {
    char                        *Name;
    char                        *FullName;
    char                        *InvocationString;
    WORD                        WinDuplex; /* eg DMDUP_SIMPLEX */
    struct _tagDUPLEX           *next;
} DUPLEX;

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
    int			DefaultResolution;
    signed int		LandscapeOrientation;
    char		*JCLBegin;
    char		*JCLToPSInterpreter;
    char		*JCLEnd;
    char		*DefaultFont;
    FONTNAME		*InstalledFonts; /* ptr to a list of FontNames */
    struct list         PageSizes;
    PAGESIZE            *DefaultPageSize;
    OPTION		*InstalledOptions;
    CONSTRAINT		*Constraints;
    INPUTSLOT		*InputSlots;
    RASTERIZEROPTION    TTRasterizer;
    DUPLEX              *Duplexes;
    DUPLEX              *DefaultDuplex;
} PPD;

typedef struct {
    DEVMODEA			dmPublic;
    struct _tagdocprivate {
      int dummy;
    }				dmDocPrivate;
    struct _tagdrvprivate {
      UINT	numInstalledOptions; /* Options at end of struct */
    }				dmDrvPrivate;

/* Now comes:

numInstalledOptions of OPTIONs

*/

} PSDRV_DEVMODEA;

typedef struct _tagPI {
    char		    *FriendlyName;
    PPD			    *ppd;
    PSDRV_DEVMODEA	    *Devmode;
    FONTFAMILY		    *Fonts;
    PPRINTER_ENUM_VALUESA   FontSubTable;
    DWORD		    FontSubTableSize;
    struct _tagPI	    *next;
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

typedef struct {
    enum fontloc        fontloc;
    union {
        BUILTIN  Builtin;
        DOWNLOAD *Download;
    }                   fontinfo;

    matrix              size;
    PSCOLOR             color;
    BOOL                set;		/* Have we done a setfont yet */

  /* These are needed by PSDRV_ExtTextOut */
    int                 escapement;
    int                 underlineThickness;
    int                 underlinePosition;
    int                 strikeoutThickness;
    int                 strikeoutPosition;

} PSFONT;

typedef struct {
    PSCOLOR		color;
    BOOL		set;
} PSBRUSH;

typedef struct {
    INT                 style;
    INT                 width;
    BYTE                join;
    BYTE                endcap;
    const char*		dash;
    PSCOLOR		color;
    BOOL		set;
} PSPEN;

typedef struct {
    DWORD		id;             /* Job id */
    HANDLE              hprinter;       /* Printer handle */
    LPSTR		output;		/* Output file/port */
    LPSTR               DocName;        /* Document Name */
    BOOL		banding;        /* Have we received a NEXTBAND */
    BOOL		OutOfPage;      /* Page header not sent yet */
    INT			PageNo;
    BOOL                quiet;          /* Don't actually output anything */
    BOOL                in_passthrough; /* In PASSTHROUGH mode */
    BOOL                had_passthrough_rect; /* See the comment in PSDRV_Rectangle */
} JOB;

typedef struct {
    HDC                 hdc;
    PSFONT		font;		/* Current PS font */
    DOWNLOAD            *downloaded_fonts;
    PSPEN		pen;
    PSBRUSH		brush;
    PSCOLOR		bkColor;
    PSCOLOR		inkColor;	/* Last colour set */
    JOB			job;
    PSDRV_DEVMODEA	*Devmode;
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

typedef struct {
    PRINTERINFO *pi;
    PSDRV_DEVMODEA *dlgdm;
} PSDRV_DLGINFO;


/*
 *  Every glyph name in the Adobe Glyph List and the 35 core PostScript fonts
 */

extern const INT    PSDRV_AGLGlyphNamesSize;
extern GLYPHNAME    PSDRV_AGLGlyphNames[];


/*
 *  The AGL encoding vector
 */

extern const INT    	    PSDRV_AGLbyNameSize;    /* sorted by name -     */
extern const UNICODEGLYPH   PSDRV_AGLbyName[];	    /*  duplicates omitted  */

extern const INT    	    PSDRV_AGLbyUVSize;	    /* sorted by UV -	    */
extern const UNICODEGLYPH   PSDRV_AGLbyUV[];	    /*  duplicates included */

extern HINSTANCE PSDRV_hInstance;
extern HANDLE PSDRV_Heap;
extern char *PSDRV_ANSIVector[256];

extern void PSDRV_MergeDevmodes(PSDRV_DEVMODEA *dm1, PSDRV_DEVMODEA *dm2,
			 PRINTERINFO *pi);
extern BOOL PSDRV_GetFontMetrics(void);
extern PPD *PSDRV_ParsePPD(char *fname);
extern PRINTERINFO *PSDRV_FindPrinterInfo(LPCSTR name);
extern const AFM *PSDRV_FindAFMinList(FONTFAMILY *head, LPCSTR name);
extern BOOL PSDRV_AddAFMtoList(FONTFAMILY **head, const AFM *afm,
    	BOOL *p_added);
extern void PSDRV_FreeAFMList( FONTFAMILY *head );

extern INT PSDRV_XWStoDS( PSDRV_PDEVICE *physDev, INT width );
extern INT PSDRV_YWStoDS( PSDRV_PDEVICE *physDev, INT height );

extern BOOL PSDRV_Brush(PSDRV_PDEVICE *physDev, BOOL EO);
extern BOOL PSDRV_SetFont( PSDRV_PDEVICE *physDev );
extern BOOL PSDRV_SetPen( PSDRV_PDEVICE *physDev );

extern void PSDRV_SetClip(PSDRV_PDEVICE* phyDev);
extern void PSDRV_ResetClip(PSDRV_PDEVICE* phyDev);

extern BOOL PSDRV_CopyColor(PSCOLOR *col1, PSCOLOR *col2);
extern void PSDRV_CreateColor( PSDRV_PDEVICE *physDev, PSCOLOR *pscolor,
		     COLORREF wincolor );
extern char PSDRV_UnicodeToANSI(int u);

extern INT PSDRV_WriteHeader( PSDRV_PDEVICE *physDev, LPCSTR title );
extern INT PSDRV_WriteFooter( PSDRV_PDEVICE *physDev );
extern INT PSDRV_WriteNewPage( PSDRV_PDEVICE *physDev );
extern INT PSDRV_WriteEndPage( PSDRV_PDEVICE *physDev );
extern BOOL PSDRV_WriteMoveTo(PSDRV_PDEVICE *physDev, INT x, INT y);
extern BOOL PSDRV_WriteLineTo(PSDRV_PDEVICE *physDev, INT x, INT y);
extern BOOL PSDRV_WriteStroke(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteRectangle(PSDRV_PDEVICE *physDev, INT x, INT y, INT width,
			INT height);
extern BOOL PSDRV_WriteRRectangle(PSDRV_PDEVICE *physDev, INT x, INT y, INT width,
			INT height);
extern BOOL PSDRV_WriteSetFont(PSDRV_PDEVICE *physDev, const char *name, matrix size,
                               INT escapement);
extern BOOL PSDRV_WriteGlyphShow(PSDRV_PDEVICE *physDev, LPCSTR g_name);
extern BOOL PSDRV_WriteSetPen(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteArc(PSDRV_PDEVICE *physDev, INT x, INT y, INT w, INT h,
			     double ang1, double ang2);
extern BOOL PSDRV_WriteSetColor(PSDRV_PDEVICE *physDev, PSCOLOR *color);
extern BOOL PSDRV_WriteSetBrush(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteFill(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteEOFill(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteGSave(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteGRestore(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteNewPath(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteClosePath(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteClip(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteRectClip(PSDRV_PDEVICE *physDev, INT x, INT y, INT w, INT h);
extern BOOL PSDRV_WriteRectClip2(PSDRV_PDEVICE *physDev, CHAR *pszArrayName);
extern BOOL PSDRV_WriteEOClip(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteHatch(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteRotate(PSDRV_PDEVICE *physDev, float ang);
extern BOOL PSDRV_WriteIndexColorSpaceBegin(PSDRV_PDEVICE *physDev, int size);
extern BOOL PSDRV_WriteIndexColorSpaceEnd(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteRGB(PSDRV_PDEVICE *physDev, COLORREF *map, int number);
extern BOOL PSDRV_WriteImage(PSDRV_PDEVICE *physDev, WORD depth, INT xDst, INT yDst,
			     INT widthDst, INT heightDst, INT widthSrc,
			     INT heightSrc, BOOL mask);
extern BOOL PSDRV_WriteBytes(PSDRV_PDEVICE *physDev, const BYTE *bytes, DWORD number);
extern BOOL PSDRV_WriteData(PSDRV_PDEVICE *physDev, const BYTE *byte, DWORD number);
extern DWORD PSDRV_WriteSpool(PSDRV_PDEVICE *physDev, LPCSTR lpData, DWORD cch);
extern BOOL PSDRV_WritePatternDict(PSDRV_PDEVICE *physDev, BITMAP *bm, BYTE *bits);
extern BOOL PSDRV_WriteDIBPatternDict(PSDRV_PDEVICE *physDev, BITMAPINFO *bmi, UINT usage);
extern BOOL PSDRV_WriteArrayPut(PSDRV_PDEVICE *physDev, CHAR *pszArrayName, INT nIndex, LONG lCoord);
extern BOOL PSDRV_WriteArrayDef(PSDRV_PDEVICE *physDev, CHAR *pszArrayName, INT nSize);

extern INT CDECL PSDRV_StartPage( PSDRV_PDEVICE *physDev );

INT PSDRV_GlyphListInit(void);
const GLYPHNAME *PSDRV_GlyphName(LPCSTR szName);
VOID PSDRV_IndexGlyphList(void);
BOOL PSDRV_GetTrueTypeMetrics(void);
BOOL PSDRV_GetType1Metrics(void);
const AFMMETRICS *PSDRV_UVMetrics(LONG UV, const AFM *afm);
SHORT PSDRV_CalcAvgCharWidth(const AFM *afm);

extern BOOL PSDRV_SelectBuiltinFont(PSDRV_PDEVICE *physDev, HFONT hfont,
				    LOGFONTW *plf, LPSTR FaceName);
extern BOOL PSDRV_WriteSetBuiltinFont(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteBuiltinGlyphShow(PSDRV_PDEVICE *physDev, LPCWSTR str, INT count);

extern BOOL PSDRV_SelectDownloadFont(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteSetDownloadFont(PSDRV_PDEVICE *physDev);
extern BOOL PSDRV_WriteDownloadGlyphShow(PSDRV_PDEVICE *physDev, WORD *glpyhs,
					 UINT count);
extern BOOL PSDRV_EmptyDownloadList(PSDRV_PDEVICE *physDev, BOOL write_undef);

extern DWORD write_spool( PSDRV_PDEVICE *physDev, const void *data, DWORD num );

#define MAX_G_NAME 31 /* max length of PS glyph name */
extern void get_glyph_name(HDC hdc, WORD index, char *name);

extern TYPE1 *T1_download_header(PSDRV_PDEVICE *physDev, char *ps_name,
                                 RECT *bbox, UINT emsize);
extern BOOL T1_download_glyph(PSDRV_PDEVICE *physDev, DOWNLOAD *pdl,
			      DWORD index, char *glyph_name);
extern void T1_free(TYPE1 *t1);

extern TYPE42 *T42_download_header(PSDRV_PDEVICE *physDev, char *ps_name,
                                   RECT *bbox, UINT emsize);
extern BOOL T42_download_glyph(PSDRV_PDEVICE *physDev, DOWNLOAD *pdl,
			       DWORD index, char *glyph_name);
extern void T42_free(TYPE42 *t42);

extern DWORD RLE_encode(BYTE *in_buf, DWORD len, BYTE *out_buf);
extern DWORD ASCII85_encode(BYTE *in_buf, DWORD len, BYTE *out_buf);

#define push_lc_numeric(x) do {					\
	const char *tmplocale = setlocale(LC_NUMERIC,NULL);	\
	setlocale(LC_NUMERIC,x);

#define pop_lc_numeric()					\
	setlocale(LC_NUMERIC,tmplocale);			\
} while (0)


#endif
