@ stub HttpAddFragmentToCache
@ stdcall HttpAddUrl(ptr wstr ptr)
@ stdcall HttpAddUrlToUrlGroup(int64 wstr int64 long)
@ stub HttpCancelHttpRequest
@ stdcall HttpCloseRequestQueue(ptr)
@ stdcall HttpCloseServerSession(int64)
@ stdcall HttpCloseUrlGroup(int64)
@ stub HttpControlService
@ stdcall HttpCreateHttpHandle(ptr long)
@ stdcall HttpCreateRequestQueue(long wstr ptr long ptr)
@ stdcall HttpCreateServerSession(long ptr long)
@ stdcall HttpCreateUrlGroup(int64 ptr long)
@ stdcall HttpDeleteServiceConfiguration(ptr long ptr long ptr)
@ stub HttpFlushResponseCache
@ stub HttpGetCounters
@ stdcall HttpInitialize(long long ptr)
@ stub HttpQueryRequestQueueProperty
@ stub HttpQueryServerSessionProperty
@ stdcall HttpQueryServiceConfiguration(ptr long ptr long ptr long ptr ptr)
@ stub HttpQueryUrlGroupProperty
@ stub HttpReadFragmentFromCache
@ stub HttpReceiveClientCertificate
@ stdcall HttpReceiveHttpRequest(ptr int64 long ptr long ptr ptr)
@ stdcall HttpReceiveRequestEntityBody(ptr int64 long ptr long ptr ptr)
@ stdcall HttpRemoveUrl(ptr wstr)
@ stdcall HttpRemoveUrlFromUrlGroup(int64 wstr long)
@ stdcall HttpSendHttpResponse(ptr int64 long ptr ptr ptr ptr long ptr ptr)
@ stdcall HttpSendResponseEntityBody(ptr int64 long long ptr ptr ptr long ptr ptr)
@ stdcall HttpSetRequestQueueProperty(ptr long ptr long long ptr)
@ stdcall HttpSetServerSessionProperty(int64 long ptr long)
@ stdcall HttpSetServiceConfiguration(ptr long ptr long ptr)
@ stdcall HttpSetUrlGroupProperty(int64 long ptr long)
@ stub HttpShutdownRequestQueue
@ stdcall HttpTerminate(long ptr)
@ stub HttpWaitForDemandStart
@ stub HttpWaitForDisconnect
@ stub HttpWaitForDisconnectEx
