name	ole32
type	win32

  1 stub BindMoniker
  2 stdcall CLSIDFromProgID(wstr ptr) CLSIDFromProgID32
  3 stdcall CLSIDFromString(wstr ptr) CLSIDFromString32
  4 stdcall CoBuildVersion() CoBuildVersion
  5 stub CoCreateFreeThreadedMarshaler
  6 stdcall CoCreateGuid(ptr) CoCreateGuid
  7 stdcall CoCreateInstance(ptr ptr long ptr ptr) CoCreateInstance
  8 stdcall CoDisconnectObject(ptr long) CoDisconnectObject
  9 stdcall CoDosDateTimeToFileTime(long long ptr) DosDateTimeToFileTime
 10 stdcall CoFileTimeNow(ptr) CoFileTimeNow
 11 stdcall CoFileTimeToDosDateTime(ptr ptr ptr) FileTimeToDosDateTime
 12 stdcall CoFreeAllLibraries() CoFreeAllLibraries
 13 stdcall CoFreeLibrary(long) CoFreeLibrary
 14 stdcall CoFreeUnusedLibraries() CoFreeUnusedLibraries
 15 stub CoGetCallerTID
 16 stdcall CoGetClassObject(ptr long ptr ptr ptr) CoGetClassObject
 17 stub CoGetCurrentLogicalThreadId
 18 stdcall CoGetCurrentProcess() CoGetCurrentProcess
 19 stub CoGetInterfaceAndReleaseStream
 20 stdcall CoGetMalloc(long ptr) CoGetMalloc32
 21 stub CoGetMarshalSizeMax
 22 stub CoGetPSClsid
 23 stub CoGetStandardMarshal
 24 stub CoGetState
 25 stub CoGetTreatAsClass
 26 stdcall CoInitialize(long) CoInitialize32
 27 stdcall CoInitializeWOW(long long) CoInitializeWOW
 28 stub CoIsHandlerConnected
 29 stub CoIsOle1Class
 30 stdcall CoLoadLibrary(ptr long) CoLoadLibrary
 31 stdcall CoLockObjectExternal(ptr long long) CoLockObjectExternal32
 32 stub CoMarshalHresult
 33 stub CoMarshalInterThreadInterfaceInStream
 34 stub CoMarshalInterface
 35 stub CoQueryReleaseObject
 36 stdcall CoRegisterClassObject(ptr ptr long long ptr) CoRegisterClassObject32
 37 stub CoRegisterMallocSpy
 38 stdcall CoRegisterMessageFilter(ptr ptr) CoRegisterMessageFilter32
 39 stub CoReleaseMarshalData
 40 stdcall CoRevokeClassObject(long) CoRevokeClassObject
 41 stub CoRevokeMallocSpy
 42 stdcall CoSetState(ptr) CoSetState32
 43 stdcall CoTaskMemAlloc(long) CoTaskMemAlloc
 44 stdcall CoTaskMemFree(ptr) CoTaskMemFree
 45 stub CoTaskMemRealloc
 46 stub CoTreatAsClass
 47 stdcall CoUninitialize() CoUninitialize
 48 stub CoUnloadingWOW
 49 stub CoUnmarshalHresult
 50 stub CoUnmarshalInterface
 51 stub CreateAntiMoniker
 52 stdcall CreateBindCtx(long ptr) CreateBindCtx32
 53 stub CreateDataAdviseHolder
 54 stub CreateDataCache
 55 stdcall CreateFileMoniker(ptr ptr) CreateFileMoniker32
 56 stub CreateGenericComposite
 57 stub CreateILockBytesOnHGlobal
 58 stdcall CreateItemMoniker(ptr ptr ptr) CreateItemMoniker32
 59 stdcall CreateOleAdviseHolder(ptr) CreateOleAdviseHolder32
 60 stub CreatePointerMoniker
 61 stub CreateStreamOnHGlobal
 62 stub DllDebugObjectRPCHook
 63 stub DllGetClassObject
 64 stub DllGetClassObjectWOW
 65 stub DoDragDrop
 66 stub EnableHookObject
 67 stub GetClassFile
 68 stub GetConvertStg
 69 stub GetDocumentBitStg
 70 stub GetHGlobalFromILockBytes
 71 stub GetHGlobalFromStream
 72 stub GetHookInterface
 73 stdcall GetRunningObjectTable(long ptr) GetRunningObjectTable32
 74 stub IIDFromString
 75 stub IsAccelerator
 76 stub IsEqualGUID
 77 stub IsValidIid
 78 stdcall IsValidInterface(ptr) IsValidInterface32
 79 stub IsValidPtrIn
 80 stub IsValidPtrOut
 81 stub MkParseDisplayName
 82 stub MonikerCommonPrefixWith
 83 stub MonikerRelativePathTo
 84 stdcall OleBuildVersion() OleBuildVersion
 85 stub OleConvertIStorageToOLESTREAM
 86 stub OleConvertIStorageToOLESTREAMEx
 87 stub OleConvertOLESTREAMToIStorage
 88 stub OleConvertOLESTREAMToIStorageEx
 89 stub OleCreate
 90 stub OleCreateDefaultHandler
 91 stub OleCreateEmbeddingHelper
 92 stub OleCreateFromData
 93 stub OleCreateFromFile
 94 stub OleCreateLink
 95 stub OleCreateLinkFromData
 96 stub OleCreateLinkToFile
 97 stub OleCreateMenuDescriptor
 98 stub OleCreateStaticFromData
 99 stub OleDestroyMenuDescriptor
