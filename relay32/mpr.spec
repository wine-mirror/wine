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
0009 stub DllCanUnloadNow
0010 stub DllGetClassObject
0011 stub MPR_11
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
0022 stdcall MPR_22(long) _MPR_22
0023 stub MPR_23
0024 stub MPR_24
0025 stdcall MultinetGetConnectionPerformanceA(ptr ptr) MultinetGetConnectionPerformance32A
0026 stub MultinetGetConnectionPerformanceW
0027 stdcall MultinetGetErrorTextA(long ptr long)MultinetGetErrorText32A
0028 stdcall MultinetGetErrorTextW(long ptr long)MultinetGetErrorText32W
0029 stub NPSAuthenticationDialogA
0030 stub NPSCopyStringA
0031 stub NPSDeviceGetNumberA
0032 stub NPSDeviceGetStringA
0033 stdcall NPSGetProviderHandleA(long) NPSGetProviderHandle32A
0034 stub NPSGetProviderNameA
0035 stub NPSGetSectionNameA
0036 stub NPSNotifyGetContextA
0037 stub NPSNotifyRegisterA
0038 stub NPSSetCustomTextA
0039 stub NPSSetExtendedErrorA
0040 stub PwdChangePasswordA
0041 stub PwdChangePasswordW
0042 stub PwdGetPasswordStatusA
0043 stub PwdGetPasswordStatusW
0044 stub PwdSetPasswordStatusA
0045 stub PwdSetPasswordStatusW
0046 stdcall WNetAddConnection2A(ptr str str long) WNetAddConnection2_32A
0047 stdcall WNetAddConnection2W(ptr wstr wstr long) WNetAddConnection2_32W
0048 stdcall WNetAddConnection3A(long ptr str str long) WNetAddConnection3_32A
0049 stdcall WNetAddConnection3W(long ptr wstr wstr long) WNetAddConnection3_32W
0050 stdcall WNetAddConnectionA(str str str) WNetAddConnection32A
0051 stdcall WNetAddConnectionW(wstr wstr wstr) WNetAddConnection32W
0052 stdcall WNetCachePassword(str long str long long) WNetCachePassword
0053 stub WNetCancelConnection2A
0054 stub WNetCancelConnection2W
0055 stub WNetCancelConnectionA
0056 stub WNetCancelConnectionW
0057 stub WNetCloseEnum
0058 stdcall WNetConnectionDialog1A(ptr) WNetConnectionDialog1_32A
0059 stdcall WNetConnectionDialog1W(ptr) WNetConnectionDialog1_32W
0060 stdcall WNetConnectionDialog(long long) WNetConnectionDialog_32
0061 stub WNetDisconnectDialog1A
0062 stub WNetDisconnectDialog1W
0063 stub WNetDisconnectDialog
0064 stdcall WNetEnumCachedPasswords(str long long ptr) WNetEnumCachedPasswords32
0065 stub WNetEnumResourceA
0066 stub WNetEnumResourceW
0067 stub WNetFormatNetworkNameA
0068 stub WNetFormatNetworkNameW
0069 stdcall WNetGetCachedPassword(ptr long ptr ptr long) WNetGetCachedPassword
0070 stdcall WNetGetConnectionA(str ptr ptr) WNetGetConnection32A
0071 stdcall WNetGetConnectionW(wstr ptr ptr) WNetGetConnection32W
0072 stub WNetGetHomeDirectoryA
0073 stub WNetGetHomeDirectoryW
0074 stub WNetGetLastErrorA
0075 stub WNetGetLastErrorW
0076 stub WNetGetNetworkInformationA
0077 stub WNetGetNetworkInformationW
0078 stub WNetGetProviderNameA
0079 stub WNetGetProviderNameW
0080 stdcall WNetGetResourceInformationA(ptr ptr ptr ptr) WNetGetResourceInformation32A
0081 stub WNetGetResourceInformationW
0082 stub WNetGetResourceParentA
0083 stub WNetGetResourceParentW
0084 stub WNetGetUniversalNameA
0085 stub WNetGetUniversalNameW
0086 stdcall WNetGetUserA(str ptr ptr) WNetGetUser32A
0087 stub WNetGetUserW
0088 stub WNetLogoffA
0089 stub WNetLogoffW
0090 stub WNetLogonA
0091 stub WNetLogonW
0092 stdcall WNetOpenEnumA(long long long ptr ptr) WNetOpenEnum32A
0093 stdcall WNetOpenEnumW(long long long ptr ptr) WNetOpenEnum32W
0094 stub WNetRemoveCachedPassword
0095 stub WNetRestoreConnectionA
0096 stub WNetRestoreConnectionW
0097 stub WNetSetConnectionA
0098 stub WNetSetConnectionW
0099 stub WNetUseConnectionA
0100 stub WNetUseConnectionW
0101 stub WNetVerifyPasswordA
0102 stub WNetVerifyPasswordW 

#additions, not in win95 mpr.dll
0103 stub WNetRestoreConnection
0104 stub WNetLogonNotify
0105 stub WNetPasswordChangeNotify
0106 stub WNetGetPropertyTextA
0107 stub WNetPropertyDialogA
0108 stub WNetGetDirectoryTypeA
0109 stub WNetFMXGetPermHelp
0110 stub WNetFMXEditPerm
0111 stub WNetFMXGetPermCaps
