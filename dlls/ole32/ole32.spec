  1 stdcall BindMoniker(ptr long ptr ptr)
  2 stdcall CLSIDFromProgID(wstr ptr)
  3 stdcall CLSIDFromString(wstr ptr)
  4 stdcall CoBuildVersion()
  5 stdcall CoCreateFreeThreadedMarshaler(ptr ptr)
  6 stdcall CoCreateGuid(ptr)
  7 stdcall CoCreateInstance(ptr ptr long ptr ptr)
  8 stdcall CoDisconnectObject(ptr long)
  9 stdcall CoDosDateTimeToFileTime(long long ptr) kernel32.DosDateTimeToFileTime
 10 stdcall CoFileTimeNow(ptr)
 11 stdcall CoFileTimeToDosDateTime(ptr ptr ptr) kernel32.FileTimeToDosDateTime
 12 stdcall CoFreeAllLibraries()
 13 stdcall CoFreeLibrary(long)
 14 stdcall CoFreeUnusedLibraries()
 15 stub CoGetCallerTID
 16 stdcall CoGetClassObject(ptr long ptr ptr ptr)
 17 stub CoGetCurrentLogicalThreadId
 18 stdcall CoGetCurrentProcess()
 19 stdcall CoGetInterfaceAndReleaseStream(ptr ptr ptr)
 20 stdcall CoGetMalloc(long ptr)
 21 stdcall CoGetMarshalSizeMax(ptr ptr ptr long ptr long)
 22 stdcall CoGetPSClsid(ptr ptr)
 23 stdcall CoGetStandardMarshal(ptr ptr long ptr long ptr)
 24 stdcall CoGetState(ptr)
 25 stdcall CoGetTreatAsClass(ptr ptr)
 26 stdcall CoInitialize(ptr)
 27 stdcall CoInitializeWOW(long long)
 28 stub CoIsHandlerConnected       # stdcall (ptr) return 0,ERR_NOTIMPLEMENTED
 29 stdcall CoIsOle1Class (ptr)
 30 stdcall CoLoadLibrary(wstr long)
 31 stdcall CoLockObjectExternal(ptr long long)
 32 stub CoMarshalHresult           # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
 33 stdcall CoMarshalInterThreadInterfaceInStream(ptr ptr ptr)
 34 stdcall CoMarshalInterface(ptr ptr ptr long ptr long)
 35 stub CoQueryReleaseObject
 36 stdcall CoRegisterClassObject(ptr ptr long long ptr)
 37 stdcall CoRegisterMallocSpy (ptr)
 38 stdcall CoRegisterMessageFilter(ptr ptr)
 39 stub CoReleaseMarshalData       # stdcall (ptr) return 0,ERR_NOTIMPLEMENTED
 40 stdcall CoRevokeClassObject(long)
 41 stdcall CoRevokeMallocSpy()
 42 stdcall CoSetState(ptr)
 43 stdcall CoTaskMemAlloc(long)
 44 stdcall CoTaskMemFree(ptr)
 45 stdcall CoTaskMemRealloc(ptr long)
 46 stdcall CoTreatAsClass(ptr ptr)
 47 stdcall CoUninitialize()
 48 stub CoUnloadingWOW
 49 stub CoUnmarshalHresult         # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
 50 stdcall CoUnmarshalInterface(ptr ptr ptr)
 51 stdcall CreateAntiMoniker(ptr)
 52 stdcall CreateBindCtx(long ptr)
 53 stdcall CreateDataAdviseHolder(ptr)
 54 stdcall CreateDataCache(ptr ptr ptr ptr)
 55 stdcall CreateFileMoniker(wstr ptr)
 56 stdcall CreateGenericComposite(ptr ptr ptr)
 57 stdcall CreateILockBytesOnHGlobal(ptr long ptr)
 58 stdcall CreateItemMoniker(wstr wstr ptr)
 59 stdcall CreateOleAdviseHolder(ptr)
 60 stub CreatePointerMoniker       # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
 61 stdcall CreateStreamOnHGlobal(ptr long ptr)
 62 stdcall DllDebugObjectRPCHook(long ptr)
 63 stdcall DllGetClassObject (ptr ptr ptr) OLE32_DllGetClassObject
 64 stub DllGetClassObjectWOW
 65 stdcall DoDragDrop(ptr ptr long ptr)
 66 stub EnableHookObject
 67 stdcall GetClassFile(wstr ptr)
 68 stdcall GetConvertStg(ptr)
 69 stub GetDocumentBitStg
 70 stdcall GetHGlobalFromILockBytes(ptr ptr)
 71 stdcall GetHGlobalFromStream(ptr ptr)
 72 stub GetHookInterface
 73 stdcall GetRunningObjectTable(long ptr)
 74 stdcall IIDFromString(wstr ptr) CLSIDFromString
 75 stdcall IsAccelerator(long long ptr long)
 76 stdcall IsEqualGUID(ptr ptr)
 77 stub IsValidIid
 78 stdcall IsValidInterface(ptr)
 79 stub IsValidPtrIn
 80 stub IsValidPtrOut
 81 stdcall MkParseDisplayName(ptr ptr ptr ptr)
 82 stdcall MonikerCommonPrefixWith(ptr ptr ptr)
 83 stub MonikerRelativePathTo
 84 stdcall OleBuildVersion()
 85 stdcall OleConvertIStorageToOLESTREAM(ptr ptr)
 86 stub OleConvertIStorageToOLESTREAMEx
 87 stdcall OleConvertOLESTREAMToIStorage(ptr ptr ptr)
 88 stub OleConvertOLESTREAMToIStorageEx
 89 stdcall OleCreate(ptr ptr long ptr ptr ptr ptr)
 90 stdcall OleCreateDefaultHandler(ptr ptr ptr ptr)
 91 stub OleCreateEmbeddingHelper
 92 stdcall OleCreateFromData(ptr ptr long ptr ptr ptr ptr)
 93 stdcall OleCreateFromFile(ptr ptr ptr long ptr ptr ptr ptr)
 94 stdcall OleCreateLink(ptr ptr long ptr ptr ptr ptr)
 95 stdcall OleCreateLinkFromData(ptr ptr long ptr ptr ptr ptr)
 96 stdcall OleCreateLinkToFile(ptr ptr long ptr ptr ptr ptr)
 97 stdcall OleCreateMenuDescriptor(long ptr)
 98 stdcall OleCreateStaticFromData(ptr ptr long ptr ptr ptr ptr)
 99 stdcall OleDestroyMenuDescriptor(long)
