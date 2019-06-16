/*
 * QuickTime Data Handler
 *
 * Copyright 2011 Aric Stewart for CodeWeavers
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

#include "config.h"

#define ULONG CoreFoundation_ULONG
#define HRESULT CoreFoundation_HRESULT

#define LoadResource __carbon_LoadResource
#define CompareString __carbon_CompareString
#define GetCurrentThread __carbon_GetCurrentThread
#define GetCurrentProcess __carbon_GetCurrentProcess
#define AnimatePalette __carbon_AnimatePalette
#define EqualRgn __carbon_EqualRgn
#define FillRgn __carbon_FillRgn
#define FrameRgn __carbon_FrameRgn
#define GetPixel __carbon_GetPixel
#define InvertRgn __carbon_InvertRgn
#define LineTo __carbon_LineTo
#define OffsetRgn __carbon_OffsetRgn
#define PaintRgn __carbon_PaintRgn
#define Polygon __carbon_Polygon
#define ResizePalette __carbon_ResizePalette
#define SetRectRgn __carbon_SetRectRgn

#define CheckMenuItem __carbon_CheckMenuItem
#define DeleteMenu __carbon_DeleteMenu
#define DrawMenuBar __carbon_DrawMenuBar
#define EnableMenuItem __carbon_EnableMenuItem
#define EqualRect __carbon_EqualRect
#define FillRect __carbon_FillRect
#define FrameRect __carbon_FrameRect
#define GetCursor __carbon_GetCursor
#define GetMenu __carbon_GetMenu
#define InvertRect __carbon_InvertRect
#define IsWindowVisible __carbon_IsWindowVisible
#define MoveWindow __carbon_MoveWindow
#define OffsetRect __carbon_OffsetRect
#define PtInRect __carbon_PtInRect
#define SetCursor __carbon_SetCursor
#define SetRect __carbon_SetRect
#define ShowCursor __carbon_ShowCursor
#define ShowWindow __carbon_ShowWindow
#define UnionRect __carbon_UnionRect

#include <QuickTime/QuickTimeComponents.h>

#undef LoadResource
#undef CompareString
#undef GetCurrentThread
#undef _CDECL
#undef GetCurrentProcess
#undef AnimatePalette
#undef EqualRgn
#undef FillRgn
#undef FrameRgn
#undef GetPixel
#undef InvertRgn
#undef LineTo
#undef OffsetRgn
#undef PaintRgn
#undef Polygon
#undef ResizePalette
#undef SetRectRgn
#undef CheckMenuItem
#undef DeleteMenu
#undef DrawMenuBar
#undef EnableMenuItem
#undef EqualRect
#undef FillRect
#undef FrameRect
#undef GetCursor
#undef GetMenu
#undef InvertRect
#undef IsWindowVisible
#undef MoveWindow
#undef OffsetRect
#undef PtInRect
#undef SetCursor
#undef SetRect
#undef ShowCursor
#undef ShowWindow
#undef UnionRect

#undef ULONG
#undef HRESULT
#undef STDMETHODCALLTYPE

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "winuser.h"
#include "dshow.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include "qtprivate.h"

WINE_DEFAULT_DEBUG_CHANNEL(qtdatahandler);

static ComponentDescription myType =
{
    'dhlr',
    'WINE',
    'WINE',
    0,
    0
};

typedef struct DHData
{
    WineDataRefRecord dataRef;

   Ptr AsyncPtr;
   long AsyncRefCon;
   DataHCompletionUPP AsyncCompletionRtn;
} DHData;

static pascal ComponentResult myComponentRoutineProc ( ComponentParameters *
cp, Handle componentStorage);

void RegisterWineDataHandler( void )
{
    ComponentRoutineUPP MyComponentRoutineUPP;

    MyComponentRoutineUPP = NewComponentRoutineUPP(&myComponentRoutineProc);
    RegisterComponent( &myType , MyComponentRoutineUPP, 0, NULL, NULL, NULL);
}

static pascal ComponentResult myDataHCanUseDataRef ( DataHandler dh,
                                       Handle dataRef,
                                       long *useFlags
)
{
    WineDataRefRecord *record = (WineDataRefRecord*)(*dataRef);
    TRACE("%p %p %p\n",dh,dataRef,useFlags);
    if (record->pReader == NULL)
        return badComponentSelector;
    *useFlags = kDataHCanRead;
    return noErr;
}

static pascal ComponentResult myDataHSetDataRef ( DataHandler dh, Handle dataRef)
{
    Handle storage = GetComponentInstanceStorage(dh);
    DHData *data = (DHData*)*storage;
    WineDataRefRecord* newRef = (WineDataRefRecord*)(*dataRef);
    TRACE("\n");
    if (newRef->pReader != data->dataRef.pReader)
        IAsyncReader_AddRef(newRef->pReader);
    data->dataRef = *newRef;
    return noErr;
}

static pascal ComponentResult myDataHGetAvailableFileSize ( DataHandler dh,
                                                   long *fileSize)
{
    Handle storage = GetComponentInstanceStorage(dh);
    DHData *data = (DHData*)*storage;
    LONGLONG total;
    LONGLONG available;

    TRACE("%p\n",dh);

    IAsyncReader_Length(data->dataRef.pReader,&total,&available);
    *fileSize = available;
    return noErr;
}

static pascal ComponentResult myDataHGetFileSize ( DataHandler dh, long *fileSize)
{
    Handle storage = GetComponentInstanceStorage(dh);
    DHData *data = (DHData*)*storage;
    LONGLONG total;
    LONGLONG available;

    TRACE("%p\n",dh);

    IAsyncReader_Length(data->dataRef.pReader,&total,&available);
    *fileSize = total;
    return noErr;
}

static pascal ComponentResult myDataHScheduleData ( DataHandler dh,
                                             Ptr PlaceToPutDataPtr,
                                             long FileOffset,
                                             long DataSize,
                                             long RefCon,
                                             DataHSchedulePtr scheduleRec,
                                             DataHCompletionUPP CompletionRtn)
{
    Handle storage = GetComponentInstanceStorage(dh);
    DHData *data = (DHData*)*storage;
    HRESULT hr;
    LONGLONG offset = FileOffset;
    BYTE* buffer = (BYTE*)PlaceToPutDataPtr;

    TRACE("%p %p %li %li %li %p %p\n",dh, PlaceToPutDataPtr, FileOffset, DataSize, RefCon, scheduleRec, CompletionRtn);

    hr = IAsyncReader_SyncRead(data->dataRef.pReader, offset, DataSize, buffer);
    TRACE("result %x\n",hr);
    if (CompletionRtn)
    {
        if (data->AsyncCompletionRtn)
            InvokeDataHCompletionUPP(data->AsyncPtr, data->AsyncRefCon, noErr, data->AsyncCompletionRtn);

        data->AsyncPtr = PlaceToPutDataPtr;
        data->AsyncRefCon = RefCon;
        data->AsyncCompletionRtn = CompletionRtn;
    }

    return noErr;
}

static pascal ComponentResult myDataHFinishData (DataHandler dh, Ptr PlaceToPutDataPtr, Boolean Cancel)
{
    Handle storage = GetComponentInstanceStorage(dh);
    DHData *data = (DHData*)*storage;
    if (!data->AsyncCompletionRtn)
        return noErr;
    if (!PlaceToPutDataPtr || PlaceToPutDataPtr == data->AsyncPtr)
    {
        if (!Cancel)
            InvokeDataHCompletionUPP(data->AsyncPtr, data->AsyncRefCon, noErr, data->AsyncCompletionRtn);
        data->AsyncPtr = NULL;
        data->AsyncRefCon = 0;
        data->AsyncCompletionRtn = NULL;
    }
    return noErr;
}

static pascal ComponentResult myDataHGetData ( DataHandler dh,
                                        Handle h,
                                        long hOffset,
                                        long offset,
                                        long size)
{
    Handle storage = GetComponentInstanceStorage(dh);
    DHData *data = (DHData*)*storage;
    BYTE *target = (BYTE*)*h;
    LONGLONG off = offset;
    HRESULT hr;

    TRACE("%p %p %li %li %li\n",dh, h, hOffset, offset, size);
    hr = IAsyncReader_SyncRead(data->dataRef.pReader, off, size, target+hOffset);
    TRACE("result %x\n",hr);

    return noErr;
}

static pascal ComponentResult myDataHCompareDataRef ( DataHandler dh,
                                               Handle dataRef, Boolean *equal)
{
    WineDataRefRecord *ptr1;
    Handle storage = GetComponentInstanceStorage(dh);
    DHData *data = (DHData*)*storage;
    TRACE("\n");

    ptr1 = (WineDataRefRecord*)dataRef;

    *equal = (ptr1->pReader == data->dataRef.pReader);
    return noErr;
}

static pascal ComponentResult myDataHGetDataRef ( DataHandler dh, Handle *dataRef)
{
    Handle storage = GetComponentInstanceStorage(dh);
    TRACE("\n");
    *dataRef = storage;
    HandToHand(dataRef);
    return noErr;
}

static pascal ComponentResult myDataHGetScheduleAheadTime ( DataHandler dh,
                                                       long *millisecs)
{
    TRACE("\n");
    *millisecs = 1000;
    return noErr;
}

static pascal ComponentResult myDataHGetInfoFlags ( DataHandler dh, UInt32 *flags)
{
    TRACE("\n");
    *flags = 0;
    return noErr;
}

static pascal ComponentResult myDataHGetFileTypeOrdering ( DataHandler dh,
                           DataHFileTypeOrderingHandle *orderingListHandle)
{
    OSType orderlist[1] = {kDataHFileTypeExtension};
    TRACE("\n");
    PtrToHand( &orderlist, (Handle*)orderingListHandle, sizeof(OSType));
    return noErr;
}

typedef struct {
    const char *const fname;
    const int sig_length;
    const BYTE sig[10];
} signature;

static const signature stream_sigs[] = {
    {"video.asf",4,{0x30,0x26,0xb2,0x75}},
    {"video.mov",8,{0x00,0x00,0x00,0x14,0x66,0x74,0x79,0x70}},
    {"video.mp4",8,{0x00,0x00,0x00,0x18,0x66,0x74,0x79,0x70}},
    {"video.m4v",8,{0x00,0x00,0x00,0x1c,0x66,0x74,0x79,0x70}},
    {"video.flv",4,{0x46,0x4C,0x56,0x01}},
    {"video.mpg",3,{0x00,0x00,0x01}},
    {"avideo.rm",4,{0x2E,0x52,0x4D,0x46}}
};

static pascal ComponentResult myDataHGetFileName ( DataHandler dh, Str255 str)
{
    Handle storage = GetComponentInstanceStorage(dh);
    DHData *data = (DHData*)*storage;
    TRACE("%p %s\n",str,debugstr_guid(&data->dataRef.streamSubtype));

    /* Todo Expand this list */
    if (IsEqualIID(&data->dataRef.streamSubtype, &MEDIASUBTYPE_MPEG1Video) ||
        IsEqualIID(&data->dataRef.streamSubtype, &MEDIASUBTYPE_MPEG1System))
        CFStringGetPascalString(CFSTR("video.mpg"),str,256,kCFStringEncodingMacRoman);
    else if(IsEqualIID(&data->dataRef.streamSubtype, &MEDIASUBTYPE_Asf))
        CFStringGetPascalString(CFSTR("video.asf"),str,256,kCFStringEncodingMacRoman);
    else if(IsEqualIID(&data->dataRef.streamSubtype, &MEDIASUBTYPE_Avi))
        CFStringGetPascalString(CFSTR("video.avi"),str,256,kCFStringEncodingMacRoman);
    else if(IsEqualIID(&data->dataRef.streamSubtype, &MEDIASUBTYPE_QTMovie))
        CFStringGetPascalString(CFSTR("video.mov"),str,256,kCFStringEncodingMacRoman);
    else
    {
        BYTE header[10] = {0,0,0,0,0,0,0,0,0,0};
        int i;
        IAsyncReader_SyncRead(data->dataRef.pReader, 0, 8, header);

        for (i = 0; i < ARRAY_SIZE(stream_sigs); i++)
            if (memcmp(header, stream_sigs[i].sig, stream_sigs[i].sig_length)==0)
            {
                str[0] = strlen(stream_sigs[i].fname);
                memcpy(str + 1, stream_sigs[i].fname, str[0]);
                return noErr;
            }

        return badComponentSelector;
    }

    return noErr;
}

