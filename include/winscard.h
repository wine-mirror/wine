/*
 * Winscard definitions
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINSCARD_H
#define __WINE_WINSCARD_H

#include <wtypes.h>
#include <winioctl.h>
#include <winsmcrd.h>
#include <scarderr.h>

#ifndef WINSCARDAPI
#define WINSCARDAPI DECLSPEC_IMPORT
#endif

/* Valid scopes for contexts */
#define SCARD_SCOPE_USER     0
#define SCARD_SCOPE_TERMINAL 1
#define SCARD_SCOPE_SYSTEM   2

#ifndef _LPCBYTE_DEFINED
#define _LPCBYTE_DEFINED
typedef const BYTE *LPCBYTE;
#endif

typedef ULONG_PTR SCARDCONTEXT, *PSCARDCONTEXT, *LPSCARDCONTEXT;
typedef ULONG_PTR SCARDHANDLE,  *PSCARDHANDLE,  *LPSCARDHANDLE;

typedef struct _SCARD_ATRMASK
{
    DWORD cbAtr;
    BYTE  rgbAtr[36];
    BYTE  rgbMask[36];
} SCARD_ATRMASK, *PSCARD_ATRMASK, *LPSCARD_ATRMASK;

typedef struct
{
    LPCSTR szReader;
    LPVOID pvUserData;
    DWORD  dwCurrentState;
    DWORD  dwEventState;
    DWORD  cbAtr;
    BYTE   rgbAtr[36];
} SCARD_READERSTATEA, *PSCARD_READERSTATEA, *LPSCARD_READERSTATEA;
typedef struct
{
    LPCWSTR szReader;
    LPVOID  pvUserData;
    DWORD   dwCurrentState;
    DWORD   dwEventState;
    DWORD   cbAtr;
    BYTE    rgbAtr[36];
} SCARD_READERSTATEW, *PSCARD_READERSTATEW, *LPSCARD_READERSTATEW;
DECL_WINELIB_TYPE_AW(SCARD_READERSTATE)
DECL_WINELIB_TYPE_AW(PSCARD_READERSTATE)
DECL_WINELIB_TYPE_AW(LPSCARD_READERSTATE)

#define SCARD_AUTOALLOCATE (DWORD)(-1)

#define SCARD_SCOPE_USER     0
#define SCARD_SCOPE_TERMINAL 1
#define SCARD_SCOPE_SYSTEM   2

#define SCARD_STATE_UNAWARE     0x00000000
#define SCARD_STATE_IGNORE      0x00000001
#define SCARD_STATE_CHANGED     0x00000002
#define SCARD_STATE_UNKNOWN     0x00000004
#define SCARD_STATE_UNAVAILABLE 0x00000008
#define SCARD_STATE_EMPTY       0x00000010
#define SCARD_STATE_PRESENT     0x00000020
#define SCARD_STATE_ATRMATCH    0x00000040
#define SCARD_STATE_EXCLUSIVE   0x00000080
#define SCARD_STATE_INUSE       0x00000100
#define SCARD_STATE_MUTE        0x00000200
#define SCARD_STATE_UNPOWERED   0x00000400

#define SCARD_SHARE_EXCLUSIVE 1
#define SCARD_SHARE_SHARED    2
#define SCARD_SHARE_DIRECT    3

#define SCARD_LEAVE_CARD   0
#define SCARD_RESET_CARD   1
#define SCARD_UNPOWER_CARD 2
#define SCARD_EJECT_CARD   3