100 stdcall OleDoAutoConvert(ptr ptr)
101 stdcall OleDraw(ptr long long ptr)
102 stdcall OleDuplicateData(long long long)
103 stdcall OleFlushClipboard()
104 stdcall OleGetAutoConvert(ptr ptr)
105 stdcall OleGetClipboard(ptr)
106 stdcall OleGetIconOfClass(ptr ptr long)
107 stub OleGetIconOfFile
108 stdcall OleInitialize(ptr)
109 stdcall OleInitializeWOW(long)
110 stdcall OleIsCurrentClipboard(ptr)
111 stdcall OleIsRunning(ptr)
112 stdcall OleLoad(ptr ptr ptr ptr)
113 stdcall OleLoadFromStream(ptr ptr ptr)
114 stdcall OleLockRunning(ptr long long)
115 stdcall OleMetafilePictFromIconAndLabel(long ptr ptr long)
116 stub OleNoteObjectVisible
117 stdcall OleQueryCreateFromData(ptr)
118 stdcall OleQueryLinkFromData(ptr)
119 stdcall OleRegEnumFormatEtc(ptr long ptr)
120 stdcall OleRegEnumVerbs(long ptr)
121 stdcall OleRegGetMiscStatus(ptr long ptr)
122 stdcall OleRegGetUserType(long long ptr)
123 stdcall OleRun(ptr)
124 stdcall OleSave(ptr ptr long)
125 stdcall OleSaveToStream(ptr ptr)
126 stdcall OleSetAutoConvert(ptr ptr)
127 stdcall OleSetClipboard(ptr)
128 stdcall OleSetContainedObject(ptr long)
129 stdcall OleSetMenuDescriptor(long long long ptr ptr)
130 stdcall OleTranslateAccelerator(ptr ptr ptr)
131 stdcall OleUninitialize()
132 stub OpenOrCreateStream
133 stdcall ProgIDFromCLSID(ptr ptr)
134 stdcall ReadClassStg(ptr ptr)
135 stdcall ReadClassStm(ptr ptr)
136 stdcall ReadFmtUserTypeStg(ptr ptr ptr)
137 stub ReadOleStg
138 stub ReadStringStream
139 stdcall RegisterDragDrop(long ptr)
140 stdcall ReleaseStgMedium(ptr)
141 stdcall RevokeDragDrop(long)
142 stdcall SetConvertStg(ptr long)
143 stub SetDocumentBitStg
144 stdcall StgCreateDocfile(wstr long long ptr)
145 stdcall StgCreateDocfileOnILockBytes(ptr long long ptr)
146 stdcall StgIsStorageFile(wstr)
147 stdcall StgIsStorageILockBytes(ptr)
148 stdcall StgOpenStorage(wstr ptr long ptr long ptr)
149 stdcall StgOpenStorageOnILockBytes(ptr ptr long long long ptr)
150 stdcall StgSetTimes(wstr ptr ptr ptr )
151 stdcall StringFromCLSID(ptr ptr)
152 stdcall StringFromGUID2(ptr ptr long)
153 stdcall StringFromIID(ptr ptr) StringFromCLSID
154 stub UtConvertDvtd16toDvtd32
155 stub UtConvertDvtd32toDvtd16
156 stub UtGetDvtd16Info
157 stub UtGetDvtd32Info
158 stdcall WriteClassStg(ptr ptr)
159 stdcall WriteClassStm(ptr ptr)
160 stdcall WriteFmtUserTypeStg(ptr long ptr)
161 stub WriteOleStg
162 stub WriteStringStream
163 stdcall CoInitializeEx(ptr long)
164 stdcall CoInitializeSecurity(ptr long ptr ptr long long ptr long ptr)
165 stdcall CoCreateInstanceEx(ptr ptr long ptr long ptr)
166 stdcall PropVariantClear(ptr)
167 stub CoCopyProxy                # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
168 stub CoGetCallContext           # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
169 stub CoGetInstanceFromFile      # stdcall (ptr ptr ptr long wstr long ptr) return 0,ERR_NOTIMPLEMENTED
170 stub CoGetInstanceFromIStorage  # stdcall (ptr ptr ptr long ptr long ptr) return 0,ERR_NOTIMPLEMENTED
171 stub CoRegisterPSClsid          # stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
172 stub CoReleaseServerProcess     # stdcall () return 0,ERR_NOTIMPLEMENTED
173 stdcall CoResumeClassObjects()
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
192 stdcall CreateErrorInfo(ptr)
193 stub CreateObjrefMoniker
194 stdcall DllRegisterServer() OLE32_DllRegisterServer
195 stdcall FreePropVariantArray(long ptr)
196 stdcall GetErrorInfo(long ptr)
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
246 stdcall PropVariantCopy(ptr ptr)
247 stub SNB_UserFree
248 stub SNB_UserMarshal
249 stub SNB_UserSize
250 stub SNB_UserUnmarshal
251 stub STGMEDIUM_UserFree
252 stub STGMEDIUM_UserMarshal
253 stub STGMEDIUM_UserSize
254 stub STGMEDIUM_UserUnmarshal
255 stdcall SetErrorInfo(long ptr)
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
@ stdcall DllUnregisterServer() OLE32_DllUnregisterServer
