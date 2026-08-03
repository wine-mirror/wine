@ stdcall -private DllMain(long long ptr)
@ stdcall D3D10CreateBlob(long ptr) #1
@ stdcall D3D11CreateDevice(ptr long ptr long ptr long long ptr ptr ptr) #2
@ stdcall D3D11CreateDeviceAndSwapChain(ptr long ptr long ptr long long ptr ptr ptr ptr ptr) #3
@ stdcall D3D11XCreateDeviceX(ptr ptr ptr) #4
@ stdcall D3D11XCreateDeviceXAndSwapChain1(ptr ptr ptr ptr ptr) #5
@ stdcall D3DAllocateGraphicsMemory(long int64 int64 long ptr) #6
@ stdcall D3DConfigureVirtualMemory(int64) #7
@ stdcall D3DFreeGraphicsMemory(ptr) #8
@ stdcall D3DMapEsramMemory(long ptr long ptr) #9
@ stdcall DXGIXGetFrameStatistics(long ptr) #10
@ stdcall DXGIXPresentArray(long long long long ptr ptr) #11
@ stdcall DXGIXSetVLineNotification(long long ptr) #12
@ extern IID_ID3D11DeviceContextX #40
@ extern IID_ID3D11DeviceX #41
@ extern IID_ID3D11Texture2D #61
@ extern IID_IDXGIDevice #70
@ extern IID_IDXGIDevice1 #71
@ extern IID_IDXGIFactory2 #76
