name	advapi32
type	win32
base	1

0000 stub AbortSystemShutdownA
0001 stub AbortSystemShutdownW
0002 stub AccessCheck
0003 stub AccessCheckAndAuditAlarmA
0004 stub AccessCheckAndAuditAlarmW
0005 stub AddAccessAllowedAce
0006 stub AddAccessDeniedAce
0007 stub AddAce
0008 stub AddAuditAccessAce
0009 stub AdjustTokenGroups
0010 stub AdjustTokenPrivileges
0011 stub AllocateAndInitializeSid
0012 stub AllocateLocallyUniqueId
0013 stub AreAllAccessesGranted
0014 stub AreAnyAccessesGranted
0015 stub BackupEventLogA
0016 stub BackupEventLogW
0017 stub ChangeServiceConfigA
0018 stub ChangeServiceConfigW
0019 stub ClearEventLogA
0020 stub ClearEventLogW
0021 stub CloseEventLog
0022 stub CloseServiceHandle
0023 stub ControlService
0024 stub CopySid
0025 stub CreatePrivateObjectSecurity
0026 stub CreateProcessAsUserA
0027 stub CreateProcessAsUserW
0028 stub CreateServiceA
0029 stub CreateServiceW
0030 stub DeleteAce
0031 stub DeleteService
0032 stub DeregisterEventSource
0033 stub DestroyPrivateObjectSecurity
0034 stub DuplicateToken
0035 stub EnumDependentServicesA
0036 stub EnumDependentServicesW
0037 stub EnumServicesStatusA
0038 stub EnumServicesStatusW
0039 stub EqualPrefixSid
0040 stub EqualSid
0041 stub FindFirstFreeAce
0042 stub FreeSid
0043 stub GetAce
0044 stub GetAclInformation
0045 stub GetFileSecurityA
0046 stub GetFileSecurityW
0047 stub GetKernelObjectSecurity
0048 stub GetLengthSid
0049 stub GetNumberOfEventLogRecords
0050 stub GetOldestEventLogRecord
0051 stub GetPrivateObjectSecurity
0052 stub GetSecurityDescriptorControl
0053 stub GetSecurityDescriptorDacl
0054 stub GetSecurityDescriptorGroup
0055 stub GetSecurityDescriptorLength
0056 stub GetSecurityDescriptorOwner
0057 stub GetSecurityDescriptorSacl
0058 stub GetServiceDisplayNameA
0059 stub GetServiceDisplayNameW
0060 stub GetServiceKeyNameA
0061 stub GetServiceKeyNameW
0062 stub GetSidIdentifierAuthority
0063 stub GetSidLengthRequired
0064 stub GetSidSubAuthority
0065 stub GetSidSubAuthorityCount
0066 stub GetTokenInformation
0067 stdcall GetUserNameA(ptr ptr) GetUserName32A
0068 stdcall GetUserNameW(ptr ptr) GetUserName32W
0069 stub ImpersonateLoggedOnUser
0070 stub ImpersonateNamedPipeClient
0071 stub ImpersonateSelf
0072 stub InitializeAcl
0073 return InitializeSecurityDescriptor 8 1
0074 stub InitializeSid
0075 stub InitiateSystemShutdownA
0076 stub InitiateSystemShutdownW
0077 stub IsTextUnicode
0078 stub IsValidAcl
0079 stub IsValidSecurityDescriptor
0080 stub IsValidSid
0081 stub LockServiceDatabase
0082 stub LogonUserA
0083 stub LogonUserW
0084 stub LookupAccountNameA
0085 stub LookupAccountNameW
0086 stub LookupAccountSidA
0087 stub LookupAccountSidW
0088 stub LookupPrivilegeDisplayNameA
0089 stub LookupPrivilegeDisplayNameW
0090 stub LookupPrivilegeNameA
0091 stub LookupPrivilegeNameW
0092 stub LookupPrivilegeValueA
0093 stub LookupPrivilegeValueW
0094 stub MakeAbsoluteSD
0095 stub MakeSelfRelativeSD
0096 stub MapGenericMask
0097 stub NotifyBootConfigStatus
0098 stub NotifyChangeEventLog
0099 stub ObjectCloseAuditAlarmA
0100 stub ObjectCloseAuditAlarmW
0101 stub ObjectOpenAuditAlarmA
0102 stub ObjectOpenAuditAlarmW
0103 stub ObjectPrivilegeAuditAlarmA
0104 stub ObjectPrivilegeAuditAlarmW
0105 stub OpenBackupEventLogA
0106 stub OpenBackupEventLogW
0107 stub OpenEventLogA
0108 stub OpenEventLogW
0109 stub OpenProcessToken
0110 stub OpenSCManagerA
0111 stub OpenSCManagerW
0112 stub OpenServiceA
0113 stub OpenServiceW
0114 stub OpenThreadToken
0115 stub PrivilegeCheck
0116 stub PrivilegedServiceAuditAlarmA
0117 stub PrivilegedServiceAuditAlarmW
0118 stub QueryServiceConfigA
0119 stub QueryServiceConfigW
0120 stub QueryServiceLockStatusA
0121 stub QueryServiceLockStatusW
0122 stub QueryServiceObjectSecurity
0123 stub QueryServiceStatus
0124 stub ReadEventLogA
0125 stub ReadEventLogW
0126 stdcall RegCloseKey(long) RegCloseKey
0127 stub RegConnectRegistryA
0128 stub RegConnectRegistryW
0129 stdcall RegCreateKeyA(long ptr ptr) RegCreateKey32A
0130 stdcall RegCreateKeyExA(long ptr long ptr long long ptr ptr ptr) RegCreateKeyEx32A
0131 stdcall RegCreateKeyExW(long ptr long ptr long long ptr ptr ptr) RegCreateKeyEx32W
0132 stdcall RegCreateKeyW(long ptr ptr) RegCreateKey32W
0133 stdcall RegDeleteKeyA(long ptr) RegDeleteKey32A
0134 stdcall RegDeleteKeyW(long ptr) RegDeleteKey32W
0135 stdcall RegDeleteValueA(long ptr) RegDeleteValue32A
0136 stdcall RegDeleteValueW(long ptr) RegDeleteValue32W
0137 stdcall RegEnumKeyA(long long ptr long) RegEnumKey32A
0138 stdcall RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr) RegEnumKeyEx32A
0139 stdcall RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr) RegEnumKeyEx32W
0140 stdcall RegEnumKeyW(long long ptr long) RegEnumKey32W
0141 stdcall RegEnumValueA(long long ptr ptr ptr ptr ptr ptr) RegEnumValue32A
0142 stdcall RegEnumValueW(long long ptr ptr ptr ptr ptr ptr) RegEnumValue32W
0143 stdcall RegFlushKey(long) RegFlushKey
0144 stub RegGetKeySecurity
0145 stub RegLoadKeyA
0146 stub RegLoadKeyW
0147 stub RegNotifyChangeKeyValue
0148 stdcall RegOpenKeyA(long ptr ptr) RegOpenKey32A
0149 stdcall RegOpenKeyExA(long ptr long long ptr) RegOpenKeyEx32A
0150 stdcall RegOpenKeyExW(long ptr long long ptr) RegOpenKeyEx32W
0151 stdcall RegOpenKeyW(long ptr ptr) RegOpenKey32W
0152 stdcall RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) RegQueryInfoKey32A
0153 stdcall RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) RegQueryInfoKey32W
0154 stub RegQueryMultipleValuesA
0155 stub RegQueryMultipleValuesW
0156 stdcall RegQueryValueA(long ptr ptr ptr) RegQueryValue32A
0157 stdcall RegQueryValueExA(long ptr ptr ptr ptr ptr) RegQueryValueEx32A
0158 stdcall RegQueryValueExW(long ptr ptr ptr ptr ptr) RegQueryValueEx32W
0159 stdcall RegQueryValueW(long ptr ptr ptr) RegQueryValue32W
0160 stub RegRemapPreDefKey
0161 stub RegReplaceKeyA
0162 stub RegReplaceKeyW
0163 stub RegRestoreKeyA
0164 stub RegRestoreKeyW
0165 stub RegSaveKeyA
0166 stub RegSaveKeyW
0167 stub RegSetKeySecurity
0168 stdcall RegSetValueA(long ptr long ptr long) RegSetValue32A
0169 stdcall RegSetValueExA(long ptr long long ptr long) RegSetValueEx32A
0170 stdcall RegSetValueExW(long ptr long long ptr long) RegSetValueEx32W
0171 stdcall RegSetValueW(long ptr long ptr long) RegSetValue32W
0172 stub RegUnLoadKeyA
0173 stub RegUnLoadKeyW
0174 stub RegisterEventSourceA
0175 stub RegisterEventSourceW
0176 stub RegisterServiceCtrlHandlerA
0177 stub RegisterServiceCtrlHandlerW
0178 stub ReportEventA
0179 stub ReportEventW
0180 stub RevertToSelf
0181 stub SetAclInformation
0182 stub SetFileSecurityA
0183 stub SetFileSecurityW
0184 stub SetKernelObjectSecurity
0185 stub SetPrivateObjectSecurity
0186 stub SetSecurityDescriptorDacl
0187 stub SetSecurityDescriptorGroup
0188 stub SetSecurityDescriptorOwner
0189 stub SetSecurityDescriptorSacl
0190 stub SetServiceBits
0191 stub SetServiceObjectSecurity
0192 stub SetServiceStatus
0193 stub SetThreadToken
0194 stub SetTokenInformation
0195 stub StartServiceA
0196 stdcall StartServiceCtrlDispatcherA(ptr) StartServiceCtrlDispatcher32A
0197 stdcall StartServiceCtrlDispatcherW(ptr) StartServiceCtrlDispatcher32W
0198 stub StartServiceW
0199 stub UnlockServiceDatabase
