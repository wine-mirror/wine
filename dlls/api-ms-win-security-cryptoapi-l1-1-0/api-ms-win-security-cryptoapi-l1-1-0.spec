@ stdcall CryptAcquireContextA(ptr str str long long) advapi32.CryptAcquireContextA
@ stdcall CryptAcquireContextW(ptr wstr wstr long long) advapi32.CryptAcquireContextW
@ stdcall CryptContextAddRef(long ptr long) advapi32.CryptContextAddRef
@ stdcall CryptCreateHash(long long long long ptr) advapi32.CryptCreateHash
@ stdcall CryptDecrypt(long long long long ptr ptr) advapi32.CryptDecrypt
@ stdcall CryptDeriveKey(long long long long ptr) advapi32.CryptDeriveKey
@ stdcall CryptDestroyHash(long) advapi32.CryptDestroyHash
@ stdcall CryptDestroyKey(long) advapi32.CryptDestroyKey
@ stdcall CryptDuplicateHash(long ptr long ptr) advapi32.CryptDuplicateHash
@ stdcall CryptDuplicateKey(long ptr long ptr) advapi32.CryptDuplicateKey
@ stdcall CryptEncrypt(long long long long ptr ptr long) advapi32.CryptEncrypt
@ stdcall CryptEnumProviderTypesA(long ptr long ptr ptr ptr) advapi32.CryptEnumProviderTypesA
@ stdcall CryptEnumProviderTypesW(long ptr long ptr ptr ptr) advapi32.CryptEnumProviderTypesW
@ stdcall CryptEnumProvidersA(long ptr long ptr ptr ptr) advapi32.CryptEnumProvidersA
@ stdcall CryptEnumProvidersW(long ptr long ptr ptr ptr) advapi32.CryptEnumProvidersW
@ stdcall CryptExportKey(long long long long ptr ptr) advapi32.CryptExportKey
@ stdcall CryptGenKey(long long long ptr) advapi32.CryptGenKey
@ stdcall CryptGenRandom(long long ptr) advapi32.CryptGenRandom
@ stdcall CryptGetDefaultProviderA(long ptr long ptr ptr) advapi32.CryptGetDefaultProviderA
@ stdcall CryptGetDefaultProviderW(long ptr long ptr ptr) advapi32.CryptGetDefaultProviderW
@ stdcall CryptGetHashParam(long long ptr ptr long) advapi32.CryptGetHashParam
@ stdcall CryptGetKeyParam(long long ptr ptr long) advapi32.CryptGetKeyParam
@ stdcall CryptGetProvParam(long long ptr ptr long) advapi32.CryptGetProvParam
@ stdcall CryptGetUserKey(long long ptr) advapi32.CryptGetUserKey
@ stdcall CryptHashData(long ptr long long) advapi32.CryptHashData
@ stdcall CryptHashSessionKey(long long long) advapi32.CryptHashSessionKey
@ stdcall CryptImportKey(long ptr long long long ptr) advapi32.CryptImportKey
@ stdcall CryptReleaseContext(long long) advapi32.CryptReleaseContext
@ stdcall CryptSetHashParam(long long ptr long) advapi32.CryptSetHashParam
@ stdcall CryptSetKeyParam(long long ptr long) advapi32.CryptSetKeyParam
@ stdcall CryptSetProvParam(long long ptr long) advapi32.CryptSetProvParam
@ stdcall CryptSetProviderA(str long) advapi32.CryptSetProviderA
@ stdcall CryptSetProviderExA(str long ptr long) advapi32.CryptSetProviderExA
@ stdcall CryptSetProviderExW(wstr long ptr long) advapi32.CryptSetProviderExW
@ stdcall CryptSetProviderW(wstr long) advapi32.CryptSetProviderW
@ stdcall CryptSignHashA(long long str long ptr ptr) advapi32.CryptSignHashA
@ stdcall CryptSignHashW(long long wstr long ptr ptr) advapi32.CryptSignHashW
@ stdcall CryptVerifySignatureA(long ptr long long str long) advapi32.CryptVerifySignatureA
@ stdcall CryptVerifySignatureW(long ptr long long wstr long) advapi32.CryptVerifySignatureW
