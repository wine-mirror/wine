/*
 * Copyright 2018 Gijs Vermeulen
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

#ifndef __WINE_QOS2_H__
#define __WINE_QOS2_H__

#include "wine/winheader_enter.h"

#include <ws2tcpip.h>
#include <mstcpip.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef ULONG QOS_FLOWID, *PQOS_FLOWID;

typedef enum _QOS_SHAPING {
    QOSShapeOnly,
    QOSShapeAndMark,
    QOSUseNonConformantMarkings
} QOS_SHAPING, *PQOS_SHAPING;

typedef enum _QOS_FLOWRATE_REASON {
    QOSFlowRateNotApplicable,
    QOSFlowRateContentChange,
    QOSFlowRateCongestion,
    QOSFlowRateHigherContentEncoding,
    QOSFlowRateUserCaused
} QOS_FLOWRATE_REASON, PQOS_FLOWRATE_REASON;

typedef enum _QOS_NOTIFY_FLOW {
    QOSNotifyCongested,
    QOSNotifyUncongested,
    QOSNotifyAvailable
} QOS_NOTIFY_FLOW, *PQOS_NOTIFY_FLOW;

typedef enum _QOS_QUERY_FLOW {
    QOSQueryFlowFundamentals,
    QOSQueryPacketPriority,
    QOSQueryOutgoingRate
} QOS_QUERY_FLOW;

typedef enum _QOS_SET_FLOW {
    QOSSetTrafficType,
    QOSSetOutgoingRate,
    QOSSetOutgoingDSCPValue
} QOS_SET_FLOW, *PQOS_SET_FLOW;

typedef enum _QOS_TRAFFIC_TYPE {
    QOSTrafficTypeBestEffort,
    QOSTrafficTypeBackground,
    QOSTrafficTypeExcellentEffort,
    QOSTrafficTypeAudioVideo,
    QOSTrafficTypeVoice,
    QOSTrafficTypeControl
} QOS_TRAFFIC_TYPE, *PQOS_TRAFFIC_TYPE;

typedef struct _QOS_FLOW_FUNDAMENTALS {
    BOOL   BottleneckBandwidthSet;
    UINT64 BottleneckBandwidth;
    BOOL   AvailableBandwidthSet;
    UINT64 AvailableBandwidth;
    BOOL   RTTSet;
    UINT32 RTT;
} QOS_FLOW_FUNDAMENTALS, *PQOS_FLOW_FUNDAMENTALS;

typedef struct _QOS_FLOWRATE_OUTGOING {
    UINT64              Bandwidth;
    QOS_SHAPING         ShapingBehavior;
    QOS_FLOWRATE_REASON Reason;
} QOS_FLOWRATE_OUTGOING, *PQOS_FLOWRATE_OUTGOING;

typedef struct _QOS_PACKET_PRIORITY {
    ULONG ConformantDSCPValue;
    ULONG NonConformantDSCPValue;
    ULONG ConformantL2Value;
    ULONG NonConformantL2Value;
} QOS_PACKET_PRIORITY, *PQOS_PACKET_PRIORITY;

typedef struct _QOS_VERSION {
    USHORT MajorVersion;
    USHORT MinorVersion;
} QOS_VERSION, *PQOS_VERSION;

#define QOS_OUTGOING_DEFAULT_MINIMUM_BANDWIDTH  0xFFFFFFFF

#define QOS_QUERYFLOW_FRESH     0x00000001
#define QOS_NON_ADAPTIVE_FLOW   0x00000002

BOOL WINAPI QOSAddSocketToFlow(HANDLE handle, SOCKET socket, PSOCKADDR addr,
                               QOS_TRAFFIC_TYPE traffictype, DWORD flags, PQOS_FLOWID flowid);
BOOL WINAPI QOSCancel(HANDLE handle, LPOVERLAPPED overlap);
BOOL WINAPI QOSCloseHandle(HANDLE handle);
BOOL WINAPI QOSCreateHandle(PQOS_VERSION version, PHANDLE handle);
BOOL WINAPI QOSEnumerateFlows(HANDLE handle, PULONG size, PVOID buffer);
BOOL WINAPI QOSNotifyFlow(HANDLE handle, QOS_FLOWID flowid, QOS_NOTIFY_FLOW op, PULONG size,
                          PVOID buffer, DWORD flags, LPOVERLAPPED overlap);
BOOL WINAPI QOSQueryFlow(HANDLE handle, QOS_FLOWID flowid, QOS_QUERY_FLOW op, PULONG size,
                         PVOID buffer, DWORD flags, LPOVERLAPPED overlap);
BOOL WINAPI QOSRemoveSocketFromFlow(HANDLE handle, SOCKET socket, QOS_FLOWID flowid, DWORD flags);
BOOL WINAPI QOSSetFlow(HANDLE handle, QOS_FLOWID flowid, QOS_SET_FLOW op, ULONG size,
                       PVOID buffer, DWORD flags, LPOVERLAPPED overlap);
BOOL WINAPI QOSStartTrackingClient(HANDLE handle, PSOCKADDR addr, DWORD flags);
BOOL WINAPI QOSStopTrackingClient(HANDLE handle, PSOCKADDR addr, DWORD flags);

#ifdef __cplusplus
}
#endif

#include "wine/winheader_exit.h"

#endif /* __WINE_QOS2_H__ */
