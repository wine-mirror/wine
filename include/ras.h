/*
 * Copyright (C) 1998 Marcus Meissner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_RAS_H
#define __WINE_RAS_H

#include "windef.h"
#include "lmcons.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "pshpack4.h"

#define RAS_MaxCallbackNumber RAS_MaxPhoneNumber
#define RAS_MaxDeviceName     128
#define RAS_MaxDeviceType     16
#define RAS_MaxEntryName      256
#define RAS_MaxPhoneNumber    128
#define RAS_MaxAreaCode       10
#define RAS_MaxPadType        32
#define RAS_MaxX25Address     200
#define RAS_MaxFacilities     200
#define RAS_MaxUserData       200

DECLARE_HANDLE(HRASCONN);

/* szDeviceType strings for RASDEVINFO */
#define	RASDT_Direct	"direct"
#define	RASDT_Modem	"modem"
#define	RASDT_Isdn	"isdn"
#define	RASDT_X25	"x25"

#define RASBASE				600
#define ERROR_BUFFER_TOO_SMALL		(RASBASE+3)
#define ERROR_INVALID_SIZE		(RASBASE+32)

typedef struct tagRASDEVINFOA {
    DWORD    dwSize;
    CHAR     szDeviceType[ RAS_MaxDeviceType + 1 ];
    CHAR     szDeviceName[ RAS_MaxDeviceName + 1 ];
} RASDEVINFOA, *LPRASDEVINFOA;

typedef struct tagRASDEVINFOW {
    DWORD    dwSize;
    WCHAR    szDeviceType[ RAS_MaxDeviceType + 1 ];
    WCHAR    szDeviceName[ RAS_MaxDeviceName + 1 ];
} RASDEVINFOW, *LPRASDEVINFOW;

DECL_WINELIB_TYPE_AW(RASDEVINFO)
DECL_WINELIB_TYPE_AW(LPRASDEVINFO)

typedef struct tagRASCONNA {
    DWORD    dwSize;
    HRASCONN hRasConn;
    CHAR     szEntryName[ RAS_MaxEntryName + 1 ];
    CHAR     szDeviceType[ RAS_MaxDeviceType + 1 ];
    CHAR     szDeviceName[ RAS_MaxDeviceName + 1 ];
    CHAR     szPhonebook[ MAX_PATH ];
    DWORD    dwSubEntry;
} RASCONNA,*LPRASCONNA;

typedef struct tagRASCONNW {
    DWORD    dwSize;
    HRASCONN hRasConn;
    WCHAR    szEntryName[ RAS_MaxEntryName + 1 ];
    WCHAR    szDeviceType[ RAS_MaxDeviceType + 1 ];
    WCHAR    szDeviceName[ RAS_MaxDeviceName + 1 ];
    WCHAR    szPhonebook[ MAX_PATH ];
    DWORD    dwSubEntry;
} RASCONNW,*LPRASCONNW;

DECL_WINELIB_TYPE_AW(RASCONN)
DECL_WINELIB_TYPE_AW(LPRASCONN)

typedef struct tagRASENTRYNAMEA {
    DWORD dwSize;
    CHAR  szEntryName[ RAS_MaxEntryName + 1 ];
} RASENTRYNAMEA, *LPRASENTRYNAMEA;

typedef struct tagRASENTRYNAMEW {
    DWORD dwSize;
    WCHAR szEntryName[ RAS_MaxEntryName + 1 ];
} RASENTRYNAMEW, *LPRASENTRYNAMEW;

DECL_WINELIB_TYPE_AW(RASENTRYNAME)
DECL_WINELIB_TYPE_AW(LPRASENTRYNAME)

typedef struct tagRASDIALPARAMSA {
    DWORD dwSize;
    CHAR szEntryName[ RAS_MaxEntryName + 1 ];
    CHAR szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    CHAR szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
    CHAR szUserName[ UNLEN + 1 ];
    CHAR szPassword[ PWLEN + 1 ];
    CHAR szDomain[ DNLEN + 1 ];
    DWORD dwSubEntry;
    DWORD dwCallbackId;
} RASDIALPARAMSA, *LPRASDIALPARAMSA;

typedef struct tagRASDIALPARAMSW {
    DWORD dwSize;
    WCHAR szEntryName[ RAS_MaxEntryName + 1 ];
    WCHAR szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    WCHAR szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
    WCHAR szUserName[ UNLEN + 1 ];
    WCHAR szPassword[ PWLEN + 1 ];
    WCHAR szDomain[ DNLEN + 1 ];
    DWORD dwSubEntry;
    DWORD dwCallbackId;
} RASDIALPARAMSW, *LPRASDIALPARAMSW;

DECL_WINELIB_TYPE_AW(RASDIALPARAMS)
DECL_WINELIB_TYPE_AW(LPRASDIALPARAMS)

typedef struct tagRASIPADDR {
	BYTE classA,classB,classC,classD;
} RASIPADDR;

