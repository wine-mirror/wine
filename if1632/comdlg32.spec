name	comdlg32
type	win32
base	1

0000 stub ArrowBtnWndProc
0001 stub ChooseColorA
0002 stub ChooseColorW
0003 stub ChooseFontA
0004 stub ChooseFontW
0005 stdcall CommDlgExtendedError() CommDlgExtendedError
0006 stub FindTextA
0007 stub FindTextW
0008 stdcall GetFileTitleA(ptr ptr long) GetFileTitle32A
0009 stdcall GetFileTitleW(ptr ptr long) GetFileTitle32W
0010 stdcall GetOpenFileNameA(ptr) GetOpenFileName32A
0011 stdcall GetOpenFileNameW(ptr) GetOpenFileName32W
0012 stdcall GetSaveFileNameA(ptr) GetSaveFileName32A
0013 stdcall GetSaveFileNameW(ptr) GetSaveFileName32A
0014 stub LoadAlterBitmap
0015 stub PageSetupDlgA
0016 stub PageSetupDlgW
0017 return PrintDlgA 4 0
0018 return PrintDlgW 4 0
0019 stub ReplaceTextA
0020 stub ReplaceTextW
0021 stub WantArrows
0022 stub dwLBSubclass
0023 stub dwOKSubclass

