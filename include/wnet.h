/* Definitions for windows network service
 * 
 * Copyright 1997 Andreas Mohr
 */
#ifndef __WINE_WNET_H
#define __WINE_WNET_H

#include "windef.h"
#include "winerror.h"

#define WNDN_MKDIR  1
#define WNDN_RMDIR  2
#define WNDN_MVDIR  3

#define WN_SUCCESS                              0x0000
#define WN_NOT_SUPPORTED                        0x0001
#define WN_NET_ERROR                            0x0002
#define WN_MORE_DATA                            0x0003
#define WN_BAD_POINTER                          0x0004
#define WN_BAD_VALUE                            0x0005
#define WN_BAD_PASSWORD                         0x0006
#define WN_ACCESS_DENIED                        0x0007
#define WN_FUNCTION_BUSY                        0x0008
#define WN_WINDOWS_ERROR                        0x0009
#define WN_BAD_USER                             0x000A
#define WN_OUT_OF_MEMORY                        0x000B
#define WN_CANCEL                               0x000C
#define WN_CONTINUE                             0x000D
#define WN_BAD_HANDLE                           ERROR_INVALID_HANDLE
#define WN_NOT_CONNECTED                        0x0030
#define WN_OPEN_FILES                           0x0031
#define WN_BAD_NETNAME                          0x0032
#define WN_BAD_LOCALNAME                        0x0033
#define WN_ALREADY_CONNECTED                    0x0034
#define WN_DEVICE_ERROR                         0x0035
#define WN_CONNECTION_CLOSED                    0x0036
#define WN_NO_NETWORK                           ERROR_NO_NETWORK

#define WNNC_SPEC_VERSION                       0x01
#define WNNC_NET_TYPE                           0x02
#define WNNC_DRIVER_VERSION                     0x03
#define WNNC_USER                               0x04
/*#define WNNC_5                                0x05*/
#define WNNC_CONNECTION                         0x06
#define WNNC_PRINTING                           0x07
#define WNNC_DIALOG                             0x08
#define WNNC_ADMIN                              0x09
#define WNNC_ERROR                              0x0a
#define WNNC_PRINTMGREXT                        0x0b

#define WNNC_NET_NONE                           0x0
#define WNNC_NET_MSNet                          0x1
#define WNNC_NET_LanMan                         0x2
#define WNNC_NET_NetWare                        0x3
#define WNNC_NET_Vines                          0x4
#define WNNC_NET_10NET                          0x5
#define WNNC_NET_Locus                          0x6
#define WNNC_NET_SUN_PC_NFS                     0x7
#define WNNC_NET_LANstep                        0x8
#define WNNC_NET_9TILES                         0x9
#define WNNC_NET_LANtastic                      0xa
#define WNNC_NET_AS400                          0xb
#define WNNC_NET_FTP_NFS                        0xc
#define WNNC_NET_PATHWORKS                      0xd
#define WNNC_NET_LifeNet                        0xe
#define WNNC_NET_POWERLan                       0xf
#define WNNC_NET_MultiNet                       0x8000

#define WNNC_SUBNET_NONE                        0x00
#define WNNC_SUBNET_MSNet                       0x01
#define WNNC_SUBNET_LanMan                      0x02
#define WNNC_SUBNET_WinWorkgroups               0x04
#define WNNC_SUBNET_NetWare                     0x08
#define WNNC_SUBNET_Vines                       0x10
#define WNNC_SUBNET_Other                       0x80

#define WNNC_CON_AddConnection                  0x0001
#define WNNC_CON_CancelConnection               0x0002
#define WNNC_CON_GetConnections                 0x0004
#define WNNC_CON_AutoConnect                    0x0008
#define WNNC_CON_BrowseDialog                   0x0010
#define WNNC_CON_RestoreConnection              0x0020

#define WNNC_PRT_OpenJob                        0x0002
#define WNNC_PRT_CloseJob                       0x0004
#define WNNC_PRT_HoldJob                        0x0010
#define WNNC_PRT_ReleaseJob                     0x0020
#define WNNC_PRT_CancelJob                      0x0040
#define WNNC_PRT_SetJobCopies                   0x0080
#define WNNC_PRT_WatchQueue                     0x0100
#define WNNC_PRT_UnwatchQueue                   0x0200
#define WNNC_PRT_LockQueueData                  0x0400
#define WNNC_PRT_UnlockQueueData                0x0800
#define WNNC_PRT_ChangeMsg                      0x1000
#define WNNC_PRT_AbortJob                       0x2000
#define WNNC_PRT_NoArbitraryLock                0x4000
#define WNNC_PRT_WriteJob                       0x8000

#define WNNC_DLG_DeviceMode                     0x0001
#define WNNC_DLG_BrowseDialog                   0x0002
#define WNNC_DLG_ConnectDialog                  0x0004
#define WNNC_DLG_DisconnectDialog               0x0008
#define WNNC_DLG_ViewQueueDialog                0x0010
#define WNNC_DLG_PropertyDialog                 0x0020
#define WNNC_DLG_ConnectionDialog               0x0040
#define WNNC_DLG_PrinterConnectDialog           0x0080
#define WNNC_DLG_SharesDialog                   0x0100
#define WNNC_DLG_ShareAsDialog                  0x0200

#define WNNC_ADM_GetDirectoryType               0x0001
#define WNNC_ADM_DirectoryNotify                0x0002
#define WNNC_ADM_LongNames                      0x0004
#define WNNC_ADM_SetDefaultDrive                0x0008

#define WNNC_ERR_GetError                       0x0001
#define WNNC_ERR_GetErrorText                   0x0002

#define WNDT_NORMAL                             0x0
#define WNDT_NETWORK                            0x1

#define WIN30X                                  0x0
#define WIN31X                                  0x1
#define WIN311X                                 0x2
#define WIN95X                                  0x3
#define WINNTX                                  0x4
#define WINOTHERX                               0x5
#define WIN32X                                  0x6

typedef LPVOID  LPNETRESOURCE16;

#endif	/* __WINE_WNET_H */
