name	comdlg32
type	win32
init	COMDLG32_DllEntryPoint
rsrc	rsrc.res

import shell32.dll
import shlwapi.dll
import comctl32.dll
import winspool.drv
import user32.dll
import gdi32.dll
import kernel32.dll
import ntdll.dll

debug_channels (commdlg)

@ stdcall ChooseColorA(ptr) ChooseColorA
@ stdcall ChooseColorW(ptr) ChooseColorW
@ stdcall ChooseFontA(ptr) ChooseFontA
@ stdcall ChooseFontW(ptr) ChooseFontW
@ stdcall CommDlgExtendedError() CommDlgExtendedError
@ stdcall FindTextA(ptr) FindTextA
@ stdcall FindTextW(ptr) FindTextW
@ stdcall GetFileTitleA(ptr ptr long) GetFileTitleA
@ stdcall GetFileTitleW(ptr ptr long) GetFileTitleW
@ stdcall GetOpenFileNameA(ptr) GetOpenFileNameA
@ stdcall GetOpenFileNameW(ptr) GetOpenFileNameW
@ stdcall GetSaveFileNameA(ptr) GetSaveFileNameA
@ stdcall GetSaveFileNameW(ptr) GetSaveFileNameW
@ stub LoadAlterBitmap
@ stdcall PageSetupDlgA(ptr) PageSetupDlgA
@ stdcall PageSetupDlgW(ptr) PageSetupDlgW
@ stdcall PrintDlgA(ptr) PrintDlgA
@ stdcall PrintDlgW(ptr) PrintDlgW
@ stdcall PrintDlgExA(ptr) PrintDlgExA
@ stdcall PrintDlgExW(ptr) PrintDlgExW
@ stdcall ReplaceTextA(ptr) ReplaceTextA
@ stdcall ReplaceTextW(ptr) ReplaceTextW
@ stub WantArrows
@ stub dwLBSubclass
@ stub dwOKSubclass