#ifdef __cplusplus
extern "C" {
#endif

WINSCARDAPI HANDLE      WINAPI SCardAccessStartedEvent(void);
WINSCARDAPI LONG        WINAPI SCardAddReaderToGroupA(SCARDCONTEXT,LPCSTR,LPCSTR);
WINSCARDAPI LONG        WINAPI SCardAddReaderToGroupW(SCARDCONTEXT,LPCWSTR,LPCWSTR);
#define                        SCardAddReaderToGroup WINELIB_NAME_AW(SCardAddReaderToGroup)
WINSCARDAPI LONG        WINAPI SCardBeginTransaction(SCARDHANDLE);
WINSCARDAPI LONG        WINAPI SCardCancel(SCARDCONTEXT);
WINSCARDAPI LONG        WINAPI SCardConnectA(SCARDCONTEXT,LPCSTR,DWORD,DWORD,LPSCARDHANDLE,LPDWORD);
WINSCARDAPI LONG        WINAPI SCardConnectW(SCARDCONTEXT,LPCWSTR,DWORD,DWORD,LPSCARDHANDLE,LPDWORD);
#define                        SCardConnect WINELIB_NAME_AW(SCardConnect)
WINSCARDAPI LONG        WINAPI SCardControl(SCARDHANDLE,DWORD,LPCVOID,DWORD,LPVOID,DWORD,LPDWORD);
WINSCARDAPI LONG        WINAPI SCardDisconnect(SCARDHANDLE,DWORD);
WINSCARDAPI LONG        WINAPI SCardEndTransaction(SCARDHANDLE,DWORD);
WINSCARDAPI LONG        WINAPI SCardEstablishContext(DWORD,LPCVOID,LPCVOID,LPSCARDCONTEXT);
WINSCARDAPI LONG        WINAPI SCardForgetCardTypeA(SCARDCONTEXT,LPCSTR);
WINSCARDAPI LONG        WINAPI SCardForgetCardTypeW(SCARDCONTEXT,LPCWSTR);
#define                        SCardForgetCardType WINELIB_NAME_AW(SCardForgetCardType)
WINSCARDAPI LONG        WINAPI SCardForgetReaderA(SCARDCONTEXT,LPCSTR);
WINSCARDAPI LONG        WINAPI SCardForgetReaderW(SCARDCONTEXT,LPCWSTR);
#define                        SCardForgetReader WINELIB_NAME_AW(SCardForgetReader)
WINSCARDAPI LONG        WINAPI SCardForgetReaderGroupA(SCARDCONTEXT,LPCSTR);
WINSCARDAPI LONG        WINAPI SCardForgetReaderGroupW(SCARDCONTEXT,LPCWSTR);
#define                        SCardForgetReaderGroup WINELIB_NAME_AW(SCardForgetReaderGroup)
WINSCARDAPI LONG        WINAPI SCardFreeMemory(SCARDCONTEXT,LPCVOID);
WINSCARDAPI LONG        WINAPI SCardGetAttrib(SCARDHANDLE,DWORD,LPBYTE,LPDWORD);
WINSCARDAPI LONG        WINAPI SCardGetCardTypeProviderNameA(SCARDCONTEXT,LPCSTR,DWORD,LPSTR,LPDWORD);
WINSCARDAPI LONG        WINAPI SCardGetCardTypeProviderNameW(SCARDCONTEXT,LPCWSTR,DWORD,LPWSTR,LPDWORD);
#define                        SCardGetCardTypeProviderName WINELIB_NAME_AW(SCardGetCardTypeProviderName)
WINSCARDAPI LONG        WINAPI SCardGetProviderIdA(SCARDCONTEXT,LPCSTR,LPGUID);
WINSCARDAPI LONG        WINAPI SCardGetProviderIdW(SCARDCONTEXT,LPCWSTR,LPGUID);
#define                        SCardGetProviderId WINELIB_NAME_AW(SCardGetProviderId)
WINSCARDAPI LONG        WINAPI SCardGetStatusChangeA(SCARDCONTEXT,DWORD,LPSCARD_READERSTATEA,DWORD);
WINSCARDAPI LONG        WINAPI SCardGetStatusChangeW(SCARDCONTEXT,DWORD,LPSCARD_READERSTATEW,DWORD);
#define                        SCardGetStatusChange WINELIB_NAME_AW(SCardGetStatusChange)
WINSCARDAPI LONG        WINAPI SCardIntroduceCardTypeA(SCARDCONTEXT,LPCSTR,LPCGUID,LPCGUID,DWORD,LPCBYTE,LPCBYTE,DWORD);
WINSCARDAPI LONG        WINAPI SCardIntroduceCardTypeW(SCARDCONTEXT,LPCWSTR,LPCGUID,LPCGUID,DWORD,LPCBYTE,LPCBYTE,DWORD);
#define                        SCardIntroduceCardType WINELIB_NAME_AW(SCardIntroduceCardType)
WINSCARDAPI LONG        WINAPI SCardIntroduceReaderA(SCARDCONTEXT,LPCSTR,LPCSTR);
WINSCARDAPI LONG        WINAPI SCardIntroduceReaderW(SCARDCONTEXT,LPCWSTR,LPCWSTR);
#define                        SCardIntroduceReader WINELIB_NAME_AW(SCardIntroduceReader)
WINSCARDAPI LONG        WINAPI SCardIntroduceReaderGroupA(SCARDCONTEXT,LPCSTR);
WINSCARDAPI LONG        WINAPI SCardIntroduceReaderGroupW(SCARDCONTEXT,LPCWSTR);
#define                        SCardIntroduceReaderGroup WINELIB_NAME_AW(SCardIntroduceReaderGroup)
WINSCARDAPI LONG        WINAPI SCardIsValidContext(SCARDCONTEXT);
WINSCARDAPI LONG        WINAPI SCardListCardsA(SCARDCONTEXT,LPCBYTE,LPCGUID,DWORD,LPSTR,LPDWORD);
WINSCARDAPI LONG        WINAPI SCardListCardsW(SCARDCONTEXT,LPCBYTE,LPCGUID,DWORD,LPWSTR,LPDWORD);
#define                        SCardListCards WINELIB_NAME_AW(SCardListCards)
WINSCARDAPI LONG        WINAPI SCardListInterfacesA(SCARDCONTEXT,LPCSTR,LPGUID,LPDWORD);
WINSCARDAPI LONG        WINAPI SCardListInterfacesW(SCARDCONTEXT,LPCWSTR,LPGUID,LPDWORD);
#define                        SCardListInterfaces WINELIB_NAME_AW(SCardListInterfaces)
WINSCARDAPI LONG        WINAPI SCardListReadersA(SCARDCONTEXT,const CHAR *,CHAR *,DWORD *);
WINSCARDAPI LONG        WINAPI SCardListReadersW(SCARDCONTEXT,const WCHAR *,WCHAR *,DWORD *);
#define                        SCardListReaders WINELIB_NAME_AW(SCardListReaders)
WINSCARDAPI LONG        WINAPI SCardListReaderGroupsA(SCARDCONTEXT,LPSTR,LPDWORD);
WINSCARDAPI LONG        WINAPI SCardListReaderGroupsW(SCARDCONTEXT,LPWSTR,LPDWORD);
#define                        SCardListReaderGroups WINELIB_NAME_AW(SCardListReaderGroups)
WINSCARDAPI LONG        WINAPI SCardLocateCardsA(SCARDCONTEXT,LPCSTR,LPSCARD_READERSTATEA,DWORD);
WINSCARDAPI LONG        WINAPI SCardLocateCardsW(SCARDCONTEXT,LPCWSTR,LPSCARD_READERSTATEW,DWORD);
#define                        SCardLocateCards WINELIB_NAME_AW(SCardLocateCards)
WINSCARDAPI LONG        WINAPI SCardLocateCardsByATRA(SCARDCONTEXT,LPSCARD_ATRMASK,DWORD,LPSCARD_READERSTATEA,DWORD);
WINSCARDAPI LONG        WINAPI SCardLocateCardsByATRW(SCARDCONTEXT,LPSCARD_ATRMASK,DWORD,LPSCARD_READERSTATEW,DWORD);
#define                        SCardLocateCardsByATR WINELIB_NAME_AW(SCardLocateCardsByATR)
WINSCARDAPI LONG        WINAPI SCardReconnect(SCARDHANDLE,DWORD,DWORD,DWORD,LPDWORD);
WINSCARDAPI LONG        WINAPI SCardReleaseContext(SCARDCONTEXT);
WINSCARDAPI void        WINAPI SCardReleaseStartedEvent(void);
WINSCARDAPI LONG        WINAPI SCardRemoveReaderFromGroupA(SCARDCONTEXT,LPCSTR,LPCSTR);
WINSCARDAPI LONG        WINAPI SCardRemoveReaderFromGroupW(SCARDCONTEXT,LPCWSTR,LPCWSTR);
#define                        SCardRemoveReaderFromGroup WINELIB_NAME_AW(SCardRemoveReaderFromGroup)
WINSCARDAPI LONG        WINAPI SCardSetAttrib(SCARDHANDLE,DWORD,LPCBYTE,DWORD);
WINSCARDAPI LONG        WINAPI SCardSetCardTypeProviderNameA(SCARDCONTEXT,LPCSTR,DWORD,LPCSTR);
WINSCARDAPI LONG        WINAPI SCardSetCardTypeProviderNameW(SCARDCONTEXT,LPCWSTR,DWORD,LPCWSTR);
#define                        SCardSetCardTypeProviderName WINELIB_NAME_AW(SCardSetCardTypeProviderName)
WINSCARDAPI LONG        WINAPI SCardState(SCARDHANDLE,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
WINSCARDAPI LONG        WINAPI SCardStatusA(SCARDHANDLE,LPSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
WINSCARDAPI LONG        WINAPI SCardStatusW(SCARDHANDLE,LPWSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
#define                        SCardStatus WINELIB_NAME_AW(SCardStatus)
WINSCARDAPI LONG        WINAPI SCardTransmit(SCARDHANDLE,LPCSCARD_IO_REQUEST,LPCBYTE,DWORD,LPSCARD_IO_REQUEST,LPBYTE,LPDWORD);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_WINSCARD_H */
