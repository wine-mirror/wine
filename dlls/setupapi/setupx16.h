#ifndef __SETUPX16_H
#define __SETUPX16_H

#include "wine/windef16.h"

typedef UINT16 HINF16;
typedef UINT16 LOGDISKID16;

/* error codes stuff */

typedef UINT16 RETERR16;
#define OK		0
#define IP_ERROR	(UINT16)100

enum _IP_ERR {
	ERR_IP_INVALID_FILENAME = IP_ERROR+1,
	ERR_IP_ALLOC_ERR,
	ERR_IP_INVALID_SECT_NAME,
	ERR_IP_OUT_OF_HANDLES,
	ERR_IP_INF_NOT_FOUND,
	ERR_IP_INVALID_INFFILE,
	ERR_IP_INVALID_HINF,
	ERR_IP_INVALID_FIELD,
	ERR_IP_SECTION_NOT_FOUND,
	ERR_IP_END_OF_SECTION,
	ERR_IP_PROFILE_NOT_FOUND,
	ERR_IP_LINE_NOT_FOUND,
	ERR_IP_FILEREAD,
	ERR_IP_TOOMANYINFFILES,
	ERR_IP_INVALID_SAVERESTORE,
	ERR_IP_INVALID_INFTYPE
};

/* logical disk identifiers (LDID) */
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

typedef struct {
    HINF16 hInf;
    HFILE hInfFile;
    LPSTR lpInfFileName;
} INF_FILE;

extern INF_FILE *InfList;
extern WORD InfNumEntries;

extern LPCSTR IP_GetFileName(HINF16 hInf);
extern void WINAPI GenFormStrWithoutPlaceHolders16( LPSTR szDst, LPCSTR szSrc, HINF16 hInf);

#endif /* __SETUPX16_H */