100 stub OleDoAutoConvert
101 stub OleDraw
102 stub OleDuplicateData
103 stub OleFlushClipboard
104 stub OleGetAutoConvert
105 stub OleGetClipboard
106 stub OleGetIconOfClass
107 stub OleGetIconOfFile
108 stdcall OleInitialize(ptr) OleInitialize
109 stdcall OleInitializeWOW(long) OleInitializeWOW
110 stub OleIsCurrentClipboard
111 stub OleIsRunning
112 stub OleLoad
113 stub OleLoadFromStream
114 stub OleLockRunning
115 stub OleMetafilePictFromIconAndLabel
116 stub OleNoteObjectVisible
117 stub OleQueryCreateFromData
118 stub OleQueryLinkFromData
119 stub OleRegEnumFormatEtc
120 stub OleRegEnumVerbs
121 stub OleRegGetMiscStatus
122 stdcall OleRegGetUserType(long long ptr) OleRegGetUserType32
123 stub OleRun
124 stub OleSave
125 stub OleSaveToStream
126 stub OleSetAutoConvert
127 stdcall OleSetClipboard(ptr) OleSetClipboard
128 stub OleSetContainedObject
129 stub OleSetMenuDescriptor
130 stub OleTranslateAccelerator
131 stdcall OleUninitialize() OleUninitialize
132 stub OpenOrCreateStream
133 stub ProgIDFromCLSID
134 stub ReadClassStg
135 stub ReadClassStm
136 stub ReadFmtUserTypeStg
137 stub ReadOleStg
138 stub ReadStringStream
139 stdcall RegisterDragDrop(long ptr) RegisterDragDrop32
140 stub ReleaseStgMedium
141 stdcall RevokeDragDrop(long) RevokeDragDrop32
142 stub SetConvertStg
143 stub SetDocumentBitStg
144 stdcall StgCreateDocfile(wstr long long ptr) StgCreateDocFile32
145 stub StgCreateDocfileOnILockBytes
146 stdcall StgIsStorageFile(wstr) StgIsStorageFile32
147 stub StgIsStorageILockBytes
148 stdcall StgOpenStorage(wstr ptr long ptr long ptr) StgOpenStorage32
149 stub StgOpenStorageOnILockBytes
150 stub StgSetTimes
151 stdcall StringFromCLSID(ptr ptr) StringFromCLSID32
152 stdcall StringFromGUID2(ptr ptr long) StringFromGUID2
153 stdcall StringFromIID(ptr ptr) StringFromCLSID32
154 stub UtConvertDvtd16toDvtd32
155 stub UtConvertDvtd32toDvtd16
156 stub UtGetDvtd16Info
157 stub UtGetDvtd32Info
158 stub WriteClassStg
159 stub WriteClassStm
160 stub WriteFmtUserTypeStg
161 stub WriteOleStg
162 stub WriteStringStream
