/*
 * Copyright 2016 Józef Kucia for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __VKD3D_UTILS_H
#define __VKD3D_UTILS_H

#include <vkd3d.h>
#include <d3dcompiler.h>

#ifndef VKD3D_UTILS_API_VERSION
#define VKD3D_UTILS_API_VERSION VKD3D_API_VERSION_1_0
#endif

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * \file vkd3d_utils.h
 *
 * This file contains definitions for the vkd3d-utils library.
 *
 * The vkd3d-utils library is a collections of routines to ease the
 * porting of a Direct3D 12 application to vkd3d.
 *
 * \since 1.0
 */

#define VKD3D_WAIT_OBJECT_0 (0)
#define VKD3D_WAIT_TIMEOUT (1)
#define VKD3D_WAIT_FAILED (~0u)
#define VKD3D_INFINITE (~0u)

#ifdef LIBVKD3D_UTILS_SOURCE
# define VKD3D_UTILS_API VKD3D_EXPORT
#else
# define VKD3D_UTILS_API VKD3D_IMPORT
#endif

/* 1.0 */
VKD3D_UTILS_API HANDLE vkd3d_create_event(void);
VKD3D_UTILS_API HRESULT vkd3d_signal_event(HANDLE event);
VKD3D_UTILS_API unsigned int vkd3d_wait_event(HANDLE event, unsigned int milliseconds);
VKD3D_UTILS_API void vkd3d_destroy_event(HANDLE event);

#if __STDC_VERSION__ >= 199901L || __cplusplus >= 201103L
# define D3D12CreateDevice(...) D3D12CreateDeviceVKD3D(__VA_ARGS__, VKD3D_UTILS_API_VERSION)
#else
# define D3D12CreateDevice(a, b, c, d) D3D12CreateDeviceVKD3D(a, b, c, d, VKD3D_UTILS_API_VERSION)
#endif
VKD3D_UTILS_API HRESULT WINAPI D3D12CreateRootSignatureDeserializer(
        const void *data, SIZE_T data_size, REFIID iid, void **deserializer);
VKD3D_UTILS_API HRESULT WINAPI D3D12GetDebugInterface(REFIID iid, void **debug);
VKD3D_UTILS_API HRESULT WINAPI D3D12SerializeRootSignature(const D3D12_ROOT_SIGNATURE_DESC *desc,
        D3D_ROOT_SIGNATURE_VERSION version, ID3DBlob **blob, ID3DBlob **error_blob);

/* 1.2 */
VKD3D_UTILS_API HRESULT WINAPI D3D12CreateDeviceVKD3D(IUnknown *adapter, D3D_FEATURE_LEVEL feature_level,
        REFIID iid, void **device, enum vkd3d_api_version api_version);
VKD3D_UTILS_API HRESULT WINAPI D3D12CreateVersionedRootSignatureDeserializer(const void *data,
        SIZE_T data_size, REFIID iid, void **deserializer);
VKD3D_UTILS_API HRESULT WINAPI D3D12SerializeVersionedRootSignature(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC *desc,
        ID3DBlob **blob, ID3DBlob **error_blob);

/* 1.3 */
VKD3D_UTILS_API HRESULT WINAPI D3DCompile(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT flags, UINT effect_flags, ID3DBlob **shader, ID3DBlob **error_messages);
/**
 * D3DCompile2() targets the behaviour of d3dcompiler_47.dll. To target the
 * behaviour of other d3dcompiler versions, use D3DCompile2VKD3D().
 *
 * \since 1.3
 */
VKD3D_UTILS_API HRESULT WINAPI D3DCompile2(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT flags, UINT effect_flags, UINT secondary_flags,
        const void *secondary_data, SIZE_T secondary_data_size, ID3DBlob **shader,
        ID3DBlob **error_messages);
VKD3D_UTILS_API HRESULT WINAPI D3DCreateBlob(SIZE_T data_size, ID3DBlob **blob);
VKD3D_UTILS_API HRESULT WINAPI D3DPreprocess(const void *data, SIZE_T size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include,
        ID3DBlob **shader, ID3DBlob **error_messages);

/**
 * Set a callback to be called when vkd3d-utils outputs debug logging.
 *
 * If NULL, or if this function has not been called, libvkd3d-utils will print
 * all enabled log output to stderr.
 *
 * Calling this function will also set the log callback for libvkd3d and
 * libvkd3d-shader.
 *
 * \param callback Callback function to set.
 *
 * \since 1.4
 */
VKD3D_UTILS_API void vkd3d_utils_set_log_callback(PFN_vkd3d_log callback);

/** \since 1.10 */
VKD3D_UTILS_API HRESULT WINAPI D3DGetBlobPart(const void *data,
        SIZE_T data_size, D3D_BLOB_PART part, UINT flags, ID3DBlob **blob);
/** \since 1.10 */
VKD3D_UTILS_API HRESULT WINAPI D3DGetDebugInfo(const void *data, SIZE_T data_size, ID3DBlob **blob);
/** \since 1.10 */
VKD3D_UTILS_API HRESULT WINAPI D3DGetInputAndOutputSignatureBlob(const void *data, SIZE_T data_size, ID3DBlob **blob);
/** \since 1.10 */
VKD3D_UTILS_API HRESULT WINAPI D3DGetInputSignatureBlob(const void *data, SIZE_T data_size, ID3DBlob **blob);
/** \since 1.10 */
VKD3D_UTILS_API HRESULT WINAPI D3DGetOutputSignatureBlob(const void *data, SIZE_T data_size, ID3DBlob **blob);
/** \since 1.10 */
VKD3D_UTILS_API HRESULT WINAPI D3DStripShader(const void *data, SIZE_T data_size, UINT flags, ID3DBlob **blob);

/** \since 1.11 */
VKD3D_UTILS_API HRESULT WINAPI D3DDisassemble(const void *data,
        SIZE_T data_size, UINT flags, const char *comments, ID3DBlob **blob);
/** \since 1.11 */
VKD3D_UTILS_API HRESULT WINAPI D3DReflect(const void *data, SIZE_T data_size, REFIID iid, void **reflection);

/**
 * As D3DCompile2(), but with an extra argument that allows targeting
 * different d3dcompiler versions.
 *
 * \param compiler_version The d3dcompiler version to target. This should be
 * set to the numerical value in the d3dcompiler library name. E.g. to target
 * the behaviour of d3dcompiler_36.dll, set this parameter to 36.
 *
 * \since 1.14
 */
VKD3D_UTILS_API HRESULT WINAPI D3DCompile2VKD3D(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT flags, UINT effect_flags, UINT secondary_flags,
        const void *secondary_data, SIZE_T secondary_data_size, ID3DBlob **shader,
        ID3DBlob **error_messages, unsigned int compiler_version);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __VKD3D_UTILS_H */
