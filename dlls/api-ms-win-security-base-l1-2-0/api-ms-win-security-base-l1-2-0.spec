@ stdcall AccessCheck(ptr long long ptr ptr ptr ptr ptr) advapi32.AccessCheck
@ stdcall AccessCheckAndAuditAlarmW(wstr ptr wstr wstr ptr long ptr long ptr ptr ptr) advapi32.AccessCheckAndAuditAlarmW
@ stdcall AccessCheckByType(ptr ptr long long ptr long ptr ptr ptr ptr ptr) advapi32.AccessCheckByType
@ stub AccessCheckByTypeAndAuditAlarmW
@ stub AccessCheckByTypeResultList
@ stub AccessCheckByTypeResultListAndAuditAlarmByHandleW
@ stub AccessCheckByTypeResultListAndAuditAlarmW
@ stdcall AddAccessAllowedAce(ptr long long ptr) advapi32.AddAccessAllowedAce
@ stdcall AddAccessAllowedAceEx(ptr long long long ptr) advapi32.AddAccessAllowedAceEx
@ stdcall AddAccessAllowedObjectAce(ptr long long long ptr ptr ptr) advapi32.AddAccessAllowedObjectAce
@ stdcall AddAccessDeniedAce(ptr long long ptr) advapi32.AddAccessDeniedAce
@ stdcall AddAccessDeniedAceEx(ptr long long long ptr) advapi32.AddAccessDeniedAceEx
@ stdcall AddAccessDeniedObjectAce(ptr long long long ptr ptr ptr) advapi32.AddAccessDeniedObjectAce
@ stdcall AddAce(ptr long long ptr long) advapi32.AddAce
@ stdcall AddAuditAccessAce(ptr long long ptr long long) advapi32.AddAuditAccessAce
@ stdcall AddAuditAccessAceEx(ptr long long long ptr long long) advapi32.AddAuditAccessAceEx
@ stdcall AddAuditAccessObjectAce(ptr long long long ptr ptr ptr long long) advapi32.AddAuditAccessObjectAce
@ stdcall AddMandatoryAce(ptr long long long ptr) advapi32.AddMandatoryAce
@ stub AddResourceAttributeAce
@ stub AddScopedPolicyIDAce
@ stdcall AdjustTokenGroups(long long ptr long ptr ptr) advapi32.AdjustTokenGroups
@ stdcall AdjustTokenPrivileges(long long ptr long ptr ptr) advapi32.AdjustTokenPrivileges
@ stdcall AllocateAndInitializeSid(ptr long long long long long long long long long ptr) advapi32.AllocateAndInitializeSid
@ stdcall AllocateLocallyUniqueId(ptr) advapi32.AllocateLocallyUniqueId
@ stdcall AreAllAccessesGranted(long long) advapi32.AreAllAccessesGranted
@ stdcall AreAnyAccessesGranted(long long) advapi32.AreAnyAccessesGranted
@ stub CheckTokenCapability
@ stdcall CheckTokenMembership(long ptr ptr) advapi32.CheckTokenMembership
@ stub CheckTokenMembershipEx
@ stdcall ConvertToAutoInheritPrivateObjectSecurity(ptr ptr ptr ptr long ptr) advapi32.ConvertToAutoInheritPrivateObjectSecurity
@ stdcall CopySid(long ptr ptr) advapi32.CopySid
@ stdcall CreatePrivateObjectSecurity(ptr ptr ptr long long ptr) advapi32.CreatePrivateObjectSecurity
@ stdcall CreatePrivateObjectSecurityEx(ptr ptr ptr ptr long long long ptr) advapi32.CreatePrivateObjectSecurityEx
@ stdcall CreatePrivateObjectSecurityWithMultipleInheritance(ptr ptr ptr ptr long long long long ptr) advapi32.CreatePrivateObjectSecurityWithMultipleInheritance
@ stdcall CreateRestrictedToken(long long long ptr long ptr long ptr ptr) advapi32.CreateRestrictedToken
@ stdcall CreateWellKnownSid(long ptr ptr ptr) advapi32.CreateWellKnownSid
@ stdcall DeleteAce(ptr long) advapi32.DeleteAce
@ stdcall DestroyPrivateObjectSecurity(ptr) advapi32.DestroyPrivateObjectSecurity
@ stdcall DuplicateToken(long long ptr) advapi32.DuplicateToken
@ stdcall DuplicateTokenEx(long long ptr long long ptr) advapi32.DuplicateTokenEx
@ stdcall EqualDomainSid(ptr ptr ptr) advapi32.EqualDomainSid
@ stdcall EqualPrefixSid(ptr ptr) advapi32.EqualPrefixSid
@ stdcall EqualSid(ptr ptr) advapi32.EqualSid
@ stdcall FindFirstFreeAce(ptr ptr) advapi32.FindFirstFreeAce
@ stdcall FreeSid(ptr) advapi32.FreeSid
@ stdcall GetAce(ptr long ptr) advapi32.GetAce
@ stdcall GetAclInformation(ptr ptr long long) advapi32.GetAclInformation
@ stub GetAppContainerAce
@ stub GetCachedSigningLevel
@ stdcall GetFileSecurityW(wstr long ptr long ptr) advapi32.GetFileSecurityW
@ stdcall GetKernelObjectSecurity(long long ptr long ptr) advapi32.GetKernelObjectSecurity
@ stdcall GetLengthSid(ptr) advapi32.GetLengthSid
@ stdcall GetPrivateObjectSecurity(ptr long ptr long ptr) advapi32.GetPrivateObjectSecurity
@ stdcall GetSecurityDescriptorControl(ptr ptr ptr) advapi32.GetSecurityDescriptorControl
@ stdcall GetSecurityDescriptorDacl(ptr ptr ptr ptr) advapi32.GetSecurityDescriptorDacl
@ stdcall GetSecurityDescriptorGroup(ptr ptr ptr) advapi32.GetSecurityDescriptorGroup
@ stdcall GetSecurityDescriptorLength(ptr) advapi32.GetSecurityDescriptorLength
@ stdcall GetSecurityDescriptorOwner(ptr ptr ptr) advapi32.GetSecurityDescriptorOwner
@ stub GetSecurityDescriptorRMControl
@ stdcall GetSecurityDescriptorSacl(ptr ptr ptr ptr) advapi32.GetSecurityDescriptorSacl
@ stdcall GetSidIdentifierAuthority(ptr) advapi32.GetSidIdentifierAuthority
@ stdcall GetSidLengthRequired(long) advapi32.GetSidLengthRequired
@ stdcall GetSidSubAuthority(ptr long) advapi32.GetSidSubAuthority
@ stdcall GetSidSubAuthorityCount(ptr) advapi32.GetSidSubAuthorityCount
@ stdcall GetTokenInformation(long long ptr long ptr) advapi32.GetTokenInformation
@ stdcall GetWindowsAccountDomainSid(ptr ptr ptr) advapi32.GetWindowsAccountDomainSid
@ stdcall ImpersonateAnonymousToken(long) advapi32.ImpersonateAnonymousToken
@ stdcall ImpersonateLoggedOnUser(long) advapi32.ImpersonateLoggedOnUser
@ stdcall ImpersonateSelf(long) advapi32.ImpersonateSelf
@ stdcall InitializeAcl(ptr long long) advapi32.InitializeAcl
@ stdcall InitializeSecurityDescriptor(ptr long) advapi32.InitializeSecurityDescriptor
@ stdcall InitializeSid(ptr ptr long) advapi32.InitializeSid
@ stdcall IsTokenRestricted(long) advapi32.IsTokenRestricted
@ stdcall IsValidAcl(ptr) advapi32.IsValidAcl
@ stdcall IsValidSecurityDescriptor(ptr) advapi32.IsValidSecurityDescriptor
@ stdcall IsValidSid(ptr) advapi32.IsValidSid
@ stdcall IsWellKnownSid(ptr long) advapi32.IsWellKnownSid
@ stdcall MakeAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) advapi32.MakeAbsoluteSD
@ stdcall MakeSelfRelativeSD(ptr ptr ptr) advapi32.MakeSelfRelativeSD
@ stdcall MapGenericMask(ptr ptr) advapi32.MapGenericMask
@ stdcall ObjectCloseAuditAlarmW(wstr ptr long) advapi32.ObjectCloseAuditAlarmW
@ stdcall ObjectDeleteAuditAlarmW(wstr ptr long) advapi32.ObjectDeleteAuditAlarmW
@ stdcall ObjectOpenAuditAlarmW(wstr ptr wstr wstr ptr long long long ptr long long ptr) advapi32.ObjectOpenAuditAlarmW
@ stdcall ObjectPrivilegeAuditAlarmW(wstr ptr long long ptr long) advapi32.ObjectPrivilegeAuditAlarmW
@ stdcall PrivilegeCheck(ptr ptr ptr) advapi32.PrivilegeCheck
@ stdcall PrivilegedServiceAuditAlarmW(wstr wstr long ptr long) advapi32.PrivilegedServiceAuditAlarmW
@ stub QuerySecurityAccessMask
@ stdcall RevertToSelf() advapi32.RevertToSelf
@ stdcall SetAclInformation(ptr ptr long long) advapi32.SetAclInformation
@ stdcall SetCachedSigningLevel(ptr long long long) kernelbase.SetCachedSigningLevel
@ stdcall SetFileSecurityW(wstr long ptr) advapi32.SetFileSecurityW
@ stdcall SetKernelObjectSecurity(long long ptr) advapi32.SetKernelObjectSecurity
@ stdcall SetPrivateObjectSecurity(long ptr ptr ptr long) advapi32.SetPrivateObjectSecurity
@ stdcall SetPrivateObjectSecurityEx(long ptr ptr long ptr long) advapi32.SetPrivateObjectSecurityEx
@ stub SetSecurityAccessMask
@ stdcall SetSecurityDescriptorControl(ptr long long) advapi32.SetSecurityDescriptorControl
@ stdcall SetSecurityDescriptorDacl(ptr long ptr long) advapi32.SetSecurityDescriptorDacl
@ stdcall SetSecurityDescriptorGroup(ptr ptr long) advapi32.SetSecurityDescriptorGroup
@ stdcall SetSecurityDescriptorOwner(ptr ptr long) advapi32.SetSecurityDescriptorOwner
@ stub SetSecurityDescriptorRMControl
@ stdcall SetSecurityDescriptorSacl(ptr long ptr long) advapi32.SetSecurityDescriptorSacl
@ stdcall SetTokenInformation(long long ptr long) advapi32.SetTokenInformation