static pascal ComponentResult myDataHOpenForRead(DataHandler dh)
{
    TRACE("\n");
    return noErr;
}

static pascal ComponentResult myDataHTask(DataHandler dh)
{
    Handle storage = GetComponentInstanceStorage(dh);
    DHData *data = (DHData*)*storage;

    if (data->AsyncCompletionRtn)
    {
        TRACE("Sending Completion\n");
        InvokeDataHCompletionUPP(data->AsyncPtr, data->AsyncRefCon, noErr, data->AsyncCompletionRtn);
        data->AsyncPtr = NULL;
        data->AsyncRefCon = 0;
        data->AsyncCompletionRtn = NULL;
    }
    return noErr;
}

static pascal ComponentResult myDataHPlaybackHints(DataHandler dh, long flags,
                unsigned long minFileOffset, unsigned long maxFileOffset,
                long bytesPerSecond)
{
    TRACE("%lu %lu %li\n",minFileOffset, maxFileOffset, bytesPerSecond);
    return noErr;
}

static pascal ComponentResult myDataHPlaybackHints64(DataHandler dh, long flags,
                wide *minFileOffset, wide *maxFileOffset,
                long bytesPerSecond)
{
    if (TRACE_ON(qtdatahandler))
    {
        SInt64 minFileOffset64 = WideToSInt64(*minFileOffset);
        LONGLONG minFileOffsetLL = minFileOffset64;
        SInt64 maxFileOffset64 = WideToSInt64(*maxFileOffset);
        LONGLONG maxFileOffsetLL = maxFileOffset64;

        TRACE("%s %s %li\n",wine_dbgstr_longlong(minFileOffsetLL), wine_dbgstr_longlong(maxFileOffsetLL), bytesPerSecond);
    }
    return noErr;
}