#define RASEO_UseCountryAndAreaCodes	0x0001
#define RASEO_SpecificIpAddr		0x0002
#define RASEO_SpecificNameServers	0x0004
#define RASEO_IpHeaderCompression	0x0008
#define RASEO_RemoteDefaultGateway	0x0010
#define RASEO_DisableLcpExtensions	0x0020
#define RASEO_TerminalBeforeDial	0x0040
#define RASEO_TerminalAfterDial		0x0080
#define RASEO_ModemLights		0x0100
#define RASEO_SwCompression		0x0200
#define RASEO_RequireEncryptedPw	0x0400
#define RASEO_RequireMsEncryptedPw	0x0800
#define RASEO_RequireDataEncryption	0x1000
#define RASEO_NetworkLogon		0x2000
#define RASEO_UseLogonCredentials	0x4000
#define RASEO_PromoteAlternates		0x8000
typedef struct tagRASENTRYA {
    DWORD dwSize;
    DWORD dwfOptions;

    /* Location */

    DWORD dwCountryID;
    DWORD dwCountryCode;
    CHAR szAreaCode[ RAS_MaxAreaCode + 1 ];
    CHAR szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    DWORD dwAlternateOffset;

    /* IP related stuff */

    RASIPADDR ipaddr;
    RASIPADDR ipaddrDns;
    RASIPADDR ipaddrDnsAlt;
    RASIPADDR ipaddrWins;
    RASIPADDR ipaddrWinsAlt;

    /* Framing (for ppp/isdn etc...) */

    DWORD dwFrameSize;
    DWORD dwfNetProtocols;
    DWORD dwFramingProtocol;

    CHAR szScript[ MAX_PATH ];

    CHAR szAutodialDll[ MAX_PATH ];
    CHAR szAutodialFunc[ MAX_PATH ];

    CHAR szDeviceType[ RAS_MaxDeviceType + 1 ];
    CHAR szDeviceName[ RAS_MaxDeviceName + 1 ];

    /* x25 only */

    CHAR szX25PadType[ RAS_MaxPadType + 1 ];
    CHAR szX25Address[ RAS_MaxX25Address + 1 ];
    CHAR szX25Facilities[ RAS_MaxFacilities + 1 ];
    CHAR szX25UserData[ RAS_MaxUserData + 1 ];
    DWORD dwChannels;

    DWORD dwReserved1;
    DWORD dwReserved2;

#if (WINVER >= 0x401)

    /* Multilink and BAP */

    DWORD dwSubEntries;
    DWORD dwDialMode;
    DWORD dwDialExtraPercent;
    DWORD dwDialExtraSampleSeconds;
    DWORD dwHangUpExtraPercent;
    DWORD dwHangUpExtraSampleSeconds;

    /* Idle time out */
    DWORD dwIdleDisconnectSeconds;

#endif
#if (WINVER >= 0x500)

    DWORD dwType;		/* entry type */
    DWORD dwEncryptionType;	/* type of encryption to use */
    DWORD dwCustomAuthKey;	/* authentication key for EAP */
    GUID guidId;		/* guid that represents the phone-book entry  */
    CHAR szCustomDialDll[MAX_PATH];    /* DLL for custom dialing  */
    DWORD dwVpnStrategy;         /* specifies type of VPN protocol */
#endif
#if (WINVER >= 0x501)
    DWORD dwfOptions2;
    DWORD dwfOptions3;
    CHAR szDnsSuffix[RAS_MaxDnsSuffix];
    DWORD dwTcpWindowSize;
    CHAR szPrerequisitePbk[MAX_PATH];
    CHAR szPrerequisiteEntry[RAS_MaxEntryName + 1];
    DWORD dwRedialCount;
    DWORD dwRedialPause;
#endif
} RASENTRYA, *LPRASENTRYA;

