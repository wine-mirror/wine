/*
 * Definitions for windows network service
 *
 * Copyright 1997 Andreas Mohr
 * Copyright 1999 Ulrich Weigand
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

#ifndef __WINE_WINNET16_H
#define __WINE_WINNET16_H

#include <windef.h>
#include <wine/windef16.h>

#pragma pack(push,1)

/*
 * Remote printing
 */

typedef struct
{
    WORD    pqName;
    WORD    pqComment;
    WORD    pqStatus;
    WORD    pqJobcount;
    WORD    pqPrinters;

} QUEUESTRUCT16, *LPQUEUESTRUCT16;

#define WNPRQ_ACTIVE    0x0
#define WNPRQ_PAUSE     0x1
#define WNPRQ_ERROR     0x2
#define WNPRQ_PENDING   0x3
#define WNPRQ_PROBLEM   0x4

typedef struct
{
    WORD    pjId;
    WORD    pjUsername;
    WORD    pjParms;
    WORD    pjPosition;
    WORD    pjStatus;
    DWORD   pjSubmitted;
    DWORD   pjSize;
    WORD    pjCopies;
    WORD    pjComment;

} JOBSTRUCT16, *LPJOBSTRUCT16;

#pragma pack(pop)

#define WNPRJ_QSTATUS           0x0007
#define WNPRJ_DEVSTATUS         0x0FF8

#define WNPRJ_QS_QUEUED         0x0000
#define WNPRJ_QS_PAUSED         0x0001
#define WNPRJ_QS_SPOOLING       0x0002
#define WNPRJ_QS_PRINTING       0x0003

#define WNPRJ_DS_COMPLETE       0x0008
#define WNPRJ_DS_INTERV         0x0010
#define WNPRJ_DS_ERROR          0x0020
#define WNPRJ_DS_DESTOFFLINE    0x0040
#define WNPRJ_DS_DESTPAUSED     0x0080
#define WNPRJ_DS_NOTIFY         0x0100
#define WNPRJ_DS_DESTNOPAPER    0x0200
#define WNPRJ_DS_DESTFORMCHG    0x0400
#define WNPRJ_DS_DESTCRTCHG     0x0800
#define WNPRJ_DS_DESTPENCHG     0x1000

#define SP_QUEUECHANGED         0x0500

#define WNJ_NULL_JOBID  0

WORD WINAPI WNetOpenJob16(LPSTR,LPSTR,WORD,LPINT16);
WORD WINAPI WNetCloseJob16(WORD,LPINT16,LPSTR);
WORD WINAPI WNetWriteJob16(HANDLE16,LPSTR,LPINT16);
WORD WINAPI WNetAbortJob16(LPSTR,WORD);
WORD WINAPI WNetHoldJob16(LPSTR,WORD);
WORD WINAPI WNetReleaseJob16(LPSTR,WORD);
WORD WINAPI WNetCancelJob16(LPSTR,WORD);
WORD WINAPI WNetSetJobCopies16(LPSTR,WORD,WORD);

WORD WINAPI WNetWatchQueue16(HWND16,LPSTR,LPSTR,WORD);
WORD WINAPI WNetUnwatchQueue16(LPSTR);
WORD WINAPI WNetLockQueueData16(LPSTR,LPSTR,LPQUEUESTRUCT16 *);
WORD WINAPI WNetUnlockQueueData16(LPSTR);


/*
 * Connections
 */

WORD WINAPI WNetAddConnection16(LPCSTR,LPCSTR,LPCSTR);
WORD WINAPI WNetCancelConnection16(LPSTR,BOOL16);
WORD WINAPI WNetGetConnection16(LPSTR,LPSTR,UINT16 *);
WORD WINAPI WNetRestoreConnection16(HWND16,LPSTR);

/*
 * Capabilities
 */

WORD WINAPI WNetGetCaps16(WORD);

