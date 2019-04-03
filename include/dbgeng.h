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
DEFINE_GUID(IID_IDebugSymbols,            0x8c31e98c, 0x983a, 0x48a5, 0x90, 0x16, 0x6f, 0xe5, 0xd6, 0x67, 0xa9, 0x50);
DEFINE_GUID(IID_IDebugControl,            0x5182e668, 0x105e, 0x416e, 0xad, 0x92, 0x24, 0xef, 0x80, 0x04, 0x24, 0xba);
DEFINE_GUID(IID_IDebugControl2,           0xd4366723, 0x44df, 0x4bed, 0x8c, 0x7e, 0x4c, 0x05, 0x42, 0x4f, 0x45, 0x88);

typedef struct _DEBUG_MODULE_PARAMETERS
{
    ULONG64 Base;
    ULONG Size;
    ULONG TimeDateStamp;
    ULONG Checksum;
    ULONG Flags;
    ULONG SymbolType;
    ULONG ImageNameSize;
    ULONG ModuleNameSize;
    ULONG LoadedImageNameSize;
    ULONG SymbolFileNameSize;
    ULONG MappedImageNameSize;
    ULONG64 Reserved[2];
} DEBUG_MODULE_PARAMETERS, *PDEBUG_MODULE_PARAMETERS;

typedef struct _DEBUG_STACK_FRAME
{
    ULONG64 InstructionOffset;
    ULONG64 ReturnOffset;
    ULONG64 FrameOffset;
    ULONG64 StackOffset;
    ULONG64 FuncTableEntry;
    ULONG64 Params[4];
    ULONG64 Reserved[6];
    BOOL Virtual;
    ULONG FrameNumber;
} DEBUG_STACK_FRAME, *PDEBUG_STACK_FRAME;

typedef struct _DEBUG_VALUE
{
    union
    {
        UCHAR I8;
        USHORT I16;
        ULONG I32;
        struct
        {
            ULONG64 I64;
            BOOL Nat;
        };
        float F32;
        double F64;
        UCHAR F80Bytes[10];
        UCHAR F82Bytes[11];
        UCHAR F128Bytes[16];
        UCHAR VI8[16];
        USHORT VI16[8];
        ULONG VI32[4];
        ULONG64 VI64[2];
        float VF32[4];
        double VF64[2];
        struct
        {
            ULONG LowPart;
            ULONG HighPart;
        } I64Parts32;
        struct
        {
            ULONG64 LowPart;
            LONG64 HighPart;
        } F128Parts64;
        UCHAR RawBytes[24];
    };
    ULONG TailOfRawBytes;
    ULONG Type;
} DEBUG_VALUE, *PDEBUG_VALUE;

typedef struct _DEBUG_BREAKPOINT_PARAMETERS
{
    ULONG64 Offset;
    ULONG Id;
    ULONG BreakType;
    ULONG ProcType;
    ULONG Flags;
    ULONG DataSize;
    ULONG DataAccessType;
    ULONG PassCount;
    ULONG CurrentPassCount;
    ULONG MatchThread;
    ULONG CommandSize;
    ULONG OffsetExpressionSize;
} DEBUG_BREAKPOINT_PARAMETERS, *PDEBUG_BREAKPOINT_PARAMETERS;

typedef struct _WINDBG_EXTENSION_APIS32 *PWINDBG_EXTENSION_APIS32;
typedef struct _WINDBG_EXTENSION_APIS64 *PWINDBG_EXTENSION_APIS64;

typedef struct _DEBUG_SPECIFIC_FILTER_PARAMETERS
{
    ULONG ExecutionOption;
    ULONG ContinueOption;
    ULONG TextSize;
    ULONG CommandSize;
    ULONG ArgumentSize;
} DEBUG_SPECIFIC_FILTER_PARAMETERS, *PDEBUG_SPECIFIC_FILTER_PARAMETERS;

