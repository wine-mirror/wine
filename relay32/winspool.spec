name	winspool
type	win32

101 stub ADVANCEDSETUPDIALOG
102 stub AbortPrinter
103 stdcall AddFormA(long long ptr) AddFormA
104 stdcall AddFormW(long long ptr) AddFormW
105 stdcall AddJobA(long long ptr long ptr) AddJobA
106 stdcall AddJobW(long long ptr long ptr) AddJobW
107 stdcall AddMonitorA(str long ptr) AddMonitorA
108 stub AddMonitorW
109 stub AddPortA
110 stub AddPortExA
111 stub AddPortExW
112 stub AddPortW
113 stub AddPrintProcessorA
114 stub AddPrintProcessorW
115 stub AddPrintProvidorA
116 stub AddPrintProvidorW
117 stdcall AddPrinterA(str long ptr) AddPrinterA
118 stub AddPrinterConnectionA
119 stub AddPrinterConnectionW
120 stdcall AddPrinterDriverA(str long ptr) AddPrinterDriverA
121 stdcall AddPrinterDriverW(wstr long ptr) AddPrinterDriverW
122 stdcall AddPrinterW(wstr long ptr) AddPrinterW
123 stub AdvancedDocumentPropertiesA
124 stub AdvancedDocumentPropertiesW
125 stub AdvancedSetupDialog
126 stdcall ClosePrinter(long) ClosePrinter
127 stub ConfigurePortA
128 stub ConfigurePortW
129 stub ConnectToPrinterDlg
130 stub CreatePrinterIC
131 stub DEVICECAPABILITIES
132 stub DEVICEMODE
133 stdcall DeleteFormA(long str) DeleteFormA
134 stdcall DeleteFormW(long wstr) DeleteFormW
135 stdcall DeleteMonitorA(str str str) DeleteMonitorA
136 stub DeleteMonitorW
137 stdcall DeletePortA(str long str) DeletePortA
138 stub DeletePortW
139 stub DeletePrintProcessorA
140 stub DeletePrintProcessorW
141 stub DeletePrintProvidorA
142 stub DeletePrintProvidorW
143 stdcall DeletePrinter(long) DeletePrinter
144 stub DeletePrinterConnectionA
145 stub DeletePrinterConnectionW
146 stdcall DeletePrinterDriverA(str str str) DeletePrinterDriverA
147 stub DeletePrinterDriverW
148 stub DeletePrinterIC
149 stub DevQueryPrint
150 stub DeviceCapabilities
151 stdcall DeviceCapabilitiesA(str str long ptr ptr) DeviceCapabilitiesA
152 stdcall DeviceCapabilitiesW(wstr wstr long wstr ptr) DeviceCapabilitiesW
153 stub DeviceMode
154 stub DocumentEvent
155 stdcall DocumentPropertiesA(long long ptr ptr ptr long) DocumentPropertiesA
156 stub DocumentPropertiesW
157 stub EXTDEVICEMODE
158 stub EndDocPrinter
159 stub EndPagePrinter
160 stub EnumFormsA
161 stub EnumFormsW
162 stub EnumJobsA
163 stub EnumJobsW
164 stub EnumMonitorsA
165 stub EnumMonitorsW
166 stdcall EnumPortsA(ptr long ptr ptr ptr ptr) EnumPortsA
167 stub EnumPortsW
168 stub EnumPrintProcessorDatatypesA
169 stub EnumPrintProcessorDatatypesW
170 stub EnumPrintProcessorsA
171 stub EnumPrintProcessorsW
172 stub EnumPrinterDriversA
173 stub EnumPrinterDriversW
174 stdcall EnumPrintersA(long ptr long ptr long ptr ptr) EnumPrintersA
175 stdcall EnumPrintersW(long ptr long ptr long ptr ptr) EnumPrintersW
176 stub ExtDeviceMode
177 stub FindClosePrinterChangeNotification
178 stub FindFirstPrinterChangeNotification
179 stub FindNextPrinterChangeNotification
180 stub FreePrinterNotifyInfo
181 stdcall GetFormA(long str long ptr long ptr) GetFormA
182 stdcall GetFormW(long wstr long ptr long ptr) GetFormW
183 stub GetJobA
184 stub GetJobW
185 stub GetPrintProcessorDirectoryA
186 stub GetPrintProcessorDirectoryW
187 stdcall GetPrinterA(long long ptr long ptr) GetPrinterA
188 stub GetPrinterDataA
189 stub GetPrinterDataW
190 stdcall GetPrinterDriverA(long str long ptr long ptr) GetPrinterDriverA
191 stdcall GetPrinterDriverDirectoryA(str str long ptr long ptr) GetPrinterDriverDirectoryA
192 stdcall GetPrinterDriverDirectoryW(wstr wstr long ptr long ptr) GetPrinterDriverDirectoryW
193 stdcall GetPrinterDriverW(long str long ptr long ptr) GetPrinterDriverW
194 stdcall GetPrinterW(long long ptr long ptr) GetPrinterW
195 stub InitializeDll
196 stdcall OpenPrinterA(str ptr ptr) OpenPrinterA
197 stdcall OpenPrinterW(wstr ptr ptr) OpenPrinterW
198 stub PlayGdiScriptOnPrinterIC
199 stub PrinterMessageBoxA
200 stub PrinterMessageBoxW
201 stdcall PrinterProperties(long long) PrinterProperties
202 stdcall ReadPrinter(long ptr long ptr) ReadPrinter
203 stdcall ResetPrinterA(long ptr) ResetPrinterA
204 stdcall ResetPrinterW(long ptr) ResetPrinterW
205 stub ScheduleJob
206 stub SetAllocFailCount
207 stdcall SetFormA(long str long ptr) SetFormA
208 stdcall SetFormW(long wstr long ptr) SetFormW
209 stdcall SetJobA(long long long ptr long) SetJobA
210 stdcall SetJobW(long long long ptr long) SetJobW
211 stdcall SetPrinterA(long long ptr long) SetPrinterA
212 stub SetPrinterDataA
213 stub SetPrinterDataW
214 stdcall SetPrinterW(long long ptr long) SetPrinterW
215 stub SpoolerDevQueryPrintW
216 stub SpoolerInit
217 stub StartDocDlgA
218 stub StartDocDlgW
219 stub StartDocPrinterA
220 stub StartDocPrinterW
221 stub StartPagePrinter
222 stub WaitForPrinterChange
223 stdcall WritePrinter(long ptr long ptr) WritePrinter
