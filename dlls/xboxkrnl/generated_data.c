/*
 * xboxkrnl.exe - placeholder storage for xboxkrnl's non-function (data)
 * ordinal exports.
 *
 * THIS FILE IS GENERATED, same basis as generated_stubs.c: ordinal numbers,
 * export names and (where practical - see the comments below) real sizes are
 * taken from Cxbx-Reloaded's KernelThunk.cpp and src/core/kernel/common/*.h.
 * A handful of these are simple scalars given a sensible startup value
 * (KdDebuggerEnabled = FALSE, KdDebuggerNotPresent = TRUE, etc); most are
 * zero-initialised storage of the right size, or - for a few genuinely
 * Xbox-internal structures whose exact layout wasn't looked up (object
 * manager OBJECT_TYPE descriptors, MMGLOBALDATA, IDE_CHANNEL_OBJECT) -
 * zero-initialised storage of a generously-sized placeholder, since nothing
 * populates or reads these correctly until the corresponding kernel
 * subsystem exists. A real memory location backs every export; the *values*
 * at most of them are not meaningful yet.
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xboxkrnl);

BYTE XBOXKRNL_ExEventObjectType[64]; /* OBJECT_TYPE - object manager not implemented */
BYTE XBOXKRNL_ExMutantObjectType[64]; /* OBJECT_TYPE - object manager not implemented */
BYTE XBOXKRNL_ExSemaphoreObjectType[64]; /* OBJECT_TYPE - object manager not implemented */
BYTE XBOXKRNL_ExTimerObjectType[64]; /* OBJECT_TYPE - object manager not implemented */
ULONG XBOXKRNL_HalDiskCachePartitionCount;
void * XBOXKRNL_HalDiskModelNumber; /* PANSI_STRING, unpopulated */
void * XBOXKRNL_HalDiskSerialNumber; /* PANSI_STRING, unpopulated */
BYTE XBOXKRNL_IoCompletionObjectType[64]; /* OBJECT_TYPE - object manager not implemented */
BYTE XBOXKRNL_IoDeviceObjectType[64]; /* OBJECT_TYPE - object manager not implemented */
BYTE XBOXKRNL_IoFileObjectType[64]; /* OBJECT_TYPE - object manager not implemented */
BYTE XBOXKRNL_MmGlobalData[128]; /* MMGLOBALDATA - memory manager not implemented */
BYTE XBOXKRNL_KeInterruptTime[12]; /* KSYSTEM_TIME (LowPart/High1Time/High2Time) */
BYTE XBOXKRNL_KeSystemTime[12]; /* KSYSTEM_TIME (LowPart/High1Time/High2Time) */
ULONG XBOXKRNL_KeTickCount;
ULONG XBOXKRNL_KeTimeIncrement;
ULONG_PTR XBOXKRNL_KiBugCheckData[5];
void * XBOXKRNL_LaunchDataPage; /* PLAUNCH_DATA_PAGE, unpopulated */
BYTE XBOXKRNL_ObDirectoryObjectType[64]; /* OBJECT_TYPE - object manager not implemented */
BYTE XBOXKRNL_ObpObjectHandleTable[64]; /* OBJECT_HANDLE_TABLE - object manager not implemented */
BYTE XBOXKRNL_ObSymbolicLinkObjectType[64]; /* OBJECT_TYPE - object manager not implemented */
BYTE XBOXKRNL_PsThreadObjectType[64]; /* OBJECT_TYPE - object manager not implemented */
BYTE XBOXKRNL_XboxEEPROMKey[16]; /* XBOX_KEY_DATA */
BYTE XBOXKRNL_XboxHardwareInfo[8]; /* XBOX_HARDWARE_INFO {ULONG Flags; UCHAR GpuRevision,McpRevision,Unk3,Unk4;} */
BYTE XBOXKRNL_XboxHDKey[16]; /* XBOX_KEY_DATA */
USHORT XBOXKRNL_XboxKrnlVersion[4]; /* XBOX_KRNL_VERSION {Major,Minor,Build,Qfe} */
BYTE XBOXKRNL_XboxSignatureKey[16]; /* XBOX_KEY_DATA */
STRING XBOXKRNL_XeImageFileName; /* OBJECT_STRING == STRING, unpopulated */
BYTE XBOXKRNL_XboxLANKey[16]; /* XBOX_KEY_DATA */
BYTE XBOXKRNL_XboxAlternateSignatureKeys[256]; /* XBOX_KEY_DATA[16] */
BYTE XBOXKRNL_XePublicKeyData[284];
ULONG XBOXKRNL_HalBootSMCVideoMode;
BYTE XBOXKRNL_IdexChannelObject[32]; /* IDE_CHANNEL_OBJECT - not implemented */

BOOLEAN XBOXKRNL_KdDebuggerEnabled = FALSE;
BOOLEAN XBOXKRNL_KdDebuggerNotPresent = TRUE;