typedef struct _DEBUG_EXCEPTION_FILTER_PARAMETERS
{
    ULONG ExecutionOption;
    ULONG ContinueOption;
    ULONG TextSize;
    ULONG CommandSize;
    ULONG SecondCommandSize;
    ULONG ExceptionCode;
} DEBUG_EXCEPTION_FILTER_PARAMETERS, *PDEBUG_EXCEPTION_FILTER_PARAMETERS;

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

#define INTERFACE IDebugSymbolGroup
DECLARE_INTERFACE_(IDebugSymbolGroup, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugSymbolGroup */
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

#define INTERFACE IDebugSymbols
DECLARE_INTERFACE_(IDebugSymbols, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugSymbols */
    STDMETHOD(GetSymbolOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddSymbolOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveSymbolOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetSymbolOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(GetNameByOffset)(THIS_ ULONG64 offset, char *buffer, ULONG buffer_size, ULONG *name_size,
            ULONG64 *displacement) PURE;
    STDMETHOD(GetOffsetByName)(THIS_ const char *symbol, ULONG64 *offset) PURE;
    STDMETHOD(GetNearNameByOffset)(THIS_ ULONG64 offset, LONG delta, char *buffer, ULONG buffer_size,ULONG *name_size,
            ULONG64 *displacement) PURE;
    STDMETHOD(GetLineByOffset)(THIS_ ULONG64 offset, ULONG *line, char *buffer, ULONG buffer_size, ULONG *file_size,
            ULONG64 *displacement) PURE;
    STDMETHOD(GetOffsetByLine)(THIS_ ULONG line, const char *file, ULONG64 *offset) PURE;
    STDMETHOD(GetNumberModules)(THIS_ ULONG *loaded, ULONG *unloaded) PURE;
    STDMETHOD(GetModuleByIndex)(THIS_ ULONG index, ULONG64 *base) PURE;
    STDMETHOD(GetModuleByModuleName)(THIS_ const char *name, ULONG start_index, ULONG *index, ULONG64 *base) PURE;
    STDMETHOD(GetModuleByOffset)(THIS_ ULONG64 offset, ULONG start_index, ULONG *index, ULONG64 *base) PURE;
    STDMETHOD(GetModuleNames)(THIS_ ULONG index, ULONG64 base, char *image_name, ULONG image_name_buffer_size,
            ULONG *image_name_size, char *module_name, ULONG module_name_buffer_size, ULONG *module_name_size,
            char *loaded_image_name, ULONG loaded_image_name_buffer_size, ULONG *loaded_image_size) PURE;
    STDMETHOD(GetModuleParameters)(THIS_ ULONG count, ULONG64 *bases, ULONG start,
            DEBUG_MODULE_PARAMETERS *parameters) PURE;
    STDMETHOD(GetSymbolModule)(THIS_ const char *symbol, ULONG64 *base) PURE;
    STDMETHOD(GetTypeName)(THIS_ ULONG64 base, ULONG type_id, char *buffer, ULONG buffer_size, ULONG *name_size) PURE;
    STDMETHOD(GetTypeId)(THIS_ ULONG64 base, ULONG type_id, ULONG *size) PURE;
    STDMETHOD(GetFieldOffset)(THIS_ ULONG64 base, ULONG type_id, const char *field, ULONG *offset) PURE;
    STDMETHOD(GetSymbolTypeId)(THIS_ const char *symbol, ULONG *type_id, ULONG64 *base) PURE;
    STDMETHOD(GetOffsetTypeId)(THIS_ ULONG64 offset, ULONG *type_id, ULONG64 *base) PURE;
    STDMETHOD(ReadTypedDataVirtual)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteTypedDataVirtual)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(OutputTypedDataVirtual)(THIS_ ULONG output_control, ULONG64 offset, ULONG64 base, ULONG type_id,
            ULONG flags) PURE;
    STDMETHOD(ReadTypedDataPhysical)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteTypedDataPhysical)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(OutputTypedDataPhysical)(THIS_ ULONG output_control, ULONG64 offset, ULONG64 base, ULONG type_id,
            ULONG flags) PURE;
    STDMETHOD(GetScope)(THIS_ ULONG64 *instr_offset, DEBUG_STACK_FRAME *frame, void *scope_context,
            ULONG scope_context_size) PURE;
    STDMETHOD(SetScope)(THIS_ ULONG64 instr_offset, DEBUG_STACK_FRAME *frame, void *scope_context,
            ULONG scope_context_size) PURE;
    STDMETHOD(ResetScope)(THIS) PURE;
    STDMETHOD(GetScopeSymbolGroup)(THIS_ ULONG flags, IDebugSymbolGroup *update, IDebugSymbolGroup **symbols) PURE;
    STDMETHOD(CreateSymbolGroup)(THIS_ IDebugSymbolGroup **group) PURE;
    STDMETHOD(StartSymbolMatch)(THIS_ const char *pattern, ULONG64 *handle) PURE;
    STDMETHOD(GetNextSymbolMatch)(THIS_ ULONG64 handle, char *buffer, ULONG buffer_size, ULONG *match_size,
            ULONG64 *offset) PURE;
    STDMETHOD(EndSymbolMatch)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(Reload)(THIS_ const char *path) PURE;
    STDMETHOD(GetSymbolPath)(THIS_ char *buffer, ULONG buffer_size, ULONG *path_size) PURE;
    STDMETHOD(SetSymbolPath)(THIS_ const char *path) PURE;
    STDMETHOD(AppendSymbolPath)(THIS_ const char *path) PURE;
    STDMETHOD(GetImagePath)(THIS_ char *buffer, ULONG buffer_size, ULONG *path_size) PURE;
    STDMETHOD(SetImagePath)(THIS_ const char *path) PURE;
    STDMETHOD(AppendImagePath)(THIS_ const char *path) PURE;
    STDMETHOD(GetSourcePath)(THIS_ char *buffer, ULONG buffer_size, ULONG *path_size) PURE;
    STDMETHOD(GetSourcePathElement)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *element_size) PURE;
    STDMETHOD(SetSourcePath)(THIS_ const char *path) PURE;
    STDMETHOD(AppendSourcePath)(THIS_ const char *path) PURE;
    STDMETHOD(FindSourceFile)(THIS_ ULONG start, const char *file, ULONG flags, ULONG *found_element, char *buffer,
            ULONG buffer_size, ULONG *found_size) PURE;
    STDMETHOD(GetSourceFileLineOffsets)(THIS_ const char *file, ULONG64 *buffer, ULONG buffer_lines,
            ULONG *file_lines) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugControl
