@ stub RasAutoDialSharedConnection
@ stdcall RasConnectionNotificationW(ptr long long) rasapi32.RasConnectionNotificationW
@ stdcall RasDialW(ptr wstr ptr long ptr ptr) rasapi32.RasDialW
@ stdcall RasEnumConnectionsW(ptr ptr ptr) rasapi32.RasEnumConnectionsW
@ stdcall RasEnumEntriesW(wstr wstr ptr ptr ptr) rasapi32.RasEnumEntriesW
@ stdcall RasGetAutodialAddressA(str ptr ptr ptr ptr) rasapi32.RasGetAutodialAddressA
@ stdcall RasGetAutodialAddressW(wstr ptr ptr ptr ptr) rasapi32.RasGetAutodialAddressW
@ stdcall RasGetConnectionStatistics(ptr ptr) rasapi32.RasGetConnectionStatistics
@ stdcall RasGetConnectStatusW(ptr ptr) rasapi32.RasGetConnectStatusW
@ stub RasGetCredentialsW
@ stdcall RasGetEntryDialParamsW(wstr ptr ptr) rasapi32.RasGetEntryDialParamsW
@ stub RasGetEntryHrasconnW
@ stdcall RasGetEntryPropertiesW(wstr wstr ptr ptr ptr ptr) rasapi32.RasGetEntryPropertiesW
@ stdcall RasGetProjectionInfoA(ptr ptr ptr ptr) rasapi32.RasGetProjectionInfoA
@ stdcall RasHangUpW(long) rasapi32.RasHangUpW
@ stub RasQuerySharedAutoDial
@ stub RasQuerySharedConnection
@ stdcall RasSetAutodialAddressW(wstr long ptr long long) rasapi32.RasSetAutodialAddressW
@ stdcall RasSetEntryDialParamsW(wstr ptr long) rasapi32.RasSetEntryDialParamsW
