/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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

#include "objbase.h"

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID(IID_IDebugInputCallbacks,     0x9f50e42c, 0xf136, 0x499e, 0x9a, 0x97, 0x73, 0x03, 0x6c, 0x94, 0xed, 0x2d);
DEFINE_GUID(IID_IDebugOutputCallbacks,    0x4bf58045, 0xd654, 0x4c40, 0xb0, 0xaf, 0x68, 0x30, 0x90, 0xf3, 0x56, 0xdc);
DEFINE_GUID(IID_IDebugEventCallbacks,     0x337be28b, 0x5036, 0x4d72, 0xb6, 0xbf, 0xc4, 0x5f, 0xbb, 0x9f, 0x2e, 0xaa);
DEFINE_GUID(IID_IDebugClient,             0x27fe5639, 0x8407, 0x4f47, 0x83, 0x64, 0xee, 0x11, 0x8f, 0xb0, 0x8a, 0xc8);
DEFINE_GUID(IID_IDebugDataSpaces,         0x88f7dfab, 0x3ea7, 0x4c3a, 0xae, 0xfb, 0xc4, 0xe8, 0x10, 0x61, 0x73, 0xaa);

#define INTERFACE IDebugBreakpoint
DECLARE_INTERFACE_(IDebugBreakpoint, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugBreakpoint */
    /* FIXME */
};
#undef INTERFACE

typedef IDebugBreakpoint* PDEBUG_BREAKPOINT;

