name mpr
type win32

import kernel32.dll

# ordinal exports
 1 stub @
 2 stub @
 3 stub @
 4 stub @
 5 stub @
 6 stub @
 7 stub @
 8 stub @
 9 stub @
12 stub @
13 stub @
14 stub @
15 stub @
16 stub @
17 stub @
18 stub @
19 stub @
20 stub @
21 stub @
22 stdcall @(long) MPR_Alloc
23 stdcall @(ptr long) MPR_ReAlloc
24 stdcall @(ptr) MPR_Free
25 stdcall @(ptr long) _MPR_25

@ stub DllCanUnloadNow
@ stub DllGetClassObject
@ stdcall MultinetGetConnectionPerformanceA(ptr ptr) MultinetGetConnectionPerformanceA
@ stdcall MultinetGetConnectionPerformanceW(ptr ptr) MultinetGetConnectionPerformanceW
@ stdcall MultinetGetErrorTextA(long ptr long)MultinetGetErrorTextA
@ stdcall MultinetGetErrorTextW(long ptr long)MultinetGetErrorTextW
@ stdcall NPSAuthenticationDialogA(ptr) NPSAuthenticationDialogA
@ stdcall NPSCopyStringA(str ptr ptr) NPSCopyStringA
@ stdcall NPSDeviceGetNumberA(str ptr ptr) NPSDeviceGetNumberA
@ stdcall NPSDeviceGetStringA(long long ptr long) NPSDeviceGetStringA
@ stdcall NPSGetProviderHandleA(ptr) NPSGetProviderHandleA
@ stdcall NPSGetProviderNameA(long ptr) NPSGetProviderNameA
@ stdcall NPSGetSectionNameA(long ptr) NPSGetSectionNameA
@ stdcall NPSNotifyGetContextA(ptr) NPSNotifyGetContextA
@ stdcall NPSNotifyRegisterA(long ptr) NPSNotifyRegisterA
@ stdcall NPSSetCustomTextA(str) NPSSetCustomTextA
@ stdcall NPSSetExtendedErrorA(long str) NPSSetExtendedErrorA
@ stub PwdChangePasswordA
@ stub PwdChangePasswordW
@ stub PwdGetPasswordStatusA
@ stub PwdGetPasswordStatusW
@ stub PwdSetPasswordStatusA
@ stub PwdSetPasswordStatusW
@ stdcall WNetAddConnection2A(ptr str str long) WNetAddConnection2A
@ stdcall WNetAddConnection2W(ptr wstr wstr long) WNetAddConnection2W
@ stdcall WNetAddConnection3A(long ptr str str long) WNetAddConnection3A
@ stdcall WNetAddConnection3W(long ptr wstr wstr long) WNetAddConnection3W
@ stdcall WNetAddConnectionA(str str str) WNetAddConnectionA
@ stdcall WNetAddConnectionW(wstr wstr wstr) WNetAddConnectionW
@ stdcall WNetCachePassword(str long str long long long) WNetCachePassword
@ stdcall WNetCancelConnection2A(str long long) WNetCancelConnection2A
@ stdcall WNetCancelConnection2W(wstr long long) WNetCancelConnection2W
@ stdcall WNetCancelConnectionA(str long) WNetCancelConnectionA
@ stdcall WNetCancelConnectionW(wstr long) WNetCancelConnectionW
@ stdcall WNetCloseEnum(long) WNetCloseEnum
@ stdcall WNetConnectionDialog1A(ptr) WNetConnectionDialog1A
@ stdcall WNetConnectionDialog1W(ptr) WNetConnectionDialog1W
@ stdcall WNetConnectionDialog(long long) WNetConnectionDialog
@ stdcall WNetDisconnectDialog1A(ptr) WNetDisconnectDialog1A
@ stdcall WNetDisconnectDialog1W(ptr) WNetDisconnectDialog1W
@ stdcall WNetDisconnectDialog(long long) WNetDisconnectDialog
@ stdcall WNetEnumCachedPasswords(str long long ptr long) WNetEnumCachedPasswords
@ stdcall WNetEnumResourceA(long ptr ptr ptr) WNetEnumResourceA
@ stdcall WNetEnumResourceW(long ptr ptr ptr) WNetEnumResourceW
@ stub WNetFormatNetworkNameA
@ stub WNetFormatNetworkNameW
@ stdcall WNetGetCachedPassword(ptr long ptr ptr long) WNetGetCachedPassword
@ stdcall WNetGetConnectionA(str ptr ptr) WNetGetConnectionA
@ stdcall WNetGetConnectionW(wstr ptr ptr) WNetGetConnectionW
@ stub WNetGetHomeDirectoryA
@ stub WNetGetHomeDirectoryW
@ stdcall WNetGetLastErrorA(ptr ptr long ptr long) WNetGetLastErrorA
@ stdcall WNetGetLastErrorW(ptr ptr long ptr long) WNetGetLastErrorW
@ stdcall WNetGetNetworkInformationA(str ptr) WNetGetNetworkInformationA
@ stdcall WNetGetNetworkInformationW(wstr ptr) WNetGetNetworkInformationW
@ stdcall WNetGetProviderNameA(long ptr ptr) WNetGetProviderNameA
@ stdcall WNetGetProviderNameW(long ptr ptr) WNetGetProviderNameW
@ stdcall WNetGetResourceInformationA(ptr ptr ptr ptr) WNetGetResourceInformationA
@ stdcall WNetGetResourceInformationW(ptr ptr ptr ptr) WNetGetResourceInformationW
@ stdcall WNetGetResourceParentA(ptr ptr ptr) WNetGetResourceParentA
@ stdcall WNetGetResourceParentW(ptr ptr ptr) WNetGetResourceParentW
@ stdcall WNetGetUniversalNameA (str long ptr ptr) WNetGetUniversalNameA
@ stdcall WNetGetUniversalNameW (wstr long ptr ptr) WNetGetUniversalNameW
@ stdcall WNetGetUserA(str ptr ptr) WNetGetUserA
@ stdcall WNetGetUserW(wstr wstr ptr) WNetGetUserW
@ stdcall WNetLogoffA(str long) WNetLogoffA
@ stdcall WNetLogoffW(wstr long) WNetLogoffW
@ stdcall WNetLogonA(str long) WNetLogonA
@ stdcall WNetLogonW(wstr long) WNetLogonW
@ stdcall WNetOpenEnumA(long long long ptr ptr) WNetOpenEnumA
@ stdcall WNetOpenEnumW(long long long ptr ptr) WNetOpenEnumW
@ stdcall WNetRemoveCachedPassword(long long long) WNetRemoveCachedPassword
@ stdcall WNetRestoreConnectionA(long str) WNetRestoreConnectionA
@ stdcall WNetRestoreConnectionW(long wstr) WNetRestoreConnectionW
@ stdcall WNetSetConnectionA(str long ptr) WNetSetConnectionA
@ stdcall WNetSetConnectionW(wstr long ptr) WNetSetConnectionW
@ stdcall WNetUseConnectionA(long ptr str str long str ptr ptr) WNetUseConnectionA 
@ stdcall WNetUseConnectionW(long ptr wstr wstr long wstr ptr ptr) WNetUseConnectionW
@ stdcall WNetVerifyPasswordA(str ptr) WNetVerifyPasswordA
@ stdcall WNetVerifyPasswordW(wstr ptr) WNetVerifyPasswordW

#additions, not in win95 mpr.dll
@ stub WNetRestoreConnection
@ stub WNetLogonNotify
@ stub WNetPasswordChangeNotify
@ stub WNetGetPropertyTextA
@ stub WNetPropertyDialogA
@ stub WNetGetDirectoryTypeA
@ stub WNetFMXGetPermHelp
@ stub WNetFMXEditPerm
@ stub WNetFMXGetPermCaps
