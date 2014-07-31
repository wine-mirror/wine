@ stdcall AddIPAddress( long long long ptr ptr )
@ stub AllocateAndGetArpEntTableFromStack
@ stdcall AllocateAndGetIfTableFromStack( ptr long long long )
#@ stub AllocateAndGetInterfaceInfoFromStack
@ stdcall AllocateAndGetIpAddrTableFromStack( ptr long long long )
@ stdcall AllocateAndGetIpForwardTableFromStack( ptr long long long )
@ stdcall AllocateAndGetIpNetTableFromStack( ptr long long long )
@ stdcall AllocateAndGetTcpTableFromStack( ptr long long long )
@ stdcall AllocateAndGetUdpTableFromStack( ptr long long long )
@ stdcall CancelIPChangeNotify( ptr )
#@ stub CancelMibChangeNotify2
#@ stub ConvertGuidToStringA
#@ stub ConvertGuidToStringW
#@ stub ConvertInterfaceAliasToLuid
#@ stub ConvertInterfaceGuidToLuid
#@ stub ConvertInterfaceIndexToLuid
#@ stub ConvertInterfaceLuidToAlias
#@ stub ConvertInterfaceLuidToGuid
#@ stub ConvertInterfaceLuidToIndex
#@ stub ConvertInterfaceLuidToNameA
#@ stub ConvertInterfaceLuidToNameW
#@ stub ConvertInterfaceNameToLuidA
#@ stub ConvertInterfaceNameToLuidW
#@ stub ConvertInterfacePhysicalAddressToLuid
#@ stub ConvertIpv4MaskToLength
#@ stub ConvertLengthToIpv4Mask
#@ stub ConvertRemoteInterfaceAliasToLuid
#@ stub ConvertRemoteInterfaceGuidToLuid
#@ stub ConvertRemoteInterfaceIndexToLuid
#@ stub ConvertRemoteInterfaceLuidToAlias
#@ stub ConvertRemoteInterfaceLuidToGuid
#@ stub ConvertRemoteInterfaceLuidToIndex
#@ stub ConvertStringToGuidA
#@ stub ConvertStringToGuidW
#@ stub ConvertStringToInterfacePhysicalAddress
#@ stub CPNatfwtCreateProviderInstance
#@ stub CPNatfwtDeregisterProviderInstance
#@ stub CPNatfwtDestroyProviderInstance
#@ stub CPNatfwtIndicateReceivedBuffers
#@ stub CPNatfwtRegisterProviderInstance
#@ stub CreateAnycastIpAddressEntry
#@ stub CreateIpForwardEntry2
@ stdcall CreateIpForwardEntry( ptr )
#@ stub CreateIpNetEntry2
@ stdcall CreateIpNetEntry( ptr )
#@ stub CreatePersistentTcpPortReservation
#@ stub CreatePersistentUdpPortReservation
@ stdcall CreateProxyArpEntry( long long long )
#@ stub CreateSortedAddressPairs
#@ stub CreateUnicastIpAddressEntry
#@ stub DeleteAnycastIpAddressEntry
@ stdcall DeleteIPAddress( long )
#@ stub DeleteIpForwardEntry2
@ stdcall DeleteIpForwardEntry( ptr )
#@ stub DeleteIpNetEntry2
@ stdcall DeleteIpNetEntry( ptr )
#@ stub DeletePersistentTcpPortReservation
#@ stub DeletePersistentUdpPortReservation
@ stdcall DeleteProxyArpEntry( long long long )
#@ stub DeleteUnicastIpAddressEntry
#@ stub DisableMediaSense
#@ stub do_echo_rep
#@ stub do_echo_req
@ stdcall EnableRouter( ptr ptr )
@ stdcall FlushIpNetTable( long )
#@ stub FlushIpNetTable2
@ stub FlushIpNetTableFromStack
#@ stub FlushIpPathTable
#@ stub FreeMibTable
@ stdcall GetAdapterIndex( wstr ptr )
@ stub GetAdapterOrderMap
@ stdcall GetAdaptersAddresses( long long ptr ptr ptr )
@ stdcall GetAdaptersInfo( ptr ptr )
#@ stub GetAnycastIpAddressEntry
#@ stub GetAnycastIpAddressTable
@ stdcall GetBestInterface( long ptr )
@ stdcall GetBestInterfaceEx( ptr ptr )
@ stub GetBestInterfaceFromStack
@ stdcall GetBestRoute( long long long )
#@ stub GetBestRoute2
@ stub GetBestRouteFromStack
#@ stub GetCurrentThreadCompartmentId
@ stdcall GetExtendedTcpTable( ptr ptr long long long long )
@ stdcall GetExtendedUdpTable( ptr ptr long long long long )
@ stdcall GetFriendlyIfIndex( long )
@ stdcall GetIcmpStatisticsEx( ptr long )
@ stdcall GetIcmpStatistics( ptr )
@ stub GetIcmpStatsFromStack
@ stdcall GetIfEntry( ptr )
#@ stub GetIfEntry2
@ stub GetIfEntryFromStack
#@ stub GetIfStackTable
@ stdcall GetIfTable( ptr ptr long )
#@ stub GetIfTable2
#@ stub GetIfTable2Ex
@ stub GetIfTableFromStack
@ stub GetIgmpList
@ stdcall GetInterfaceInfo( ptr ptr )
#@ stub GetInvertedIfStackTable
@ stdcall GetIpAddrTable( ptr ptr long )
@ stub GetIpAddrTableFromStack
#@ stub GetIpErrorString
#@ stub GetIpForwardEntry2
@ stdcall GetIpForwardTable( ptr ptr long )
#@ stub GetIpForwardTable2
@ stub GetIpForwardTableFromStack
#@ stub GetIpInterfaceEntry
#@ stub GetIpInterfaceTable
#@ stub GetIpNetEntry2
@ stdcall GetIpNetTable( ptr ptr long )
#@ stub GetIpNetTable2
@ stub GetIpNetTableFromStack
#@ stub GetIpPathEntry
#@ stub GetIpPathTable
@ stdcall GetIpStatisticsEx( ptr long )
@ stdcall GetIpStatistics( ptr )
@ stub GetIpStatsFromStack
#@ stub GetMulticastIpAddressEntry
#@ stub GetMulticastIpAddressTable
#@ stub GetNetworkInformation
@ stdcall GetNetworkParams( ptr ptr )
@ stdcall GetNumberOfInterfaces( ptr )
#@ stub GetOwnerModuleFromPidAndInfo
#@ stub GetOwnerModuleFromTcp6Entry
#@ stub GetOwnerModuleFromTcpEntry
#@ stub GetOwnerModuleFromUdp6Entry
#@ stub GetOwnerModuleFromUdpEntry
@ stdcall GetPerAdapterInfo( long ptr ptr )
#@ stub GetPerTcp6ConnectionEStats
#@ stub GetPerTcp6ConnectionStats
#@ stub GetPerTcpConnectionEStats
#@ stub GetPerTcpConnectionStats
@ stdcall GetRTTAndHopCount( long ptr long ptr )
#@ stub GetSessionCompartmentId
@ stdcall GetTcp6Table( ptr ptr long )
@ stdcall GetTcp6Table2( ptr ptr long )
@ stdcall GetTcpStatisticsEx( ptr long )
@ stdcall GetTcpStatistics( ptr )
@ stub GetTcpStatsFromStack
@ stdcall GetTcpTable( ptr ptr long )
@ stdcall GetTcpTable2( ptr ptr long )
@ stub GetTcpTableFromStack
#@ stub GetTeredoPort
#@ stub GetUdp6Table
@ stdcall GetUdpStatisticsEx( ptr long )
@ stdcall GetUdpStatistics( ptr )
@ stub GetUdpStatsFromStack
@ stdcall GetUdpTable( ptr ptr long )
@ stub GetUdpTableFromStack
#@ stub GetUnicastIpAddressEntry
#@ stub GetUnicastIpAddressTable
@ stdcall GetUniDirectionalAdapterInfo( ptr ptr )
#@ stub Icmp6CreateFile
#@ stub Icmp6ParseReplies
#@ stub Icmp6SendEcho2
@ stdcall IcmpCloseHandle(ptr)
@ stdcall IcmpCreateFile()
@ stub IcmpParseReplies
@ stdcall IcmpSendEcho2Ex(ptr ptr ptr ptr long long ptr long ptr ptr long long)
@ stdcall IcmpSendEcho2(ptr ptr ptr ptr long ptr long ptr ptr long long)
@ stdcall IcmpSendEcho(ptr long ptr long ptr ptr long long)
#@ stub if_indextoname
#@ stub if_nametoindex
#@ stub InitializeIpForwardEntry
#@ stub InitializeIpInterfaceEntry
#@ stub InitializeUnicastIpAddressEntry
#@ stub InternalCleanupPersistentStore
#@ stub InternalCreateAnycastIpAddressEntry
@ stub InternalCreateIpForwardEntry
#@ stub InternalCreateIpForwardEntry2
@ stub InternalCreateIpNetEntry
#@ stub InternalCreateIpNetEntry2
#@ stub InternalCreateUnicastIpAddressEntry
#@ stub InternalDeleteAnycastIpAddressEntry
@ stub InternalDeleteIpForwardEntry
#@ stub InternalDeleteIpForwardEntry2
@ stub InternalDeleteIpNetEntry
#@ stub InternalDeleteIpNetEntry2
#@ stub InternalDeleteUnicastIpAddressEntry
#@ stub InternalFindInterfaceByAddress
#@ stub InternalGetAnycastIpAddressEntry
#@ stub InternalGetAnycastIpAddressTable
#@ stub InternalGetForwardIpTable2
#@ stub InternalGetIfEntry2
@ stub InternalGetIfTable
#@ stub InternalGetIfTable2
@ stub InternalGetIpAddrTable
#@ stub InternalGetIpForwardEntry2
@ stub InternalGetIpForwardTable
#@ stub InternalGetIpInterfaceEntry
#@ stub InternalGetIpInterfaceTable
#@ stub InternalGetIpNetEntry2
@ stub InternalGetIpNetTable
#@ stub InternalGetIpNetTable2
#@ stub InternalGetMulticastIpAddressEntry
#@ stub InternalGetMulticastIpAddressTable
#@ stub InternalGetTcp6Table2
#@ stub InternalGetTcp6TableWithOwnerModule
#@ stub InternalGetTcp6TableWithOwnerPid
@ stub InternalGetTcpTable
#@ stub InternalGetTcpTable2
#@ stub InternalGetTcpTableEx
#@ stub InternalGetTcpTableWithOwnerModule
#@ stub InternalGetTcpTableWithOwnerPid
#@ stub InternalGetTunnelPhysicalAdapter
#@ stub InternalGetUdp6TableWithOwnerModule
#@ stub InternalGetUdp6TableWithOwnerPid
@ stub InternalGetUdpTable
#@ stub InternalGetUdpTableEx
#@ stub InternalGetUdpTableWithOwnerModule
#@ stub InternalGetUdpTableWithOwnerPid
#@ stub InternalGetUnicastIpAddressEntry
#@ stub InternalGetUnicastIpAddressTable
@ stub InternalSetIfEntry
@ stub InternalSetIpForwardEntry
#@ stub InternalSetIpForwardEntry2
#@ stub InternalSetIpInterfaceEntry
@ stub InternalSetIpNetEntry
#@ stub InternalSetIpNetEntry2
@ stub InternalSetIpStats
@ stub InternalSetTcpEntry
#@ stub InternalSetTeredoPort
#@ stub InternalSetUnicastIpAddressEntry
@ stdcall IpReleaseAddress( ptr )
@ stdcall IpRenewAddress( ptr )
@ stub IsLocalAddress
#@ stub LookupPersistentTcpPortReservation
#@ stub LookupPersistentUdpPortReservation
@ stub NhGetGuidFromInterfaceName
#@ stub NhGetInterfaceDescriptionFromGuid
#@ stub NhGetInterfaceNameFromDeviceGuid
@ stub NhGetInterfaceNameFromGuid
@ stub NhpAllocateAndGetInterfaceInfoFromStack
@ stub NhpGetInterfaceIndexFromStack
@ stdcall NotifyAddrChange( ptr ptr )
#@ stub NotifyIpInterfaceChange
@ stdcall NotifyRouteChange( ptr ptr )
#@ stub NotifyRouteChange2
@ stub NotifyRouteChangeEx
#@ stub NotifyStableUnicastIpAddressTable
#@ stub NotifyTeredoPortChange
#@ stub NotifyUnicastIpAddressChange
#@ stub NTPTimeToNTFileTime
#@ stub NTTimeToNTPTime
#@ stub ParseNetworkString
@ stub _PfAddFiltersToInterface@24
@ stub _PfAddGlobalFilterToInterface@8
@ stdcall _PfBindInterfaceToIPAddress@12(long long ptr) PfBindInterfaceToIPAddress
@ stub _PfBindInterfaceToIndex@16
@ stdcall _PfCreateInterface@24(long long long long long ptr) PfCreateInterface
@ stub _PfDeleteInterface@4
@ stub _PfDeleteLog@0
@ stub _PfGetInterfaceStatistics@16
@ stub _PfMakeLog@4
@ stub _PfRebindFilters@8
@ stub _PfRemoveFilterHandles@12
@ stub _PfRemoveFiltersFromInterface@20
@ stub _PfRemoveGlobalFilterFromInterface@8
@ stub _PfSetLogBuffer@28
@ stub _PfTestPacket@20
@ stub _PfUnBindInterface@4
#@ stub register_icmp
#@ stub ResolveIpNetEntry2
#@ stub ResolveNeighbor
#@ stub RestoreMediaSense
@ stdcall SendARP( long long ptr ptr )
@ stub SetAdapterIpAddress
@ stub SetBlockRoutes
#@ stub SetCurrentThreadCompartmentId
@ stdcall SetIfEntry( ptr )
@ stub SetIfEntryToStack
#@ stub SetIpForwardEntry2
@ stdcall SetIpForwardEntry( ptr )
@ stub SetIpForwardEntryToStack
#@ stub SetIpInterfaceEntry
@ stub SetIpMultihopRouteEntryToStack
#@ stub SetIpNetEntry2
@ stdcall SetIpNetEntry( ptr )
@ stub SetIpNetEntryToStack
@ stub SetIpRouteEntryToStack
#@ stub SetIpStatisticsEx
@ stdcall SetIpStatistics( ptr )
@ stub SetIpStatsToStack
@ stdcall SetIpTTL( long )
#@ stub SetNetworkInformation
#@ stub SetPerTcp6ConnectionEStats
#@ stub SetPerTcp6ConnectionStats
#@ stub SetPerTcpConnectionEStats
#@ stub SetPerTcpConnectionStats
@ stub SetProxyArpEntryToStack
@ stub SetRouteWithRef
#@ stub SetSessionCompartmentId
@ stdcall SetTcpEntry( ptr )
@ stub SetTcpEntryToStack
#@ stub SetUnicastIpAddressEntry
@ stdcall UnenableRouter( ptr ptr )
