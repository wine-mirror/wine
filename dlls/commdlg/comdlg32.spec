name	comdlg32
type	win32
init	COMDLG32_DllEntryPoint
rsrc	comdlg32

import	shell32.dll
import	comctl32.dll
import	winspool.drv

 0 stub ArrowBtnWndProc
 1 stdcall ChooseColorA(ptr) ChooseColorA
 2 stdcall ChooseColorW(ptr) ChooseColorW
 3 stdcall ChooseFontA(ptr) ChooseFontA
 4 stdcall ChooseFontW(ptr) ChooseFontW
 5 stdcall CommDlgExtendedError() CommDlgExtendedError
 6 stdcall FindTextA(ptr) FindTextA
 7 stdcall FindTextW(ptr) FindTextW
 8 stdcall GetFileTitleA(ptr ptr long) GetFileTitleA
 9 stdcall GetFileTitleW(ptr ptr long) GetFileTitleW
10 stdcall GetOpenFileNameA(ptr) GetOpenFileNameA
11 stdcall GetOpenFileNameW(ptr) GetOpenFileNameW
12 stdcall GetSaveFileNameA(ptr) GetSaveFileNameA
13 stdcall GetSaveFileNameW(ptr) GetSaveFileNameW
14 stub LoadAlterBitmap
15 stdcall PageSetupDlgA(ptr) PageSetupDlgA
16 stub PageSetupDlgW
17 stdcall PrintDlgA(ptr) PrintDlgA
18 stdcall PrintDlgW(ptr) PrintDlgW
19 stdcall ReplaceTextA(ptr) ReplaceTextA
20 stdcall ReplaceTextW(ptr) ReplaceTextW
21 stub WantArrows
22 stub dwLBSubclass
23 stub dwOKSubclass

