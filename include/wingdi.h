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
    INT32  iKernAmount;
} KERNINGPAIR32, *LPKERNINGPAIR32;

DECL_WINELIB_TYPE(KERNINGPAIR)
DECL_WINELIB_TYPE(LPKERNINGPAIR)

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
    HDC32   hdc;
    BOOL32  fErase;
    RECT32  rcPaint;
    BOOL32  fRestore;
    BOOL32  fIncUpdate;
    BYTE    rgbReserved[32];
} PAINTSTRUCT32, *LPPAINTSTRUCT32;

DECL_WINELIB_TYPE(PAINTSTRUCT)
DECL_WINELIB_TYPE(LPPAINTSTRUCT)


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
    INT32  bmType;
    INT32  bmWidth;
    INT32  bmHeight;
    INT32  bmWidthBytes;
    WORD   bmPlanes;
    WORD   bmBitsPixel;
    LPVOID bmBits WINE_PACKED;
} BITMAP32, *LPBITMAP32;

DECL_WINELIB_TYPE(BITMAP)
DECL_WINELIB_TYPE(LPBITMAP)

  /* Brushes */

typedef struct
{ 
    UINT16     lbStyle;
    COLORREF   lbColor WINE_PACKED;
    INT16      lbHatch;
} LOGBRUSH16, *LPLOGBRUSH16;

typedef struct
{ 
    UINT32     lbStyle;
    COLORREF   lbColor;
    INT32      lbHatch;
} LOGBRUSH32, *LPLOGBRUSH32;

DECL_WINELIB_TYPE(LOGBRUSH)
DECL_WINELIB_TYPE(LPLOGBRUSH)

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
    INT32  lfHeight;
    INT32  lfWidth;
    INT32  lfEscapement;
    INT32  lfOrientation;
    INT32  lfWeight;
    BYTE   lfItalic;
    BYTE   lfUnderline;
    BYTE   lfStrikeOut;
    BYTE   lfCharSet;
    BYTE   lfOutPrecision;
    BYTE   lfClipPrecision;
    BYTE   lfQuality;
    BYTE   lfPitchAndFamily;
    CHAR   lfFaceName[LF_FACESIZE];
} LOGFONT32A, *LPLOGFONT32A;

typedef struct
{
    INT32  lfHeight;
    INT32  lfWidth;
    INT32  lfEscapement;
    INT32  lfOrientation;
    INT32  lfWeight;
    BYTE   lfItalic;
    BYTE   lfUnderline;
    BYTE   lfStrikeOut;
    BYTE   lfCharSet;
    BYTE   lfOutPrecision;
    BYTE   lfClipPrecision;
    BYTE   lfQuality;
    BYTE   lfPitchAndFamily;
    WCHAR  lfFaceName[LF_FACESIZE];
} LOGFONT32W, *LPLOGFONT32W;

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
  LOGFONT32A elfLogFont;
  BYTE       elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  BYTE       elfStyle[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONT32A, *LPENUMLOGFONT32A;

typedef struct
{
  LOGFONT32W elfLogFont;
  WCHAR      elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  WCHAR      elfStyle[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONT32W, *LPENUMLOGFONT32W;

typedef struct
{
  LOGFONT16  elfLogFont;
  BYTE       elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  BYTE       elfStyle[LF_FACESIZE] WINE_PACKED;
  BYTE       elfScript[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONTEX16, *LPENUMLOGFONTEX16;

typedef struct
{
  LOGFONT32A elfLogFont;
  BYTE       elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  BYTE       elfStyle[LF_FACESIZE] WINE_PACKED;
  BYTE       elfScript[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONTEX32A,*LPENUMLOGFONTEX32A;

typedef struct
{
  LOGFONT32W elfLogFont;
  WCHAR      elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  WCHAR      elfStyle[LF_FACESIZE] WINE_PACKED;
  WCHAR      elfScript[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONTEX32W,*LPENUMLOGFONTEX32W;

DECL_WINELIB_TYPE_AW(ENUMLOGFONT)
DECL_WINELIB_TYPE_AW(LPENUMLOGFONT)
DECL_WINELIB_TYPE_AW(LPENUMLOGFONTEX)

typedef struct
{
  DWORD fsUsb[4];
  DWORD fsCsb[2];
} FONTSIGNATURE,*LPFONTSIGNATURE;

typedef struct 
{
  UINT32	ciCharset;
  UINT32	ciACP;
  FONTSIGNATURE	fs;
} CHARSETINFO,*LPCHARSETINFO;

/* Flags for ModifyWorldTransform */
#define MWT_IDENTITY      1
#define MWT_LEFTMULTIPLY  2
#define MWT_RIGHTMULTIPLY 3

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
    CHAR  dfCharSet;
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
#define ANSI_CHARSET	      (CHAR)0   /* CP1252, ansi-0, iso8859-{1,15} */
#define DEFAULT_CHARSET       (CHAR)1
#define SYMBOL_CHARSET	      (CHAR)2
#define SHIFTJIS_CHARSET      (CHAR)128
#define HANGEUL_CHARSET       (CHAR)129 /* ksc5601.1987-0 */
#define GB2312_CHARSET        (CHAR)134 /* gb2312.1980-0 */
#define CHINESEBIG5_CHARSET   (CHAR)136 /* big5.et-0 */
#define GREEK_CHARSET         (CHAR)161	/* CP1253 */
#define TURKISH_CHARSET       (CHAR)162	/* CP1254, -iso8859-9 */
#define HEBREW_CHARSET        (CHAR)177	/* CP1255, -iso8859-8 */
#define ARABIC_CHARSET        (CHAR)178	/* CP1256, -iso8859-6 */
#define BALTIC_CHARSET        (CHAR)186	/* CP1257, -iso8859-10 */
#define RUSSIAN_CHARSET       (CHAR)204	/* CP1251, -iso8859-5 */
#define EE_CHARSET	      (CHAR)238	/* CP1250, -iso8859-2 */
#define OEM_CHARSET	      (CHAR)255
/* I don't know if the values of *_CHARSET macros are defined in Windows
 * or if we can choose them as we want. -- srtxg
 */
#define THAI_CHARSET	      (CHAR)239 /* iso8859-11, tis620 */
#define VISCII_CHARSET        (CHAR)240 /* viscii1.1-1 */
#define TCVN_CHARSET          (CHAR)241 /* tcvn-0 */
#define KOI8_CHARSET          (CHAR)242 /* koi8-{r,u,ru} */
#define ISO3_CHARSET          (CHAR)243 /* iso8859-3 */
#define ISO4_CHARSET          (CHAR)244 /* iso8859-4 */

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
    INT32     tmHeight;
    INT32     tmAscent;
    INT32     tmDescent;
    INT32     tmInternalLeading;
    INT32     tmExternalLeading;
    INT32     tmAveCharWidth;
    INT32     tmMaxCharWidth;
    INT32     tmWeight;
    INT32     tmOverhang;
    INT32     tmDigitizedAspectX;
    INT32     tmDigitizedAspectY;
    BYTE      tmFirstChar;
    BYTE      tmLastChar;
    BYTE      tmDefaultChar;
    BYTE      tmBreakChar;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
} TEXTMETRIC32A, *LPTEXTMETRIC32A;

typedef struct
{
    INT32     tmHeight;
    INT32     tmAscent;
    INT32     tmDescent;
    INT32     tmInternalLeading;
    INT32     tmExternalLeading;
    INT32     tmAveCharWidth;
    INT32     tmMaxCharWidth;
    INT32     tmWeight;
    INT32     tmOverhang;
    INT32     tmDigitizedAspectX;
    INT32     tmDigitizedAspectY;
    WCHAR     tmFirstChar;
    WCHAR     tmLastChar;
    WCHAR     tmDefaultChar;
    WCHAR     tmBreakChar;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
} TEXTMETRIC32W, *LPTEXTMETRIC32W;

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


typedef struct _OUTLINETEXTMETRIC32A
{
    UINT32          otmSize;
    TEXTMETRIC32A   otmTextMetrics;
    BYTE            otmFilter;
    PANOSE          otmPanoseNumber;
    UINT32          otmfsSelection;
    UINT32          otmfsType;
    INT32           otmsCharSlopeRise;
    INT32           otmsCharSlopeRun;
    INT32           otmItalicAngle;
    UINT32          otmEMSquare;
    INT32           otmAscent;
    INT32           otmDescent;
    UINT32          otmLineGap;
    UINT32          otmsCapEmHeight;
    UINT32          otmsXHeight;
    RECT32          otmrcFontBox;
    INT32           otmMacAscent;
    INT32           otmMacDescent;
    UINT32          otmMacLineGap;
    UINT32          otmusMinimumPPEM;
    POINT32         otmptSubscriptSize;
    POINT32         otmptSubscriptOffset;
    POINT32         otmptSuperscriptSize;
    POINT32         otmptSuperscriptOffset;
    UINT32          otmsStrikeoutSize;
    INT32           otmsStrikeoutPosition;
    INT32           otmsUnderscoreSize;
    INT32           otmsUnderscorePosition;
    LPSTR           otmpFamilyName;
    LPSTR           otmpFaceName;
    LPSTR           otmpStyleName;
    LPSTR           otmpFullName;
} OUTLINETEXTMETRIC32A, *LPOUTLINETEXTMETRIC32A;

typedef struct _OUTLINETEXTMETRIC32W
{
    UINT32          otmSize;
    TEXTMETRIC32W   otmTextMetrics;
    BYTE            otmFilter;
    PANOSE          otmPanoseNumber;
    UINT32          otmfsSelection;
    UINT32          otmfsType;
    INT32           otmsCharSlopeRise;
    INT32           otmsCharSlopeRun;
    INT32           otmItalicAngle;
    UINT32          otmEMSquare;
    INT32           otmAscent;
    INT32           otmDescent;
    UINT32          otmLineGap;
    UINT32          otmsCapEmHeight;
    UINT32          otmsXHeight;
    RECT32          otmrcFontBox;
    INT32           otmMacAscent;
    INT32           otmMacDescent;
    UINT32          otmMacLineGap;
    UINT32          otmusMinimumPPEM;
    POINT32         otmptSubscriptSize;
    POINT32         otmptSubscriptOffset;
    POINT32         otmptSuperscriptSize;
    POINT32         otmptSuperscriptOffset;
    UINT32          otmsStrikeoutSize;
    INT32           otmsStrikeoutPosition;
    INT32           otmsUnderscoreSize;
    INT32           otmsUnderscorePosition;
    LPSTR           otmpFamilyName;
    LPSTR           otmpFaceName;
    LPSTR           otmpStyleName;
    LPSTR           otmpFullName;
} OUTLINETEXTMETRIC32W, *LPOUTLINETEXTMETRIC32W;

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
    INT32           otmsUnderscorePosition;
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
    INT32     tmHeight;
    INT32     tmAscent;
    INT32     tmDescent;
    INT32     tmInternalLeading;
    INT32     tmExternalLeading;
    INT32     tmAveCharWidth;
    INT32     tmMaxCharWidth;
    INT32     tmWeight;
    INT32     tmOverhang;
    INT32     tmDigitizedAspectX;
    INT32     tmDigitizedAspectY;
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
    UINT32    ntmSizeEM;
    UINT32    ntmCellHeight;
    UINT32    ntmAvgWidth;
} NEWTEXTMETRIC32A, *LPNEWTEXTMETRIC32A;

typedef struct
{
    INT32     tmHeight;
    INT32     tmAscent;
    INT32     tmDescent;
    INT32     tmInternalLeading;
    INT32     tmExternalLeading;
    INT32     tmAveCharWidth;
    INT32     tmMaxCharWidth;
    INT32     tmWeight;
    INT32     tmOverhang;
    INT32     tmDigitizedAspectX;
    INT32     tmDigitizedAspectY;
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
    UINT32    ntmSizeEM;
    UINT32    ntmCellHeight;
    UINT32    ntmAvgWidth;
} NEWTEXTMETRIC32W, *LPNEWTEXTMETRIC32W;

DECL_WINELIB_TYPE_AW(NEWTEXTMETRIC)
DECL_WINELIB_TYPE_AW(LPNEWTEXTMETRIC)

typedef struct
{
    NEWTEXTMETRIC16	ntmetm;
    FONTSIGNATURE       ntmeFontSignature;
} NEWTEXTMETRICEX16,*LPNEWTEXTMETRICEX16;

typedef struct
{
    NEWTEXTMETRIC32A	ntmetm;
    FONTSIGNATURE       ntmeFontSignature;
} NEWTEXTMETRICEX32A,*LPNEWTEXTMETRICEX32A;

typedef struct
{
    NEWTEXTMETRIC32W	ntmetm;
    FONTSIGNATURE       ntmeFontSignature;
} NEWTEXTMETRICEX32W,*LPNEWTEXTMETRICEX32W;

DECL_WINELIB_TYPE_AW(NEWTEXTMETRICEX)
DECL_WINELIB_TYPE_AW(LPNEWTEXTMETRICEX)


typedef INT16 (CALLBACK *FONTENUMPROC16)(SEGPTR,SEGPTR,UINT16,LPARAM);
typedef INT32 (CALLBACK *FONTENUMPROC32A)(LPENUMLOGFONT32A,LPNEWTEXTMETRIC32A,
                                          UINT32,LPARAM);
typedef INT32 (CALLBACK *FONTENUMPROC32W)(LPENUMLOGFONT32W,LPNEWTEXTMETRIC32W,
                                          UINT32,LPARAM);
DECL_WINELIB_TYPE_AW(FONTENUMPROC)

typedef INT16 (CALLBACK *FONTENUMPROCEX16)(SEGPTR,SEGPTR,UINT16,LPARAM);
typedef INT32 (CALLBACK *FONTENUMPROCEX32A)(LPENUMLOGFONTEX32A,LPNEWTEXTMETRICEX32A,UINT32,LPARAM);
typedef INT32 (CALLBACK *FONTENUMPROCEX32W)(LPENUMLOGFONTEX32W,LPNEWTEXTMETRICEX32W,UINT32,LPARAM);
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
    UINT32	gmBlackBoxX;
    UINT32	gmBlackBoxY;
    POINT32	gmptGlyphOrigin;
    INT16	gmCellIncX;
    INT16	gmCellIncY;
} GLYPHMETRICS32, *LPGLYPHMETRICS32;

DECL_WINELIB_TYPE(GLYPHMETRICS)
DECL_WINELIB_TYPE(LPGLYPHMETRICS)

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
    INT32   abcA;
    UINT32  abcB;
    INT32   abcC;
} ABC32, *LPABC32;

DECL_WINELIB_TYPE(ABC)
DECL_WINELIB_TYPE(LPABC)

  /* for GetCharacterPlacement () */
typedef struct tagGCP_RESULTS32A
{
    DWORD  lStructSize;
    LPSTR  lpOutString;
    UINT32 *lpOrder;
    INT32  *lpDx;
    INT32  *lpCaretPos;
    LPSTR  lpClass;
    UINT32 *lpGlyphs;
    UINT32 nGlyphs;
    UINT32 nMaxFit;
} GCP_RESULTS32A;

typedef struct tagGCP_RESULTS32W
{
    DWORD  lStructSize;
    LPWSTR lpOutString;
    UINT32 *lpOrder;
    INT32  *lpDx;
    INT32  *lpCaretPos;
    LPWSTR lpClass;
    UINT32 *lpGlyphs;
    UINT32 nGlyphs;
    UINT32 nMaxFit;
} GCP_RESULTS32W;

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
    UINT32   lopnStyle; 
    POINT32  lopnWidth WINE_PACKED;
    COLORREF lopnColor WINE_PACKED;
} LOGPEN32, *LPLOGPEN32;

DECL_WINELIB_TYPE(LOGPEN)
DECL_WINELIB_TYPE(LPLOGPEN)

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
	BITMAP32		dsBm;
	BITMAPINFOHEADER	dsBmih;
	DWORD			dsBitfields[3];
	HANDLE32		dshSection;
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
    WORD       rdParam[1];
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
    HGDIOBJ32 objectHandle[1];
} HANDLETABLE32, *LPHANDLETABLE32;

DECL_WINELIB_TYPE(HANDLETABLE)
DECL_WINELIB_TYPE(LPHANDLETABLE)

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
    INT32        mm;
    INT32        xExt;
    INT32        yExt;
    HMETAFILE32  hMF;
} METAFILEPICT32, *LPMETAFILEPICT32;

DECL_WINELIB_TYPE(METAFILEPICT)
DECL_WINELIB_TYPE(LPMETAFILEPICT)

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
typedef INT32 (CALLBACK *MFENUMPROC32)(HDC32,HANDLETABLE32*,METARECORD*,
                                       INT32,LPARAM);
DECL_WINELIB_TYPE(MFENUMPROC)

/* enhanced metafile structures and functions */

/* note that ENHMETAHEADER is just a particular kind of ENHMETARECORD,
   ie. the header is just the first record in the metafile */
typedef struct {
    DWORD iType; 
    DWORD nSize; 
    RECT32 rclBounds; 
    RECT32 rclFrame; 
    DWORD dSignature; 
    DWORD nVersion; 
    DWORD nBytes; 
    DWORD nRecords; 
    WORD  nHandles; 
    WORD  sReserved; 
    DWORD nDescription; 
    DWORD offDescription; 
    DWORD nPalEntries; 
    SIZE32 szlDevice; 
    SIZE32 szlMillimeters;
    DWORD cbPixelFormat;
    DWORD offPixelFormat;
    DWORD bOpenGL;
} ENHMETAHEADER, *LPENHMETAHEADER; 

typedef struct {
    DWORD iType; 
    DWORD nSize; 
    DWORD dParm[1]; 
} ENHMETARECORD, *LPENHMETARECORD; 

typedef INT32 (CALLBACK *ENHMFENUMPROC32)(HDC32, LPHANDLETABLE32, 
					  LPENHMETARECORD, INT32, LPVOID);

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
} DEVMODE32A, *LPDEVMODE32A;

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
} DEVMODE32W, *LPDEVMODE32W;

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
    INT32    cbSize;
    LPCSTR   lpszDocName;
    LPCSTR   lpszOutput;
    LPCSTR   lpszDatatype;
    DWORD    fwType;
} DOCINFO32A, *LPDOCINFO32A;

