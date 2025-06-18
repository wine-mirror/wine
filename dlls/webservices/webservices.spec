@ stub WsAbandonCall
@ stub WsAbandonMessage
@ stdcall WsAbortChannel(ptr ptr)
@ stub WsAbortListener
@ stub WsAbortServiceHost
@ stdcall WsAbortServiceProxy(ptr ptr)
@ stdcall WsAcceptChannel(ptr ptr ptr ptr)
@ stdcall WsAddCustomHeader(ptr ptr long ptr long long ptr)
@ stdcall WsAddErrorString(ptr ptr)
@ stdcall WsAddMappedHeader(ptr ptr long long ptr long ptr)
@ stdcall WsAddressMessage(ptr ptr ptr)
@ stdcall WsAlloc(ptr long ptr ptr)
@ stub WsAsyncExecute
@ stdcall WsCall(ptr ptr ptr ptr ptr long ptr ptr)
@ stub WsCheckMustUnderstandHeaders
@ stdcall WsCloseChannel(ptr ptr ptr)
@ stdcall WsCloseListener(ptr ptr ptr)
@ stub WsCloseServiceHost
@ stdcall WsCloseServiceProxy(ptr ptr ptr)
@ stub WsCombineUrl
@ stub WsCopyError
@ stdcall WsCopyNode(ptr ptr ptr)
@ stdcall WsCreateChannel(long long ptr long ptr ptr ptr)
@ stdcall WsCreateChannelForListener(ptr ptr long ptr ptr)
@ stdcall WsCreateError(ptr long ptr)
@ stub WsCreateFaultFromError
@ stdcall WsCreateHeap(long long ptr long ptr ptr)
@ stdcall WsCreateListener(long long ptr long ptr ptr ptr)
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
@ stdcall WsFillBody(ptr long ptr ptr)
@ stdcall WsFillReader(ptr long ptr ptr)
@ stdcall WsFindAttribute(ptr ptr ptr long ptr ptr)
@ stdcall WsFlushBody(ptr long ptr ptr)
@ stdcall WsFlushWriter(ptr long ptr ptr)
@ stdcall WsFreeChannel(ptr)
@ stdcall WsFreeError(ptr)
@ stdcall WsFreeHeap(ptr)
@ stdcall WsFreeListener(ptr)
@ stdcall WsFreeMessage(ptr)
@ stub WsFreeMetadata
@ stdcall WsFreeReader(ptr)
@ stub WsFreeSecurityToken
@ stub WsFreeServiceHost
@ stdcall WsFreeServiceProxy(ptr)
@ stdcall WsFreeWriter(ptr)
@ stdcall WsGetChannelProperty(ptr long ptr long ptr)
@ stdcall WsGetCustomHeader(ptr ptr long long long ptr ptr long ptr ptr)
@ stdcall WsGetDictionary(long ptr ptr)
@ stdcall WsGetErrorProperty(ptr long ptr long)
@ stdcall WsGetErrorString(ptr long ptr)
@ stdcall WsGetFaultErrorDetail(ptr ptr ptr ptr ptr long)
@ stdcall WsGetFaultErrorProperty(ptr long ptr long)
@ stdcall WsGetHeader(ptr long long long ptr ptr long ptr)
@ stub WsGetHeaderAttributes
@ stdcall WsGetHeapProperty(ptr long ptr long ptr)
@ stdcall WsGetListenerProperty(ptr long ptr long ptr)
@ stdcall WsGetMappedHeader(ptr ptr long long long long ptr ptr long ptr)
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
@ stdcall WsOpenListener(ptr ptr ptr ptr)
@ stub WsOpenServiceHost
@ stdcall WsOpenServiceProxy(ptr ptr ptr ptr)
@ stub WsPullBytes
@ stub WsPushBytes
@ stub WsReadArray
@ stdcall WsReadAttribute(ptr ptr long ptr ptr long ptr)
@ stdcall WsReadBody(ptr ptr long ptr ptr long ptr)
@ stdcall WsReadBytes(ptr ptr long ptr ptr)
@ stdcall WsReadChars(ptr ptr long ptr ptr)
@ stdcall WsReadCharsUtf8(ptr ptr long ptr ptr)
@ stdcall WsReadElement(ptr ptr long ptr ptr long ptr)
@ stdcall WsReadEndAttribute(ptr ptr)
@ stdcall WsReadEndElement(ptr ptr)
@ stub WsReadEndpointAddressExtension
@ stdcall WsReadEnvelopeEnd(ptr ptr)
@ stdcall WsReadEnvelopeStart(ptr ptr ptr ptr ptr)
@ stdcall WsReadMessageEnd(ptr ptr ptr ptr)
@ stdcall WsReadMessageStart(ptr ptr ptr ptr)
@ stub WsReadMetadata
@ stdcall WsReadNode(ptr ptr)
@ stdcall WsReadQualifiedName(ptr ptr ptr ptr ptr ptr)
@ stdcall WsReadStartAttribute(ptr long ptr)
@ stdcall WsReadStartElement(ptr ptr)
@ stdcall WsReadToStartElement(ptr ptr ptr ptr ptr)
@ stdcall WsReadType(ptr long long ptr long ptr ptr long ptr)
@ stdcall WsReadValue(ptr long ptr long ptr)
@ stdcall WsReadXmlBuffer(ptr ptr ptr ptr)
@ stub WsReadXmlBufferFromBytes
@ stdcall WsReceiveMessage(ptr ptr ptr long long long ptr ptr long ptr ptr ptr)
@ stub WsRegisterOperationForCancel
@ stdcall WsRemoveCustomHeader(ptr ptr ptr ptr)
@ stdcall WsRemoveHeader(ptr long ptr)
@ stdcall WsRemoveMappedHeader(ptr ptr ptr)
@ stub WsRemoveNode
@ stdcall WsRequestReply(ptr ptr ptr long ptr long ptr ptr long ptr ptr long ptr ptr)
@ stub WsRequestSecurityToken
@ stdcall WsResetChannel(ptr ptr)
@ stdcall WsResetError(ptr)
@ stdcall WsResetHeap(ptr ptr)
@ stdcall WsResetListener(ptr ptr)
@ stdcall WsResetMessage(ptr ptr)
@ stub WsResetMetadata
@ stub WsResetServiceHost
@ stdcall WsResetServiceProxy(ptr ptr)
@ stub WsRevokeSecurityContext
@ stub WsSendFaultMessageForError
@ stdcall WsSendMessage(ptr ptr ptr long ptr long ptr ptr)
@ stdcall WsSendReplyMessage(ptr ptr ptr long ptr long ptr ptr ptr)
@ stdcall WsSetChannelProperty(ptr long ptr long ptr)
@ stdcall WsSetErrorProperty(ptr long ptr long)
@ stub WsSetFaultErrorDetail
@ stdcall WsSetFaultErrorProperty(ptr long ptr long)
@ stdcall WsSetHeader(ptr long long long ptr long ptr)
@ stdcall WsSetInput(ptr ptr ptr ptr long ptr)
@ stdcall WsSetInputToBuffer(ptr ptr ptr long ptr)
@ stdcall WsSetListenerProperty(ptr long ptr long ptr)
@ stdcall WsSetMessageProperty(ptr long ptr long ptr)
@ stdcall WsSetOutput(ptr ptr ptr ptr long ptr)
@ stdcall WsSetOutputToBuffer(ptr ptr ptr long ptr)
@ stdcall WsSetReaderPosition(ptr ptr ptr)
@ stdcall WsSetWriterPosition(ptr ptr ptr)
@ stdcall WsShutdownSessionChannel(ptr ptr ptr)
@ stdcall WsSkipNode(ptr ptr)
@ stub WsStartReaderCanonicalization
@ stub WsStartWriterCanonicalization
@ stub WsTrimXmlWhitespace
@ stub WsVerifyXmlNCName
@ stdcall WsWriteArray(ptr ptr ptr long ptr long long long ptr)
@ stdcall WsWriteAttribute(ptr ptr long ptr long ptr)
@ stdcall WsWriteBody(ptr ptr long ptr long ptr)
@ stdcall WsWriteBytes(ptr ptr long ptr)
@ stdcall WsWriteChars(ptr ptr long ptr)
@ stdcall WsWriteCharsUtf8(ptr ptr long ptr)
@ stdcall WsWriteElement(ptr ptr long ptr long ptr)
@ stdcall WsWriteEndAttribute(ptr ptr)
@ stdcall WsWriteEndCData(ptr ptr)
@ stdcall WsWriteEndElement(ptr ptr)
@ stdcall WsWriteEndStartElement(ptr ptr)
@ stdcall WsWriteEnvelopeEnd(ptr ptr)
@ stdcall WsWriteEnvelopeStart(ptr ptr ptr ptr ptr)
@ stdcall WsWriteMessageEnd(ptr ptr ptr ptr)
@ stdcall WsWriteMessageStart(ptr ptr ptr ptr)
@ stdcall WsWriteNode(ptr ptr ptr)
@ stdcall WsWriteQualifiedName(ptr ptr ptr ptr ptr)
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
