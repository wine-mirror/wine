name mpr
type win32

0001 stub MPR_1
0002 stub MPR_2
0003 stub MPR_3
0004 stub MPR_4
0005 stub MPR_5
0006 stub MPR_6
0007 stub MPR_7
0008 stub MPR_8
0009 stub MPR_9
0010 stub DllCanUnloadNow
0011 stub DllGetClassObject
0012 stub MPR_12
0013 stub MPR_13
0014 stub MPR_14
0015 stub MPR_15
0016 stub MPR_16
0017 stub MPR_17
0018 stub MPR_18
0019 stub MPR_19
0020 stub MPR_20
0021 stub MPR_21
0022 stdcall MPR_Alloc(long) MPR_Alloc
0023 stdcall MPR_ReAlloc(ptr long) MPR_ReAlloc
0024 stdcall MPR_Free(ptr) MPR_Free
0025 stdcall MPR_25(ptr long) _MPR_25
0026 stdcall MultinetGetConnectionPerformanceA(ptr ptr) MultinetGetConnectionPerformanceA
0027 stub MultinetGetConnectionPerformanceW
0028 stdcall MultinetGetErrorTextA(long ptr long)MultinetGetErrorTextA
0029 stdcall MultinetGetErrorTextW(long ptr long)MultinetGetErrorTextW
0030 stub NPSAuthenticationDialogA
0031 stub NPSCopyStringA
0032 stub NPSDeviceGetNumberA
0033 stub NPSDeviceGetStringA
0034 stdcall NPSGetProviderHandleA(long) NPSGetProviderHandleA
0035 stdcall NPSGetProviderNameA(long ptr) NPSGetProviderNameA
0036 stdcall NPSGetSectionNameA(long ptr) NPSGetSectionNameA
0037 stub NPSNotifyGetContextA
0038 stub NPSNotifyRegisterA
0039 stub NPSSetCustomTextA
0040 stub NPSSetExtendedErrorA
0041 stub PwdChangePasswordA
0042 stub PwdChangePasswordW
0043 stub PwdGetPasswordStatusA
0044 stub PwdGetPasswordStatusW
0045 stub PwdSetPasswordStatusA
0046 stub PwdSetPasswordStatusW
0047 stdcall WNetAddConnection2A(ptr str str long) WNetAddConnection2A
0048 stdcall WNetAddConnection2W(ptr wstr wstr long) WNetAddConnection2W
0049 stdcall WNetAddConnection3A(long ptr str str long) WNetAddConnection3A
0050 stdcall WNetAddConnection3W(long ptr wstr wstr long) WNetAddConnection3W
0051 stdcall WNetAddConnectionA(str str str) WNetAddConnectionA
0052 stdcall WNetAddConnectionW(wstr wstr wstr) WNetAddConnectionW
0053 stdcall WNetCachePassword(str long str long long) WNetCachePassword
0054 stdcall WNetCancelConnection2A(str long long) WNetCancelConnection2A
0055 stdcall WNetCancelConnection2W(wstr long long) WNetCancelConnection2W
0056 stdcall WNetCancelConnectionA(str long) WNetCancelConnectionA
0057 stdcall WNetCancelConnectionW(wstr long) WNetCancelConnectionW
0058 stub WNetCloseEnum
0059 stdcall WNetConnectionDialog1A(ptr) WNetConnectionDialog1A
0060 stdcall WNetConnectionDialog1W(ptr) WNetConnectionDialog1W
0061 stdcall WNetConnectionDialog(long long) WNetConnectionDialog
0062 stub WNetDisconnectDialog1A
0063 stub WNetDisconnectDialog1W
0064 stub WNetDisconnectDialog
0065 stdcall WNetEnumCachedPasswords(str long long ptr) WNetEnumCachedPasswords
0066 stub WNetEnumResourceA
0067 stub WNetEnumResourceW
0068 stub WNetFormatNetworkNameA
0069 stub WNetFormatNetworkNameW
0070 stdcall WNetGetCachedPassword(ptr long ptr ptr long) WNetGetCachedPassword
0071 stdcall WNetGetConnectionA(str ptr ptr) WNetGetConnectionA
0072 stdcall WNetGetConnectionW(wstr ptr ptr) WNetGetConnectionW
0073 stub WNetGetHomeDirectoryA
0074 stub WNetGetHomeDirectoryW
0075 stub WNetGetLastErrorA
0076 stub WNetGetLastErrorW
0077 stub WNetGetNetworkInformationA
0078 stub WNetGetNetworkInformationW
0079 stdcall WNetGetProviderNameA(long ptr ptr) WNetGetProviderNameA
0080 stdcall WNetGetProviderNameW(long ptr ptr) WNetGetProviderNameW
0081 stdcall WNetGetResourceInformationA(ptr ptr ptr ptr) WNetGetResourceInformationA
0082 stub WNetGetResourceInformationW
0083 stub WNetGetResourceParentA
0084 stub WNetGetResourceParentW
0085 stdcall WNetGetUniversalNameA (str long ptr ptr) WNetGetUniversalNameA
0086 stdcall WNetGetUniversalNameW (wstr long ptr ptr) WNetGetUniversalNameW
0087 stdcall WNetGetUserA(str ptr ptr) WNetGetUserA
0088 stdcall WNetGetUserW(wstr wstr ptr) WNetGetUserW
0089 stub WNetLogoffA
0090 stub WNetLogoffW
0091 stub WNetLogonA
0092 stub WNetLogonW
0093 stdcall WNetOpenEnumA(long long long ptr ptr) WNetOpenEnumA
0094 stdcall WNetOpenEnumW(long long long ptr ptr) WNetOpenEnumW
0095 stdcall WNetRemoveCachedPassword(long long long) WNetRemoveCachedPassword
0096 stub WNetRestoreConnectionA
0097 stub WNetRestoreConnectionW
0098 stub WNetSetConnectionA
0099 stub WNetSetConnectionW
0100 stdcall WNetUseConnectionA(long ptr str str long str ptr ptr) WNetUseConnectionA 
0101 stdcall WNetUseConnectionW(long ptr wstr wstr long wstr ptr ptr) WNetUseConnectionW
0102 stub WNetVerifyPasswordA
0103 stub WNetVerifyPasswordW 

#additions, not in win95 mpr.dll
0104 stub WNetRestoreConnection
0105 stub WNetLogonNotify
0106 stub WNetPasswordChangeNotify
0107 stub WNetGetPropertyTextA
0108 stub WNetPropertyDialogA
0109 stub WNetGetDirectoryTypeA
0110 stub WNetFMXGetPermHelp
0111 stub WNetFMXEditPerm
0112 stub WNetFMXGetPermCaps