static pascal ComponentResult myDataHGetFileSize64(DataHandler dh, wide * fileSize)
{
    Handle storage = GetComponentInstanceStorage(dh);
    DHData *data = (DHData*)*storage;
    LONGLONG total;
    LONGLONG available;
    SInt64 total64;

    TRACE("%p\n",dh);

    IAsyncReader_Length(data->dataRef.pReader,&total,&available);
    total64 = total;
    *fileSize = SInt64ToWide(total64);
    return noErr;
}

static pascal ComponentResult myDataHGetFileSizeAsync ( DataHandler dh, wide *fileSize, DataHCompletionUPP CompletionRtn, long RefCon )
{
    Handle storage = GetComponentInstanceStorage(dh);
    DHData *data = (DHData*)*storage;
    LONGLONG total;
    LONGLONG available;
    SInt64 total64;

    TRACE("%p\n",dh);

    IAsyncReader_Length(data->dataRef.pReader,&total,&available);
    total64 = total;
    *fileSize = SInt64ToWide(total64);

    if (CompletionRtn)
    {
        if (data->AsyncCompletionRtn)
            InvokeDataHCompletionUPP(data->AsyncPtr, data->AsyncRefCon, noErr, data->AsyncCompletionRtn);

        data->AsyncPtr = (Ptr)fileSize;
        data->AsyncRefCon = RefCon;
        data->AsyncCompletionRtn = CompletionRtn;
    }

    return noErr;
}

