init	RPCRT4_LibMain

@ stub DceErrorInqTextA
@ stub DceErrorInqTextW
@ stdcall DllRegisterServer() RPCRT4_DllRegisterServer

@ stub MesBufferHandleReset
@ stub MesDecodeBufferHandleCreate
@ stub MesDecodeIncrementalHandleCreate
@ stub MesEncodeDynBufferHandleCreate
@ stub MesEncodeFixedBufferHandleCreate
@ stub MesEncodeIncrementalHandleCreate
@ stub MesHandleFree
@ stub MesIncrementalHandleReset
@ stub MesInqProcEncodingId

@ stub MqGetContext # win9x
@ stub MqRegisterQueue # win9x

@ stub RpcAbortAsyncCall
@ stub RpcAsyncAbortCall
@ stub RpcAsyncCancelCall
@ stub RpcAsyncCompleteCall
@ stub RpcAsyncGetCallStatus
@ stub RpcAsyncInitializeHandle
@ stub RpcAsyncRegisterInfo
@ stub RpcBindingCopy
@ stdcall RpcBindingFree(ptr) RpcBindingFree
@ stdcall RpcBindingFromStringBindingA(str  ptr) RpcBindingFromStringBindingA
@ stdcall RpcBindingFromStringBindingW(wstr ptr) RpcBindingFromStringBindingW
@ stub RpcBindingInqAuthClientA
@ stub RpcBindingInqAuthClientW
@ stub RpcBindingInqAuthClientExA
@ stub RpcBindingInqAuthClientExW
@ stub RpcBindingInqAuthInfoA
@ stub RpcBindingInqAuthInfoW
@ stub RpcBindingInqAuthInfoExA
@ stub RpcBindingInqAuthInfoExW
@ stdcall RpcBindingInqObject(ptr ptr) RpcBindingInqObject
@ stub RpcBindingInqOption
@ stub RpcBindingReset
@ stub RpcBindingServerFromClient
@ stub RpcBindingSetAuthInfoA
@ stub RpcBindingSetAuthInfoW
@ stub RpcBindingSetAuthInfoExA
@ stub RpcBindingSetAuthInfoExW
@ stdcall RpcBindingSetObject(ptr ptr) RpcBindingSetObject
@ stub RpcBindingSetOption
@ stdcall RpcBindingToStringBindingA(ptr ptr) RpcBindingToStringBindingA
@ stdcall RpcBindingToStringBindingW(ptr ptr) RpcBindingToStringBindingW
@ stdcall RpcBindingVectorFree(ptr) RpcBindingVectorFree
@ stub RpcCancelAsyncCall
@ stub RpcCancelThread
@ stub RpcCancelThreadEx
@ stub RpcCertGeneratePrincipalNameA
@ stub RpcCertGeneratePrincipalNameW
@ stub RpcCompleteAsyncCall
@ stub RpcEpRegisterA
@ stub RpcEpRegisterW
@ stub RpcEpRegisterNoReplaceA
@ stub RpcEpRegisterNoReplaceW
@ stub RpcEpResolveBinding
@ stub RpcEpUnregister
@ stub RpcGetAsyncCallStatus
@ stub RpcIfIdVectorFree
@ stub RpcIfInqId
@ stub RpcImpersonateClient
@ stub RpcInitializeAsyncHandle
@ stub RpcMgmtBindingInqParameter # win9x
@ stub RpcMgmtBindingSetParameter # win9x
@ stub RpcMgmtEnableIdleCleanup
@ stub RpcMgmtEpEltInqBegin
@ stub RpcMgmtEpEltInqDone
@ stub RpcMgmtEpEltInqNextA
@ stub RpcMgmtEpEltInqNextW
@ stub RpcMgmtEpUnregister
@ stub RpcMgmtInqComTimeout
@ stub RpcMgmtInqDefaultProtectLevel
@ stub RpcMgmtInqIfIds
@ stub RpcMgmtInqParameter # win9x
@ stub RpcMgmtInqServerPrincNameA
@ stub RpcMgmtInqServerPrincNameW
@ stub RpcMgmtInqStats
@ stub RpcMgmtIsServerListening
@ stub RpcMgmtSetAuthorizationFn
@ stub RpcMgmtSetCancelTimeout
@ stub RpcMgmtSetComTimeout
@ stub RpcMgmtSetParameter # win9x
@ stub RpcMgmtSetServerStackSize
@ stub RpcMgmtStatsVectorFree
@ stub RpcMgmtStopServerListening
@ stub RpcMgmtWaitServerListen
@ stub RpcNetworkInqProtseqsA
@ stub RpcNetworkInqProtseqsW
@ stub RpcNetworkIsProtseqValidA
@ stub RpcNetworkIsProtseqValidW
@ stub RpcNsBindingInqEntryNameA
@ stub RpcNsBindingInqEntryNameW
@ stub RpcObjectInqType
@ stub RpcObjectSetInqFn
@ stub RpcObjectSetType
@ stub RpcProtseqVectorFreeA
@ stub RpcProtseqVectorFreeW
@ stub RpcRaiseException
@ stub RpcRegisterAsyncInfo
@ stub RpcRevertToSelf
@ stub RpcRevertToSelfEx
@ stub RpcServerInqBindings
@ stub RpcServerInqDefaultPrincNameA
@ stub RpcServerInqDefaultPrincNameW
@ stub RpcServerInqIf
@ stdcall RpcServerListen(long long long) RpcServerListen
@ stdcall RpcServerRegisterAuthInfoA(str  long ptr ptr) RpcServerRegisterAuthInfoA
@ stdcall RpcServerRegisterAuthInfoW(wstr long ptr ptr) RpcServerRegisterAuthInfoW
@ stdcall RpcServerRegisterIf(ptr ptr ptr)                     RpcServerRegisterIf
@ stdcall RpcServerRegisterIfEx(ptr ptr ptr long long ptr)     RpcServerRegisterIfEx
@ stdcall RpcServerRegisterIf2(ptr ptr ptr long long long ptr) RpcServerRegisterIf2
@ stub RpcServerTestCancel
@ stub RpcServerUnregisterIf
@ stub RpcServerUseAllProtseqs
@ stub RpcServerUseAllProtseqsEx
@ stub RpcServerUseAllProtseqsIf
@ stub RpcServerUseAllProtseqsIfEx
@ stub RpcServerUseProtseqA
@ stub RpcServerUseProtseqW
@ stub RpcServerUseProtseqExA
@ stub RpcServerUseProtseqExW
@ stdcall RpcServerUseProtseqEpA(str  long str  ptr) RpcServerUseProtseqEpA
@ stdcall RpcServerUseProtseqEpW(wstr long wstr ptr) RpcServerUseProtseqEpW
@ stdcall RpcServerUseProtseqEpExA(str  long str  ptr ptr) RpcServerUseProtseqEpExA
@ stdcall RpcServerUseProtseqEpExW(wstr long wstr ptr ptr) RpcServerUseProtseqEpExW
@ stub RpcServerUseProtseqIfA
@ stub RpcServerUseProtseqIfW
@ stub RpcServerUseProtseqIfExA
@ stub RpcServerUseProtseqIfExW
@ stub RpcServerYield
@ stub RpcSmAllocate
@ stub RpcSmClientFree
@ stub RpcSmDestroyClientContext
@ stub RpcSmDisableAllocate
@ stub RpcSmEnableAllocate
@ stub RpcSmFree
@ stub RpcSmGetThreadHandle
@ stub RpcSmSetClientAllocFree
@ stub RpcSmSetThreadHandle
@ stub RpcSmSwapClientAllocFree
@ stub RpcSsAllocate
@ stub RpcSsDestroyClientContext
@ stub RpcSsDisableAllocate
@ stub RpcSsDontSerializeContext
@ stub RpcSsEnableAllocate
@ stub RpcSsFree
@ stub RpcSsGetContextBinding
@ stub RpcSsGetThreadHandle
@ stub RpcSsSetClientAllocFree
@ stub RpcSsSetThreadHandle
@ stub RpcSsSwapClientAllocFree
@ stdcall RpcStringBindingComposeA(str  str  str  str  str  ptr) RpcStringBindingComposeA
@ stdcall RpcStringBindingComposeW(wstr wstr wstr wstr wstr ptr) RpcStringBindingComposeW
@ stdcall RpcStringBindingParseA(str  ptr ptr ptr ptr ptr) RpcStringBindingParseA
@ stdcall RpcStringBindingParseW(wstr ptr ptr ptr ptr ptr) RpcStringBindingParseW
@ stdcall RpcStringFreeA(ptr) RpcStringFreeA
@ stdcall RpcStringFreeW(ptr) RpcStringFreeW
@ stub RpcTestCancel

