1 stdcall DllCanUnloadNow() MSI_DllCanUnloadNow
2 stub DllGetClassObject
3 stub DllRegisterServer
4 stub DllUnregisterServer
5 stub MsiAdvertiseProductA
6 stub MsiAdvertiseProductW
7 stdcall MsiCloseAllHandles() MsiCloseAllHandles
8 stdcall MsiCloseHandle(long) MsiCloseHandle
9 stub MsiCollectUserInfoA
10 stub MsiCollectUserInfoW
11 stub MsiConfigureFeatureA
12 stub MsiConfigureFeatureFromDescriptorA
13 stub MsiConfigureFeatureFromDescriptorW
14 stub MsiConfigureFeatureW
15 stdcall MsiConfigureProductA(str long long) MsiConfigureProductA
16 stdcall MsiConfigureProductW(wstr long long) MsiConfigureProductW
17 stdcall MsiCreateRecord(long) MsiCreateRecord
18 stdcall MsiDatabaseApplyTransformA(long str long) MsiDatabaseApplyTransformA
19 stdcall MsiDatabaseApplyTransformW(long wstr long) MsiDatabaseApplyTransformW
20 stdcall MsiDatabaseCommit(long) MsiDatabaseCommit
21 stub MsiDatabaseExportA
22 stub MsiDatabaseExportW
23 stdcall MsiDatabaseGenerateTransformA(long long str long long) MsiDatabaseGenerateTransformA
24 stdcall MsiDatabaseGenerateTransformW(long long wstr long long) MsiDatabaseGenerateTransformW
25 stdcall MsiDatabaseGetPrimaryKeysA(long str ptr) MsiDatabaseGetPrimaryKeysA
26 stdcall MsiDatabaseGetPrimaryKeysW(long wstr ptr) MsiDatabaseGetPrimaryKeysW
27 stdcall MsiDatabaseImportA(str str) MsiDatabaseImportA
28 stdcall MsiDatabaseImportW(wstr wstr) MsiDatabaseImportW
29 stub MsiDatabaseMergeA
30 stub MsiDatabaseMergeW
31 stdcall MsiDatabaseOpenViewA(str ptr) MsiDatabaseOpenViewA
32 stdcall MsiDatabaseOpenViewW(wstr ptr) MsiDatabaseOpenViewW
33 stdcall MsiDoActionA(long str) MsiDoActionA
34 stdcall MsiDoActionW(long wstr) MsiDoActionW
35 stub MsiEnableUIPreview
36 stdcall MsiEnumClientsA(long ptr) MsiEnumClientsA
37 stdcall MsiEnumClientsW(long ptr) MsiEnumClientsW
38 stub MsiEnumComponentQualifiersA
39 stub MsiEnumComponentQualifiersW
40 stdcall MsiEnumComponentsA(long ptr) MsiEnumComponentsA
41 stdcall MsiEnumComponentsW(long ptr) MsiEnumComponentsW
42 stdcall MsiEnumFeaturesA(str long ptr ptr) MsiEnumFeaturesA
43 stdcall MsiEnumFeaturesW(wstr long ptr ptr) MsiEnumFeaturesW
44 stdcall MsiEnumProductsA(long ptr) MsiEnumProductsA
45 stdcall MsiEnumProductsW(long ptr) MsiEnumProductsW
46 stub MsiEvaluateConditionA
47 stub MsiEvaluateConditionW
48 stub MsiGetLastErrorRecord
49 stub MsiGetActiveDatabase
50 stub MsiGetComponentStateA
51 stub MsiGetComponentStateW
52 stub MsiGetDatabaseState
53 stub MsiGetFeatureCostA
54 stub MsiGetFeatureCostW
55 stub MsiGetFeatureInfoA
56 stub MsiGetFeatureInfoW
57 stub MsiGetFeatureStateA
58 stub MsiGetFeatureStateW
59 stub MsiGetFeatureUsageA
60 stub MsiGetFeatureUsageW
61 stub MsiGetFeatureValidStatesA
62 stub MsiGetFeatureValidStatesW
63 stub MsiGetLanguage
64 stub MsiGetMode
65 stdcall MsiGetProductCodeA(str str) MsiGetProductCodeA
66 stdcall MsiGetProductCodeW(wstr wstr) MsiGetProductCodeW
67 stdcall MsiGetProductInfoA(str str str long) MsiGetProductInfoA
68 stub MsiGetProductInfoFromScriptA
69 stub MsiGetProductInfoFromScriptW
70 stdcall MsiGetProductInfoW(wstr wstr wstr long) MsiGetProductInfoW
71 stub MsiGetProductPropertyA
72 stub MsiGetProductPropertyW
73 stub MsiGetPropertyA
74 stub MsiGetPropertyW
75 stub MsiGetSourcePathA
76 stub MsiGetSourcePathW
77 stdcall MsiGetSummaryInformationA(str long ptr) MsiGetSummaryInformationA
78 stdcall MsiGetSummaryInformationW(wstr long ptr) MsiGetSummaryInformationW
79 stub MsiGetTargetPathA
80 stub MsiGetTargetPathW
81 stub MsiGetUserInfoA
82 stub MsiGetUserInfoW
83 stub MsiInstallMissingComponentA
84 stub MsiInstallMissingComponentW
85 stub MsiInstallMissingFileA
86 stub MsiInstallMissingFileW
87 stdcall MsiInstallProductA(str str) MsiInstallProductA
88 stdcall MsiInstallProductW(wstr wstr) MsiInstallProductW
89 stub MsiLocateComponentA
90 stub MsiLocateComponentW
91 stdcall MsiOpenDatabaseA(str str ptr) MsiOpenDatabaseA
92 stdcall MsiOpenDatabaseW(wstr wstr ptr) MsiOpenDatabaseW
93 stdcall MsiOpenPackageA(str ptr) MsiOpenPackageA
94 stdcall MsiOpenPackageW(wstr ptr) MsiOpenPackageW
95 stdcall MsiOpenProductA(str ptr) MsiOpenProductA
96 stdcall MsiOpenProductW(wstr ptr) MsiOpenProductW
97 stub MsiPreviewBillboardA
98 stub MsiPreviewBillboardW
99 stub MsiPreviewDialogA
100 stub MsiPreviewDialogW
101 stub MsiProcessAdvertiseScriptA
102 stub MsiProcessAdvertiseScriptW
103 stub MsiProcessMessage
104 stub MsiProvideComponentA
105 stdcall MsiProvideComponentFromDescriptorA(str ptr ptr ptr) MsiProvideComponentFromDescriptorA
106 stdcall MsiProvideComponentFromDescriptorW(wstr ptr ptr ptr) MsiProvideComponentFromDescriptorW
107 stub MsiProvideComponentW
108 stub MsiProvideQualifiedComponentA
109 stub MsiProvideQualifiedComponentW
110 stub MsiQueryFeatureStateA
111 stub MsiQueryFeatureStateW
112 stdcall MsiQueryProductStateA(str) MsiQueryProductStateA
113 stdcall MsiQueryProductStateW(wstr) MsiQueryProductStateW
114 stdcall MsiRecordDataSize(long long) MsiRecordDataSize
115 stdcall MsiRecordGetFieldCount(long) MsiRecordGetFieldCount
116 stdcall MsiRecordGetInteger(long long) MsiRecordGetInteger
117 stdcall MsiRecordGetStringA(long long ptr ptr) MsiRecordGetStringA
118 stdcall MsiRecordGetStringW(long long ptr ptr) MsiRecordGetStringW
119 stdcall MsiRecordIsNull(long long) MsiRecordIsNull
120 stdcall MsiRecordReadStream(long long ptr ptr) MsiRecordReadStream
121 stdcall MsiRecordSetInteger(long long long) MsiRecordSetInteger
122 stdcall MsiRecordSetStreamA(long long str) MsiRecordSetStreamA
123 stdcall MsiRecordSetStreamW(long long wstr) MsiRecordSetStreamW
124 stdcall MsiRecordSetStringA(long long str) MsiRecordSetStringA
125 stdcall MsiRecordSetStringW(long long wstr) MsiRecordSetStringW
126 stub MsiReinstallFeatureA
127 stub MsiReinstallFeatureFromDescriptorA
128 stub MsiReinstallFeatureFromDescriptorW
129 stub MsiReinstallFeatureW
130 stub MsiReinstallProductA
131 stub MsiReinstallProductW
132 stub MsiSequenceA
133 stub MsiSequenceW
134 stub MsiSetComponentStateA
135 stub MsiSetComponentStateW
136 stub MsiSetExternalUIA
137 stub MsiSetExternalUIW
138 stub MsiSetFeatureStateA
139 stub MsiSetFeatureStateW
140 stub MsiSetInstallLevel
141 stdcall MsiSetInternalUI(long ptr) MsiSetInternalUI
142 stub MsiVerifyDiskSpace
143 stub MsiSetMode
144 stub MsiSetPropertyA
145 stub MsiSetPropertyW
146 stub MsiSetTargetPathA
147 stub MsiSetTargetPathW
148 stdcall MsiSummaryInfoGetPropertyA(long long ptr ptr ptr ptr ptr) MsiSummaryInfoGetPropertyA
149 stdcall MsiSummaryInfoGetPropertyCount(long ptr) MsiSummaryInfoGetPropertyCount
150 stdcall MsiSummaryInfoGetPropertyW(long long ptr ptr ptr ptr ptr) MsiSummaryInfoGetPropertyW
151 stub MsiSummaryInfoPersist
152 stub MsiSummaryInfoSetPropertyA
153 stub MsiSummaryInfoSetPropertyW
154 stub MsiUseFeatureA
155 stub MsiUseFeatureW
156 stub MsiVerifyPackageA
157 stub MsiVerifyPackageW
158 stdcall MsiViewClose(long) MsiViewClose
159 stdcall MsiViewExecute(long long) MsiViewExecute
160 stdcall MsiViewFetch(long ptr) MsiViewFetch
161 stub MsiViewGetErrorA
162 stub MsiViewGetErrorW
163 stub MsiViewModify
164 stdcall MsiDatabaseIsTablePersistentA(long str) MsiDatabaseIsTablePersistentA
165 stdcall MsiDatabaseIsTablePersistentW(long wstr) MsiDatabaseIsTablePersistentW
166 stdcall MsiViewGetColumnInfo(long long ptr) MsiViewGetColumnInfo
167 stdcall MsiRecordClearData(long) MsiRecordClearData
168 stdcall MsiEnableLogA(long str long) MsiEnableLogA
169 stdcall MsiEnableLogW(long wstr long) MsiEnableLogW
170 stdcall MsiFormatRecordA(long long ptr ptr) MsiFormatRecordA
171 stdcall MsiFormatRecordW(long long ptr ptr) MsiFormatRecordW
172 stub MsiGetComponentPathA
173 stub MsiGetComponentPathW
174 stub MsiApplyPatchA
175 stub MsiApplyPatchW
176 stub MsiAdvertiseScriptA
177 stub MsiAdvertiseScriptW
178 stub MsiGetPatchInfoA
179 stub MsiGetPatchInfoW
180 stub MsiEnumPatchesA
181 stub MsiEnumPatchesW
182 stdcall DllGetVersion(ptr) MSI_DllGetVersion
183 stub MsiGetProductCodeFromPackageCodeA
184 stub MsiGetProductCodeFromPackageCodeW
185 stub MsiCreateTransformSummaryInfoA
186 stub MsiCreateTransformSummaryInfoW
187 stub MsiQueryFeatureStateFromDescriptorA
188 stub MsiQueryFeatureStateFromDescriptorW
189 stub MsiConfigureProductExA
190 stub MsiConfigureProductExW
191 stub MsiInvalidateFeatureCache
192 stub MsiUseFeatureExA
193 stub MsiUseFeatureExW
194 stub MsiGetFileVersionA
195 stub MsiGetFileVersionW
196 stdcall MsiLoadStringA(long long long long long long) MsiLoadStringA
197 stdcall MsiLoadStringW(long long long long long long) MsiLoadStringW
198 stdcall MsiMessageBoxA(long long long long long long) MsiMessageBoxA
199 stdcall MsiMessageBoxW(long long long long long long) MsiMessageBoxW
200 stub MsiDecomposeDescriptorA
201 stub MsiDecomposeDescriptorW
202 stub MsiProvideQualifiedComponentExA
203 stub MsiProvideQualifiedComponentExW
204 stub MsiEnumRelatedProductsA
205 stub MsiEnumRelatedProductsW
206 stub MsiSetFeatureAttributesA
207 stub MsiSetFeatureAttributesW
208 stub MsiSourceListClearAllA
209 stub MsiSourceListClearAllW
210 stub MsiSourceListAddSourceA
211 stub MsiSourceListAddSourceW
212 stub MsiSourceListForceResolutionA
213 stub MsiSourceListForceResolutionW
214 stub MsiIsProductElevatedA
215 stub MsiIsProductElevatedW
216 stub MsiGetShortcutTargetA
217 stub MsiGetShortcutTargetW

