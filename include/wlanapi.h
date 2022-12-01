/*
 * Copyright (C) 2016 Austin English
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

#ifndef _WLAN_WLANAPI_H
#define _WLAN_WLANAPI_H

#include <windot11.h>

typedef enum _WLAN_INTERFACE_STATE
{
    wlan_interface_state_not_ready,
    wlan_interface_state_connected,
    wlan_interface_state_ad_hoc_network_formed,
    wlan_interface_state_disconnecting,
    wlan_interface_state_disconnected,
    wlan_interface_state_associating,
    wlan_interface_state_discovering,
    wlan_interface_state_authenticating
} WLAN_INTERFACE_STATE, *PWLAN_INTERFACE_STATE;

typedef struct _WLAN_INTERFACE_INFO
{
    GUID InterfaceGuid;
    WCHAR strInterfaceDescription[256];
    WLAN_INTERFACE_STATE isState;
} WLAN_INTERFACE_INFO, *PWLAN_INTERFACE_INFO;

typedef struct _WLAN_INTERFACE_INFO_LIST
{
    DWORD dwNumberOfItems;
    DWORD dwIndex;
    WLAN_INTERFACE_INFO InterfaceInfo[1];
} WLAN_INTERFACE_INFO_LIST, *PWLAN_INTERFACE_INFO_LIST;

#define DOT11_SSID_MAX_LENGTH 32
typedef struct _DOT11_SSID
{
    DWORD uSSIDLength;
    UCHAR ucSSID[DOT11_SSID_MAX_LENGTH];
} DOT11_SSID;

typedef struct _WLAN_RAW_DATA
{
    DWORD dwDataSize;
    BYTE DataBlob[1];
} WLAN_RAW_DATA, *PWLAN_RAW_DATA;

typedef struct _WLAN_NOTIFICATION_DATA
{
    DWORD NotificationSource;
    DWORD NotificationCode;
    GUID InterfaceGuid;
    DWORD dwDataSize;
    PVOID pData;
} WLAN_NOTIFICATION_DATA, *PWLAN_NOTIFICATION_DATA;

typedef void (WINAPI *WLAN_NOTIFICATION_CALLBACK)(WLAN_NOTIFICATION_DATA *, void *);

typedef enum _DOT11_BSS_TYPE
{
    dot11_BSS_type_infrastructure = 0x01,
    dot11_BSS_type_independent = 0x02,
    dot11_BSS_type_any = 0x03
} DOT11_BSS_TYPE, *PDOT11_BSS_TYPE;

typedef DWORD WLAN_REASON_CODE, *PWLAN_REASON_CODE, WLAN_SIGNAL_QUALITY;

typedef enum _DOT11_AUTH_ALGORITHM
{
    DOT11_AUTH_ALGO_80211_OPEN = 0x01,
    DOT11_AUTH_ALGO_80211_SHARED_KEY = 0x02,
    DOT11_AUTH_ALGO_WPA = 0x03,
    DOT11_AUTH_ALGO_WPA_PSK = 0x04,
    DOT11_AUTH_ALGO_WPA_NONE = 0x05,
    DOT11_AUTH_ALGO_RSNA = 0x06,
    DOT11_AUTH_ALGO_RSNA_PSK = 0x07,
    DOT11_AUTH_ALGO_IHV_START = 0x80000000,
    DOT11_AUTH_ALGO_IHV_END = 0xFFFFFFFF
} DOT11_AUTH_ALGORITHM, *PDOT11_AUTH_ALGORITHM;

typedef enum _DOT11_CIPHER_ALGORITHM
{
    DOT11_CIPHER_ALGO_NONE = 0x00,
    DOT11_CIPHER_ALGO_WEP40 = 0x01,
    DOT11_CIPHER_ALGO_TKIP = 0x02,
    DOT11_CIPHER_ALGO_CCMP = 0x04,
    DOT11_CIPHER_ALGO_WEP104 = 0x05,
    DOT11_CIPHER_ALGO_WPA_USE_GROUP = 0x100,
    DOT11_CIPHER_ALGO_RSN_USE_GROUP = 0x100,
    DOT11_CIPHER_ALGO_WEP = 0x101,
    DOT11_CIPHER_ALGO_IHV_START = 0x80000000,
    DOT11_CIPHER_ALGO_IHV_END = 0xFFFFFFFF
} DOT11_CIPHER_ALGORITHM, *PDOT11_CIPHER_ALGORITHM;

#define WLAN_MAX_PHY_TYPE_NUMBER 8

typedef struct _WLAN_AVAILABLE_NETWORK
{
    WCHAR strProfileName[256];
    DOT11_SSID dot11Ssid;
    DOT11_BSS_TYPE dot11BssType;
    ULONG uNumberOfBssids;
    BOOL bNetworkConnectable;
    WLAN_REASON_CODE wlanNotConnectableReason;
    ULONG uNumberOfPhyTypes;
    DOT11_PHY_TYPE dot11PhyTypes[WLAN_MAX_PHY_TYPE_NUMBER];
    BOOL bMorePhyTypes;
    WLAN_SIGNAL_QUALITY wlanSignalQuality;
    BOOL bSecurityEnabled;
    DOT11_AUTH_ALGORITHM dot11DefaultAuthAlgorithm;
    DOT11_CIPHER_ALGORITHM dot11DefaultCipherAlgorithm;
    DWORD dwFlags;
    DWORD dwReserved;
} WLAN_AVAILABLE_NETWORK, *PWLAN_AVAILABLE_NETWORK;

typedef enum _WLAN_INTF_OPCODE
{
    wlan_intf_opcode_autoconf_start = 0x000000000,
    wlan_intf_opcode_autoconf_enabled,
    wlan_intf_opcode_background_scan_enabled,
    wlan_intf_opcode_media_streaming_mode,
    wlan_intf_opcode_radio_state,
    wlan_intf_opcode_bss_type,
    wlan_intf_opcode_interface_state,
    wlan_intf_opcode_current_connection,
    wlan_intf_opcode_channel_number,
    wlan_intf_opcode_supported_infrastructure_auth_cipher_pairs,
    wlan_intf_opcode_supported_adhoc_auth_cipher_pairs,
    wlan_intf_opcode_supported_country_or_region_string_list,
    wlan_intf_opcode_current_operation_mode,
    wlan_intf_opcode_supported_safe_mode,
    wlan_intf_opcode_certified_safe_mode,
    wlan_intf_opcode_hosted_network_capable,
    wlan_intf_opcode_management_frame_protection_capable,
    wlan_intf_opcode_autoconf_end = 0x0fffffff,
    wlan_intf_opcode_msm_start = 0x10000100,
    wlan_intf_opcode_statistics,
    wlan_intf_opcode_rssi,
    wlan_intf_opcode_msm_end = 0x1fffffff,
    wlan_intf_opcode_security_start = 0x20010000,
    wlan_intf_opcode_security_end = 0x2fffffff,
    wlan_intf_opcode_ihv_start = 0x30000000,
    wlan_intf_opcode_ihv_end = 0x3fffffff
} WLAN_INTF_OPCODE, *PWLAN_INTF_OPCODE;

typedef enum _WLAN_OPCODE_VALUE_TYPE
{
    wlan_opcode_value_type_query_only = 0,
    wlan_opcode_value_type_set_by_group_policy,
    wlan_opcode_value_type_set_by_user,
    wlan_opcode_value_type_invalid
} WLAN_OPCODE_VALUE_TYPE, *PWLAN_OPCODE_VALUE_TYPE;

typedef struct _WLAN_AVAILABLE_NETWORK_LIST
{
    DWORD dwNumberOfItems;
    DWORD dwIndex;
    WLAN_AVAILABLE_NETWORK Network[1];
} WLAN_AVAILABLE_NETWORK_LIST, *PWLAN_AVAILABLE_NETWORK_LIST;

typedef enum _WLAN_HOSTED_NETWORK_OPCODE
{
    wlan_hosted_network_opcode_connection_settings,
    wlan_hosted_network_opcode_security_settings,
    wlan_hosted_network_opcode_station_profile,
    wlan_hosted_network_opcode_enable
} WLAN_HOSTED_NETWORK_OPCODE, *PWLAN_HOSTED_NETWORK_OPCODE;

typedef enum _WLAN_HOSTED_NETWORK_REASON
{
    wlan_hosted_network_reason_success = 0,
    wlan_hosted_network_reason_unspecified,
    wlan_hosted_network_reason_bad_parameters,
    wlan_hosted_network_reason_service_shutting_down,
    wlan_hosted_network_reason_insufficient_resources,
    wlan_hosted_network_reason_elevation_required,
    wlan_hosted_network_reason_read_only,
    wlan_hosted_network_reason_persistence_failed,
    wlan_hosted_network_reason_crypt_error,
    wlan_hosted_network_reason_impersonation,
    wlan_hosted_network_reason_stop_before_start,
    wlan_hosted_network_reason_interface_available,
    wlan_hosted_network_reason_interface_unavailable,
    wlan_hosted_network_reason_miniport_stopped,
    wlan_hosted_network_reason_miniport_started,
    wlan_hosted_network_reason_incompatible_connection_started,
    wlan_hosted_network_reason_incompatible_connection_stopped,
    wlan_hosted_network_reason_user_action,
    wlan_hosted_network_reason_client_abort,
    wlan_hosted_network_reason_ap_start_failed,
    wlan_hosted_network_reason_peer_arrived,
    wlan_hosted_network_reason_peer_departed,
    wlan_hosted_network_reason_peer_timeout,
    wlan_hosted_network_reason_gp_denied,
    wlan_hosted_network_reason_service_unavailable,
    wlan_hosted_network_reason_device_change,
    wlan_hosted_network_reason_properties_change,
    wlan_hosted_network_reason_virtual_station_blocking_use,
    wlan_hosted_network_reason_service_available_on_virtual_station
} WLAN_HOSTED_NETWORK_REASON, *PWLAN_HOSTED_NETWORK_REASON;

typedef enum _WLAN_HOSTED_NETWORK_STATE
{
    wlan_hosted_network_unavailable,
    wlan_hosted_network_idle,
    wlan_hosted_network_active
} WLAN_HOSTED_NETWORK_STATE, *PWLAN_HOSTED_NETWORK_STATE;

typedef enum _WLAN_HOSTED_NETWORK_PEER_AUTH_STATE
{
    wlan_hosted_network_peer_state_invalid,
    wlan_hosted_network_peer_state_authenticated
} WLAN_HOSTED_NETWORK_PEER_AUTH_STATE, *PWLAN_HOSTED_NETWORK_PEER_AUTH_STATE;

typedef struct _WLAN_HOSTED_NETWORK_PEER_STATE
{
    DOT11_MAC_ADDRESS PeerMacAddress;
    WLAN_HOSTED_NETWORK_PEER_AUTH_STATE PeerAuthState;
} WLAN_HOSTED_NETWORK_PEER_STATE, *PWLAN_HOSTED_NETWORK_PEER_STATE;

typedef struct _WLAN_HOSTED_NETWORK_STATUS
{
    WLAN_HOSTED_NETWORK_STATE HostedNetworkState;
    GUID IPDeviceID;
    DOT11_MAC_ADDRESS wlanHostedNetworkBSSID;
    DOT11_PHY_TYPE dot11PhyType;
    ULONG ulChannelFrequency;
    DWORD dwNumberOfPeers;
    WLAN_HOSTED_NETWORK_PEER_STATE PeerList[1];
} WLAN_HOSTED_NETWORK_STATUS, *PWLAN_HOSTED_NETWORK_STATUS;

DWORD WINAPI WlanCloseHandle(HANDLE, void *);
DWORD WINAPI WlanEnumInterfaces(HANDLE, void *, WLAN_INTERFACE_INFO_LIST **);
DWORD WINAPI WlanOpenHandle(DWORD, void *, DWORD *, HANDLE *);
void WINAPI WlanFreeMemory(void *);
void *WINAPI WlanAllocateMemory(DWORD) __WINE_ALLOC_SIZE(1) __WINE_DEALLOC(WlanFreeMemory) __WINE_MALLOC;
DWORD WINAPI WlanScan(HANDLE, const GUID *, const DOT11_SSID *, const WLAN_RAW_DATA *, void *);
DWORD WINAPI WlanRegisterNotification(HANDLE, DWORD, BOOL, WLAN_NOTIFICATION_CALLBACK, void *, void *, DWORD *);
DWORD WINAPI WlanGetAvailableNetworkList(HANDLE, const GUID *, DWORD, void *, WLAN_AVAILABLE_NETWORK_LIST **);
DWORD WINAPI WlanQueryInterface(HANDLE, const GUID *, WLAN_INTF_OPCODE, void *, DWORD *, void **, WLAN_OPCODE_VALUE_TYPE *);

#endif /* _WLAN_WLANAPI_H */