static pascal ComponentResult myDataHGetAvailableFileSize64(DataHandler dh, wide * fileSize)
{
    Handle storage = GetComponentInstanceStorage(dh);
    DHData *data = (DHData*)*storage;
    LONGLONG total;
    LONGLONG available;
    SInt64 total64;

    TRACE("%p\n",dh);

    IAsyncReader_Length(data->dataRef.pReader,&total,&available);
    total64 = available;
    *fileSize = SInt64ToWide(total64);
    return noErr;
}

static pascal ComponentResult myDataHScheduleData64( DataHandler dh,
                                          Ptr PlaceToPutDataPtr,
                                          const wide * FileOffset,
                                          long DataSize,
                                          long RefCon,
                                          DataHSchedulePtr scheduleRec,
                                          DataHCompletionUPP CompletionRtn)
{
    Handle storage = GetComponentInstanceStorage(dh);
    DHData *data = (DHData*)*storage;
    HRESULT hr;
    SInt64 fileOffset64 = WideToSInt64(*FileOffset);
    LONGLONG offset = fileOffset64;
    BYTE* buffer = (BYTE*)PlaceToPutDataPtr;

    TRACE("%p %p %s %li %li %p %p\n",dh, PlaceToPutDataPtr, wine_dbgstr_longlong(offset), DataSize, RefCon, scheduleRec, CompletionRtn);

    hr = IAsyncReader_SyncRead(data->dataRef.pReader, offset, DataSize, buffer);
    TRACE("result %x\n",hr);
    if (CompletionRtn)
    {
        if (data->AsyncCompletionRtn)
            InvokeDataHCompletionUPP(data->AsyncPtr, data->AsyncRefCon, noErr, data->AsyncCompletionRtn);

        data->AsyncPtr = PlaceToPutDataPtr;
        data->AsyncRefCon = RefCon;
        data->AsyncCompletionRtn = CompletionRtn;
    }

    return noErr;
}

