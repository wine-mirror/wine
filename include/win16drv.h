/*
 * Win16 printer driver definitions
 */

#ifndef __WINE_WIN16DRV_H
#define __WINE_WIN16DRV_H

#include "windows.h"
#include "gdi.h"

#define SETHIGHBIT 
#undef SETHIGHBIT
#ifdef SETHIGHBIT
#define GETGDIINFO  0x8001
#define INITPDEVICE 0x8000
#else
#define GETGDIINFO  0x0001
#define INITPDEVICE 0x0000
#endif

/* Internal Data */
#define ORD_BITBLT		1
#define ORD_COLORINFO		2		
#define ORD_CONTROL		3
#define ORD_DISABLE		4
#define ORD_ENABLE		5
#define ORD_ENUMDFONTS		6
#define ORD_ENUMOBJ		7
#define ORD_OUTPUT		8
#define ORD_PIXEL		9	
#define ORD_REALIZEOBJECT	10
#define ORD_STRBLT		11
#define ORD_SCANLR		12
#define ORD_DEVICEMODE		13
#define ORD_EXTTEXTOUT		14
#define ORD_GETCHARWIDTH	15
#define ORD_DEVICEBITMAP	16
#define ORD_FASTBORDER		17
#define ORD_SETATTRIBUTE	18

#define ORD_STRETCHBLT		27
#define ORD_STRETCHDIBITS	28
#define ORD_SELECTBITMAP	29
#define ORD_BITMAPBITS		30

#define ORD_EXTDEVICEMODE	90
#define ORD_DEVICECAPABILITIES	91
#define ORD_ADVANCEDSETUPDIALOG	93

#define ORD_DIALOGFN		100
#define ORD_PSEUDOEDIT		101
                        
enum {
    FUNC_BITBLT = 0,		 
    FUNC_COLORINFO,    	 
    FUNC_CONTROL,    	
    FUNC_DISABLE,    	
    FUNC_ENABLE,    		
    FUNC_ENUMDFONTS,    	
    FUNC_ENUMOBJ,    	
    FUNC_OUTPUT,    		
    FUNC_PIXEL,    			
    FUNC_REALIZEOBJECT,    	
    FUNC_STRBLT,    		
    FUNC_SCANLR,    		
    FUNC_DEVICEMODE,    	
    FUNC_EXTTEXTOUT,    	
    FUNC_GETCHARWIDTH,    	
    FUNC_DEVICEBITMAP,    	
    FUNC_FASTBORDER,         
    FUNC_SETATTRIBUTE,    				
    FUNC_STRETCHBLT,    	
    FUNC_STRETCHDIBITS,  	
    FUNC_SELECTBITMAP,    	
    FUNC_BITMAPBITS,    			       
    FUNC_EXTDEVICEMODE,    	
    FUNC_DEVICECAPABILITIES,	
    FUNC_ADVANCEDSETUPDIALOG,			
    FUNC_DIALOGFN,		
    FUNC_PSEUDOEDIT,
    TOTAL_PRINTER_DRIVER_FUNCTIONS /* insert functions before here */
};

typedef struct PRINTER_FONTS_INFO
{
    LOGFONT16		lf;			/* LogFont infomation */
    TEXTMETRIC16	tm;			/* Text metrics infomation */
} PRINTER_FONTS_INFO;

typedef struct
{
    char 	szDriver[9];		/* Driver name eg EPSON */
    HINSTANCE16	hInst;			/* Handle for driver */
    WORD	ds_reg;			/* DS of driver */
    FARPROC16 	fn[TOTAL_PRINTER_DRIVER_FUNCTIONS];	/* Printer functions */
    int		nUsageCount;		/* Usage count, unload == 0 */
    int		nPrinterFonts;		/* Number of printer fonts */
    PRINTER_FONTS_INFO *paPrinterFonts; /* array of printer fonts */
    int		nIndex;			/* Index in global driver array */
    HGLOBAL16   hThunk;			/* Thunking buffer */
    SEGPTR	ThunkBufSegPtr;
    SEGPTR	ThunkBufLimit;
} LOADED_PRINTER_DRIVER;

