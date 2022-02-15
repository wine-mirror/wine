@ stdcall CryptAcquireContextA(ptr str str long long) cryptsp.CryptAcquireContextA
@ stdcall CryptAcquireContextW(ptr wstr wstr long long) cryptsp.CryptAcquireContextW
@ stdcall CryptContextAddRef(long ptr long) cryptsp.CryptContextAddRef
@ stdcall CryptCreateHash(long long long long ptr) cryptsp.CryptCreateHash
@ stdcall CryptDecrypt(long long long long ptr ptr) cryptsp.CryptDecrypt
@ stdcall CryptDeriveKey(long long long long ptr) cryptsp.CryptDeriveKey
@ stdcall CryptDestroyHash(long) cryptsp.CryptDestroyHash
@ stdcall CryptDestroyKey(long) cryptsp.CryptDestroyKey
@ stdcall CryptDuplicateHash(long ptr long ptr) cryptsp.CryptDuplicateHash
@ stdcall CryptDuplicateKey(long ptr long ptr) cryptsp.CryptDuplicateKey
@ stdcall CryptEncrypt(long long long long ptr ptr long) cryptsp.CryptEncrypt
@ stdcall CryptEnumProviderTypesA(long ptr long ptr ptr ptr) cryptsp.CryptEnumProviderTypesA
@ stdcall CryptEnumProviderTypesW(long ptr long ptr ptr ptr) cryptsp.CryptEnumProviderTypesW
@ stdcall CryptEnumProvidersA(long ptr long ptr ptr ptr) cryptsp.CryptEnumProvidersA
@ stdcall CryptEnumProvidersW(long ptr long ptr ptr ptr) cryptsp.CryptEnumProvidersW
@ stdcall CryptExportKey(long long long long ptr ptr) cryptsp.CryptExportKey
@ stdcall CryptGenKey(long long long ptr) cryptsp.CryptGenKey
@ stdcall CryptGenRandom(long long ptr) cryptsp.CryptGenRandom
@ stdcall CryptGetDefaultProviderA(long ptr long ptr ptr) cryptsp.CryptGetDefaultProviderA
@ stdcall CryptGetDefaultProviderW(long ptr long ptr ptr) cryptsp.CryptGetDefaultProviderW
@ stdcall CryptGetHashParam(long long ptr ptr long) cryptsp.CryptGetHashParam
@ stdcall CryptGetKeyParam(long long ptr ptr long) cryptsp.CryptGetKeyParam
@ stdcall CryptGetProvParam(long long ptr ptr long) cryptsp.CryptGetProvParam
@ stdcall CryptGetUserKey(long long ptr) cryptsp.CryptGetUserKey
@ stdcall CryptHashData(long ptr long long) cryptsp.CryptHashData
@ stdcall CryptHashSessionKey(long long long) cryptsp.CryptHashSessionKey
@ stdcall CryptImportKey(long ptr long long long ptr) cryptsp.CryptImportKey
@ stdcall CryptReleaseContext(long long) cryptsp.CryptReleaseContext
@ stdcall CryptSetHashParam(long long ptr long) cryptsp.CryptSetHashParam
@ stdcall CryptSetKeyParam(long long ptr long) cryptsp.CryptSetKeyParam
@ stdcall CryptSetProvParam(long long ptr long) cryptsp.CryptSetProvParam
@ stdcall CryptSetProviderA(str long) cryptsp.CryptSetProviderA
@ stdcall CryptSetProviderExA(str long ptr long) cryptsp.CryptSetProviderExA
@ stdcall CryptSetProviderExW(wstr long ptr long) cryptsp.CryptSetProviderExW
@ stdcall CryptSetProviderW(wstr long) cryptsp.CryptSetProviderW
@ stdcall CryptSignHashA(long long str long ptr ptr) cryptsp.CryptSignHashA
@ stdcall CryptSignHashW(long long wstr long ptr ptr) cryptsp.CryptSignHashW
@ stdcall CryptVerifySignatureA(long ptr long long str long) cryptsp.CryptVerifySignatureA
@ stdcall CryptVerifySignatureW(long ptr long long wstr long) cryptsp.CryptVerifySignatureW
