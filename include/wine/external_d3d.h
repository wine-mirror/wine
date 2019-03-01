#ifndef EXTERNAL_D3D_H
#define EXTERNAL_D3D_H

#ifndef EXTERNAL_D3D_NO_WINDOWS_H
  #ifdef __GNUC__
    #pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
  #endif
  #include <windows.h>
  #include <d3d11.h>
  #include <d3d10_1.h>
  #include <dxgi1_2.h>
#endif

#ifndef EXTERNAL_D3D_NO_VULKAN_H
  #include <vulkan/vulkan.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef VkResult (*PFN_create_vulkan_surface)(VkInstance instance, void* window, VkSurfaceKHR *surface);
  typedef struct tag_native_info
  {
      PFN_vkGetInstanceProcAddr  pfn_vkGetInstanceProcAddr;
      PFN_create_vulkan_surface  pfn_create_vulkan_surface;
  } native_info;

  HRESULT native_core_create_d3d11_device(
    native_info            native_info,
    IDXGIFactory*               pFactory,
    IDXGIAdapter*               pAdapter,
    UINT                        Flags,
    const D3D_FEATURE_LEVEL*    pFeatureLevels,
    UINT                        FeatureLevels,
    ID3D11Device**              ppDevice
  );

  typedef HRESULT (*PFN_native_core_create_d3d11_device)(native_info,IDXGIFactory*,IDXGIAdapter*,UINT,const D3D_FEATURE_LEVEL*,UINT,ID3D11Device**);

  HRESULT native_core_create_d3d10_device(
    native_info        native_info,
    IDXGIFactory*           pFactory,
    IDXGIAdapter*           pAdapter,
    UINT                    Flags,
    D3D_FEATURE_LEVEL       FeatureLevel,
    ID3D10Device**         ppDevice
  );

  typedef HRESULT (*PFN_native_core_create_d3d10_device)(native_info,IDXGIFactory*,IDXGIAdapter*,UINT,D3D_FEATURE_LEVEL,ID3D10Device**);

  extern native_info g_native_info;

#ifdef __cplusplus
}
#endif

#endif