typedef struct 
{
    INT32    cbSize;
    LPCWSTR  lpszDocName;
    LPCWSTR  lpszOutput;
    LPCWSTR  lpszDatatype;
    DWORD    fwType;
} DOCINFO32W, *LPDOCINFO32W;

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
	UINT32		cbSize;
	INT32		iBorderWidth;
	INT32		iScrollWidth;
	INT32		iScrollHeight;
	INT32		iCaptionWidth;
	INT32		iCaptionHeight;
	LOGFONT32A	lfCaptionFont;
	INT32		iSmCaptionWidth;
	INT32		iSmCaptionHeight;
	LOGFONT32A	lfSmCaptionFont;
	INT32		iMenuWidth;
	INT32		iMenuHeight;
	LOGFONT32A	lfMenuFont;
	LOGFONT32A	lfStatusFont;
	LOGFONT32A	lfMessageFont;
} NONCLIENTMETRICS32A,*LPNONCLIENTMETRICS32A;

typedef struct {
	UINT32		cbSize;
	INT32		iBorderWidth;
	INT32		iScrollWidth;
	INT32		iScrollHeight;
	INT32		iCaptionWidth;
	INT32		iCaptionHeight;
	LOGFONT32W	lfCaptionFont;
	INT32		iSmCaptionWidth;
	INT32		iSmCaptionHeight;
	LOGFONT32W	lfSmCaptionFont;
	INT32		iMenuWidth;
	INT32		iMenuHeight;
	LOGFONT32W	lfMenuFont;
	LOGFONT32W	lfStatusFont;
	LOGFONT32W	lfMessageFont;
} NONCLIENTMETRICS32W,*LPNONCLIENTMETRICS32W;

DECL_WINELIB_TYPE_AW(NONCLIENTMETRICS)
DECL_WINELIB_TYPE_AW(LPNONCLIENTMETRICS)

#define	RDH_RECTANGLES  1

typedef struct _RGNDATAHEADER {
    DWORD	dwSize;
    DWORD	iType;
    DWORD	nCount;
    DWORD	nRgnSize;
    RECT32	rcBound;
} RGNDATAHEADER,*LPRGNDATAHEADER;

typedef struct _RGNDATA {
    RGNDATAHEADER	rdh;
    char		Buffer[1];
} RGNDATA,*PRGNDATA,*LPRGNDATA;

#pragma pack(4)

/* Declarations for functions that exist only in Win32 */