static const struct { LPVOID proc; ProcInfoType type;} componentFunctions[] =
{
    {NULL, 0}, /* 0 */
    {NULL, 0}, /* 1 */
    {myDataHGetData,  kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Handle)))
            | STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(long)))
            | STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(long)))
            | STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(long)))
}, /* kDataHGetDataSelect                        */
    {NULL, 0}, /* kDataHPutDataSelect                        */
    {NULL, 0}, /* kDataHFlushDataSelect                      */
    {NULL, 0}, /* kDataHOpenForWriteSelect                   */
    {NULL, 0}, /* kDataHCloseForWriteSelect                  */
    {NULL, 0}, /* 7 */
    {myDataHOpenForRead, kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
}, /* kDataHOpenForReadSelect
*/
    {NULL, 0}, /* kDataHCloseForReadSelect                   */
    {myDataHSetDataRef,  kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Handle)))
}, /* kDataHSetDataRefSelect                     */
    {myDataHGetDataRef,  kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Handle)))
}, /* kDataHGetDataRefSelect                     */
    {myDataHCompareDataRef,  kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Handle)))
            | STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(Boolean*)))
}, /* kDataHCompareDataRefSelect                 */
    {myDataHTask, kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
}, /* kDataHTaskSelect                           */
    {myDataHScheduleData,  kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Ptr)))
            | STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(long)))
            | STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(long)))
            | STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(long)))
            | STACK_ROUTINE_PARAMETER(6, SIZE_CODE(sizeof(DataHSchedulePtr)))
            | STACK_ROUTINE_PARAMETER(7, SIZE_CODE(sizeof(DataHCompletionUPP)))
}, /* kDataHScheduleDataSelect                   */
    {myDataHFinishData, kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Ptr)))
            | STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(Boolean)))
}, /* kDataHFinishDataSelect                     */
    {NULL, 0}, /* kDataHFlushCacheSelect       0x10              */
    {NULL, 0}, /* kDataHResolveDataRefSelect                 */
    {myDataHGetFileSize,  kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long*)))
}, /* kDataHGetFileSizeSelect                    */
    {myDataHCanUseDataRef,  kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Handle)))
            | STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(long*)))
}, /* kDataHCanUseDataRefSelect                  */
    {NULL, 0}, /* kDataHGetVolumeListSelect                  */
    {NULL, 0}, /* kDataHWriteSelect                          */
    {NULL, 0}, /* kDataHPreextendSelect                      */
    {NULL, 0}, /* kDataHSetFileSizeSelect                    */
    {NULL, 0}, /* kDataHGetFreeSpaceSelect                   */
    {NULL, 0}, /* kDataHCreateFileSelect                     */
    {NULL, 0}, /* kDataHGetPreferredBlockSizeSelect          */
    {NULL, 0}, /* kDataHGetDeviceIndexSelect                 */
    {NULL, 0}, /* kDataHIsStreamingDataHandlerSelect         */
    {NULL, 0}, /* kDataHGetDataInBufferSelect                */
    {myDataHGetScheduleAheadTime,  kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long*)))
}, /* kDataHGetScheduleAheadTimeSelect           */
/* End of Required List */
    {NULL, 0}, /* kDataHSetCacheSizeLimitSelect              */
    {NULL, 0}, /* kDataHGetCacheSizeLimitSelect       0x20   */
    {NULL, 0}, /* kDataHGetMovieSelect                       */
    {NULL, 0}, /* kDataHAddMovieSelect                       */
    {NULL, 0}, /* kDataHUpdateMovieSelect                    */
    {NULL, 0}, /* kDataHDoesBufferSelect                     */
    {myDataHGetFileName,  kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Str255*)))
}, /* kDataHGetFileNameSelect                    */
    {myDataHGetAvailableFileSize,   kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long*)))
}, /* kDataHGetAvailableFileSizeSelect           */
    {NULL, 0}, /* kDataHGetMacOSFileTypeSelect               */
    {NULL, 0}, /* kDataHGetMIMETypeSelect                    */
    {NULL, 0}, /* kDataHSetDataRefWithAnchorSelect           */
    {NULL, 0}, /* kDataHGetDataRefWithAnchorSelect           */
    {NULL, 0}, /* kDataHSetMacOSFileTypeSelect               */
    {NULL, 0}, /* kDataHSetTimeBaseSelect                    */
    {myDataHGetInfoFlags,  kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(UInt32*)))
}, /* kDataHGetInfoFlagsSelect                   */
    {myDataHScheduleData64, kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Ptr)))
            | STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(wide*)))
            | STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(long)))
            | STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(long)))
            | STACK_ROUTINE_PARAMETER(6, SIZE_CODE(sizeof(DataHSchedulePtr)))
            | STACK_ROUTINE_PARAMETER(7, SIZE_CODE(sizeof(DataHCompletionUPP)))
}, /* kDataHScheduleData64Select                 */
    {NULL, 0}, /* kDataHWrite64Select                        */
    {myDataHGetFileSize64, kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(wide*)))
}, /* kDataHGetFileSize64Select     0x30         */
    {NULL, 0}, /* kDataHPreextend64Select                    */
    {NULL, 0}, /* kDataHSetFileSize64Select                  */
    {NULL, 0}, /* kDataHGetFreeSpace64Select                 */
    {NULL, 0}, /* kDataHAppend64Select                       */
    {NULL, 0}, /* kDataHReadAsyncSelect                      */
    {NULL, 0}, /* kDataHPollReadSelect                       */
    {NULL, 0}, /* kDataHGetDataAvailabilitySelect            */
    {NULL, 0}, /* 0x0038 */
    {NULL, 0}, /* 0x0039 */
    {myDataHGetFileSizeAsync, kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(wide*)))
            | STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(DataHCompletionUPP)))
            | STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(long)))
}, /* kDataHGetFileSizeAsyncSelect               */
    {NULL, 0}, /* kDataHGetDataRefAsTypeSelect               */
    {NULL, 0}, /* kDataHSetDataRefExtensionSelect            */
    {NULL, 0}, /* kDataHGetDataRefExtensionSelect            */
    {NULL, 0}, /* kDataHGetMovieWithFlagsSelect              */
    {NULL, 0}, /* 0x3F */
    {myDataHGetFileTypeOrdering, kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(DataHFileTypeOrderingHandle*)))
}, /* kDataHGetFileTypeOrderingSelect  0x40      */
    {NULL, 0}, /* kDataHCreateFileWithFlagsSelect            */
    {NULL, 0}, /* kDataHGetMIMETypeAsyncSelect               */
    {NULL, 0}, /* kDataHGetInfoSelect                        */
    {NULL, 0}, /* kDataHSetIdleManagerSelect                 */
    {NULL, 0}, /* kDataHDeleteFileSelect                     */
    {NULL, 0}, /* kDataHSetMovieUsageFlagsSelect             */
    {NULL, 0}, /* kDataHUseTemporaryDataRefSelect            */
    {NULL, 0}, /* kDataHGetTemporaryDataRefCapabilitiesSelect */
    {NULL, 0}, /* kDataHRenameFileSelect                     */
    {NULL, 0}, /* 0x4A */
    {NULL, 0}, /* 0x4B */
    {NULL, 0}, /* 0x4C */
    {NULL, 0}, /* 0x4D */
    {myDataHGetAvailableFileSize64, kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(wide*)))
}, /* kDataHGetAvailableFileSize64Select         */
    {NULL, 0}, /* kDataHGetDataAvailability64Select          */
};

