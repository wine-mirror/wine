name	serialui
type	win32
init	SERIALUI_LibMain
rsrc	serialui_rc

2 stdcall EnumPropPages(ptr ptr ptr) SERIALUI_EnumPropPages
3 stdcall drvCommConfigDialog(ptr long ptr) SERIALUI_CommConfigDialog
4 stdcall drvSetDefaultCommConfig() SERIALUI_SetDefaultCommConfig
5 stdcall drvGetDefaultCommConfig() SERIALUI_GetDefaultCommConfig
