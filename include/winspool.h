/* Definitions for printing
 *
 * Copyright 1998 Huw Davies, Andreas Mohr
 *
 * Portions Copyright (c) 1999 Corel Corporation 
 *                             (Paul Quinn, Albert Den Haan)
 */
#ifndef __WINE_WINSPOOL_H
#define __WINE_WINSPOOL_H

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DEFINES */

#define PRINTER_ATTRIBUTE_QUEUED         0x00000001
#define PRINTER_ATTRIBUTE_DIRECT         0x00000002
#define PRINTER_ATTRIBUTE_DEFAULT        0x00000004
#define PRINTER_ATTRIBUTE_SHARED         0x00000008
#define PRINTER_ATTRIBUTE_NETWORK        0x00000010
#define PRINTER_ATTRIBUTE_HIDDEN         0x00000020
#define PRINTER_ATTRIBUTE_LOCAL          0x00000040

#define PRINTER_ATTRIBUTE_ENABLE_DEVQ       0x00000080
#define PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS   0x00000100
#define PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST 0x00000200

#define PRINTER_ATTRIBUTE_WORK_OFFLINE   0x00000400
#define PRINTER_ATTRIBUTE_ENABLE_BIDI    0x00000800

#define PRINTER_ENUM_DEFAULT     0x00000001
#define PRINTER_ENUM_LOCAL       0x00000002
#define PRINTER_ENUM_CONNECTIONS 0x00000004
#define PRINTER_ENUM_FAVORITE    0x00000004
#define PRINTER_ENUM_NAME        0x00000008
#define PRINTER_ENUM_REMOTE      0x00000010
#define PRINTER_ENUM_SHARED      0x00000020
#define PRINTER_ENUM_NETWORK     0x00000040

#define PRINTER_ENUM_EXPAND      0x00004000
#define PRINTER_ENUM_CONTAINER   0x00008000

#define PRINTER_ENUM_ICONMASK    0x00ff0000
#define PRINTER_ENUM_ICON1       0x00010000
#define PRINTER_ENUM_ICON2       0x00020000
#define PRINTER_ENUM_ICON3       0x00040000
#define PRINTER_ENUM_ICON4       0x00080000
#define PRINTER_ENUM_ICON5       0x00100000
#define PRINTER_ENUM_ICON6       0x00200000
#define PRINTER_ENUM_ICON7       0x00400000
#define PRINTER_ENUM_ICON8       0x00800000

/* TYPES */
typedef struct _PRINTER_DEFAULTSA {
  LPSTR        pDatatype;
  LPDEVMODEA pDevMode;
  ACCESS_MASK  DesiredAccess;
} PRINTER_DEFAULTSA, *LPPRINTER_DEFAULTSA;

typedef struct _PRINTER_DEFAULTSW {
  LPWSTR       pDatatype;
  LPDEVMODEW pDevMode;
  ACCESS_MASK  DesiredAccess;
} PRINTER_DEFAULTSW, *LPPRINTER_DEFAULTSW;

DECL_WINELIB_TYPE_AW(PRINTER_DEFAULTS)
DECL_WINELIB_TYPE_AW(LPPRINTER_DEFAULTS)

typedef struct _DRIVER_INFO_1A {
  LPSTR     pName;
} DRIVER_INFO_1A, *PDRIVER_INFO_1A, *LPDRIVER_INFO_1A;

typedef struct _DRIVER_INFO_1W {
  LPWSTR    pName;
} DRIVER_INFO_1W, *PDRIVER_INFO_1W, *LPDRIVER_INFO_1W;

DECL_WINELIB_TYPE_AW(DRIVER_INFO_1)
DECL_WINELIB_TYPE_AW(PDRIVER_INFO_1)
DECL_WINELIB_TYPE_AW(LPDRIVER_INFO_1)

typedef struct _DRIVER_INFO_2A {
  DWORD   cVersion;
  LPSTR     pName;
  LPSTR     pEnvironment;
  LPSTR     pDriverPath;
  LPSTR     pDataFile; 
  LPSTR     pConfigFile;
} DRIVER_INFO_2A, *PDRIVER_INFO_2A, *LPDRIVER_INFO_2A;

typedef struct _DRIVER_INFO_2W {
  DWORD   cVersion;
  LPWSTR    pName;     
  LPWSTR    pEnvironment;
  LPWSTR    pDriverPath;
  LPWSTR    pDataFile; 
  LPWSTR    pConfigFile;
} DRIVER_INFO_2W, *PDRIVER_INFO_2W, *LPDRIVER_INFO_2W;

