@ stdcall LsaAddAccountRights(ptr ptr ptr long) sechost.LsaAddAccountRights
@ stdcall LsaClose(ptr) sechost.LsaClose
@ stub LsaCreateSecret
@ stdcall LsaEnumerateAccountRights(ptr ptr ptr ptr) sechost.LsaEnumerateAccountRights
@ stdcall LsaEnumerateAccountsWithUserRight(ptr ptr ptr ptr) sechost.LsaEnumerateAccountsWithUserRight
@ stdcall LsaFreeMemory(ptr) sechost.LsaFreeMemory
@ stub LsaICLookupNames
@ stub LsaICLookupNamesWithCreds
@ stub LsaICLookupSids
@ stub LsaICLookupSidsWithCreds
@ stdcall LsaLookupNames2(ptr long long ptr ptr ptr) sechost.LsaLookupNames2
@ stdcall LsaLookupSids(ptr long ptr ptr ptr) sechost.LsaLookupSids
@ stub LsaLookupSids2
@ stdcall LsaOpenPolicy(long ptr long long) sechost.LsaOpenPolicy
@ stub LsaOpenSecret
@ stdcall LsaQueryInformationPolicy(ptr long ptr) sechost.LsaQueryInformationPolicy
@ stub LsaQuerySecret
@ stdcall LsaRemoveAccountRights(ptr ptr long ptr long) sechost.LsaRemoveAccountRights
@ stdcall LsaRetrievePrivateData(ptr ptr ptr) sechost.LsaRetrievePrivateData
@ stdcall LsaSetInformationPolicy(long long ptr) sechost.LsaSetInformationPolicy
@ stdcall LsaSetSecret(ptr ptr ptr) sechost.LsaSetSecret
@ stdcall LsaStorePrivateData(ptr ptr ptr) sechost.LsaStorePrivateData
