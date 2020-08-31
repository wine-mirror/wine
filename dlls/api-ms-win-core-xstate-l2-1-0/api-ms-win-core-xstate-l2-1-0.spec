@ stub CopyContext
@ stdcall -ret64 -arch=i386,x86_64 GetEnabledXStateFeatures() kernel32.GetEnabledXStateFeatures
@ stub GetXStateFeaturesMask
@ stdcall -arch=i386,x86_64 InitializeContext(ptr long ptr ptr) kernel32.InitializeContext
@ stdcall -arch=i386,x86_64 LocateXStateFeature(ptr long ptr) kernel32.LocateXStateFeature
@ stub SetXStateFeaturesMask