BOOL32      WINAPI AngleArc32(HDC32, INT32, INT32, DWORD, FLOAT, FLOAT);
BOOL32      WINAPI ArcTo32(HDC32, INT32, INT32, INT32, INT32, INT32, INT32, INT32, INT32); 
HENHMETAFILE32 WINAPI CloseEnhMetaFile32(HDC32);
HBRUSH32    WINAPI CreateDIBPatternBrushPt(BITMAPINFO*,UINT32);
HDC32       WINAPI CreateEnhMetaFile32A(HDC32,LPCSTR,const RECT32*,LPCSTR);
HDC32       WINAPI CreateEnhMetaFile32W(HDC32,LPCWSTR,const RECT32*,LPCWSTR);
#define     CreateEnhMetaFile WINELIB_NAME_AW(CreateEnhMetaFile)
INT32       WINAPI DrawEscape32(HDC32,INT32,INT32,LPCSTR);
INT16       WINAPI ExcludeVisRect(HDC16,INT16,INT16,INT16,INT16);
BOOL16      WINAPI FastWindowFrame(HDC16,const RECT16*,INT16,INT16,DWORD);
UINT16      WINAPI GDIRealizePalette(HDC16);
HPALETTE16  WINAPI GDISelectPalette(HDC16,HPALETTE16,WORD);
BOOL32      WINAPI GdiComment32(HDC32,UINT32,const BYTE *);
DWORD       WINAPI GetBitmapDimension(HBITMAP16);
DWORD       WINAPI GetBrushOrg(HDC16);
BOOL32      WINAPI GetCharABCWidthsFloat32A(HDC32,UINT32,UINT32,LPABCFLOAT);
BOOL32      WINAPI GetCharABCWidthsFloat32W(HDC32,UINT32,UINT32,LPABCFLOAT);
#define     GetCharABCWidthsFloat WINELIB_NAME_AW(GetCharABCWidthsFloat)
BOOL32      WINAPI GetCharWidthFloat32A(HDC32,UINT32,UINT32,PFLOAT);
BOOL32      WINAPI GetCharWidthFloat32W(HDC32,UINT32,UINT32,PFLOAT);
#define     GetCharWidthFloat WINELIB_NAME_AW(GetCharWidthFloat)
BOOL32      WINAPI GetColorAdjustment32(HDC32, LPCOLORADJUSTMENT);
HFONT16     WINAPI GetCurLogFont(HDC16);
DWORD       WINAPI GetCurrentPosition(HDC16);
DWORD       WINAPI GetDCHook(HDC16,FARPROC16*);
DWORD       WINAPI GetDCOrg(HDC16);
HDC16       WINAPI GetDCState(HDC16);
INT16       WINAPI GetEnvironment(LPCSTR,LPDEVMODE16,UINT16);
HGLOBAL16   WINAPI GetMetaFileBits(HMETAFILE16);
BOOL32      WINAPI GetMiterLimit(HDC32, PFLOAT);
DWORD       WINAPI GetTextExtent(HDC16,LPCSTR,INT16);
DWORD       WINAPI GetViewportExt(HDC16);
DWORD       WINAPI GetViewportOrg(HDC16);
DWORD       WINAPI GetWindowExt(HDC16);
DWORD       WINAPI GetWindowOrg(HDC16);
HRGN16      WINAPI InquireVisRgn(HDC16);
INT16       WINAPI IntersectVisRect(HDC16,INT16,INT16,INT16,INT16);
BOOL16      WINAPI IsDCCurrentPalette(HDC16);
BOOL16      WINAPI IsGDIObject(HGDIOBJ16);
BOOL16      WINAPI IsValidMetaFile(HMETAFILE16);
BOOL32      WINAPI MaskBlt(HDC32,INT32,INT32,INT32,INT32,HDC32,INT32,INT32,HBITMAP32,INT32,INT32,DWORD);
DWORD       WINAPI MoveTo(HDC16,INT16,INT16);
DWORD       WINAPI OffsetViewportOrg(HDC16,INT16,INT16);
INT16       WINAPI OffsetVisRgn(HDC16,INT16,INT16);
DWORD       WINAPI OffsetWindowOrg(HDC16,INT16,INT16);
BOOL32      WINAPI PlgBlt(HDC32,const POINT32*,HDC32,INT32,INT32,INT32,INT32,HBITMAP32,INT32,INT32);
BOOL32      WINAPI PolyDraw32(HDC32,const POINT32*,const BYTE*,DWORD);
UINT16      WINAPI RealizeDefaultPalette(HDC16);
INT16       WINAPI RestoreVisRgn(HDC16);
HRGN16      WINAPI SaveVisRgn(HDC16);
DWORD       WINAPI ScaleViewportExt(HDC16,INT16,INT16,INT16,INT16);
DWORD       WINAPI ScaleWindowExt(HDC16,INT16,INT16,INT16,INT16);
INT16       WINAPI SelectVisRgn(HDC16,HRGN16);
DWORD       WINAPI SetBitmapDimension(HBITMAP16,INT16,INT16);
DWORD       WINAPI SetBrushOrg(HDC16,INT16,INT16);
BOOL32      WINAPI SetColorAdjustment32(HDC32,const COLORADJUSTMENT*);
BOOL16      WINAPI SetDCHook(HDC16,FARPROC16,DWORD);
DWORD       WINAPI SetDCOrg(HDC16,INT16,INT16);
VOID        WINAPI SetDCState(HDC16,HDC16);
INT16       WINAPI SetEnvironment(LPCSTR,LPDEVMODE16,UINT16);
WORD        WINAPI SetHookFlags(HDC16,WORD);
HMETAFILE16 WINAPI SetMetaFileBits(HGLOBAL16);
BOOL32      WINAPI SetMiterLimit(HDC32, FLOAT, PFLOAT);
DWORD       WINAPI SetViewportExt(HDC16,INT16,INT16);
DWORD       WINAPI SetViewportOrg(HDC16,INT16,INT16);
DWORD       WINAPI SetWindowExt(HDC16,INT16,INT16);
DWORD       WINAPI SetWindowOrg(HDC16,INT16,INT16);
BOOL32      WINAPI CombineTransform(LPXFORM,const XFORM *,const XFORM *);
HENHMETAFILE32 WINAPI CopyEnhMetaFile32A(HENHMETAFILE32,LPCSTR);
HENHMETAFILE32 WINAPI CopyEnhMetaFile32W(HENHMETAFILE32,LPCWSTR);
#define     CopyEnhMetaFile WINELIB_NAME_AW(CopyEnhMetaFile)
HPALETTE32  WINAPI CreateHalftonePalette(HDC32);
BOOL32      WINAPI DeleteEnhMetaFile(HENHMETAFILE32);
INT32       WINAPI ExtSelectClipRgn(HDC32,HRGN32,INT32);
HRGN32      WINAPI ExtCreateRegion(const XFORM*,DWORD,const RGNDATA*);
INT32       WINAPI ExtEscape32(HDC32,INT32,INT32,LPCSTR,INT32,LPSTR);
BOOL32      WINAPI FixBrushOrgEx(HDC32,INT32,INT32,LPPOINT32);
HANDLE32    WINAPI GetCurrentObject(HDC32,UINT32);
BOOL32      WINAPI GetDCOrgEx(HDC32,LPPOINT32);
HENHMETAFILE32 WINAPI GetEnhMetaFile32A(LPCSTR);
HENHMETAFILE32 WINAPI GetEnhMetaFile32W(LPCWSTR);
#define     GetEnhMetaFile WINELIB_NAME_AW(GetEnhMetaFile)
UINT32      WINAPI GetEnhMetaFileBits(HENHMETAFILE32,UINT32,LPBYTE);
UINT32      WINAPI GetEnhMetaFileHeader(HENHMETAFILE32,UINT32,LPENHMETAHEADER);
UINT32      WINAPI GetEnhMetaFilePaletteEntries(HENHMETAFILE32,UINT32,LPPALETTEENTRY);
INT32       WINAPI GetGraphicsMode(HDC32);
UINT32      WINAPI GetMetaFileBitsEx(HMETAFILE32,UINT32,LPVOID);
DWORD       WINAPI GetObjectType(HANDLE32);
UINT32      WINAPI GetTextCharsetInfo(HDC32,LPFONTSIGNATURE,DWORD);
BOOL32      WINAPI GetTextExtentExPoint32A(HDC32,LPCSTR,INT32,INT32,
                                           LPINT32,LPINT32,LPSIZE32);
BOOL32      WINAPI GetTextExtentExPoint32W(HDC32,LPCWSTR,INT32,INT32,
                                           LPINT32,LPINT32,LPSIZE32);
#define     GetTextExtentExPoint WINELIB_NAME_AW(GetTextExtentExPoint)
BOOL32      WINAPI GetWorldTransform(HDC32,LPXFORM);
BOOL32      WINAPI ModifyWorldTransform(HDC32,const XFORM *, DWORD);
BOOL32      WINAPI PlayEnhMetaFile(HDC32,HENHMETAFILE32,const RECT32*);
BOOL32      WINAPI PlayEnhMetaFileRecord(HDC32,LPHANDLETABLE32,const ENHMETARECORD*,UINT32);
BOOL32      WINAPI PolyPolyline32(HDC32,const POINT32*,const DWORD*,DWORD);
BOOL32      WINAPI SetBrushOrgEx(HDC32,INT32,INT32,LPPOINT32);
HENHMETAFILE32 WINAPI SetEnhMetaFileBits(UINT32,const BYTE *);
INT32       WINAPI SetGraphicsMode(HDC32,INT32);
HMETAFILE32 WINAPI SetMetaFileBitsEx(UINT32,const BYTE*);
BOOL32      WINAPI SetWorldTransform(HDC32,const XFORM*);
BOOL32      WINAPI TranslateCharsetInfo(LPDWORD,LPCHARSETINFO,DWORD);

/* Declarations for functions that change between Win16 and Win32 */

