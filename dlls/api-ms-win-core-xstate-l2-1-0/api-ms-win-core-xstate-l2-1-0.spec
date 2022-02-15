@ stdcall -arch=i386,x86_64 CopyContext(ptr long ptr) kernelbase.CopyContext
@ stdcall -ret64 -arch=i386,x86_64 GetEnabledXStateFeatures() kernelbase.GetEnabledXStateFeatures
@ stdcall -arch=i386,x86_64 GetXStateFeaturesMask(ptr ptr) kernelbase.GetXStateFeaturesMask
@ stdcall -arch=i386,x86_64 InitializeContext(ptr long ptr ptr) kernelbase.InitializeContext
@ stdcall -arch=i386,x86_64 LocateXStateFeature(ptr long ptr) kernelbase.LocateXStateFeature
@ stdcall -arch=i386,x86_64 SetXStateFeaturesMask(ptr int64) kernelbase.SetXStateFeaturesMask
