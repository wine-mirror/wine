@ stub SystemPrng
@ stub AcceptSecurityContext
@ stub AcquireCredentialsHandleW
@ stub AddCredentialsW
@ stub ApplyControlToken
@ stdcall BCryptCloseAlgorithmProvider(ptr long) bcrypt.BCryptCloseAlgorithmProvider
@ stdcall BCryptCreateHash(ptr ptr ptr long ptr long long) bcrypt.BCryptCreateHash
@ stdcall BCryptDecrypt(ptr ptr long ptr ptr long ptr long ptr long) bcrypt.BCryptDecrypt
@ stdcall BCryptDeriveKey(ptr wstr ptr ptr long ptr long) bcrypt.BCryptDeriveKey
@ stdcall BCryptDeriveKeyCapi(ptr ptr ptr long long) bcrypt.BCryptDeriveKeyCapi
@ stdcall BCryptDeriveKeyPBKDF2(ptr ptr long ptr long int64 ptr long long) bcrypt.BCryptDeriveKeyPBKDF2
@ stdcall BCryptDestroyHash(ptr) bcrypt.BCryptDestroyHash
@ stdcall BCryptDestroyKey(ptr) bcrypt.BCryptDestroyKey
@ stdcall BCryptDestroySecret(ptr) bcrypt.BCryptDestroySecret
@ stdcall BCryptDuplicateHash(ptr ptr ptr long long) bcrypt.BCryptDuplicateHash
@ stdcall BCryptDuplicateKey(ptr ptr ptr long long) bcrypt.BCryptDuplicateKey
@ stdcall BCryptEncrypt(ptr ptr long ptr ptr long ptr long ptr long) bcrypt.BCryptEncrypt
@ stdcall BCryptEnumAlgorithms(long ptr ptr long) bcrypt.BCryptEnumAlgorithms
@ stub BCryptEnumProviders
@ stdcall BCryptExportKey(ptr ptr wstr ptr long ptr long) bcrypt.BCryptExportKey
@ stdcall BCryptFinalizeKeyPair(ptr long) bcrypt.BCryptFinalizeKeyPair
@ stdcall BCryptFinishHash(ptr ptr long long) bcrypt.BCryptFinishHash
@ stdcall BCryptFreeBuffer(ptr) bcrypt.BCryptFreeBuffer
@ stdcall BCryptGenRandom(ptr ptr long long) bcrypt.BCryptGenRandom
@ stdcall BCryptGenerateKeyPair(ptr ptr long long) bcrypt.BCryptGenerateKeyPair
@ stdcall BCryptGenerateSymmetricKey(ptr ptr ptr long ptr long long) bcrypt.BCryptGenerateSymmetricKey
@ stdcall BCryptGetFipsAlgorithmMode(ptr) bcrypt.BCryptGetFipsAlgorithmMode
@ stdcall BCryptGetProperty(ptr wstr ptr long ptr long) bcrypt.BCryptGetProperty
@ stdcall BCryptHashData(ptr ptr long long) bcrypt.BCryptHashData
@ stdcall BCryptImportKey(ptr ptr wstr ptr ptr long ptr long long) bcrypt.BCryptImportKey
@ stdcall BCryptImportKeyPair(ptr ptr wstr ptr ptr long long) bcrypt.BCryptImportKeyPair
@ stdcall BCryptKeyDerivation(ptr ptr ptr long ptr long) bcrypt.BCryptKeyDerivation
@ stdcall BCryptOpenAlgorithmProvider(ptr wstr wstr long) bcrypt.BCryptOpenAlgorithmProvider
@ stub BCryptRegisterConfigChangeNotify
@ stub BCryptResolveProviders
@ stdcall BCryptSecretAgreement(ptr ptr ptr long) bcrypt.BCryptSecretAgreement
@ stdcall BCryptSetProperty(ptr wstr ptr long long) bcrypt.BCryptSetProperty
@ stdcall BCryptSignHash(ptr ptr ptr long ptr long ptr long) bcrypt.BCryptSignHash
@ stub BCryptUnregisterConfigChangeNotify
@ stdcall BCryptVerifySignature(ptr ptr ptr long ptr long long) bcrypt.BCryptVerifySignature
@ stub CompleteAuthToken
@ stub CredMarshalTargetInfo
@ stub DeleteSecurityContext
@ stub EnumerateSecurityPackagesW
@ stub ExportSecurityContext
@ stub FreeContextBuffer
@ stub FreeCredentialsHandle
@ stub GetSecurityUserInfo
@ stub ImpersonateSecurityContext
@ stub ImportSecurityContextW
@ stub InitSecurityInterfaceW
@ stub InitializeSecurityContextW
@ stub KSecRegisterSecurityProvider
@ stub KSecValidateBuffer
@ stub LsaEnumerateLogonSessions
@ stub LsaGetLogonSessionData
@ stub MakeSignature
@ stub MapSecurityError
@ stub QueryContextAttributesW
@ stub QueryCredentialsAttributesW
@ stub QuerySecurityContextToken
@ stub QuerySecurityPackageInfoW
@ stub RevertSecurityContext
@ stub SealMessage
@ stub SecLookupAccountName
@ stub SecLookupAccountSid
@ stub SecLookupWellKnownSid
@ stub SecMakeSPN
@ stub SecMakeSPNEx
@ stub SecMakeSPNEx2
@ stub SecSetPagingMode
@ stub SetCredentialsAttributesW
@ stub SslDecryptPacket
@ stub SslEncryptPacket
@ stub SslExportKey
@ stub SslFreeObject
@ stub SslGetServerIdentity
@ stub SslImportKey
@ stub SslLookupCipherSuiteInfo
@ stub SslOpenProvider
@ stub SspiAcceptSecurityContextAsync
@ stub SspiAcquireCredentialsHandleAsyncW
@ stub SspiCompareAuthIdentities
@ stub SspiCopyAuthIdentity
@ stub SspiCreateAsyncContext
@ stub SspiDeleteSecurityContextAsync
@ stub SspiEncodeAuthIdentityAsStrings
@ stub SspiEncodeStringsAsAuthIdentity
@ stub SspiFreeAsyncContext
@ stub SspiFreeAuthIdentity
@ stub SspiFreeCredentialsHandleAsync
@ stub SspiGetAsyncCallStatus
@ stub SspiInitializeSecurityContextAsyncW
@ stub SspiLocalFree
@ stub SspiMarshalAuthIdentity
@ stub SspiReinitAsyncContext
@ stub SspiSetAsyncNotifyCallback
@ stub SspiUnmarshalAuthIdentity
@ stub SspiValidateAuthIdentity
@ stub SspiZeroAuthIdentity
@ stub TokenBindingGetKeyTypesServer
@ stub TokenBindingVerifyMessage
@ stub UnsealMessage
@ stub VerifySignature
