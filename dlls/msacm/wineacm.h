/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/***********************************************************************
 * Wine specific - Win32
 */
typedef struct _WINE_ACMDRIVERID *PWINE_ACMDRIVERID;
typedef struct _WINE_ACMDRIVER   *PWINE_ACMDRIVER;

#define WINE_ACMOBJ_DONTCARE	0x5EED0000
#define WINE_ACMOBJ_DRIVERID	0x5EED0001
#define WINE_ACMOBJ_DRIVER	0x5EED0002
#define WINE_ACMOBJ_STREAM	0x5EED0003

typedef struct _WINE_ACMOBJ
{
    DWORD		dwType;
    PWINE_ACMDRIVERID	pACMDriverID;
} WINE_ACMOBJ, *PWINE_ACMOBJ;

typedef struct _WINE_ACMDRIVER
{
    WINE_ACMOBJ		obj;
    HDRVR      		hDrvr;
    DRIVERPROC		pfnDriverProc;
    PWINE_ACMDRIVER	pNextACMDriver;
} WINE_ACMDRIVER;

typedef struct _WINE_ACMSTREAM
{
    WINE_ACMOBJ		obj;
    PWINE_ACMDRIVER	pDrv;
    ACMDRVSTREAMINSTANCE drvInst;
    HACMDRIVER		hAcmDriver;
} WINE_ACMSTREAM, *PWINE_ACMSTREAM;

typedef struct _WINE_ACMDRIVERID
{
    WINE_ACMOBJ		obj;
    LPSTR               pszDriverAlias;
    LPSTR               pszFileName;
    HINSTANCE		hInstModule;          /* NULL if global */
    DWORD		dwProcessID;	      /* ID of process which installed a local driver */
    BOOL                bEnabled;
    PWINE_ACMDRIVER     pACMDriverList;
    PWINE_ACMDRIVERID   pNextACMDriverID;
    PWINE_ACMDRIVERID	pPrevACMDriverID;
} WINE_ACMDRIVERID;

/* From internal.c */
extern HANDLE MSACM_hHeap;
extern PWINE_ACMDRIVERID MSACM_pFirstACMDriverID;
extern PWINE_ACMDRIVERID MSACM_pLastACMDriverID;
extern PWINE_ACMDRIVERID MSACM_RegisterDriver(LPSTR pszDriverAlias, LPSTR pszFileName,
					      HINSTANCE hinstModule);
extern void MSACM_RegisterAllDrivers(void);
extern PWINE_ACMDRIVERID MSACM_UnregisterDriver(PWINE_ACMDRIVERID p);
extern void MSACM_UnregisterAllDrivers(void);
extern PWINE_ACMDRIVERID MSACM_GetDriverID(HACMDRIVERID hDriverID);
extern PWINE_ACMDRIVER MSACM_GetDriver(HACMDRIVER hDriver);
extern PWINE_ACMOBJ MSACM_GetObj(HACMOBJ hObj, DWORD type);

extern MMRESULT MSACM_Message(HACMDRIVER, UINT, LPARAM, LPARAM);

/* From msacm32.c */
extern HINSTANCE MSACM_hInstance32;

/* Dialog box templates */
#define DLG_ACMFORMATCHOOSE_ID              70
#define IDD_ACMFORMATCHOOSE_BTN_HELP        9
#define IDD_ACMFORMATCHOOSE_CMB_CUSTOM      100
#define IDD_ACMFORMATCHOOSE_CMB_FORMATTAG   101
#define IDD_ACMFORMATCHOOSE_CMB_FORMAT      102
#define IDD_ACMFORMATCHOOSE_BTN_SETNAME     103
#define IDD_ACMFORMATCHOOSE_BTN_DELNAME     104

#define DLG_ACMFILTERCHOOSE_ID              71
#define IDD_ACMFILTERCHOOSE_BTN_HELP        9
#define IDD_ACMFILTERCHOOSE_CMB_CUSTOM      100
#define IDD_ACMFILTERCHOOSE_CMB_FILTERTAG   101
#define IDD_ACMFILTERCHOOSE_CMB_FILTER      102
#define IDD_ACMFILTERCHOOSE_BTN_SETNAME     103
#define IDD_ACMFILTERCHOOSE_BTN_DELNAME     104
