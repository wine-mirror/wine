@ stdcall AccessCheck(ptr long long ptr ptr ptr ptr ptr) kernelbase.AccessCheck
@ stub AccessCheckandAuditAlarmW
@ stdcall AccessCheckByType(ptr ptr long long ptr long ptr ptr ptr ptr ptr) kernelbase.AccessCheckByType
@ stub AccessCheckByTypeandAuditAlarmW
@ stub AccessCheckByTypeResultList
@ stub AccessCheckByTypeResultListandAuditAlarmByHandleW
@ stub AccessCheckByTypeResultListandAuditAlarmW
@ stdcall AddAccessAllowedAce(ptr long long ptr) kernelbase.AddAccessAllowedAce
@ stdcall AddAccessAllowedAceEx(ptr long long long ptr) kernelbase.AddAccessAllowedAceEx
@ stdcall AddAccessAllowedObjectAce(ptr long long long ptr ptr ptr) kernelbase.AddAccessAllowedObjectAce
@ stdcall AddAccessDeniedAce(ptr long long ptr) kernelbase.AddAccessDeniedAce
@ stdcall AddAccessDeniedAceEx(ptr long long long ptr) kernelbase.AddAccessDeniedAceEx
@ stdcall AddAccessDeniedObjectAce(ptr long long long ptr ptr ptr) kernelbase.AddAccessDeniedObjectAce
@ stdcall AddAce(ptr long long ptr long) kernelbase.AddAce
@ stdcall AddAuditAccessAce(ptr long long ptr long long) kernelbase.AddAuditAccessAce
@ stdcall AddAuditAccessAceEx(ptr long long long ptr long long) kernelbase.AddAuditAccessAceEx
@ stdcall AddAuditAccessObjectAce(ptr long long long ptr ptr ptr long long) kernelbase.AddAuditAccessObjectAce
@ stdcall AddMandatoryAce(ptr long long long ptr) kernelbase.AddMandatoryAce
@ stdcall AdjustTokenGroups(long long ptr long ptr ptr) kernelbase.AdjustTokenGroups
@ stdcall AdjustTokenPrivileges(long long ptr long ptr ptr) kernelbase.AdjustTokenPrivileges
@ stdcall AllocateAndInitializeSid(ptr long long long long long long long long long ptr) kernelbase.AllocateAndInitializeSid
@ stdcall AllocateLocallyUniqueId(ptr) kernelbase.AllocateLocallyUniqueId
@ stdcall AreAllAccessesGranted(long long) kernelbase.AreAllAccessesGranted
@ stdcall AreAnyAccessesGranted(long long) kernelbase.AreAnyAccessesGranted
@ stdcall CheckTokenMembership(long ptr ptr) kernelbase.CheckTokenMembership
@ stdcall ConvertToAutoInheritPrivateObjectSecurity(ptr ptr ptr ptr long ptr) kernelbase.ConvertToAutoInheritPrivateObjectSecurity
@ stdcall CopySid(long ptr ptr) kernelbase.CopySid
@ stdcall CreatePrivateObjectSecurity(ptr ptr ptr long long ptr) kernelbase.CreatePrivateObjectSecurity
@ stdcall CreatePrivateObjectSecurityEx(ptr ptr ptr ptr long long long ptr) kernelbase.CreatePrivateObjectSecurityEx
@ stdcall CreatePrivateObjectSecurityWithMultipleInheritance(ptr ptr ptr ptr long long long long ptr) kernelbase.CreatePrivateObjectSecurityWithMultipleInheritance
@ stdcall CreateRestrictedToken(long long long ptr long ptr long ptr ptr) kernelbase.CreateRestrictedToken
@ stdcall CreateWellKnownSid(long ptr ptr ptr) kernelbase.CreateWellKnownSid
@ stdcall DeleteAce(ptr long) kernelbase.DeleteAce
@ stdcall DestroyPrivateObjectSecurity(ptr) kernelbase.DestroyPrivateObjectSecurity
@ stdcall DuplicateToken(long long ptr) kernelbase.DuplicateToken
@ stdcall DuplicateTokenEx(long long ptr long long ptr) kernelbase.DuplicateTokenEx
@ stdcall EqualDomainSid(ptr ptr ptr) kernelbase.EqualDomainSid
@ stdcall EqualPrefixSid(ptr ptr) kernelbase.EqualPrefixSid
@ stdcall EqualSid(ptr ptr) kernelbase.EqualSid
@ stdcall FindFirstFreeAce(ptr ptr) kernelbase.FindFirstFreeAce
@ stdcall FreeSid(ptr) kernelbase.FreeSid
@ stdcall GetAce(ptr long ptr) kernelbase.GetAce
@ stdcall GetAclInformation(ptr ptr long long) kernelbase.GetAclInformation
@ stdcall GetFileSecurityW(wstr long ptr long ptr) kernelbase.GetFileSecurityW
@ stdcall GetKernelObjectSecurity(long long ptr long ptr) kernelbase.GetKernelObjectSecurity
@ stdcall GetLengthSid(ptr) kernelbase.GetLengthSid
@ stdcall GetPrivateObjectSecurity(ptr long ptr long ptr) kernelbase.GetPrivateObjectSecurity
@ stdcall GetSecurityDescriptorControl(ptr ptr ptr) kernelbase.GetSecurityDescriptorControl
@ stdcall GetSecurityDescriptorDacl(ptr ptr ptr ptr) kernelbase.GetSecurityDescriptorDacl
@ stdcall GetSecurityDescriptorGroup(ptr ptr ptr) kernelbase.GetSecurityDescriptorGroup
@ stdcall GetSecurityDescriptorLength(ptr) kernelbase.GetSecurityDescriptorLength
@ stdcall GetSecurityDescriptorOwner(ptr ptr ptr) kernelbase.GetSecurityDescriptorOwner
@ stub GetSecurityDescriptorRMControl
@ stdcall GetSecurityDescriptorSacl(ptr ptr ptr ptr) kernelbase.GetSecurityDescriptorSacl
@ stdcall GetSidIdentifierAuthority(ptr) kernelbase.GetSidIdentifierAuthority
@ stdcall GetSidLengthRequired(long) kernelbase.GetSidLengthRequired
@ stdcall GetSidSubAuthority(ptr long) kernelbase.GetSidSubAuthority
@ stdcall GetSidSubAuthorityCount(ptr) kernelbase.GetSidSubAuthorityCount
@ stdcall GetTokenInformation(long long ptr long ptr) kernelbase.GetTokenInformation
@ stdcall GetWindowsAccountDomainSid(ptr ptr ptr) kernelbase.GetWindowsAccountDomainSid
@ stdcall ImpersonateAnonymousToken(long) kernelbase.ImpersonateAnonymousToken
@ stdcall ImpersonateLoggedOnUser(long) kernelbase.ImpersonateLoggedOnUser
@ stdcall ImpersonateSelf(long) kernelbase.ImpersonateSelf
@ stdcall InitializeAcl(ptr long long) kernelbase.InitializeAcl
@ stdcall InitializeSecurityDescriptor(ptr long) kernelbase.InitializeSecurityDescriptor
@ stdcall InitializeSid(ptr ptr long) kernelbase.InitializeSid
@ stdcall IsTokenRestricted(long) kernelbase.IsTokenRestricted
@ stdcall IsValidAcl(ptr) kernelbase.IsValidAcl
@ stub IsValidRelativeSecurityDescriptor
@ stdcall IsValidSecurityDescriptor(ptr) kernelbase.IsValidSecurityDescriptor
@ stdcall IsValidSid(ptr) kernelbase.IsValidSid
@ stdcall IsWellKnownSid(ptr long) kernelbase.IsWellKnownSid
@ stdcall MakeAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) kernelbase.MakeAbsoluteSD
@ stub MakeAbsoluteSD2
@ stdcall MakeSelfRelativeSD(ptr ptr ptr) kernelbase.MakeSelfRelativeSD
@ stdcall MapGenericMask(ptr ptr) kernelbase.MapGenericMask
@ stdcall ObjectCloseAuditAlarmW(wstr ptr long) kernelbase.ObjectCloseAuditAlarmW
@ stdcall ObjectDeleteAuditAlarmW(wstr ptr long) kernelbase.ObjectDeleteAuditAlarmW
@ stdcall ObjectOpenAuditAlarmW(wstr ptr wstr wstr ptr long long long ptr long long ptr) kernelbase.ObjectOpenAuditAlarmW
@ stdcall ObjectPrivilegeAuditAlarmW(wstr ptr long long ptr long) kernelbase.ObjectPrivilegeAuditAlarmW
@ stdcall PrivilegeCheck(ptr ptr ptr) kernelbase.PrivilegeCheck
@ stdcall PrivilegedServiceAuditAlarmW(wstr wstr long ptr long) kernelbase.PrivilegedServiceAuditAlarmW
@ stub QuerySecurityAccessMask
@ stdcall RevertToSelf() kernelbase.RevertToSelf
@ stdcall SetAclInformation(ptr ptr long long) kernelbase.SetAclInformation
@ stdcall SetFileSecurityW(wstr long ptr) kernelbase.SetFileSecurityW
@ stdcall SetKernelObjectSecurity(long long ptr) kernelbase.SetKernelObjectSecurity
@ stdcall SetPrivateObjectSecurity(long ptr ptr ptr long) kernelbase.SetPrivateObjectSecurity
@ stdcall SetPrivateObjectSecurityEx(long ptr ptr long ptr long) kernelbase.SetPrivateObjectSecurityEx
@ stub SetSecurityAccessMask
@ stdcall SetSecurityDescriptorControl(ptr long long) kernelbase.SetSecurityDescriptorControl
@ stdcall SetSecurityDescriptorDacl(ptr long ptr long) kernelbase.SetSecurityDescriptorDacl
@ stdcall SetSecurityDescriptorGroup(ptr ptr long) kernelbase.SetSecurityDescriptorGroup
@ stdcall SetSecurityDescriptorOwner(ptr ptr long) kernelbase.SetSecurityDescriptorOwner
@ stub SetSecurityDescriptorRMControl
@ stdcall SetSecurityDescriptorSacl(ptr long ptr long) kernelbase.SetSecurityDescriptorSacl
@ stdcall SetTokenInformation(long long ptr long) kernelbase.SetTokenInformation