INT16       WINAPI AbortDoc16(HDC16);
INT32       WINAPI AbortDoc32(HDC32);
#define     AbortDoc WINELIB_NAME(AbortDoc)
BOOL16      WINAPI AbortPath16(HDC16);
BOOL32      WINAPI AbortPath32(HDC32);
#define     AbortPath WINELIB_NAME(AbortPath)
INT16       WINAPI AddFontResource16(LPCSTR);
INT32       WINAPI AddFontResource32A(LPCSTR);
INT32       WINAPI AddFontResource32W(LPCWSTR);
#define     AddFontResource WINELIB_NAME_AW(AddFontResource)
void        WINAPI AnimatePalette16(HPALETTE16,UINT16,UINT16,const PALETTEENTRY*);
BOOL32      WINAPI AnimatePalette32(HPALETTE32,UINT32,UINT32,const PALETTEENTRY*);
#define     AnimatePalette WINELIB_NAME(AnimatePalette)
BOOL16      WINAPI Arc16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL32      WINAPI Arc32(HDC32,INT32,INT32,INT32,INT32,INT32,INT32,INT32,INT32);
#define     Arc WINELIB_NAME(Arc)
BOOL16      WINAPI BeginPath16(HDC16);
BOOL32      WINAPI BeginPath32(HDC32);
#define     BeginPath WINELIB_NAME(BeginPath)
BOOL16      WINAPI BitBlt16(HDC16,INT16,INT16,INT16,INT16,HDC16,INT16,INT16,DWORD);
BOOL32      WINAPI BitBlt32(HDC32,INT32,INT32,INT32,INT32,HDC32,INT32,INT32,DWORD);
#define     BitBlt WINELIB_NAME(BitBlt)
INT32       WINAPI ChoosePixelFormat(HDC32,const PIXELFORMATDESCRIPTOR*);
BOOL16      WINAPI Chord16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL32      WINAPI Chord32(HDC32,INT32,INT32,INT32,INT32,INT32,INT32,INT32,INT32);
#define     Chord WINELIB_NAME(Chord)
BOOL16      WINAPI CloseFigure16(HDC16);
BOOL32      WINAPI CloseFigure32(HDC32);
#define     CloseFigure WINELIB_NAME(CloseFigure)
HMETAFILE16 WINAPI CloseMetaFile16(HDC16);
HMETAFILE32 WINAPI CloseMetaFile32(HDC32);
#define     CloseMetaFile WINELIB_NAME(CloseMetaFile)
INT16       WINAPI CombineRgn16(HRGN16,HRGN16,HRGN16,INT16);
INT32       WINAPI CombineRgn32(HRGN32,HRGN32,HRGN32,INT32);
#define     CombineRgn WINELIB_NAME(CombineRgn)
HMETAFILE16 WINAPI CopyMetaFile16(HMETAFILE16,LPCSTR);
HMETAFILE32 WINAPI CopyMetaFile32A(HMETAFILE32,LPCSTR);
HMETAFILE32 WINAPI CopyMetaFile32W(HMETAFILE32,LPCWSTR);
#define     CopyMetaFile WINELIB_NAME_AW(CopyMetaFile)
HBITMAP16   WINAPI CreateBitmap16(INT16,INT16,UINT16,UINT16,LPCVOID);
HBITMAP32   WINAPI CreateBitmap32(INT32,INT32,UINT32,UINT32,LPCVOID);
#define     CreateBitmap WINELIB_NAME(CreateBitmap)
HBITMAP16   WINAPI CreateBitmapIndirect16(const BITMAP16*);
HBITMAP32   WINAPI CreateBitmapIndirect32(const BITMAP32*);
#define     CreateBitmapIndirect WINELIB_NAME(CreateBitmapIndirect)
HBRUSH16    WINAPI CreateBrushIndirect16(const LOGBRUSH16*);
HBRUSH32    WINAPI CreateBrushIndirect32(const LOGBRUSH32*);
#define     CreateBrushIndirect WINELIB_NAME(CreateBrushIndirect)
HBITMAP16   WINAPI CreateCompatibleBitmap16(HDC16,INT16,INT16);
HBITMAP32   WINAPI CreateCompatibleBitmap32(HDC32,INT32,INT32);
#define     CreateCompatibleBitmap WINELIB_NAME(CreateCompatibleBitmap)
HDC16       WINAPI CreateCompatibleDC16(HDC16);
HDC32       WINAPI CreateCompatibleDC32(HDC32);
#define     CreateCompatibleDC WINELIB_NAME(CreateCompatibleDC)
HDC16       WINAPI CreateDC16(LPCSTR,LPCSTR,LPCSTR,const DEVMODE16*);
HDC32       WINAPI CreateDC32A(LPCSTR,LPCSTR,LPCSTR,const DEVMODE32A*);
HDC32       WINAPI CreateDC32W(LPCWSTR,LPCWSTR,LPCWSTR,const DEVMODE32W*);
#define     CreateDC WINELIB_NAME_AW(CreateDC)
HBITMAP16   WINAPI CreateDIBitmap16(HDC16,const BITMAPINFOHEADER*,DWORD,
                                    LPCVOID,const BITMAPINFO*,UINT16);
HBITMAP32   WINAPI CreateDIBitmap32(HDC32,const BITMAPINFOHEADER*,DWORD,
                                    LPCVOID,const BITMAPINFO*,UINT32);
#define     CreateDIBitmap WINELIB_NAME(CreateDIBitmap)
HBRUSH16    WINAPI CreateDIBPatternBrush16(HGLOBAL16,UINT16);
HBRUSH32    WINAPI CreateDIBPatternBrush32(HGLOBAL32,UINT32);
#define     CreateDIBPatternBrush WINELIB_NAME(CreateDIBPatternBrush)
HBITMAP16   WINAPI CreateDIBSection16 (HDC16, BITMAPINFO *, UINT16,
				       LPVOID **, HANDLE32, DWORD offset);
HBITMAP32   WINAPI CreateDIBSection32 (HDC32, BITMAPINFO *, UINT32,
				       LPVOID **, HANDLE32, DWORD offset);
#define     CreateDIBSection WINELIB_NAME(CreateDIBSection)
HBITMAP16   WINAPI CreateDiscardableBitmap16(HDC16,INT16,INT16);
HBITMAP32   WINAPI CreateDiscardableBitmap32(HDC32,INT32,INT32);
#define     CreateDiscardableBitmap WINELIB_NAME(CreateDiscardableBitmap)
HRGN16      WINAPI CreateEllipticRgn16(INT16,INT16,INT16,INT16);
HRGN32      WINAPI CreateEllipticRgn32(INT32,INT32,INT32,INT32);
#define     CreateEllipticRgn WINELIB_NAME(CreateEllipticRgn)
HRGN16      WINAPI CreateEllipticRgnIndirect16(const RECT16 *);
HRGN32      WINAPI CreateEllipticRgnIndirect32(const RECT32 *);
#define     CreateEllipticRgnIndirect WINELIB_NAME(CreateEllipticRgnIndirect)
HFONT16     WINAPI CreateFont16(INT16,INT16,INT16,INT16,INT16,BYTE,BYTE,BYTE,
                                BYTE,BYTE,BYTE,BYTE,BYTE,LPCSTR);
HFONT32     WINAPI CreateFont32A(INT32,INT32,INT32,INT32,INT32,DWORD,DWORD,
                                 DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCSTR);
HFONT32     WINAPI CreateFont32W(INT32,INT32,INT32,INT32,INT32,DWORD,DWORD,
                                 DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCWSTR);
#define     CreateFont WINELIB_NAME_AW(CreateFont)
HFONT16     WINAPI CreateFontIndirect16(const LOGFONT16*);
HFONT32     WINAPI CreateFontIndirect32A(const LOGFONT32A*);
HFONT32     WINAPI CreateFontIndirect32W(const LOGFONT32W*);
#define     CreateFontIndirect WINELIB_NAME_AW(CreateFontIndirect)
HBRUSH16    WINAPI CreateHatchBrush16(INT16,COLORREF);
HBRUSH32    WINAPI CreateHatchBrush32(INT32,COLORREF);
#define     CreateHatchBrush WINELIB_NAME(CreateHatchBrush)
HDC16       WINAPI CreateIC16(LPCSTR,LPCSTR,LPCSTR,const DEVMODE16*);
HDC32       WINAPI CreateIC32A(LPCSTR,LPCSTR,LPCSTR,const DEVMODE32A*);
HDC32       WINAPI CreateIC32W(LPCWSTR,LPCWSTR,LPCWSTR,const DEVMODE32W*);
#define     CreateIC WINELIB_NAME_AW(CreateIC)
HDC16       WINAPI CreateMetaFile16(LPCSTR);
HDC32       WINAPI CreateMetaFile32A(LPCSTR);
HDC32       WINAPI CreateMetaFile32W(LPCWSTR);
#define     CreateMetaFile WINELIB_NAME_AW(CreateMetaFile)
HPALETTE16  WINAPI CreatePalette16(const LOGPALETTE*);
HPALETTE32  WINAPI CreatePalette32(const LOGPALETTE*);
#define     CreatePalette WINELIB_NAME(CreatePalette)
HBRUSH16    WINAPI CreatePatternBrush16(HBITMAP16);
HBRUSH32    WINAPI CreatePatternBrush32(HBITMAP32);
#define     CreatePatternBrush WINELIB_NAME(CreatePatternBrush)
HPEN16      WINAPI CreatePen16(INT16,INT16,COLORREF);
HPEN32      WINAPI CreatePen32(INT32,INT32,COLORREF);
#define     CreatePen WINELIB_NAME(CreatePen)
HPEN16      WINAPI CreatePenIndirect16(const LOGPEN16*);
HPEN32      WINAPI CreatePenIndirect32(const LOGPEN32*);
#define     CreatePenIndirect WINELIB_NAME(CreatePenIndirect)
HRGN16      WINAPI CreatePolyPolygonRgn16(const POINT16*,const INT16*,INT16,INT16);
HRGN32      WINAPI CreatePolyPolygonRgn32(const POINT32*,const INT32*,INT32,INT32);
#define     CreatePolyPolygonRgn WINELIB_NAME(CreatePolyPolygonRgn)
HRGN16      WINAPI CreatePolygonRgn16(const POINT16*,INT16,INT16);
HRGN32      WINAPI CreatePolygonRgn32(const POINT32*,INT32,INT32);
#define     CreatePolygonRgn WINELIB_NAME(CreatePolygonRgn)
HRGN16      WINAPI CreateRectRgn16(INT16,INT16,INT16,INT16);
HRGN32      WINAPI CreateRectRgn32(INT32,INT32,INT32,INT32);
#define     CreateRectRgn WINELIB_NAME(CreateRectRgn)
HRGN16      WINAPI CreateRectRgnIndirect16(const RECT16*);
HRGN32      WINAPI CreateRectRgnIndirect32(const RECT32*);
#define     CreateRectRgnIndirect WINELIB_NAME(CreateRectRgnIndirect)
HRGN16      WINAPI CreateRoundRectRgn16(INT16,INT16,INT16,INT16,INT16,INT16);
HRGN32      WINAPI CreateRoundRectRgn32(INT32,INT32,INT32,INT32,INT32,INT32);
#define     CreateRoundRectRgn WINELIB_NAME(CreateRoundRectRgn)
BOOL16      WINAPI CreateScalableFontResource16(UINT16,LPCSTR,LPCSTR,LPCSTR);
BOOL32      WINAPI CreateScalableFontResource32A(DWORD,LPCSTR,LPCSTR,LPCSTR);
BOOL32      WINAPI CreateScalableFontResource32W(DWORD,LPCWSTR,LPCWSTR,LPCWSTR);
#define     CreateScalableFontResource WINELIB_NAME_AW(CreateScalableFontResource)
HBRUSH16    WINAPI CreateSolidBrush16(COLORREF);
HBRUSH32    WINAPI CreateSolidBrush32(COLORREF);
#define     CreateSolidBrush WINELIB_NAME(CreateSolidBrush)
BOOL16      WINAPI DeleteDC16(HDC16);
BOOL32      WINAPI DeleteDC32(HDC32);
#define     DeleteDC WINELIB_NAME(DeleteDC)
BOOL16      WINAPI DeleteMetaFile16(HMETAFILE16);
BOOL32      WINAPI DeleteMetaFile32(HMETAFILE32);
#define     DeleteMetaFile WINELIB_NAME(DeleteMetaFile)
BOOL16      WINAPI DeleteObject16(HGDIOBJ16);
BOOL32      WINAPI DeleteObject32(HGDIOBJ32);
#define     DeleteObject WINELIB_NAME(DeleteObject)
INT32       WINAPI DescribePixelFormat(HDC32,int,UINT32,
                                       LPPIXELFORMATDESCRIPTOR);
