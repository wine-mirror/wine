name olepro32
type win32

import oleaut32.dll

@ forward OleIconToCursor OLEAUT32.OleIconToCursor
@ forward OleCreatePropertyFrameIndirect OLEAUT32.OleCreatePropertyFrameIndirect
@ forward OleCreatePropertyFrame OLEAUT32.OleCreatePropertyFrame
@ forward OleLoadPicture OLEAUT32.OleLoadPicture
@ forward OleCreatePictureIndirect OLEAUT32.OleCreatePictureIndirect
@ forward OleCreateFontIndirect OLEAUT32.OleCreateFontIndirect
@ forward OleTranslateColor OLEAUT32.OleTranslateColor
@ stdcall DllCanUnloadNow() OLEPRO32_DllCanUnloadNow
@ stdcall DllGetClassObjecti( ptr ptr ptr )  OLEPRO32_DllGetClassObject
@ stdcall DllRegisterServer() OLEPRO32_DllRegisterServer
@ stdcall DllUnregisterServer() OLEPRO32_DllUnregisterServer
