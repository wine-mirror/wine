name olepro32
type win32

@ stdcall OleIconToCursor( ptr ptr) OleIconToCursor
@ stdcall OleCreatePropertyFrameIndirect( ptr ) OleCreatePropertyFrameIndirect
@ stdcall OleCreatePropertyFrame( ptr long long ptr long ptr long ptr ptr long ptr ) OleCreatePropertyFrame
@ stdcall OleLoadPicture( ptr long long ptr ptr ) OleLoadPicture
@ stdcall OleCreatePictureIndirect( ptr ptr long ptr ) OleCreatePictureIndirect
@ stdcall OleCreateFontIndirect( ptr ptr ptr ) OleCreateFontIndirect
@ stdcall OleTranslateColor( long ptr ptr ) OleTranslateColor
@ stdcall DllCanUnloadNow() OLEPRO32_DllCanUnloadNow
@ stdcall DllGetClassObjecti( ptr ptr ptr )  OLEPRO32_DllGetClassObject
@ stdcall DllRegisterServer() OLEPRO32_DllRegisterServer
@ stdcall DllUnregisterServer() OLEPRO32_DllUnregisterServer