DECL_WINELIB_TYPE_AW(DRIVER_INFO_2)
DECL_WINELIB_TYPE_AW(PDRIVER_INFO_2)
DECL_WINELIB_TYPE_AW(LPDRIVER_INFO_2)

typedef struct _PRINTER_INFO_1A {
  DWORD   Flags;
  LPSTR   pDescription;
  LPSTR   pName;
  LPSTR   pComment;
} PRINTER_INFO_1A, *PPRINTER_INFO_1A, *LPPRINTER_INFO_1A;

typedef struct _PRINTER_INFO_1W {
  DWORD   Flags;
  LPWSTR  pDescription;
  LPWSTR  pName;
  LPWSTR  pComment;
} PRINTER_INFO_1W, *PPRINTER_INFO_1W, *LPPRINTER_INFO_1W;

DECL_WINELIB_TYPE_AW(PRINTER_INFO_1)
DECL_WINELIB_TYPE_AW(PPRINTER_INFO_1)
DECL_WINELIB_TYPE_AW(LPPRINTER_INFO_1)

/* FIXME: winspool.h declares some structure members with the name Status.
 * unfortunatly <X11/ICE/ICElib.h> #defines Status to the type 'int' 
 * therfore the following hack */
#ifndef Status

typedef struct _PRINTER_INFO_2A {
  LPSTR     pServerName;
  LPSTR     pPrinterName;
  LPSTR     pShareName;
  LPSTR     pPortName;
  LPSTR     pDriverName;
  LPSTR     pComment;
  LPSTR     pLocation;
  LPDEVMODEA pDevMode;
  LPSTR     pSepFile;
  LPSTR     pPrintProcessor;
  LPSTR     pDatatype;
  LPSTR     pParameters;
  PSECURITY_DESCRIPTOR pSecurityDescriptor;
  DWORD   Attributes;
  DWORD   Priority;
  DWORD   DefaultPriority;
  DWORD   StartTime;
  DWORD   UntilTime;
  DWORD   Status;
  DWORD   cJobs;
  DWORD   AveragePPM;
} PRINTER_INFO_2A, *PPRINTER_INFO_2A, *LPPRINTER_INFO_2A;

typedef struct _PRINTER_INFO_2W {
  LPWSTR    pServerName;
  LPWSTR    pPrinterName;
  LPWSTR    pShareName;
  LPWSTR    pPortName;
  LPWSTR    pDriverName;
  LPWSTR    pComment;
  LPWSTR    pLocation;
  LPDEVMODEW pDevMode;
  LPWSTR    pSepFile;
  LPWSTR    pPrintProcessor;
  LPWSTR    pDatatype;
  LPWSTR    pParameters;
  PSECURITY_DESCRIPTOR pSecurityDescriptor;
  DWORD   Attributes;
  DWORD   Priority;
  DWORD   DefaultPriority;
  DWORD   StartTime;
  DWORD   UntilTime;
  DWORD   Status;
  DWORD   cJobs;
  DWORD   AveragePPM;
} PRINTER_INFO_2W, *PPRINTER_INFO_2W, *LPPRINTER_INFO_2W;

DECL_WINELIB_TYPE_AW(PRINTER_INFO_2)
DECL_WINELIB_TYPE_AW(PPRINTER_INFO_2)
DECL_WINELIB_TYPE_AW(LPPRINTER_INFO_2)

typedef struct _PRINTER_INFO_4A {
  LPSTR     pPrinterName;
  LPSTR     pServerName;
  DWORD     Attributes;
} PRINTER_INFO_4A, *PPRINTER_INFO_4A, *LPPRINTER_INFO_4A;

typedef struct _PRINTER_INFO_4W {
  LPWSTR     pPrinterName;
  LPWSTR     pServerName;
  DWORD     Attributes;
} PRINTER_INFO_4W, *PPRINTER_INFO_4W, *LPPRINTER_INFO_4W;

DECL_WINELIB_TYPE_AW(PRINTER_INFO_4)
DECL_WINELIB_TYPE_AW(PPRINTER_INFO_4)
DECL_WINELIB_TYPE_AW(LPPRINTER_INFO_4)

