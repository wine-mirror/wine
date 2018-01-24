1 stub @
@ stdcall CommandLineToArgvW(wstr ptr) shell32.CommandLineToArgvW
@ stub CreateRandomAccessStreamOnFile
@ stub CreateRandomAccessStreamOverStream
@ stub CreateStreamOverRandomAccessStream
@ stdcall -private DllCanUnloadNow() shell32.DllCanUnloadNow
@ stub DllGetActivationFactory
@ stdcall -private DllGetClassObject(ptr ptr ptr) shell32.DllGetClassObject
@ stdcall GetCurrentProcessExplicitAppUserModelID(ptr) shell32.GetCurrentProcessExplicitAppUserModelID
@ stdcall GetDpiForMonitor(long long ptr ptr)
@ stub GetDpiForShellUIComponent
@ stdcall GetProcessDpiAwareness(long ptr)
@ stub GetProcessReference
@ stub GetScaleFactorForDevice
@ stub GetScaleFactorForMonitor
@ stub IStream_Copy
@ stdcall IStream_Read(ptr ptr long) shlwapi.IStream_Read
@ stub IStream_ReadStr
@ stdcall IStream_Reset(ptr) shlwapi.IStream_Reset
@ stdcall IStream_Size(ptr ptr) shlwapi.IStream_Size
@ stdcall IStream_Write(ptr ptr long) shlwapi.IStream_Write
@ stub IStream_WriteStr
@ stdcall IUnknown_AtomicRelease(long) shlwapi.IUnknown_AtomicRelease
@ stdcall IUnknown_GetSite(ptr ptr ptr) shlwapi.IUnknown_GetSite
@ stdcall IUnknown_QueryService(ptr ptr ptr ptr) shlwapi.IUnknown_QueryService
@ stdcall IUnknown_Set(ptr ptr) shlwapi.IUnknown_Set
@ stdcall IUnknown_SetSite(ptr ptr) shlwapi.IUnknown_SetSite
@ stdcall IsOS(long) shlwapi.IsOS
@ stub RegisterScaleChangeEvent
@ stub RegisterScaleChangeNotifications
@ stub RevokeScaleChangeNotifications
@ stdcall SHAnsiToAnsi(str ptr long) shlwapi.SHAnsiToAnsi
@ stdcall SHAnsiToUnicode(str ptr long) shlwapi.SHAnsiToUnicode
@ stdcall SHCopyKeyA(long str long long) shlwapi.SHCopyKeyA
@ stdcall SHCopyKeyW(long wstr long long) shlwapi.SHCopyKeyW
@ stdcall SHCreateMemStream(ptr long) shlwapi.SHCreateMemStream
@ stdcall SHCreateStreamOnFileA(str long ptr) shlwapi.SHCreateStreamOnFileA
@ stdcall SHCreateStreamOnFileEx(wstr long long long ptr ptr) shlwapi.SHCreateStreamOnFileEx
@ stdcall SHCreateStreamOnFileW(wstr long ptr) shlwapi.SHCreateStreamOnFileW
@ stdcall SHCreateThread(ptr ptr long ptr) shlwapi.SHCreateThread
@ stdcall SHCreateThreadRef(ptr ptr) shlwapi.SHCreateThreadRef
@ stub SHCreateThreadWithHandle
@ stdcall SHDeleteEmptyKeyA(long ptr) shlwapi.SHDeleteEmptyKeyA
@ stdcall SHDeleteEmptyKeyW(long ptr) shlwapi.SHDeleteEmptyKeyW
@ stdcall SHDeleteKeyA(long str) shlwapi.SHDeleteKeyA
@ stdcall SHDeleteKeyW(long wstr) shlwapi.SHDeleteKeyW
@ stdcall SHDeleteValueA(long  str  str) shlwapi.SHDeleteValueA
@ stdcall SHDeleteValueW(long wstr wstr) shlwapi.SHDeleteValueW
@ stdcall SHEnumKeyExA(long long str ptr) shlwapi.SHEnumKeyExA
@ stdcall SHEnumKeyExW(long long wstr ptr) shlwapi.SHEnumKeyExW
@ stdcall SHEnumValueA(long long str ptr ptr ptr ptr) shlwapi.SHEnumValueA
@ stdcall SHEnumValueW(long long wstr ptr ptr ptr ptr) shlwapi.SHEnumValueW
@ stdcall SHGetThreadRef(ptr) shlwapi.SHGetThreadRef
@ stdcall SHGetValueA( long str str ptr ptr ptr ) shlwapi.SHGetValueA
@ stdcall SHGetValueW( long wstr wstr ptr ptr ptr ) shlwapi.SHGetValueW
@ stdcall SHOpenRegStream2A(long str str long) shlwapi.SHOpenRegStream2A
@ stdcall SHOpenRegStream2W(long wstr wstr long) shlwapi.SHOpenRegStream2W
@ stdcall SHOpenRegStreamA(long str str long) shlwapi.SHOpenRegStreamA
@ stdcall SHOpenRegStreamW(long wstr wstr long) shlwapi.SHOpenRegStreamW
@ stdcall SHQueryInfoKeyA(long ptr ptr ptr ptr) shlwapi.SHQueryInfoKeyA
@ stdcall SHQueryInfoKeyW(long ptr ptr ptr ptr) shlwapi.SHQueryInfoKeyW
@ stdcall SHQueryValueExA(long str ptr ptr ptr ptr) shlwapi.SHQueryValueExA
@ stdcall SHQueryValueExW(long wstr ptr ptr ptr ptr) shlwapi.SHQueryValueExW
@ stdcall SHRegDuplicateHKey(long) shlwapi.SHRegDuplicateHKey
@ stdcall SHRegGetIntW(ptr wstr long) shlwapi.SHRegGetIntW
@ stdcall SHRegGetPathA(long str str ptr long) shlwapi.SHRegGetPathA
@ stdcall SHRegGetPathW(long wstr wstr ptr long) shlwapi.SHRegGetPathW
@ stdcall SHRegGetValueA( long str str long ptr ptr ptr ) shlwapi.SHRegGetValueA
@ stdcall SHRegGetValueW( long wstr wstr long ptr ptr ptr ) shlwapi.SHRegGetValueW
@ stdcall SHRegSetPathA(long str str str long) shlwapi.SHRegSetPathA
@ stdcall SHRegSetPathW(long wstr wstr wstr long) shlwapi.SHRegSetPathW
@ stdcall SHReleaseThreadRef() shlwapi.SHReleaseThreadRef
@ stdcall SHSetThreadRef(ptr) shlwapi.SHSetThreadRef
@ stdcall SHSetValueA(long  str  str long ptr long) shlwapi.SHSetValueA
@ stdcall SHSetValueW(long wstr wstr long ptr long) shlwapi.SHSetValueW
@ stdcall SHStrDupA(str ptr) shlwapi.SHStrDupA
@ stdcall SHStrDupW(wstr ptr) shlwapi.SHStrDupW
@ stdcall SHUnicodeToAnsi(wstr ptr ptr) shlwapi.SHUnicodeToAnsi
@ stdcall SHUnicodeToUnicode(wstr ptr long) shlwapi.SHUnicodeToUnicode
@ stdcall SetCurrentProcessExplicitAppUserModelID(wstr) shell32.SetCurrentProcessExplicitAppUserModelID
@ stdcall SetProcessDpiAwareness(long)
@ stub SetProcessReference
@ stub UnregisterScaleChangeEvent

