name msvideo
type win16

2 pascal VideoForWindowsVersion() VideoForWindowsVersion
20 stub VIDEOGETNUMDEVS
21 stub VIDEOGETERRORTEXT
22 pascal VIDEOCAPDRIVERDESCANDVER(word ptr word ptr word) VideoCapDriverDescAndVer
28 stub VIDEOOPEN
29 stub VIDEOCLOSE
30 stub VIDEODIALOG
31 stub VIDEOFRAME
32 stub VIDEOCONFIGURE
33 stub VIDEOCONFIGURESTORAGE
34 stub VIDEOGETCHANNELCAPS
35 stub VIDEOUPDATE
40 stub VIDEOSTREAMADDBUFFER
41 stub VIDEOSTREAMFINI
42 stub VIDEOSTREAMGETERROR
43 stub VIDEOSTREAMGETPOSITION
44 stub VIDEOSTREAMINIT
46 stub VIDEOSTREAMPREPAREHEADER
47 stub VIDEOSTREAMRESET
49 stub VIDEOSTREAMSTART
50 stub VIDEOSTREAMSTOP
51 stub VIDEOSTREAMUNPREPAREHEADER
52 stub VIDEOSTREAMALLOCHDRANDBUFFER
53 stub VIDEOSTREAMFREEHDRANDBUFFER
60 stub VIDEOMESSAGE
102 pascal16 DrawDibOpen() DrawDibOpen16
103 pascal16 DrawDibClose(word) DrawDibClose16
104 pascal16 DrawDibBegin(word word s_word s_word ptr s_word s_word word) DrawDibBegin16
105 pascal16 DrawDibEnd(word) DrawDibEnd16
106 pascal16 DrawDibDraw(word word s_word s_word s_word s_word ptr ptr s_word s_word s_word s_word word) DrawDibDraw16
108 pascal16 DrawDibGetPalette(word) DrawDibGetPalette16
110 pascal16 DrawDibSetPalette(word word) DrawDibSetPalette16
111 stub DRAWDIBCHANGEPALETTE
112 pascal16 DrawDibRealize(word word word) DrawDibRealize16
113 stub DRAWDIBTIME
114 stub DRAWDIBPROFILEDISPLAY
115 stub STRETCHDIB
118 pascal16 DrawDibStart(word long) DrawDibStart16
119 pascal16 DrawDibStop(word) DrawDibStop16
120 stub DRAWDIBGETBUFFER
200 pascal16 ICInfo(long long segptr) ICInfo16
201 stub ICINSTALL
202 stub ICREMOVE
203 pascal16 ICOpen(long long word) ICOpen16
204 pascal ICClose(word) ICClose16
205 pascal ICSendMessage(word word long long) ICSendMessage16
206 pascal16 ICOpenFunction(long long word segptr) ICOpenFunction16
207 pascal _ICMessage() ICMessage16
212 pascal ICGetInfo(word segptr long) ICGetInfo16
213 pascal16 ICLocate(long long ptr ptr word) ICLocate16
224 cdecl _ICCompress(word long segptr segptr segptr segptr segptr segptr long long long segptr segptr) ICCompress16
230 cdecl _ICDecompress(word long segptr segptr segptr segptr) ICDecompress16
232 cdecl _ICDrawBegin(word long word word word s_word s_word s_word s_word segptr s_word s_word s_word s_word long long) ICDrawBegin16
234 cdecl _ICDraw(word long segptr segptr long long) ICDraw16
239 pascal16 ICGetDisplayFormat(word ptr ptr s_word s_word s_word) ICGetDisplayFormat16
240 stub ICIMAGECOMPRESS
241 stub ICIMAGEDECOMPRESS
242 stub ICCOMPRESSORCHOOSE
243 stub ICCOMPRESSORFREE
244 stub ICSEQCOMPRESSFRAMESTART
245 stub ICSEQCOMPRESSFRAMEEND
246 stub ICSEQCOMPRESSFRAME
250 stub _MCIWNDCREATE
251 stub _MCIWNDREGISTERCLASS
252 stub GETOPENFILENAMEPREVIEW
253 stub GETSAVEFILENAMEPREVIEW