BOOL16      WINAPI DPtoLP16(HDC16,LPPOINT16,INT16);
BOOL32      WINAPI DPtoLP32(HDC32,LPPOINT32,INT32);
#define     DPtoLP WINELIB_NAME(DPtoLP)
BOOL16      WINAPI Ellipse16(HDC16,INT16,INT16,INT16,INT16);
BOOL32      WINAPI Ellipse32(HDC32,INT32,INT32,INT32,INT32);
#define     Ellipse WINELIB_NAME(Ellipse)
INT16       WINAPI EndDoc16(HDC16);
INT32       WINAPI EndDoc32(HDC32);
#define     EndDoc WINELIB_NAME(EndDoc)
BOOL16      WINAPI EndPath16(HDC16);
BOOL32      WINAPI EndPath32(HDC32);
#define     EndPath WINELIB_NAME(EndPath)
INT16       WINAPI EnumFontFamilies16(HDC16,LPCSTR,FONTENUMPROC16,LPARAM);
INT32       WINAPI EnumFontFamilies32A(HDC32,LPCSTR,FONTENUMPROC32A,LPARAM);
INT32       WINAPI EnumFontFamilies32W(HDC32,LPCWSTR,FONTENUMPROC32W,LPARAM);
#define     EnumFontFamilies WINELIB_NAME_AW(EnumFontFamilies)
INT16       WINAPI EnumFontFamiliesEx16(HDC16,LPLOGFONT16,FONTENUMPROCEX16,LPARAM,DWORD);
INT32       WINAPI EnumFontFamiliesEx32A(HDC32,LPLOGFONT32A,FONTENUMPROCEX32A,LPARAM,DWORD);
INT32       WINAPI EnumFontFamiliesEx32W(HDC32,LPLOGFONT32W,FONTENUMPROCEX32W,LPARAM,DWORD);
#define     EnumFontFamiliesEx WINELIB_NAME_AW(EnumFontFamiliesEx)
INT16       WINAPI EnumFonts16(HDC16,LPCSTR,FONTENUMPROC16,LPARAM);
INT32       WINAPI EnumFonts32A(HDC32,LPCSTR,FONTENUMPROC32A,LPARAM);
INT32       WINAPI EnumFonts32W(HDC32,LPCWSTR,FONTENUMPROC32W,LPARAM);
#define     EnumFonts WINELIB_NAME_AW(EnumFonts)
BOOL16      WINAPI EnumMetaFile16(HDC16,HMETAFILE16,MFENUMPROC16,LPARAM);
BOOL32      WINAPI EnumMetaFile32(HDC32,HMETAFILE32,MFENUMPROC32,LPARAM);
#define     EnumMetaFile WINELIB_NAME(EnumMetaFile)
INT16       WINAPI EnumObjects16(HDC16,INT16,GOBJENUMPROC16,LPARAM);
INT32       WINAPI EnumObjects32(HDC32,INT32,GOBJENUMPROC32,LPARAM);
#define     EnumObjects WINELIB_NAME(EnumObjects)
BOOL16      WINAPI EqualRgn16(HRGN16,HRGN16);
BOOL32      WINAPI EqualRgn32(HRGN32,HRGN32);
#define     EqualRgn WINELIB_NAME(EqualRgn)
INT16       WINAPI Escape16(HDC16,INT16,INT16,SEGPTR,SEGPTR);
INT32       WINAPI Escape32(HDC32,INT32,INT32,LPCSTR,LPVOID);
#define     Escape WINELIB_NAME(Escape)
INT16       WINAPI ExcludeClipRect16(HDC16,INT16,INT16,INT16,INT16);
INT32       WINAPI ExcludeClipRect32(HDC32,INT32,INT32,INT32,INT32);
#define     ExcludeClipRect WINELIB_NAME(ExcludeClipRect)
HPEN16      WINAPI ExtCreatePen16(DWORD,DWORD,const LOGBRUSH16*,DWORD,const DWORD*);
HPEN32      WINAPI ExtCreatePen32(DWORD,DWORD,const LOGBRUSH32*,DWORD,const DWORD*);
#define     ExtCreatePen WINELIB_NAME(ExtCreatePen)
BOOL16      WINAPI ExtFloodFill16(HDC16,INT16,INT16,COLORREF,UINT16);
BOOL32      WINAPI ExtFloodFill32(HDC32,INT32,INT32,COLORREF,UINT32);
#define     ExtFloodFill WINELIB_NAME(ExtFloodFill)
BOOL16      WINAPI ExtTextOut16(HDC16,INT16,INT16,UINT16,const RECT16*,
                                LPCSTR,UINT16,const INT16*);
BOOL32      WINAPI ExtTextOut32A(HDC32,INT32,INT32,UINT32,const RECT32*,
                                 LPCSTR,UINT32,const INT32*);
BOOL32      WINAPI ExtTextOut32W(HDC32,INT32,INT32,UINT32,const RECT32*,
                                 LPCWSTR,UINT32,const INT32*);
