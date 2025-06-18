# ordinal exports
101 stdcall -noname IEWinMain(str long)
102 stub -noname CreateShortcutInDirA
103 stub -noname CreateShortcutInDirW
104 stdcall -noname WhichPlatform() shlwapi.WhichPlatform
105 stub -noname CreateShortcutInDirEx
106 stub HlinkFindFrame
107 stub SetShellOfflineState
108 stub AddUrlToFavorites
110 stdcall -noname WinList_Init()
111 stub -noname WinList_Terminate
115 stub -noname CreateFromDesktop
116 stub -noname DDECreatePostNotify
117 stub -noname DDEHandleViewFolderNotify
118 stdcall -noname ShellDDEInit(long)
119 stub -noname SHCreateDesktop
120 stub -noname SHDesktopMessageLoop
121 stdcall -noname StopWatchModeFORWARD()
122 stdcall -noname StopWatchFlushFORWARD()
123 stdcall -noname StopWatchAFORWARD(long str long long long)
124 stdcall -noname StopWatchWFORWARD(long wstr long long long)
125 stdcall -noname RunInstallUninstallStubs()
130 stdcall -noname RunInstallUninstallStubs2(long)
131 stub -noname SHCreateSplashScreen
135 stub -noname IsFileUrl
136 stub -noname IsFileUrlW
137 stub -noname PathIsFilePath
138 stub -noname URLSubLoadString
139 stub -noname OpenPidlOrderStream
140 stub -noname DragDrop
141 stub -noname IEInvalidateImageList
142 stub -noname IEMapPIDLToSystemImageListIndex
143 stub -noname ILIsWeb
145 stub -noname IEGetAttributesOf
146 stub -noname IEBindToObject
147 stub -noname IEGetNameAndFlags
148 stub -noname IEGetDisplayName
149 stub -noname IEBindToObjectEx
150 stub -noname _GetStdLocation
151 stub -noname URLSubRegQueryA(str str long ptr long long)
152 stub -noname CShellUIHelper_CreateInstance2
153 stub -noname IsURLChild
158 stdcall -noname SHRestricted2A(long str long)
159 stdcall -noname SHRestricted2W(long wstr long)
160 stub -noname SHIsRestricted2W
161 stub @ # CSearchAssistantOC::OnDraw
162 stub -noname CDDEAuto_Navigate
163 stub SHAddSubscribeFavorite
164 stdcall -noname ResetProfileSharing(long)
165 stub -noname URLSubstitution
167 stub -noname IsIEDefaultBrowser
169 stub -noname ParseURLFromOutsideSourceA(str ptr ptr ptr)
170 stub -noname ParseURLFromOutsideSourceW(wstr ptr ptr ptr)
171 stub -noname _DeletePidlDPA
172 stub -noname IURLQualify
173 stub -noname SHIsRestricted
174 stub -noname SHIsGlobalOffline
175 stub -noname DetectAndFixAssociations
176 stub -noname EnsureWebViewRegSettings
177 stub -noname WinList_NotifyNewLocation
178 stub -noname WinList_FindFolderWindow
179 stub -noname WinList_GetShellWindows
180 stub -noname WinList_RegisterPending
181 stub -noname WinList_Revoke
183 stub -noname SHMapNbspToSp
185 stub -noname FireEvent_Quit
187 stub -noname SHDGetPageLocation
188 stub -noname SHIEErrorMsgBox
189 stub @ # FIXME: same as ordinal 148
190 stub -noname SHRunIndirectRegClientCommandForward
191 stub -noname SHIsRegisteredClient
192 stub -noname SHGetHistoryPIDL
194 stub -noname IECleanUpAutomationObject
195 stub -noname IEOnFirstBrowserCreation
196 stub -noname IEDDE_WindowDestroyed
197 stub -noname IEDDE_NewWindow
198 stub -noname IsErrorUrl
199 stub @
200 stub -noname SHGetViewStream
203 stub -noname NavToUrlUsingIEA
204 stub -noname NavToUrlUsingIEW
208 stub -noname SearchForElementInHead
209 stub -noname JITCoCreateInstance
210 stub -noname UrlHitsNetW
211 stub -noname ClearAutoSuggestForForms
212 stub -noname GetLinkInfo
213 stub -noname UseCustomInternetSearch
214 stub -noname GetSearchAssistantUrlW
215 stub -noname GetSearchAssistantUrlA
216 stub -noname GetDefaultInternetSearchUrlW
217 stub -noname GetDefaultInternetSearchUrlA
218 stdcall -noname IEParseDisplayNameWithBCW(long wstr ptr ptr)
219 stub -noname IEILIsEqual
220 stub @
221 stub -noname IECreateFromPathCPWithBCA
222 stub -noname IECreateFromPathCPWithBCW
223 stub -noname ResetWebSettings
224 stub -noname IsResetWebSettingsRequired
225 stub -noname PrepareURLForDisplayUTF8W
226 stub -noname IEIsLinkSafe
227 stub -noname SHUseClassicToolbarGlyphs
228 stub -noname SafeOpenPromptForShellExec
229 stub -noname SafeOpenPromptForPackager

@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllGetVersion(ptr)
@ stdcall -private DllInstall(long wstr)
@ stdcall -private DllRegisterServer()
@ stub DllRegisterWindowClasses
@ stdcall -private DllUnregisterServer()
@ stub DoAddToFavDlg
@ stub DoAddToFavDlgW
@ stdcall DoFileDownload(wstr)
@ stub DoFileDownloadEx
@ stdcall DoOrganizeFavDlg(long str)
@ stdcall DoOrganizeFavDlgW(long wstr)
@ stub DoPrivacyDlg
@ stub HlinkFrameNavigate
@ stub HlinkFrameNavigateNHL
@ stub IEAboutBox
@ stub IEWriteErrorLog
@ stdcall ImportPrivacySettings(wstr ptr ptr)
@ stdcall InstallReg_RunDLL(long long str long)
@ stdcall OpenURL(long long str long) ieframe.OpenURL
@ stub SHGetIDispatchForFolder
@ stdcall SetQueryNetSessionCount(long)
@ stub SoftwareUpdateMessageBox
@ stub URLQualifyA
@ stub URLQualifyW