typedef struct _PRINTER_INFO_5A {
  LPSTR     pPrinterName;
  LPSTR     pPortName;
  DWORD     Attributes;
  DWORD     DeviceNotSelectedTimeOut;
  DWORD     TransmissionRetryTimeout;
} PRINTER_INFO_5A, *PPRINTER_INFO_5A, *LPPRINTER_INFO_5A;

typedef struct _PRINTER_INFO_5W {
  LPWSTR    pPrinterName;
  LPWSTR    pPortName;
  DWORD     Attributes;
  DWORD     DeviceNotSelectedTimeOut;
  DWORD     TransmissionRetryTimeout;
} PRINTER_INFO_5W, *PPRINTER_INFO_5W, *LPPRINTER_INFO_5W;

DECL_WINELIB_TYPE_AW(PRINTER_INFO_5)
DECL_WINELIB_TYPE_AW(PPRINTER_INFO_5)
DECL_WINELIB_TYPE_AW(LPPRINTER_INFO_5)

#endif /* Status */

/* DECLARATIONS */
DWORD WINAPI DrvGetPrinterData16(LPSTR lpPrinter, LPSTR lpProfile,
	  LPDWORD lpType, LPBYTE lpPrinterData, int cbData, LPDWORD lpNeeded);
DWORD WINAPI DrvSetPrinterData16(LPSTR lpPrinter, LPSTR lpProfile,
          DWORD lpType, LPBYTE lpPrinterData, DWORD dwSize);
HANDLE16 WINAPI OpenJob16(LPSTR lpOutput, LPSTR lpTitle, HDC16 hDC);
int WINAPI CloseJob16(HANDLE16 hJob);
int WINAPI WriteSpool16(HANDLE16 hJob, LPSTR lpData, WORD cch);
int WINAPI DeleteJob16(HANDLE16 hJob, WORD wNotUsed);
int WINAPI StartSpoolPage16(HANDLE16 hJob);
int WINAPI EndSpoolPage16(HANDLE16 hJob);
DWORD WINAPI GetSpoolJob16(int nOption, LONG param);
int WINAPI WriteDialog16(HANDLE16 hJob, LPSTR lpMsg, WORD cchMsg);

INT WINAPI DeviceCapabilitiesA(LPCSTR pDevice,LPCSTR pPort,WORD fwCapability,
			       LPSTR pOutput, LPDEVMODEA pDevMode);
INT WINAPI DeviceCapabilitiesW(LPCWSTR pDevice, LPCWSTR pPort,
			       WORD fwCapability, LPWSTR pOutput,
			       const DEVMODEW *pDevMode);

#define DeviceCapabilities WINELIB_NAME_AW(DeviceCapabilities)

LONG WINAPI DocumentPropertiesA(HWND hWnd,HANDLE hPrinter,
                                LPSTR pDeviceName, LPDEVMODEA pDevModeOutput,
                                  LPDEVMODEA pDevModeInput,DWORD fMode );
LONG WINAPI DocumentPropertiesW(HWND hWnd, HANDLE hPrinter,
                                  LPWSTR pDeviceName,
                                  LPDEVMODEW pDevModeOutput,
                                  LPDEVMODEW pDevModeInput, DWORD fMode);

#define DocumentProperties WINELIB_NAME_AW(DocumentProperties)

BOOL WINAPI OpenPrinterA(LPSTR lpPrinterName,HANDLE *phPrinter,
			     LPPRINTER_DEFAULTSA pDefault);
BOOL WINAPI OpenPrinterW(LPWSTR lpPrinterName,HANDLE *phPrinter,
			     LPPRINTER_DEFAULTSW pDefault);

#define OpenPrinter WINELIB_NAME_AW(OpenPrinter)

BOOL WINAPI ClosePrinter (HANDLE phPrinter);

BOOL  WINAPI EnumPrintersA(DWORD dwType, LPSTR lpszName,
			       DWORD dwLevel, LPBYTE lpbPrinters,
			       DWORD cbBuf, LPDWORD lpdwNeeded,
			       LPDWORD lpdwReturned);
BOOL  WINAPI EnumPrintersW(DWORD dwType, LPWSTR lpszName,
			       DWORD dwLevel, LPBYTE lpbPrinters,
			       DWORD cbBuf, LPDWORD lpdwNeeded,
			       LPDWORD lpdwReturned);
#define EnumPrinters WINELIB_NAME_AW(EnumPrinters)

BOOL WINAPI PrinterProperties(HWND hWnd, HANDLE hPrinter);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* __WINE_WINSPOOL_H */

