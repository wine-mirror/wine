#ifndef __WINE_WINGDI_H
#define __WINE_WINGDI_H

#include "wintypes.h"

#pragma pack(1)

typedef struct _ABCFLOAT {
    FLOAT   abcfA;
    FLOAT   abcfB;
    FLOAT   abcfC;
} ABCFLOAT, *PABCFLOAT, *LPABCFLOAT;

typedef struct
{
    WORD   wFirst;
    WORD   wSecond;
    INT16  iKernAmount;
} KERNINGPAIR16, *LPKERNINGPAIR16;

typedef struct
{
    WORD   wFirst;
    WORD   wSecond;
    INT  iKernAmount;
} KERNINGPAIR, *LPKERNINGPAIR;


typedef struct
{
    HDC16   hdc;
    BOOL16  fErase;
    RECT16  rcPaint;
    BOOL16  fRestore;
    BOOL16  fIncUpdate;
    BYTE    rgbReserved[16];
} PAINTSTRUCT16, *LPPAINTSTRUCT16;

typedef struct
{
    HDC   hdc;
    BOOL  fErase;
    RECT  rcPaint;
    BOOL  fRestore;
    BOOL  fIncUpdate;
    BYTE    rgbReserved[32];
} PAINTSTRUCT, *LPPAINTSTRUCT;



typedef struct tagPIXELFORMATDESCRIPTOR {
    WORD  nSize;
    WORD  nVersion;
    DWORD dwFlags;
    BYTE  iPixelType;
    BYTE  cColorBits;
    BYTE  cRedBits;
    BYTE  cRedShift;
    BYTE  cGreenBits;
    BYTE  cGreenShift;
    BYTE  cBlueBits;
    BYTE  cBlueShift;
    BYTE  cAlphaBits;
    BYTE  cAlphaShift;
    BYTE  cAccumBits;
    BYTE  cAccumRedBits;
    BYTE  cAccumGreenBits;
    BYTE  cAccumBlueBits;
    BYTE  cAccumAlphaBits;
    BYTE  cDepthBits;
    BYTE  cStencilBits;
    BYTE  cAuxBuffers;
    BYTE  iLayerType;
    BYTE  bReserved;
    DWORD dwLayerMask;
    DWORD dwVisibleMask;
    DWORD dwDamageMask;
} PIXELFORMATDESCRIPTOR, *LPPIXELFORMATDESCRIPTOR;

typedef struct tagCOLORADJUSTMENT
{
	WORD   caSize;
	WORD   caFlags;
	WORD   caIlluminantIndex;
	WORD   caRedGamma;
	WORD   caGreenGamma;
	WORD   caBlueGamma;
	WORD   caReferenceBlack;
	WORD   caReferenceWhite;
	SHORT  caContrast;
	SHORT  caBrightness;
	SHORT  caColorfulness;
	SHORT  caRedGreenTint;
} COLORADJUSTMENT, *PCOLORADJUSTMENT, *LPCOLORADJUSTMENT;

#define DC_FIELDS		1
#define DC_PAPERS		2
#define DC_PAPERSIZE		3
#define DC_MINEXTENT		4
#define DC_MAXEXTENT		5
#define DC_BINS			6
#define DC_DUPLEX		7
#define DC_SIZE			8
#define DC_EXTRA		9
#define DC_VERSION		10
#define DC_DRIVER		11
#define DC_BINNAMES		12
#define DC_ENUMRESOLUTIONS	13
#define DC_FILEDEPENDENCIES	14
#define DC_TRUETYPE		15
#define DC_PAPERNAMES		16
#define DC_ORIENTATION		17
#define DC_COPIES		18

/* Flag returned from Escape QUERYDIBSUPPORT */
#define	QDI_SETDIBITS		1
#define	QDI_GETDIBITS		2
#define	QDI_DIBTOSCREEN		4
#define	QDI_STRETCHDIB		8


#define PR_JOBSTATUS	0x0000


/* GDI Escape commands */
#define	NEWFRAME		1
#define	ABORTDOC		2
#define	NEXTBAND		3
#define	SETCOLORTABLE		4
#define	GETCOLORTABLE		5
#define	FLUSHOUTPUT		6
#define	DRAFTMODE		7
#define	QUERYESCSUPPORT		8
#define	SETABORTPROC		9
#define	STARTDOC		10
#define	ENDDOC			11
#define	GETPHYSPAGESIZE		12
#define	GETPRINTINGOFFSET	13
#define	GETSCALINGFACTOR	14
#define	MFCOMMENT		15
#define	GETPENWIDTH		16
#define	SETCOPYCOUNT		17
#define	SELECTPAPERSOURCE	18
#define	DEVICEDATA		19
#define	PASSTHROUGH		19
#define	GETTECHNOLGY		20
#define	GETTECHNOLOGY		20 /* yes, both of them */
#define	SETLINECAP		21
#define	SETLINEJOIN		22
#define	SETMITERLIMIT		23
#define	BANDINFO		24
#define	DRAWPATTERNRECT		25
#define	GETVECTORPENSIZE	26
#define	GETVECTORBRUSHSIZE	27
#define	ENABLEDUPLEX		28
#define	GETSETPAPERBINS		29
#define	GETSETPRINTORIENT	30
#define	ENUMPAPERBINS		31
#define	SETDIBSCALING		32
#define	EPSPRINTING		33
#define	ENUMPAPERMETRICS	34
#define	GETSETPAPERMETRICS	35
#define	POSTSCRIPT_DATA		37
#define	POSTSCRIPT_IGNORE	38
#define	MOUSETRAILS		39
#define	GETDEVICEUNITS		42

#define	GETEXTENDEDTEXTMETRICS	256
#define	GETEXTENTTABLE		257
#define	GETPAIRKERNTABLE	258
#define	GETTRACKKERNTABLE	259
#define	EXTTEXTOUT		512
#define	GETFACENAME		513
#define	DOWNLOADFACE		514
#define	ENABLERELATIVEWIDTHS	768
#define	ENABLEPAIRKERNING	769
#define	SETKERNTRACK		770
#define	SETALLJUSTVALUES	771
#define	SETCHARSET		772

#define	STRETCHBLT		2048
#define	GETSETSCREENPARAMS	3072
#define	QUERYDIBSUPPORT		3073
#define	BEGIN_PATH		4096
#define	CLIP_TO_PATH		4097
#define	END_PATH		4098
#define	EXT_DEVICE_CAPS		4099
#define	RESTORE_CTM		4100
#define	SAVE_CTM		4101
#define	SET_ARC_DIRECTION	4102
#define	SET_BACKGROUND_COLOR	4103
#define	SET_POLY_MODE		4104
#define	SET_SCREEN_ANGLE	4105
#define	SET_SPREAD		4106
#define	TRANSFORM_CTM		4107
#define	SET_CLIP_BOX		4108
#define	SET_BOUNDS		4109
#define	SET_MIRROR_MODE		4110
#define	OPENCHANNEL		4110
#define	DOWNLOADHEADER		4111
#define CLOSECHANNEL		4112
#define	POSTSCRIPT_PASSTHROUGH	4115
#define	ENCAPSULATED_POSTSCRIPT	4116

/* Spooler Error Codes */
#define	SP_NOTREPORTED	0x4000
#define	SP_ERROR	(-1)
#define	SP_APPABORT	(-2)
#define	SP_USERABORT	(-3)
#define	SP_OUTOFDISK	(-4)
#define	SP_OUTOFMEMORY	(-5)


  /* Raster operations */

#define R2_BLACK         1
#define R2_NOTMERGEPEN   2
#define R2_MASKNOTPEN    3
#define R2_NOTCOPYPEN    4
#define R2_MASKPENNOT    5
#define R2_NOT           6
#define R2_XORPEN        7
#define R2_NOTMASKPEN    8
#define R2_MASKPEN       9
#define R2_NOTXORPEN    10
#define R2_NOP          11
#define R2_MERGENOTPEN  12
#define R2_COPYPEN      13
#define R2_MERGEPENNOT  14
#define R2_MERGEPEN     15
#define R2_WHITE        16

#define SRCCOPY         0xcc0020
#define SRCPAINT        0xee0086
#define SRCAND          0x8800c6
#define SRCINVERT       0x660046
#define SRCERASE        0x440328
#define NOTSRCCOPY      0x330008
#define NOTSRCERASE     0x1100a6
#define MERGECOPY       0xc000ca
#define MERGEPAINT      0xbb0226
#define PATCOPY         0xf00021
#define PATPAINT        0xfb0a09
#define PATINVERT       0x5a0049
#define DSTINVERT       0x550009
#define BLACKNESS       0x000042
#define WHITENESS       0xff0062

  /* StretchBlt() modes */
#define BLACKONWHITE         1
#define WHITEONBLACK         2
#define COLORONCOLOR	     3

#define STRETCH_ANDSCANS     BLACKONWHITE
#define STRETCH_ORSCANS      WHITEONBLACK
#define STRETCH_DELETESCANS  COLORONCOLOR

  /* Colors */

typedef DWORD COLORREF;

#define RGB(r,g,b)          ((COLORREF)((r) | ((g) << 8) | ((b) << 16)))
#define PALETTERGB(r,g,b)   (0x02000000 | RGB(r,g,b))
#define PALETTEINDEX(i)     ((COLORREF)(0x01000000 | (WORD)(i)))

#define GetRValue(rgb)	    ((rgb) & 0xff)
#define GetGValue(rgb)      (((rgb) >> 8) & 0xff)
#define GetBValue(rgb)	    (((rgb) >> 16) & 0xff)

#define COLOR_SCROLLBAR		    0
#define COLOR_BACKGROUND	    1
#define COLOR_ACTIVECAPTION	    2
#define COLOR_INACTIVECAPTION	    3
#define COLOR_MENU		    4
#define COLOR_WINDOW		    5
#define COLOR_WINDOWFRAME	    6
#define COLOR_MENUTEXT		    7
#define COLOR_WINDOWTEXT	    8
#define COLOR_CAPTIONTEXT  	    9
#define COLOR_ACTIVEBORDER	   10
#define COLOR_INACTIVEBORDER	   11
#define COLOR_APPWORKSPACE	   12
#define COLOR_HIGHLIGHT		   13
#define COLOR_HIGHLIGHTTEXT	   14
#define COLOR_BTNFACE              15
#define COLOR_BTNSHADOW            16
#define COLOR_GRAYTEXT             17
#define COLOR_BTNTEXT		   18
#define COLOR_INACTIVECAPTIONTEXT  19
#define COLOR_BTNHIGHLIGHT         20
/* win95 colors */
#define COLOR_3DDKSHADOW           21
#define COLOR_3DLIGHT              22
#define COLOR_INFOTEXT             23
#define COLOR_INFOBK               24
#define COLOR_DESKTOP              COLOR_BACKGROUND
#define COLOR_3DFACE               COLOR_BTNFACE
#define COLOR_3DSHADOW             COLOR_BTNSHADOW
#define COLOR_3DHIGHLIGHT          COLOR_BTNHIGHLIGHT
#define COLOR_3DHILIGHT            COLOR_BTNHIGHLIGHT
#define COLOR_BTNHILIGHT           COLOR_BTNHIGHLIGHT
/* win98 colors */
#define COLOR_ALTERNATEBTNFACE         25  /* undocumented, constant's name unknown */
#define COLOR_HOTLIGHT                 26
#define COLOR_GRADIENTACTIVECAPTION    27
#define COLOR_GRADIENTINACTIVECAPTION  28

  /* WM_CTLCOLOR values */
#define CTLCOLOR_MSGBOX             0
#define CTLCOLOR_EDIT               1
#define CTLCOLOR_LISTBOX            2
#define CTLCOLOR_BTN                3
#define CTLCOLOR_DLG                4
#define CTLCOLOR_SCROLLBAR          5
#define CTLCOLOR_STATIC             6

#define ICM_OFF   1
#define ICM_ON    2
#define ICM_QUERY 3

  /* Bounds Accumulation APIs */
#define DCB_RESET       0x0001
#define DCB_ACCUMULATE  0x0002
#define DCB_DIRTY       DCB_ACCUMULATE
#define DCB_SET         (DCB_RESET | DCB_ACCUMULATE)
#define DCB_ENABLE      0x0004
#define DCB_DISABLE     0x0008

  /* Bitmaps */

typedef struct
{
    INT16  bmType;
    INT16  bmWidth;
    INT16  bmHeight;
    INT16  bmWidthBytes;
    BYTE   bmPlanes;
    BYTE   bmBitsPixel;
    SEGPTR bmBits WINE_PACKED;
} BITMAP16, *LPBITMAP16;

typedef struct
{
    INT  bmType;
    INT  bmWidth;
    INT  bmHeight;
    INT  bmWidthBytes;
    WORD   bmPlanes;
    WORD   bmBitsPixel;
    LPVOID bmBits WINE_PACKED;
} BITMAP, *LPBITMAP;


  /* Brushes */

typedef struct
{ 
    UINT16     lbStyle;
    COLORREF   lbColor WINE_PACKED;
    INT16      lbHatch;
} LOGBRUSH16, *LPLOGBRUSH16;

typedef struct
{ 
    UINT     lbStyle;
    COLORREF   lbColor;
    INT      lbHatch;
} LOGBRUSH, *LPLOGBRUSH;


  /* Brush styles */
#define BS_SOLID	    0
#define BS_NULL		    1
#define BS_HOLLOW	    1
#define BS_HATCHED	    2
#define BS_PATTERN	    3
#define BS_INDEXED	    4
#define	BS_DIBPATTERN	    5
#define	BS_DIBPATTERNPT	    6
#define BS_PATTERN8X8	    7
#define	BS_DIBPATTERN8X8    8
#define BS_MONOPATTERN      9

  /* Hatch styles */
#define HS_HORIZONTAL       0
#define HS_VERTICAL         1
#define HS_FDIAGONAL        2
#define HS_BDIAGONAL        3
#define HS_CROSS            4
#define HS_DIAGCROSS        5

  /* Fonts */

#define LF_FACESIZE     32
#define LF_FULLFACESIZE 64

#define RASTER_FONTTYPE     0x0001
#define DEVICE_FONTTYPE     0x0002
#define TRUETYPE_FONTTYPE   0x0004

typedef struct
{
    INT16  lfHeight;
    INT16  lfWidth;
    INT16  lfEscapement;
    INT16  lfOrientation;
    INT16  lfWeight;
    BYTE   lfItalic;
    BYTE   lfUnderline;
    BYTE   lfStrikeOut;
    BYTE   lfCharSet;
    BYTE   lfOutPrecision;
    BYTE   lfClipPrecision;
    BYTE   lfQuality;
    BYTE   lfPitchAndFamily;
    CHAR   lfFaceName[LF_FACESIZE] WINE_PACKED;
} LOGFONT16, *LPLOGFONT16;

typedef struct
{
    INT  lfHeight;
    INT  lfWidth;
    INT  lfEscapement;
    INT  lfOrientation;
    INT  lfWeight;
    BYTE   lfItalic;
    BYTE   lfUnderline;
    BYTE   lfStrikeOut;
    BYTE   lfCharSet;
    BYTE   lfOutPrecision;
    BYTE   lfClipPrecision;
    BYTE   lfQuality;
    BYTE   lfPitchAndFamily;
    CHAR   lfFaceName[LF_FACESIZE];
} LOGFONTA, *LPLOGFONTA;