#define WNNC16_SPEC_VERSION                       0x01
#define WNNC16_NET_TYPE                           0x02
#define WNNC16_DRIVER_VERSION                     0x03
#define WNNC16_USER                               0x04
#define WNNC16_CONNECTION                         0x06
#define WNNC16_PRINTING                           0x07
#define WNNC16_DIALOG                             0x08
#define WNNC16_ADMIN                              0x09
#define WNNC16_ERROR                              0x0a
#define WNNC16_PRINTMGREXT                        0x0b

#define WNNC16_NET_NONE                           0x0
#define WNNC16_NET_MSNet                          0x1
#define WNNC16_NET_LanMan                         0x2
#define WNNC16_NET_NetWare                        0x3
#define WNNC16_NET_Vines                          0x4
#define WNNC16_NET_10NET                          0x5
#define WNNC16_NET_Locus                          0x6
#define WNNC16_NET_SUN_PC_NFS                     0x7
#define WNNC16_NET_LANstep                        0x8
#define WNNC16_NET_9TILES                         0x9
#define WNNC16_NET_LANtastic                      0xa
#define WNNC16_NET_AS400                          0xb
#define WNNC16_NET_FTP_NFS                        0xc
#define WNNC16_NET_PATHWORKS                      0xd
#define WNNC16_NET_LifeNet                        0xe
#define WNNC16_NET_POWERLan                       0xf
#define WNNC16_NET_MultiNet                       0x8000

#define WNNC16_SUBNET_NONE                        0x00
#define WNNC16_SUBNET_MSNet                       0x01
#define WNNC16_SUBNET_LanMan                      0x02
#define WNNC16_SUBNET_WinWorkgroups               0x04
#define WNNC16_SUBNET_NetWare                     0x08
#define WNNC16_SUBNET_Vines                       0x10
#define WNNC16_SUBNET_Other                       0x80

#define WNNC16_CON_AddConnection                  0x0001
#define WNNC16_CON_CancelConnection               0x0002
#define WNNC16_CON_GetConnections                 0x0004
#define WNNC16_CON_AutoConnect                    0x0008
#define WNNC16_CON_BrowseDialog                   0x0010
#define WNNC16_CON_RestoreConnection              0x0020

#define WNNC16_PRT_OpenJob                        0x0002
#define WNNC16_PRT_CloseJob                       0x0004
#define WNNC16_PRT_HoldJob                        0x0010
#define WNNC16_PRT_ReleaseJob                     0x0020
#define WNNC16_PRT_CancelJob                      0x0040
#define WNNC16_PRT_SetJobCopies                   0x0080
#define WNNC16_PRT_WatchQueue                     0x0100
#define WNNC16_PRT_UnwatchQueue                   0x0200
#define WNNC16_PRT_LockQueueData                  0x0400
#define WNNC16_PRT_UnlockQueueData                0x0800
#define WNNC16_PRT_ChangeMsg                      0x1000
#define WNNC16_PRT_AbortJob                       0x2000
#define WNNC16_PRT_NoArbitraryLock                0x4000
#define WNNC16_PRT_WriteJob                       0x8000

#define WNNC16_DLG_DeviceMode                     0x0001
#define WNNC16_DLG_BrowseDialog                   0x0002
#define WNNC16_DLG_ConnectDialog                  0x0004
#define WNNC16_DLG_DisconnectDialog               0x0008
#define WNNC16_DLG_ViewQueueDialog                0x0010
#define WNNC16_DLG_PropertyDialog                 0x0020
#define WNNC16_DLG_ConnectionDialog               0x0040
#define WNNC16_DLG_PrinterConnectDialog           0x0080
#define WNNC16_DLG_SharesDialog                   0x0100
#define WNNC16_DLG_ShareAsDialog                  0x0200

#define WNNC16_ADM_GetDirectoryType               0x0001
#define WNNC16_ADM_DirectoryNotify                0x0002
#define WNNC16_ADM_LongNames                      0x0004
#define WNNC16_ADM_SetDefaultDrive                0x0008