100 stub @
101 stub @
102 stub @
103 stub @
104 stub @
105 stub @
106 stub @
107 stub @
108 stub @
109 stub @
110 stub @
111 stub @
115 stub @
116 stub @
117 stub @
120 stub @
121 stub @
122 stub SHRegGetValueFromHKCUHKLM
123 stub @
124 stub @
125 stub @
126 stub @
127 stub @
130 stub @
131 stub @
132 stub @
133 stub @
140 stub @
141 stub @
142 stub @
143 stub @
144 stub @
145 stub @
150 stub @
151 stub @
152 stub @
153 stub @
160 stub @
161 stub @
162 stub @
170 stub @
171 stub @
172 stub @
173 stub @
181 stub @
182 stub @
183 stub @
184 stub @
185 stub @
186 stub @
187 stub @
188 stub @
190 stub @
191 stub @
192 stub @
193 stub @
200 stub @
220 stub @
221 stub @
222 stub @
223 stub @
224 stub @
225 stub @
226 stub @
227 stub @
228 stub @
230 stub @
231 stub @
232 stub @
233 stub @
234 stub @
240 stub @
241 stub @
242 stub @
243 stub @
244 stub @
245 stub @
246 stub @
247 stub @
248 stub @
250 stub @
251 stub @
252 stub @
253 stub @
254 stub @
255 stub @
260 stub @
261 stub @
270 stub @
280 stub @
281 stub @
282 stub @
283 stub @
284 stub @
290 stub @
291 stub @
292 stub @
