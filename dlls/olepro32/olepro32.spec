name olepro32
type win32

import oleaut32.dll

248 forward OleIconToCursor OLEAUT32.OleIconToCursor
249 forward OleCreatePropertyFrameIndirect OLEAUT32.OleCreatePropertyFrameIndirect
250 forward OleCreatePropertyFrame OLEAUT32.OleCreatePropertyFrame
251 forward OleLoadPicture OLEAUT32.OleLoadPicture
252 forward OleCreatePictureIndirect OLEAUT32.OleCreatePictureIndirect
253 forward OleCreateFontIndirect OLEAUT32.OleCreateFontIndirect
254 forward OleTranslateColor OLEAUT32.OleTranslateColor
255 stdcall DllCanUnloadNow() OLEPRO32_DllCanUnloadNow
256 stdcall DllGetClassObject( ptr ptr ptr )  OLEPRO32_DllGetClassObject
257 stdcall DllRegisterServer() OLEPRO32_DllRegisterServer
258 stdcall DllUnregisterServer() OLEPRO32_DllUnregisterServer  
