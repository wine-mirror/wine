#ifndef __WINE_PSDRV_H
#define __WINE_PSDRV_H

/*
 *	PostScript driver definitions
 *
 *	Copyright 1998  Huw D M Davies
 */
#include "wingdi.h"
#include "font.h"
#include "pen.h"
#include "brush.h"
#include "wine/wingdi16.h"

typedef struct {
    float	llx, lly, urx, ury;
} AFMBBOX;

typedef struct _tagAFMLIGS {
    char		*successor;
    char		*ligature;
    struct _tagAFMLIGS	*next;
} AFMLIGS;

typedef struct _tagAFMMETRICS {
    int				C;			/* character */  
    float			WX;
    char			*N;			/* name */
    AFMBBOX			B;
    AFMLIGS			*L;			/* Ligatures */
    struct _tagAFMMETRICS	*next;
} AFMMETRICS;

typedef struct _tagAFM {
    char		*FontName;
    char		*FullName;
    char		*FamilyName;
    char		*EncodingScheme;
    int			Weight;			/* FW_NORMAL etc. */
    float		ItalicAngle;
    BOOL		IsFixedPitch;
    float		UnderlinePosition;
    float		UnderlineThickness;
    AFMBBOX		FontBBox;
    float		CapHeight;
    float		XHeight;
    float		Ascender;
    float		Descender;
    float		FullAscender;		/* Ascent of Aring character */
    float		CharWidths[256];
    int			NumofMetrics;
    AFMMETRICS		*Metrics;
} AFM; /* CharWidths is a shortcut to the WX values of numbered glyphs */

/* Note no 'next' in AFM. Use AFMLISTENTRY as a container. This allow more than
   one list to exist without having to reallocate the entire AFM structure. We
   keep a global list of all afms (PSDRV_AFMFontList) plus a list of available
   fonts for each DC (dc->physDev->Fonts) */

typedef struct _tagAFMLISTENTRY {
    AFM				*afm;
    struct _tagAFMLISTENTRY	*next;
} AFMLISTENTRY;

typedef struct _tagFONTFAMILY {
    char			*FamilyName; /* family name */
    AFMLISTENTRY		*afmlist;    /* list of afms for this family */
    struct _tagFONTFAMILY	*next;       /* next family */
} FONTFAMILY;

extern FONTFAMILY *PSDRV_AFMFontList;

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

typedef struct _tagPAGESIZE {
    char		*Name;
    char		*FullName;
    char		*InvocationString;
    IMAGEABLEAREA	*ImageableArea;
    PAPERDIMENSION	*PaperDimension;
    WORD		WinPage; /*eg DMPAPER_A4. Doesn't really belong here */
    struct _tagPAGESIZE *next;
} PAGESIZE;


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
    char			*Name;
    char			*FullName;
    char			*InvocationString;
    WORD			WinBin; /* eg DMBIN_LOWER */
    struct _tagINPUTSLOT	*next;
} INPUTSLOT;

typedef struct {
    char		*NickName;
    int			LanguageLevel;
    BOOL		ColorDevice;
    int			DefaultResolution;
    signed int		LandscapeOrientation;
    char		*JCLBegin;
    char		*JCLToPSInterpreter;
    char		*JCLEnd;
    char		*DefaultFont;
    FONTNAME		*InstalledFonts; /* ptr to a list of FontNames */
    PAGESIZE		*PageSizes;
    OPTION		*InstalledOptions;
    CONSTRAINT		*Constraints;
    INPUTSLOT		*InputSlots;
} PPD;

typedef struct {
    DEVMODEA			dmPublic;
    struct _tagdocprivate {
      int dummy;
    }				dmDocPrivate;
    struct _tagdrvprivate {
      char	ppdFileName[256]; /* Hack */
      UINT	numInstalledOptions; /* Options at end of struct */
    }				dmDrvPrivate;

/* Now comes:

numInstalledOptions of OPTIONs

*/

} PSDRV_DEVMODEA;

typedef struct _tagPI {
    char		*FriendlyName;
    PPD			*ppd;
    PSDRV_DEVMODEA	*Devmode;
    FONTFAMILY		*Fonts;
    struct _tagPI	*next;
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
    AFM			*afm;
    TEXTMETRICA	tm;
    INT		size;
    float		scale;
    INT		escapement;
    PSCOLOR		color;
    BOOL		set;		/* Have we done a setfont yet */
} PSFONT;

typedef struct {
    PSCOLOR		color;
    BOOL		set;
} PSBRUSH;

typedef struct {
    INT                 style;
    INT		width;
    char		*dash;
    PSCOLOR		color;
    BOOL		set;
} PSPEN;

typedef struct {
    HANDLE16		hJob;
    LPSTR		output;		/* Output file/port */
    BOOL		banding;        /* Have we received a NEXTBAND */
    BOOL		OutOfPage;      /* Page header not sent yet */
    INT			PageNo;
} JOB;