@ stub TowerConstruct
@ stub TowerExplode

@ stdcall UuidCompare(ptr ptr ptr) UuidCompare
@ stdcall UuidCreate(ptr) UuidCreate
@ stdcall UuidCreateSequential(ptr) UuidCreateSequential # win 2000
@ stdcall UuidCreateNil(ptr) UuidCreateNil
@ stdcall UuidEqual(ptr ptr ptr) UuidEqual
@ stdcall UuidFromStringA(str ptr) UuidFromStringA
@ stdcall UuidFromStringW(wstr ptr) UuidFromStringW
@ stdcall UuidHash(ptr ptr) UuidHash
@ stdcall UuidIsNil(ptr ptr) UuidIsNil
@ stdcall UuidToStringA(ptr ptr) UuidToStringA
@ stdcall UuidToStringW(ptr ptr) UuidToStringW

@ stdcall CStdStubBuffer_QueryInterface(ptr ptr) CStdStubBuffer_QueryInterface
@ stdcall CStdStubBuffer_AddRef(ptr) CStdStubBuffer_AddRef
@ stdcall CStdStubBuffer_Connect(ptr ptr) CStdStubBuffer_Connect
@ stdcall CStdStubBuffer_Disconnect(ptr) CStdStubBuffer_Disconnect
@ stdcall CStdStubBuffer_Invoke(ptr ptr ptr) CStdStubBuffer_Invoke
@ stdcall CStdStubBuffer_IsIIDSupported(ptr ptr) CStdStubBuffer_IsIIDSupported
@ stdcall CStdStubBuffer_CountRefs(ptr) CStdStubBuffer_CountRefs
@ stdcall CStdStubBuffer_DebugServerQueryInterface(ptr ptr) CStdStubBuffer_DebugServerQueryInterface
@ stdcall CStdStubBuffer_DebugServerRelease(ptr ptr) CStdStubBuffer_DebugServerRelease
@ stdcall NdrCStdStubBuffer_Release(ptr ptr)  NdrCStdStubBuffer_Release
@ stub NdrCStdStubBuffer2_Release

