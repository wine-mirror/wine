1 stdcall -private DllCanUnloadNow()
2 stdcall -private DllGetClassObject(ptr ptr ptr)
3 stdcall -private DllRegisterServer()
4 stdcall -private DllUnregisterServer()
10 stdcall AtlAdvise(ptr ptr ptr ptr)
11 stdcall AtlUnadvise(ptr ptr long)
12 stdcall AtlFreeMarshalStream(ptr)
13 stdcall AtlMarshalPtrInProc(ptr ptr ptr)
14 stdcall AtlUnmarshalPtr(ptr ptr ptr)
15 stdcall AtlModuleGetClassObject(ptr ptr ptr ptr)
16 stdcall AtlModuleInit(ptr long long)
17 stdcall AtlModuleRegisterClassObjects(ptr long long)
18 stdcall AtlModuleRegisterServer(ptr long ptr)
19 stdcall AtlModuleRegisterTypeLib(ptr wstr)
20 stdcall AtlModuleRevokeClassObjects(ptr)
21 stdcall AtlModuleTerm(ptr)
22 stdcall AtlModuleUnregisterServer(ptr ptr)
23 stdcall AtlModuleUpdateRegistryFromResourceD(ptr wstr long ptr ptr)
24 stub AtlWaitWithMessageLoop
25 stub AtlSetErrorInfo
26 stub AtlCreateTargetDC
27 stub AtlHiMetricToPixel
28 stub AtlPixelToHiMetric
29 stub AtlDevModeW2A
30 stdcall AtlComPtrAssign(ptr ptr)
31 stub AtlComQIPtrAssign
32 stdcall AtlInternalQueryInterface(ptr ptr ptr ptr)
34 stub AtlGetVersion
35 stub AtlAxDialogBoxW
36 stub AtlAxDialogBoxA
37 stub AtlAxCreateDialogW
38 stub AtlAxCreateDialogA
39 stdcall AtlAxCreateControl(ptr ptr ptr ptr)
40 stub AtlAxCreateControlEx
41 stub AtlAxAttachControl
42 stdcall AtlAxWinInit()
43 stub AtlModuleAddCreateWndData
44 stub AtlModuleExtractCreateWndData
45 stdcall AtlModuleRegisterWndClassInfoW(ptr ptr ptr)
46 stub AtlModuleRegisterWndClassInfoA
47 stub AtlAxGetControl
48 stub AtlAxGetHost
49 stub AtlRegisterClassCategoriesHelper
50 stub AtlIPersistStreamInit_Load
51 stub AtlIPersistStreamInit_Save
52 stub AtlIPersistPropertyBag_Load
53 stub AtlIPersistPropertyBag_Save
54 stub AtlGetObjectSourceInterface
55 stub AtlModuleUnRegisterTypeLib
56 stub AtlModuleLoadTypeLib
57 stdcall AtlModuleUnregisterServerEx(ptr long ptr)
58 stdcall AtlModuleAddTermFunc(ptr ptr long)