#define WNNC16_ERR_GetError                       0x0001
#define WNNC16_ERR_GetErrorText                   0x0002


/*
 * Get User
 */

WORD WINAPI WNetGetUser16(LPSTR,LPINT16);


/*
 * Browsing
 */

#define WNBD_CONN_UNKNOWN       0x0
#define WNBD_CONN_DISKTREE      0x1
#define WNBD_CONN_PRINTQ        0x3
#define WNBD_MAX_LENGTH         0x80

#define WNTYPE_DRIVE            1
#define WNTYPE_FILE             2
#define WNTYPE_PRINTER          3
#define WNTYPE_COMM             4

#define WNPS_FILE               0
#define WNPS_DIR                1
#define WNPS_MULT               2

WORD WINAPI WNetDeviceMode16(HWND16);
WORD WINAPI WNetBrowseDialog16(HWND16,WORD,LPSTR);
WORD WINAPI WNetConnectDialog16(HWND16,WORD);
WORD WINAPI WNetDisconnectDialog16(HWND16,WORD);
WORD WINAPI WNetConnectionDialog16(HWND16,WORD);
WORD WINAPI WNetViewQueueDialog16(HWND16,LPSTR);
WORD WINAPI WNetPropertyDialog16(HWND16,WORD,WORD,LPSTR,WORD);
WORD WINAPI WNetGetPropertyText16(WORD,WORD,LPSTR,LPSTR,WORD,WORD);


/*
 * Admin
 */

#define WNDT_NORMAL   0
#define WNDT_NETWORK  1

#define WNDN_MKDIR    1
#define WNDN_RMDIR    2
#define WNDN_MVDIR    3

WORD WINAPI WNetGetDirectoryType16(LPSTR,LPINT16);
WORD WINAPI WNetDirectoryNotify16(HWND16,LPSTR,WORD);


/*
 * Status codes
 */

WORD WINAPI WNetGetError16(LPINT16);
WORD WINAPI WNetGetErrorText16(WORD,LPSTR,LPINT16);
WORD WINAPI WNetErrorText16(WORD,LPSTR,WORD);

#define WN16_SUCCESS                      0x0000
#define WN16_NOT_SUPPORTED                0x0001
#define WN16_NET_ERROR                    0x0002
#define WN16_MORE_DATA                    0x0003
#define WN16_BAD_POINTER                  0x0004
#define WN16_BAD_VALUE                    0x0005
#define WN16_BAD_PASSWORD                 0x0006
#define WN16_ACCESS_DENIED                0x0007
#define WN16_FUNCTION_BUSY                0x0008
#define WN16_WINDOWS_ERROR                0x0009
#define WN16_BAD_USER                     0x000A
#define WN16_OUT_OF_MEMORY                0x000B
#define WN16_CANCEL                       0x000C
#define WN16_CONTINUE                     0x000D
#define WN16_NOT_CONNECTED                0x0030
#define WN16_OPEN_FILES                   0x0031
#define WN16_BAD_NETNAME                  0x0032
#define WN16_BAD_LOCALNAME                0x0033
#define WN16_ALREADY_CONNECTED            0x0034
#define WN16_DEVICE_ERROR                 0x0035
#define WN16_CONNECTION_CLOSED            0x0036
#define WN16_BAD_JOBID                    0x0040
#define WN16_JOB_NOT_FOUND                0x0041
#define WN16_JOB_NOT_HELD                 0x0042
#define WN16_BAD_QUEUE                    0x0043
#define WN16_BAD_FILE_HANDLE              0x0044
#define WN16_CANT_SET_COPIES              0x0045
#define WN16_ALREADY_LOCKED               0x0046
#define WN16_NO_ERROR                     0x0050



#endif /* __WINE_WINNET16_H */
