name	advapi32
type	win32

0000 stub AbortSystemShutdownA
0001 stub AbortSystemShutdownW
0002 stdcall AccessCheck(ptr long long ptr ptr ptr ptr ptr) AccessCheck
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
0015 stdcall BackupEventLogA (long str) BackupEventLogA
0016 stdcall BackupEventLogW (long wstr) BackupEventLogW
0017 stub ChangeServiceConfigA
0018 stub ChangeServiceConfigW
0019 stdcall ClearEventLogA (long str) ClearEventLogA
0020 stdcall ClearEventLogW (long wstr) ClearEventLogW
0021 stdcall CloseEventLog (long) CloseEventLog
0022 stdcall CloseServiceHandle(long) CloseServiceHandle
0023 stdcall ControlService(long long ptr) ControlService
0024 stdcall CopySid(long ptr ptr) CopySid
0025 stub CreatePrivateObjectSecurity
0026 stub CreateProcessAsUserA
0027 stub CreateProcessAsUserW
0028 stdcall CreateServiceA(long ptr ptr long long long long ptr ptr ptr ptr ptr ptr) CreateServiceA
0029 stdcall CreateServiceW (long ptr ptr long long long long ptr ptr ptr ptr ptr ptr) CreateServiceW
0030 stdcall CryptAcquireContextA(ptr str str long long) CryptAcquireContextA
0031 stub CryptAcquireContextW
0032 stub CryptContextAddRef
0033 stub CryptCreateHash
0034 stub CryptDecrypt
0035 stub CryptDeriveKey
0036 stub CryptDestroyHash
0037 stub CryptDestroyKey
0038 stub CryptDuplicateKey
0039 stub CryptDuplicateHash
0040 stub CryptEncrypt
0041 stub CryptEnumProvidersA
0042 stub CryptEnumProvidersW
0043 stub CryptEnumProviderTypesA
0044 stub CryptEnumProviderTypesW
0045 stub CryptExportKey
0046 stub CryptGenKey
0047 stub CryptGetKeyParam
0048 stub CryptGetHashParam
0049 stub CryptGetProvParam
0050 stub CryptGenRandom
0051 stub CryptGetDefaultProviderA
0052 stub CryptGetDefaultProviderW
0053 stub CryptGetUserKey
0054 stub CryptHashData
0055 stub CryptHashSessionKey
0056 stub CryptImportKey
0057 stub CryptReleaseContext
0058 stub CryptSetHashParam
0059 stdcall CryptSetKeyParam(long long ptr long) CryptSetKeyParam
0060 stub CryptSetProvParam
0061 stub CryptSignHashA
0062 stub CryptSignHashW
0063 stub CryptSetProviderA
0064 stub CryptSetProviderW
0065 stub CryptSetProviderExA
0066 stub CryptSetProviderExW
0067 stub CryptVerifySignatureA
0068 stub CryptVerifySignatureW
0069 stub DeleteAce
0070 stdcall DeleteService(long) DeleteService
0071 stdcall DeregisterEventSource(long) DeregisterEventSource
0072 stub DestroyPrivateObjectSecurity
0073 stub DuplicateToken
0074 stub EnumDependentServicesA
0075 stub EnumDependentServicesW
0076 stdcall EnumServicesStatusA (long long long ptr long ptr ptr ptr) EnumServicesStatusA
0077 stdcall EnumServicesStatusW (long long long ptr long ptr ptr ptr) EnumServicesStatusA
0078 stdcall EqualPrefixSid(ptr ptr) EqualPrefixSid
0079 stdcall EqualSid(ptr ptr) EqualSid
0080 stub FindFirstFreeAce
0081 stdcall FreeSid(ptr) FreeSid
0082 stub GetAce
0083 stub GetAclInformation
0084 stdcall GetFileSecurityA(str long ptr long ptr) GetFileSecurityA
0085 stdcall GetFileSecurityW(wstr long ptr long ptr) GetFileSecurityW
0086 stub GetKernelObjectSecurity
0087 stdcall GetLengthSid(ptr) GetLengthSid
0088 stdcall GetNumberOfEventLogRecords (long ptr) GetNumberOfEventLogRecords
0089 stdcall GetOldestEventLogRecord (long ptr) GetOldestEventLogRecord
0090 stub GetPrivateObjectSecurity
0091 stdcall GetSecurityDescriptorControl (ptr ptr ptr) GetSecurityDescriptorControl
0092 stdcall GetSecurityDescriptorDacl (ptr ptr ptr ptr) GetSecurityDescriptorDacl
0093 stdcall GetSecurityDescriptorGroup(ptr ptr ptr) GetSecurityDescriptorGroup
0094 stdcall GetSecurityDescriptorLength(ptr) GetSecurityDescriptorLength
0095 stdcall GetSecurityDescriptorOwner(ptr ptr ptr) GetSecurityDescriptorOwner
0096 stdcall GetSecurityDescriptorSacl (ptr ptr ptr ptr) GetSecurityDescriptorSacl
0097 stub GetServiceDisplayNameA
0098 stub GetServiceDisplayNameW
0099 stub GetServiceKeyNameA
0100 stub GetServiceKeyNameW
0101 stdcall GetSidIdentifierAuthority(ptr) GetSidIdentifierAuthority
0102 stdcall GetSidLengthRequired(long) GetSidLengthRequired
0103 stdcall GetSidSubAuthority(ptr long) GetSidSubAuthority
0104 stdcall GetSidSubAuthorityCount(ptr) GetSidSubAuthorityCount
0105 stdcall GetTokenInformation(long long ptr long ptr) GetTokenInformation
0106 stdcall GetUserNameA(ptr ptr) GetUserNameA
0107 stdcall GetUserNameW(ptr ptr) GetUserNameW
0108 stub ImpersonateLoggedOnUser
0109 stub ImpersonateNamedPipeClient
0110 stdcall ImpersonateSelf(long) ImpersonateSelf
0111 stub InitializeAcl
0112 stdcall InitializeSecurityDescriptor(ptr long) InitializeSecurityDescriptor
0113 stdcall InitializeSid(ptr ptr long) InitializeSid
0114 stub InitiateSystemShutdownA
0115 stub InitiateSystemShutdownW
0116 stdcall IsTextUnicode(ptr long ptr) RtlIsTextUnicode
0117 stub IsValidAcl
0118 stdcall IsValidSecurityDescriptor(ptr) IsValidSecurityDescriptor
0119 stdcall IsValidSid(ptr) IsValidSid
0120 stub LockServiceDatabase
0121 stub LogonUserA
0122 stub LogonUserW
0123 stub LookupAccountNameA
0124 stub LookupAccountNameW
0125 stdcall LookupAccountSidA(ptr ptr ptr ptr ptr ptr ptr) LookupAccountSidA
0126 stdcall LookupAccountSidW(ptr ptr ptr ptr ptr ptr ptr) LookupAccountSidW
0127 stub LookupPrivilegeDisplayNameA
0128 stub LookupPrivilegeDisplayNameW
0129 stub LookupPrivilegeNameA
0130 stub LookupPrivilegeNameW
0131 stdcall LookupPrivilegeValueA(ptr ptr ptr) LookupPrivilegeValueA
0132 stdcall LookupPrivilegeValueW(ptr ptr ptr) LookupPrivilegeValueW
0133 stub MakeAbsoluteSD
0134 stdcall MakeSelfRelativeSD(ptr ptr ptr) MakeSelfRelativeSD
0135 stub MapGenericMask
0136 stdcall NotifyBootConfigStatus(long) NotifyBootConfigStatus
0137 stdcall NotifyChangeEventLog (long long) NotifyChangeEventLog
0138 stub ObjectCloseAuditAlarmA
0139 stub ObjectCloseAuditAlarmW
0140 stub ObjectOpenAuditAlarmA
0141 stub ObjectOpenAuditAlarmW
0142 stub ObjectPrivilegeAuditAlarmA
0143 stub ObjectPrivilegeAuditAlarmW
0144 stdcall OpenBackupEventLogA (str str) OpenBackupEventLogA
0145 stdcall OpenBackupEventLogW (wstr wstr) OpenBackupEventLogW
0146 stdcall OpenEventLogA (str str) OpenEventLogA
0147 stdcall OpenEventLogW (wstr wstr) OpenEventLogW
0148 stdcall OpenProcessToken(long long ptr) OpenProcessToken
0149 stdcall OpenSCManagerA(ptr ptr long) OpenSCManagerA
0150 stdcall OpenSCManagerW(ptr ptr long) OpenSCManagerW
0151 stdcall OpenServiceA(long str long) OpenServiceA
0152 stdcall OpenServiceW(long wstr long) OpenServiceW
0153 stdcall OpenThreadToken(long long long ptr) OpenThreadToken
0154 stub PrivilegeCheck
0155 stub PrivilegedServiceAuditAlarmA
0156 stub PrivilegedServiceAuditAlarmW
0157 stub QueryServiceConfigA
0158 stub QueryServiceConfigW
0159 stub QueryServiceLockStatusA
0160 stub QueryServiceLockStatusW
0161 stub QueryServiceObjectSecurity
0162 stdcall QueryServiceStatus(long ptr) QueryServiceStatus
0163 stdcall ReadEventLogA (long long long ptr long ptr ptr) ReadEventLogA
0164 stdcall ReadEventLogW (long long long ptr long ptr ptr) ReadEventLogW
0165 stdcall RegCloseKey(long) RegCloseKey
0166 stdcall RegConnectRegistryA(str long ptr) RegConnectRegistryA
0167 stdcall RegConnectRegistryW(wstr long ptr) RegConnectRegistryW
0168 stdcall RegCreateKeyA(long str ptr) RegCreateKeyA
0169 stdcall RegCreateKeyExA(long str long ptr long long ptr ptr ptr) RegCreateKeyExA
0170 stdcall RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr) RegCreateKeyExW
0171 stdcall RegCreateKeyW(long wstr ptr) RegCreateKeyW
0172 stdcall RegDeleteKeyA(long str) RegDeleteKeyA
0173 stdcall RegDeleteKeyW(long wstr) RegDeleteKeyW
0174 stdcall RegDeleteValueA(long str) RegDeleteValueA
0175 stdcall RegDeleteValueW(long wstr) RegDeleteValueW
0176 stdcall RegEnumKeyA(long long ptr long) RegEnumKeyA
0177 stdcall RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr) RegEnumKeyExA
0178 stdcall RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr) RegEnumKeyExW
0179 stdcall RegEnumKeyW(long long ptr long) RegEnumKeyW
0180 stdcall RegEnumValueA(long long ptr ptr ptr ptr ptr ptr) RegEnumValueA
0181 stdcall RegEnumValueW(long long ptr ptr ptr ptr ptr ptr) RegEnumValueW
0182 stdcall RegFlushKey(long) RegFlushKey
0183 stdcall RegGetKeySecurity(long long ptr ptr) RegGetKeySecurity
0184 stdcall RegLoadKeyA(long str str) RegLoadKeyA
0185 stdcall RegLoadKeyW(long wstr wstr) RegLoadKeyW
0186 stdcall RegNotifyChangeKeyValue(long long long long long) RegNotifyChangeKeyValue
0187 stdcall RegOpenKeyA(long str ptr) RegOpenKeyA
0188 stdcall RegOpenKeyExA(long str long long ptr) RegOpenKeyExA
0189 stdcall RegOpenKeyExW(long wstr long long ptr) RegOpenKeyExW
0190 stdcall RegOpenKeyW(long wstr ptr) RegOpenKeyW
0191 stdcall RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) RegQueryInfoKeyA
0192 stdcall RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) RegQueryInfoKeyW
0193 stub RegQueryMultipleValuesA
0194 stub RegQueryMultipleValuesW
0195 stdcall RegQueryValueA(long str ptr ptr) RegQueryValueA
0196 stdcall RegQueryValueExA(long str ptr ptr ptr ptr) RegQueryValueExA
0197 stdcall RegQueryValueExW(long wstr ptr ptr ptr ptr) RegQueryValueExW
0198 stdcall RegQueryValueW(long wstr ptr ptr) RegQueryValueW
0199 stub RegRemapPreDefKey
0200 stdcall RegReplaceKeyA(long str str str) RegReplaceKeyA
0201 stdcall RegReplaceKeyW(long wstr wstr wstr) RegReplaceKeyW
0202 stdcall RegRestoreKeyA(long str long) RegRestoreKeyA
0203 stdcall RegRestoreKeyW(long wstr long) RegRestoreKeyW
0204 stdcall RegSaveKeyA(long ptr ptr) RegSaveKeyA
0205 stdcall RegSaveKeyW(long ptr ptr) RegSaveKeyW
0206 stdcall RegSetKeySecurity(long long ptr) RegSetKeySecurity
0207 stdcall RegSetValueA(long str long ptr long) RegSetValueA
0208 stdcall RegSetValueExA(long str long long ptr long) RegSetValueExA
0209 stdcall RegSetValueExW(long wstr long long ptr long) RegSetValueExW
0210 stdcall RegSetValueW(long wstr long ptr long) RegSetValueW
0211 stdcall RegUnLoadKeyA(long str) RegUnLoadKeyA
0212 stdcall RegUnLoadKeyW(long wstr) RegUnLoadKeyW
0213 stdcall RegisterEventSourceA(ptr ptr) RegisterEventSourceA
0214 stdcall RegisterEventSourceW(ptr ptr) RegisterEventSourceW
0215 stdcall RegisterServiceCtrlHandlerA (ptr ptr) RegisterServiceCtrlHandlerA
0216 stdcall RegisterServiceCtrlHandlerW (ptr ptr) RegisterServiceCtrlHandlerW
0217 stdcall ReportEventA (long long long long ptr long long str ptr) ReportEventA
0218 stdcall ReportEventW (long long long long ptr long long wstr ptr) ReportEventW
0219 stdcall RevertToSelf() RevertToSelf
0220 stub SetAclInformation
0221 stdcall SetFileSecurityA(str long ptr ) SetFileSecurityA
0222 stdcall SetFileSecurityW(wstr long ptr) SetFileSecurityW
0223 stub SetKernelObjectSecurity
0224 stub SetPrivateObjectSecurity
0225 stdcall SetSecurityDescriptorDacl(ptr long ptr long) SetSecurityDescriptorDacl
0226 stdcall SetSecurityDescriptorGroup (ptr ptr long) SetSecurityDescriptorGroup
0227 stdcall SetSecurityDescriptorOwner (ptr ptr long) SetSecurityDescriptorOwner
0228 stdcall SetSecurityDescriptorSacl(ptr long ptr long) SetSecurityDescriptorSacl
0229 stub SetServiceBits
0230 stub SetServiceObjectSecurity
0231 stdcall SetServiceStatus(long long)SetServiceStatus
0232 stdcall SetThreadToken (ptr ptr) SetThreadToken
0233 stub SetTokenInformation
0234 stdcall StartServiceA(long long ptr) StartServiceA
0235 stdcall StartServiceCtrlDispatcherA(ptr) StartServiceCtrlDispatcherA
0236 stdcall StartServiceCtrlDispatcherW(ptr) StartServiceCtrlDispatcherW
0237 stdcall StartServiceW(long long ptr) StartServiceW
0238 stub UnlockServiceDatabase
0239 stdcall LsaOpenPolicy(long long long long) LsaOpenPolicy
0240 stub LsaLookupSids
0241 stdcall LsaFreeMemory(ptr)LsaFreeMemory
0242 stdcall LsaQueryInformationPolicy(ptr long ptr)LsaQueryInformationPolicy
0243 stdcall LsaClose(ptr)LsaClose
0244 stub LsaSetInformationPolicy
0245 stub LsaLookupNames
0246 stub SystemFunction001
0247 stub SystemFunction002
0248 stub SystemFunction003
0249 stub SystemFunction004
0250 stub SystemFunction005
0251 stub SystemFunction006
0252 stub SystemFunction007
0253 stub SystemFunction008
0254 stub SystemFunction009
0255 stub SystemFunction010
0256 stub SystemFunction011
0257 stub SystemFunction012
0258 stub SystemFunction013
0259 stub SystemFunction014
0260 stub SystemFunction015
0261 stub SystemFunction016
0262 stub SystemFunction017
0263 stub SystemFunction018
0264 stub SystemFunction019
0265 stub SystemFunction020
0266 stub SystemFunction021
0267 stub SystemFunction022
0268 stub SystemFunction023
0269 stub SystemFunction024
0270 stub SystemFunction025
0271 stub SystemFunction026
0272 stub SystemFunction027
0273 stub SystemFunction028
0274 stub SystemFunction029
0275 stub SystemFunction030
0276 stub LsaQueryInfoTrustedDomain
0277 stub LsaQuerySecret
0278 stub LsaCreateSecret
0279 stub LsaOpenSecret
0280 stub LsaCreateTrustedDomain
0281 stub LsaOpenTrustedDomain
0282 stub LsaSetSecret
0283 stub LsaQuerySecret
0284 stub LsaCreateAccount
0285 stub LsaAddPrivilegesToAccount
0286 stub LsaRemovePrivilegesFromAccount
0287 stub LsaDelete
0288 stub LsaSetSystemAccessAccount
0289 stub LsaEnumeratePrivilegesOfAccount
0290 stub LsaEnumerateAccounts
0291 stub LsaOpenTrustedDomain
0292 stub LsaGetSystemAccessAccount
0293 stub LsaSetInformationTrustedDomain
0294 stub LsaEnumerateTrustedDomains
0295 stub LsaOpenAccount
0296 stub LsaEnumeratePrivileges
0297 stub LsaLookupPrivilegeDisplayName
0298 stub LsaICLookupNames
0299 stub ElfRegisterEventSourceW
0300 stub ElfReportEventW
0301 stub ElfDeregisterEventSource
0302 stub ElfDeregisterEventSourceW
0303 stub I_ScSetServiceBit
0304 stdcall SynchronizeWindows31FilesAndWindowsNTRegistry(long long long long) SynchronizeWindows31FilesAndWindowsNTRegistry
0305 stdcall QueryWindows31FilesMigration(long) QueryWindows31FilesMigration
0306 stub LsaICLookupSids
0307 stub SystemFunction031
0308 stub I_ScSetServiceBitsA
0309 stub EnumServiceGroupA
0310 stub EnumServiceGroupW
