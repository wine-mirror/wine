#ifndef __SETUPX16_H
#define __SETUPX16_H

#include "wine/windef16.h"

typedef UINT16 HINF16;
typedef UINT16 LOGDISKID16;

#define LINE_LEN	256

/* error codes stuff */

typedef UINT16 RETERR16;
#define OK		0
#define IP_ERROR	(UINT16)100
#define TP_ERROR	(UINT16)200
#define VCP_ERROR	(UINT16)300
#define GEN_ERROR	(UINT16)400
#define DI_ERROR	(UINT16)500

enum _IP_ERR {
	ERR_IP_INVALID_FILENAME = IP_ERROR+1,
	ERR_IP_ALLOC_ERR,
	ERR_IP_INVALID_SECT_NAME,
	ERR_IP_OUT_OF_HANDLES,
	ERR_IP_INF_NOT_FOUND,
	ERR_IP_INVALID_INFFILE,
	ERR_IP_INVALID_HINF,
	ERR_IP_INVALID_FIELD,
	ERR_IP_SECT_NOT_FOUND,
	ERR_IP_END_OF_SECTION,
	ERR_IP_PROFILE_NOT_FOUND,
	ERR_IP_LINE_NOT_FOUND,
	ERR_IP_FILEREAD,
	ERR_IP_TOOMANYINFFILES,
	ERR_IP_INVALID_SAVERESTORE,
	ERR_IP_INVALID_INFTYPE
};

enum _ERR_VCP {
	ERR_VCP_IOFAIL = VCP_ERROR+1,
	ERR_VCP_STRINGTOOLONG,
	ERR_VCP_NOMEM,
	ERR_VCP_QUEUEFULL,
	ERR_VCP_NOVHSTR,
	ERR_VCP_OVERFLOW,
	ERR_VCP_BADARG,
	ERR_VCP_UNINIT,
	ERR_VCP_NOTFOUND,
	ERR_VCP_BUSY,
	ERR_VCP_INTERRUPTED,
	ERR_VCP_BADDEST,
	ERR_VCP_SKIPPED,
	ERR_VCP_IO,
	ERR_VCP_LOCKED,
	ERR_VCP_WRONGDISK,
	ERR_VCP_CHANGEMODE,
	ERR_VCP_LDDINVALID,
	ERR_VCP_LDDFIND,
	ERR_VCP_LDDUNINIT,
	ERR_VCP_LDDPATH_INVALID,
	ERR_VCP_NOEXPANSION,
	ERR_VCP_NOTOPEN,
	ERR_VCP_NO_DIGITAL_SIGNATURE_CATALOG,
	ERR_VCP_NO_DIGITAL_SIGNATURE_FILE
};

/****** logical disk management ******/

typedef struct _LOGDISKDESC_S { /* ldd */
	WORD        cbSize;       /* struct size */
	LOGDISKID16 ldid;         /* logical disk ID */
	LPSTR       pszPath;      /* path this descriptor points to */
	LPSTR       pszVolLabel;  /* volume label of the disk related to it */
	LPSTR       pszDiskName;  /* name of this disk */
	WORD        wVolTime;     /* modification time of volume label */
	WORD   	    wVolDate;     /* modification date */
	DWORD       dwSerNum;     /* serial number of disk */
	WORD        wFlags;
} LOGDISKDESC_S, *LPLOGDISKDESC;

/** logical disk identifiers (LDID) **/

/* predefined LDIDs */
#define LDID_PREDEF_START	0x0001
#define LDID_PREDEF_END		0x7fff

/* registry-assigned LDIDs */
#define LDID_VAR_START		0x7000
#define LDID_VAR_END		0x7fff

/* dynamically assigned LDIDs */
#define LDID_ASSIGN_START	0x8000
#define LDID_ASSIGN_END		0xbfff

#define LDID_NULL		0
#define LDID_ABSOLUTE		((UINT)-1)
#define LDID_SRCPATH		1		/* setup source path */
#define LDID_SETUPTEMP		2		/* setup temp dir */
#define LDID_UNINSTALL		3		/* uninstall dir */
#define LDID_BACKUP		4		/* backup dir */
#define LDID_SETUPSCRATCH	5		/* setup scratch dir */
#define LDID_WIN		10		/* win dir */
#define LDID_SYS		11		/* win system dir */
#define LDID_IOS		12		/* win Iosubsys dir */
#define LDID_CMD		13		/* win command dir */
#define LDID_CPL		14		/* win control panel dir */
#define LDID_PRINT		15		/* win printer dir */
#define LDID_MAIL		16		/* win mail dir */
#define LDID_INF		17		/* win inf dir */
#define LDID_HELP		18		/* win help dir */
#define LDID_WINADMIN		19		/* admin dir */
#define LDID_FONTS		20		/* win fonts dir */
#define LDID_VIEWERS		21		/* win viewers dir */
#define LDID_VMM32		22		/* win VMM32 dir */
#define LDID_COLOR		23		/* win color mngment dir */
#define LDID_APPS		24		/* win apps dir */
#define LDID_SHARED		25		/* win shared dir */
#define LDID_WINBOOT		26		/* guaranteed win boot drive */
#define LDID_MACHINE		27		/* machine specific files */
#define LDID_HOST_WINBOOT	28
#define LDID_BOOT		30		/* boot drive root dir */
#define LDID_BOOT_HOST		31		/* boot drive host root dir */
#define LDID_OLD_WINBOOT	32		/* root subdir */
#define LDID_OLD_WIN		33		/* old windows dir */