typedef struct {
    PSFONT		font;		/* Current PS font */
    PSPEN		pen;
    PSBRUSH		brush;
    PSCOLOR		bkColor;
    PSCOLOR		inkColor;	/* Last colour set */
    JOB			job;
    PSDRV_DEVMODEA	*Devmode;
    PRINTERINFO		*pi;
} PSDRV_PDEVICE;


extern INT16 WINAPI PSDRV_ExtDeviceMode16(HWND16 hwnd, HANDLE16 hDriver,
		    LPDEVMODEA lpdmOutput, LPSTR lpszDevice, LPSTR lpszPort,
		    LPDEVMODEA lpdmInput, LPSTR lpszProfile, WORD fwMode);

extern HANDLE PSDRV_Heap;
extern char *PSDRV_ANSIVector[256];

extern void PSDRV_MergeDevmodes(PSDRV_DEVMODEA *dm1, PSDRV_DEVMODEA *dm2,
			 PRINTERINFO *pi);
extern BOOL PSDRV_GetFontMetrics(void);
extern PPD *PSDRV_ParsePPD(char *fname);
extern PRINTERINFO *PSDRV_FindPrinterInfo(LPCSTR name);
extern AFM *PSDRV_FindAFMinList(FONTFAMILY *head, char *name);
extern void PSDRV_AddAFMtoList(FONTFAMILY **head, AFM *afm);
extern void PSDRV_FreeAFMList( FONTFAMILY *head );

extern BOOL PSDRV_Init(void);
extern HFONT16 PSDRV_FONT_SelectObject( DC *dc, HFONT16 hfont, FONTOBJ *font);
extern HPEN PSDRV_PEN_SelectObject( DC * dc, HPEN hpen, PENOBJ * pen );
extern HBRUSH PSDRV_BRUSH_SelectObject( DC * dc, HBRUSH hbrush,
					  BRUSHOBJ * brush );

extern BOOL PSDRV_Brush(DC *dc, BOOL EO);
extern BOOL PSDRV_SetFont( DC *dc );
extern BOOL PSDRV_SetPen( DC *dc );

extern BOOL PSDRV_CmpColor(PSCOLOR *col1, PSCOLOR *col2);
extern BOOL PSDRV_CopyColor(PSCOLOR *col1, PSCOLOR *col2);
extern void PSDRV_CreateColor( PSDRV_PDEVICE *physDev, PSCOLOR *pscolor,
		     COLORREF wincolor );


extern INT PSDRV_WriteHeader( DC *dc, LPCSTR title );
extern INT PSDRV_WriteFooter( DC *dc );
extern INT PSDRV_WriteNewPage( DC *dc );
extern INT PSDRV_WriteEndPage( DC *dc );
extern BOOL PSDRV_WriteMoveTo(DC *dc, INT x, INT y);
extern BOOL PSDRV_WriteLineTo(DC *dc, INT x, INT y);
extern BOOL PSDRV_WriteStroke(DC *dc);
extern BOOL PSDRV_WriteRectangle(DC *dc, INT x, INT y, INT width, 
			INT height);
extern BOOL PSDRV_WriteRRectangle(DC *dc, INT x, INT y, INT width, 
			INT height);
extern BOOL PSDRV_WriteSetFont(DC *dc, BOOL UseANSI);
extern BOOL PSDRV_WriteShow(DC *dc, char *str, INT count);
extern BOOL PSDRV_WriteReencodeFont(DC *dc);
extern BOOL PSDRV_WriteSetPen(DC *dc);
extern BOOL PSDRV_WriteArc(DC *dc, INT x, INT y, INT w, INT h,
			     double ang1, double ang2);
extern BOOL PSDRV_WriteSetColor(DC *dc, PSCOLOR *color);
extern BOOL PSDRV_WriteSetBrush(DC *dc);
extern BOOL PSDRV_WriteFill(DC *dc);
extern BOOL PSDRV_WriteEOFill(DC *dc);
extern BOOL PSDRV_WriteGSave(DC *dc);
extern BOOL PSDRV_WriteGRestore(DC *dc);
extern BOOL PSDRV_WriteNewPath(DC *dc);
extern BOOL PSDRV_WriteClosePath(DC *dc);
extern BOOL PSDRV_WriteClip(DC *dc);
extern BOOL PSDRV_WriteRectClip(DC *dc, CHAR *pszArrayName);
extern BOOL PSDRV_WriteEOClip(DC *dc);
extern BOOL PSDRV_WriteHatch(DC *dc);
extern BOOL PSDRV_WriteRotate(DC *dc, float ang);
extern BOOL PSDRV_WriteIndexColorSpaceBegin(DC *dc, int size);
extern BOOL PSDRV_WriteIndexColorSpaceEnd(DC *dc);
extern BOOL PSDRV_WriteRGB(DC *dc, COLORREF *map, int number);
extern BOOL PSDRV_WriteImageDict(DC *dc, WORD depth, INT xDst, INT yDst,
				 INT widthDst, INT heightDst, INT widthSrc,
				 INT heightSrc, char *bits);
