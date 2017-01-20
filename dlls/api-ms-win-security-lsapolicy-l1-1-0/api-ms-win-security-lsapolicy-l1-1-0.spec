@ stdcall LsaAddAccountRights(ptr ptr ptr long) advapi32.LsaAddAccountRights
@ stdcall LsaClose(ptr) advapi32.LsaClose
@ stub LsaCreateSecret
@ stdcall LsaEnumerateAccountRights(ptr ptr ptr ptr) advapi32.LsaEnumerateAccountRights
@ stdcall LsaEnumerateAccountsWithUserRight(ptr ptr ptr ptr) advapi32.LsaEnumerateAccountsWithUserRight
@ stdcall LsaFreeMemory(ptr) advapi32.LsaFreeMemory
@ stub LsaICLookupNames
@ stub LsaICLookupNamesWithCreds
@ stub LsaICLookupSids
@ stub LsaICLookupSidsWithCreds
@ stdcall LsaLookupNames2(ptr long long ptr ptr ptr) advapi32.LsaLookupNames2
@ stdcall LsaLookupSids(ptr long ptr ptr ptr) advapi32.LsaLookupSids
@ stub LsaLookupSids2
@ stdcall LsaOpenPolicy(long ptr long long) advapi32.LsaOpenPolicy
@ stub LsaOpenSecret
@ stdcall LsaQueryInformationPolicy(ptr long ptr) advapi32.LsaQueryInformationPolicy
@ stub LsaQuerySecret
@ stdcall LsaRemoveAccountRights(ptr ptr long ptr long) advapi32.LsaRemoveAccountRights
@ stdcall LsaRetrievePrivateData(ptr ptr ptr) advapi32.LsaRetrievePrivateData
@ stdcall LsaSetInformationPolicy(long long ptr) advapi32.LsaSetInformationPolicy
@ stdcall LsaSetSecret(ptr ptr ptr) advapi32.LsaSetSecret
@ stdcall LsaStorePrivateData(ptr ptr ptr) advapi32.LsaStorePrivateData
