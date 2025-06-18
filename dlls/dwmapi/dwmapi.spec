100 stub DwmpDxGetWindowSharedSurface
101 stub DwmpDxUpdateWindowSharedSurface
102 stdcall DwmEnableComposition(long)
103 stub -noname DwmpRestartComposition
104 stub -noname DwmpSetColorizationColor
105 stub -noname DwmpStartOrStopFlip3D
106 stub -noname DwmpIsCompositionCapable
107 stub -noname DwmpGetGlobalState
108 stub -noname DwmpEnableRedirection
109 stub -noname DwmpOpenGraphicsStream
110 stub -noname DwmpCloseGraphicsStream
112 stub -noname DwmpSetGraphicsStreamTransformHint
113 stub -noname DwmpActivateLivePreview
114 stub -noname DwmpQueryThumbnailType
115 stub -noname DwmpStartupViaUserInit
118 stub -noname DwmpGetAssessment
119 stub -noname DwmpGetAssessmentUsage
120 stub -noname DwmpSetAssessmentUsage
121 stub -noname DwmpIsSessionDWM
124 stub -noname DwmpRegisterThumbnail
125 stub DwmpDxBindSwapChain
126 stub DwmpDxUnbindSwapChain
127 stdcall -noname DwmpGetColorizationParameters(ptr)
128 stub DwmpDxgiIsThreadDesktopComposited
129 stub -noname DwmpDxgiDisableRedirection
130 stub -noname DwmpDxgiEnableRedirection
131 stub -noname DwmpSetColorizationParameters
132 stub -noname DwmpGetCompositionTimingInfoEx
133 stub DwmpDxUpdateWindowRedirectionBltSurface
134 stub -noname DwmpDxSetContentHostingInformation
135 stub DwmpRenderFlick
136 stub DwmpAllocateSecurityDescriptor
137 stub DwmpFreeSecurityDescriptor
138 stub @
139 stub @
140 stub @
141 stub @
142 stub @
143 stub DwmpEnableDDASupport
144 stub @
145 stub @
146 stub @
147 stub @
148 stub @
150 stub @
151 stub @
152 stub @
153 stub @
154 stub @
155 stub @
156 stub DwmTetherTextContact
157 stub @
158 stub @
159 stub @
160 stub @
161 stub @
162 stub @
163 stub @
164 stub @

# @ stdcall -private DllCanUnloadNow()
# @ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall DwmAttachMilContent(long)
@ stdcall DwmDefWindowProc(long long long long ptr)
@ stdcall DwmDetachMilContent(long)
@ stdcall DwmEnableBlurBehindWindow(ptr ptr)
@ stdcall DwmEnableMMCSS(long)
@ stdcall DwmExtendFrameIntoClientArea(long ptr)
@ stdcall DwmFlush()
@ stdcall DwmGetColorizationColor(ptr ptr)
@ stdcall DwmGetCompositionTimingInfo(long ptr)
@ stdcall DwmGetGraphicsStreamClient(long ptr)
@ stdcall DwmGetGraphicsStreamTransformHint(long ptr)
@ stdcall DwmGetTransportAttributes(ptr ptr ptr)
@ stdcall DwmGetWindowAttribute(ptr long ptr long)
@ stdcall DwmInvalidateIconicBitmaps(ptr)
@ stdcall DwmIsCompositionEnabled(ptr)
@ stub DwmModifyPreviousDxFrameDuration
@ stub DwmQueryThumbnailSourceSize
@ stdcall DwmRegisterThumbnail(long long ptr)
# @ stub DwmRenderGesture
@ stub DwmSetDxFrameDuration
@ stdcall DwmSetIconicLivePreviewBitmap(long long ptr long)
@ stdcall DwmSetIconicThumbnail(long long long)
@ stdcall DwmSetPresentParameters(ptr ptr)
@ stdcall DwmSetWindowAttribute(long long ptr long)
# @ stub DwmShowContact
# @ stub DwmTetherContact
# @ stub DwmTransitionOwnedWindow
@ stdcall DwmUnregisterThumbnail(long)
@ stdcall DwmUpdateThumbnailProperties(ptr ptr)