@ stdcall IUnknown_QueryInterface_Proxy(ptr ptr ptr) IUnknown_QueryInterface_Proxy
@ stdcall IUnknown_AddRef_Proxy(ptr) IUnknown_AddRef_Proxy
@ stdcall IUnknown_Release_Proxy(ptr) IUnknown_Release_Proxy

@ stdcall NdrDllCanUnloadNow(ptr) NdrDllCanUnloadNow
@ stdcall NdrDllGetClassObject(ptr ptr ptr ptr ptr ptr) NdrDllGetClassObject
@ stdcall NdrDllRegisterProxy(long ptr ptr) NdrDllRegisterProxy
@ stdcall NdrDllUnregisterProxy(long ptr ptr) NdrDllUnregisterProxy

@ stub NdrAllocate
@ stub NdrAsyncClientCall
@ stub NdrAsyncServerCall
@ stub NdrClearOutParameters
@ stub NdrClientCall
@ varargs NdrClientCall2(ptr ptr) NdrClientCall2
@ stub NdrClientInitialize
@ stub NdrClientInitializeNew
@ stub NdrContextHandleInitialize
@ stub NdrContextHandleSize
@ stub NdrConvert
@ stub NdrConvert2
@ stub NdrCorrelationFree
@ stub NdrCorrelationInitialize
@ stub NdrCorrelationPass
@ stub NdrDcomAsyncClientCall
@ stub NdrDcomAsyncStubCall
@ stub NdrFreeBuffer
@ stub NdrFullPointerFree
@ stub NdrFullPointerInsertRefId
@ stub NdrFullPointerQueryPointer
@ stub NdrFullPointerQueryRefId
@ stub NdrFullPointerXlatFree
@ stub NdrFullPointerXlatInit
@ stub NdrGetBuffer
@ stub NdrGetDcomProtocolVersion
@ stub NdrGetPartialBuffer
@ stub NdrGetPipeBuffer
@ stub NdrGetUserMarshalInfo
@ stub NdrIsAppDoneWithPipes
@ stub NdrMapCommAndFaultStatus
@ stub NdrMarkNextActivePipe
@ stub NdrMesProcEncodeDecode
@ stub NdrMesProcEncodeDecode2
@ stub NdrMesSimpleTypeAlignSize
@ stub NdrMesSimpleTypeDecode
@ stub NdrMesSimpleTypeEncode
@ stub NdrMesTypeAlignSize
@ stub NdrMesTypeAlignSize2
@ stub NdrMesTypeDecode
@ stub NdrMesTypeDecode2
@ stub NdrMesTypeEncode
@ stub NdrMesTypeEncode2
@ stub NdrMesTypeFree2
@ stub NdrNsGetBuffer
@ stub NdrNsSendReceive
@ stub NdrOleAllocate
@ stub NdrOleFree
@ stub NdrPipePull
@ stub NdrPipePush
@ stub NdrPipeSendReceive
@ stub NdrPipesDone
@ stub NdrPipesInitialize
@ stub NdrProxyErrorHandler
@ stdcall NdrProxyFreeBuffer(ptr ptr) NdrProxyFreeBuffer
@ stdcall NdrProxyGetBuffer(ptr ptr) NdrProxyGetBuffer
@ stdcall NdrProxyInitialize(ptr ptr ptr ptr long) NdrProxyInitialize
@ stdcall NdrProxySendReceive(ptr ptr) NdrProxySendReceive
@ stub NdrRangeUnmarshall
@ stub NdrRpcSmClientAllocate
@ stub NdrRpcSmClientFree
@ stub NdrRpcSmSetClientToOsf
@ stub NdrRpcSsDefaultAllocate
@ stub NdrRpcSsDefaultFree
@ stub NdrRpcSsDisableAllocate
@ stub NdrRpcSsEnableAllocate
@ stub NdrSendReceive
@ stub NdrServerCall
@ stub NdrServerCall2
@ stub NdrStubCall
@ stub NdrStubCall2
@ stub NdrStubForwardingFunction
@ stdcall NdrStubGetBuffer(ptr ptr ptr) NdrStubGetBuffer
@ stdcall NdrStubInitialize(ptr ptr ptr ptr) NdrStubInitialize
@ stub NdrStubInitializeMarshall
@ stub NdrpSetRpcSsDefaults

