name	winspool
type	win32

101 stub ADVANCEDSETUPDIALOG
102 stub AbortPrinter
103 stdcall AddFormA(long long ptr) AddForm32A
104 stdcall AddFormW(long long ptr) AddForm32W
105 stdcall AddJobA(long long ptr long ptr) AddJob32A
106 stdcall AddJobW(long long ptr long ptr) AddJob32W
107 stdcall AddMonitorA(str long ptr) AddMonitor32A
108 stub AddMonitorW
109 stub AddPortA
110 stub AddPortExA
111 stub AddPortExW
112 stub AddPortW
113 stub AddPrintProcessorA
114 stub AddPrintProcessorW
115 stub AddPrintProvidorA
116 stub AddPrintProvidorW
117 stdcall AddPrinterA(str long ptr) AddPrinter32A
118 stub AddPrinterConnectionA
119 stub AddPrinterConnectionW
120 stub AddPrinterDriverA
121 stub AddPrinterDriverW
122 stdcall AddPrinterW(wstr long ptr) AddPrinter32W
123 stub AdvancedDocumentPropertiesA
124 stub AdvancedDocumentPropertiesW
125 stub AdvancedSetupDialog
126 stdcall ClosePrinter(long) ClosePrinter32
127 stub ConfigurePortA
128 stub ConfigurePortW
129 stub ConnectToPrinterDlg
130 stub CreatePrinterIC
131 stub DEVICECAPABILITIES
132 stub DEVICEMODE
133 stdcall DeleteFormA(long str) DeleteForm32A
134 stdcall DeleteFormW(long wstr) DeleteForm32W
135 stdcall DeleteMonitorA(str str str) DeleteMonitor32A
136 stub DeleteMonitorW
137 stdcall DeletePortA(str long str) DeletePort32A
138 stub DeletePortW
139 stub DeletePrintProcessorA
140 stub DeletePrintProcessorW
141 stub DeletePrintProvidorA
142 stub DeletePrintProvidorW
143 stdcall DeletePrinter(long) DeletePrinter32
144 stub DeletePrinterConnectionA
145 stub DeletePrinterConnectionW
146 stdcall DeletePrinterDriverA(str str str) DeletePrinterDriver32A
147 stub DeletePrinterDriverW
148 stub DeletePrinterIC
149 stub DevQueryPrint
150 stub DeviceCapabilities
151 stdcall DeviceCapabilitiesA(str str long ptr ptr) DeviceCapabilities32A
152 stdcall DeviceCapabilitiesW(wstr wstr long wstr ptr) DeviceCapabilities32W
153 stub DeviceMode
154 stub DocumentEvent
155 stdcall DocumentPropertiesA(long long ptr ptr ptr long) DocumentProperties32A
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
166 stdcall EnumPortsA(ptr long ptr ptr ptr ptr) EnumPorts32A
167 stub EnumPortsW
168 stub EnumPrintProcessorDatatypesA
169 stub EnumPrintProcessorDatatypesW
170 stub EnumPrintProcessorsA
171 stub EnumPrintProcessorsW
172 stub EnumPrinterDriversA
173 stub EnumPrinterDriversW
174 stdcall EnumPrintersA(long ptr long ptr long ptr ptr) EnumPrinters32A
175 stdcall EnumPrintersW(long ptr long ptr long ptr ptr) EnumPrinters32W
176 stub ExtDeviceMode
177 stub FindClosePrinterChangeNotification
178 stub FindFirstPrinterChangeNotification
179 stub FindNextPrinterChangeNotification
180 stub FreePrinterNotifyInfo
181 stdcall GetFormA(long str long ptr long ptr) GetForm32A
182 stdcall GetFormW(long wstr long ptr long ptr) GetForm32W
183 stub GetJobA
184 stub GetJobW
185 stub GetPrintProcessorDirectoryA
186 stub GetPrintProcessorDirectoryW
187 stdcall GetPrinterA(long long ptr long ptr) GetPrinter32A
188 stub GetPrinterDataA
189 stub GetPrinterDataW
190 stdcall GetPrinterDriverA(long str long ptr long ptr) GetPrinterDriver32A
191 stub GetPrinterDriverDirectoryA
192 stub GetPrinterDriverDirectoryW
193 stdcall GetPrinterDriverW(long str long ptr long ptr) GetPrinterDriver32W
194 stdcall GetPrinterW(long long ptr long ptr) GetPrinter32W
195 stub InitializeDll
196 stdcall OpenPrinterA(str ptr ptr) OpenPrinter32A
197 stdcall OpenPrinterW(wstr ptr ptr) OpenPrinter32W
198 stub PlayGdiScriptOnPrinterIC
199 stub PrinterMessageBoxA
200 stub PrinterMessageBoxW
201 stub PrinterProperties
202 stdcall ReadPrinter(long ptr long ptr) ReadPrinter32
203 stdcall ResetPrinterA(long ptr) ResetPrinter32A
204 stdcall ResetPrinterW(long ptr) ResetPrinter32W
205 stub ScheduleJob
206 stub SetAllocFailCount
207 stdcall SetFormA(long str long ptr) SetForm32A
208 stdcall SetFormW(long wstr long ptr) SetForm32W
209 stdcall SetJobA(long long long ptr long) SetJob32A
210 stdcall SetJobW(long long long ptr long) SetJob32W
211 stdcall SetPrinterA(long long ptr long) SetPrinter32A
212 stub SetPrinterDataA
213 stub SetPrinterDataW
214 stdcall SetPrinterW(long long ptr long) SetPrinter32W
215 stub SpoolerDevQueryPrintW
216 stub SpoolerInit
217 stub StartDocDlgA
218 stub StartDocDlgW
219 stub StartDocPrinterA
220 stub StartDocPrinterW
221 stub StartPagePrinter
222 stub WaitForPrinterChange
223 stdcall WritePrinter(long ptr long ptr) WritePrinter32