typedef struct tagRASENTRYW {
    DWORD dwSize;
    DWORD dwfOptions;

    /* Location */

    DWORD dwCountryID;
    DWORD dwCountryCode;
    WCHAR szAreaCode[ RAS_MaxAreaCode + 1 ];
    WCHAR szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    DWORD dwAlternateOffset;

    /* IP related stuff */

    RASIPADDR ipaddr;
    RASIPADDR ipaddrDns;
    RASIPADDR ipaddrDnsAlt;
    RASIPADDR ipaddrWins;
    RASIPADDR ipaddrWinsAlt;

    /* Framing (for ppp/isdn etc...) */

    DWORD dwFrameSize;
    DWORD dwfNetProtocols;
    DWORD dwFramingProtocol;

    WCHAR szScript[ MAX_PATH ];

    WCHAR szAutodialDll[ MAX_PATH ];
    WCHAR szAutodialFunc[ MAX_PATH ];

    WCHAR szDeviceType[ RAS_MaxDeviceType + 1 ];
    WCHAR szDeviceName[ RAS_MaxDeviceName + 1 ];

    /* x25 only */

    WCHAR szX25PadType[ RAS_MaxPadType + 1 ];
    WCHAR szX25Address[ RAS_MaxX25Address + 1 ];
    WCHAR szX25Facilities[ RAS_MaxFacilities + 1 ];
    WCHAR szX25UserData[ RAS_MaxUserData + 1 ];
    DWORD dwChannels;

    DWORD dwReserved1;
    DWORD dwReserved2;

#if (WINVER >= 0x401)

    /* Multilink and BAP */

    DWORD dwSubEntries;
    DWORD dwDialMode;
    DWORD dwDialExtraPercent;
    DWORD dwDialExtraSampleSeconds;
    DWORD dwHangUpExtraPercent;
    DWORD dwHangUpExtraSampleSeconds;

    /* Idle time out */
    DWORD dwIdleDisconnectSeconds;

#endif
#if (WINVER >= 0x500)

    DWORD dwType;		/* entry type */
    DWORD dwEncryptionType;	/* type of encryption to use */
    DWORD dwCustomAuthKey;	/* authentication key for EAP */
    GUID guidId;		/* guid that represents the phone-book entry  */
    WCHAR szCustomDialDll[MAX_PATH];    /* DLL for custom dialing  */
    DWORD dwVpnStrategy;         /* specifies type of VPN protocol */
#endif
#if (WINVER >= 0x501)
    DWORD dwfOptions2;
    DWORD dwfOptions3;
    WCHAR szDnsSuffix[RAS_MaxDnsSuffix];
    DWORD dwTcpWindowSize;
    WCHAR szPrerequisitePbk[MAX_PATH];
    WCHAR szPrerequisiteEntry[RAS_MaxEntryName + 1];
    DWORD dwRedialCount;
    DWORD dwRedialPause;
#endif
} RASENTRYW, *LPRASENTRYW;

DECL_WINELIB_TYPE_AW(RASENTRY)


DWORD WINAPI RasEnumConnectionsA(LPRASCONNA,LPDWORD,LPDWORD);
DWORD WINAPI RasEnumConnectionsW(LPRASCONNW,LPDWORD,LPDWORD);
#define      RasEnumConnections WINELIB_NAME_AW(RasEnumConnections)
DWORD WINAPI RasEnumEntriesA(LPCSTR,LPCSTR,LPRASENTRYNAMEA,LPDWORD,LPDWORD);
DWORD WINAPI RasEnumEntriesW(LPCWSTR,LPCWSTR,LPRASENTRYNAMEW,LPDWORD,LPDWORD);
#define      RasEnumEntries WINELIB_NAME_AW(RasEnumEntries)
DWORD WINAPI RasGetEntryDialParamsA(LPCSTR,LPRASDIALPARAMSA,LPBOOL);
DWORD WINAPI RasGetEntryDialParamsW(LPCWSTR,LPRASDIALPARAMSW,LPBOOL);
#define      RasGetEntryDialParams WINELIB_NAME_AW(RasGetEntryDialParams)
DWORD WINAPI RasHangUpA(HRASCONN);
DWORD WINAPI RasHangUpW(HRASCONN);
#define      RasHangUp WINELIB_NAME_AW(RasHangUp)
DWORD WINAPI RasValidateEntryNameA(LPCSTR  lpszPhonebook, LPCSTR  lpszEntry);
DWORD WINAPI RasValidateEntryNameW(LPCWSTR lpszPhonebook, LPCWSTR lpszEntry);
#define RasValidateEntryName WINELIB_NAME_AW(RasValidateEntryName)
DWORD WINAPI RasSetEntryPropertiesA(LPCSTR lpszPhonebook, LPCSTR lpszEntry,
        LPRASENTRYA lpRasEntry, DWORD dwEntryInfoSize, LPBYTE lpbDeviceInfo,
	DWORD dwDeviceInfoSize);
DWORD WINAPI RasSetEntryPropertiesW(LPCWSTR lpszPhonebook, LPCWSTR lpszEntry,
        LPRASENTRYW lpRasEntry, DWORD dwEntryInfoSize, LPBYTE lpbDeviceInfo,
	DWORD dwDeviceInfoSize);
#define RasSetEntryProperties WINELIB_NAME_AW(RasSetEntryProperties)
DWORD WINAPI RasGetAutodialParamA(DWORD dwKey, LPVOID lpvValue, LPDWORD lpdwcbValue);
DWORD WINAPI RasGetAutodialParamW(DWORD dwKey, LPVOID lpvValue, LPDWORD lpdwcbValue);
#define RasGetAutodialParam WINELIB_NAME_AW(RasGetAutodialParam)
DWORD WINAPI RasSetAutodialEnableA(DWORD dwDialingLocation, BOOL fEnabled);
DWORD WINAPI RasSetAutodialEnableW(DWORD dwDialingLocation, BOOL fEnabled);
#define RasSetAutodialEnable WINELIB_NAME_AW(RasSetAutodialEnable)

#include "poppack.h"
#ifdef __cplusplus
}
#endif

#endif
