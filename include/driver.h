/*
 * Drivers definitions
 */

#ifndef __WINE_DRIVER_H
#define __WINE_DRIVER_H

#define DRV_LOAD                0x0001
#define DRV_ENABLE              0x0002
#define DRV_OPEN                0x0003
#define DRV_CLOSE               0x0004
#define DRV_DISABLE             0x0005
#define DRV_FREE                0x0006
#define DRV_CONFIGURE           0x0007
#define DRV_QUERYCONFIGURE      0x0008
#define DRV_INSTALL             0x0009
#define DRV_REMOVE              0x000A
#define DRV_EXITSESSION         0x000B
#define DRV_EXITAPPLICATION     0x000C
#define DRV_POWER               0x000F

#define DRV_RESERVED            0x0800
#define DRV_USER                0x4000

#define DRVCNF_CANCEL           0x0000
#define DRVCNF_OK               0x0001
#define DRVCNF_RESTART 			0x0002

#define DRVEA_NORMALEXIT  		0x0001
#define DRVEA_ABNORMALEXIT 		0x0002

#define GND_FIRSTINSTANCEONLY 	0x00000001

#define GND_FORWARD  			0x00000000
#define GND_REVERSE    			0x00000002

/* FIXME: unused? */
typedef struct {
	DWORD   dwDCISize;
	LPCSTR  lpszDCISectionName;
	LPCSTR  lpszDCIAliasName;
} xDRVCONFIGINFO, *xLPDRVCONFIGINFO;

/* GetDriverInfo16 references this structure, so this a struct defined
 * in the Win16 API.
 */
typedef struct
{
    UINT16       length;
    HDRVR16      hDriver;
    HINSTANCE16  hModule;
    CHAR         szAliasName[128];
} DRIVERINFOSTRUCT16, *LPDRIVERINFOSTRUCT16;

/* FIXME: Is this a WINE internal struct? */
typedef struct tagDRIVERITEM
{
    DRIVERINFOSTRUCT16    dis;
    WORD                  count;
    struct tagDRIVERITEM *lpPrevItem;
    struct tagDRIVERITEM *lpNextItem;
    DRIVERPROC16          lpDrvProc;
} DRIVERITEM, *LPDRIVERITEM;

/* internal */
typedef struct
{
    UINT32	length;
    HDRVR32	hDriver;
    HMODULE32	hModule;
    CHAR	szAliasName[128];
} DRIVERINFOSTRUCT32A, *LPDRIVERINFOSTRUCT32A;

/* internal */
typedef struct tagDRIVERITEM32A {
	DRIVERINFOSTRUCT32A	dis;
	DWORD			count;
	struct tagDRIVERITEM32A	*next;
	DRIVERPROC32		driverproc;
} DRIVERITEM32A,*LPDRIVERITEM32A;

LRESULT WINAPI DefDriverProc(DWORD dwDevID, HDRVR16 hDriv, UINT16 wMsg, 
                             LPARAM dwParam1, LPARAM dwParam2);
HDRVR16 WINAPI OpenDriver16(LPCSTR szDriverName, LPCSTR szSectionName,
                            LPARAM lParam2);
HDRVR32 WINAPI OpenDriver32A(LPCSTR szDriverName, LPCSTR szSectionName,
                             LPARAM lParam2);
HDRVR32 WINAPI OpenDriver32W(LPCWSTR szDriverName, LPCWSTR szSectionName,
                             LPARAM lParam2);
#define OpenDriver WINELIB_NAME_AW(OpenDriver)
LRESULT WINAPI CloseDriver16(HDRVR16 hDriver, LPARAM lParam1, LPARAM lParam2);
LRESULT WINAPI CloseDriver32(HDRVR32 hDriver, LPARAM lParam1, LPARAM lParam2);
#define CloseDriver WINELIB_NAME(CloseDriver)
LRESULT WINAPI SendDriverMessage16( HDRVR16 hDriver, UINT16 message,
                                    LPARAM lParam1, LPARAM lParam2 );
LRESULT WINAPI SendDriverMessage32( HDRVR32 hDriver, UINT32 message,
                                    LPARAM lParam1, LPARAM lParam2 );
#define SendDriverMessage WINELIB_NAME(SendDriverMessage)
HMODULE16 WINAPI GetDriverModuleHandle16(HDRVR16 hDriver);
HMODULE32 WINAPI GetDriverModuleHandle32(HDRVR32 hDriver);
#define GetDriverModuleHandle WINELIB_NAME(GetDriverModuleHandle)

/* only win31 version for those below ? */
HDRVR16 WINAPI GetNextDriver(HDRVR16, DWORD);
BOOL16 WINAPI GetDriverInfo(HDRVR16, DRIVERINFOSTRUCT16 *);

#endif  /* __WINE_DRIVER_H */
