@ stdcall CPAcquireContext(ptr str long ptr) RSA_CPAcquireContext
@ stdcall CPCreateHash(long long ptr long ptr) RSA_CPCreateHash
@ stdcall CPDecrypt(long long long long long ptr ptr) RSA_CPDecrypt
@ stdcall CPDeriveKey(long long long long ptr) RSA_CPDeriveKey
@ stdcall CPDestroyHash(long long) RSA_CPDestroyHash
@ stdcall CPDestroyKey(long long) RSA_CPDestroyKey
@ stdcall CPDuplicateHash(long long ptr long ptr) RSA_CPDuplicateHash
@ stdcall CPDuplicateKey(long long ptr long ptr) RSA_CPDuplicateKey
@ stdcall CPEncrypt(long long long long long ptr ptr long) RSA_CPEncrypt
@ stdcall CPExportKey(long long long long long ptr ptr) RSA_CPExportKey
@ stdcall CPGenKey(long long long ptr) RSA_CPGenKey
@ stdcall CPGenRandom(long long ptr) RSA_CPGenRandom
@ stdcall CPGetHashParam(long long long ptr ptr long) RSA_CPGetHashParam
@ stdcall CPGetKeyParam(long long long ptr ptr long) RSA_CPGetKeyParam
@ stdcall CPGetProvParam(long long ptr ptr long) RSA_CPGetProvParam
@ stdcall CPGetUserKey(long long ptr) RSA_CPGetUserKey
@ stdcall CPHashData(long long ptr long long) RSA_CPHashData
@ stdcall CPHashSessionKey(long long long long) RSA_CPHashSessionKey
@ stdcall CPImportKey(long ptr long long long ptr) RSA_CPImportKey
@ stdcall CPReleaseContext(long long) RSA_CPReleaseContext
@ stdcall CPSetHashParam(long long long ptr long) RSA_CPSetHashParam
@ stdcall CPSetKeyParam(long long long ptr long) RSA_CPSetKeyParam
@ stdcall CPSetProvParam(long long ptr long) RSA_CPSetProvParam
@ stdcall CPSignHash(long long long wstr long ptr ptr) RSA_CPSignHash
@ stdcall CPVerifySignature(long long ptr long long wstr long) RSA_CPVerifySignature
@ stdcall -private DllRegisterServer() RSABASE_DllRegisterServer
@ stdcall -private DllUnregisterServer() RSABASE_DllUnregisterServer
