10 stdcall AtlAdvise(ptr ptr ptr ptr) atl.AtlAdvise
11 stdcall AtlUnadvise(ptr ptr long) atl.AtlUnadvise
12 stdcall AtlFreeMarshalStream(ptr) atl.AtlFreeMarshalStream
13 stdcall AtlMarshalPtrInProc(ptr ptr ptr) atl.AtlMarshalPtrInProc
14 stdcall AtlUnmarshalPtr(ptr ptr ptr) atl.AtlUnmarshalPtr
15 stdcall AtlModuleGetClassObject(ptr ptr ptr ptr) atl.AtlModuleGetClassObject
16 stdcall AtlModuleInit(ptr long long) atl.AtlModuleInit
17 stdcall AtlModuleRegisterClassObjects(ptr long long) atl.AtlModuleRegisterClassObjects
18 stdcall AtlModuleRegisterServer(ptr long ptr) atl.AtlModuleRegisterServer
19 stdcall AtlModuleRegisterTypeLib(ptr wstr) atl.AtlModuleRegisterTypeLib
20 stdcall AtlModuleRevokeClassObjects(ptr) atl.AtlModuleRevokeClassObjects
21 stdcall AtlModuleTerm(ptr) atl.AtlModuleTerm
22 stdcall AtlModuleUnregisterServer(ptr ptr) atl.AtlModuleUnregisterServer
23 stdcall AtlModuleUpdateRegistryFromResourceD(ptr wstr long ptr ptr) atl.AtlModuleUpdateRegistryFromResourceD
24 stub AtlWaitWithMessageLoop
25 stub AtlSetErrorInfo
26 stdcall AtlCreateTargetDC(long ptr) atl.AtlCreateTargetDC
27 stdcall AtlHiMetricToPixel(ptr ptr) atl.AtlHiMetricToPixel
28 stdcall AtlPixelToHiMetric(ptr ptr) atl.AtlPixelToHiMetric
29 stub AtlDevModeW2A
30 stdcall AtlComPtrAssign(ptr ptr) atl.AtlComPtrAssign
31 stdcall AtlComQIPtrAssign(ptr ptr ptr) atl.AtlComQIPtrAssign
32 stdcall AtlInternalQueryInterface(ptr ptr ptr ptr) atl.AtlInternalQueryInterface
34 stdcall AtlGetVersion(ptr)
35 stub AtlAxDialogBoxW
36 stub AtlAxDialogBoxA
37 stdcall AtlAxCreateDialogW(long wstr long ptr long) atl.AtlAxCreateDialogW
38 stdcall AtlAxCreateDialogA(long str long ptr long) atl.AtlAxCreateDialogA
39 stdcall AtlAxCreateControl(ptr ptr ptr ptr) atl.AtlAxCreateControl
40 stdcall AtlAxCreateControlEx(ptr ptr ptr ptr ptr ptr ptr) atl.AtlAxCreateControlEx
41 stdcall AtlAxAttachControl(ptr ptr ptr) atl.AtlAxAttachControl
42 stdcall AtlAxWinInit() atl.AtlAxWinInit
43 stdcall AtlModuleAddCreateWndData(ptr ptr ptr) atl.AtlModuleAddCreateWndData
44 stdcall AtlModuleExtractCreateWndData(ptr) atl.AtlModuleExtractCreateWndData
45 stdcall AtlModuleRegisterWndClassInfoW(ptr ptr ptr) atl.AtlModuleRegisterWndClassInfoW
46 stdcall AtlModuleRegisterWndClassInfoA(ptr ptr ptr) atl.AtlModuleRegisterWndClassInfoA
47 stdcall AtlAxGetControl(long ptr) atl.AtlAxGetControl
48 stdcall AtlAxGetHost(long ptr) atl.AtlAxGetHost
49 stub AtlRegisterClassCategoriesHelper
50 stdcall AtlIPersistStreamInit_Load(ptr ptr ptr ptr) atl.AtlIPersistStreamInit_Load
51 stdcall AtlIPersistStreamInit_Save(ptr long ptr ptr ptr) atl.AtlIPersistStreamInit_Save
52 stub AtlIPersistPropertyBag_Load
53 stub AtlIPersistPropertyBag_Save
54 stub AtlGetObjectSourceInterface
55 stub AtlModuleUnRegisterTypeLib
56 stdcall AtlModuleLoadTypeLib(ptr wstr ptr ptr) atl.AtlModuleLoadTypeLib
57 stdcall AtlModuleUnregisterServerEx(ptr long ptr) atl.AtlModuleUnregisterServerEx
58 stdcall AtlModuleAddTermFunc(ptr ptr long) atl.AtlModuleAddTermFunc
59 stub AtlAxCreateControlLic
60 stub AtlAxCreateControlLicEx
61 stdcall AtlCreateRegistrar(ptr)
62 stub AltWinModuleRegisterClassExW
63 stub AltWinModuleRegisterClassExA
64 stub AltCallTermFunc
65 stub AltWinModuleInit
66 stub AltWinModuleTerm