@ stub NdrByteCountPointerBufferSize
@ stub NdrByteCountPointerFree
@ stub NdrByteCountPointerMarshall
@ stub NdrByteCountPointerUnmarshall
@ stub NdrClientContextMarshall
@ stub NdrClientContextUnmarshall
@ stub NdrComplexArrayBufferSize
@ stub NdrComplexArrayFree
@ stub NdrComplexArrayMarshall
@ stub NdrComplexArrayMemorySize
@ stub NdrComplexArrayUnmarshall
@ stub NdrComplexStructBufferSize
@ stub NdrComplexStructFree
@ stub NdrComplexStructMarshall
@ stub NdrComplexStructMemorySize
@ stub NdrComplexStructUnmarshall
@ stub NdrConformantArrayBufferSize
@ stub NdrConformantArrayFree
@ stub NdrConformantArrayMarshall
@ stub NdrConformantArrayMemorySize
@ stub NdrConformantArrayUnmarshall
@ stub NdrConformantStringBufferSize
@ stub NdrConformantStringMarshall
@ stub NdrConformantStringMemorySize
@ stub NdrConformantStringUnmarshall
@ stub NdrConformantStructBufferSize
@ stub NdrConformantStructFree
@ stub NdrConformantStructMarshall
@ stub NdrConformantStructMemorySize
@ stub NdrConformantStructUnmarshall
@ stub NdrConformantVaryingArrayBufferSize
@ stub NdrConformantVaryingArrayFree
@ stub NdrConformantVaryingArrayMarshall
@ stub NdrConformantVaryingStructMemorySize
@ stub NdrConformantVaryingArrayUnmarshall
@ stub NdrEncapsulatedUnionBufferSize
@ stub NdrEncapsulatedUnionFree
@ stub NdrEncapsulatedUnionMarshall
@ stub NdrEncapsulatedUnionMemorySize
@ stub NdrEncapsulatedUnionUnmarshall
@ stub NdrFixedArrayBufferSize
@ stub NdrFixedArrayFree
@ stub NdrFixedArrayMarshall
@ stub NdrFixedArrayMemorySize
@ stub NdrFixedArrayUnmarshall
@ stub NdrHardStructBufferSize
@ stub NdrHardStructFree
@ stub NdrHardStructMarshall
@ stub NdrHardStructMemorySize
@ stub NdrHardStructUnmarshall
@ stub NdrInterfacePointerBufferSize
@ stub NdrInterfacePointerFree
@ stub NdrInterfacePointerMarshall
@ stub NdrInterfacePointerMemorySize
@ stub NdrInterfacePointerUnmarshall
@ stub NdrNonConformantStringBufferSize
@ stub NdrNonConformantStringMarshall
@ stub NdrNonConformantStringMemorySize
@ stub NdrNonConformantStringUnmarshall
@ stub NdrNonEncapsulatedUnionBufferSize
@ stub NdrNonEncapsulatedUnionFree
@ stub NdrNonEncapsulatedUnionMarshall
@ stub NdrNonEncapsulatedUnionMemorySize
@ stub NdrNonEncapsulatedUnionUnmarshall
@ stub NdrPointerBufferSize
@ stub NdrPointerFree
@ stub NdrPointerMarshall
@ stub NdrPointerMemorySize
@ stub NdrPointerUnmarshall
@ stub NdrServerContextMarshall
@ stub NdrServerContextUnmarshall
@ stub NdrServerInitialize
@ stub NdrServerInitializeMarshall
@ stub NdrServerInitializeNew
@ stub NdrServerInitializeUnmarshall
@ stub NdrServerMarshall
@ stub NdrServerUnmarshall
@ stub NdrSimpleStructBufferSize
@ stub NdrSimpleStructFree
@ stub NdrSimpleStructMarshall
@ stub NdrSimpleStructMemorySize
@ stub NdrSimpleStructUnmarshall
@ stub NdrSimpleTypeMarshall
@ stub NdrSimpleTypeUnmarshall
@ stub NdrUserMarshalBufferSize
@ stub NdrUserMarshalFree
@ stub NdrUserMarshalMarshall
@ stub NdrUserMarshalMemorySize
@ stub NdrUserMarshalSimpleTypeConvert
@ stub NdrUserMarshalUnmarshall
@ stub NdrVaryingArrayBufferSize
@ stub NdrVaryingArrayFree
@ stub NdrVaryingArrayMarshall
@ stub NdrVaryingArrayMemorySize
@ stub NdrVaryingArrayUnmarshall
@ stub NdrXmitOrRepAsBufferSize
@ stub NdrXmitOrRepAsFree
@ stub NdrXmitOrRepAsMarshall
@ stub NdrXmitOrRepAsMemorySize
@ stub NdrXmitOrRepAsUnmarshall

