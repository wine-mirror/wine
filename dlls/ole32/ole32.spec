name	ole32
type	win32

  1 stub BindMoniker                # stdcall (ptr long ptr ptr) return 0,ERR_NOTIMPLEMENTED
  2 stdcall CLSIDFromProgID(wstr ptr) CLSIDFromProgID
  3 stdcall CLSIDFromString(wstr ptr) CLSIDFromString
  4 stdcall CoBuildVersion() CoBuildVersion
  5 stub CoCreateFreeThreadedMarshaler # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
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
 19 stub CoGetInterfaceAndReleaseStream # stdcall (ptr ptr ptr) return 0,ERR_NOTIMPLEMENTED
 20 stdcall CoGetMalloc(long ptr) CoGetMalloc
 21 stub CoGetMarshalSizeMax        # stdcall (ptr ptr ptr long ptr long) return 0,ERR_NOTIMPLEMENTED
 22 stub CoGetPSClsid               # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
 23 stub CoGetStandardMarshal       # stdcall (ptr ptr long ptr long ptr) return 0,ERR_NOTIMPLEMENTED
 24 stub CoGetState
 25 stub CoGetTreatAsClass          # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
 26 stdcall CoInitialize(ptr) CoInitialize
 27 stdcall CoInitializeWOW(long long) CoInitializeWOW
 28 stub CoIsHandlerConnected       # stdcall (ptr) return 0,ERR_NOTIMPLEMENTED
 29 stub CoIsOle1Class              # stdcall (ptr) return 0,ERR_NOTIMPLEMENTED
 30 stdcall CoLoadLibrary(wstr long) CoLoadLibrary
 31 stdcall CoLockObjectExternal(ptr long long) CoLockObjectExternal
 32 stub CoMarshalHresult           # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
 33 stub CoMarshalInterThreadInterfaceInStream # stdcall (ptr ptr ptr) return 0,ERR_NOTIMPLEMENTED
 34 stub CoMarshalInterface         # stdcall (ptr ptr ptr long ptr long) return 0,ERR_NOTIMPLEMENTED
 35 stub CoQueryReleaseObject
 36 stdcall CoRegisterClassObject(ptr ptr long long ptr) CoRegisterClassObject
 37 stub CoRegisterMallocSpy        # stdcall (ptr) return 0,ERR_NOTIMPLEMENTED
 38 stdcall CoRegisterMessageFilter(ptr ptr) CoRegisterMessageFilter
 39 stub CoReleaseMarshalData       # stdcall (ptr) return 0,ERR_NOTIMPLEMENTED
 40 stdcall CoRevokeClassObject(long) CoRevokeClassObject
 41 stub CoRevokeMallocSpy          # stdcall () return 0,ERR_NOTIMPLEMENTED
 42 stdcall CoSetState(ptr) CoSetState
 43 stdcall CoTaskMemAlloc(long) CoTaskMemAlloc
 44 stdcall CoTaskMemFree(ptr) CoTaskMemFree
 45 stdcall CoTaskMemRealloc(ptr long) CoTaskMemRealloc
 46 stdcall CoTreatAsClass(ptr ptr) CoTreatAsClass
 47 stdcall CoUninitialize() CoUninitialize
 48 stub CoUnloadingWOW
 49 stub CoUnmarshalHresult         # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
 50 stub CoUnmarshalInterface       # stdcall (ptr ptr ptr) return 0,ERR_NOTIMPLEMENTED
 51 stdcall CreateAntiMoniker(ptr)  CreateAntiMoniker
 52 stdcall CreateBindCtx(long ptr) CreateBindCtx
 53 stdcall CreateDataAdviseHolder(ptr) CreateDataAdviseHolder
 54 stdcall CreateDataCache(ptr ptr ptr ptr) CreateDataCache
 55 stdcall CreateFileMoniker(wstr ptr) CreateFileMoniker
 56 stdcall CreateGenericComposite(ptr ptr ptr) CreateGenericComposite
 57 stdcall CreateILockBytesOnHGlobal(ptr long ptr) CreateILockBytesOnHGlobal
 58 stdcall CreateItemMoniker(wstr wstr ptr) CreateItemMoniker
 59 stdcall CreateOleAdviseHolder(ptr) CreateOleAdviseHolder
 60 stub CreatePointerMoniker       # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
 61 stdcall CreateStreamOnHGlobal(ptr long ptr) CreateStreamOnHGlobal
 62 stub DllDebugObjectRPCHook
 63 stdcall DllGetClassObject (ptr ptr ptr) OLE32_DllGetClassObject
 64 stub DllGetClassObjectWOW
 65 stdcall DoDragDrop(ptr ptr long ptr) DoDragDrop
 66 stub EnableHookObject
 67 stdcall GetClassFile(ptr ptr) GetClassFile
 68 stub GetConvertStg
 69 stub GetDocumentBitStg
 70 stdcall GetHGlobalFromILockBytes(ptr ptr) GetHGlobalFromILockBytes
 71 stdcall GetHGlobalFromStream(ptr ptr) GetHGlobalFromStream
 72 stub GetHookInterface
 73 stdcall GetRunningObjectTable(long ptr) GetRunningObjectTable
 74 stub IIDFromString
 75 stdcall IsAccelerator(long long ptr long) IsAccelerator
 76 stdcall IsEqualGUID(ptr ptr) IsEqualGUID32
 77 stub IsValidIid
 78 stdcall IsValidInterface(ptr) IsValidInterface
 79 stub IsValidPtrIn
 80 stub IsValidPtrOut
 81 stub MkParseDisplayName
 82 stdcall MonikerCommonPrefixWith(ptr ptr ptr) MonikerCommonPrefixWith
 83 stub MonikerRelativePathTo
 84 stdcall OleBuildVersion() OleBuildVersion
 85 stub OleConvertIStorageToOLESTREAM
 86 stub OleConvertIStorageToOLESTREAMEx
 87 stub OleConvertOLESTREAMToIStorage
 88 stub OleConvertOLESTREAMToIStorageEx
 89 stdcall OleCreate(ptr ptr long ptr ptr ptr ptr) OleCreate
 90 stdcall OleCreateDefaultHandler(ptr ptr ptr ptr) OleCreateDefaultHandler
 91 stub OleCreateEmbeddingHelper
 92 stdcall OleCreateFromData(ptr ptr long ptr ptr ptr ptr) OleCreateFromData
 93 stdcall OleCreateFromFile(ptr ptr ptr long ptr ptr ptr ptr) OleCreateFromFile
 94 stdcall OleCreateLink(ptr ptr long ptr ptr ptr ptr) OleCreateLink
 95 stdcall OleCreateLinkFromData(ptr ptr long ptr ptr ptr ptr) OleCreateLinkFromData
 96 stdcall OleCreateLinkToFile(ptr ptr long ptr ptr ptr ptr) OleCreateLinkToFile
 97 stdcall OleCreateMenuDescriptor(long ptr) OleCreateMenuDescriptor
 98 stdcall OleCreateStaticFromData(ptr ptr long ptr ptr ptr ptr) OleCreateStaticFromData
 99 stdcall OleDestroyMenuDescriptor(long) OleDestroyMenuDescriptor
