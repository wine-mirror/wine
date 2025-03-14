@ stub CheckSignatureInFile
@ stdcall -import CryptAcquireContextA(ptr str str long long)
@ stdcall -import CryptAcquireContextW(ptr wstr wstr long long)
@ stdcall -import CryptContextAddRef(long ptr long)
@ stdcall -import CryptCreateHash(long long long long ptr)
@ stdcall -import CryptDecrypt(long long long long ptr ptr)
@ stdcall -import CryptDeriveKey(long long long long ptr)
@ stdcall -import CryptDestroyHash(long)
@ stdcall -import CryptDestroyKey(long)
@ stdcall -import CryptDuplicateHash(long ptr long ptr)
@ stdcall -import CryptDuplicateKey(long ptr long ptr)
@ stdcall -import CryptEncrypt(long long long long ptr ptr long)
@ stdcall -import CryptEnumProviderTypesA(long ptr long ptr ptr ptr)
@ stdcall -import CryptEnumProviderTypesW(long ptr long ptr ptr ptr)
@ stdcall -import CryptEnumProvidersA(long ptr long ptr ptr ptr)
@ stdcall -import CryptEnumProvidersW(long ptr long ptr ptr ptr)
@ stdcall -import CryptExportKey(long long long long ptr ptr)
@ stdcall -import CryptGenKey(long long long ptr)
@ stdcall -import CryptGenRandom(long long ptr)
@ stdcall -import CryptGetDefaultProviderA(long ptr long ptr ptr)
@ stdcall -import CryptGetDefaultProviderW(long ptr long ptr ptr)
@ stdcall -import CryptGetHashParam(long long ptr ptr long)
@ stdcall -import CryptGetKeyParam(long long ptr ptr long)
@ stdcall -import CryptGetProvParam(long long ptr ptr long)
@ stdcall -import CryptGetUserKey(long long ptr)
@ stdcall -import CryptHashData(long ptr long long)
@ stdcall -import CryptHashSessionKey(long long long)
@ stdcall -import CryptImportKey(long ptr long long long ptr)
@ stdcall -import CryptReleaseContext(long long)
@ stdcall -import CryptSetHashParam(long long ptr long)
@ stdcall -import CryptSetKeyParam(long long ptr long)
@ stdcall -import CryptSetProvParam(long long ptr long)
@ stdcall -import CryptSetProviderA(str long)
@ stdcall -import CryptSetProviderExA(str long ptr long)
@ stdcall -import CryptSetProviderExW(wstr long ptr long)
@ stdcall -import CryptSetProviderW(wstr long)
@ stdcall -import CryptSignHashA(long long str long ptr ptr)
@ stdcall -import CryptSignHashW(long long wstr long ptr ptr)
@ stdcall -import CryptVerifySignatureA(long ptr long long str long)
@ stdcall -import CryptVerifySignatureW(long ptr long long wstr long)
@ stdcall SystemFunction006(ptr ptr)
@ stdcall -import SystemFunction007(ptr ptr)
@ stdcall SystemFunction008(ptr ptr ptr)
@ stdcall SystemFunction009(ptr ptr ptr) SystemFunction008
@ stdcall -import SystemFunction010(ptr ptr ptr)
@ stdcall -import SystemFunction011(ptr ptr ptr)
@ stdcall SystemFunction012(ptr ptr ptr)
@ stdcall SystemFunction013(ptr ptr ptr)
@ stdcall SystemFunction014(ptr ptr ptr) SystemFunction012
@ stdcall SystemFunction015(ptr ptr ptr) SystemFunction013
@ stdcall SystemFunction016(ptr ptr ptr) SystemFunction012
@ stdcall SystemFunction018(ptr ptr ptr) SystemFunction012
@ stdcall SystemFunction020(ptr ptr ptr) SystemFunction012
@ stdcall SystemFunction021(ptr ptr ptr) SystemFunction013
@ stdcall SystemFunction022(ptr ptr ptr) SystemFunction012
@ stdcall SystemFunction023(ptr ptr ptr) SystemFunction013
@ stdcall SystemFunction024(ptr ptr ptr)
@ stdcall SystemFunction025(ptr ptr ptr)
@ stdcall SystemFunction026(ptr ptr ptr) SystemFunction024
@ stdcall SystemFunction027(ptr ptr ptr) SystemFunction025
@ stdcall -import SystemFunction030(ptr ptr)
@ stdcall -import SystemFunction031(ptr ptr)
@ stdcall SystemFunction032(ptr ptr)
@ stub SystemFunction033
@ stdcall -import SystemFunction035(str)
