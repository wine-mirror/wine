name	advapi32
type	win32

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
0010 stdcall AdjustTokenPrivileges(long long ptr long ptr ptr) AdjustTokenPrivileges
0011 stdcall AllocateAndInitializeSid(ptr long long long long long long long long long ptr) AllocateAndInitializeSid
0012 stdcall AllocateLocallyUniqueId(ptr) AllocateLocallyUniqueId
0013 stub AreAllAccessesGranted
0014 stub AreAnyAccessesGranted
0015 stdcall BackupEventLogA (long str) BackupEventLog32A
0016 stdcall BackupEventLogW (long wstr) BackupEventLog32W
0017 stub ChangeServiceConfigA
0018 stub ChangeServiceConfigW
0019 stdcall ClearEventLogA (long str) ClearEventLog32A
0020 stdcall ClearEventLogW (long wstr) ClearEventLog32W
0021 stdcall CloseEventLog (long) CloseEventLog32
0022 stdcall CloseServiceHandle(long) CloseServiceHandle
0023 stdcall ControlService(long long ptr) ControlService
0024 stdcall CopySid(long ptr ptr) CopySid
0025 stub CreatePrivateObjectSecurity
0026 stub CreateProcessAsUserA
0027 stub CreateProcessAsUserW
0028 stdcall CreateServiceA(long ptr ptr long long long long ptr ptr ptr ptr ptr ptr) CreateService32A
0029 stdcall CreateServiceW (long ptr ptr long long long long ptr ptr ptr ptr ptr ptr) CreateService32A
0030 stub DeleteAce
0031 stdcall DeleteService(long) DeleteService
0032 stdcall DeregisterEventSource(long) DeregisterEventSource32
0033 stub DestroyPrivateObjectSecurity
0034 stub DuplicateToken
0035 stub EnumDependentServicesA
0036 stub EnumDependentServicesW
0037 stdcall EnumServicesStatusA (long long long ptr long ptr ptr ptr) EnumServicesStatus32A
0038 stdcall EnumServicesStatusW (long long long ptr long ptr ptr ptr) EnumServicesStatus32A
0039 stdcall EqualPrefixSid(ptr ptr) EqualPrefixSid
0040 stdcall EqualSid(ptr ptr) EqualSid
0041 stub FindFirstFreeAce
0042 stdcall FreeSid(ptr) FreeSid
0043 stub GetAce
0044 stub GetAclInformation
0045 stdcall GetFileSecurityA(str long ptr long ptr) GetFileSecurity32A
0046 stdcall GetFileSecurityW(wstr long ptr long ptr) GetFileSecurity32W
0047 stub GetKernelObjectSecurity
0048 stdcall GetLengthSid(ptr) GetLengthSid
0049 stdcall GetNumberOfEventLogRecords (long ptr) GetNumberOfEventLogRecords32
0050 stdcall GetOldestEventLogRecord (long ptr) GetOldestEventLogRecord32
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
0062 stdcall GetSidIdentifierAuthority(ptr) GetSidIdentifierAuthority
0063 stdcall GetSidLengthRequired(long) GetSidLengthRequired
0064 stdcall GetSidSubAuthority(ptr long) GetSidSubAuthority
0065 stdcall GetSidSubAuthorityCount(ptr) GetSidSubAuthorityCount
0066 stdcall GetTokenInformation(long long ptr long ptr) GetTokenInformation
0067 stdcall GetUserNameA(ptr ptr) GetUserName32A
0068 stdcall GetUserNameW(ptr ptr) GetUserName32W
0069 stub ImpersonateLoggedOnUser
0070 stub ImpersonateNamedPipeClient
0071 stub ImpersonateSelf
0072 stub InitializeAcl
0073 stdcall InitializeSecurityDescriptor(ptr long) InitializeSecurityDescriptor
0074 stdcall InitializeSid(ptr ptr long) InitializeSid
0075 stub InitiateSystemShutdownA
0076 stub InitiateSystemShutdownW
0077 stdcall IsTextUnicode(ptr long ptr) RtlIsTextUnicode
0078 stub IsValidAcl
0079 stdcall IsValidSecurityDescriptor(ptr) IsValidSecurityDescriptor
0080 stdcall IsValidSid(ptr) IsValidSid
0081 stub LockServiceDatabase
0082 stub LogonUserA
0083 stub LogonUserW
0084 stub LookupAccountNameA
0085 stub LookupAccountNameW
0086 stdcall LookupAccountSidA(ptr ptr ptr ptr ptr ptr ptr) LookupAccountSid32A
0087 stdcall LookupAccountSidW(ptr ptr ptr ptr ptr ptr ptr) LookupAccountSid32W
0088 stub LookupPrivilegeDisplayNameA
0089 stub LookupPrivilegeDisplayNameW
0090 stub LookupPrivilegeNameA
0091 stub LookupPrivilegeNameW
0092 stdcall LookupPrivilegeValueA(ptr ptr ptr) LookupPrivilegeValue32A
0093 stdcall LookupPrivilegeValueW(ptr ptr ptr) LookupPrivilegeValue32W
0094 stub MakeAbsoluteSD
0095 stdcall MakeSelfRelativeSD(ptr ptr ptr) MakeSelfRelativeSD
0096 stub MapGenericMask
0097 stdcall NotifyBootConfigStatus(long) NotifyBootConfigStatus
0098 stdcall NotifyChangeEventLog (long long) NotifyChangeEventLog32
0099 stub ObjectCloseAuditAlarmA
0100 stub ObjectCloseAuditAlarmW
0101 stub ObjectOpenAuditAlarmA
0102 stub ObjectOpenAuditAlarmW
0103 stub ObjectPrivilegeAuditAlarmA
0104 stub ObjectPrivilegeAuditAlarmW
0105 stdcall OpenBackupEventLogA (str str) OpenBackupEventLog32A
0106 stdcall OpenBackupEventLogW (wstr wstr) OpenBackupEventLog32W
0107 stdcall OpenEventLogA (str str) OpenEventLog32A
0108 stdcall OpenEventLogW (wstr wstr) OpenEventLog32W
0109 stdcall OpenProcessToken(long long ptr) OpenProcessToken
0110 stdcall OpenSCManagerA(ptr ptr long) OpenSCManager32A
0111 stdcall OpenSCManagerW(ptr ptr long) OpenSCManager32W
0112 stdcall OpenServiceA(long str long) OpenService32A
0113 stdcall OpenServiceW(long wstr long) OpenService32W
0114 stdcall OpenThreadToken(long long long ptr) OpenThreadToken
0115 stub PrivilegeCheck
0116 stub PrivilegedServiceAuditAlarmA
0117 stub PrivilegedServiceAuditAlarmW
0118 stub QueryServiceConfigA
0119 stub QueryServiceConfigW
0120 stub QueryServiceLockStatusA
0121 stub QueryServiceLockStatusW
0122 stub QueryServiceObjectSecurity
0123 stdcall QueryServiceStatus(long ptr) QueryServiceStatus
0124 stdcall ReadEventLogA (long long long ptr long ptr ptr) ReadEventLog32A
0125 stdcall ReadEventLogW (long long long ptr long ptr ptr) ReadEventLog32W
0126 stdcall RegCloseKey(long) RegCloseKey
0127 stdcall RegConnectRegistryA(str long ptr) RegConnectRegistry32A
0128 stdcall RegConnectRegistryW(wstr long ptr) RegConnectRegistry32W
0129 stdcall RegCreateKeyA(long str ptr) RegCreateKey32A
0130 stdcall RegCreateKeyExA(long str long ptr long long ptr ptr ptr) RegCreateKeyEx32A
0131 stdcall RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr) RegCreateKeyEx32W
0132 stdcall RegCreateKeyW(long wstr ptr) RegCreateKey32W
0133 stdcall RegDeleteKeyA(long str) RegDeleteKey32A
0134 stdcall RegDeleteKeyW(long wstr) RegDeleteKey32W
0135 stdcall RegDeleteValueA(long str) RegDeleteValue32A
0136 stdcall RegDeleteValueW(long wstr) RegDeleteValue32W
0137 stdcall RegEnumKeyA(long long ptr long) RegEnumKey32A
0138 stdcall RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr) RegEnumKeyEx32A
0139 stdcall RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr) RegEnumKeyEx32W
0140 stdcall RegEnumKeyW(long long ptr long) RegEnumKey32W
0141 stdcall RegEnumValueA(long long ptr ptr ptr ptr ptr ptr) RegEnumValue32A
0142 stdcall RegEnumValueW(long long ptr ptr ptr ptr ptr ptr) RegEnumValue32W
0143 stdcall RegFlushKey(long) RegFlushKey
0144 stdcall RegGetKeySecurity(long long ptr ptr) RegGetKeySecurity
0145 stdcall RegLoadKeyA(long str str) RegLoadKey32A
0146 stdcall RegLoadKeyW(long wstr wstr) RegLoadKey32W
0147 stdcall RegNotifyChangeKeyValue(long long long long long) RegNotifyChangeKeyValue
0148 stdcall RegOpenKeyA(long str ptr) RegOpenKey32A
0149 stdcall RegOpenKeyExA(long str long long ptr) RegOpenKeyEx32A
0150 stdcall RegOpenKeyExW(long wstr long long ptr) RegOpenKeyEx32W
0151 stdcall RegOpenKeyW(long wstr ptr) RegOpenKey32W
0152 stdcall RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) RegQueryInfoKey32A
0153 stdcall RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) RegQueryInfoKey32W
0154 stub RegQueryMultipleValuesA
0155 stub RegQueryMultipleValuesW
0156 stdcall RegQueryValueA(long str ptr ptr) RegQueryValue32A
0157 stdcall RegQueryValueExA(long str ptr ptr ptr ptr) RegQueryValueEx32A
0158 stdcall RegQueryValueExW(long wstr ptr ptr ptr ptr) RegQueryValueEx32W
0159 stdcall RegQueryValueW(long wstr ptr ptr) RegQueryValue32W
0160 stub RegRemapPreDefKey
0161 stdcall RegReplaceKeyA(long str str str) RegReplaceKey32A
0162 stdcall RegReplaceKeyW(long wstr wstr wstr) RegReplaceKey32W
0163 stdcall RegRestoreKeyA(long str long) RegRestoreKey32A
0164 stdcall RegRestoreKeyW(long wstr long) RegRestoreKey32W
0165 stdcall RegSaveKeyA(long ptr ptr) RegSaveKey32A
0166 stdcall RegSaveKeyW(long ptr ptr) RegSaveKey32W
0167 stdcall RegSetKeySecurity(long long ptr) RegSetKeySecurity
0168 stdcall RegSetValueA(long str long ptr long) RegSetValue32A
0169 stdcall RegSetValueExA(long str long long ptr long) RegSetValueEx32A
0170 stdcall RegSetValueExW(long wstr long long ptr long) RegSetValueEx32W
0171 stdcall RegSetValueW(long wstr long ptr long) RegSetValue32W
0172 stdcall RegUnLoadKeyA(long str) RegUnLoadKey32A
0173 stdcall RegUnLoadKeyW(long wstr) RegUnLoadKey32W
0174 stdcall RegisterEventSourceA(ptr ptr) RegisterEventSource32A
0175 stdcall RegisterEventSourceW(ptr ptr) RegisterEventSource32W
0176 stdcall RegisterServiceCtrlHandlerA (ptr ptr) RegisterServiceCtrlHandlerA
0177 stdcall RegisterServiceCtrlHandlerW (ptr ptr) RegisterServiceCtrlHandlerW
0178 stdcall ReportEventA (long long long long ptr long long str ptr) ReportEvent32A
0179 stdcall ReportEventW (long long long long ptr long long wstr ptr) ReportEvent32W
0180 stub RevertToSelf
0181 stub SetAclInformation
0182 stdcall SetFileSecurityA(str long ptr ) SetFileSecurity32A
0183 stdcall SetFileSecurityW(wstr long ptr) SetFileSecurity32W
0184 stub SetKernelObjectSecurity
0185 stub SetPrivateObjectSecurity
0186 stdcall SetSecurityDescriptorDacl(ptr long ptr long) RtlSetDaclSecurityDescriptor
0187 stub SetSecurityDescriptorGroup
0188 stub SetSecurityDescriptorOwner
0189 stub SetSecurityDescriptorSacl
0190 stub SetServiceBits
0191 stub SetServiceObjectSecurity
0192 stdcall SetServiceStatus(long long)SetServiceStatus
0193 stub SetThreadToken
0194 stub SetTokenInformation
0195 stdcall StartServiceA(long long ptr) StartService32A
0196 stdcall StartServiceCtrlDispatcherA(ptr) StartServiceCtrlDispatcher32A
0197 stdcall StartServiceCtrlDispatcherW(ptr) StartServiceCtrlDispatcher32W
0198 stdcall StartServiceW(long long ptr) StartService32W
0199 stub UnlockServiceDatabase
0200 stdcall LsaOpenPolicy(long long long long) LsaOpenPolicy
0201 stub LsaLookupSids
0202 stub LsaFreeMemory
0203 stub LsaQueryInformationPolicy
0204 stub LsaClose
0205 stub LsaSetInformationPolicy
0206 stub LsaLookupNames
0207 stub SystemFunction001
0208 stub SystemFunction002
0209 stub SystemFunction003
0210 stub SystemFunction004
0211 stub SystemFunction005
0212 stub SystemFunction006
0213 stub SystemFunction007
0214 stub SystemFunction008
0215 stub SystemFunction009
0216 stub SystemFunction010
0217 stub SystemFunction011
0218 stub SystemFunction012
0219 stub SystemFunction013
0220 stub SystemFunction014
0221 stub SystemFunction015
0222 stub SystemFunction016
0223 stub SystemFunction017
0224 stub SystemFunction018
0225 stub SystemFunction019
0226 stub SystemFunction020
0227 stub SystemFunction021
0228 stub SystemFunction022
0229 stub SystemFunction023
0230 stub SystemFunction024
0231 stub SystemFunction025
0232 stub SystemFunction026
0233 stub SystemFunction027
0234 stub SystemFunction028
0235 stub SystemFunction029
0236 stub SystemFunction030
0237 stub LsaQueryInfoTrustedDomain
0238 stub LsaQuerySecret
0239 stub LsaCreateSecret
0240 stub LsaOpenSecret
0241 stub LsaCreateTrustedDomain
0242 stub LsaOpenTrustedDomain
0243 stub LsaSetSecret
0244 stub LsaQuerySecret
0245 stub LsaCreateAccount
0246 stub LsaAddPrivilegesToAccount
0247 stub LsaRemovePrivilegesFromAccount
0248 stub LsaDelete
0249 stub LsaSetSystemAccessAccount
0250 stub LsaEnumeratePrivilegesOfAccount
0251 stub LsaEnumerateAccounts
0252 stub LsaOpenTrustedDomain
0253 stub LsaGetSystemAccessAccount
0254 stub LsaSetInformationTrustedDomain
0255 stub LsaEnumerateTrustedDomains
0256 stub LsaOpenAccount
0257 stub LsaEnumeratePrivileges
0258 stub LsaLookupPrivilegeDisplayName
0259 stub LsaICLookupNames
0260 stub ElfRegisterEventSourceW
0261 stub ElfReportEventW
0262 stub ElfDeregisterEventSource
0263 stub ElfDeregisterEventSourceW
0264 stub I_ScSetServiceBit
0265 stdcall SynchronizeWindows31FilesAndWindowsNTRegistry(long long long long) SynchronizeWindows31FilesAndWindowsNTRegistry
0266 stdcall QueryWindows31FilesMigration(long) QueryWindows31FilesMigration
0267 stub LsaICLookupSids
0268 stub SystemFunction031
0269 stub I_ScSetServiceBitsA