100 stub OleDoAutoConvert
101 stub OleDraw
102 stdcall OleDuplicateData(long long long) OleDuplicateData
103 stdcall OleFlushClipboard() OleFlushClipboard
104 stub OleGetAutoConvert
105 stdcall OleGetClipboard(ptr) OleGetClipboard
106 stdcall OleGetIconOfClass(ptr ptr long) OleGetIconOfClass
107 stub OleGetIconOfFile
108 stdcall OleInitialize(ptr) OleInitialize
109 stdcall OleInitializeWOW(long) OleInitializeWOW
110 stdcall OleIsCurrentClipboard(ptr) OleIsCurrentClipboard
111 stdcall OleIsRunning(ptr) OleIsRunning
112 stdcall OleLoad(ptr ptr ptr ptr) OleLoad
113 stdcall OleLoadFromStream(ptr ptr ptr) OleLoadFromStream
114 stdcall OleLockRunning(ptr long long) OleLockRunning
115 stub OleMetafilePictFromIconAndLabel
116 stub OleNoteObjectVisible
117 stdcall OleQueryCreateFromData(ptr) OleQueryCreateFromData
118 stdcall OleQueryLinkFromData(ptr) OleQueryLinkFromData
119 stdcall OleRegEnumFormatEtc(ptr long ptr) OleRegEnumFormatEtc
120 stdcall OleRegEnumVerbs(long ptr) OleRegEnumVerbs
121 stdcall OleRegGetMiscStatus(ptr long ptr) OleRegGetMiscStatus
122 stdcall OleRegGetUserType(long long ptr) OleRegGetUserType
123 stdcall OleRun(ptr) OleRun
124 stdcall OleSave(ptr ptr long) OleSave
125 stdcall OleSaveToStream(ptr ptr) OleSaveToStream
126 stub OleSetAutoConvert
127 stdcall OleSetClipboard(ptr) OleSetClipboard
128 stdcall OleSetContainedObject(ptr long) OleSetContainedObject
129 stdcall OleSetMenuDescriptor(long long long ptr ptr) OleSetMenuDescriptor
130 stdcall OleTranslateAccelerator(ptr ptr ptr) OleTranslateAccelerator
131 stdcall OleUninitialize() OleUninitialize
132 stub OpenOrCreateStream
133 stdcall ProgIDFromCLSID(wstr ptr) ProgIDFromCLSID
134 stdcall ReadClassStg(ptr ptr) ReadClassStg
135 stdcall ReadClassStm(ptr ptr) ReadClassStm
136 stdcall ReadFmtUserTypeStg(ptr ptr ptr) ReadFmtUserTypeStg
137 stub ReadOleStg
138 stub ReadStringStream
139 stdcall RegisterDragDrop(long ptr) RegisterDragDrop
140 stdcall ReleaseStgMedium(ptr) ReleaseStgMedium
141 stdcall RevokeDragDrop(long) RevokeDragDrop
142 stdcall SetConvertStg(ptr long) SetConvertStg
143 stub SetDocumentBitStg
144 stdcall StgCreateDocfile(wstr long long ptr) StgCreateDocfile
145 stdcall StgCreateDocfileOnILockBytes(ptr long long ptr) StgCreateDocfileOnILockBytes
146 stdcall StgIsStorageFile(wstr) StgIsStorageFile
147 stdcall StgIsStorageILockBytes(ptr) StgIsStorageILockBytes
148 stdcall StgOpenStorage(wstr ptr long ptr long ptr) StgOpenStorage
149 stdcall StgOpenStorageOnILockBytes(ptr ptr long long long ptr) StgOpenStorageOnILockBytes
150 stdcall StgSetTimes(ptr ptr ptr ptr ) StgSetTimes
151 stdcall StringFromCLSID(ptr ptr) StringFromCLSID
152 stdcall StringFromGUID2(ptr ptr long) StringFromGUID2
153 stdcall StringFromIID(ptr ptr) StringFromCLSID
154 stub UtConvertDvtd16toDvtd32
155 stub UtConvertDvtd32toDvtd16
156 stub UtGetDvtd16Info
157 stub UtGetDvtd32Info
158 stdcall WriteClassStg(ptr ptr) WriteClassStg
159 stdcall WriteClassStm(ptr ptr) WriteClassStm
160 stdcall WriteFmtUserTypeStg(ptr long ptr) WriteFmtUserTypeStg
161 stub WriteOleStg
162 stub WriteStringStream
163 stdcall CoInitializeEx(ptr long) CoInitializeEx
164 stub CoInitializeSecurity       # stdcall (ptr long ptr ptr long long ptr long ptr) return 0,ERR_NOTIMPLEMENTED
165 stdcall CoCreateInstanceEx(ptr ptr long ptr long ptr) CoCreateInstanceEx
166 stdcall PropVariantClear(ptr) PropVariantClear
167 stub CoCopyProxy                # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
168 stub CoGetCallContext           # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
169 stub CoGetInstanceFromFile      # stdcall (ptr ptr ptr long wstr long ptr) return 0,ERR_NOTIMPLEMENTED
170 stub CoGetInstanceFromIStorage  # stdcall (ptr ptr ptr long ptr long ptr) return 0,ERR_NOTIMPLEMENTED
171 stub CoRegisterPSClsid          # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
172 stub CoReleaseServerProcess     # stdcall () return 0,ERR_NOTIMPLEMENTED
173 stdcall CoResumeClassObjects() CoResumeClassObjects
174 stub CoRevertToSelf             # stdcall () return 0,ERR_NOTIMPLEMENTED
175 stub CoSetProxyBlanket          # stdcall (ptr long long wstr long long ptr long) return 0,ERR_NOTIMPLEMENTED
176 stub CoSuspendClassObjects      # stdcall () return 0,ERR_NOTIMPLEMENTED
177 stub CreateClassMoniker         # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
178 stub CLIPFORMAT_UserFree
179 stub CLIPFORMAT_UserMarshal
180 stub CLIPFORMAT_UserSize
181 stub CLIPFORMAT_UserUnmarshal
182 stub CoAddRefServerProcess
183 stub CoGetObject
184 stub CoGetTIDFromIPID
185 stub CoImpersonateClient
186 stub CoQueryAuthenticationServices
187 stub CoQueryClientBlanket
188 stub CoQueryProxyBlanket
189 stub CoRegisterChannelHook
190 stub CoRegisterSurrogate
191 stub CoSwitchCallContext
192 stub CreateErrorInfo
193 stub CreateObjrefMoniker
194 stub DllRegisterServer
195 stdcall FreePropVariantArray(long ptr) FreePropVariantArray
196 stub GetErrorInfo
197 stub HACCEL_UserFree
198 stub HACCEL_UserMarshal
199 stub HACCEL_UserSize
200 stub HACCEL_UserUnmarshal
201 stub HBITMAP_UserFree
202 stub HBITMAP_UserMarshal
203 stub HBITMAP_UserSize
204 stub HBITMAP_UserUnmarshal
205 stub HBRUSH_UserFree
206 stub HBRUSH_UserMarshal
207 stub HBRUSH_UserSize
208 stub HBRUSH_UserUnmarshal
209 stub HENHMETAFILE_UserFree
210 stub HENHMETAFILE_UserMarshal
211 stub HENHMETAFILE_UserSize
212 stub HENHMETAFILE_UserUnmarshal
213 stub HGLOBAL_UserFree
214 stub HGLOBAL_UserMarshal
215 stub HGLOBAL_UserSize
216 stub HGLOBAL_UserUnmarshal
217 stub HMENU_UserFree
218 stub HMENU_UserMarshal
219 stub HMENU_UserSize
220 stub HMENU_UserUnmarshal
221 stub HMETAFILEPICT_UserFree
222 stub HMETAFILEPICT_UserMarshal
223 stub HMETAFILEPICT_UserSize
224 stub HMETAFILEPICT_UserUnmarshal
225 stub HMETAFILE_UserFree
226 stub HMETAFILE_UserMarshal
227 stub HMETAFILE_UserSize
228 stub HMETAFILE_UserUnmarshal
229 stub HPALETTE_UserFree
230 stub HPALETTE_UserMarshal
231 stub HPALETTE_UserSize
232 stub HPALETTE_UserUnmarshal
233 stub HWND_UserFree
234 stub HWND_UserMarshal
235 stub HWND_UserSize
236 stub HWND_UserUnmarshal
237 stub I_RemoteMain
238 stub OleCreateEx
239 stub OleCreateFromDataEx
240 stub OleCreateFromFileEx
241 stub OleCreateLinkEx
242 stub OleCreateLinkFromDataEx
243 stub OleCreateLinkToFileEx
244 stub PropSysAllocString
245 stub PropSysFreeString
246 stdcall PropVariantCopy(ptr ptr) PropVariantCopy
247 stub SNB_UserFree
248 stub SNB_UserMarshal
249 stub SNB_UserSize
250 stub SNB_UserUnmarshal
251 stub STGMEDIUM_UserFree
252 stub STGMEDIUM_UserMarshal
253 stub STGMEDIUM_UserSize
254 stub STGMEDIUM_UserUnmarshal
255 stdcall SetErrorInfo(long ptr) SetErrorInfo
256 stub StgCreateStorageEx
257 stub StgGetIFillLockBytesOnFile
258 stub StgGetIFillLockBytesOnILockBytes
259 stub StgOpenAsyncDocfileOnIFillLockBytes
260 stub StgOpenStorageEx
261 stub UpdateDCOMSettings
262 stub WdtpInterfacePointer_UserFree
263 stub WdtpInterfacePointer_UserMarshal
264 stub WdtpInterfacePointer_UserSize
265 stub WdtpInterfacePointer_UserUnmarshal