typedef struct PDEVICE_HEADER
{
    LOADED_PRINTER_DRIVER *pLPD;	/* Associated printer driver */
} PDEVICE_HEADER;

typedef short SHORT; 

#pragma pack(1)
#define PCOLOR DWORD
typedef struct DRAWMODE 
{
    SHORT    Rop2;       
    SHORT    bkMode;     
    PCOLOR   bkColor;    
    PCOLOR   TextColor;  
    SHORT    TBreakExtra;
    SHORT    BreakExtra; 
    SHORT    BreakErr;   
    SHORT    BreakRem;   
    SHORT    BreakCount; 
    SHORT    CharExtra;  
    COLORREF LbkColor;   
    COLORREF LTextColor; 
} DRAWMODE, *LPDRAWMODE;


#pragma pack(4)

typedef struct WINE_ENUM_PRINTER_FONT_CALLBACK
{
    DWORD	magic;			/* magic number */
    int 	nMode;			/* Mode 0=count, 1=store */
    int 	nCount;			/* Callback count */
    LOADED_PRINTER_DRIVER *pLPD;    	/* Printer driver info */
} WEPFC;

#define OBJ_PEN 	1       
#define OBJ_BRUSH 	2  
#define OBJ_FONT 	3   
#define OBJ_PBITMAP 	5

/* Win16 printer driver physical DC */
typedef struct
{
    SEGPTR	segptrPDEVICE;	/* PDEVICE used by 16 bit printer drivers */
    LOGFONT16	lf;		/* Current font details */
    TEXTMETRIC16	tm;		/* Current font metrics */
    SEGPTR	segptrFontInfo; /* Current font realized by printer driver */
} WIN16DRV_PDEVICE;

/*
 * Printer driver functions
 */
typedef SEGPTR LPPDEVICE;
LOADED_PRINTER_DRIVER *LoadPrinterDriver(const char *pszDriver);

extern INT16 PRTDRV_Control(LPPDEVICE lpDestDev, WORD wfunction, SEGPTR lpInData, SEGPTR lpOutData);
extern WORD PRTDRV_Enable(LPVOID lpDevInfo, WORD wStyle, LPCSTR  lpDestDevType, 
                          LPCSTR lpDeviceName, LPCSTR lpOutputFile, LPVOID lpData);
extern WORD PRTDRV_EnumDFonts(LPPDEVICE lpDestDev, LPSTR lpFaceName,  
		       FARPROC16 lpCallbackFunc, LPVOID lpClientData);
extern DWORD PRTDRV_RealizeObject(LPPDEVICE lpDestDev, WORD wStyle, 
				  LPVOID lpInObj, LPVOID lpOutObj,
				  LPTEXTXFORM16 lpTextXForm);

extern BOOL16 PRTDRV_EnumObj(LPPDEVICE lpDestDev, WORD iStyle, FARPROC16 lpfn, LPVOID lpb);
extern DWORD PRTDRV_ExtTextOut(LPPDEVICE lpDestDev, WORD wDestXOrg, WORD wDestYOrg,
			       RECT16 *lpClipRect, LPCSTR lpString, WORD wCount, 
			       SEGPTR lpFontInfo, LPDRAWMODE lpDrawMode, 
			       LPTEXTXFORM16 lpTextXForm, SHORT *lpCharWidths,
			       RECT16 *     lpOpaqueRect, WORD wOptions);


/* Wine driver functions */

extern BOOL32 WIN16DRV_GetCharWidth( struct tagDC *dc, UINT32 firstChar, UINT32 lastChar,
				   LPINT32 buffer );

extern BOOL32 WIN16DRV_GetTextExtentPoint( DC *dc, LPCSTR str, INT32 count,
                                           LPSIZE32 size );
extern BOOL32 WIN16DRV_GetTextMetrics( DC *dc, TEXTMETRIC32A *metrics );

extern BOOL32 WIN16DRV_ExtTextOut( DC *dc, INT32 x, INT32 y, UINT32 flags,
                                  const RECT32 *lprect, LPCSTR str, UINT32 count,
                                  const INT32 *lpDx );
extern HGDIOBJ32 WIN16DRV_SelectObject( DC *dc, HGDIOBJ32 handle );



#endif  /* __WINE_WIN16DRV_H */
