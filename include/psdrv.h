
/*
 *	PostScript driver definitions
 *
 *	Copyright 1998  Huw D M Davies
 */
#include "windows.h"
#include "font.h"
#include "pen.h"
#include "brush.h"

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
    BOOL32		IsFixedPitch;
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
    BOOL32		ColorDevice;
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
    DEVMODE16			dmPublic;
    struct _tagdocprivate {
    }				dmDocPrivate;
    struct _tagdrvprivate {
      char	ppdFileName[100]; /* Hack */
      UINT32	numInstalledOptions; /* Options at end of struct */
    }				dmDrvPrivate;

/* Now comes:

numInstalledOptions of OPTIONs

*/

} PSDRV_DEVMODE16;

typedef struct _tagPI {
    char		*FriendlyName;
    PPD			*ppd;
    PSDRV_DEVMODE16	*Devmode;
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
    TEXTMETRIC32A	tm;
    INT32		size;
    float		scale;
    INT32		escapement;
    PSCOLOR		color;
    BOOL32		set;		/* Have we done a setfont yet */
} PSFONT;

typedef struct {
    PSCOLOR		color;
    BOOL32		set;
} PSBRUSH;

typedef struct {
    INT32		width;
    char		*dash;
    PSCOLOR		color;
    BOOL32		set;
} PSPEN;

typedef struct {
    HANDLE16		hJob;
    LPSTR		output;		/* Output file/port */
    BOOL32              banding;        /* Have we received a NEXTBAND */
    BOOL32		NeedPageHeader; /* Page header not sent yet */
    INT32		PageNo;
} JOB;

typedef struct {
    PSFONT		font;		/* Current PS font */
    PSPEN		pen;
    PSBRUSH		brush;
    PSCOLOR		bkColor;
    PSCOLOR		inkColor;	/* Last colour set */
    JOB			job;
    PSDRV_DEVMODE16	*Devmode;
    PRINTERINFO		*pi;
} PSDRV_PDEVICE;

extern HANDLE32 PSDRV_Heap;
extern char *PSDRV_ANSIVector[256];

extern void PSDRV_MergeDevmodes(PSDRV_DEVMODE16 *dm1, PSDRV_DEVMODE16 *dm2,
			 PRINTERINFO *pi);
extern BOOL32 PSDRV_GetFontMetrics(void);
extern PPD *PSDRV_ParsePPD(char *fname);
extern PRINTERINFO *PSDRV_FindPrinterInfo(LPCSTR name);
extern AFM *PSDRV_FindAFMinList(FONTFAMILY *head, char *name);
extern void PSDRV_AddAFMtoList(FONTFAMILY **head, AFM *afm);
extern void PSDRV_FreeAFMList( FONTFAMILY *head );

extern BOOL32 PSDRV_Init(void);
extern HFONT16 PSDRV_FONT_SelectObject( DC *dc, HFONT16 hfont, FONTOBJ *font);
extern HPEN32 PSDRV_PEN_SelectObject( DC * dc, HPEN32 hpen, PENOBJ * pen );
extern HBRUSH32 PSDRV_BRUSH_SelectObject( DC * dc, HBRUSH32 hbrush,
					  BRUSHOBJ * brush );

extern BOOL32 PSDRV_Brush(DC *dc, BOOL32 EO);
extern BOOL32 PSDRV_SetFont( DC *dc );
extern BOOL32 PSDRV_SetPen( DC *dc );

extern BOOL32 PSDRV_CmpColor(PSCOLOR *col1, PSCOLOR *col2);
extern BOOL32 PSDRV_CopyColor(PSCOLOR *col1, PSCOLOR *col2);
extern void PSDRV_CreateColor( PSDRV_PDEVICE *physDev, PSCOLOR *pscolor,
		     COLORREF wincolor );


extern INT32 PSDRV_WriteHeader( DC *dc, char *title, int len );
extern INT32 PSDRV_WriteFooter( DC *dc );
extern INT32 PSDRV_WriteNewPage( DC *dc );
extern INT32 PSDRV_WriteEndPage( DC *dc );
extern BOOL32 PSDRV_WriteMoveTo(DC *dc, INT32 x, INT32 y);
extern BOOL32 PSDRV_WriteLineTo(DC *dc, INT32 x, INT32 y);
extern BOOL32 PSDRV_WriteStroke(DC *dc);
extern BOOL32 PSDRV_WriteRectangle(DC *dc, INT32 x, INT32 y, INT32 width, 
			INT32 height);