#define     ExtTextOut WINELIB_NAME_AW(ExtTextOut)
BOOL16      WINAPI FillPath16(HDC16);
BOOL32      WINAPI FillPath32(HDC32);
#define     FillPath WINELIB_NAME(FillPath)
BOOL16      WINAPI FillRgn16(HDC16,HRGN16,HBRUSH16);
BOOL32      WINAPI FillRgn32(HDC32,HRGN32,HBRUSH32);
#define     FillRgn WINELIB_NAME(FillRgn)
BOOL16      WINAPI FlattenPath16(HDC16);
BOOL32      WINAPI FlattenPath32(HDC32);
#define     FlattenPath WINELIB_NAME(FlattenPath)
BOOL16      WINAPI FloodFill16(HDC16,INT16,INT16,COLORREF);
BOOL32      WINAPI FloodFill32(HDC32,INT32,INT32,COLORREF);
#define     FloodFill WINELIB_NAME(FloodFill)
BOOL16      WINAPI FrameRgn16(HDC16,HRGN16,HBRUSH16,INT16,INT16);
BOOL32      WINAPI FrameRgn32(HDC32,HRGN32,HBRUSH32,INT32,INT32);
#define     FrameRgn WINELIB_NAME(FrameRgn)
INT16       WINAPI GetArcDirection16(HDC16);
INT32       WINAPI GetArcDirection32(HDC32);
#define     GetArcDirection WINELIB_NAME(GetArcDirection)
BOOL16      WINAPI GetAspectRatioFilterEx16(HDC16,LPSIZE16);
BOOL32      WINAPI GetAspectRatioFilterEx32(HDC32,LPSIZE32);
#define     GetAspectRatioFilterEx WINELIB_NAME(GetAspectRatioFilterEx)
LONG        WINAPI GetBitmapBits16(HBITMAP16,LONG,LPVOID);
LONG        WINAPI GetBitmapBits32(HBITMAP32,LONG,LPVOID);
#define     GetBitmapBits WINELIB_NAME(GetBitmapBits)
BOOL16      WINAPI GetBitmapDimensionEx16(HBITMAP16,LPSIZE16);
BOOL32      WINAPI GetBitmapDimensionEx32(HBITMAP32,LPSIZE32);
#define     GetBitmapDimensionEx WINELIB_NAME(GetBitmapDimensionEx)
BOOL16      WINAPI GetBrushOrgEx16(HDC16,LPPOINT16);
BOOL32      WINAPI GetBrushOrgEx32(HDC32,LPPOINT32);
#define     GetBrushOrgEx WINELIB_NAME(GetBrushOrgEx)
COLORREF    WINAPI GetBkColor16(HDC16);
COLORREF    WINAPI GetBkColor32(HDC32);
#define     GetBkColor WINELIB_NAME(GetBkColor)
INT16       WINAPI GetBkMode16(HDC16);
INT32       WINAPI GetBkMode32(HDC32);
#define     GetBkMode WINELIB_NAME(GetBkMode)
UINT16      WINAPI GetBoundsRect16(HDC16,LPRECT16,UINT16);
UINT32      WINAPI GetBoundsRect32(HDC32,LPRECT32,UINT32);
#define     GetBoundsRect WINELIB_NAME(GetBoundsRect)
BOOL16      WINAPI GetCharABCWidths16(HDC16,UINT16,UINT16,LPABC16);
BOOL32      WINAPI GetCharABCWidths32A(HDC32,UINT32,UINT32,LPABC32);
BOOL32      WINAPI GetCharABCWidths32W(HDC32,UINT32,UINT32,LPABC32);
#define     GetCharABCWidths WINELIB_NAME_AW(GetCharABCWidths)
DWORD       WINAPI GetCharacterPlacement32A(HDC32,LPCSTR,INT32,INT32,GCP_RESULTS32A*,DWORD);
DWORD       WINAPI GetCharacterPlacement32W(HDC32,LPCWSTR,INT32,INT32,GCP_RESULTS32W*,DWORD);
#define     GetCharacterPlacement WINELIB_NAME_AW(GetCharacterPlacement)
BOOL16      WINAPI GetCharWidth16(HDC16,UINT16,UINT16,LPINT16);
BOOL32      WINAPI GetCharWidth32A(HDC32,UINT32,UINT32,LPINT32);
BOOL32      WINAPI GetCharWidth32W(HDC32,UINT32,UINT32,LPINT32);
#define     GetCharWidth WINELIB_NAME_AW(GetCharWidth)
INT16       WINAPI GetClipBox16(HDC16,LPRECT16);
INT32       WINAPI GetClipBox32(HDC32,LPRECT32);
#define     GetClipBox WINELIB_NAME(GetClipBox)
HRGN16      WINAPI GetClipRgn16(HDC16);
INT32       WINAPI GetClipRgn32(HDC32,HRGN32);
#define     GetClipRgn WINELIB_NAME(GetClipRgn)
BOOL16      WINAPI GetCurrentPositionEx16(HDC16,LPPOINT16);
BOOL32      WINAPI GetCurrentPositionEx32(HDC32,LPPOINT32);
#define     GetCurrentPositionEx WINELIB_NAME(GetCurrentPositionEx)
INT16       WINAPI GetDeviceCaps16(HDC16,INT16);
INT32       WINAPI GetDeviceCaps32(HDC32,INT32);
#define     GetDeviceCaps WINELIB_NAME(GetDeviceCaps)
UINT16      WINAPI GetDIBColorTable16(HDC16,UINT16,UINT16,RGBQUAD*);
UINT32      WINAPI GetDIBColorTable32(HDC32,UINT32,UINT32,RGBQUAD*);
#define     GetDIBColorTable WINELIB_NAME(GetDIBColorTable)
INT16       WINAPI GetDIBits16(HDC16,HBITMAP16,UINT16,UINT16,LPSTR,LPBITMAPINFO,UINT16);
INT32       WINAPI GetDIBits32(HDC32,HBITMAP32,UINT32,UINT32,LPSTR,LPBITMAPINFO,UINT32);
#define     GetDIBits WINELIB_NAME(GetDIBits)
DWORD       WINAPI GetFontData32(HDC32,DWORD,DWORD,LPVOID,DWORD);
#define     GetFontData WINELIB_NAME(GetFontData)
DWORD       WINAPI GetFontLanguageInfo16(HDC16);
DWORD       WINAPI GetFontLanguageInfo32(HDC32);
#define     GetFontLanguageInfo WINELIB_NAME(GetFontLanguageInfo)
DWORD       WINAPI GetGlyphOutline16(HDC16,UINT16,UINT16,LPGLYPHMETRICS16,DWORD,LPVOID,const MAT2*);
DWORD       WINAPI GetGlyphOutline32A(HDC32,UINT32,UINT32,LPGLYPHMETRICS32,DWORD,LPVOID,const MAT2*);
DWORD       WINAPI GetGlyphOutline32W(HDC32,UINT32,UINT32,LPGLYPHMETRICS32,DWORD,LPVOID,const MAT2*);
#define     GetGlyphOutline WINELIB_NAME_AW(GetGlyphOutline)
INT16       WINAPI GetKerningPairs16(HDC16,INT16,LPKERNINGPAIR16);
DWORD       WINAPI GetKerningPairs32A(HDC32,DWORD,LPKERNINGPAIR32);
DWORD       WINAPI GetKerningPairs32W(HDC32,DWORD,LPKERNINGPAIR32);
#define     GetKerningPairs WINELIB_NAME_AW(GetKerningPairs)
INT16       WINAPI GetMapMode16(HDC16);
INT32       WINAPI GetMapMode32(HDC32);
#define     GetMapMode WINELIB_NAME(GetMapMode)
HMETAFILE16 WINAPI GetMetaFile16(LPCSTR);
HMETAFILE32 WINAPI GetMetaFile32A(LPCSTR);
HMETAFILE32 WINAPI GetMetaFile32W(LPCWSTR);
#define     GetMetaFile WINELIB_NAME_AW(GetMetaFile)
DWORD       WINAPI GetNearestColor16(HDC16,DWORD);
DWORD       WINAPI GetNearestColor32(HDC32,DWORD);
#define     GetNearestColor WINELIB_NAME(GetNearestColor)
UINT16      WINAPI GetNearestPaletteIndex16(HPALETTE16,COLORREF);
UINT32      WINAPI GetNearestPaletteIndex32(HPALETTE32,COLORREF);
#define     GetNearestPaletteIndex WINELIB_NAME(GetNearestPaletteIndex)
INT16       WINAPI GetObject16(HANDLE16,INT16,LPVOID);
INT32       WINAPI GetObject32A(HANDLE32,INT32,LPVOID);
INT32       WINAPI GetObject32W(HANDLE32,INT32,LPVOID);
#define     GetObject WINELIB_NAME_AW(GetObject)
UINT16      WINAPI GetOutlineTextMetrics16(HDC16,UINT16,LPOUTLINETEXTMETRIC16);
UINT32      WINAPI GetOutlineTextMetrics32A(HDC32,UINT32,LPOUTLINETEXTMETRIC32A);
UINT32      WINAPI GetOutlineTextMetrics32W(HDC32,UINT32,LPOUTLINETEXTMETRIC32W);
#define     GetOutlineTextMetrics WINELIB_NAME_AW(GetOutlineTextMetrics)
UINT16      WINAPI GetPaletteEntries16(HPALETTE16,UINT16,UINT16,LPPALETTEENTRY);
UINT32      WINAPI GetPaletteEntries32(HPALETTE32,UINT32,UINT32,LPPALETTEENTRY);
#define     GetPaletteEntries WINELIB_NAME(GetPaletteEntries)
INT16       WINAPI GetPath16(HDC16,LPPOINT16,LPBYTE,INT16);
INT32       WINAPI GetPath32(HDC32,LPPOINT32,LPBYTE,INT32);
#define     GetPath WINELIB_NAME(GetPath)
COLORREF    WINAPI GetPixel16(HDC16,INT16,INT16);
COLORREF    WINAPI GetPixel32(HDC32,INT32,INT32);
#define     GetPixel WINELIB_NAME(GetPixel)
INT32       WINAPI GetPixelFormat(HDC32);
INT16       WINAPI GetPolyFillMode16(HDC16);
INT32       WINAPI GetPolyFillMode32(HDC32);
#define     GetPolyFillMode WINELIB_NAME(GetPolyFillMode)
BOOL16      WINAPI GetRasterizerCaps16(LPRASTERIZER_STATUS,UINT16);
BOOL32      WINAPI GetRasterizerCaps32(LPRASTERIZER_STATUS,UINT32);
#define     GetRasterizerCaps WINELIB_NAME(GetRasterizerCaps)
DWORD       WINAPI GetRegionData16(HRGN16,DWORD,LPRGNDATA);
DWORD       WINAPI GetRegionData32(HRGN32,DWORD,LPRGNDATA);
#define     GetRegionData WINELIB_NAME(GetRegionData)
INT16       WINAPI GetRelAbs16(HDC16);
INT32       WINAPI GetRelAbs32(HDC32);
#define     GetRelAbs WINELIB_NAME(GetRelAbs)
INT16       WINAPI GetRgnBox16(HRGN16,LPRECT16);
INT32       WINAPI GetRgnBox32(HRGN32,LPRECT32);
#define     GetRgnBox WINELIB_NAME(GetRgnBox)
INT16       WINAPI GetROP216(HDC16);
INT32       WINAPI GetROP232(HDC32);
#define     GetROP2 WINELIB_NAME(GetROP2)
HGDIOBJ16   WINAPI GetStockObject16(INT16);
HGDIOBJ32   WINAPI GetStockObject32(INT32);
#define     GetStockObject WINELIB_NAME(GetStockObject)
INT16       WINAPI GetStretchBltMode16(HDC16);
INT32       WINAPI GetStretchBltMode32(HDC32);
#define     GetStretchBltMode WINELIB_NAME(GetStretchBltMode)
UINT16      WINAPI GetSystemPaletteEntries16(HDC16,UINT16,UINT16,LPPALETTEENTRY);
UINT32      WINAPI GetSystemPaletteEntries32(HDC32,UINT32,UINT32,LPPALETTEENTRY);
#define     GetSystemPaletteEntries WINELIB_NAME(GetSystemPaletteEntries)
UINT16      WINAPI GetSystemPaletteUse16(HDC16);
UINT32      WINAPI GetSystemPaletteUse32(HDC32);
#define     GetSystemPaletteUse WINELIB_NAME(GetSystemPaletteUse)
UINT16      WINAPI GetTextAlign16(HDC16);
UINT32      WINAPI GetTextAlign32(HDC32);
#define     GetTextAlign WINELIB_NAME(GetTextAlign)
INT16       WINAPI GetTextCharacterExtra16(HDC16);
INT32       WINAPI GetTextCharacterExtra32(HDC32);
#define     GetTextCharacterExtra WINELIB_NAME(GetTextCharacterExtra)
UINT16      WINAPI GetTextCharset16(HDC16);
UINT32      WINAPI GetTextCharset32(HDC32);
#define     GetTextCharset WINELIB_NAME(GetTextCharset)
COLORREF    WINAPI GetTextColor16(HDC16);
COLORREF    WINAPI GetTextColor32(HDC32);
#define     GetTextColor WINELIB_NAME(GetTextColor)
/* this one is different, because Win32 has *both* 
 * GetTextExtentPoint and GetTextExtentPoint32 !
 */