/* flags for GenInstall() */
#define GENINSTALL_DO_FILES		1
#define GENINSTALL_DO_INI		2
#define GENINSTALL_DO_REG		4
#define GENINSTALL_DO_INI2REG		8
#define GENINSTALL_DO_CFGAUTO		16
#define GENINSTALL_DO_LOGCONFIG		32
#define GENINSTALL_DO_REGSRCPATH	64
#define GENINSTALL_DO_PERUSER		128

#define GEINISTALL_DO_INIREG		14
#define GENINSTALL_DO_ALL		255

/*
 * flags for InstallHinfSection()
 * 128 can be added, too. This means that the .inf file is provided by you
 * instead of being a 32 bit file (i.e. Windows .inf file).
 * In this case all files you install must be in the same dir
 * as your .inf file on the install disk.
 */
#define HOW_NEVER_REBOOT		0
#define HOW_ALWAYS_SILENT_REBOOT	1
#define HOW_ALWAYS_PROMPT_REBOOT	2
#define HOW_SILENT_REBOOT		3
#define HOW_PROMPT_REBOOT		4

/****** device installation stuff ******/

#define MAX_CLASS_NAME_LEN	32
#define MAX_DEVNODE_ID_LEN	256
#define MAX_GUID_STR		50

typedef struct _DEVICE_INFO
{
	UINT16              cbSize;
	struct _DEVICE_INFO *lpNextDi;
	char                szDescription[LINE_LEN];
	DWORD               dnDevnode;
	HKEY                hRegKey;
	char                szRegSubkey[MAX_DEVNODE_ID_LEN];
	char                szClassName[MAX_CLASS_NAME_LEN];
	DWORD               Flags;
	HWND16              hwndParent;
	/*LPDRIVER_NODE*/ LPVOID      lpCompatDrvList;
	/*LPDRIVER_NODE*/ LPVOID      lpClassDrvList;
	/*LPDRIVER_NODE*/ LPVOID      lpSelectedDriver;
	ATOM                atDriverPath;
	ATOM                atTempInfFile;
	HINSTANCE16         hinstClassInstaller;
	HINSTANCE16         hinstClassPropProvidor;
	HINSTANCE16         hinstDevicePropProvidor;
	HINSTANCE16         hinstBasicPropProvidor;
	FARPROC16           fpClassInstaller;
	FARPROC16           fpClassEnumPropPages;
	FARPROC16           fpDeviceEnumPropPages;
	FARPROC16           fpEnumBasicProperties;
	DWORD               dwSetupReserved;
	DWORD               dwClassInstallReserved;
	/*GENCALLBACKPROC*/ LPVOID     gicpGenInstallCallBack;

	LPARAM              gicplParam;
	UINT16              InfType;

	HINSTANCE16         hinstPrivateProblemHandler;
	FARPROC16           fpPrivateProblemHandler;
	LPARAM              lpClassInstallParams;
	struct _DEVICE_INFO *lpdiChildList;
	DWORD               dwFlagsEx;
	/*LPDRIVER_INFO*/ LPVOID       lpCompatDrvInfoList;
	/*LPDRIVER_INFO*/ LPVOID      lpClassDrvInfoList;
	char                szClassGUID[MAX_GUID_STR];
} DEVICE_INFO16, *LPDEVICE_INFO16, **LPLPDEVICE_INFO16;


typedef LRESULT (CALLBACK *VIFPROC)(LPVOID lpvObj, UINT uMsg, WPARAM wParam, LPARAM lParam, LPARAM lparamRef);

extern void WINAPI GenFormStrWithoutPlaceHolders16(LPSTR,LPCSTR,HINF16);
extern RETERR16 WINAPI IpOpen16(LPCSTR,HINF16 *);
extern RETERR16 WINAPI IpClose16(HINF16);
extern RETERR16 WINAPI CtlSetLdd16(LPLOGDISKDESC);
extern RETERR16 WINAPI CtlGetLdd16(LPLOGDISKDESC);
extern RETERR16 WINAPI CtlFindLdd16(LPLOGDISKDESC);
extern RETERR16 WINAPI CtlAddLdd16(LPLOGDISKDESC);
extern RETERR16 WINAPI CtlDelLdd16(LOGDISKID16);
extern RETERR16 WINAPI GenInstall16(HINF16,LPCSTR,WORD);

#endif /* __SETUPX16_H */
