/* Definitions for printing
 *
 * Copyright 1998 Huw Davies, Andreas Mohr
 *
 * Portions Copyright (c) 1999 Corel Corporation 
 *                             (Paul Quinn, Albert Den Haan)
 */
#ifndef __WINE_WINSPOOL_H
#define __WINE_WINSPOOL_H

#include "wintypes.h"
#include "winbase.h"
#include "wingdi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DEFINES */
#define INT_PD_DEFAULT_DEVMODE  1
#define INT_PD_DEFAULT_MODEL    2

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
typedef struct _PRINTER_DEFAULTS32A {
  LPSTR        pDatatype;
  LPDEVMODE32A pDevMode;
  ACCESS_MASK  DesiredAccess;
} PRINTER_DEFAULTS32A, *LPPRINTER_DEFAULTS32A;

typedef struct _PRINTER_DEFAULTS32W {
  LPWSTR       pDatatype;
  LPDEVMODE32W pDevMode;
  ACCESS_MASK  DesiredAccess;
} PRINTER_DEFAULTS32W, *LPPRINTER_DEFAULTS32W;

DECL_WINELIB_TYPE_AW(PRINTER_DEFAULTS)
DECL_WINELIB_TYPE_AW(LPPRINTER_DEFAULTS)

typedef struct _DRIVER_INFO_132A {
  LPSTR     pName;
} DRIVER_INFO_132A, *PDRIVER_INFO_132A, *LPDRIVER_INFO_132A;

typedef struct _DRIVER_INFO_132W {
  LPWSTR    pName;
} DRIVER_INFO_132W, *PDRIVER_INFO_132W, *LPDRIVER_INFO_132W;

DECL_WINELIB_TYPE_AW(DRIVER_INFO_1)
DECL_WINELIB_TYPE_AW(PDRIVER_INFO_1)
DECL_WINELIB_TYPE_AW(LPDRIVER_INFO_1)

typedef struct _DRIVER_INFO_232A {
  DWORD   cVersion;
  LPSTR     pName;
  LPSTR     pEnvironment;
  LPSTR     pDriverPath;
  LPSTR     pDataFile; 
  LPSTR     pConfigFile;
} DRIVER_INFO_232A, *PDRIVER_INFO_232A, *LPDRIVER_INFO_232A;

typedef struct _DRIVER_INFO_232W {
  DWORD   cVersion;
  LPWSTR    pName;     
  LPWSTR    pEnvironment;
  LPWSTR    pDriverPath;
  LPWSTR    pDataFile; 
  LPWSTR    pConfigFile;
} DRIVER_INFO_232W, *PDRIVER_INFO_232W, *LPDRIVER_INFO_232W;

DECL_WINELIB_TYPE_AW(DRIVER_INFO_2)
DECL_WINELIB_TYPE_AW(PDRIVER_INFO_2)
DECL_WINELIB_TYPE_AW(LPDRIVER_INFO_2)

typedef struct _PRINTER_INFO_132A {
  DWORD   Flags;
  LPSTR   pDescription;
  LPSTR   pName;
  LPSTR   pComment;
} PRINTER_INFO_132A, *PPRINTER_INFO_132A, *LPPRINTER_INFO_132A;

typedef struct _PRINTER_INFO_132W {
  DWORD   Flags;
  LPWSTR  pDescription;
  LPWSTR  pName;
  LPWSTR  pComment;
} PRINTER_INFO_132W, *PPRINTER_INFO_132W, *LPPRINTER_INFO_132W;

DECL_WINELIB_TYPE_AW(PRINTER_INFO_1)
DECL_WINELIB_TYPE_AW(PPRINTER_INFO_1)
DECL_WINELIB_TYPE_AW(LPPRINTER_INFO_1)

/* FIXME: winspool.h declares some structure members with the name Status.
 * unfortunatly <X11/ICE/ICElib.h> #defines Status to the type 'int' 
 * therfore the following hack */
#ifndef Status

