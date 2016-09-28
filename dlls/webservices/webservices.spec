@ stub WsAbandonCall
@ stub WsAbandonMessage
@ stub WsAbortChannel
@ stub WsAbortListener
@ stub WsAbortServiceHost
@ stdcall WsAbortServiceProxy(ptr ptr)
@ stub WsAcceptChannel
@ stdcall WsAddCustomHeader(ptr ptr long ptr long long ptr)
@ stub WsAddErrorString
@ stdcall WsAddMappedHeader(ptr ptr long long ptr long ptr)
@ stdcall WsAddressMessage(ptr ptr ptr)
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
@ stdcall WsCopyNode(ptr ptr ptr)
@ stdcall WsCreateChannel(long long ptr long ptr ptr ptr)
@ stub WsCreateChannelForListener
@ stdcall WsCreateError(ptr long ptr)
@ stub WsCreateFaultFromError
@ stdcall WsCreateHeap(long long ptr long ptr ptr)
@ stub WsCreateListener
@ stdcall WsCreateMessage(long long ptr long ptr ptr)
@ stdcall WsCreateMessageForChannel(ptr ptr long ptr ptr)
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
@ stdcall WsDecodeUrl(ptr long ptr ptr ptr)
@ stdcall WsEncodeUrl(ptr long ptr ptr ptr)
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
@ stdcall WsFreeMessage(ptr)
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
@ stdcall WsGetMessageProperty(ptr long ptr long ptr)
@ stub WsGetMetadataEndpoints
@ stub WsGetMetadataProperty
@ stub WsGetMissingMetadataDocumentAddress
@ stdcall WsGetNamespaceFromPrefix(ptr ptr long ptr ptr)
@ stub WsGetOperationContextProperty
@ stub WsGetPolicyAlternativeCount
@ stub WsGetPolicyProperty
@ stdcall WsGetPrefixFromNamespace(ptr ptr long ptr ptr)
@ stdcall WsGetReaderNode(ptr ptr ptr)
@ stdcall WsGetReaderPosition(ptr ptr ptr)
@ stdcall WsGetReaderProperty(ptr long ptr long ptr)
@ stub WsGetSecurityContextProperty
@ stub WsGetSecurityTokenProperty
@ stub WsGetServiceHostProperty
@ stdcall WsGetServiceProxyProperty(ptr long ptr long ptr)
@ stdcall WsGetWriterPosition(ptr ptr ptr)
@ stdcall WsGetWriterProperty(ptr long ptr long ptr)
@ stdcall WsGetXmlAttribute(ptr ptr ptr ptr ptr ptr)
@ stdcall WsInitializeMessage(ptr long ptr ptr)
@ stub WsMarkHeaderAsUnderstood
@ stub WsMatchPolicyAlternative
@ stdcall WsMoveReader(ptr long ptr ptr)
@ stdcall WsMoveWriter(ptr long ptr ptr)
@ stdcall WsOpenChannel(ptr ptr ptr ptr)
@ stub WsOpenListener
@ stub WsOpenServiceHost
@ stdcall WsOpenServiceProxy(ptr ptr ptr ptr)
@ stub WsPullBytes
@ stub WsPushBytes
@ stub WsReadArray
@ stub WsReadAttribute
@ stdcall WsReadBody(ptr ptr long ptr ptr long ptr)
@ stub WsReadBytes
@ stub WsReadChars
@ stub WsReadCharsUtf8
@ stdcall WsReadElement(ptr ptr long ptr ptr long ptr)
@ stdcall WsReadEndAttribute(ptr ptr)
@ stdcall WsReadEndElement(ptr ptr)
@ stub WsReadEndpointAddressExtension
@ stdcall WsReadEnvelopeEnd(ptr ptr)
@ stdcall WsReadEnvelopeStart(ptr ptr ptr ptr ptr)
@ stub WsReadMessageEnd
@ stub WsReadMessageStart
@ stub WsReadMetadata
@ stdcall WsReadNode(ptr ptr)
@ stub WsReadQualifiedName
@ stdcall WsReadStartAttribute(ptr long ptr)
@ stdcall WsReadStartElement(ptr ptr)
@ stdcall WsReadToStartElement(ptr ptr ptr ptr ptr)
@ stdcall WsReadType(ptr long long ptr long ptr ptr long ptr)
@ stdcall WsReadValue(ptr long ptr long ptr)
@ stub WsReadXmlBuffer
@ stub WsReadXmlBufferFromBytes
@ stdcall WsReceiveMessage(ptr ptr ptr long long long ptr ptr long ptr ptr ptr)
@ stub WsRegisterOperationForCancel
@ stdcall WsRemoveCustomHeader(ptr ptr ptr ptr)
@ stdcall WsRemoveHeader(ptr long ptr)
@ stdcall WsRemoveMappedHeader(ptr ptr ptr)
@ stub WsRemoveNode
@ stub WsRequestReply
@ stub WsRequestSecurityToken
@ stub WsResetChannel
@ stdcall WsResetError(ptr)
@ stdcall WsResetHeap(ptr ptr)
@ stub WsResetListener
@ stub WsResetMessage
@ stub WsResetMetadata
@ stub WsResetServiceHost
@ stub WsResetServiceProxy
@ stub WsRevokeSecurityContext
@ stub WsSendFaultMessageForError
@ stdcall WsSendMessage(ptr ptr ptr long ptr long ptr ptr)
@ stub WsSendReplyMessage
@ stdcall WsSetChannelProperty(ptr long ptr long ptr)
@ stdcall WsSetErrorProperty(ptr long ptr long)
@ stub WsSetFaultErrorDetail
@ stub WsSetFaultErrorProperty
@ stdcall WsSetHeader(ptr long long long ptr long ptr)
@ stdcall WsSetInput(ptr ptr ptr ptr long ptr)
@ stdcall WsSetInputToBuffer(ptr ptr ptr long ptr)
@ stub WsSetListenerProperty
@ stdcall WsSetMessageProperty(ptr long ptr long ptr)
@ stdcall WsSetOutput(ptr ptr ptr ptr long ptr)
@ stdcall WsSetOutputToBuffer(ptr ptr ptr long ptr)
@ stdcall WsSetReaderPosition(ptr ptr ptr)
@ stdcall WsSetWriterPosition(ptr ptr ptr)
@ stub WsShutdownSessionChannel
@ stub WsSkipNode
@ stub WsStartReaderCanonicalization
@ stub WsStartWriterCanonicalization
@ stub WsTrimXmlWhitespace
@ stub WsVerifyXmlNCName
@ stdcall WsWriteArray(ptr ptr ptr long ptr long long long ptr)
@ stdcall WsWriteAttribute(ptr ptr long ptr long ptr)
@ stdcall WsWriteBody(ptr ptr long ptr long ptr)
@ stub WsWriteBytes
@ stub WsWriteChars
@ stub WsWriteCharsUtf8
@ stdcall WsWriteElement(ptr ptr long ptr long ptr)
@ stdcall WsWriteEndAttribute(ptr ptr)
@ stdcall WsWriteEndCData(ptr ptr)
@ stdcall WsWriteEndElement(ptr ptr)
@ stdcall WsWriteEndStartElement(ptr ptr)
@ stdcall WsWriteEnvelopeEnd(ptr ptr)
@ stdcall WsWriteEnvelopeStart(ptr ptr ptr ptr ptr)
@ stub WsWriteMessageEnd
@ stub WsWriteMessageStart
@ stdcall WsWriteNode(ptr ptr ptr)
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
