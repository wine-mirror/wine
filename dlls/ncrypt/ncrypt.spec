@ stdcall BCryptAddContextFunction(long wstr long wstr long) bcrypt.BCryptAddContextFunction
@ stdcall BCryptAddContextFunctionProvider(long wstr long wstr wstr long) bcrypt.BCryptAddContextFunctionProvider
@ stdcall BCryptCloseAlgorithmProvider(ptr long) bcrypt.BCryptCloseAlgorithmProvider
@ stub BCryptConfigureContext
@ stub BCryptConfigureContextFunction
@ stub BCryptCreateContext
@ stdcall BCryptCreateHash(ptr ptr ptr long ptr long long) bcrypt.BCryptCreateHash
@ stdcall BCryptDecrypt(ptr ptr long ptr ptr long ptr long ptr long) bcrypt.BCryptDecrypt
@ stub BCryptDeleteContext
@ stub BCryptDeriveKey
@ stub BCryptDeriveKeyCapi
@ stub BCryptDeriveKeyPBKDF2
@ stdcall BCryptDestroyHash(ptr) bcrypt.BCryptDestroyHash
@ stdcall BCryptDestroyKey(ptr) bcrypt.BCryptDestroyKey
@ stub BCryptDestroySecret
@ stdcall BCryptDuplicateHash(ptr ptr ptr long long) bcrypt.BCryptDuplicateHash
@ stdcall BCryptDuplicateKey(ptr ptr ptr long long) bcrypt.BCryptDuplicateKey
@ stdcall BCryptEncrypt(ptr ptr long ptr ptr long ptr long ptr long) bcrypt.BCryptEncrypt
@ stdcall BCryptEnumAlgorithms(long ptr ptr long) bcrypt.BCryptEnumAlgorithms
@ stub BCryptEnumContextFunctionProviders
@ stub BCryptEnumContextFunctions
@ stub BCryptEnumContexts
@ stub BCryptEnumProviders
@ stub BCryptEnumRegisteredProviders
@ stdcall BCryptExportKey(ptr ptr wstr ptr long ptr long) bcrypt.BCryptExportKey
@ stub BCryptFinalizeKeyPair
@ stdcall BCryptFinishHash(ptr ptr long long) bcrypt.BCryptFinishHash
@ stub BCryptFreeBuffer
@ stdcall BCryptGenRandom(ptr ptr long long) bcrypt.BCryptGenRandom
@ stub BCryptGenerateKeyPair
@ stdcall BCryptGenerateSymmetricKey(ptr ptr ptr long ptr long long) bcrypt.BCryptGenerateSymmetricKey
@ stdcall BCryptGetFipsAlgorithmMode(ptr) bcrypt.BCryptGetFipsAlgorithmMode
@ stdcall BCryptGetProperty(ptr wstr ptr long ptr long) bcrypt.BCryptGetProperty
@ stdcall BCryptHash(ptr ptr long ptr long ptr long) bcrypt.BCryptHash
@ stdcall BCryptHashData(ptr ptr long long) bcrypt.BCryptHashData
@ stdcall BCryptImportKey(ptr ptr wstr ptr ptr long ptr long long) bcrypt.BCryptImportKey
@ stub BCryptImportKeyPair
@ stub BCryptKeyDerivation
@ stdcall BCryptOpenAlgorithmProvider(ptr wstr wstr long) bcrypt.BCryptOpenAlgorithmProvider
@ stub BCryptQueryContextConfiguration
@ stub BCryptQueryContextFunctionConfiguration
@ stub BCryptQueryContextFunctionProperty
@ stub BCryptQueryProviderRegistration
@ stub BCryptRegisterConfigChangeNotify
@ stdcall BCryptRegisterProvider(wstr long ptr) bcrypt.BCryptRegisterProvider
@ stdcall BCryptRemoveContextFunction(long wstr long wstr) bcrypt.BCryptRemoveContextFunction
@ stdcall BCryptRemoveContextFunctionProvider(long wstr long wstr wstr) bcrypt.BCryptRemoveContextFunctionProvider
@ stub BCryptResolveProviders
@ stub BCryptSecretAgreement
@ stub BCryptSetAuditingInterface
@ stub BCryptSetContextFunctionProperty
@ stdcall BCryptSetProperty(ptr wstr ptr long long) bcrypt.BCryptSetProperty
@ stub BCryptSignHash
@ stub BCryptUnregisterConfigChangeNotify
@ stdcall BCryptUnregisterProvider(wstr) bcrypt.BCryptUnregisterProvider
@ stub BCryptVerifySignature
@ stub GetIsolationServerInterface
@ stub GetKeyStorageInterface
@ stub GetSChannelInterface
@ stub NCryptCloseKeyProtector
@ stub NCryptCloseProtectionDescriptor
@ stub NCryptCreateClaim
@ stdcall NCryptCreatePersistedKey(long ptr wstr wstr long long)
@ stub NCryptCreateProtectionDescriptor
@ stdcall NCryptDecrypt(long ptr long ptr ptr long ptr long)
@ stub NCryptDeleteKey
@ stub NCryptDeriveKey
@ stub NCryptDuplicateKeyProtectorHandle
@ stdcall NCryptEncrypt(long ptr long ptr ptr long ptr long)
@ stub NCryptEnumAlgorithms
@ stub NCryptEnumKeys
@ stub NCryptEnumStorageProviders
@ stub NCryptExportKey
@ stdcall NCryptFinalizeKey(long long)
@ stub NCryptFreeBuffer
@ stdcall NCryptFreeObject(long)
@ stub NCryptGetProperty
@ stub NCryptGetProtectionDescriptorInfo
@ stub NCryptImportKey
@ stub NCryptIsAlgSupported
@ stub NCryptIsKeyHandle
@ stub NCryptKeyDerivation
@ stub NCryptNotifyChangeKey
@ stdcall NCryptOpenKey(long ptr wstr long long)
@ stub NCryptOpenKeyProtector
@ stdcall NCryptOpenStorageProvider(ptr wstr long)
@ stub NCryptProtectKey
@ stub NCryptProtectSecret
@ stub NCryptQueryProtectionDescriptorName
@ stub NCryptRegisterProtectionDescriptorName
@ stub NCryptSecretAgreement
@ stub NCryptSetAuditingInterface
@ stub NCryptSetProperty
@ stub NCryptSignHash
@ stub NCryptStreamClose
@ stub NCryptStreamOpenToProtect
@ stub NCryptStreamOpenToUnprotect
@ stub NCryptStreamOpenToUnprotectEx
@ stub NCryptStreamUpdate
@ stub NCryptTranslateHandle
@ stub NCryptUnprotectKey
@ stub NCryptUnprotectSecret
@ stub NCryptVerifyClaim
@ stub NCryptVerifySignature
@ stub SslChangeNotify
@ stub SslComputeClientAuthHash
@ stub SslComputeEapKeyBlock
@ stub SslComputeFinishedHash
@ stub SslComputeSessionHash
@ stub SslCreateClientAuthHash
@ stub SslCreateEphemeralKey
@ stub SslCreateHandshakeHash
@ stub SslDecrementProviderReferenceCount
@ stub SslDecryptPacket
@ stub SslEncryptPacket
@ stub SslEnumCipherSuites
@ stub SslEnumEccCurves
@ stub SslEnumProtocolProviders
@ stub SslExportKey
@ stub SslExportKeyingMaterial
@ stub SslFreeBuffer
@ stub SslFreeObject
@ stub SslGenerateMasterKey
@ stub SslGeneratePreMasterKey
@ stub SslGenerateSessionKeys
@ stub SslGetCipherSuitePRFHashAlgorithm
@ stub SslGetKeyProperty
@ stub SslGetProviderProperty
@ stub SslHashHandshake
@ stub SslImportKey
@ stub SslImportMasterKey
@ stub SslIncrementProviderReferenceCount
@ stub SslLookupCipherLengths
@ stub SslLookupCipherSuiteInfo
@ stub SslOpenPrivateKey
@ stub SslOpenProvider
@ stub SslSignHash
@ stub SslVerifySignature
