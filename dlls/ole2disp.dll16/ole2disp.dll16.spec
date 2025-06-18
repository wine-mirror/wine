1 stub DLLGETCLASSOBJECT
2 pascal SysAllocString(str)		SysAllocString16
3 pascal SysReallocString(ptr str)	SysReAllocString16
4 pascal SysAllocStringLen(str word)	SysAllocStringLen16
5 pascal SysReAllocStringLen(ptr str word) SysReAllocStringLen16
6 pascal SysFreeString(segstr)		SysFreeString16
7 pascal SysStringLen(segstr)		SysStringLen16
8 pascal VariantInit(ptr) VariantInit16
9 stub VARIANTCLEAR
10 stub VARIANTCOPY
11 stub VARIANTCOPYIND
12 pascal VariantChangeType(ptr ptr word word) VariantChangeType16
13 stub VARIANTTIMETODOSDATETIME
14 stub DOSDATETIMETOVARIANTTIME
15 stub SAFEARRAYCREATE
16 stub SAFEARRAYDESTROY
17 pascal -ret16 SafeArrayGetDim(ptr) SafeArrayGetDim16
18 pascal -ret16 SafeArrayGetElemsize(ptr) SafeArrayGetElemsize16
19 pascal SafeArrayGetUBound(ptr word ptr) SafeArrayGetUBound16
20 pascal SafeArrayGetLBound(ptr word ptr) SafeArrayGetLBound16
21 pascal SafeArrayLock(ptr) SafeArrayLock16
22 pascal SafeArrayUnlock(ptr) SafeArrayUnlock16
23 pascal SafeArrayAccessData(ptr ptr) SafeArrayAccessData16
24 pascal SafeArrayUnaccessData(ptr) SafeArrayUnaccessData16
25 stub SAFEARRAYGETELEMENT
26 stub SAFEARRAYPUTELEMENT
27 stub SAFEARRAYCOPY
28 stub DISPGETPARAM
29 stub DISPGETIDSOFNAMES
30 stub DISPINVOKE
31 pascal CreateDispTypeInfo(ptr long ptr) CreateDispTypeInfo16
32 pascal CreateStdDispatch(ptr ptr ptr ptr) CreateStdDispatch16
33 variable _IID_IDispatch(0x00020400 0x00000000 0x000000c0 0x46000000)
34 variable _IID_IEnumVARIANT(0x00020404 0x00000000 0x000000c0 0x46000000)
35 pascal RegisterActiveObject(ptr ptr long ptr) RegisterActiveObject16
36 stub REVOKEACTIVEOBJECT
37 stub GETACTIVEOBJECT
38 pascal SafeArrayAllocDescriptor(word ptr) SafeArrayAllocDescriptor16
39 pascal SafeArrayAllocData(ptr) SafeArrayAllocData16
40 pascal SafeArrayDestroyDescriptor(segptr) SafeArrayDestroyDescriptor16
41 pascal SafeArrayDestroyData(ptr) SafeArrayDestroyData16
42 stub SAFEARRAYREDIM
43 stub VARI2FROMI4
44 stub VARI2FROMR4
45 stub VARI2FROMR8
46 stub VARI2FROMCY
47 stub VARI2FROMDATE
48 stub VARI2FROMSTR
49 stub VARI2FROMDISP
50 stub VARI2FROMBOOL
51 stub VARI4FROMI2
52 stub VARI4FROMR4
53 stub VARI4FROMR8
54 stub VARI4FROMCY
55 stub VARI4FROMDATE
56 stub VARI4FROMSTR
57 stub VARI4FROMDISP
58 stub VARI4FROMBOOL
59 stub VARR4FROMI2
60 stub VARR4FROMI4
61 stub VARR4FROMR8
62 stub VARR4FROMCY
63 stub VARR4FROMDATE
64 stub VARR4FROMSTR
65 stub VARR4FROMDISP
66 stub VARR4FROMBOOL
67 stub VARR8FROMI2
68 stub VARR8FROMI4
69 stub VARR8FROMR4
70 stub VARR8FROMCY
71 stub VARR8FROMDATE
72 stub VARR8FROMSTR
73 stub VARR8FROMDISP
74 stub VARR8FROMBOOL
75 stub VARDATEFROMI2
76 stub VARDATEFROMI4
77 stub VARDATEFROMR4
78 stub VARDATEFROMR8
79 stub VARDATEFROMCY
80 stub VARDATEFROMSTR
81 stub VARDATEFROMDISP
82 stub VARDATEFROMBOOL
83 stub VARCYFROMI2
84 stub VARCYFROMI4
85 stub VARCYFROMR4
86 stub VARCYFROMR8
87 stub VARCYFROMDATE
88 stub VARCYFROMSTR
89 stub VARCYFROMDISP
90 stub VARCYFROMBOOL
91 stub VARBSTRFROMI2
92 stub VARBSTRFROMI4
93 stub VARBSTRFROMR4
94 stub VARBSTRFROMR8
95 stub VARBSTRFROMCY
96 stub VARBSTRFROMDATE
97 stub VARBSTRFROMDISP
98 stub VARBSTRFROMBOOL
99 stub VARBOOLFROMI2
100 stub VARBOOLFROMI4
101 stub VARBOOLFROMR4
102 stub VARBOOLFROMR8
103 stub VARBOOLFROMDATE
104 stub VARBOOLFROMCY
105 stub VARBOOLFROMSTR
106 stub VARBOOLFROMDISP
107 stub DOINVOKEMETHOD
108 pascal VariantChangeTypeEx(ptr ptr long word word) VariantChangeTypeEx16
109 stub SAFEARRAYPTROFINDEX
110 pascal SetErrorInfo(long ptr) SetErrorInfo16
111 stub GETERRORINFO
112 stub CREATEERRORINFO
113 variable _IID_IErrorInfo(0x1cf2b120 0x101b547d 0x0008658e 0x19d12b2b)
114 variable _IID_ICreateErrorInfo(0x22f03340 0x101b547d 0x0008658e 0x19d12b2b)
115 variable _IID_ISupportErrorInfo(0xdf0b3d60 0x101b548f 0x0008658e 0x19d12b2b)
116 stub VARUI1FROMI2
117 stub VARUI1FROMI4
118 stub VARUI1FROMR4
119 stub VARUI1FROMR8
120 stub VARUI1FROMCY
121 stub VARUI1FROMDATE
122 stub VARUI1FROMSTR
123 stub VARUI1FROMDISP
124 stub VARUI1FROMBOOL
125 stub VARI2FROMUI1
126 stub VARI4FROMUI1
127 stub VARR4FROMUI1
128 stub VARR8FROMUI1
129 stub VARDATEFROMUI1
130 stub VARCYFROMUI1
131 stub VARBSTRFROMUI1
132 stub VARBOOLFROMUI1
133 stub DLLCANUNLOADNOW
#134 stub WEP
#135 stub ___EXPORTEDSTUB