typedef struct _PRINTER_INFO_232A {
  LPSTR     pServerName;
  LPSTR     pPrinterName;
  LPSTR     pShareName;
  LPSTR     pPortName;
  LPSTR     pDriverName;
  LPSTR     pComment;
  LPSTR     pLocation;
  LPDEVMODE32A pDevMode;
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
} PRINTER_INFO_232A, *PPRINTER_INFO_232A, *LPPRINTER_INFO_232A;

typedef struct _PRINTER_INFO_232W {
  LPWSTR    pServerName;
  LPWSTR    pPrinterName;
  LPWSTR    pShareName;
  LPWSTR    pPortName;
  LPWSTR    pDriverName;
  LPWSTR    pComment;
  LPWSTR    pLocation;
  LPDEVMODE32W pDevMode;
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
} PRINTER_INFO_232W, *PPRINTER_INFO_232W, *LPPRINTER_INFO_232W;

DECL_WINELIB_TYPE_AW(PRINTER_INFO_2)
DECL_WINELIB_TYPE_AW(PPRINTER_INFO_2)
DECL_WINELIB_TYPE_AW(LPPRINTER_INFO_2)

#endif /* Status */

/* DECLARATIONS */
DWORD WINAPI DrvGetPrinterData(LPSTR lpPrinter, LPSTR lpProfile,
	  LPDWORD lpType, LPBYTE lpPrinterData, int cbData, LPDWORD lpNeeded);
DWORD WINAPI DrvSetPrinterData(LPSTR lpPrinter, LPSTR lpProfile,
          DWORD lpType, LPBYTE lpPrinterData, DWORD dwSize);
HANDLE16 WINAPI OpenJob(LPSTR lpOutput, LPSTR lpTitle, HDC16 hDC);
int WINAPI CloseJob(HANDLE16 hJob);
int WINAPI WriteSpool(HANDLE16 hJob, LPSTR lpData, WORD cch);
int WINAPI DeleteJob(HANDLE16 hJob, WORD wNotUsed);
int WINAPI StartSpoolPage(HANDLE16 hJob);
int WINAPI EndSpoolPage(HANDLE16 hJob);
DWORD WINAPI GetSpoolJob(int nOption, LONG param);
int WINAPI WriteDialog(HANDLE16 hJob, LPSTR lpMsg, WORD cchMsg);

INT32 WINAPI DeviceCapabilities32A(LPCSTR printer,LPCSTR target,WORD z,
                                   LPSTR a,LPDEVMODE32A b);
INT32 WINAPI DeviceCapabilities32W(LPCWSTR pDevice, LPCWSTR pPort,
                                   WORD fwCapability, LPWSTR pOutput,
                                   const DEVMODE32W *pDevMode);

#define DeviceCapabilities WINELIB_NAME_AW(DeviceCapabilities)

LONG WINAPI DocumentProperties32A(HWND32 hWnd,HANDLE32 hPrinter,
                                LPSTR pDeviceName, LPDEVMODE32A pDevModeOutput,
                                  LPDEVMODE32A pDevModeInput,DWORD fMode );
LONG WINAPI DocumentProperties32W(HWND32 hWnd, HANDLE32 hPrinter,
                                  LPWSTR pDeviceName,
                                  LPDEVMODE32W pDevModeOutput,
                                  LPDEVMODE32W pDevModeInput, DWORD fMode);

#define DocumentProperties WINELIB_NAME_AW(DocumentProperties)

BOOL32 WINAPI OpenPrinter32A(LPSTR lpPrinterName,HANDLE32 *phPrinter,
			     LPPRINTER_DEFAULTS32A pDefault);
BOOL32 WINAPI OpenPrinter32W(LPWSTR lpPrinterName,HANDLE32 *phPrinter,
			     LPPRINTER_DEFAULTS32W pDefault);

#define OpenPrinter WINELIB_NAME_AW(OpenPrinter)

BOOL32 WINAPI ClosePrinter32 (HANDLE32 phPrinter);

#define ClosePrinter WINELIB_NAME(ClosePrinter)

#ifdef __cplusplus
} // extern "C"
#endif

#endif  /* __WINE_WINSPOOL_H */