extern BOOL PSDRV_WriteBytes(DC *dc, const BYTE *bytes, int number);
extern BOOL PSDRV_WriteDIBits16(DC *dc, const WORD *words, int number);
extern BOOL PSDRV_WriteDIBits24(DC *dc, const BYTE *bits, int number);
extern BOOL PSDRV_WriteDIBits32(DC *dc, const BYTE *bits, int number);
extern int PSDRV_WriteSpool(DC *dc, LPSTR lpData, WORD cch);
extern BOOL PSDRV_WritePatternDict(DC *dc, BITMAP *bm, BYTE *bits);
extern BOOL PSDRV_WriteArrayPut(DC *dc, CHAR *pszArrayName, INT nIndex, LONG lCoord);
extern BOOL PSDRV_WriteArrayDef(DC *dc, CHAR *pszArrayName, INT nSize);

extern BOOL PSDRV_Arc( DC *dc, INT left, INT top, INT right,
			 INT bottom, INT xstart, INT ystart,
			 INT xend, INT yend );
extern BOOL PSDRV_Chord( DC *dc, INT left, INT top, INT right,
			   INT bottom, INT xstart, INT ystart,
			   INT xend, INT yend );
extern BOOL PSDRV_Ellipse( DC *dc, INT left, INT top, INT right,
			     INT bottom );
extern INT PSDRV_EndDoc( DC *dc );
extern INT PSDRV_EndPage( DC *dc );
extern BOOL PSDRV_EnumDeviceFonts( DC* dc, LPLOGFONT16 plf, 
				     DEVICEFONTENUMPROC proc, LPARAM lp );
extern INT PSDRV_Escape( DC *dc, INT nEscape, INT cbInput, 
			   SEGPTR lpInData, SEGPTR lpOutData );
extern BOOL PSDRV_ExtTextOut( DC *dc, INT x, INT y, UINT flags,
				const RECT *lprect, LPCSTR str, UINT count,
				const INT *lpDx );
extern BOOL PSDRV_GetCharWidth( DC *dc, UINT firstChar, UINT lastChar,
				  LPINT buffer );
extern BOOL PSDRV_GetTextExtentPoint( DC *dc, LPCSTR str, INT count,
					LPSIZE size );
extern BOOL PSDRV_GetTextMetrics( DC *dc, TEXTMETRICA *metrics );
extern BOOL PSDRV_LineTo( DC *dc, INT x, INT y );
extern BOOL PSDRV_MoveToEx( DC *dc, INT x, INT y, LPPOINT pt );
extern BOOL PSDRV_PatBlt( DC *dc, INT x, INT y, INT width, INT height, DWORD
			  dwRop);
extern BOOL PSDRV_Pie( DC *dc, INT left, INT top, INT right,
			 INT bottom, INT xstart, INT ystart,
			 INT xend, INT yend );
extern BOOL PSDRV_Polygon( DC *dc, const POINT* pt, INT count );
extern BOOL PSDRV_Polyline( DC *dc, const POINT* pt, INT count );
extern BOOL PSDRV_PolyPolygon( DC *dc, const POINT* pts, const INT* counts,
				 UINT polygons );
extern BOOL PSDRV_PolyPolyline( DC *dc, const POINT* pts, const DWORD* counts,
				  DWORD polylines );
extern BOOL PSDRV_Rectangle( DC *dc, INT left, INT top, INT right,
			      INT bottom );
extern BOOL PSDRV_RoundRect(DC *dc, INT left, INT top, INT right,
			      INT bottom, INT ell_width, INT ell_height);
extern HGDIOBJ PSDRV_SelectObject( DC *dc, HGDIOBJ handle );
extern COLORREF PSDRV_SetBkColor( DC *dc, COLORREF color );
extern VOID PSDRV_SetDeviceClipping( DC *dc );
extern COLORREF PSDRV_SetPixel( DC *dc, INT x, INT y, COLORREF color );
extern COLORREF PSDRV_SetTextColor( DC *dc, COLORREF color );
extern INT PSDRV_StartDoc( DC *dc, const DOCINFOA *doc );
extern INT PSDRV_StartPage( DC *dc );
extern INT PSDRV_StretchDIBits( DC *dc, INT xDst, INT yDst,
				INT widthDst, INT heightDst, INT xSrc,
				INT ySrc, INT widthSrc, INT heightSrc,
				const void *bits, const BITMAPINFO *info,
				UINT wUsage, DWORD dwRop );

extern INT PSDRV_ExtDeviceMode(LPSTR lpszDriver, HWND hwnd,
			       LPDEVMODEA lpdmOutput,
			       LPSTR lpszDevice, LPSTR lpszPort,
			       LPDEVMODEA lpdmInput, LPSTR lpszProfile,
			       DWORD dwMode);
extern DWORD PSDRV_DeviceCapabilities(LPSTR lpszDriver, LPCSTR lpszDevice,
				      LPCSTR lpszPort,
				      WORD fwCapability, LPSTR lpszOutput,
				      LPDEVMODEA lpdm);
VOID PSDRV_DrawLine( DC *dc );
#endif


