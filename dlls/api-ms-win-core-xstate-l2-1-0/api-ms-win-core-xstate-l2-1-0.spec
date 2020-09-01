@ stub CopyContext
@ stdcall -ret64 -arch=i386,x86_64 GetEnabledXStateFeatures() kernel32.GetEnabledXStateFeatures
@ stdcall -arch=i386,x86_64 GetXStateFeaturesMask(ptr ptr) kernel32.GetXStateFeaturesMask
@ stdcall -arch=i386,x86_64 InitializeContext(ptr long ptr ptr) kernel32.InitializeContext
@ stdcall -arch=i386,x86_64 LocateXStateFeature(ptr long ptr) kernel32.LocateXStateFeature
@ stdcall -arch=i386,x86_64 SetXStateFeaturesMask(ptr int64) kernel32.SetXStateFeaturesMask