static const struct { LPVOID proc; ProcInfoType type;} componentFunctions_2[] =
{
    {myDataHPlaybackHints, kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long)))
            | STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(unsigned long)))
            | STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(unsigned long)))
            | STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(long)))
}, /* kDataHPlaybackHintsSelect     0x103 */
    {myDataHPlaybackHints64, kPascalStackBased
            | RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
            | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DataHandler)))
            | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long)))
            | STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(wide*)))
            | STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(wide*)))
            | STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(long)))
}, /* kDataHPlaybackHints64Select   0x10E */
    {NULL, 0}, /* kDataHGetDataRateSelect       0x110 */
    {NULL, 0}, /* kDataHSetTimeHintsSelect      0x111 */
};

/* Component Functions */

static pascal ComponentResult myComponentOpen(ComponentInstance ci, ComponentInstance self)
{
    DHData myData;
    Handle storage;

    ZeroMemory(&myData,sizeof(DHData));
    PtrToHand( &myData, &storage, sizeof(DHData));
    SetComponentInstanceStorage(self,storage);

    return noErr;
}

static pascal ComponentResult myComponentClose(ComponentInstance ci, ComponentInstance self)
{
    Handle storage = GetComponentInstanceStorage(self);
    DHData *data;
    if (storage)
    {
        data = (DHData*)*storage;
        if (data && data->dataRef.pReader != NULL)
            IAsyncReader_Release(data->dataRef.pReader);
        DisposeHandle(storage);
        SetComponentInstanceStorage(self,NULL);
    }
    return noErr;
}

