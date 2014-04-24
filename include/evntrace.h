/*
 * Copyright (C) 2005 Mike McCormack
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

#ifndef _EVNTRACE_
#define _EVNTRACE_

#include <guiddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_TRACE_CONTROL_QUERY     0
#define EVENT_TRACE_CONTROL_STOP      1
#define EVENT_TRACE_CONTROL_UPDATE    2
#define EVENT_TRACE_CONTROL_FLUSH     3

#define TRACE_LEVEL_NONE              0
#define TRACE_LEVEL_CRITICAL          1
#define TRACE_LEVEL_FATAL             1
#define TRACE_LEVEL_ERROR             2
#define TRACE_LEVEL_WARNING           3
#define TRACE_LEVEL_INFORMATION       4
#define TRACE_LEVEL_VERBOSE           5

typedef ULONG64 TRACEHANDLE, *PTRACEHANDLE;

struct _EVENT_TRACE_LOGFILEA;
struct _EVENT_TRACE_LOGFILEW;

typedef struct _EVENT_TRACE_LOGFILEA EVENT_TRACE_LOGFILEA, *PEVENT_TRACE_LOGFILEA;
typedef struct _EVENT_TRACE_LOGFILEW EVENT_TRACE_LOGFILEW, *PEVENT_TRACE_LOGFILEW;

typedef ULONG (WINAPI * PEVENT_TRACE_BUFFER_CALLBACKA)( PEVENT_TRACE_LOGFILEA );
typedef ULONG (WINAPI * PEVENT_TRACE_BUFFER_CALLBACKW)( PEVENT_TRACE_LOGFILEW );

typedef ULONG (WINAPI * WMIDPREQUEST)( WMIDPREQUESTCODE, PVOID, ULONG*, PVOID );

typedef struct _TRACE_GUID_REGISTRATION
{
    LPCGUID Guid;
    HANDLE RegHandle;
} TRACE_GUID_REGISTRATION, *PTRACE_GUID_REGISTRATION;

typedef struct _TRACE_GUID_PROPERTIES {
    GUID    Guid;
    ULONG   GuidType;
    ULONG   LoggerId;
    ULONG   EnableLevel;
    ULONG   EnableFlags;
    BOOLEAN IsEnable;
} TRACE_GUID_PROPERTIES, *PTRACE_GUID_PROPERTIES;

typedef struct _EVENT_TRACE_HEADER
{
    USHORT Size;
    union
    {
        USHORT FieldTypeFlags;
        struct
        {
            UCHAR HeaderType;
            UCHAR MarkerFlags;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    union
    {
        ULONG Version;
        struct
        {
            UCHAR Type;
            UCHAR Level;
            USHORT Version;
        } Class;
    } DUMMYUNIONNAME1;
    ULONG ThreadId;
    ULONG ProcessId;
    LARGE_INTEGER TimeStamp;
    union
    {
        GUID Guid;
        ULONGLONG GuidPtr;
    } DUMMYUNIONNAME2;
    union
    {
        struct
        {
            ULONG ClientContext;
            ULONG Flags;
        } DUMMYSTRUCTNAME1;
        struct
        {
            ULONG KernelTime;
            ULONG UserTime;
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME3;
} EVENT_TRACE_HEADER, *PEVENT_TRACE_HEADER;

typedef struct _EVENT_TRACE
{
    EVENT_TRACE_HEADER Header;
    ULONG InstanceId;
    ULONG ParentInstanceId;
    GUID ParentGuid;
    PVOID MofData;
    ULONG MofLength;
    ULONG ClientContext;
} EVENT_TRACE, *PEVENT_TRACE;

typedef VOID (WINAPI * PEVENT_CALLBACK)( PEVENT_TRACE );

typedef struct _TRACE_LOGFILE_HEADER
{
    ULONG BufferSize;
    union
    {
        ULONG Version;
        struct 
        {
            UCHAR MajorVersion;
            UCHAR MinorVersion;
            UCHAR SubVersion;
            UCHAR SubMinorVersion;
        } VersionDetail;
    } DUMMYUNIONNAME;
    ULONG ProviderVersion;
    ULONG NumberOfProcessors;
    LARGE_INTEGER EndTime;
    ULONG TimerResolution;
    ULONG MaximumFileSize;
    ULONG LogFileMode;
    ULONG BuffersWritten;
    union
    {
        GUID LogInstanceGuid;
        struct
        {
            ULONG StartBuffers;
            ULONG PointerSize;
            ULONG EventsLost;
            ULONG CpuSpeedInMHZ;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME1;
    LPWSTR LoggerName;
    LPWSTR LogFileName;
    TIME_ZONE_INFORMATION TimeZone;
    LARGE_INTEGER BootTime;
    LARGE_INTEGER PerfFreq;
    LARGE_INTEGER StartTime;
    ULONG ReservedTime;
    ULONG BuffersLost;
} TRACE_LOGFILE_HEADER, *PTRACE_LOGFILE_HEADER;

struct _EVENT_TRACE_LOGFILEW
{
    LPWSTR LogFileName;
    LPWSTR LoggerName;
    LONGLONG CurrentTime;
    ULONG LogFileMode;
    EVENT_TRACE CurrentEvent;
    TRACE_LOGFILE_HEADER LogfileHeader;
    PEVENT_TRACE_BUFFER_CALLBACKW BufferCallback;
    ULONG BufferSize;
    ULONG Filled;
    ULONG EventsLost;
    PEVENT_CALLBACK EventCallback;
    PVOID Context;
};

struct _EVENT_TRACE_LOGFILEA
{
    LPSTR LogFileName;
    LPSTR LoggerName;
    LONGLONG CurrentTime;
    ULONG LogFileMode;
    EVENT_TRACE CurrentEvent;
    TRACE_LOGFILE_HEADER LogfileHeader;
    PEVENT_TRACE_BUFFER_CALLBACKA BufferCallback;
    ULONG BufferSize;
    ULONG Filled;
    ULONG EventsLost;
    PEVENT_CALLBACK EventCallback;
    PVOID Context;
};

typedef struct _EVENT_TRACE_PROPERTIES
{
    WNODE_HEADER Wnode;
    ULONG BufferSize;
    ULONG MinimumBuffers;
    ULONG MaximumBuffers;
    ULONG MaximumFileSize;
    ULONG LogFileMode;
    ULONG FlushTimer;
    LONG AgeLimit;
    ULONG NumberOfBuffers;
    ULONG FreeBuffers;
    ULONG EventsLost;
    ULONG BuffersWritten;
    ULONG LogBuffersLost;
    ULONG RealTimeBuffersLost;
    HANDLE LoggerThreadId;
    ULONG LoggerFileNameOffset;
    ULONG LoggerNameOffset;
} EVENT_TRACE_PROPERTIES, *PEVENT_TRACE_PROPERTIES;

#define INVALID_PROCESSTRACE_HANDLE ((TRACEHANDLE)~(ULONG_PTR)0)

ULONG WINAPI CloseTrace(TRACEHANDLE);
ULONG WINAPI ControlTraceA(TRACEHANDLE,LPCSTR,PEVENT_TRACE_PROPERTIES,ULONG);
ULONG WINAPI ControlTraceW(TRACEHANDLE,LPCWSTR,PEVENT_TRACE_PROPERTIES,ULONG);
#define      ControlTrace WINELIB_NAME_AW(ControlTrace)
ULONG WINAPI EnableTrace(ULONG,ULONG,ULONG,LPCGUID,TRACEHANDLE);
ULONG WINAPI FlushTraceA(TRACEHANDLE,LPCSTR,PEVENT_TRACE_PROPERTIES);
ULONG WINAPI FlushTraceW(TRACEHANDLE,LPCWSTR,PEVENT_TRACE_PROPERTIES);
#define      FlushTrace WINELIB_NAME_AW(FlushTrace)
ULONG WINAPI GetTraceEnableFlags(TRACEHANDLE);
UCHAR WINAPI GetTraceEnableLevel(TRACEHANDLE);
TRACEHANDLE WINAPI GetTraceLoggerHandle(PVOID);
ULONG WINAPI QueryAllTracesA(PEVENT_TRACE_PROPERTIES*,ULONG,PULONG);
ULONG WINAPI QueryAllTracesW(PEVENT_TRACE_PROPERTIES*,ULONG,PULONG);
#define      QueryAllTraces WINELIB_NAME_AW(QueryAllTraces)
ULONG WINAPI RegisterTraceGuidsA(WMIDPREQUEST,PVOID,LPCGUID,ULONG,PTRACE_GUID_REGISTRATION,LPCSTR,LPCSTR,PTRACEHANDLE);
ULONG WINAPI RegisterTraceGuidsW(WMIDPREQUEST,PVOID,LPCGUID,ULONG,PTRACE_GUID_REGISTRATION,LPCWSTR,LPCWSTR,PTRACEHANDLE);
#define      RegisterTraceGuids WINELIB_NAME_AW(RegisterTraceGuids)
ULONG WINAPI StartTraceA(PTRACEHANDLE,LPCSTR,PEVENT_TRACE_PROPERTIES);
ULONG WINAPI StartTraceW(PTRACEHANDLE,LPCWSTR,PEVENT_TRACE_PROPERTIES);
#define      StartTrace WINELIB_NAME_AW(StartTrace)
ULONG WINAPI TraceEvent(TRACEHANDLE,PEVENT_TRACE_HEADER);
ULONG WINAPIV TraceMessage(TRACEHANDLE,ULONG,LPGUID,USHORT,...);
ULONG WINAPI TraceMessageVa(TRACEHANDLE,ULONG,LPGUID,USHORT,__ms_va_list);
ULONG WINAPI UnregisterTraceGuids(TRACEHANDLE);

#ifdef __cplusplus
}
#endif

#endif /* _EVNTRACE_ */