DECLARE_INTERFACE_(IDebugControl, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugControl */
    STDMETHOD(GetInterrupt)(THIS) PURE;
    STDMETHOD(SetInterrupt)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetIterruptTimeout)(THIS_ ULONG *timeout) PURE;
    STDMETHOD(SetInterruptTimeout)(THIS_ ULONG timeout) PURE;
    STDMETHOD(GetLogFile)(THIS_ char *buffer, ULONG buffer_size, ULONG *file_size, BOOL *append) PURE;
    STDMETHOD(OpenLogFile)(THIS_ const char *file, BOOL append) PURE;
    STDMETHOD(CloseLogFile)(THIS) PURE;
    STDMETHOD(GetLogMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(SetLogMask)(THIS_ ULONG mask) PURE;
    STDMETHOD(Input)(THIS_ char *buffer, ULONG buffer_size, ULONG *input_size) PURE;
    STDMETHOD(ReturnInput)(THIS_ const char *buffer) PURE;
    STDMETHODV(Output)(THIS_ ULONG mask, const char *format, ...) PURE;
    STDMETHOD(OutputVaList)(THIS_ ULONG mask, const char *format, __ms_va_list args) PURE;
    STDMETHODV(ControlledOutput)(THIS_ ULONG output_control, ULONG mask, const char *format, ...) PURE;
    STDMETHOD(ControlledOutputVaList)(THIS_ ULONG output_control, ULONG mask, const char *format,
            __ms_va_list args) PURE;
    STDMETHODV(OutputPrompt)(THIS_ ULONG output_control, const char *format, ...) PURE;
    STDMETHOD(OutputPromptVaList)(THIS_ ULONG output_control, const char *format, __ms_va_list args) PURE;
    STDMETHOD(GetPromptText)(THIS_ char *buffer, ULONG buffer_size, ULONG *text_size) PURE;
    STDMETHOD(OutputCurrentState)(THIS_ ULONG output_control, ULONG flags) PURE;
    STDMETHOD(OutputVersionInformation)(THIS_ ULONG output_control) PURE;
    STDMETHOD(GetNotifyEventHandle)(THIS_ ULONG64 *handle) PURE;
    STDMETHOD(SetNotifyEventHandle)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(Assemble)(THIS_ ULONG64 offset, const char *code, ULONG64 *end_offset) PURE;
    STDMETHOD(Disassemble)(THIS_ ULONG64 offset, ULONG flags, char *buffer, ULONG buffer_size, ULONG *disassm_size,
            ULONG64 *end_offset) PURE;
    STDMETHOD(GetDisassembleEffectiveOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(OutputDisassembly)(THIS_ ULONG output_control, ULONG64 offset, ULONG flags, ULONG64 *end_offset) PURE;
    STDMETHOD(OutputDisassemblyLines)(THIS_ ULONG output_control, ULONG prev_lines, ULONG total_lines, ULONG64 offset,
            ULONG flags, ULONG *offset_line, ULONG64 *start_offset, ULONG64 *end_offset, ULONG64 *line_offsets) PURE;
    STDMETHOD(GetNearInstruction)(THIS_ ULONG64 offset, LONG delta, ULONG64 *instr_offset) PURE;
    STDMETHOD(GetStackTrace)(THIS_ ULONG64 frame_offset, ULONG64 stack_offset, ULONG64 instr_offset,
            DEBUG_STACK_FRAME *frames, ULONG frames_size, ULONG *frames_filled) PURE;
    STDMETHOD(GetReturnOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(OutputStackTrace)(THIS_ ULONG output_control, DEBUG_STACK_FRAME *frames, ULONG frames_size,
            ULONG flags) PURE;
    STDMETHOD(GetDebuggeeType)(THIS_ ULONG *_class, ULONG *qualifier) PURE;
    STDMETHOD(GetActualProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(GetExecutingProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(GetNumberPossibleExecutingProcessorTypes)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetPossibleExecutingProcessorTypes)(THIS_ ULONG start, ULONG count, ULONG *types) PURE;
    STDMETHOD(GetNumberProcessors)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetSystemVersion)(THIS_ ULONG *platform_id, ULONG *major, ULONG *minor, char *sp_string,
            ULONG sp_string_size, ULONG *sp_string_used, ULONG *sp_number, char *build_string, ULONG build_string_size,
            ULONG *build_string_used) PURE;
    STDMETHOD(GetPageSize)(THIS_ ULONG *size) PURE;
    STDMETHOD(IsPointer64Bit)(THIS) PURE;
    STDMETHOD(ReadBugCheckData)(THIS_ ULONG *code, ULONG64 *arg1, ULONG64 *arg2, ULONG64 *arg3, ULONG64 *arg4) PURE;
    STDMETHOD(GetNumberSupportedProcessorTypes)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetSupportedProcessorTypes)(THIS_ ULONG start, ULONG count, ULONG *types) PURE;
    STDMETHOD(GetProcessorTypeNames)(THIS_ ULONG type, char *full_name, ULONG full_name_buffer_size,
            ULONG *full_name_size, char *abbrev_name, ULONG abbrev_name_buffer_size, ULONG *abbrev_name_size) PURE;
    STDMETHOD(GetEffectiveProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(SetEffectiveProcessorType)(THIS_ ULONG type) PURE;
    STDMETHOD(GetExecutionStatus)(THIS_ ULONG *status) PURE;
    STDMETHOD(SetExecutionStatus)(THIS_ ULONG status) PURE;
    STDMETHOD(GetCodeLevel)(THIS_ ULONG *level) PURE;
    STDMETHOD(SetCodeLevel)(THIS_ ULONG level) PURE;
    STDMETHOD(GetEngineOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(GetSystemErrorControl)(THIS_ ULONG *output_level, ULONG *break_level) PURE;
    STDMETHOD(SetSystemErrorControl)(THIS_ ULONG output_level, ULONG break_level) PURE;
    STDMETHOD(GetTextMacro)(THIS_ ULONG slot, char *buffer, ULONG buffer_size, ULONG *macro_size) PURE;
    STDMETHOD(SetTextMacro)(THIS_ ULONG slot, const char *macro) PURE;
    STDMETHOD(GetRadix)(THIS_ ULONG *radix) PURE;
    STDMETHOD(SetRadix)(THIS_ ULONG radix) PURE;
    STDMETHOD(Evaluate)(THIS_ const char *expression, ULONG desired_type, DEBUG_VALUE *value,
            ULONG *remainder_index) PURE;
    STDMETHOD(CoerceValue)(THIS_ DEBUG_VALUE input, ULONG output_type, DEBUG_VALUE *output) PURE;
    STDMETHOD(CoerceValues)(THIS_ ULONG count, DEBUG_VALUE *input, ULONG *output_types, DEBUG_VALUE *output) PURE;
    STDMETHOD(Execute)(THIS_ ULONG output_control, const char *command, ULONG flags) PURE;
    STDMETHOD(ExecuteCommandFile)(THIS_ ULONG output_control, const char *command_file, ULONG flags) PURE;
    STDMETHOD(GetNumberBreakpoints)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetBreakpointByIndex)(THIS_ ULONG index, IDebugBreakpoint **bp) PURE;
    STDMETHOD(GetBreakpointById)(THIS_ ULONG id, IDebugBreakpoint **bp) PURE;
    STDMETHOD(GetBreakpointParameters)(THIS_ ULONG count, ULONG *ids, ULONG start,
            DEBUG_BREAKPOINT_PARAMETERS *parameters) PURE;
    STDMETHOD(AddBreakpoint)(THIS_ ULONG type, ULONG desired_id, IDebugBreakpoint **bp) PURE;
    STDMETHOD(RemoveBreakpoint)(THIS_ IDebugBreakpoint *bp) PURE;
    STDMETHOD(AddExtension)(THIS_ const char *path, ULONG flags, ULONG64 *handle) PURE;
    STDMETHOD(RemoveExtension)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(GetExtensionByPath)(THIS_ const char *path, ULONG64 *handle) PURE;
    STDMETHOD(CallExtension)(THIS_ ULONG64 handle, const char *function, const char *args) PURE;
    STDMETHOD(GetExtensionFunction)(THIS_ ULONG64 handle, const char *name, void *function) PURE;
    STDMETHOD(GetWindbgExtensionApis32)(THIS_ PWINDBG_EXTENSION_APIS32 api) PURE;
    STDMETHOD(GetWindbgExtensionApis64)(THIS_ PWINDBG_EXTENSION_APIS64 api) PURE;
    STDMETHOD(GetNumberEventFilters)(THIS_ ULONG *specific_events, ULONG *specific_exceptions,
            ULONG *arbitrary_exceptions) PURE;
    STDMETHOD(GetEventFilterText)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *text_size) PURE;
    STDMETHOD(GetEventFilterCommand)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *command_size) PURE;
    STDMETHOD(SetEventFilterCommand)(THIS_ ULONG index, const char *command) PURE;
    STDMETHOD(GetSpecificFilterParameters)(THIS_ ULONG start, ULONG count,
            DEBUG_SPECIFIC_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(SetSpecificFilterParameters)(THIS_ ULONG start, ULONG count,
            DEBUG_SPECIFIC_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(GetSpecificFilterArgument)(THIS_ ULONG index, char *buffer, ULONG buffer_size,
            ULONG *argument_size) PURE;
    STDMETHOD(SetSpecificFilterArgument)(THIS_ ULONG index, const char *argument) PURE;
    STDMETHOD(GetExceptionFilterParameters)(THIS_ ULONG count, ULONG *codes, ULONG start,
            DEBUG_EXCEPTION_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(SetExceptionFilterParameters)(THIS_ ULONG count, DEBUG_EXCEPTION_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(GetExceptionFilterSecondCommand)(THIS_ ULONG index, char *buffer, ULONG buffer_size,
            ULONG *command_size) PURE;
    STDMETHOD(SetExceptionFilterSecondCommand)(THIS_ ULONG index, const char *command) PURE;
    STDMETHOD(WaitForEvent)(THIS_ ULONG flags, ULONG timeout) PURE;
    STDMETHOD(GetLastEventInformation)(THIS_ ULONG *type, ULONG *pid, ULONG *tid, void *extra_info,
            ULONG extra_info_size, ULONG *extra_info_used, char *description, ULONG desc_size, ULONG *desc_used) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugControl2
DECLARE_INTERFACE_(IDebugControl2, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugControl */
    STDMETHOD(GetInterrupt)(THIS) PURE;
    STDMETHOD(SetInterrupt)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetIterruptTimeout)(THIS_ ULONG *timeout) PURE;
    STDMETHOD(SetInterruptTimeout)(THIS_ ULONG timeout) PURE;
    STDMETHOD(GetLogFile)(THIS_ char *buffer, ULONG buffer_size, ULONG *file_size, BOOL *append) PURE;
    STDMETHOD(OpenLogFile)(THIS_ const char *file, BOOL append) PURE;
    STDMETHOD(CloseLogFile)(THIS) PURE;
    STDMETHOD(GetLogMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(SetLogMask)(THIS_ ULONG mask) PURE;
    STDMETHOD(Input)(THIS_ char *buffer, ULONG buffer_size, ULONG *input_size) PURE;
    STDMETHOD(ReturnInput)(THIS_ const char *buffer) PURE;
    STDMETHODV(Output)(THIS_ ULONG mask, const char *format, ...) PURE;
    STDMETHOD(OutputVaList)(THIS_ ULONG mask, const char *format, __ms_va_list args) PURE;
    STDMETHODV(ControlledOutput)(THIS_ ULONG output_control, ULONG mask, const char *format, ...) PURE;
    STDMETHOD(ControlledOutputVaList)(THIS_ ULONG output_control, ULONG mask, const char *format,
            __ms_va_list args) PURE;
    STDMETHODV(OutputPrompt)(THIS_ ULONG output_control, const char *format, ...) PURE;
    STDMETHOD(OutputPromptVaList)(THIS_ ULONG output_control, const char *format, __ms_va_list args) PURE;
    STDMETHOD(GetPromptText)(THIS_ char *buffer, ULONG buffer_size, ULONG *text_size) PURE;
    STDMETHOD(OutputCurrentState)(THIS_ ULONG output_control, ULONG flags) PURE;
    STDMETHOD(OutputVersionInformation)(THIS_ ULONG output_control) PURE;
    STDMETHOD(GetNotifyEventHandle)(THIS_ ULONG64 *handle) PURE;
    STDMETHOD(SetNotifyEventHandle)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(Assemble)(THIS_ ULONG64 offset, const char *code, ULONG64 *end_offset) PURE;
    STDMETHOD(Disassemble)(THIS_ ULONG64 offset, ULONG flags, char *buffer, ULONG buffer_size, ULONG *disassm_size,
            ULONG64 *end_offset) PURE;
    STDMETHOD(GetDisassembleEffectiveOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(OutputDisassembly)(THIS_ ULONG output_control, ULONG64 offset, ULONG flags, ULONG64 *end_offset) PURE;
    STDMETHOD(OutputDisassemblyLines)(THIS_ ULONG output_control, ULONG prev_lines, ULONG total_lines, ULONG64 offset,
            ULONG flags, ULONG *offset_line, ULONG64 *start_offset, ULONG64 *end_offset, ULONG64 *line_offsets) PURE;
    STDMETHOD(GetNearInstruction)(THIS_ ULONG64 offset, LONG delta, ULONG64 *instr_offset) PURE;
    STDMETHOD(GetStackTrace)(THIS_ ULONG64 frame_offset, ULONG64 stack_offset, ULONG64 instr_offset,
            DEBUG_STACK_FRAME *frames, ULONG frames_size, ULONG *frames_filled) PURE;
    STDMETHOD(GetReturnOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(OutputStackTrace)(THIS_ ULONG output_control, DEBUG_STACK_FRAME *frames, ULONG frames_size,
            ULONG flags) PURE;
    STDMETHOD(GetDebuggeeType)(THIS_ ULONG *_class, ULONG *qualifier) PURE;
    STDMETHOD(GetActualProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(GetExecutingProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(GetNumberPossibleExecutingProcessorTypes)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetPossibleExecutingProcessorTypes)(THIS_ ULONG start, ULONG count, ULONG *types) PURE;
    STDMETHOD(GetNumberProcessors)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetSystemVersion)(THIS_ ULONG *platform_id, ULONG *major, ULONG *minor, char *sp_string,
            ULONG sp_string_size, ULONG *sp_string_used, ULONG *sp_number, char *build_string, ULONG build_string_size,
            ULONG *build_string_used) PURE;
    STDMETHOD(GetPageSize)(THIS_ ULONG *size) PURE;
    STDMETHOD(IsPointer64Bit)(THIS) PURE;
    STDMETHOD(ReadBugCheckData)(THIS_ ULONG *code, ULONG64 *arg1, ULONG64 *arg2, ULONG64 *arg3, ULONG64 *arg4) PURE;
    STDMETHOD(GetNumberSupportedProcessorTypes)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetSupportedProcessorTypes)(THIS_ ULONG start, ULONG count, ULONG *types) PURE;
    STDMETHOD(GetProcessorTypeNames)(THIS_ ULONG type, char *full_name, ULONG full_name_buffer_size,
            ULONG *full_name_size, char *abbrev_name, ULONG abbrev_name_buffer_size, ULONG *abbrev_name_size) PURE;
    STDMETHOD(GetEffectiveProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(SetEffectiveProcessorType)(THIS_ ULONG type) PURE;
    STDMETHOD(GetExecutionStatus)(THIS_ ULONG *status) PURE;
    STDMETHOD(SetExecutionStatus)(THIS_ ULONG status) PURE;
    STDMETHOD(GetCodeLevel)(THIS_ ULONG *level) PURE;
    STDMETHOD(SetCodeLevel)(THIS_ ULONG level) PURE;
    STDMETHOD(GetEngineOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(GetSystemErrorControl)(THIS_ ULONG *output_level, ULONG *break_level) PURE;
    STDMETHOD(SetSystemErrorControl)(THIS_ ULONG output_level, ULONG break_level) PURE;
    STDMETHOD(GetTextMacro)(THIS_ ULONG slot, char *buffer, ULONG buffer_size, ULONG *macro_size) PURE;
    STDMETHOD(SetTextMacro)(THIS_ ULONG slot, const char *macro) PURE;
    STDMETHOD(GetRadix)(THIS_ ULONG *radix) PURE;
    STDMETHOD(SetRadix)(THIS_ ULONG radix) PURE;
    STDMETHOD(Evaluate)(THIS_ const char *expression, ULONG desired_type, DEBUG_VALUE *value,
            ULONG *remainder_index) PURE;
    STDMETHOD(CoerceValue)(THIS_ DEBUG_VALUE input, ULONG output_type, DEBUG_VALUE *output) PURE;
    STDMETHOD(CoerceValues)(THIS_ ULONG count, DEBUG_VALUE *input, ULONG *output_types, DEBUG_VALUE *output) PURE;
    STDMETHOD(Execute)(THIS_ ULONG output_control, const char *command, ULONG flags) PURE;
    STDMETHOD(ExecuteCommandFile)(THIS_ ULONG output_control, const char *command_file, ULONG flags) PURE;
    STDMETHOD(GetNumberBreakpoints)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetBreakpointByIndex)(THIS_ ULONG index, IDebugBreakpoint **bp) PURE;
    STDMETHOD(GetBreakpointById)(THIS_ ULONG id, IDebugBreakpoint **bp) PURE;
    STDMETHOD(GetBreakpointParameters)(THIS_ ULONG count, ULONG *ids, ULONG start,
            DEBUG_BREAKPOINT_PARAMETERS *parameters) PURE;
    STDMETHOD(AddBreakpoint)(THIS_ ULONG type, ULONG desired_id, IDebugBreakpoint **bp) PURE;
    STDMETHOD(RemoveBreakpoint)(THIS_ IDebugBreakpoint *bp) PURE;
    STDMETHOD(AddExtension)(THIS_ const char *path, ULONG flags, ULONG64 *handle) PURE;
    STDMETHOD(RemoveExtension)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(GetExtensionByPath)(THIS_ const char *path, ULONG64 *handle) PURE;
    STDMETHOD(CallExtension)(THIS_ ULONG64 handle, const char *function, const char *args) PURE;
    STDMETHOD(GetExtensionFunction)(THIS_ ULONG64 handle, const char *name, void *function) PURE;
    STDMETHOD(GetWindbgExtensionApis32)(THIS_ PWINDBG_EXTENSION_APIS32 api) PURE;
    STDMETHOD(GetWindbgExtensionApis64)(THIS_ PWINDBG_EXTENSION_APIS64 api) PURE;
    STDMETHOD(GetNumberEventFilters)(THIS_ ULONG *specific_events, ULONG *specific_exceptions,
            ULONG *arbitrary_exceptions) PURE;
    STDMETHOD(GetEventFilterText)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *text_size) PURE;
    STDMETHOD(GetEventFilterCommand)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *command_size) PURE;
    STDMETHOD(SetEventFilterCommand)(THIS_ ULONG index, const char *command) PURE;
    STDMETHOD(GetSpecificFilterParameters)(THIS_ ULONG start, ULONG count,
            DEBUG_SPECIFIC_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(SetSpecificFilterParameters)(THIS_ ULONG start, ULONG count,
            DEBUG_SPECIFIC_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(GetSpecificFilterArgument)(THIS_ ULONG index, char *buffer, ULONG buffer_size,
            ULONG *argument_size) PURE;
    STDMETHOD(SetSpecificFilterArgument)(THIS_ ULONG index, const char *argument) PURE;
    STDMETHOD(GetExceptionFilterParameters)(THIS_ ULONG count, ULONG *codes, ULONG start,
            DEBUG_EXCEPTION_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(SetExceptionFilterParameters)(THIS_ ULONG count, DEBUG_EXCEPTION_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(GetExceptionFilterSecondCommand)(THIS_ ULONG index, char *buffer, ULONG buffer_size,
            ULONG *command_size) PURE;
    STDMETHOD(SetExceptionFilterSecondCommand)(THIS_ ULONG index, const char *command) PURE;
    STDMETHOD(WaitForEvent)(THIS_ ULONG flags, ULONG timeout) PURE;
    STDMETHOD(GetLastEventInformation)(THIS_ ULONG *type, ULONG *pid, ULONG *tid, void *extra_info,
            ULONG extra_info_size, ULONG *extra_info_used, char *description, ULONG desc_size, ULONG *desc_used) PURE;
    /* IDebugControl2 */
    STDMETHOD(GetCurrentTimeDate)(THIS_ ULONG timedate) PURE;
    STDMETHOD(GetCurrentSystemUpTime)(THIS_ ULONG uptime) PURE;
    STDMETHOD(GetDumpFormatFlags)(THIS_ ULONG *flags) PURE;
    STDMETHOD(GetNumberTextPlacements)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetNumberTextReplacement)(THIS_ const char *src_text, ULONG index, char *src_buffer,
            ULONG src_buffer_size, ULONG *src_size, char *dst_buffer, ULONG dst_buffer_size, ULONG *dst_size) PURE;
    STDMETHOD(SetTextReplacement)(THIS_ const char *src_text, const char *dst_text) PURE;
    STDMETHOD(RemoveTextReplacements)(THIS) PURE;
    STDMETHOD(OutputTextReplacements)(THIS_ ULONG output_control, ULONG flags) PURE;
};
#undef INTERFACE

#ifdef __cplusplus
}
#endif
