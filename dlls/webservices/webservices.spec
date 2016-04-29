@ stub WsAbandonCall
@ stub WsAbandonMessage
@ stub WsAbortChannel
@ stub WsAbortListener
@ stub WsAbortServiceHost
@ stub WsAbortServiceProxy
@ stub WsAcceptChannel
@ stub WsAddCustomHeader
@ stub WsAddErrorString
@ stub WsAddMappedHeader
@ stub WsAddressMessage
@ stdcall WsAlloc(ptr long ptr ptr)
@ stub WsAsyncExecute
@ stdcall WsCall(ptr ptr ptr ptr ptr long ptr ptr)
@ stub WsCheckMustUnderstandHeaders
@ stdcall WsCloseChannel(ptr ptr ptr)
@ stub WsCloseListener
@ stub WsCloseServiceHost
@ stdcall WsCloseServiceProxy(ptr ptr ptr)
@ stub WsCombineUrl
@ stub WsCopyError
@ stub WsCopyNode
@ stdcall WsCreateChannel(long long ptr long ptr ptr ptr)
@ stub WsCreateChannelForListener
@ stdcall WsCreateError(ptr long ptr)
@ stub WsCreateFaultFromError
@ stdcall WsCreateHeap(long long ptr long ptr ptr)
@ stub WsCreateListener
@ stub WsCreateMessage
@ stub WsCreateMessageForChannel
@ stub WsCreateMetadata
@ stdcall WsCreateReader(ptr long ptr ptr)
@ stub WsCreateServiceEndpointFromTemplate
@ stub WsCreateServiceHost
@ stdcall WsCreateServiceProxy(long long ptr ptr long ptr long ptr ptr)
@ stdcall WsCreateServiceProxyFromTemplate(long ptr long long ptr long ptr long ptr ptr)
@ stdcall WsCreateWriter(ptr long ptr ptr)
@ stdcall WsCreateXmlBuffer(ptr ptr long ptr ptr)
@ stub WsCreateXmlSecurityToken
@ stdcall WsDateTimeToFileTime(ptr ptr ptr)
@ stub WsDecodeUrl
@ stub WsEncodeUrl
@ stub WsEndReaderCanonicalization
@ stub WsEndWriterCanonicalization
@ stdcall WsFileTimeToDateTime(ptr ptr ptr)
@ stub WsFillBody
@ stdcall WsFillReader(ptr long ptr ptr)
@ stdcall WsFindAttribute(ptr ptr ptr long ptr ptr)
@ stub WsFlushBody
@ stub WsFlushWriter
@ stdcall WsFreeChannel(ptr)
@ stdcall WsFreeError(ptr)
@ stdcall WsFreeHeap(ptr)
@ stub WsFreeListener
@ stub WsFreeMessage
@ stub WsFreeMetadata
@ stdcall WsFreeReader(ptr)
@ stub WsFreeSecurityToken
@ stub WsFreeServiceHost
@ stdcall WsFreeServiceProxy(ptr)
@ stdcall WsFreeWriter(ptr)
@ stdcall WsGetChannelProperty(ptr long ptr long ptr)
@ stub WsGetCustomHeader
@ stub WsGetDictionary
@ stdcall WsGetErrorProperty(ptr long ptr long)
@ stdcall WsGetErrorString(ptr long ptr)
@ stub WsGetFaultErrorDetail
@ stub WsGetFaultErrorProperty
@ stub WsGetHeader
@ stub WsGetHeaderAttributes
@ stdcall WsGetHeapProperty(ptr long ptr long ptr)
@ stub WsGetListenerProperty
@ stub WsGetMappedHeader
@ stub WsGetMessageProperty
@ stub WsGetMetadataEndpoints
@ stub WsGetMetadataProperty
@ stub WsGetMissingMetadataDocumentAddress
@ stdcall WsGetNamespaceFromPrefix(ptr ptr long ptr ptr)
@ stub WsGetOperationContextProperty
@ stub WsGetPolicyAlternativeCount
@ stub WsGetPolicyProperty
@ stdcall WsGetPrefixFromNamespace(ptr ptr long ptr ptr)
@ stdcall WsGetReaderNode(ptr ptr ptr)
@ stub WsGetReaderPosition
@ stdcall WsGetReaderProperty(ptr long ptr long ptr)
@ stub WsGetSecurityContextProperty
@ stub WsGetSecurityTokenProperty
@ stub WsGetServiceHostProperty
@ stdcall WsGetServiceProxyProperty(ptr long ptr long ptr)
@ stub WsGetWriterPosition
@ stdcall WsGetWriterProperty(ptr long ptr long ptr)
@ stdcall WsGetXmlAttribute(ptr ptr ptr ptr ptr ptr)
@ stub WsInitializeMessage
@ stub WsMarkHeaderAsUnderstood
@ stub WsMatchPolicyAlternative
@ stdcall WsMoveReader(ptr long ptr ptr)
@ stub WsMoveWriter
@ stdcall WsOpenChannel(ptr ptr ptr ptr)
@ stub WsOpenListener
@ stub WsOpenServiceHost
@ stdcall WsOpenServiceProxy(ptr ptr ptr ptr)
@ stub WsPullBytes
@ stub WsPushBytes
@ stub WsReadArray
@ stub WsReadAttribute
@ stub WsReadBody
@ stub WsReadBytes
@ stub WsReadChars
@ stub WsReadCharsUtf8
@ stub WsReadElement
@ stdcall WsReadEndAttribute(ptr ptr)
@ stdcall WsReadEndElement(ptr ptr)
@ stub WsReadEndpointAddressExtension
@ stub WsReadEnvelopeEnd
@ stub WsReadEnvelopeStart
@ stub WsReadMessageEnd
@ stub WsReadMessageStart
@ stub WsReadMetadata
@ stdcall WsReadNode(ptr ptr)
@ stub WsReadQualifiedName
@ stdcall WsReadStartAttribute(ptr long ptr)
@ stdcall WsReadStartElement(ptr ptr)
@ stdcall WsReadToStartElement(ptr ptr ptr ptr ptr)
@ stdcall WsReadType(ptr long long ptr long ptr ptr long ptr)
@ stub WsReadValue
@ stub WsReadXmlBuffer
@ stub WsReadXmlBufferFromBytes
@ stub WsReceiveMessage
@ stub WsRegisterOperationForCancel
@ stub WsRemoveCustomHeader
@ stub WsRemoveHeader
@ stub WsRemoveMappedHeader
@ stub WsRemoveNode
@ stub WsRequestReply
@ stub WsRequestSecurityToken
@ stub WsResetChannel
@ stub WsResetError
@ stdcall WsResetHeap(ptr ptr)
@ stub WsResetListener
@ stub WsResetMessage
@ stub WsResetMetadata
@ stub WsResetServiceHost
@ stub WsResetServiceProxy
@ stub WsRevokeSecurityContext
@ stub WsSendFaultMessageForError
@ stub WsSendMessage
@ stub WsSendReplyMessage
@ stdcall WsSetChannelProperty(ptr long ptr long ptr)
@ stdcall WsSetErrorProperty(ptr long ptr long)
@ stub WsSetFaultErrorDetail
@ stub WsSetFaultErrorProperty
@ stub WsSetHeader
@ stdcall WsSetInput(ptr ptr ptr ptr long ptr)
@ stdcall WsSetInputToBuffer(ptr ptr ptr long ptr)
@ stub WsSetListenerProperty
@ stub WsSetMessageProperty
@ stdcall WsSetOutput(ptr ptr ptr ptr long ptr)
@ stdcall WsSetOutputToBuffer(ptr ptr ptr long ptr)
@ stub WsSetReaderPosition
@ stub WsSetWriterPosition
@ stub WsShutdownSessionChannel
@ stub WsSkipNode
@ stub WsStartReaderCanonicalization
@ stub WsStartWriterCanonicalization
@ stub WsTrimXmlWhitespace
@ stub WsVerifyXmlNCName
@ stub WsWriteArray
@ stdcall WsWriteAttribute(ptr ptr long ptr long ptr)
@ stub WsWriteBody
@ stub WsWriteBytes
@ stub WsWriteChars
@ stub WsWriteCharsUtf8
@ stdcall WsWriteElement(ptr ptr long ptr long ptr)
@ stdcall WsWriteEndAttribute(ptr ptr)
@ stdcall WsWriteEndCData(ptr ptr)
@ stdcall WsWriteEndElement(ptr ptr)
@ stdcall WsWriteEndStartElement(ptr ptr)
@ stub WsWriteEnvelopeEnd
@ stub WsWriteEnvelopeStart
@ stub WsWriteMessageEnd
@ stub WsWriteMessageStart
@ stub WsWriteNode
@ stub WsWriteQualifiedName
@ stdcall WsWriteStartAttribute(ptr ptr ptr ptr long ptr)
@ stdcall WsWriteStartCData(ptr ptr)
@ stdcall WsWriteStartElement(ptr ptr ptr ptr ptr)
@ stdcall WsWriteText(ptr ptr ptr)
@ stdcall WsWriteType(ptr long long ptr long ptr long ptr)
@ stdcall WsWriteValue(ptr long ptr long ptr)
@ stdcall WsWriteXmlBuffer(ptr ptr ptr)
@ stdcall WsWriteXmlBufferToBytes(ptr ptr ptr ptr long ptr ptr ptr ptr)
@ stdcall WsWriteXmlnsAttribute(ptr ptr ptr long ptr)
@ stdcall WsXmlStringEquals(ptr ptr ptr)
