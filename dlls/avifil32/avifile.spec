# I'm just using "long" instead of "ptr" for the interface pointers,
# because they are 32-bit pointers, not converted to 16-bit format,
# but the app doesn't really need to know, it should never need to
# dereference the interface pointer itself (if we're lucky)...

1   stub     WEP
2   stub     DLLGETCLASSOBJECT
3   stub     DLLCANUNLOADNOW
4   stub     ___EXPORTEDSTUB
10  stub     _IID_IAVISTREAM
11  stub     _IID_IAVIFILE
12  stub     _IID_IAVIEDITSTREAM
13  stub     _IID_IGETFRAME
14  stub     _CLSID_AVISIMPLEUNMARSHAL
100 pascal   AVIFileInit() AVIFileInit
101 pascal   AVIFileExit() AVIFileExit
102 pascal   AVIFileOpen(ptr str word ptr) AVIFileOpenA
103 pascal   AVIStreamOpenFromFile(ptr str long word ptr) AVIStreamOpenFromFileA
104 pascal   AVIStreamCreate(ptr long long ptr) AVIStreamCreate
105 stub     AVIMAKECOMPRESSEDSTREAM
106 stub     AVIMAKEFILEFROMSTREAMS
107 stub     AVIMAKESTREAMFROMCLIPBOARD
110 stub     AVISTREAMGETFRAME
111 stub     AVISTREAMGETFRAMECLOSE
112 stub     AVISTREAMGETFRAMEOPEN
120 stub     _AVISAVE
121 stub     AVISAVEV
122 stub     AVISAVEOPTIONS
123 stub     AVIBUILDFILTER
124 stub     AVISAVEOPTIONSFREE
130 pascal   AVIStreamStart(long) AVIStreamStart
131 pascal   AVIStreamLength(long) AVIStreamLength
132 pascal   AVIStreamTimeToSample(long long) AVIStreamTimeToSample
133 pascal   AVIStreamSampleToTime(long long) AVIStreamSampleToTime
140 pascal   AVIFileAddRef(long) AVIFileAddRef
141 pascal   AVIFileRelease(long) AVIFileRelease
142 pascal   AVIFileInfo(long ptr long) AVIFileInfoA
143 pascal   AVIFileGetStream(long ptr long long) AVIFileGetStream
144 stub     AVIFILECREATESTREAM
146 stub     AVIFILEWRITEDATA
147 stub     AVIFILEREADDATA
148 pascal   AVIFileEndRecord(long) AVIFileEndRecord
160 pascal   AVIStreamAddRef(long) AVIStreamAddRef
161 pascal   AVIStreamRelease(long) AVIStreamRelease
162 stub     AVISTREAMINFO
163 stub     AVISTREAMFINDSAMPLE
164 stub     AVISTREAMREADFORMAT
165 stub     AVISTREAMREADDATA
166 stub     AVISTREAMWRITEDATA
167 stub     AVISTREAMREAD
168 stub     AVISTREAMWRITE
169 stub     AVISTREAMSETFORMAT
180 stub     EDITSTREAMCOPY
181 stub     EDITSTREAMCUT
182 stub     EDITSTREAMPASTE
184 stub     CREATEEDITABLESTREAM
185 stub     AVIPUTFILEONCLIPBOARD
187 stub     AVIGETFROMCLIPBOARD
188 stub     AVICLEARCLIPBOARD
190 stub     EDITSTREAMCLONE
191 stub     EDITSTREAMSETNAME
192 stub     EDITSTREAMSETINFO
200 stub     AVISTREAMBEGINSTREAMING
201 stub     AVISTREAMENDSTREAMING