#define INTERFACE IDebugInputCallbacks
DECLARE_INTERFACE_(IDebugInputCallbacks, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugInputCallbacks */
    STDMETHOD(StartInput)(THIS_ ULONG buffer_size) PURE;
    STDMETHOD(EndInput)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugOutputCallbacks
DECLARE_INTERFACE_(IDebugOutputCallbacks, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugOutputCallbacks */
    STDMETHOD(Output)(THIS_ ULONG mask, const char *text) PURE;
};
#undef INTERFACE

#ifdef WINE_NO_UNICODE_MACROS
#undef CreateProcess
#endif

#define INTERFACE IDebugEventCallbacks
DECLARE_INTERFACE_(IDebugEventCallbacks, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugEventCallbacks */
    STDMETHOD(GetInterestMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(Breakpoint)(THIS_ PDEBUG_BREAKPOINT breakpoint) PURE;
    STDMETHOD(Exception)(THIS_ EXCEPTION_RECORD64 *exception, ULONG first_chance) PURE;
    STDMETHOD(CreateThread)(THIS_ ULONG64 handle, ULONG64 data_offset, ULONG64 start_offset) PURE;
    STDMETHOD(ExitThread)(THIS_ ULONG exit_code) PURE;
    STDMETHOD(CreateProcess)(THIS_ ULONG64 image_handle, ULONG64 handle, ULONG64 base_offset, ULONG module_size,
            const char *module_name, const char *image_name, ULONG checksum, ULONG timedatestamp,
            ULONG64 initial_thread_handle, ULONG64 thread_data_offset, ULONG64 start_offset) PURE;
    STDMETHOD(ExitProcess)(THIS_ ULONG exit_code) PURE;
    STDMETHOD(LoadModule)(THIS_ ULONG64 image_handle, ULONG64 base_offset, ULONG module_size,  const char *module_name,
            const char *image_name, ULONG checksum, ULONG timedatestamp) PURE;
    STDMETHOD(UnloadModule)(THIS_ const char *image_basename, ULONG64 base_offset) PURE;
    STDMETHOD(SystemError)(THIS_ ULONG error, ULONG level) PURE;
    STDMETHOD(SessionStatus)(THIS_ ULONG status) PURE;
    STDMETHOD(ChangeDebuggeeState)(THIS_ ULONG flags, ULONG64 argument) PURE;
    STDMETHOD(ChangeEngineState)(THIS_ ULONG flags, ULONG64 argument) PURE;
    STDMETHOD(ChangeSymbolState)(THIS_ ULONG flags, ULONG64 argument) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugClient
DECLARE_INTERFACE_(IDebugClient, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugClient */
    STDMETHOD(AttachKernel)(THIS_ ULONG flags, const char *options) PURE;
    STDMETHOD(GetKernelConnectionOptions)(THIS_ char *buffer, ULONG buffer_size, ULONG *options_size) PURE;
    STDMETHOD(SetKernelConnectionOptions)(THIS_ const char *options) PURE;
    STDMETHOD(StartProcessServer)(THIS_ ULONG flags, const char *options, void *reserved) PURE;
    STDMETHOD(ConnectProcessServer)(THIS_ const char *remote_options, ULONG64 *server) PURE;
    STDMETHOD(DisconnectProcessServer)(THIS_ ULONG64 server) PURE;
    STDMETHOD(GetRunningProcessSystemIds)(THIS_ ULONG64 server, ULONG *ids, ULONG count, ULONG *actual_count) PURE;
    STDMETHOD(GetRunningProcessSystemIdByExecutableName)(THIS_ ULONG64 server, const char *exe_name,
            ULONG flags, ULONG *id) PURE;
    STDMETHOD(GetRunningProcessDescription)(THIS_ ULONG64 server, ULONG systemid, ULONG flags, char *exe_name,
            ULONG exe_name_size, ULONG *actual_exe_name_size, char *description, ULONG description_size,
            ULONG *actual_description_size) PURE;
    STDMETHOD(AttachProcess)(THIS_ ULONG64 server, ULONG pid, ULONG flags) PURE;
    STDMETHOD(CreateProcess)(THIS_ ULONG64 server, char *cmdline, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttach)(THIS_ ULONG64 server, char *cmdline, ULONG create_flags,
            ULONG pid, ULONG attach_flags) PURE;
    STDMETHOD(GetProcessOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(OpenDumpFile)(THIS_ const char *filename) PURE;
    STDMETHOD(WriteDumpFile)(THIS_ const char *filename, ULONG qualifier) PURE;
    STDMETHOD(ConnectSession)(THIS_ ULONG flags, ULONG history_limit) PURE;
    STDMETHOD(StartServer)(THIS_ const char *options) PURE;
    STDMETHOD(OutputServers)(THIS_ ULONG output_control, const char *machine, ULONG flags) PURE;
    STDMETHOD(TerminateProcesses)(THIS) PURE;
    STDMETHOD(DetachProcesses)(THIS) PURE;
    STDMETHOD(EndSession)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetExitCode)(THIS_ ULONG *code) PURE;
    STDMETHOD(DispatchCallbacks)(THIS_ ULONG timeout) PURE;
    STDMETHOD(ExitDispatch)(THIS_ IDebugClient *client) PURE;
    STDMETHOD(CreateClient)(THIS_ IDebugClient **client) PURE;
    STDMETHOD(GetInputCallbacks)(THIS_ IDebugInputCallbacks **callbacks) PURE;
    STDMETHOD(SetInputCallbacks)(THIS_ IDebugInputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputCallbacks)(THIS_ IDebugOutputCallbacks **callbacks) PURE;
    STDMETHOD(SetOutputCallbacks)(THIS_ IDebugOutputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(SetOutputMask)(THIS_ ULONG mask) PURE;
    STDMETHOD(GetOtherOutputMask)(THIS_ IDebugClient *client, ULONG *mask) PURE;
    STDMETHOD(SetOtherOutputMask)(THIS_ IDebugClient *client, ULONG mask) PURE;
    STDMETHOD(GetOutputWidth)(THIS_ ULONG *columns) PURE;
    STDMETHOD(SetOutputWidth)(THIS_ ULONG columns) PURE;
    STDMETHOD(GetOutputLinePrefix)(THIS_ char *buffer, ULONG buffer_size, ULONG *prefix_size) PURE;
    STDMETHOD(SetOutputLinePrefix)(THIS_ const char *prefix) PURE;
    STDMETHOD(GetIdentity)(THIS_ char *buffer, ULONG buffer_size, ULONG *identity_size) PURE;
    STDMETHOD(OutputIdentity)(THIS_ ULONG output_control, ULONG flags, const char *format) PURE;
    STDMETHOD(GetEventCallbacks)(THIS_ IDebugEventCallbacks **callbacks) PURE;
    STDMETHOD(SetEventCallbacks)(THIS_ IDebugEventCallbacks *callbacks) PURE;
    STDMETHOD(FlushCallbacks)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugDataSpaces
DECLARE_INTERFACE_(IDebugDataSpaces, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugDataSpaces */
    STDMETHOD(ReadVirtual)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteVirtual)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(SearchVirtual)(THIS_ ULONG64 offset, ULONG64 length, void *pattern, ULONG pattern_size,
            ULONG pattern_granularity, ULONG64 *ret_offset) PURE;
    STDMETHOD(ReadVirtualUncached)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteVirtualUncached)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(ReadPointersVirtual)(THIS_ ULONG count, ULONG64 offset, ULONG64 *pointers) PURE;
    STDMETHOD(WritePointersVirtual)(THIS_ ULONG count, ULONG64 offset, ULONG64 *pointers) PURE;
    STDMETHOD(ReadPhysical)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WritePhysical)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(ReadControl)(THIS_ ULONG processor, ULONG64 offset, void *buffer, ULONG buffer_size,
            ULONG *read_len) PURE;
    STDMETHOD(WriteControl)(THIS_ ULONG processor, ULONG64 offset, void *buffer, ULONG buffer_size,
            ULONG *written) PURE;
    STDMETHOD(ReadIo)(THIS_ ULONG type, ULONG bus_number, ULONG address_space, ULONG64 offset, void *buffer,
            ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteIo)(THIS_ ULONG type, ULONG bus_number, ULONG address_space, ULONG64 offset, void *buffer,
            ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(ReadMsr)(THIS_ ULONG msr, ULONG64 *value) PURE;
    STDMETHOD(WriteMsr)(THIS_ ULONG msr, ULONG64 value) PURE;
    STDMETHOD(ReadBusData)(THIS_ ULONG data_type, ULONG bus_number, ULONG slot_number, ULONG offset, void *buffer,
            ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteBusData)(THIS_ ULONG data_type, ULONG bus_number, ULONG slot_number, ULONG offset, void *buffer,
            ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(CheckLowMemory)(THIS) PURE;
    STDMETHOD(ReadDebuggerData)(THIS_ ULONG index, void *buffer, ULONG buffer_size, ULONG *data_size) PURE;
    STDMETHOD(ReadProcessorSystemData)(THIS_ ULONG processor, ULONG index, void *buffer, ULONG buffer_size,
            ULONG *data_size) PURE;
};
#undef INTERFACE

#ifdef __cplusplus
}
#endif
