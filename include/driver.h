/*
 * Drivers definitions
 */

#ifndef __WINE_DRIVER_H
#define __WINE_DRIVER_H

#include "windef.h"

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
#define DRVCNF_RESTART 		0x0002

#define DRVEA_NORMALEXIT  	0x0001
#define DRVEA_ABNORMALEXIT 	0x0002

#define DRV_SUCCESS		0x0001
#define DRV_FAILURE		0x0000

#define GND_FIRSTINSTANCEONLY 	0x00000001

#define GND_FORWARD  		0x00000000
#define GND_REVERSE    		0x00000002

typedef struct {
    DWORD   			dwDCISize;
    LPCSTR  			lpszDCISectionName;
    LPCSTR  			lpszDCIAliasName;
} DRVCONFIGINFO16, *LPDRVCONFIGINFO16;

typedef struct {
    DWORD   			dwDCISize;
    LPCWSTR  			lpszDCISectionName;
    LPCWSTR  			lpszDCIAliasName;
} DRVCONFIGINFO, *LPDRVCONFIGINFO;


/* GetDriverInfo16 references this structure, so this a struct defined
 * in the Win16 API.
 * GetDriverInfo has been deprecated in Win32.
 */
typedef struct
{
    UINT16       		length;
    HDRVR16      		hDriver;
    HINSTANCE16  		hModule;
    CHAR         		szAliasName[128];
} DRIVERINFOSTRUCT16, *LPDRIVERINFOSTRUCT16;

LRESULT WINAPI DefDriverProc16(DWORD dwDevID, HDRVR16 hDriv, UINT16 wMsg, 
                               LPARAM dwParam1, LPARAM dwParam2);
LRESULT WINAPI DefDriverProc(DWORD dwDriverIdentifier, HDRVR hdrvr,
                               UINT Msg, LPARAM lParam1, LPARAM lParam2);
HDRVR16 WINAPI OpenDriver16(LPCSTR szDriverName, LPCSTR szSectionName,
                            LPARAM lParam2);
HDRVR WINAPI OpenDriverA(LPCSTR szDriverName, LPCSTR szSectionName,
                             LPARAM lParam2);
HDRVR WINAPI OpenDriverW(LPCWSTR szDriverName, LPCWSTR szSectionName,
                             LPARAM lParam2);
#define OpenDriver WINELIB_NAME_AW(OpenDriver)
LRESULT WINAPI CloseDriver16(HDRVR16 hDriver, LPARAM lParam1, LPARAM lParam2);
LRESULT WINAPI CloseDriver(HDRVR hDriver, LPARAM lParam1, LPARAM lParam2);
LRESULT WINAPI SendDriverMessage16( HDRVR16 hDriver, UINT16 message,
                                    LPARAM lParam1, LPARAM lParam2 );
LRESULT WINAPI SendDriverMessage( HDRVR hDriver, UINT message,
                                    LPARAM lParam1, LPARAM lParam2 );
HMODULE16 WINAPI GetDriverModuleHandle16(HDRVR16 hDriver);
HMODULE WINAPI GetDriverModuleHandle(HDRVR hDriver);

/* only win31 version for those below ? */
HDRVR16 WINAPI GetNextDriver16(HDRVR16, DWORD);
BOOL16 WINAPI GetDriverInfo16(HDRVR16, DRIVERINFOSTRUCT16 *);

/* The following definitions are WINE internals */
/* FIXME: This is a WINE internal struct and should be moved in include/wine directory
 * Please note that WINE shares 16 and 32 bit drivers on a single list...
 * Basically, we maintain an external double view on drivers, so that a 16 bit drivers 
 * can be loaded/used... by 32 functions transparently 
 */

/* Who said goofy boy ? */
#define	WINE_DI_MAGIC	0x900F1B01

typedef struct tagWINE_DRIVER
{
    DWORD			dwMagic;
    char			szAliasName[128];
    /* as usual LPWINE_DRIVER == hDriver32 */
    HDRVR16			hDriver16;
    union {
       struct {
	  HMODULE16		hModule;
	  DRIVERPROC16          lpDrvProc;
       } d16;
       struct {
	  HMODULE		hModule;
	  DRIVERPROC		lpDrvProc;
       } d32;
    } d;
    DWORD		  	dwDriverID;
    DWORD			dwFlags;
    struct tagWINE_DRIVER*	lpPrevItem;
    struct tagWINE_DRIVER*	lpNextItem;
} WINE_DRIVER, *LPWINE_DRIVER;

/* values for dwFlags */
#define WINE_DI_TYPE_MASK	0x00000007ul
#define WINE_DI_TYPE_16		0x00000001ul
#define WINE_DI_TYPE_32		0x00000002ul

LPWINE_DRIVER	DRIVER_RegisterDriver16(LPCSTR, HMODULE16, DRIVERPROC16, LPARAM, BOOL);
LPWINE_DRIVER	DRIVER_RegisterDriver32(LPCSTR, HMODULE,   DRIVERPROC,   LPARAM, BOOL);
int		DRIVER_GetType(HDRVR);

#if 0
#errro "it's never used"
/* internal */
typedef struct
{
    UINT			length;
    HDRVR			hDriver;
    HMODULE			hModule;
    CHAR			szAliasName[128];
} DRIVERINFOSTRUCTA, *LPDRIVERINFOSTRUCTA;
#endif

#endif  /* __WINE_DRIVER_H */