extern BOOL32 PSDRV_WriteSetFont(DC *dc, BOOL32 UseANSI);
extern BOOL32 PSDRV_WriteShow(DC *dc, char *str, INT32 count);
extern BOOL32 PSDRV_WriteReencodeFont(DC *dc);
extern BOOL32 PSDRV_WriteSetPen(DC *dc);
extern BOOL32 PSDRV_WriteArc(DC *dc, INT32 x, INT32 y, INT32 w, INT32 h,
			     double ang1, double ang2);
extern BOOL32 PSDRV_WriteSetColor(DC *dc, PSCOLOR *color);
extern BOOL32 PSDRV_WriteSetBrush(DC *dc);
extern BOOL32 PSDRV_WriteFill(DC *dc);
extern BOOL32 PSDRV_WriteEOFill(DC *dc);
extern BOOL32 PSDRV_WriteGSave(DC *dc);
extern BOOL32 PSDRV_WriteGRestore(DC *dc);
extern BOOL32 PSDRV_WriteClosePath(DC *dc);
extern BOOL32 PSDRV_WriteClip(DC *dc);
extern BOOL32 PSDRV_WriteEOClip(DC *dc);
extern BOOL32 PSDRV_WriteHatch(DC *dc);
extern BOOL32 PSDRV_WriteRotate(DC *dc, float ang);





extern BOOL32 PSDRV_Arc( DC *dc, INT32 left, INT32 top, INT32 right,
			 INT32 bottom, INT32 xstart, INT32 ystart,
			 INT32 xend, INT32 yend );
extern BOOL32 PSDRV_Chord( DC *dc, INT32 left, INT32 top, INT32 right,
			   INT32 bottom, INT32 xstart, INT32 ystart,
			   INT32 xend, INT32 yend );
extern BOOL32 PSDRV_Ellipse( DC *dc, INT32 left, INT32 top, INT32 right,
			     INT32 bottom );
extern BOOL32 PSDRV_EnumDeviceFonts( DC* dc, LPLOGFONT16 plf, 
				     DEVICEFONTENUMPROC proc, LPARAM lp );
extern INT32 PSDRV_Escape( DC *dc, INT32 nEscape, INT32 cbInput, 
			   SEGPTR lpInData, SEGPTR lpOutData );
extern BOOL32 PSDRV_ExtTextOut( DC *dc, INT32 x, INT32 y, UINT32 flags,
				const RECT32 *lprect, LPCSTR str, UINT32 count,
				const INT32 *lpDx );
extern BOOL32 PSDRV_GetCharWidth( DC *dc, UINT32 firstChar, UINT32 lastChar,
				  LPINT32 buffer );
extern BOOL32 PSDRV_GetTextExtentPoint( DC *dc, LPCSTR str, INT32 count,
					LPSIZE32 size );
extern BOOL32 PSDRV_GetTextMetrics( DC *dc, TEXTMETRIC32A *metrics );
extern BOOL32 PSDRV_LineTo( DC *dc, INT32 x, INT32 y );
extern BOOL32 PSDRV_MoveToEx( DC *dc, INT32 x, INT32 y, LPPOINT32 pt );
extern BOOL32 PSDRV_Pie( DC *dc, INT32 left, INT32 top, INT32 right,
			 INT32 bottom, INT32 xstart, INT32 ystart,
			 INT32 xend, INT32 yend );
extern BOOL32 PSDRV_Polygon( DC *dc, LPPOINT32 pt, INT32 count );
extern BOOL32 PSDRV_Polyline( DC *dc, const LPPOINT32 pt, INT32 count );
extern BOOL32 PSDRV_PolyPolygon( DC *dc, LPPOINT32 pts, LPINT32 counts,
				 UINT32 polygons );
extern BOOL32 PSDRV_PolyPolyline( DC *dc, LPPOINT32 pts, LPDWORD counts,
				  DWORD polylines );
extern BOOL32 PSDRV_Rectangle( DC *dc, INT32 left, INT32 top, INT32 right,
			      INT32 bottom );
extern BOOL32 PSDRV_RoundRect(DC *dc, INT32 left, INT32 top, INT32 right,
			      INT32 bottom, INT32 ell_width, INT32 ell_height);
extern HGDIOBJ32 PSDRV_SelectObject( DC *dc, HGDIOBJ32 handle );
extern COLORREF PSDRV_SetBkColor( DC *dc, COLORREF color );
extern COLORREF PSDRV_SetPixel( DC *dc, INT32 x, INT32 y, COLORREF color );
extern COLORREF PSDRV_SetTextColor( DC *dc, COLORREF color );
extern INT32 PSDRV_StretchDIBits( DC *dc, INT32 xDst, INT32 yDst,
				  INT32 widthDst, INT32 heightDst, INT32 xSrc,
				  INT32 ySrc, INT32 widthSrc, INT32 heightSrc,
				  const void *bits, const BITMAPINFO *info,
				  UINT32 wUsage, DWORD dwRop );