@ stub NDRCContextBinding
@ stub NDRCContextMarshall
@ stub NDRCContextUnmarshall
@ stub NDRSContextMarshall
@ stub NDRSContextUnmarshall
@ stub NDRSContextMarshallEx
@ stub NDRSContextUnmarshallEx
@ stub NDRSContextMarshall2
@ stub NDRSContextUnmarshall2
@ stub NDRcopy

@ stub MIDL_wchar_strcpy
@ stub MIDL_wchar_strlen
@ stub char_array_from_ndr
@ stub char_from_ndr
@ stub data_from_ndr
@ stub data_into_ndr
@ stub data_size_ndr
@ stub double_array_from_ndr
@ stub double_from_ndr
@ stub enum_from_ndr
@ stub float_array_from_ndr
@ stub float_from_ndr
@ stub long_array_from_ndr
@ stub long_from_ndr
@ stub long_from_ndr_temp
@ stub short_array_from_ndr
@ stub short_from_ndr
@ stub short_from_ndr_temp
@ stub tree_into_ndr
@ stub tree_peek_ndr
@ stub tree_size_ndr

@ stub I_RpcAbortAsyncCall
@ stub I_RpcAllocate
@ stub I_RpcAsyncAbortCall
@ stub I_RpcAsyncSendReceive # NT4
@ stub I_RpcAsyncSetHandle
@ stub I_RpcBCacheAllocate
@ stub I_RpcBCacheFree
@ stub I_RpcBindingCopy
@ stub I_RpcBindingInqConnId
@ stub I_RpcBindingInqDynamicEndPoint
@ stub I_RpcBindingInqDynamicEndPointA
@ stub I_RpcBindingInqDynamicEndPointW
@ stub I_RpcBindingInqSecurityContext
@ stub I_RpcBindingInqTransportType
@ stub I_RpcBindingInqWireIdForSnego
@ stub I_RpcBindingIsClientLocal
@ stub I_RpcBindingToStaticStringBindingW
@ stdcall I_RpcBindingSetAsync(ptr ptr long) I_RpcBindingSetAsync # win9x
# NT version of I_RpcBindingSetAsync has 2 arguments, not 3
@ stub I_RpcClearMutex
@ stub I_RpcConnectionInqSockBuffSize
@ stub I_RpcConnectionInqSockBuffSize2
@ stub I_RpcConnectionSetSockBuffSize
@ stub I_RpcDeleteMutex
@ stub I_RpcFree
@ stdcall I_RpcFreeBuffer(ptr) I_RpcFreeBuffer
@ stub I_RpcFreePipeBuffer
@ stub I_RpcGetAssociationContext
@ stdcall I_RpcGetBuffer(ptr) I_RpcGetBuffer
@ stub I_RpcGetBufferWithObject
@ stub I_RpcGetCurrentCallHandle
@ stub I_RpcGetExtendedError
@ stub I_RpcGetServerContextList
@ stub I_RpcGetThreadEvent # win9x
@ stub I_RpcGetThreadWindowHandle # win9x
@ stub I_RpcIfInqTransferSyntaxes
@ stub I_RpcLaunchDatagramReceiveThread # win9x
@ stub I_RpcLogEvent
@ stub I_RpcMapWin32Status
@ stub I_RpcMonitorAssociation
@ stub I_RpcNsBindingSetEntryName
@ stub I_RpcNsBindingSetEntryNameA
@ stub I_RpcNsBindingSetEntryNameW
@ stub I_RpcNsInterfaceExported
@ stub I_RpcNsInterfaceUnexported
@ stub I_RpcParseSecurity
@ stub I_RpcPauseExecution
@ stub I_RpcReallocPipeBuffer
@ stdcall I_RpcReceive(ptr) I_RpcReceive
@ stub I_RpcRequestMutex
@ stdcall I_RpcSend(ptr) I_RpcSend
@ stdcall I_RpcSendReceive(ptr) I_RpcSendReceive
@ stub I_RpcServerAllocateIpPort
@ stub I_RpcServerInqAddressChangeFn
@ stub I_RpcServerInqTransportType
@ stub I_RpcServerRegisterForwardFunction
@ stub I_RpcServerSetAddressChangeFn
@ stub I_RpcServerStartListening # win9x
@ stub I_RpcServerStopListening # win9x
@ stub I_RpcServerUnregisterEndpointA # win9x
@ stub I_RpcServerUnregisterEndpointW # win9x
@ stub I_RpcServerUseProtseq2A
@ stub I_RpcServerUseProtseq2W
@ stub I_RpcServerUseProtseqEp2A
@ stub I_RpcServerUseProtseqEp2W
@ stub I_RpcSetAsyncHandle
@ stub I_RpcSetAssociationContext # win9x
@ stub I_RpcSetServerContextList
@ stub I_RpcSetThreadParams # win9x
@ stub I_RpcSetWMsgEndpoint # NT4
@ stub I_RpcSsDontSerializeContext
@ stub I_RpcStopMonitorAssociation
@ stub I_RpcTransCancelMigration # win9x
@ stub I_RpcTransClientMaxFrag # win9x
@ stub I_RpcTransClientReallocBuffer # win9x
@ stub I_RpcTransConnectionAllocatePacket
@ stub I_RpcTransConnectionFreePacket
@ stub I_RpcTransConnectionReallocPacket
@ stub I_RpcTransDatagramAllocate
@ stub I_RpcTransDatagramAllocate2
@ stub I_RpcTransDatagramFree
@ stub I_RpcTransGetAddressList
@ stub I_RpcTransGetThreadEvent
@ stub I_RpcTransIoCancelled
@ stub I_RpcTransMaybeMakeReceiveAny # win9x
@ stub I_RpcTransMaybeMakeReceiveDirect # win9x
@ stub I_RpcTransPingServer # win9x
@ stub I_RpcTransServerFindConnection # win9x
@ stub I_RpcTransServerFreeBuffer # win9x
@ stub I_RpcTransServerMaxFrag # win9x
@ stub I_RpcTransServerNewConnection
@ stub I_RpcTransServerProtectThread # win9x
@ stub I_RpcTransServerReallocBuffer # win9x
@ stub I_RpcTransServerReceiveDirectReady # win9x
@ stub I_RpcTransServerUnprotectThread # win9x
@ stub I_RpcWindowProc # win9x
@ stub I_RpcltDebugSetPDUFilter
@ stub I_UuidCreate

@ stub CreateProxyFromTypeInfo
@ stub CreateStubFromTypeInfo
@ stub PerformRpcInitialization
@ stub StartServiceIfNecessary # win9x
@ stub GlobalMutexClearExternal
@ stub GlobalMutexRequestExternal