static pascal ComponentResult myComponentCanDo(ComponentInstance ci, SInt16 ftnNumber)
{
    TRACE("test 0x%x\n",ftnNumber);
    if (ftnNumber <= kComponentOpenSelect && ftnNumber >= kComponentCanDoSelect)
        return TRUE;
    if (ftnNumber == kDataHPlaybackHintsSelect)
        return TRUE;
    if (ftnNumber == kDataHPlaybackHints64Select)
        return TRUE;
    if (ftnNumber > kDataHGetDataAvailability64Select)
        return FALSE;
    TRACE("impl? %i\n",(componentFunctions[ftnNumber].proc != NULL));
    return (componentFunctions[ftnNumber].proc != NULL);
}

/* Main Proc */

static ComponentResult callOurFunction(LPVOID proc, ProcInfoType type, ComponentParameters * cp)
{
    ComponentRoutineUPP myUUP;
    ComponentResult result;

    myUUP = NewComponentFunctionUPP(proc, type);
    result = CallComponentFunction(cp, myUUP);
    DisposeComponentFunctionUPP(myUUP);
    return result;
}

static pascal ComponentResult myComponentRoutineProc ( ComponentParameters * cp,
                                         Handle componentStorage)
{
    switch (cp->what)
    {
        case kComponentOpenSelect:
            return callOurFunction(myComponentOpen, uppCallComponentOpenProcInfo,  cp);
        case kComponentCloseSelect:
            return callOurFunction(myComponentClose, uppCallComponentOpenProcInfo,  cp);
        case kComponentCanDoSelect:
            return callOurFunction(myComponentCanDo, uppCallComponentCanDoProcInfo,  cp);
        case kDataHPlaybackHintsSelect:
            return callOurFunction(componentFunctions_2[0].proc, componentFunctions_2[0].type, cp);
        case kDataHPlaybackHints64Select:
            return callOurFunction(componentFunctions_2[1].proc, componentFunctions_2[1].type, cp);
    }

    if (cp->what > 0 && cp->what <=kDataHGetDataAvailability64Select && componentFunctions[cp->what].proc)
        return callOurFunction(componentFunctions[cp->what].proc, componentFunctions[cp->what].type, cp);

    FIXME("unhandled select 0x%x\n",cp->what);
    return badComponentSelector;
}
