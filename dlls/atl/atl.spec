1 stub DllCanUnloadNow
2 stub DllGetClassObject
3 stdcall -private DllRegisterServer() ATL_DllRegisterServer
4 stdcall -private DllUnregisterServer() ATL_DllUnregisterServer
10 stub AtlAdvise
11 stub AtlUnadvise
12 stub AtlFreeMarshalStream
13 stub AtlMarshalPtrInProc
14 stub AtlUnmarshalPtr
15 stub AtlModuleGetClassObject
16 stdcall AtlModuleInit(ptr long long)
17 stdcall AtlModuleRegisterClassObjects(ptr long long)
18 stub AtlModuleRegisterServer
19 stub AtlModuleRegisterTypeLib
20 stub AtlModuleRevokeClassObjects
21 stdcall AtlModuleTerm(ptr)
22 stub AtlModuleUnregisterServer
23 stdcall AtlModuleUpdateRegistryFromResourceD(ptr wstr long ptr ptr)
24 stub AtlWaitWithMessageLoop
25 stub AtlSetErrorInfo
26 stub AtlCreateTargetDC
27 stub AtlHiMetricToPixel
28 stub AtlPixelToHiMetric
29 stub AtlDevModeW2A
30 stub AtlComPtrAssign
31 stub AtlComQIPtrAssign
32 stdcall AtlInternalQueryInterface(ptr ptr ptr ptr)
34 stub AtlGetVersion
35 stub AtlAxDialogBoxW
36 stub AtlAxDialogBoxA
37 stub AtlAxCreateDialogW
38 stub AtlAxCreateDialogA
39 stub AtlAxCreateControl
40 stub AtlAxCreateControlEx
41 stub AtlAxAttachControl
42 stub AtlAxWinInit
43 stub AtlModuleAddCreateWndData
44 stub AtlModuleExtractCreateWndData
45 stub AtlModuleRegisterWndClassInfoW
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
57 stub AtlModuleUnregisterServerEx
58 stub AtlModuleAddTermFunc
