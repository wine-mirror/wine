# I'm just using "long" instead of "ptr" for the interface pointers,
# because they are 32-bit pointers, not converted to 16-bit format,
# but the app doesn't really need to know, it should never need to
# dereference the interface pointer itself (if we're lucky)...

#1   stub     WEP
2   stub     DLLGETCLASSOBJECT
3   stub     DLLCANUNLOADNOW
4   stub     ___EXPORTEDSTUB
10  variable _IID_IAVIStream(0x00020021 0x00000000 0x000000c0 0x46000000)
11  variable _IID_IAVIFile(0x00020020 0x00000000 0x000000c0 0x46000000)
12  variable _IID_IAVIEditStream(0x00020024 0x00000000 0x000000c0 0x46000000)
13  variable _IID_IGetFrame(0x00020023 0x00000000 0x000000c0 0x46000000)
14  variable _CLSID_AVISimpleUnMarshal(0x00020009 0x00000000 0x000000c0 0x46000000)
100 pascal   AVIFileInit() AVIFileInit
101 pascal   AVIFileExit() AVIFileExit
102 pascal   AVIFileOpen(ptr str word ptr) AVIFileOpenA
103 pascal   AVIStreamOpenFromFile(ptr str long long word ptr) AVIStreamOpenFromFileA
104 pascal   AVIStreamCreate(ptr long long ptr) AVIStreamCreate
105 stub     AVIMAKECOMPRESSEDSTREAM
106 stub     AVIMAKEFILEFROMSTREAMS
107 stub     AVIMAKESTREAMFROMCLIPBOARD
110 pascal   AVIStreamGetFrame(long long) AVIStreamGetFrame16
111 pascal   AVIStreamGetFrameClose(long) AVIStreamGetFrameClose16
112 pascal   AVIStreamGetFrameOpen(long ptr) AVIStreamGetFrameOpen16
120 stub     _AVISAVE
121 stub     AVISAVEV
122 stub     AVISAVEOPTIONS
123 pascal   AVIBuildFilter(str long word) AVIBuildFilterA
124 pascal   AVISaveOptionsFree(word ptr) AVISaveOptionsFree
130 pascal   AVIStreamStart(long) AVIStreamStart
131 pascal   AVIStreamLength(long) AVIStreamLength
132 pascal   AVIStreamTimeToSample(long long) AVIStreamTimeToSample
133 pascal   AVIStreamSampleToTime(long long) AVIStreamSampleToTime
140 pascal   AVIFileAddRef(long) AVIFileAddRef
141 pascal   AVIFileRelease(long) AVIFileRelease
142 pascal   AVIFileInfo(long ptr long) AVIFileInfoA
143 pascal   AVIFileGetStream(long ptr long long) AVIFileGetStream
144 pascal   AVIFileCreateStream(long ptr ptr) AVIFileCreateStream16
146 pascal   AVIFileWriteData(long long ptr long) AVIFileWriteData
147 pascal   AVIFileReadData(long long ptr ptr) AVIFileReadData
148 pascal   AVIFileEndRecord(long) AVIFileEndRecord
160 pascal   AVIStreamAddRef(long) AVIStreamAddRef
161 pascal   AVIStreamRelease(long) AVIStreamRelease
162 pascal   AVIStreamInfo(long ptr long) AVIStreamInfo16
163 pascal   AVIStreamFindSample(long long long) AVIStreamFindSample
164 pascal   AVIStreamReadFormat(long long ptr ptr) AVIStreamReadFormat
165 pascal   AVIStreamReadData(long long ptr ptr) AVIStreamReadData
166 pascal   AVIStreamWriteData(long long ptr long) AVIStreamWriteData
167 pascal   AVIStreamRead(long long long ptr long ptr ptr) AVIStreamRead
168 pascal   AVIStreamWrite(long long long ptr long long ptr ptr) AVIStreamWrite
169 pascal   AVIStreamSetFormat(long long ptr long) AVIStreamSetFormat
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