typedef struct
{
    INT  lfHeight;
    INT  lfWidth;
    INT  lfEscapement;
    INT  lfOrientation;
    INT  lfWeight;
    BYTE   lfItalic;
    BYTE   lfUnderline;
    BYTE   lfStrikeOut;
    BYTE   lfCharSet;
    BYTE   lfOutPrecision;
    BYTE   lfClipPrecision;
    BYTE   lfQuality;
    BYTE   lfPitchAndFamily;
    WCHAR  lfFaceName[LF_FACESIZE];
} LOGFONTW, *LPLOGFONTW;

DECL_WINELIB_TYPE_AW(LOGFONT)
DECL_WINELIB_TYPE_AW(LPLOGFONT)

typedef struct
{
  LOGFONT16  elfLogFont;
  BYTE       elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  BYTE       elfStyle[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONT16, *LPENUMLOGFONT16;

typedef struct
{
  LOGFONTA elfLogFont;
  BYTE       elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  BYTE       elfStyle[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONTA, *LPENUMLOGFONTA;

typedef struct
{
  LOGFONTW elfLogFont;
  WCHAR      elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  WCHAR      elfStyle[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONTW, *LPENUMLOGFONTW;

typedef struct
{
  LOGFONT16  elfLogFont;
  BYTE       elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  BYTE       elfStyle[LF_FACESIZE] WINE_PACKED;
  BYTE       elfScript[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONTEX16, *LPENUMLOGFONTEX16;

typedef struct
{
  LOGFONTA elfLogFont;
  BYTE       elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  BYTE       elfStyle[LF_FACESIZE] WINE_PACKED;
  BYTE       elfScript[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONTEXA,*LPENUMLOGFONTEXA;

typedef struct
{
  LOGFONTW elfLogFont;
  WCHAR      elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  WCHAR      elfStyle[LF_FACESIZE] WINE_PACKED;
  WCHAR      elfScript[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONTEXW,*LPENUMLOGFONTEXW;

DECL_WINELIB_TYPE_AW(ENUMLOGFONT)
DECL_WINELIB_TYPE_AW(LPENUMLOGFONT)
DECL_WINELIB_TYPE_AW(LPENUMLOGFONTEX)

/*
 * The FONTSIGNATURE tells which Unicode ranges and which code pages
 * have glyphs in a font.
 *
 * fsUsb  128-bit bitmap. The most significant bits are 10 (magic number).
 *        The remaining 126 bits map the Unicode ISO 10646 subranges
 *        for which the font provides glyphs.
 *
 * fsCsb  64-bit bitmap. The low 32 bits map the Windows codepages for
 *        which the font provides glyphs. The high 32 bits are for 
 *        non Windows codepages.
 */
typedef struct
{
  DWORD fsUsb[4];
  DWORD fsCsb[2];
} FONTSIGNATURE,*LPFONTSIGNATURE;

typedef struct 
{
  UINT	ciCharset; /* character set */
  UINT	ciACP; /* ANSI code page */
  FONTSIGNATURE	fs;
} CHARSETINFO,*LPCHARSETINFO;

/* Flags for TranslateCharsetInfo */
#define TCI_SRCCHARSET    1
#define TCI_SRCCODEPAGE   2
#define TCI_SRCFONTSIG    3

/* Flags for ModifyWorldTransform */
#define MWT_IDENTITY      1
#define MWT_LEFTMULTIPLY  2
#define MWT_RIGHTMULTIPLY 3

/* Object Definitions for EnumObjects() */
#define OBJ_PEN             1
#define OBJ_BRUSH           2
#define OBJ_DC              3
#define OBJ_METADC          4
#define OBJ_PAL             5
#define OBJ_FONT            6
#define OBJ_BITMAP          7
#define OBJ_REGION          8
#define OBJ_METAFILE        9
#define OBJ_MEMDC           10
#define OBJ_EXTPEN          11
#define OBJ_ENHMETADC       12
#define OBJ_ENHMETAFILE     13

 
typedef struct
{
    FLOAT  eM11;
    FLOAT  eM12;
    FLOAT  eM21;
    FLOAT  eM22;
    FLOAT  eDx;
    FLOAT  eDy;
} XFORM, *LPXFORM;

typedef struct 
{
    INT16  txfHeight;
    INT16  txfWidth;
    INT16  txfEscapement;
    INT16  txfOrientation;
    INT16  txfWeight;
    CHAR   txfItalic;
    CHAR   txfUnderline;
    CHAR   txfStrikeOut;
    CHAR   txfOutPrecision;
    CHAR   txfClipPrecision;
    INT16  txfAccelerator WINE_PACKED;
    INT16  txfOverhang WINE_PACKED;
} TEXTXFORM16, *LPTEXTXFORM16;

typedef struct
{
    INT16 dfType;
    INT16 dfPoints;
    INT16 dfVertRes;
    INT16 dfHorizRes;
    INT16 dfAscent;
    INT16 dfInternalLeading;
    INT16 dfExternalLeading;
    CHAR  dfItalic;
    CHAR  dfUnderline;
    CHAR  dfStrikeOut;
    INT16 dfWeight;
    BYTE  dfCharSet;
    INT16 dfPixWidth;
    INT16 dfPixHeight;
    CHAR  dfPitchAndFamily;
    INT16 dfAvgWidth;
    INT16 dfMaxWidth;
    CHAR  dfFirstChar;
    CHAR  dfLastChar;
    CHAR  dfDefaultChar;
    CHAR  dfBreakChar;
    INT16 dfWidthBytes;
    LONG  dfDevice;
    LONG  dfFace;
    LONG  dfBitsPointer;
    LONG  dfBitsOffset;
    CHAR  dfReserved;
    LONG  dfFlags;
    INT16 dfAspace;
    INT16 dfBspace;
    INT16 dfCspace;
    LONG  dfColorPointer;
    LONG  dfReserved1[4];
} FONTINFO16, *LPFONTINFO16;

  /* lfWeight values */
#define FW_DONTCARE	    0
#define FW_THIN 	    100
#define FW_EXTRALIGHT	    200
#define FW_ULTRALIGHT	    200
#define FW_LIGHT	    300
#define FW_NORMAL	    400
#define FW_REGULAR	    400
#define FW_MEDIUM	    500
#define FW_SEMIBOLD	    600
#define FW_DEMIBOLD	    600
#define FW_BOLD 	    700
#define FW_EXTRABOLD	    800
#define FW_ULTRABOLD	    800
#define FW_HEAVY	    900
#define FW_BLACK	    900

  /* lfCharSet values */
#define ANSI_CHARSET	      (BYTE)0   /* CP1252, ansi-0, iso8859-{1,15} */
#define DEFAULT_CHARSET       (BYTE)1
#define SYMBOL_CHARSET	      (BYTE)2
#define SHIFTJIS_CHARSET      (BYTE)128 /* CP932 */
#define HANGEUL_CHARSET       (BYTE)129 /* CP949, ksc5601.1987-0 */
#define HANGUL_CHARSET        HANGEUL_CHARSET
#define GB2312_CHARSET        (BYTE)134 /* CP936, gb2312.1980-0 */
#define CHINESEBIG5_CHARSET   (BYTE)136 /* CP950, big5.et-0 */
#define GREEK_CHARSET         (BYTE)161	/* CP1253 */
#define TURKISH_CHARSET       (BYTE)162	/* CP1254, -iso8859-9 */
#define HEBREW_CHARSET        (BYTE)177	/* CP1255, -iso8859-8 */
#define ARABIC_CHARSET        (BYTE)178	/* CP1256, -iso8859-6 */
#define BALTIC_CHARSET        (BYTE)186	/* CP1257, -iso8859-10 */
#define RUSSIAN_CHARSET       (BYTE)204	/* CP1251, -iso8859-5 */
#define EE_CHARSET	      (BYTE)238	/* CP1250, -iso8859-2 */
#define EASTEUROPE_CHARSET    EE_CHARSET
#define THAI_CHARSET	      (BYTE)222 /* CP874, iso8859-11, tis620 */
#define JOHAB_CHARSET         (BYTE)130 /* korean (johab) CP1361 */
#define OEM_CHARSET	      (BYTE)255
/* I don't know if the values of *_CHARSET macros are defined in Windows
 * or if we can choose them as we want. -- srtxg
 */
#define VISCII_CHARSET        (BYTE)240 /* viscii1.1-1 */
#define TCVN_CHARSET          (BYTE)241 /* tcvn-0 */
#define KOI8_CHARSET          (BYTE)242 /* koi8-{r,u,ru} */
#define ISO3_CHARSET          (BYTE)243 /* iso8859-3 */
#define ISO4_CHARSET          (BYTE)244 /* iso8859-4 */

  /* lfOutPrecision values */
#define OUT_DEFAULT_PRECIS	0
#define OUT_STRING_PRECIS	1
#define OUT_CHARACTER_PRECIS	2
#define OUT_STROKE_PRECIS	3
#define OUT_TT_PRECIS		4
#define OUT_DEVICE_PRECIS	5
#define OUT_RASTER_PRECIS	6
#define OUT_TT_ONLY_PRECIS	7

  /* lfClipPrecision values */
#define CLIP_DEFAULT_PRECIS     0x00
#define CLIP_CHARACTER_PRECIS   0x01
#define CLIP_STROKE_PRECIS      0x02
#define CLIP_MASK		0x0F
#define CLIP_LH_ANGLES		0x10
#define CLIP_TT_ALWAYS		0x20
#define CLIP_EMBEDDED		0x80

  /* lfQuality values */
#define DEFAULT_QUALITY     0
#define DRAFT_QUALITY       1
#define PROOF_QUALITY       2

  /* lfPitchAndFamily pitch values */
#define DEFAULT_PITCH       0x00
#define FIXED_PITCH         0x01
#define VARIABLE_PITCH      0x02
#define FF_DONTCARE         0x00
#define FF_ROMAN            0x10
#define FF_SWISS            0x20
#define FF_MODERN           0x30
#define FF_SCRIPT           0x40
#define FF_DECORATIVE       0x50

typedef struct
{
    INT16     tmHeight;
    INT16     tmAscent;
    INT16     tmDescent;
    INT16     tmInternalLeading;
    INT16     tmExternalLeading;
    INT16     tmAveCharWidth;
    INT16     tmMaxCharWidth;
    INT16     tmWeight;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmFirstChar;
    BYTE      tmLastChar;
    BYTE      tmDefaultChar;
    BYTE      tmBreakChar;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
    INT16     tmOverhang WINE_PACKED;
    INT16     tmDigitizedAspectX WINE_PACKED;
    INT16     tmDigitizedAspectY WINE_PACKED;
} TEXTMETRIC16, *LPTEXTMETRIC16;

typedef struct
{
    INT     tmHeight;
    INT     tmAscent;
    INT     tmDescent;
    INT     tmInternalLeading;
    INT     tmExternalLeading;
    INT     tmAveCharWidth;
    INT     tmMaxCharWidth;
    INT     tmWeight;
    INT     tmOverhang;
    INT     tmDigitizedAspectX;
    INT     tmDigitizedAspectY;
    BYTE      tmFirstChar;
    BYTE      tmLastChar;
    BYTE      tmDefaultChar;
    BYTE      tmBreakChar;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
} TEXTMETRICA, *LPTEXTMETRICA;

typedef struct
{
    INT     tmHeight;
    INT     tmAscent;
    INT     tmDescent;
    INT     tmInternalLeading;
    INT     tmExternalLeading;
    INT     tmAveCharWidth;
    INT     tmMaxCharWidth;
    INT     tmWeight;
    INT     tmOverhang;
    INT     tmDigitizedAspectX;
    INT     tmDigitizedAspectY;
    WCHAR     tmFirstChar;
    WCHAR     tmLastChar;
    WCHAR     tmDefaultChar;
    WCHAR     tmBreakChar;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
} TEXTMETRICW, *LPTEXTMETRICW;

DECL_WINELIB_TYPE_AW(TEXTMETRIC)
DECL_WINELIB_TYPE_AW(LPTEXTMETRIC)


typedef struct tagPANOSE
{
    BYTE bFamilyType;
    BYTE bSerifStyle;
    BYTE bWeight;
    BYTE bProportion;
    BYTE bContrast;
    BYTE bStrokeVariation;
    BYTE bArmStyle;
    BYTE bLetterform;
    BYTE bMidline;
    BYTE bXHeight;
} PANOSE;


typedef struct _OUTLINETEXTMETRICA
{
    UINT          otmSize;
    TEXTMETRICA   otmTextMetrics;
    BYTE            otmFilter;
    PANOSE          otmPanoseNumber;
    UINT          otmfsSelection;
    UINT          otmfsType;
    INT           otmsCharSlopeRise;
    INT           otmsCharSlopeRun;
    INT           otmItalicAngle;
    UINT          otmEMSquare;
    INT           otmAscent;
    INT           otmDescent;
    UINT          otmLineGap;
    UINT          otmsCapEmHeight;
    UINT          otmsXHeight;
    RECT          otmrcFontBox;
    INT           otmMacAscent;
    INT           otmMacDescent;
    UINT          otmMacLineGap;
    UINT          otmusMinimumPPEM;
    POINT         otmptSubscriptSize;
    POINT         otmptSubscriptOffset;
    POINT         otmptSuperscriptSize;
    POINT         otmptSuperscriptOffset;
    UINT          otmsStrikeoutSize;
    INT           otmsStrikeoutPosition;
    INT           otmsUnderscoreSize;
    INT           otmsUnderscorePosition;
    LPSTR           otmpFamilyName;
    LPSTR           otmpFaceName;
    LPSTR           otmpStyleName;
    LPSTR           otmpFullName;
} OUTLINETEXTMETRICA, *LPOUTLINETEXTMETRICA;

typedef struct _OUTLINETEXTMETRICW
{
    UINT          otmSize;
    TEXTMETRICW   otmTextMetrics;
    BYTE            otmFilter;
    PANOSE          otmPanoseNumber;
    UINT          otmfsSelection;
    UINT          otmfsType;
    INT           otmsCharSlopeRise;
    INT           otmsCharSlopeRun;
    INT           otmItalicAngle;
    UINT          otmEMSquare;
    INT           otmAscent;
    INT           otmDescent;
    UINT          otmLineGap;
    UINT          otmsCapEmHeight;
    UINT          otmsXHeight;
    RECT          otmrcFontBox;
    INT           otmMacAscent;
    INT           otmMacDescent;
    UINT          otmMacLineGap;
    UINT          otmusMinimumPPEM;
    POINT         otmptSubscriptSize;
    POINT         otmptSubscriptOffset;
    POINT         otmptSuperscriptSize;
    POINT         otmptSuperscriptOffset;
    UINT          otmsStrikeoutSize;
    INT           otmsStrikeoutPosition;
    INT           otmsUnderscoreSize;
    INT           otmsUnderscorePosition;
    LPSTR           otmpFamilyName;
    LPSTR           otmpFaceName;
    LPSTR           otmpStyleName;
    LPSTR           otmpFullName;
} OUTLINETEXTMETRICW, *LPOUTLINETEXTMETRICW;

typedef struct _OUTLINETEXTMETRIC16
{
    UINT16          otmSize;
    TEXTMETRIC16    otmTextMetrics;
    BYTE            otmFilter;
    PANOSE          otmPanoseNumber;
    UINT16          otmfsSelection;
    UINT16          otmfsType;
    INT16           otmsCharSlopeRise;
    INT16           otmsCharSlopeRun;
    INT16           otmItalicAngle;
    UINT16          otmEMSquare;
    INT16           otmAscent;
    INT16           otmDescent;
    UINT16          otmLineGap;
    UINT16          otmsCapEmHeight;
    UINT16          otmsXHeight;
    RECT16          otmrcFontBox;
    INT16           otmMacAscent;
    INT16           otmMacDescent;
    UINT16          otmMacLineGap;
    UINT16          otmusMinimumPPEM;
    POINT16         otmptSubscriptSize;
    POINT16         otmptSubscriptOffset;
    POINT16         otmptSuperscriptSize;
    POINT16         otmptSuperscriptOffset;
    UINT16          otmsStrikeoutSize;
    INT16           otmsStrikeoutPosition;
    INT16           otmsUnderscoreSize;
    INT           otmsUnderscorePosition;
    LPSTR           otmpFamilyName;
    LPSTR           otmpFaceName;
    LPSTR           otmpStyleName;
    LPSTR           otmpFullName;
} OUTLINETEXTMETRIC16,*LPOUTLINETEXTMETRIC16;

DECL_WINELIB_TYPE_AW(OUTLINETEXTMETRIC)
DECL_WINELIB_TYPE_AW(LPOUTLINETEXTMETRIC)



/* ntmFlags field flags */
#define NTM_REGULAR     0x00000040L
#define NTM_BOLD        0x00000020L
#define NTM_ITALIC      0x00000001L

typedef struct
{
    INT16     tmHeight;
    INT16     tmAscent;
    INT16     tmDescent;
    INT16     tmInternalLeading;
    INT16     tmExternalLeading;
    INT16     tmAveCharWidth;
    INT16     tmMaxCharWidth;
    INT16     tmWeight;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmFirstChar;
    BYTE      tmLastChar;
    BYTE      tmDefaultChar;
    BYTE      tmBreakChar;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
    INT16     tmOverhang WINE_PACKED;
    INT16     tmDigitizedAspectX WINE_PACKED;
    INT16     tmDigitizedAspectY WINE_PACKED;
    DWORD     ntmFlags;
    UINT16    ntmSizeEM;
    UINT16    ntmCellHeight;
    UINT16    ntmAvgWidth;
} NEWTEXTMETRIC16,*LPNEWTEXTMETRIC16;

typedef struct
{
    INT     tmHeight;
    INT     tmAscent;
    INT     tmDescent;
    INT     tmInternalLeading;
    INT     tmExternalLeading;
    INT     tmAveCharWidth;
    INT     tmMaxCharWidth;
    INT     tmWeight;
    INT     tmOverhang;
    INT     tmDigitizedAspectX;
    INT     tmDigitizedAspectY;
    BYTE      tmFirstChar;
    BYTE      tmLastChar;
    BYTE      tmDefaultChar;
    BYTE      tmBreakChar;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
    DWORD     ntmFlags;
    UINT    ntmSizeEM;
    UINT    ntmCellHeight;
    UINT    ntmAvgWidth;
} NEWTEXTMETRICA, *LPNEWTEXTMETRICA;

typedef struct
{
    INT     tmHeight;
    INT     tmAscent;
    INT     tmDescent;
    INT     tmInternalLeading;
    INT     tmExternalLeading;
    INT     tmAveCharWidth;
    INT     tmMaxCharWidth;
    INT     tmWeight;
    INT     tmOverhang;
    INT     tmDigitizedAspectX;
    INT     tmDigitizedAspectY;
    WCHAR     tmFirstChar;
    WCHAR     tmLastChar;
    WCHAR     tmDefaultChar;
    WCHAR     tmBreakChar;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
    DWORD     ntmFlags;
    UINT    ntmSizeEM;
    UINT    ntmCellHeight;
    UINT    ntmAvgWidth;
} NEWTEXTMETRICW, *LPNEWTEXTMETRICW;

DECL_WINELIB_TYPE_AW(NEWTEXTMETRIC)
DECL_WINELIB_TYPE_AW(LPNEWTEXTMETRIC)

typedef struct
{
    NEWTEXTMETRIC16	ntmetm;
    FONTSIGNATURE       ntmeFontSignature;
} NEWTEXTMETRICEX16,*LPNEWTEXTMETRICEX16;

typedef struct
{
    NEWTEXTMETRICA	ntmetm;
    FONTSIGNATURE       ntmeFontSignature;
} NEWTEXTMETRICEXA,*LPNEWTEXTMETRICEXA;

typedef struct
{
    NEWTEXTMETRICW	ntmetm;
    FONTSIGNATURE       ntmeFontSignature;
} NEWTEXTMETRICEXW,*LPNEWTEXTMETRICEXW;

DECL_WINELIB_TYPE_AW(NEWTEXTMETRICEX)
DECL_WINELIB_TYPE_AW(LPNEWTEXTMETRICEX)


typedef INT16 (CALLBACK *FONTENUMPROC16)(SEGPTR,SEGPTR,UINT16,LPARAM);
typedef INT (CALLBACK *FONTENUMPROCA)(LPENUMLOGFONTA,LPNEWTEXTMETRICA,
                                          UINT,LPARAM);
typedef INT (CALLBACK *FONTENUMPROCW)(LPENUMLOGFONTW,LPNEWTEXTMETRICW,
                                          UINT,LPARAM);
DECL_WINELIB_TYPE_AW(FONTENUMPROC)

typedef INT16 (CALLBACK *FONTENUMPROCEX16)(SEGPTR,SEGPTR,UINT16,LPARAM);
typedef INT (CALLBACK *FONTENUMPROCEXA)(LPENUMLOGFONTEXA,LPNEWTEXTMETRICEXA,UINT,LPARAM);
typedef INT (CALLBACK *FONTENUMPROCEXW)(LPENUMLOGFONTEXW,LPNEWTEXTMETRICEXW,UINT,LPARAM);
DECL_WINELIB_TYPE_AW(FONTENUMPROCEX)

  /* tmPitchAndFamily bits */
#define TMPF_FIXED_PITCH    1		/* means variable pitch */
#define TMPF_VECTOR	    2
#define TMPF_TRUETYPE	    4
#define TMPF_DEVICE	    8

  /* Text alignment */
#define TA_NOUPDATECP       0x00
#define TA_UPDATECP         0x01
#define TA_LEFT             0x00
#define TA_RIGHT            0x02
#define TA_CENTER           0x06
#define TA_TOP              0x00
#define TA_BOTTOM           0x08
#define TA_BASELINE         0x18

  /* ExtTextOut() parameters */
#define ETO_GRAYED          0x01
#define ETO_OPAQUE          0x02
#define ETO_CLIPPED         0x04

typedef struct
{
    UINT16	gmBlackBoxX;
    UINT16	gmBlackBoxY;
    POINT16	gmptGlyphOrigin;
    INT16	gmCellIncX;
    INT16	gmCellIncY;
} GLYPHMETRICS16, *LPGLYPHMETRICS16;

typedef struct
{
    UINT	gmBlackBoxX;
    UINT	gmBlackBoxY;
    POINT	gmptGlyphOrigin;
    INT16	gmCellIncX;
    INT16	gmCellIncY;
} GLYPHMETRICS, *LPGLYPHMETRICS;


#define GGO_METRICS         0
#define GGO_BITMAP          1
#define GGO_NATIVE          2

typedef struct
{
    UINT16  fract;
    INT16   value;
} FIXED;

typedef struct
{
     FIXED  eM11;
     FIXED  eM12;
     FIXED  eM21;
     FIXED  eM22;
} MAT2, *LPMAT2;

  /* for GetCharABCWidths() */
typedef struct
{
    INT16   abcA;
    UINT16  abcB;
    INT16   abcC;
} ABC16, *LPABC16;

typedef struct
{
    INT   abcA;
    UINT  abcB;
    INT   abcC;
} ABC, *LPABC;


  /* for GetCharacterPlacement () */
typedef struct tagGCP_RESULTSA
{
    DWORD  lStructSize;
    LPSTR  lpOutString;
    UINT *lpOrder;
    INT  *lpDx;
    INT  *lpCaretPos;
    LPSTR  lpClass;
    UINT *lpGlyphs;
    UINT nGlyphs;
    UINT nMaxFit;
} GCP_RESULTSA;

typedef struct tagGCP_RESULTSW
{
    DWORD  lStructSize;
    LPWSTR lpOutString;
    UINT *lpOrder;
    INT  *lpDx;
    INT  *lpCaretPos;
    LPWSTR lpClass;
    UINT *lpGlyphs;
    UINT nGlyphs;
    UINT nMaxFit;
} GCP_RESULTSW;

DECL_WINELIB_TYPE_AW(GCP_RESULTS)

  /* Rasterizer status */
typedef struct
{
    INT16 nSize;
    INT16 wFlags;
    INT16 nLanguageID;
} RASTERIZER_STATUS, *LPRASTERIZER_STATUS;

#define TT_AVAILABLE        0x0001
#define TT_ENABLED          0x0002

/* Get/SetSystemPaletteUse() values */
#define SYSPAL_STATIC   1
#define SYSPAL_NOSTATIC 2

typedef struct tagPALETTEENTRY
{
	BYTE peRed, peGreen, peBlue, peFlags;
} PALETTEENTRY, *LPPALETTEENTRY;

/* Logical palette entry flags */
#define PC_RESERVED     0x01
#define PC_EXPLICIT     0x02
#define PC_NOCOLLAPSE   0x04

typedef struct
{ 
    WORD           palVersion;
    WORD           palNumEntries;
    PALETTEENTRY   palPalEntry[1] WINE_PACKED;
} LOGPALETTE, *LPLOGPALETTE;

  /* Pens */

typedef struct
{
    UINT16   lopnStyle; 
    POINT16  lopnWidth WINE_PACKED;
    COLORREF lopnColor WINE_PACKED;
} LOGPEN16, *LPLOGPEN16;

typedef struct
{
    UINT   lopnStyle; 
    POINT  lopnWidth WINE_PACKED;
    COLORREF lopnColor WINE_PACKED;
} LOGPEN, *LPLOGPEN;


typedef struct tagEXTLOGPEN
{
	DWORD elpPenStyle;
	DWORD elpWidth;
	DWORD elpBrushStyle;
	DWORD elpColor;
	DWORD elpNumEntries;
	DWORD elpStyleEntry[1];
} EXTLOGPEN, *PEXTLOGPEN, *NPEXTLOGPEN, *LPEXTLOGPEN;

#define PS_SOLID         0x00000000
#define PS_DASH          0x00000001
#define PS_DOT           0x00000002
#define PS_DASHDOT       0x00000003
#define PS_DASHDOTDOT    0x00000004
#define PS_NULL          0x00000005
#define PS_INSIDEFRAME   0x00000006
#define PS_USERSTYLE     0x00000007
#define PS_ALTERNATE     0x00000008
#define PS_STYLE_MASK    0x0000000f

#define PS_ENDCAP_ROUND  0x00000000
#define PS_ENDCAP_SQUARE 0x00000100
#define PS_ENDCAP_FLAT   0x00000200
#define PS_ENDCAP_MASK   0x00000f00

#define PS_JOIN_ROUND    0x00000000
#define PS_JOIN_BEVEL    0x00001000
#define PS_JOIN_MITER    0x00002000
#define PS_JOIN_MASK     0x0000f000

#define PS_COSMETIC      0x00000000
#define PS_GEOMETRIC     0x00010000
#define PS_TYPE_MASK     0x000f0000

  /* Regions */

#define ERROR             0
#define NULLREGION        1
#define SIMPLEREGION      2
#define COMPLEXREGION     3

#define RGN_AND           1
#define RGN_OR            2
#define RGN_XOR           3
#define RGN_DIFF          4
#define RGN_COPY          5

  /* Device contexts */

/* GetDCEx flags */
#define DCX_WINDOW           0x00000001
#define DCX_CACHE            0x00000002
#define DCX_CLIPCHILDREN     0x00000008
#define DCX_CLIPSIBLINGS     0x00000010
#define DCX_PARENTCLIP       0x00000020
#define DCX_EXCLUDERGN       0x00000040
#define DCX_INTERSECTRGN     0x00000080
#define DCX_LOCKWINDOWUPDATE 0x00000400
#define DCX_USESTYLE         0x00010000

  /* Polygon modes */
#define ALTERNATE         1
#define WINDING           2

  /* Background modes */
#ifdef TRANSPARENT  /*Apparently some broken svr4 includes define TRANSPARENT*/
#undef TRANSPARENT
#endif
#define TRANSPARENT       1
#define OPAQUE            2


  /* Graphics Modes */
#define GM_COMPATIBLE     1
#define GM_ADVANCED       2
#define GM_LAST           2

  /* Arc direction modes */
#define AD_COUNTERCLOCKWISE 1
#define AD_CLOCKWISE        2

  /* Map modes */
#define MM_TEXT		  1
#define MM_LOMETRIC	  2
#define MM_HIMETRIC	  3
#define MM_LOENGLISH	  4
#define MM_HIENGLISH	  5
#define MM_TWIPS	  6
#define MM_ISOTROPIC	  7
#define MM_ANISOTROPIC	  8

  /* Coordinate modes */
#define ABSOLUTE          1
#define RELATIVE          2

  /* Flood fill modes */
#define FLOODFILLBORDER   0
#define FLOODFILLSURFACE  1

  /* Device parameters for GetDeviceCaps() */
#define DRIVERVERSION     0
#define TECHNOLOGY        2
#define HORZSIZE          4
#define VERTSIZE          6
#define HORZRES           8
#define VERTRES           10
#define BITSPIXEL         12
#define PLANES            14
#define NUMBRUSHES        16
#define NUMPENS           18
#define NUMMARKERS        20
#define NUMFONTS          22
#define NUMCOLORS         24
#define PDEVICESIZE       26
#define CURVECAPS         28
#define LINECAPS          30
#define POLYGONALCAPS     32
#define TEXTCAPS          34
#define CLIPCAPS          36
#define RASTERCAPS        38
#define ASPECTX           40
#define ASPECTY           42
#define ASPECTXY          44
#define LOGPIXELSX        88
#define LOGPIXELSY        90
#define SIZEPALETTE       104
#define NUMRESERVED       106
#define COLORRES          108

/* TECHNOLOGY */
#define DT_PLOTTER        0
#define DT_RASDISPLAY     1
#define DT_RASPRINTER     2
#define DT_RASCAMERA      3
#define DT_CHARSTREAM     4
#define DT_METAFILE       5
#define DT_DISPFILE       6

/* CURVECAPS */
#define CC_NONE           0x0000
#define CC_CIRCLES        0x0001
#define CC_PIE            0x0002
#define CC_CHORD          0x0004
#define CC_ELLIPSES       0x0008
#define CC_WIDE           0x0010
#define CC_STYLED         0x0020
#define CC_WIDESTYLED     0x0040
#define CC_INTERIORS      0x0080
#define CC_ROUNDRECT      0x0100

/* LINECAPS */
#define LC_NONE           0x0000
#define LC_POLYLINE       0x0002
#define LC_MARKER         0x0004
#define LC_POLYMARKER     0x0008
#define LC_WIDE           0x0010
#define LC_STYLED         0x0020
#define LC_WIDESTYLED     0x0040
#define LC_INTERIORS      0x0080

/* POLYGONALCAPS */
#define PC_NONE           0x0000
#define PC_POLYGON        0x0001
#define PC_RECTANGLE      0x0002
#define PC_WINDPOLYGON    0x0004
#define PC_SCANLINE       0x0008
#define PC_WIDE           0x0010
#define PC_STYLED         0x0020
#define PC_WIDESTYLED     0x0040
#define PC_INTERIORS      0x0080

/* TEXTCAPS */
#define TC_OP_CHARACTER   0x0001
#define TC_OP_STROKE      0x0002
#define TC_CP_STROKE      0x0004
#define TC_CR_90          0x0008
#define TC_CR_ANY         0x0010
#define TC_SF_X_YINDEP    0x0020
#define TC_SA_DOUBLE      0x0040
#define TC_SA_INTEGER     0x0080
#define TC_SA_CONTIN      0x0100
#define TC_EA_DOUBLE      0x0200
#define TC_IA_ABLE        0x0400
#define TC_UA_ABLE        0x0800
#define TC_SO_ABLE        0x1000
#define TC_RA_ABLE        0x2000
#define TC_VA_ABLE        0x4000
#define TC_RESERVED       0x8000

/* CLIPCAPS */
#define CP_NONE           0x0000
#define CP_RECTANGLE      0x0001
#define CP_REGION         0x0002

/* RASTERCAPS */
#define RC_NONE           0x0000
#define RC_BITBLT         0x0001
#define RC_BANDING        0x0002
#define RC_SCALING        0x0004
#define RC_BITMAP64       0x0008
#define RC_GDI20_OUTPUT   0x0010
#define RC_GDI20_STATE    0x0020
#define RC_SAVEBITMAP     0x0040
#define RC_DI_BITMAP      0x0080
#define RC_PALETTE        0x0100
#define RC_DIBTODEV       0x0200
#define RC_BIGFONT        0x0400
#define RC_STRETCHBLT     0x0800
#define RC_FLOODFILL      0x1000
#define RC_STRETCHDIB     0x2000
#define RC_OP_DX_OUTPUT   0x4000
#define RC_DEVBITS        0x8000

  /* GetSystemMetrics() codes */
#define SM_CXSCREEN	       0
#define SM_CYSCREEN            1
#define SM_CXVSCROLL           2
#define SM_CYHSCROLL	       3
#define SM_CYCAPTION	       4
#define SM_CXBORDER	       5
#define SM_CYBORDER	       6
#define SM_CXDLGFRAME	       7
#define SM_CYDLGFRAME	       8
#define SM_CYVTHUMB	       9
#define SM_CXHTHUMB	      10
#define SM_CXICON	      11
#define SM_CYICON	      12
#define SM_CXCURSOR	      13
#define SM_CYCURSOR	      14
#define SM_CYMENU	      15
#define SM_CXFULLSCREEN       16
#define SM_CYFULLSCREEN       17
#define SM_CYKANJIWINDOW      18
#define SM_MOUSEPRESENT       19
#define SM_CYVSCROLL	      20
#define SM_CXHSCROLL	      21
#define SM_DEBUG	      22
#define SM_SWAPBUTTON	      23
#define SM_RESERVED1	      24
#define SM_RESERVED2	      25
#define SM_RESERVED3	      26
#define SM_RESERVED4	      27
#define SM_CXMIN	      28
#define SM_CYMIN	      29
#define SM_CXSIZE	      30
#define SM_CYSIZE	      31
#define SM_CXFRAME	      32
#define SM_CYFRAME	      33
#define SM_CXMINTRACK	      34
#define SM_CYMINTRACK	      35
#define SM_CXDOUBLECLK        36
#define SM_CYDOUBLECLK        37
#define SM_CXICONSPACING      38
#define SM_CYICONSPACING      39
#define SM_MENUDROPALIGNMENT  40
#define SM_PENWINDOWS         41
#define SM_DBCSENABLED        42
#define SM_CMOUSEBUTTONS      43
#define SM_CXFIXEDFRAME	      SM_CXDLGFRAME
#define SM_CYFIXEDFRAME	      SM_CYDLGFRAME
#define SM_CXSIZEFRAME	      SM_CXFRAME
#define SM_CYSIZEFRAME	      SM_CYFRAME
#define SM_SECURE	      44
#define SM_CXEDGE	      45
#define SM_CYEDGE	      46
#define SM_CXMINSPACING	      47
#define SM_CYMINSPACING	      48
#define SM_CXSMICON	      49
#define SM_CYSMICON	      50
#define SM_CYSMCAPTION	      51
#define SM_CXSMSIZE	      52
#define SM_CYSMSIZE	      53
#define SM_CXMENUSIZE	      54
#define SM_CYMENUSIZE	      55
#define SM_ARRANGE	      56
#define SM_CXMINIMIZED	      57
#define SM_CYMINIMIZED	      58
#define SM_CXMAXTRACK	      59
#define SM_CYMAXTRACK	      60
#define SM_CXMAXIMIZED	      61
#define SM_CYMAXIMIZED	      62
#define SM_NETWORK	      63
#define SM_CLEANBOOT	      67
#define SM_CXDRAG	      68
#define SM_CYDRAG	      69
#define SM_SHOWSOUNDS	      70
#define SM_CXMENUCHECK	      71
#define SM_CYMENUCHECK	      72
#define SM_SLOWMACHINE	      73
#define SM_MIDEASTENABLED     74
#define SM_MOUSEWHEELPRESENT  75
#define SM_XVIRTUALSCREEN     76
#define SM_YVIRTUALSCREEN     77
#define SM_CXVIRTUALSCREEN    78
#define SM_CYVIRTUALSCREEN    79
#define SM_CMONITORS          80
#define SM_SAMEDISPLAYFORMAT  81
#define SM_CMETRICS           83


  /* Device-independent bitmaps */

typedef struct { BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved; } RGBQUAD;
typedef struct { BYTE rgbtBlue, rgbtGreen, rgbtRed; } RGBTRIPLE;

typedef struct
{
    UINT16  bfType;
    DWORD   bfSize WINE_PACKED;
    UINT16  bfReserved1 WINE_PACKED;
    UINT16  bfReserved2 WINE_PACKED;
    DWORD   bfOffBits WINE_PACKED;
} BITMAPFILEHEADER;

typedef struct
{
    DWORD 	biSize;
    LONG  	biWidth;
    LONG  	biHeight;
    WORD 	biPlanes;
    WORD 	biBitCount;
    DWORD 	biCompression;
    DWORD 	biSizeImage;
    LONG  	biXPelsPerMeter;
    LONG  	biYPelsPerMeter;
    DWORD 	biClrUsed;
    DWORD 	biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER;

  /* biCompression */
#define BI_RGB           0
#define BI_RLE8          1
#define BI_RLE4          2

typedef struct {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD	bmiColors[1];
} BITMAPINFO;
typedef BITMAPINFO *LPBITMAPINFO;
typedef BITMAPINFO *NPBITMAPINFO;
typedef BITMAPINFO *PBITMAPINFO;

typedef struct
{
    DWORD   bcSize;
    UINT16  bcWidth;
    UINT16  bcHeight;
    UINT16  bcPlanes;
    UINT16  bcBitCount;
} BITMAPCOREHEADER;

typedef struct
{
    BITMAPCOREHEADER bmciHeader;
    RGBTRIPLE        bmciColors[1];
} BITMAPCOREINFO, *LPBITMAPCOREINFO;

#define DIB_RGB_COLORS   0
#define DIB_PAL_COLORS   1
#define CBM_INIT         4

typedef struct 
{
	BITMAP		dsBm;
	BITMAPINFOHEADER	dsBmih;
	DWORD			dsBitfields[3];
	HANDLE		dshSection;
	DWORD			dsOffset;
} DIBSECTION,*LPDIBSECTION;

  /* Stock GDI objects for GetStockObject() */

#define WHITE_BRUSH	    0
#define LTGRAY_BRUSH	    1
#define GRAY_BRUSH	    2
#define DKGRAY_BRUSH	    3
#define BLACK_BRUSH	    4
#define NULL_BRUSH	    5
#define HOLLOW_BRUSH	    5
#define WHITE_PEN	    6
#define BLACK_PEN	    7
#define NULL_PEN	    8
#define OEM_FIXED_FONT	    10
#define ANSI_FIXED_FONT     11
#define ANSI_VAR_FONT	    12
#define SYSTEM_FONT	    13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE     15
#define SYSTEM_FIXED_FONT   16
#define DEFAULT_GUI_FONT    17

#define STOCK_LAST          17

/* Metafile header structure */
typedef struct
{
    WORD       mtType;
    WORD       mtHeaderSize;
    WORD       mtVersion;
    DWORD      mtSize WINE_PACKED;
    WORD       mtNoObjects;
    DWORD      mtMaxRecord WINE_PACKED;
    WORD       mtNoParameters;
} METAHEADER;

/* Metafile typical record structure */
typedef struct
{
    DWORD      rdSize;
    WORD       rdFunction;
    WORD       rdParm[1];
} METARECORD;
typedef METARECORD *PMETARECORD;
typedef METARECORD *LPMETARECORD;

/* Handle table structure */

typedef struct
{
    HGDIOBJ16 objectHandle[1];
} HANDLETABLE16, *LPHANDLETABLE16;

typedef struct
{
    HGDIOBJ objectHandle[1];
} HANDLETABLE, *LPHANDLETABLE;


/* Clipboard metafile picture structure */
typedef struct
{
    INT16        mm;
    INT16        xExt;
    INT16        yExt;
    HMETAFILE16  hMF;
} METAFILEPICT16, *LPMETAFILEPICT16;

typedef struct
{
    INT        mm;
    INT        xExt;
    INT        yExt;
    HMETAFILE  hMF;
} METAFILEPICT, *LPMETAFILEPICT;


/* Metafile functions */
#define META_SETBKCOLOR              0x0201
#define META_SETBKMODE               0x0102
#define META_SETMAPMODE              0x0103
#define META_SETROP2                 0x0104
#define META_SETRELABS               0x0105
#define META_SETPOLYFILLMODE         0x0106
#define META_SETSTRETCHBLTMODE       0x0107
#define META_SETTEXTCHAREXTRA        0x0108
#define META_SETTEXTCOLOR            0x0209
#define META_SETTEXTJUSTIFICATION    0x020A
#define META_SETWINDOWORG            0x020B
#define META_SETWINDOWEXT            0x020C
#define META_SETVIEWPORTORG          0x020D
#define META_SETVIEWPORTEXT          0x020E
#define META_OFFSETWINDOWORG         0x020F
#define META_SCALEWINDOWEXT          0x0410
#define META_OFFSETVIEWPORTORG       0x0211
#define META_SCALEVIEWPORTEXT        0x0412
#define META_LINETO                  0x0213
#define META_MOVETO                  0x0214
#define META_EXCLUDECLIPRECT         0x0415
#define META_INTERSECTCLIPRECT       0x0416
#define META_ARC                     0x0817
#define META_ELLIPSE                 0x0418
#define META_FLOODFILL               0x0419
#define META_PIE                     0x081A
#define META_RECTANGLE               0x041B
#define META_ROUNDRECT               0x061C
#define META_PATBLT                  0x061D
#define META_SAVEDC                  0x001E
#define META_SETPIXEL                0x041F
#define META_OFFSETCLIPRGN           0x0220
#define META_TEXTOUT                 0x0521
#define META_BITBLT                  0x0922
#define META_STRETCHBLT              0x0B23
#define META_POLYGON                 0x0324
#define META_POLYLINE                0x0325
#define META_ESCAPE                  0x0626
#define META_RESTOREDC               0x0127
#define META_FILLREGION              0x0228
#define META_FRAMEREGION             0x0429
#define META_INVERTREGION            0x012A
#define META_PAINTREGION             0x012B
#define META_SELECTCLIPREGION        0x012C
#define META_SELECTOBJECT            0x012D
#define META_SETTEXTALIGN            0x012E
#define META_DRAWTEXT                0x062F
#define META_CHORD                   0x0830
#define META_SETMAPPERFLAGS          0x0231
#define META_EXTTEXTOUT              0x0A32
#define META_SETDIBTODEV             0x0D33
#define META_SELECTPALETTE           0x0234
#define META_REALIZEPALETTE          0x0035
#define META_ANIMATEPALETTE          0x0436
#define META_SETPALENTRIES           0x0037
#define META_POLYPOLYGON             0x0538
#define META_RESIZEPALETTE           0x0139
#define META_DIBBITBLT               0x0940
#define META_DIBSTRETCHBLT           0x0B41
#define META_DIBCREATEPATTERNBRUSH   0x0142
#define META_STRETCHDIB              0x0F43
#define META_EXTFLOODFILL            0x0548
#define META_RESETDC                 0x014C
#define META_STARTDOC                0x014D
#define META_STARTPAGE               0x004F
#define META_ENDPAGE                 0x0050
#define META_ABORTDOC                0x0052
#define META_ENDDOC                  0x005E
#define META_DELETEOBJECT            0x01F0
#define META_CREATEPALETTE           0x00F7
#define META_CREATEBRUSH             0x00F8
#define META_CREATEPATTERNBRUSH      0x01F9
#define META_CREATEPENINDIRECT       0x02FA
#define META_CREATEFONTINDIRECT      0x02FB
#define META_CREATEBRUSHINDIRECT     0x02FC
#define META_CREATEBITMAPINDIRECT    0x02FD
#define META_CREATEBITMAP            0x06FE
#define META_CREATEREGION            0x06FF
#define META_UNKNOWN                 0x0529  /* FIXME: unknown meta magic */

typedef INT16 (CALLBACK *MFENUMPROC16)(HDC16,HANDLETABLE16*,METARECORD*,
                                       INT16,LPARAM);
typedef INT (CALLBACK *MFENUMPROC)(HDC,HANDLETABLE*,METARECORD*,
                                       INT,LPARAM);

/* enhanced metafile structures and functions */

/* note that ENHMETAHEADER is just a particular kind of ENHMETARECORD,
   ie. the header is just the first record in the metafile */
typedef struct {
    DWORD iType; 
    DWORD nSize; 
    RECT rclBounds; 
    RECT rclFrame; 
    DWORD dSignature; 
    DWORD nVersion; 
    DWORD nBytes; 
    DWORD nRecords; 
    WORD  nHandles; 
    WORD  sReserved; 
    DWORD nDescription; 
    DWORD offDescription; 
    DWORD nPalEntries; 
    SIZE szlDevice; 
    SIZE szlMillimeters;
    DWORD cbPixelFormat;
    DWORD offPixelFormat;
    DWORD bOpenGL;
} ENHMETAHEADER, *LPENHMETAHEADER; 

typedef struct {
    DWORD iType; 
    DWORD nSize; 
    DWORD dParm[1]; 
} ENHMETARECORD, *LPENHMETARECORD; 

typedef INT (CALLBACK *ENHMFENUMPROC)(HDC, LPHANDLETABLE, 
					  LPENHMETARECORD, INT, LPVOID);

#define EMR_HEADER	1
#define EMR_POLYBEZIER	2
#define EMR_POLYGON	3
#define EMR_POLYLINE	4
#define EMR_POLYBEZIERTO	5
#define EMR_POLYLINETO	6
#define EMR_POLYPOLYLINE	7
#define EMR_POLYPOLYGON	8
#define EMR_SETWINDOWEXTEX	9
#define EMR_SETWINDOWORGEX	10
#define EMR_SETVIEWPORTEXTEX	11
#define EMR_SETVIEWPORTORGEX	12
#define EMR_SETBRUSHORGEX	13
#define EMR_EOF	14
#define EMR_SETPIXELV	15
#define EMR_SETMAPPERFLAGS	16
#define EMR_SETMAPMODE	17
#define EMR_SETBKMODE	18
#define EMR_SETPOLYFILLMODE	19
#define EMR_SETROP2	20
#define EMR_SETSTRETCHBLTMODE	21
#define EMR_SETTEXTALIGN	22
#define EMR_SETCOLORADJUSTMENT	23
#define EMR_SETTEXTCOLOR	24
#define EMR_SETBKCOLOR	25
#define EMR_OFFSETCLIPRGN	26
#define EMR_MOVETOEX	27
#define EMR_SETMETARGN	28
#define EMR_EXCLUDECLIPRECT	29
#define EMR_INTERSECTCLIPRECT	30
#define EMR_SCALEVIEWPORTEXTEX	31
#define EMR_SCALEWINDOWEXTEX	32
#define EMR_SAVEDC	33
#define EMR_RESTOREDC	34
#define EMR_SETWORLDTRANSFORM	35
#define EMR_MODIFYWORLDTRANSFORM	36
#define EMR_SELECTOBJECT	37
#define EMR_CREATEPEN	38
#define EMR_CREATEBRUSHINDIRECT	39
#define EMR_DELETEOBJECT	40
#define EMR_ANGLEARC	41
#define EMR_ELLIPSE	42
#define EMR_RECTANGLE	43
#define EMR_ROUNDRECT	44
#define EMR_ARC	45
#define EMR_CHORD	46
#define EMR_PIE	47
#define EMR_SELECTPALETTE	48
#define EMR_CREATEPALETTE	49
#define EMR_SETPALETTEENTRIES	50
#define EMR_RESIZEPALETTE	51
#define EMR_REALIZEPALETTE	52
#define EMR_EXTFLOODFILL	53
#define EMR_LINETO	54
#define EMR_ARCTO	55
#define EMR_POLYDRAW	56
#define EMR_SETARCDIRECTION	57
#define EMR_SETMITERLIMIT	58
#define EMR_BEGINPATH	59
#define EMR_ENDPATH	60
#define EMR_CLOSEFIGURE	61
#define EMR_FILLPATH	62
#define EMR_STROKEANDFILLPATH	63
#define EMR_STROKEPATH	64
#define EMR_FLATTENPATH	65
#define EMR_WIDENPATH	66
#define EMR_SELECTCLIPPATH	67
#define EMR_ABORTPATH	68
#define EMR_GDICOMMENT	70
#define EMR_FILLRGN	71
#define EMR_FRAMERGN	72
#define EMR_INVERTRGN	73
#define EMR_PAINTRGN	74
#define EMR_EXTSELECTCLIPRGN	75
#define EMR_BITBLT	76
#define EMR_STRETCHBLT	77
#define EMR_MASKBLT	78
#define EMR_PLGBLT	79
#define EMR_SETDIBITSTODEVICE	80
#define EMR_STRETCHDIBITS	81
#define EMR_EXTCREATEFONTINDIRECTW	82
#define EMR_EXTTEXTOUTA	83
#define EMR_EXTTEXTOUTW	84
#define EMR_POLYBEZIER16	85
#define EMR_POLYGON16	86
#define EMR_POLYLINE16	87
#define EMR_POLYBEZIERTO16	88
#define EMR_POLYLINETO16	89
#define EMR_POLYPOLYLINE16	90
#define EMR_POLYPOLYGON16	91
#define EMR_POLYDRAW16	92
#define EMR_CREATEMONOBRUSH	93
#define EMR_CREATEDIBPATTERNBRUSHPT	94
#define EMR_EXTCREATEPEN	95
#define EMR_POLYTEXTOUTA	96
#define EMR_POLYTEXTOUTW	97
#define EMR_SETICMMODE	98
#define EMR_CREATECOLORSPACE	99
#define EMR_SETCOLORSPACE	100
#define EMR_DELETECOLORSPACE	101
#define EMR_GLSRECORD	102
#define EMR_GLSBOUNDEDRECORD	103
#define EMR_PIXELFORMAT 104

#define ENHMETA_SIGNATURE	1179469088

#define CCHDEVICENAME 32
#define CCHFORMNAME   32

typedef struct
{
    BYTE   dmDeviceName[CCHDEVICENAME];
    WORD   dmSpecVersion;
    WORD   dmDriverVersion;
    WORD   dmSize;
    WORD   dmDriverExtra;
    DWORD  dmFields;
    INT16  dmOrientation;
    INT16  dmPaperSize;
    INT16  dmPaperLength;
    INT16  dmPaperWidth;
    INT16  dmScale;
    INT16  dmCopies;
    INT16  dmDefaultSource;
    INT16  dmPrintQuality;
    INT16  dmColor;
    INT16  dmDuplex;
    INT16  dmYResolution;
    INT16  dmTTOption;
    INT16  dmCollate;
    BYTE   dmFormName[CCHFORMNAME];
    WORD   dmUnusedPadding;
    WORD   dmBitsPerPel;
    DWORD  dmPelsWidth;
    DWORD  dmPelsHeight;
    DWORD  dmDisplayFlags;
    DWORD  dmDisplayFrequency;
} DEVMODE16, *LPDEVMODE16;

typedef struct
{
    BYTE   dmDeviceName[CCHDEVICENAME];
    WORD   dmSpecVersion;
    WORD   dmDriverVersion;
    WORD   dmSize;
    WORD   dmDriverExtra;
    DWORD  dmFields;
    INT16  dmOrientation;
    INT16  dmPaperSize;
    INT16  dmPaperLength;
    INT16  dmPaperWidth;
    INT16  dmScale;
    INT16  dmCopies;
    INT16  dmDefaultSource;
    INT16  dmPrintQuality;
    INT16  dmColor;
    INT16  dmDuplex;
    INT16  dmYResolution;
    INT16  dmTTOption;
    INT16  dmCollate;
    BYTE   dmFormName[CCHFORMNAME];
    WORD   dmLogPixels;
    DWORD  dmBitsPerPel;
    DWORD  dmPelsWidth;
    DWORD  dmPelsHeight;
    DWORD  dmDisplayFlags;
    DWORD  dmDisplayFrequency;
    DWORD  dmICMMethod;
    DWORD  dmICMIntent;
    DWORD  dmMediaType;
    DWORD  dmDitherType;
    DWORD  dmReserved1;
    DWORD  dmReserved2;
} DEVMODEA, *LPDEVMODEA;

typedef struct
{
    WCHAR  dmDeviceName[CCHDEVICENAME];
    WORD   dmSpecVersion;
    WORD   dmDriverVersion;
    WORD   dmSize;
    WORD   dmDriverExtra;
    DWORD  dmFields;
    INT16  dmOrientation;
    INT16  dmPaperSize;
    INT16  dmPaperLength;
    INT16  dmPaperWidth;
    INT16  dmScale;
    INT16  dmCopies;
    INT16  dmDefaultSource;
    INT16  dmPrintQuality;
    INT16  dmColor;
    INT16  dmDuplex;
    INT16  dmYResolution;
    INT16  dmTTOption;
    INT16  dmCollate;
    WCHAR  dmFormName[CCHFORMNAME];
    WORD   dmLogPixels;
    DWORD  dmBitsPerPel;
    DWORD  dmPelsWidth;
    DWORD  dmPelsHeight;
    DWORD  dmDisplayFlags;
    DWORD  dmDisplayFrequency;
    DWORD  dmICMMethod;
    DWORD  dmICMIntent;
    DWORD  dmMediaType;
    DWORD  dmDitherType;
    DWORD  dmReserved1;
    DWORD  dmReserved2;
} DEVMODEW, *LPDEVMODEW;

DECL_WINELIB_TYPE_AW(DEVMODE)
DECL_WINELIB_TYPE_AW(LPDEVMODE)

typedef struct 
{
    INT16    cbSize;
    SEGPTR   lpszDocName WINE_PACKED;
    SEGPTR   lpszOutput WINE_PACKED;
} DOCINFO16, *LPDOCINFO16;

typedef struct 
{
    INT    cbSize;
    LPCSTR   lpszDocName;
    LPCSTR   lpszOutput;
    LPCSTR   lpszDatatype;
    DWORD    fwType;
} DOCINFOA, *LPDOCINFOA;

typedef struct 
{
    INT    cbSize;
    LPCWSTR  lpszDocName;
    LPCWSTR  lpszOutput;
    LPCWSTR  lpszDatatype;
    DWORD    fwType;
} DOCINFOW, *LPDOCINFOW;

DECL_WINELIB_TYPE_AW(DOCINFO)
DECL_WINELIB_TYPE_AW(LPDOCINFO)

typedef struct {
	UINT16		cbSize;
	INT16		iBorderWidth;
	INT16		iScrollWidth;
	INT16		iScrollHeight;
	INT16		iCaptionWidth;
	INT16		iCaptionHeight;
	LOGFONT16	lfCaptionFont;
	INT16		iSmCaptionWidth;
	INT16		iSmCaptionHeight;
	LOGFONT16	lfSmCaptionFont;
	INT16		iMenuWidth;
	INT16		iMenuHeight;
	LOGFONT16	lfMenuFont;
	LOGFONT16	lfStatusFont;
	LOGFONT16	lfMessageFont;
} NONCLIENTMETRICS16,*LPNONCLIENTMETRICS16;

typedef struct {
	UINT		cbSize;
	INT		iBorderWidth;
	INT		iScrollWidth;
	INT		iScrollHeight;
	INT		iCaptionWidth;
	INT		iCaptionHeight;
	LOGFONTA	lfCaptionFont;
	INT		iSmCaptionWidth;
	INT		iSmCaptionHeight;
	LOGFONTA	lfSmCaptionFont;
	INT		iMenuWidth;
	INT		iMenuHeight;
	LOGFONTA	lfMenuFont;
	LOGFONTA	lfStatusFont;
	LOGFONTA	lfMessageFont;
} NONCLIENTMETRICSA,*LPNONCLIENTMETRICSA;

typedef struct {
	UINT		cbSize;
	INT		iBorderWidth;
	INT		iScrollWidth;
	INT		iScrollHeight;
	INT		iCaptionWidth;
	INT		iCaptionHeight;
	LOGFONTW	lfCaptionFont;
	INT		iSmCaptionWidth;
	INT		iSmCaptionHeight;
	LOGFONTW	lfSmCaptionFont;
	INT		iMenuWidth;
	INT		iMenuHeight;
	LOGFONTW	lfMenuFont;
	LOGFONTW	lfStatusFont;
	LOGFONTW	lfMessageFont;
} NONCLIENTMETRICSW,*LPNONCLIENTMETRICSW;

DECL_WINELIB_TYPE_AW(NONCLIENTMETRICS)
DECL_WINELIB_TYPE_AW(LPNONCLIENTMETRICS)

/* Flags for PolyDraw and GetPath */
#define PT_CLOSEFIGURE          0x0001
#define PT_LINETO               0x0002
#define PT_BEZIERTO             0x0004
#define PT_MOVETO               0x0006

#define	RDH_RECTANGLES  1

typedef struct _RGNDATAHEADER {
    DWORD	dwSize;
    DWORD	iType;
    DWORD	nCount;
    DWORD	nRgnSize;
    RECT	rcBound;
} RGNDATAHEADER,*LPRGNDATAHEADER;

typedef struct _RGNDATA {
    RGNDATAHEADER	rdh;
    char		Buffer[1];
} RGNDATA,*PRGNDATA,*LPRGNDATA;

typedef BOOL16 (CALLBACK* ABORTPROC16)(HDC16, INT16);
typedef BOOL (CALLBACK* ABORTPROC)(HDC, INT);

#pragma pack(4)

/* Declarations for functions that exist only in Win16 */

VOID        WINAPI Death16(HDC16);
VOID        WINAPI Resurrection16(HDC16,WORD,WORD,WORD,WORD,WORD,WORD);

/* Declarations for functions that exist only in Win32 */

BOOL      WINAPI AngleArc(HDC, INT, INT, DWORD, FLOAT, FLOAT);
BOOL      WINAPI ArcTo(HDC, INT, INT, INT, INT, INT, INT, INT, INT); 
HENHMETAFILE WINAPI CloseEnhMetaFile(HDC);
HBRUSH    WINAPI CreateDIBPatternBrushPt(const void*,UINT);
HDC       WINAPI CreateEnhMetaFileA(HDC,LPCSTR,const RECT*,LPCSTR);
HDC       WINAPI CreateEnhMetaFileW(HDC,LPCWSTR,const RECT*,LPCWSTR);
#define     CreateEnhMetaFile WINELIB_NAME_AW(CreateEnhMetaFile)
INT       WINAPI DrawEscape(HDC,INT,INT,LPCSTR);
INT16       WINAPI ExcludeVisRect16(HDC16,INT16,INT16,INT16,INT16);
BOOL16      WINAPI FastWindowFrame16(HDC16,const RECT16*,INT16,INT16,DWORD);
UINT16      WINAPI GDIRealizePalette16(HDC16);
HPALETTE16  WINAPI GDISelectPalette16(HDC16,HPALETTE16,WORD);
BOOL      WINAPI GdiComment(HDC,UINT,const BYTE *);
DWORD       WINAPI GetBitmapDimension16(HBITMAP16);
DWORD       WINAPI GetBrushOrg16(HDC16);
BOOL      WINAPI GetCharABCWidthsFloatA(HDC,UINT,UINT,LPABCFLOAT);
BOOL      WINAPI GetCharABCWidthsFloatW(HDC,UINT,UINT,LPABCFLOAT);
#define     GetCharABCWidthsFloat WINELIB_NAME_AW(GetCharABCWidthsFloat)
BOOL      WINAPI GetCharWidthFloatA(HDC,UINT,UINT,PFLOAT);
BOOL      WINAPI GetCharWidthFloatW(HDC,UINT,UINT,PFLOAT);
#define     GetCharWidthFloat WINELIB_NAME_AW(GetCharWidthFloat)
BOOL      WINAPI GetColorAdjustment(HDC, LPCOLORADJUSTMENT);
HFONT16     WINAPI GetCurLogFont16(HDC16);
DWORD       WINAPI GetCurrentPosition16(HDC16);
DWORD       WINAPI GetDCHook(HDC16,FARPROC16*);
DWORD       WINAPI GetDCOrg16(HDC16);
HDC16       WINAPI GetDCState16(HDC16);
INT16       WINAPI GetEnvironment16(LPCSTR,LPDEVMODE16,UINT16);
HGLOBAL16   WINAPI GetMetaFileBits16(HMETAFILE16);
BOOL      WINAPI GetMiterLimit(HDC, PFLOAT);
DWORD       WINAPI GetTextExtent16(HDC16,LPCSTR,INT16);
DWORD       WINAPI GetViewportExt16(HDC16);
DWORD       WINAPI GetViewportOrg16(HDC16);
DWORD       WINAPI GetWindowExt16(HDC16);
DWORD       WINAPI GetWindowOrg16(HDC16);
HRGN16      WINAPI InquireVisRgn16(HDC16);
INT16       WINAPI IntersectVisRect16(HDC16,INT16,INT16,INT16,INT16);
BOOL16      WINAPI IsDCCurrentPalette16(HDC16);
BOOL16      WINAPI IsGDIObject16(HGDIOBJ16);
BOOL16      WINAPI IsValidMetaFile16(HMETAFILE16);
BOOL      WINAPI MaskBlt(HDC,INT,INT,INT,INT,HDC,INT,INT,HBITMAP,INT,INT,DWORD);
DWORD       WINAPI MoveTo16(HDC16,INT16,INT16);
DWORD       WINAPI OffsetViewportOrg16(HDC16,INT16,INT16);
INT16       WINAPI OffsetVisRgn16(HDC16,INT16,INT16);
DWORD       WINAPI OffsetWindowOrg16(HDC16,INT16,INT16);
BOOL      WINAPI PlgBlt(HDC,const POINT*,HDC,INT,INT,INT,INT,HBITMAP,INT,INT);
BOOL      WINAPI PolyDraw(HDC,const POINT*,const BYTE*,DWORD);
UINT16      WINAPI RealizeDefaultPalette16(HDC16);
INT16       WINAPI RestoreVisRgn16(HDC16);
HRGN16      WINAPI SaveVisRgn16(HDC16);
DWORD       WINAPI ScaleViewportExt16(HDC16,INT16,INT16,INT16,INT16);
DWORD       WINAPI ScaleWindowExt16(HDC16,INT16,INT16,INT16,INT16);
INT16       WINAPI SelectVisRgn16(HDC16,HRGN16);
DWORD       WINAPI SetBitmapDimension16(HBITMAP16,INT16,INT16);
DWORD       WINAPI SetBrushOrg16(HDC16,INT16,INT16);
BOOL      WINAPI SetColorAdjustment(HDC,const COLORADJUSTMENT*);
BOOL16      WINAPI SetDCHook(HDC16,FARPROC16,DWORD);
DWORD       WINAPI SetDCOrg16(HDC16,INT16,INT16);
VOID        WINAPI SetDCState16(HDC16,HDC16);
INT16       WINAPI SetEnvironment16(LPCSTR,LPDEVMODE16,UINT16);
WORD        WINAPI SetHookFlags16(HDC16,WORD);
HMETAFILE16 WINAPI SetMetaFileBits16(HGLOBAL16);
BOOL      WINAPI SetMiterLimit(HDC, FLOAT, PFLOAT);
DWORD       WINAPI SetViewportExt16(HDC16,INT16,INT16);
DWORD       WINAPI SetViewportOrg16(HDC16,INT16,INT16);
DWORD       WINAPI SetWindowExt16(HDC16,INT16,INT16);
DWORD       WINAPI SetWindowOrg16(HDC16,INT16,INT16);
BOOL      WINAPI CombineTransform(LPXFORM,const XFORM *,const XFORM *);
HENHMETAFILE WINAPI CopyEnhMetaFileA(HENHMETAFILE,LPCSTR);
HENHMETAFILE WINAPI CopyEnhMetaFileW(HENHMETAFILE,LPCWSTR);
#define     CopyEnhMetaFile WINELIB_NAME_AW(CopyEnhMetaFile)
HPALETTE  WINAPI CreateHalftonePalette(HDC);
BOOL      WINAPI DeleteEnhMetaFile(HENHMETAFILE);
INT       WINAPI ExtSelectClipRgn(HDC,HRGN,INT);
HRGN      WINAPI ExtCreateRegion(const XFORM*,DWORD,const RGNDATA*);
INT       WINAPI ExtEscape(HDC,INT,INT,LPCSTR,INT,LPSTR);
BOOL      WINAPI FixBrushOrgEx(HDC,INT,INT,LPPOINT);
HANDLE    WINAPI GetCurrentObject(HDC,UINT);
BOOL      WINAPI GetDCOrgEx(HDC,LPPOINT);
HENHMETAFILE WINAPI GetEnhMetaFileA(LPCSTR);
HENHMETAFILE WINAPI GetEnhMetaFileW(LPCWSTR);
#define     GetEnhMetaFile WINELIB_NAME_AW(GetEnhMetaFile)
UINT      WINAPI GetEnhMetaFileBits(HENHMETAFILE,UINT,LPBYTE);
UINT      WINAPI GetEnhMetaFileHeader(HENHMETAFILE,UINT,LPENHMETAHEADER);
UINT      WINAPI GetEnhMetaFilePaletteEntries(HENHMETAFILE,UINT,LPPALETTEENTRY);
INT       WINAPI GetGraphicsMode(HDC);
UINT      WINAPI GetMetaFileBitsEx(HMETAFILE,UINT,LPVOID);
DWORD       WINAPI GetObjectType(HANDLE);
UINT      WINAPI GetTextCharsetInfo(HDC,LPFONTSIGNATURE,DWORD);
BOOL      WINAPI GetTextExtentExPointA(HDC,LPCSTR,INT,INT,
                                           LPINT,LPINT,LPSIZE);
BOOL      WINAPI GetTextExtentExPointW(HDC,LPCWSTR,INT,INT,
                                           LPINT,LPINT,LPSIZE);
#define     GetTextExtentExPoint WINELIB_NAME_AW(GetTextExtentExPoint)
BOOL      WINAPI GetWorldTransform(HDC,LPXFORM);
BOOL      WINAPI ModifyWorldTransform(HDC,const XFORM *, DWORD);
BOOL      WINAPI PlayEnhMetaFile(HDC,HENHMETAFILE,const RECT*);
BOOL      WINAPI PlayEnhMetaFileRecord(HDC,LPHANDLETABLE,const ENHMETARECORD*,UINT);
BOOL      WINAPI PolyPolyline(HDC,const POINT*,const DWORD*,DWORD);
BOOL      WINAPI SetBrushOrgEx(HDC,INT,INT,LPPOINT);
HENHMETAFILE WINAPI SetEnhMetaFileBits(UINT,const BYTE *);
INT       WINAPI SetGraphicsMode(HDC,INT);
HMETAFILE WINAPI SetMetaFileBitsEx(UINT,const BYTE*);
BOOL      WINAPI SetWorldTransform(HDC,const XFORM*);
BOOL      WINAPI TranslateCharsetInfo(LPDWORD,LPCHARSETINFO,DWORD);

/* Declarations for functions that change between Win16 and Win32 */

INT16       WINAPI AbortDoc16(HDC16);
INT       WINAPI AbortDoc(HDC);
BOOL16      WINAPI AbortPath16(HDC16);
BOOL      WINAPI AbortPath(HDC);
INT16       WINAPI AddFontResource16(LPCSTR);
INT       WINAPI AddFontResourceA(LPCSTR);
INT       WINAPI AddFontResourceW(LPCWSTR);
#define     AddFontResource WINELIB_NAME_AW(AddFontResource)
void        WINAPI AnimatePalette16(HPALETTE16,UINT16,UINT16,const PALETTEENTRY*);
BOOL      WINAPI AnimatePalette(HPALETTE,UINT,UINT,const PALETTEENTRY*);
BOOL16      WINAPI Arc16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL      WINAPI Arc(HDC,INT,INT,INT,INT,INT,INT,INT,INT);
BOOL16      WINAPI BeginPath16(HDC16);
BOOL      WINAPI BeginPath(HDC);
BOOL16      WINAPI BitBlt16(HDC16,INT16,INT16,INT16,INT16,HDC16,INT16,INT16,DWORD);
BOOL      WINAPI BitBlt(HDC,INT,INT,INT,INT,HDC,INT,INT,DWORD);
INT       WINAPI ChoosePixelFormat(HDC,const PIXELFORMATDESCRIPTOR*);
BOOL16      WINAPI Chord16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL      WINAPI Chord(HDC,INT,INT,INT,INT,INT,INT,INT,INT);
BOOL16      WINAPI CloseFigure16(HDC16);
BOOL      WINAPI CloseFigure(HDC);
HMETAFILE16 WINAPI CloseMetaFile16(HDC16);
HMETAFILE WINAPI CloseMetaFile(HDC);
INT16       WINAPI CombineRgn16(HRGN16,HRGN16,HRGN16,INT16);
INT       WINAPI CombineRgn(HRGN,HRGN,HRGN,INT);
HMETAFILE16 WINAPI CopyMetaFile16(HMETAFILE16,LPCSTR);
HMETAFILE WINAPI CopyMetaFileA(HMETAFILE,LPCSTR);
HMETAFILE WINAPI CopyMetaFileW(HMETAFILE,LPCWSTR);
#define     CopyMetaFile WINELIB_NAME_AW(CopyMetaFile)
HBITMAP16   WINAPI CreateBitmap16(INT16,INT16,UINT16,UINT16,LPCVOID);
HBITMAP   WINAPI CreateBitmap(INT,INT,UINT,UINT,LPCVOID);
HBITMAP16   WINAPI CreateBitmapIndirect16(const BITMAP16*);
HBITMAP   WINAPI CreateBitmapIndirect(const BITMAP*);
HBRUSH16    WINAPI CreateBrushIndirect16(const LOGBRUSH16*);
HBRUSH    WINAPI CreateBrushIndirect(const LOGBRUSH*);
HBITMAP16   WINAPI CreateCompatibleBitmap16(HDC16,INT16,INT16);
HBITMAP   WINAPI CreateCompatibleBitmap(HDC,INT,INT);
HDC16       WINAPI CreateCompatibleDC16(HDC16);
HDC       WINAPI CreateCompatibleDC(HDC);
HDC16       WINAPI CreateDC16(LPCSTR,LPCSTR,LPCSTR,const DEVMODE16*);
HDC       WINAPI CreateDCA(LPCSTR,LPCSTR,LPCSTR,const DEVMODEA*);
HDC       WINAPI CreateDCW(LPCWSTR,LPCWSTR,LPCWSTR,const DEVMODEW*);
#define     CreateDC WINELIB_NAME_AW(CreateDC)
HBITMAP16   WINAPI CreateDIBitmap16(HDC16,const BITMAPINFOHEADER*,DWORD,
                                    LPCVOID,const BITMAPINFO*,UINT16);
HBITMAP   WINAPI CreateDIBitmap(HDC,const BITMAPINFOHEADER*,DWORD,
                                    LPCVOID,const BITMAPINFO*,UINT);
HBRUSH16    WINAPI CreateDIBPatternBrush16(HGLOBAL16,UINT16);
HBRUSH    WINAPI CreateDIBPatternBrush(HGLOBAL,UINT);
HBITMAP16   WINAPI CreateDIBSection16 (HDC16, BITMAPINFO *, UINT16,
				       SEGPTR *, HANDLE, DWORD offset);
HBITMAP   WINAPI CreateDIBSection (HDC, BITMAPINFO *, UINT,
				       LPVOID *, HANDLE, DWORD offset);
HBITMAP16   WINAPI CreateDiscardableBitmap16(HDC16,INT16,INT16);
HBITMAP   WINAPI CreateDiscardableBitmap(HDC,INT,INT);
HRGN16      WINAPI CreateEllipticRgn16(INT16,INT16,INT16,INT16);
HRGN      WINAPI CreateEllipticRgn(INT,INT,INT,INT);
HRGN16      WINAPI CreateEllipticRgnIndirect16(const RECT16 *);
HRGN      WINAPI CreateEllipticRgnIndirect(const RECT *);
HFONT16     WINAPI CreateFont16(INT16,INT16,INT16,INT16,INT16,BYTE,BYTE,BYTE,
                                BYTE,BYTE,BYTE,BYTE,BYTE,LPCSTR);
HFONT     WINAPI CreateFontA(INT,INT,INT,INT,INT,DWORD,DWORD,
                                 DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCSTR);
HFONT     WINAPI CreateFontW(INT,INT,INT,INT,INT,DWORD,DWORD,
                                 DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCWSTR);
#define     CreateFont WINELIB_NAME_AW(CreateFont)
HFONT16     WINAPI CreateFontIndirect16(const LOGFONT16*);
HFONT     WINAPI CreateFontIndirectA(const LOGFONTA*);
HFONT     WINAPI CreateFontIndirectW(const LOGFONTW*);
#define     CreateFontIndirect WINELIB_NAME_AW(CreateFontIndirect)
HBRUSH16    WINAPI CreateHatchBrush16(INT16,COLORREF);
HBRUSH    WINAPI CreateHatchBrush(INT,COLORREF);
HDC16       WINAPI CreateIC16(LPCSTR,LPCSTR,LPCSTR,const DEVMODE16*);
HDC       WINAPI CreateICA(LPCSTR,LPCSTR,LPCSTR,const DEVMODEA*);
HDC       WINAPI CreateICW(LPCWSTR,LPCWSTR,LPCWSTR,const DEVMODEW*);
#define     CreateIC WINELIB_NAME_AW(CreateIC)
HDC16       WINAPI CreateMetaFile16(LPCSTR);
HDC       WINAPI CreateMetaFileA(LPCSTR);
HDC       WINAPI CreateMetaFileW(LPCWSTR);
#define     CreateMetaFile WINELIB_NAME_AW(CreateMetaFile)
HPALETTE16  WINAPI CreatePalette16(const LOGPALETTE*);
HPALETTE  WINAPI CreatePalette(const LOGPALETTE*);
HBRUSH16    WINAPI CreatePatternBrush16(HBITMAP16);
HBRUSH    WINAPI CreatePatternBrush(HBITMAP);
HPEN16      WINAPI CreatePen16(INT16,INT16,COLORREF);
HPEN      WINAPI CreatePen(INT,INT,COLORREF);
HPEN16      WINAPI CreatePenIndirect16(const LOGPEN16*);
HPEN      WINAPI CreatePenIndirect(const LOGPEN*);
HRGN16      WINAPI CreatePolyPolygonRgn16(const POINT16*,const INT16*,INT16,INT16);
HRGN      WINAPI CreatePolyPolygonRgn(const POINT*,const INT*,INT,INT);
HRGN16      WINAPI CreatePolygonRgn16(const POINT16*,INT16,INT16);
HRGN      WINAPI CreatePolygonRgn(const POINT*,INT,INT);
HRGN16      WINAPI CreateRectRgn16(INT16,INT16,INT16,INT16);
HRGN      WINAPI CreateRectRgn(INT,INT,INT,INT);
HRGN16      WINAPI CreateRectRgnIndirect16(const RECT16*);
HRGN      WINAPI CreateRectRgnIndirect(const RECT*);
HRGN16      WINAPI CreateRoundRectRgn16(INT16,INT16,INT16,INT16,INT16,INT16);
HRGN      WINAPI CreateRoundRectRgn(INT,INT,INT,INT,INT,INT);
BOOL16      WINAPI CreateScalableFontResource16(UINT16,LPCSTR,LPCSTR,LPCSTR);
BOOL      WINAPI CreateScalableFontResourceA(DWORD,LPCSTR,LPCSTR,LPCSTR);
BOOL      WINAPI CreateScalableFontResourceW(DWORD,LPCWSTR,LPCWSTR,LPCWSTR);
#define     CreateScalableFontResource WINELIB_NAME_AW(CreateScalableFontResource)
HBRUSH16    WINAPI CreateSolidBrush16(COLORREF);
HBRUSH    WINAPI CreateSolidBrush(COLORREF);
BOOL16      WINAPI DeleteDC16(HDC16);
BOOL      WINAPI DeleteDC(HDC);
BOOL16      WINAPI DeleteMetaFile16(HMETAFILE16);
BOOL      WINAPI DeleteMetaFile(HMETAFILE);
BOOL16      WINAPI DeleteObject16(HGDIOBJ16);
BOOL      WINAPI DeleteObject(HGDIOBJ);
INT       WINAPI DescribePixelFormat(HDC,int,UINT,
                                       LPPIXELFORMATDESCRIPTOR);
BOOL16      WINAPI DPtoLP16(HDC16,LPPOINT16,INT16);
BOOL      WINAPI DPtoLP(HDC,LPPOINT,INT);
BOOL16      WINAPI Ellipse16(HDC16,INT16,INT16,INT16,INT16);
BOOL      WINAPI Ellipse(HDC,INT,INT,INT,INT);
INT16       WINAPI EndDoc16(HDC16);
INT       WINAPI EndDoc(HDC);
BOOL16      WINAPI EndPath16(HDC16);
BOOL      WINAPI EndPath(HDC);
INT16       WINAPI EnumFontFamilies16(HDC16,LPCSTR,FONTENUMPROC16,LPARAM);
INT       WINAPI EnumFontFamiliesA(HDC,LPCSTR,FONTENUMPROCA,LPARAM);
INT       WINAPI EnumFontFamiliesW(HDC,LPCWSTR,FONTENUMPROCW,LPARAM);
#define     EnumFontFamilies WINELIB_NAME_AW(EnumFontFamilies)
INT16       WINAPI EnumFontFamiliesEx16(HDC16,LPLOGFONT16,FONTENUMPROCEX16,LPARAM,DWORD);
INT       WINAPI EnumFontFamiliesExA(HDC,LPLOGFONTA,FONTENUMPROCEXA,LPARAM,DWORD);
INT       WINAPI EnumFontFamiliesExW(HDC,LPLOGFONTW,FONTENUMPROCEXW,LPARAM,DWORD);
#define     EnumFontFamiliesEx WINELIB_NAME_AW(EnumFontFamiliesEx)
INT16       WINAPI EnumFonts16(HDC16,LPCSTR,FONTENUMPROC16,LPARAM);
INT       WINAPI EnumFontsA(HDC,LPCSTR,FONTENUMPROCA,LPARAM);
INT       WINAPI EnumFontsW(HDC,LPCWSTR,FONTENUMPROCW,LPARAM);
#define     EnumFonts WINELIB_NAME_AW(EnumFonts)
BOOL16      WINAPI EnumMetaFile16(HDC16,HMETAFILE16,MFENUMPROC16,LPARAM);
BOOL      WINAPI EnumMetaFile(HDC,HMETAFILE,MFENUMPROC,LPARAM);
INT16       WINAPI EnumObjects16(HDC16,INT16,GOBJENUMPROC16,LPARAM);
INT       WINAPI EnumObjects(HDC,INT,GOBJENUMPROC,LPARAM);
BOOL16      WINAPI EqualRgn16(HRGN16,HRGN16);
BOOL      WINAPI EqualRgn(HRGN,HRGN);
INT16       WINAPI Escape16(HDC16,INT16,INT16,SEGPTR,SEGPTR);
INT       WINAPI Escape(HDC,INT,INT,LPCSTR,LPVOID);
INT16       WINAPI ExcludeClipRect16(HDC16,INT16,INT16,INT16,INT16);
INT       WINAPI ExcludeClipRect(HDC,INT,INT,INT,INT);
HPEN16      WINAPI ExtCreatePen16(DWORD,DWORD,const LOGBRUSH16*,DWORD,const DWORD*);
HPEN      WINAPI ExtCreatePen(DWORD,DWORD,const LOGBRUSH*,DWORD,const DWORD*);
BOOL16      WINAPI ExtFloodFill16(HDC16,INT16,INT16,COLORREF,UINT16);
BOOL      WINAPI ExtFloodFill(HDC,INT,INT,COLORREF,UINT);
BOOL16      WINAPI ExtTextOut16(HDC16,INT16,INT16,UINT16,const RECT16*,
                                LPCSTR,UINT16,const INT16*);
BOOL      WINAPI ExtTextOutA(HDC,INT,INT,UINT,const RECT*,
                                 LPCSTR,UINT,const INT*);
BOOL      WINAPI ExtTextOutW(HDC,INT,INT,UINT,const RECT*,
                                 LPCWSTR,UINT,const INT*);
#define     ExtTextOut WINELIB_NAME_AW(ExtTextOut)
BOOL16      WINAPI FillPath16(HDC16);
BOOL      WINAPI FillPath(HDC);
BOOL16      WINAPI FillRgn16(HDC16,HRGN16,HBRUSH16);
BOOL      WINAPI FillRgn(HDC,HRGN,HBRUSH);
BOOL16      WINAPI FlattenPath16(HDC16);
BOOL      WINAPI FlattenPath(HDC);
BOOL16      WINAPI FloodFill16(HDC16,INT16,INT16,COLORREF);
BOOL      WINAPI FloodFill(HDC,INT,INT,COLORREF);
BOOL16      WINAPI FrameRgn16(HDC16,HRGN16,HBRUSH16,INT16,INT16);
BOOL      WINAPI FrameRgn(HDC,HRGN,HBRUSH,INT,INT);
INT16       WINAPI GetArcDirection16(HDC16);
INT       WINAPI GetArcDirection(HDC);
BOOL16      WINAPI GetAspectRatioFilterEx16(HDC16,LPSIZE16);
BOOL      WINAPI GetAspectRatioFilterEx(HDC,LPSIZE);
LONG        WINAPI GetBitmapBits16(HBITMAP16,LONG,LPVOID);
LONG        WINAPI GetBitmapBits(HBITMAP,LONG,LPVOID);
BOOL16      WINAPI GetBitmapDimensionEx16(HBITMAP16,LPSIZE16);
BOOL      WINAPI GetBitmapDimensionEx(HBITMAP,LPSIZE);
BOOL16      WINAPI GetBrushOrgEx16(HDC16,LPPOINT16);
BOOL      WINAPI GetBrushOrgEx(HDC,LPPOINT);
COLORREF    WINAPI GetBkColor16(HDC16);
COLORREF    WINAPI GetBkColor(HDC);
INT16       WINAPI GetBkMode16(HDC16);
INT       WINAPI GetBkMode(HDC);
UINT16      WINAPI GetBoundsRect16(HDC16,LPRECT16,UINT16);
UINT      WINAPI GetBoundsRect(HDC,LPRECT,UINT);
BOOL16      WINAPI GetCharABCWidths16(HDC16,UINT16,UINT16,LPABC16);
BOOL      WINAPI GetCharABCWidthsA(HDC,UINT,UINT,LPABC);
BOOL      WINAPI GetCharABCWidthsW(HDC,UINT,UINT,LPABC);
#define     GetCharABCWidths WINELIB_NAME_AW(GetCharABCWidths)
DWORD       WINAPI GetCharacterPlacementA(HDC,LPCSTR,INT,INT,GCP_RESULTSA*,DWORD);
DWORD       WINAPI GetCharacterPlacementW(HDC,LPCWSTR,INT,INT,GCP_RESULTSW*,DWORD);
#define     GetCharacterPlacement WINELIB_NAME_AW(GetCharacterPlacement)
BOOL16      WINAPI GetCharWidth16(HDC16,UINT16,UINT16,LPINT16);
BOOL      WINAPI GetCharWidth32A(HDC,UINT,UINT,LPINT);
BOOL      WINAPI GetCharWidth32W(HDC,UINT,UINT,LPINT);
#define     GetCharWidth WINELIB_NAME_AW(GetCharWidth)
INT16       WINAPI GetClipBox16(HDC16,LPRECT16);
INT       WINAPI GetClipBox(HDC,LPRECT);
HRGN16      WINAPI GetClipRgn16(HDC16);
INT       WINAPI GetClipRgn(HDC,HRGN);
BOOL16      WINAPI GetCurrentPositionEx16(HDC16,LPPOINT16);
BOOL      WINAPI GetCurrentPositionEx(HDC,LPPOINT);
INT16       WINAPI GetDeviceCaps16(HDC16,INT16);
INT       WINAPI GetDeviceCaps(HDC,INT);
UINT16      WINAPI GetDIBColorTable16(HDC16,UINT16,UINT16,RGBQUAD*);
UINT      WINAPI GetDIBColorTable(HDC,UINT,UINT,RGBQUAD*);
INT16       WINAPI GetDIBits16(HDC16,HBITMAP16,UINT16,UINT16,LPVOID,LPBITMAPINFO,UINT16);
INT       WINAPI GetDIBits(HDC,HBITMAP,UINT,UINT,LPVOID,LPBITMAPINFO,UINT);
DWORD       WINAPI GetFontData(HDC,DWORD,DWORD,LPVOID,DWORD);
DWORD       WINAPI GetFontLanguageInfo16(HDC16);
DWORD       WINAPI GetFontLanguageInfo(HDC);
DWORD       WINAPI GetGlyphOutline16(HDC16,UINT16,UINT16,LPGLYPHMETRICS16,DWORD,LPVOID,const MAT2*);
DWORD       WINAPI GetGlyphOutlineA(HDC,UINT,UINT,LPGLYPHMETRICS,DWORD,LPVOID,const MAT2*);
DWORD       WINAPI GetGlyphOutlineW(HDC,UINT,UINT,LPGLYPHMETRICS,DWORD,LPVOID,const MAT2*);
#define     GetGlyphOutline WINELIB_NAME_AW(GetGlyphOutline)
INT16       WINAPI GetKerningPairs16(HDC16,INT16,LPKERNINGPAIR16);
DWORD       WINAPI GetKerningPairsA(HDC,DWORD,LPKERNINGPAIR);
DWORD       WINAPI GetKerningPairsW(HDC,DWORD,LPKERNINGPAIR);
#define     GetKerningPairs WINELIB_NAME_AW(GetKerningPairs)
INT16       WINAPI GetMapMode16(HDC16);
INT       WINAPI GetMapMode(HDC);
HMETAFILE16 WINAPI GetMetaFile16(LPCSTR);
HMETAFILE WINAPI GetMetaFileA(LPCSTR);
HMETAFILE WINAPI GetMetaFileW(LPCWSTR);
#define     GetMetaFile WINELIB_NAME_AW(GetMetaFile)
DWORD       WINAPI GetNearestColor16(HDC16,DWORD);
DWORD       WINAPI GetNearestColor(HDC,DWORD);
UINT16      WINAPI GetNearestPaletteIndex16(HPALETTE16,COLORREF);
UINT      WINAPI GetNearestPaletteIndex(HPALETTE,COLORREF);
INT16       WINAPI GetObject16(HANDLE16,INT16,LPVOID);
INT       WINAPI GetObjectA(HANDLE,INT,LPVOID);
INT       WINAPI GetObjectW(HANDLE,INT,LPVOID);
#define     GetObject WINELIB_NAME_AW(GetObject)
UINT16      WINAPI GetOutlineTextMetrics16(HDC16,UINT16,LPOUTLINETEXTMETRIC16);
UINT      WINAPI GetOutlineTextMetricsA(HDC,UINT,LPOUTLINETEXTMETRICA);
UINT      WINAPI GetOutlineTextMetricsW(HDC,UINT,LPOUTLINETEXTMETRICW);
#define     GetOutlineTextMetrics WINELIB_NAME_AW(GetOutlineTextMetrics)
UINT16      WINAPI GetPaletteEntries16(HPALETTE16,UINT16,UINT16,LPPALETTEENTRY);
UINT      WINAPI GetPaletteEntries(HPALETTE,UINT,UINT,LPPALETTEENTRY);
INT16       WINAPI GetPath16(HDC16,LPPOINT16,LPBYTE,INT16);
INT       WINAPI GetPath(HDC,LPPOINT,LPBYTE,INT);
COLORREF    WINAPI GetPixel16(HDC16,INT16,INT16);
COLORREF    WINAPI GetPixel(HDC,INT,INT);
INT       WINAPI GetPixelFormat(HDC);
INT16       WINAPI GetPolyFillMode16(HDC16);
INT       WINAPI GetPolyFillMode(HDC);
BOOL16      WINAPI GetRasterizerCaps16(LPRASTERIZER_STATUS,UINT16);
BOOL      WINAPI GetRasterizerCaps(LPRASTERIZER_STATUS,UINT);
DWORD       WINAPI GetRegionData16(HRGN16,DWORD,LPRGNDATA);
DWORD       WINAPI GetRegionData(HRGN,DWORD,LPRGNDATA);
INT16       WINAPI GetRelAbs16(HDC16);
INT       WINAPI GetRelAbs(HDC);
INT16       WINAPI GetRgnBox16(HRGN16,LPRECT16);
INT       WINAPI GetRgnBox(HRGN,LPRECT);
INT16       WINAPI GetROP216(HDC16);
INT       WINAPI GetROP2(HDC);
HGDIOBJ16   WINAPI GetStockObject16(INT16);
HGDIOBJ   WINAPI GetStockObject(INT);
INT16       WINAPI GetStretchBltMode16(HDC16);
INT       WINAPI GetStretchBltMode(HDC);
UINT16      WINAPI GetSystemPaletteEntries16(HDC16,UINT16,UINT16,LPPALETTEENTRY);
UINT      WINAPI GetSystemPaletteEntries(HDC,UINT,UINT,LPPALETTEENTRY);
UINT16      WINAPI GetSystemPaletteUse16(HDC16);
UINT      WINAPI GetSystemPaletteUse(HDC);
UINT16      WINAPI GetTextAlign16(HDC16);
UINT      WINAPI GetTextAlign(HDC);
INT16       WINAPI GetTextCharacterExtra16(HDC16);
INT       WINAPI GetTextCharacterExtra(HDC);
UINT16      WINAPI GetTextCharset16(HDC16);
UINT      WINAPI GetTextCharset(HDC);
COLORREF    WINAPI GetTextColor16(HDC16);
COLORREF    WINAPI GetTextColor(HDC);
/* this one is different, because Win32 has *both* 
 * GetTextExtentPoint and GetTextExtentPoint32 !
 */
BOOL16      WINAPI GetTextExtentPoint16(HDC16,LPCSTR,INT16,LPSIZE16);
BOOL      WINAPI GetTextExtentPoint32A(HDC,LPCSTR,INT,LPSIZE);
BOOL      WINAPI GetTextExtentPoint32W(HDC,LPCWSTR,INT,LPSIZE);
BOOL      WINAPI GetTextExtentPointA(HDC,LPCSTR,INT,LPSIZE);
BOOL      WINAPI GetTextExtentPointW(HDC,LPCWSTR,INT,LPSIZE);
#ifdef UNICODE
#define     GetTextExtentPoint GetTextExtentPointW
#define     GetTextExtentPoint32 GetTextExtentPoint32W
#else
#define     GetTextExtentPoint GetTextExtentPointA
#define     GetTextExtentPoint32 GetTextExtentPoint32A
#endif
INT16       WINAPI GetTextFace16(HDC16,INT16,LPSTR);
INT       WINAPI GetTextFaceA(HDC,INT,LPSTR);
INT       WINAPI GetTextFaceW(HDC,INT,LPWSTR);
#define     GetTextFace WINELIB_NAME_AW(GetTextFace)
BOOL16      WINAPI GetTextMetrics16(HDC16,LPTEXTMETRIC16);
BOOL      WINAPI GetTextMetricsA(HDC,LPTEXTMETRICA);
BOOL      WINAPI GetTextMetricsW(HDC,LPTEXTMETRICW);
#define     GetTextMetrics WINELIB_NAME_AW(GetTextMetrics)
BOOL16      WINAPI GetViewportExtEx16(HDC16,LPSIZE16);
BOOL      WINAPI GetViewportExtEx(HDC,LPSIZE);
BOOL16      WINAPI GetViewportOrgEx16(HDC16,LPPOINT16);
BOOL      WINAPI GetViewportOrgEx(HDC,LPPOINT);
BOOL16      WINAPI GetWindowExtEx16(HDC16,LPSIZE16);
BOOL      WINAPI GetWindowExtEx(HDC,LPSIZE);
BOOL16      WINAPI GetWindowOrgEx16(HDC16,LPPOINT16);
BOOL      WINAPI GetWindowOrgEx(HDC,LPPOINT);
INT16       WINAPI IntersectClipRect16(HDC16,INT16,INT16,INT16,INT16);
INT       WINAPI IntersectClipRect(HDC,INT,INT,INT,INT);
BOOL16      WINAPI InvertRgn16(HDC16,HRGN16);
BOOL      WINAPI InvertRgn(HDC,HRGN);
VOID        WINAPI LineDDA16(INT16,INT16,INT16,INT16,LINEDDAPROC16,LPARAM);
BOOL      WINAPI LineDDA(INT,INT,INT,INT,LINEDDAPROC,LPARAM);
BOOL16      WINAPI LineTo16(HDC16,INT16,INT16);
BOOL      WINAPI LineTo(HDC,INT,INT);
BOOL16      WINAPI LPtoDP16(HDC16,LPPOINT16,INT16);
BOOL      WINAPI LPtoDP(HDC,LPPOINT,INT);
BOOL16      WINAPI MoveToEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL      WINAPI MoveToEx(HDC,INT,INT,LPPOINT);
INT16       WINAPI MulDiv16(INT16,INT16,INT16);
/* FIXME This is defined in kernel32.spec !?*/
INT       WINAPI MulDiv(INT,INT,INT);
INT16       WINAPI OffsetClipRgn16(HDC16,INT16,INT16);
INT       WINAPI OffsetClipRgn(HDC,INT,INT);
INT16       WINAPI OffsetRgn16(HRGN16,INT16,INT16);
INT       WINAPI OffsetRgn(HRGN,INT,INT);
BOOL16      WINAPI OffsetViewportOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL      WINAPI OffsetViewportOrgEx(HDC,INT,INT,LPPOINT);
BOOL16      WINAPI OffsetWindowOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL      WINAPI OffsetWindowOrgEx(HDC,INT,INT,LPPOINT);
BOOL16      WINAPI PaintRgn16(HDC16,HRGN16);
BOOL      WINAPI PaintRgn(HDC,HRGN);
BOOL16      WINAPI PatBlt16(HDC16,INT16,INT16,INT16,INT16,DWORD);
BOOL      WINAPI PatBlt(HDC,INT,INT,INT,INT,DWORD);
HRGN16      WINAPI PathToRegion16(HDC16);
HRGN      WINAPI PathToRegion(HDC);
BOOL16      WINAPI Pie16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL      WINAPI Pie(HDC,INT,INT,INT,INT,INT,INT,INT,INT);
BOOL16      WINAPI PlayMetaFile16(HDC16,HMETAFILE16);
BOOL      WINAPI PlayMetaFile(HDC,HMETAFILE);
VOID        WINAPI PlayMetaFileRecord16(HDC16,LPHANDLETABLE16,LPMETARECORD,UINT16);
BOOL      WINAPI PlayMetaFileRecord(HDC,LPHANDLETABLE,LPMETARECORD,UINT);
BOOL16      WINAPI PolyBezier16(HDC16,const POINT16*,INT16);
BOOL      WINAPI PolyBezier(HDC,const POINT*,DWORD);
BOOL16      WINAPI PolyBezierTo16(HDC16,const POINT16*,INT16);
BOOL      WINAPI PolyBezierTo(HDC,const POINT*,DWORD);
BOOL16      WINAPI PolyPolygon16(HDC16,const POINT16*,const INT16*,UINT16);
BOOL      WINAPI PolyPolygon(HDC,const POINT*,const INT*,UINT);
BOOL16      WINAPI Polygon16(HDC16,const POINT16*,INT16);
BOOL      WINAPI Polygon(HDC,const POINT*,INT);
BOOL16      WINAPI Polyline16(HDC16,const POINT16*,INT16);
BOOL      WINAPI Polyline(HDC,const POINT*,INT);
BOOL      WINAPI PolylineTo(HDC,const POINT*,DWORD);
BOOL16      WINAPI PtInRegion16(HRGN16,INT16,INT16);
BOOL      WINAPI PtInRegion(HRGN,INT,INT);
BOOL16      WINAPI PtVisible16(HDC16,INT16,INT16);
BOOL      WINAPI PtVisible(HDC,INT,INT);
/* FIXME This is defined in user.spec !? */
UINT16      WINAPI RealizePalette16(HDC16);
UINT      WINAPI RealizePalette(HDC);
BOOL16      WINAPI Rectangle16(HDC16,INT16,INT16,INT16,INT16);
BOOL      WINAPI Rectangle(HDC,INT,INT,INT,INT);
BOOL16      WINAPI RectInRegion16(HRGN16,const RECT16 *);
BOOL      WINAPI RectInRegion(HRGN,const RECT *);
BOOL16      WINAPI RectVisible16(HDC16,const RECT16*);
BOOL      WINAPI RectVisible(HDC,const RECT*);
BOOL16      WINAPI RemoveFontResource16(SEGPTR);
BOOL      WINAPI RemoveFontResourceA(LPCSTR);
BOOL      WINAPI RemoveFontResourceW(LPCWSTR);
#define     RemoveFontResource WINELIB_NAME_AW(RemoveFontResource)
HDC16       WINAPI ResetDC16(HDC16,const DEVMODE16 *);
HDC       WINAPI ResetDCA(HDC,const DEVMODEA *);
HDC       WINAPI ResetDCW(HDC,const DEVMODEW *);
#define     ResetDC WINELIB_NAME_AW(ResetDC)
BOOL16      WINAPI ResizePalette16(HPALETTE16,UINT16);
BOOL      WINAPI ResizePalette(HPALETTE,UINT);
BOOL16      WINAPI RestoreDC16(HDC16,INT16);
BOOL      WINAPI RestoreDC(HDC,INT);
BOOL16      WINAPI RoundRect16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL      WINAPI RoundRect(HDC,INT,INT,INT,INT,INT,INT);
INT16       WINAPI SaveDC16(HDC16);
INT       WINAPI SaveDC(HDC);
BOOL16      WINAPI ScaleViewportExtEx16(HDC16,INT16,INT16,INT16,INT16,LPSIZE16);
BOOL      WINAPI ScaleViewportExtEx(HDC,INT,INT,INT,INT,LPSIZE);
BOOL16      WINAPI ScaleWindowExtEx16(HDC16,INT16,INT16,INT16,INT16,LPSIZE16);
BOOL      WINAPI ScaleWindowExtEx(HDC,INT,INT,INT,INT,LPSIZE);
BOOL16      WINAPI SelectClipPath16(HDC16,INT16);
BOOL      WINAPI SelectClipPath(HDC,INT);
INT16       WINAPI SelectClipRgn16(HDC16,HRGN16);
INT       WINAPI SelectClipRgn(HDC,HRGN);
HGDIOBJ16   WINAPI SelectObject16(HDC16,HGDIOBJ16);
HGDIOBJ   WINAPI SelectObject(HDC,HGDIOBJ);
/* FIXME This is defined in user.spec !? */
HPALETTE16  WINAPI SelectPalette16(HDC16,HPALETTE16,BOOL16);
HPALETTE  WINAPI SelectPalette(HDC,HPALETTE,BOOL);
INT16       WINAPI SetAbortProc16(HDC16,SEGPTR);
INT       WINAPI SetAbortProc(HDC,ABORTPROC);
INT16       WINAPI SetArcDirection16(HDC16,INT16);
INT       WINAPI SetArcDirection(HDC,INT);
LONG        WINAPI SetBitmapBits16(HBITMAP16,LONG,LPCVOID);
LONG        WINAPI SetBitmapBits(HBITMAP,LONG,LPCVOID);
BOOL16      WINAPI SetBitmapDimensionEx16(HBITMAP16,INT16,INT16,LPSIZE16);
BOOL      WINAPI SetBitmapDimensionEx(HBITMAP,INT,INT,LPSIZE);
COLORREF    WINAPI SetBkColor16(HDC16,COLORREF);
COLORREF    WINAPI SetBkColor(HDC,COLORREF);
INT16       WINAPI SetBkMode16(HDC16,INT16);
INT       WINAPI SetBkMode(HDC,INT);
UINT16      WINAPI SetBoundsRect16(HDC16,const RECT16*,UINT16);
UINT      WINAPI SetBoundsRect(HDC,const RECT*,UINT);
UINT16      WINAPI SetDIBColorTable16(HDC16,UINT16,UINT16,RGBQUAD*);
UINT      WINAPI SetDIBColorTable(HDC,UINT,UINT,RGBQUAD*);
INT16       WINAPI SetDIBits16(HDC16,HBITMAP16,UINT16,UINT16,LPCVOID,const BITMAPINFO*,UINT16);
INT       WINAPI SetDIBits(HDC,HBITMAP,UINT,UINT,LPCVOID,const BITMAPINFO*,UINT);
INT16       WINAPI SetDIBitsToDevice16(HDC16,INT16,INT16,INT16,INT16,INT16,
                         INT16,UINT16,UINT16,LPCVOID,const BITMAPINFO*,UINT16);
INT       WINAPI SetDIBitsToDevice(HDC,INT,INT,DWORD,DWORD,INT,
                         INT,UINT,UINT,LPCVOID,const BITMAPINFO*,UINT);
INT16       WINAPI SetMapMode16(HDC16,INT16);
INT       WINAPI SetMapMode(HDC,INT);
DWORD       WINAPI SetMapperFlags16(HDC16,DWORD);
DWORD       WINAPI SetMapperFlags(HDC,DWORD);
UINT16      WINAPI SetPaletteEntries16(HPALETTE16,UINT16,UINT16,LPPALETTEENTRY);
UINT      WINAPI SetPaletteEntries(HPALETTE,UINT,UINT,LPPALETTEENTRY);
COLORREF    WINAPI SetPixel16(HDC16,INT16,INT16,COLORREF);
COLORREF    WINAPI SetPixel(HDC,INT,INT,COLORREF);
BOOL      WINAPI SetPixelV(HDC,INT,INT,COLORREF);
BOOL      WINAPI SetPixelFormat(HDC,int,const PIXELFORMATDESCRIPTOR*);
INT16       WINAPI SetPolyFillMode16(HDC16,INT16);
INT       WINAPI SetPolyFillMode(HDC,INT);
VOID        WINAPI SetRectRgn16(HRGN16,INT16,INT16,INT16,INT16);
VOID        WINAPI SetRectRgn(HRGN,INT,INT,INT,INT);
INT16       WINAPI SetRelAbs16(HDC16,INT16);
INT       WINAPI SetRelAbs(HDC,INT);
INT16       WINAPI SetROP216(HDC16,INT16);
INT       WINAPI SetROP2(HDC,INT);
INT16       WINAPI SetStretchBltMode16(HDC16,INT16);
INT       WINAPI SetStretchBltMode(HDC,INT);
UINT16      WINAPI SetSystemPaletteUse16(HDC16,UINT16);
UINT      WINAPI SetSystemPaletteUse(HDC,UINT);
UINT16      WINAPI SetTextAlign16(HDC16,UINT16);
UINT      WINAPI SetTextAlign(HDC,UINT);
INT16       WINAPI SetTextCharacterExtra16(HDC16,INT16);
INT       WINAPI SetTextCharacterExtra(HDC,INT);
COLORREF    WINAPI SetTextColor16(HDC16,COLORREF);
COLORREF    WINAPI SetTextColor(HDC,COLORREF);
INT16       WINAPI SetTextJustification16(HDC16,INT16,INT16);
BOOL      WINAPI SetTextJustification(HDC,INT,INT);
BOOL16      WINAPI SetViewportExtEx16(HDC16,INT16,INT16,LPSIZE16);
BOOL      WINAPI SetViewportExtEx(HDC,INT,INT,LPSIZE);
BOOL16      WINAPI SetViewportOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL      WINAPI SetViewportOrgEx(HDC,INT,INT,LPPOINT);
BOOL16      WINAPI SetWindowExtEx16(HDC16,INT16,INT16,LPSIZE16);
BOOL      WINAPI SetWindowExtEx(HDC,INT,INT,LPSIZE);
BOOL16      WINAPI SetWindowOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL      WINAPI SetWindowOrgEx(HDC,INT,INT,LPPOINT);
HENHMETAFILE WINAPI SetWinMetaFileBits(UINT,CONST BYTE*,HDC,CONST METAFILEPICT *);
INT16       WINAPI StartDoc16(HDC16,const DOCINFO16*);
INT       WINAPI StartDocA(HDC,const DOCINFOA*);
INT       WINAPI StartDocW(HDC,const DOCINFOW*);
#define     StartDoc WINELIB_NAME_AW(StartDoc)
INT16       WINAPI StartPage16(HDC16);
INT       WINAPI StartPage(HDC);
INT16       WINAPI EndPage16(HDC16);
INT       WINAPI EndPage(HDC);
BOOL16      WINAPI StretchBlt16(HDC16,INT16,INT16,INT16,INT16,HDC16,INT16,
                                INT16,INT16,INT16,DWORD);
BOOL      WINAPI StretchBlt(HDC,INT,INT,INT,INT,HDC,INT,
                                INT,INT,INT,DWORD);
INT16       WINAPI StretchDIBits16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,
                       INT16,INT16,const VOID*,const BITMAPINFO*,UINT16,DWORD);
INT       WINAPI StretchDIBits(HDC,INT,INT,INT,INT,INT,INT,
                       INT,INT,const VOID*,const BITMAPINFO*,UINT,DWORD);
BOOL16      WINAPI StrokeAndFillPath16(HDC16);
BOOL      WINAPI StrokeAndFillPath(HDC);
BOOL16      WINAPI StrokePath16(HDC16);
BOOL      WINAPI StrokePath(HDC);
BOOL      WINAPI SwapBuffers(HDC);
BOOL16      WINAPI TextOut16(HDC16,INT16,INT16,LPCSTR,INT16);
BOOL      WINAPI TextOutA(HDC,INT,INT,LPCSTR,INT);
BOOL      WINAPI TextOutW(HDC,INT,INT,LPCWSTR,INT);
#define     TextOut WINELIB_NAME_AW(TextOut)
BOOL16      WINAPI UnrealizeObject16(HGDIOBJ16);
BOOL      WINAPI UnrealizeObject(HGDIOBJ);
INT16       WINAPI UpdateColors16(HDC16);
BOOL      WINAPI UpdateColors(HDC);
BOOL16      WINAPI WidenPath16(HDC16);
BOOL      WINAPI WidenPath(HDC);

#endif /* __WINE_WINGDI_H */