BOOL16      WINAPI GetTextExtentPoint16(HDC16,LPCSTR,INT16,LPSIZE16);
BOOL32      WINAPI GetTextExtentPoint32A(HDC32,LPCSTR,INT32,LPSIZE32);
BOOL32      WINAPI GetTextExtentPoint32W(HDC32,LPCWSTR,INT32,LPSIZE32);
BOOL32      WINAPI GetTextExtentPoint32ABuggy(HDC32,LPCSTR,INT32,LPSIZE32);
BOOL32      WINAPI GetTextExtentPoint32WBuggy(HDC32,LPCWSTR,INT32,LPSIZE32);
#ifdef UNICODE
#define     GetTextExtentPoint GetTextExtentPoint32WBuggy
#define     GetTextExtentPoint32 GetTextExtentPoint32W
#else
#define     GetTextExtentPoint GetTextExtentPoint32ABuggy
#define     GetTextExtentPoint32 GetTextExtentPoint32A
#endif
INT16       WINAPI GetTextFace16(HDC16,INT16,LPSTR);
INT32       WINAPI GetTextFace32A(HDC32,INT32,LPSTR);
INT32       WINAPI GetTextFace32W(HDC32,INT32,LPWSTR);
#define     GetTextFace WINELIB_NAME_AW(GetTextFace)
BOOL16      WINAPI GetTextMetrics16(HDC16,LPTEXTMETRIC16);
BOOL32      WINAPI GetTextMetrics32A(HDC32,LPTEXTMETRIC32A);
BOOL32      WINAPI GetTextMetrics32W(HDC32,LPTEXTMETRIC32W);
#define     GetTextMetrics WINELIB_NAME_AW(GetTextMetrics)
BOOL16      WINAPI GetViewportExtEx16(HDC16,LPSIZE16);
BOOL32      WINAPI GetViewportExtEx32(HDC32,LPSIZE32);
#define     GetViewportExtEx WINELIB_NAME(GetViewportExtEx)
BOOL16      WINAPI GetViewportOrgEx16(HDC16,LPPOINT16);
BOOL32      WINAPI GetViewportOrgEx32(HDC32,LPPOINT32);
#define     GetViewportOrgEx WINELIB_NAME(GetViewportOrgEx)
BOOL16      WINAPI GetWindowExtEx16(HDC16,LPSIZE16);
BOOL32      WINAPI GetWindowExtEx32(HDC32,LPSIZE32);
#define     GetWindowExtEx WINELIB_NAME(GetWindowExtEx)
BOOL16      WINAPI GetWindowOrgEx16(HDC16,LPPOINT16);
BOOL32      WINAPI GetWindowOrgEx32(HDC32,LPPOINT32);
#define     GetWindowOrgEx WINELIB_NAME(GetWindowOrgEx)
INT16       WINAPI IntersectClipRect16(HDC16,INT16,INT16,INT16,INT16);
INT32       WINAPI IntersectClipRect32(HDC32,INT32,INT32,INT32,INT32);
#define     IntersectClipRect WINELIB_NAME(IntersectClipRect)
BOOL16      WINAPI InvertRgn16(HDC16,HRGN16);
BOOL32      WINAPI InvertRgn32(HDC32,HRGN32);
#define     InvertRgn WINELIB_NAME(InvertRgn)
VOID        WINAPI LineDDA16(INT16,INT16,INT16,INT16,LINEDDAPROC16,LPARAM);
BOOL32      WINAPI LineDDA32(INT32,INT32,INT32,INT32,LINEDDAPROC32,LPARAM);
#define     LineDDA WINELIB_NAME(LineDDA)
BOOL16      WINAPI LineTo16(HDC16,INT16,INT16);
BOOL32      WINAPI LineTo32(HDC32,INT32,INT32);
#define     LineTo WINELIB_NAME(LineTo)
BOOL16      WINAPI LPtoDP16(HDC16,LPPOINT16,INT16);
BOOL32      WINAPI LPtoDP32(HDC32,LPPOINT32,INT32);
#define     LPtoDP WINELIB_NAME(LPtoDP)
BOOL16      WINAPI MoveToEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL32      WINAPI MoveToEx32(HDC32,INT32,INT32,LPPOINT32);
#define     MoveToEx WINELIB_NAME(MoveToEx)
INT16       WINAPI MulDiv16(INT16,INT16,INT16);
/* FIXME This is defined in kernel32.spec !?*/
INT32       WINAPI MulDiv32(INT32,INT32,INT32);
#define     MulDiv WINELIB_NAME(MulDiv)
INT16       WINAPI OffsetClipRgn16(HDC16,INT16,INT16);
INT32       WINAPI OffsetClipRgn32(HDC32,INT32,INT32);
#define     OffsetClipRgn WINELIB_NAME(OffsetClipRgn)
INT16       WINAPI OffsetRgn16(HRGN16,INT16,INT16);
INT32       WINAPI OffsetRgn32(HRGN32,INT32,INT32);
#define     OffsetRgn WINELIB_NAME(OffsetRgn)
BOOL16      WINAPI OffsetViewportOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL32      WINAPI OffsetViewportOrgEx32(HDC32,INT32,INT32,LPPOINT32);
#define     OffsetViewportOrgEx WINELIB_NAME(OffsetViewportOrgEx)
BOOL16      WINAPI OffsetWindowOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL32      WINAPI OffsetWindowOrgEx32(HDC32,INT32,INT32,LPPOINT32);
#define     OffsetWindowOrgEx WINELIB_NAME(OffsetWindowOrgEx)
BOOL16      WINAPI PaintRgn16(HDC16,HRGN16);
BOOL32      WINAPI PaintRgn32(HDC32,HRGN32);
#define     PaintRgn WINELIB_NAME(PaintRgn)
BOOL16      WINAPI PatBlt16(HDC16,INT16,INT16,INT16,INT16,DWORD);
BOOL32      WINAPI PatBlt32(HDC32,INT32,INT32,INT32,INT32,DWORD);
#define     PatBlt WINELIB_NAME(PatBlt)
HRGN16      WINAPI PathToRegion16(HDC16);
HRGN32      WINAPI PathToRegion32(HDC32);
#define     PathToRegion WINELIB_NAME(PathToRegion)
BOOL16      WINAPI Pie16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL32      WINAPI Pie32(HDC32,INT32,INT32,INT32,INT32,INT32,INT32,INT32,INT32);
#define     Pie WINELIB_NAME(Pie)
BOOL16      WINAPI PlayMetaFile16(HDC16,HMETAFILE16);
BOOL32      WINAPI PlayMetaFile32(HDC32,HMETAFILE32);
#define     PlayMetaFile WINELIB_NAME(PlayMetaFile)
VOID        WINAPI PlayMetaFileRecord16(HDC16,LPHANDLETABLE16,LPMETARECORD,UINT16);
BOOL32      WINAPI PlayMetaFileRecord32(HDC32,LPHANDLETABLE32,LPMETARECORD,UINT32);
#define     PlayMetaFileRecord WINELIB_NAME(PlayMetaFileRecord)
BOOL16      WINAPI PolyBezier16(HDC16,const POINT16*,INT16);
BOOL32      WINAPI PolyBezier32(HDC32,const POINT32*,DWORD);
#define     PolyBezier WINELIB_NAME(PolyBezier)
BOOL16      WINAPI PolyBezierTo16(HDC16,const POINT16*,INT16);
BOOL32      WINAPI PolyBezierTo32(HDC32,const POINT32*,DWORD);
#define     PolyBezierTo WINELIB_NAME(PolyBezierTo)
BOOL16      WINAPI PolyPolygon16(HDC16,const POINT16*,const INT16*,UINT16);
BOOL32      WINAPI PolyPolygon32(HDC32,const POINT32*,const INT32*,UINT32);
#define     PolyPolygon WINELIB_NAME(PolyPolygon)
BOOL16      WINAPI Polygon16(HDC16,const POINT16*,INT16);
BOOL32      WINAPI Polygon32(HDC32,const POINT32*,INT32);
#define     Polygon WINELIB_NAME(Polygon)
BOOL16      WINAPI Polyline16(HDC16,const POINT16*,INT16);
BOOL32      WINAPI Polyline32(HDC32,const POINT32*,INT32);
#define     Polyline WINELIB_NAME(Polyline)
BOOL32      WINAPI PolylineTo32(HDC32,const POINT32*,DWORD);
BOOL16      WINAPI PtInRegion16(HRGN16,INT16,INT16);
BOOL32      WINAPI PtInRegion32(HRGN32,INT32,INT32);
#define     PtInRegion WINELIB_NAME(PtInRegion)
BOOL16      WINAPI PtVisible16(HDC16,INT16,INT16);
BOOL32      WINAPI PtVisible32(HDC32,INT32,INT32);
#define     PtVisible WINELIB_NAME(PtVisible)
/* FIXME This is defined in user.spec !? */
UINT16      WINAPI RealizePalette16(HDC16);
UINT32      WINAPI RealizePalette32(HDC32);
#define     RealizePalette WINELIB_NAME(RealizePalette)
BOOL16      WINAPI Rectangle16(HDC16,INT16,INT16,INT16,INT16);
BOOL32      WINAPI Rectangle32(HDC32,INT32,INT32,INT32,INT32);
#define     Rectangle WINELIB_NAME(Rectangle)
BOOL16      WINAPI RectInRegion16(HRGN16,const RECT16 *);
BOOL32      WINAPI RectInRegion32(HRGN32,const RECT32 *);
#define     RectInRegion WINELIB_NAME(RectInRegion)
BOOL16      WINAPI RectVisible16(HDC16,const RECT16*);
BOOL32      WINAPI RectVisible32(HDC32,const RECT32*);
#define     RectVisible WINELIB_NAME(RectVisible)
BOOL16      WINAPI RemoveFontResource16(SEGPTR);
BOOL32      WINAPI RemoveFontResource32A(LPCSTR);
BOOL32      WINAPI RemoveFontResource32W(LPCWSTR);
#define     RemoveFontResource WINELIB_NAME_AW(RemoveFontResource)
HDC16       WINAPI ResetDC16(HDC16,const DEVMODE16 *);
HDC32       WINAPI ResetDC32A(HDC32,const DEVMODE32A *);
HDC32       WINAPI ResetDC32W(HDC32,const DEVMODE32W *);
#define     ResetDC WINELIB_NAME_AW(ResetDC)
BOOL16      WINAPI ResizePalette16(HPALETTE16,UINT16);
BOOL32      WINAPI ResizePalette32(HPALETTE32,UINT32);
#define     ResizePalette WINELIB_NAME(ResizePalette)
BOOL16      WINAPI RestoreDC16(HDC16,INT16);
BOOL32      WINAPI RestoreDC32(HDC32,INT32);
#define     RestoreDC WINELIB_NAME(RestoreDC)
BOOL16      WINAPI RoundRect16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL32      WINAPI RoundRect32(HDC32,INT32,INT32,INT32,INT32,INT32,INT32);
#define     RoundRect WINELIB_NAME(RoundRect)
INT16       WINAPI SaveDC16(HDC16);
INT32       WINAPI SaveDC32(HDC32);
#define     SaveDC WINELIB_NAME(SaveDC)
BOOL16      WINAPI ScaleViewportExtEx16(HDC16,INT16,INT16,INT16,INT16,LPSIZE16);
BOOL32      WINAPI ScaleViewportExtEx32(HDC32,INT32,INT32,INT32,INT32,LPSIZE32);
#define     ScaleViewportExtEx WINELIB_NAME(ScaleViewportExtEx)
BOOL16      WINAPI ScaleWindowExtEx16(HDC16,INT16,INT16,INT16,INT16,LPSIZE16);
BOOL32      WINAPI ScaleWindowExtEx32(HDC32,INT32,INT32,INT32,INT32,LPSIZE32);
#define     ScaleWindowExtEx WINELIB_NAME(ScaleWindowExtEx)
BOOL16      WINAPI SelectClipPath16(HDC16,INT16);
BOOL32      WINAPI SelectClipPath32(HDC32,INT32);
#define     SelectClipPath WINELIB_NAME(SelectClipPath)
INT16       WINAPI SelectClipRgn16(HDC16,HRGN16);
INT32       WINAPI SelectClipRgn32(HDC32,HRGN32);
#define     SelectClipRgn WINELIB_NAME(SelectClipRgn)
HGDIOBJ16   WINAPI SelectObject16(HDC16,HGDIOBJ16);
HGDIOBJ32   WINAPI SelectObject32(HDC32,HGDIOBJ32);
#define     SelectObject WINELIB_NAME(SelectObject)
/* FIXME This is defined in user.spec !? */
HPALETTE16  WINAPI SelectPalette16(HDC16,HPALETTE16,BOOL16);
HPALETTE32  WINAPI SelectPalette32(HDC32,HPALETTE32,BOOL32);
#define     SelectPalette WINELIB_NAME(SelectPalette)
INT16       WINAPI SetAbortProc16(HDC16,SEGPTR);
INT32       WINAPI SetAbortProc32(HDC32,FARPROC32);
#define     SetAbortProc WINELIB_NAME(SetAbortProc)
INT16       WINAPI SetArcDirection16(HDC16,INT16);
INT32       WINAPI SetArcDirection32(HDC32,INT32);
#define     SetArcDirection WINELIB_NAME(SetArcDirection)
LONG        WINAPI SetBitmapBits16(HBITMAP16,LONG,LPCVOID);
LONG        WINAPI SetBitmapBits32(HBITMAP32,LONG,LPCVOID);
#define     SetBitmapBits WINELIB_NAME(SetBitmapBits)
BOOL16      WINAPI SetBitmapDimensionEx16(HBITMAP16,INT16,INT16,LPSIZE16);
BOOL32      WINAPI SetBitmapDimensionEx32(HBITMAP32,INT32,INT32,LPSIZE32);
#define     SetBitmapDimensionEx WINELIB_NAME(SetBitmapDimensionEx)
COLORREF    WINAPI SetBkColor16(HDC16,COLORREF);
COLORREF    WINAPI SetBkColor32(HDC32,COLORREF);
#define     SetBkColor WINELIB_NAME(SetBkColor)
INT16       WINAPI SetBkMode16(HDC16,INT16);
INT32       WINAPI SetBkMode32(HDC32,INT32);
#define     SetBkMode WINELIB_NAME(SetBkMode)
UINT16      WINAPI SetBoundsRect16(HDC16,const RECT16*,UINT16);
UINT32      WINAPI SetBoundsRect32(HDC32,const RECT32*,UINT32);
#define     SetBoundsRect WINELIB_NAME(SetBoundsRect)
UINT16      WINAPI SetDIBColorTable16(HDC16,UINT16,UINT16,RGBQUAD*);
UINT32      WINAPI SetDIBColorTable32(HDC32,UINT32,UINT32,RGBQUAD*);
#define     SetDIBColorTable WINELIB_NAME(SetDIBColorTable)
INT16       WINAPI SetDIBits16(HDC16,HBITMAP16,UINT16,UINT16,LPCVOID,const BITMAPINFO*,UINT16);
INT32       WINAPI SetDIBits32(HDC32,HBITMAP32,UINT32,UINT32,LPCVOID,const BITMAPINFO*,UINT32);
#define     SetDIBits WINELIB_NAME(SetDIBits)
INT16       WINAPI SetDIBitsToDevice16(HDC16,INT16,INT16,INT16,INT16,INT16,
                         INT16,UINT16,UINT16,LPCVOID,const BITMAPINFO*,UINT16);
INT32       WINAPI SetDIBitsToDevice32(HDC32,INT32,INT32,DWORD,DWORD,INT32,
                         INT32,UINT32,UINT32,LPCVOID,const BITMAPINFO*,UINT32);
#define     SetDIBitsToDevice WINELIB_NAME(SetDIBitsToDevice)
INT16       WINAPI SetMapMode16(HDC16,INT16);
INT32       WINAPI SetMapMode32(HDC32,INT32);
#define     SetMapMode WINELIB_NAME(SetMapMode)
DWORD       WINAPI SetMapperFlags16(HDC16,DWORD);
DWORD       WINAPI SetMapperFlags32(HDC32,DWORD);
#define     SetMapperFlags WINELIB_NAME(SetMapperFlags)
UINT16      WINAPI SetPaletteEntries16(HPALETTE16,UINT16,UINT16,LPPALETTEENTRY);
UINT32      WINAPI SetPaletteEntries32(HPALETTE32,UINT32,UINT32,LPPALETTEENTRY);
#define     SetPaletteEntries WINELIB_NAME(SetPaletteEntries)
COLORREF    WINAPI SetPixel16(HDC16,INT16,INT16,COLORREF);
COLORREF    WINAPI SetPixel32(HDC32,INT32,INT32,COLORREF);
#define     SetPixel WINELIB_NAME(SetPixel)
BOOL32      WINAPI SetPixelV32(HDC32,INT32,INT32,COLORREF);
BOOL32      WINAPI SetPixelFormat(HDC32,int,const PIXELFORMATDESCRIPTOR*);
INT16       WINAPI SetPolyFillMode16(HDC16,INT16);
INT32       WINAPI SetPolyFillMode32(HDC32,INT32);
#define     SetPolyFillMode WINELIB_NAME(SetPolyFillMode)
VOID        WINAPI SetRectRgn16(HRGN16,INT16,INT16,INT16,INT16);
VOID        WINAPI SetRectRgn32(HRGN32,INT32,INT32,INT32,INT32);
#define     SetRectRgn WINELIB_NAME(SetRectRgn)
INT16       WINAPI SetRelAbs16(HDC16,INT16);
INT32       WINAPI SetRelAbs32(HDC32,INT32);
#define     SetRelAbs WINELIB_NAME(SetRelAbs)
INT16       WINAPI SetROP216(HDC16,INT16);
INT32       WINAPI SetROP232(HDC32,INT32);
#define     SetROP2 WINELIB_NAME(SetROP2)
INT16       WINAPI SetStretchBltMode16(HDC16,INT16);
INT32       WINAPI SetStretchBltMode32(HDC32,INT32);
#define     SetStretchBltMode WINELIB_NAME(SetStretchBltMode)
UINT16      WINAPI SetSystemPaletteUse16(HDC16,UINT16);
UINT32      WINAPI SetSystemPaletteUse32(HDC32,UINT32);
#define     SetSystemPaletteUse WINELIB_NAME(SetSystemPaletteUse)
UINT16      WINAPI SetTextAlign16(HDC16,UINT16);
UINT32      WINAPI SetTextAlign32(HDC32,UINT32);
#define     SetTextAlign WINELIB_NAME(SetTextAlign)
INT16       WINAPI SetTextCharacterExtra16(HDC16,INT16);
INT32       WINAPI SetTextCharacterExtra32(HDC32,INT32);
#define     SetTextCharacterExtra WINELIB_NAME(SetTextCharacterExtra)
COLORREF    WINAPI SetTextColor16(HDC16,COLORREF);
COLORREF    WINAPI SetTextColor32(HDC32,COLORREF);
#define     SetTextColor WINELIB_NAME(SetTextColor)
INT16       WINAPI SetTextJustification16(HDC16,INT16,INT16);
BOOL32      WINAPI SetTextJustification32(HDC32,INT32,INT32);
#define     SetTextJustification WINELIB_NAME(SetTextJustification)
BOOL16      WINAPI SetViewportExtEx16(HDC16,INT16,INT16,LPSIZE16);
BOOL32      WINAPI SetViewportExtEx32(HDC32,INT32,INT32,LPSIZE32);
#define     SetViewportExtEx WINELIB_NAME(SetViewportExtEx)
BOOL16      WINAPI SetViewportOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL32      WINAPI SetViewportOrgEx32(HDC32,INT32,INT32,LPPOINT32);
#define     SetViewportOrgEx WINELIB_NAME(SetViewportOrgEx)
BOOL16      WINAPI SetWindowExtEx16(HDC16,INT16,INT16,LPSIZE16);
BOOL32      WINAPI SetWindowExtEx32(HDC32,INT32,INT32,LPSIZE32);
#define     SetWindowExtEx WINELIB_NAME(SetWindowExtEx)
BOOL16      WINAPI SetWindowOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL32      WINAPI SetWindowOrgEx32(HDC32,INT32,INT32,LPPOINT32);
#define     SetWindowOrgEx WINELIB_NAME(SetWindowOrgEx)
HENHMETAFILE32 WINAPI SetWinMetaFileBits(UINT32,CONST BYTE*,HDC32,CONST METAFILEPICT32 *);
INT16       WINAPI StartDoc16(HDC16,const DOCINFO16*);
INT32       WINAPI StartDoc32A(HDC32,const DOCINFO32A*);
INT32       WINAPI StartDoc32W(HDC32,const DOCINFO32W*);
#define     StartDoc WINELIB_NAME_AW(StartDoc)
INT16       WINAPI StartPage16(HDC16);
INT32       WINAPI StartPage32(HDC32);
#define     StartPage WINELIB_NAME(StartPage)
INT16       WINAPI EndPage16(HDC16);
INT32       WINAPI EndPage32(HDC32);
#define     EndPage WINELIB_NAME(EndPage)
BOOL16      WINAPI StretchBlt16(HDC16,INT16,INT16,INT16,INT16,HDC16,INT16,
                                INT16,INT16,INT16,DWORD);
BOOL32      WINAPI StretchBlt32(HDC32,INT32,INT32,INT32,INT32,HDC32,INT32,
                                INT32,INT32,INT32,DWORD);
#define     StretchBlt WINELIB_NAME(StretchBlt)
INT16       WINAPI StretchDIBits16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,
                       INT16,INT16,const VOID*,const BITMAPINFO*,UINT16,DWORD);
INT32       WINAPI StretchDIBits32(HDC32,INT32,INT32,INT32,INT32,INT32,INT32,
                       INT32,INT32,const VOID*,const BITMAPINFO*,UINT32,DWORD);
#define     StretchDIBits WINELIB_NAME(StretchDIBits)
BOOL16      WINAPI StrokeAndFillPath16(HDC16);
BOOL32      WINAPI StrokeAndFillPath32(HDC32);
#define     StrokeAndFillPath WINELIB_NAME(StrokeAndFillPath)
BOOL16      WINAPI StrokePath16(HDC16);
BOOL32      WINAPI StrokePath32(HDC32);
#define     StrokePath WINELIB_NAME(StrokePath)
BOOL32      WINAPI SwapBuffers(HDC32);
BOOL16      WINAPI TextOut16(HDC16,INT16,INT16,LPCSTR,INT16);
BOOL32      WINAPI TextOut32A(HDC32,INT32,INT32,LPCSTR,INT32);
BOOL32      WINAPI TextOut32W(HDC32,INT32,INT32,LPCWSTR,INT32);
#define     TextOut WINELIB_NAME_AW(TextOut)
BOOL16      WINAPI UnrealizeObject16(HGDIOBJ16);
BOOL32      WINAPI UnrealizeObject32(HGDIOBJ32);
#define     UnrealizeObject WINELIB_NAME(UnrealizeObject)
INT16       WINAPI UpdateColors16(HDC16);
BOOL32      WINAPI UpdateColors32(HDC32);
#define     UpdateColors WINELIB_NAME(UpdateColors)
BOOL16      WINAPI WidenPath16(HDC16);
BOOL32      WINAPI WidenPath32(HDC32);
#define     WidenPath WINELIB_NAME(WidenPath)

#endif /* __WINE_WINGDI_H */
