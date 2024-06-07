/*
 *	MiniDump dumping utility
 *
 * 	Copyright 2005 Eric Pouech
 * 	          2024 Eric Pouech for CodeWeavers
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
#include <stdarg.h>

#include "winedump.h"
#include "winver.h"
#include "dbghelp.h"

static void dump_mdmp_data(const MINIDUMP_LOCATION_DESCRIPTOR* md, const char* pfx)
{
    if (md->DataSize)
        dump_data(PRD(md->Rva, md->DataSize), md->DataSize, pfx);
}

static const char *get_mdmp_str(DWORD rva)
{
    const MINIDUMP_STRING*      ms;
    if (!rva)
        return "<<rva=0>>";
    if ((ms = PRD(rva, sizeof(MINIDUMP_STRING))))
        return get_unicode_str( ms->Buffer, ms->Length / sizeof(WCHAR) );
    return "<<?>>";
}

enum FileSig get_kind_mdmp(void)
{
    const DWORD*        pdw;

    pdw = PRD(0, sizeof(DWORD));
    if (!pdw) {printf("Can't get main signature, aborting\n"); return SIG_UNKNOWN;}

    if (*pdw == 0x504D444D /* "MDMP" */) return SIG_MDMP;
    return SIG_UNKNOWN;
}

static void dump_system_time(const SYSTEMTIME *t, const char *pfx)
{
    printf("%swYear: %u\n", pfx, t->wYear);
    printf("%swMonth: %u\n", pfx, t->wYear);
    printf("%swMonth: %u\n", pfx, t->wMonth);
    printf("%swDayOfWeek: %u\n", pfx, t->wDayOfWeek);
    printf("%swDay: %u\n", pfx, t->wDay);
    printf("%swHour: %u\n", pfx, t->wHour);
    printf("%swMinute: %u\n", pfx, t->wMinute);
    printf("%swSecond: %u\n", pfx, t->wSecond);
    printf("%swMilliseconds: %u\n", pfx, t->wMilliseconds);
}

void mdmp_dump(void)
{
    const MINIDUMP_HEADER*      hdr = PRD(0, sizeof(MINIDUMP_HEADER));
    const MINIDUMP_DIRECTORY*   dir;
    const void*                 stream;
    unsigned int i, idx;
    const BYTE *ptr;

    if (!hdr)
    {
        printf("Corrupt file, cannot get Minidump header\n");
        return;
    }

    printf("Header\n");
    printf("  Signature: %#x (%.4s)\n", hdr->Signature, (const char*)&hdr->Signature);
    printf("  Version: %#x\n", hdr->Version);
    printf("  NumberOfStreams: %u\n", hdr->NumberOfStreams);
    printf("  StreamDirectoryRva: %#x\n", (UINT)hdr->StreamDirectoryRva);
    printf("  CheckSum: %#x (%u)\n", hdr->CheckSum, hdr->CheckSum);
    printf("  TimeDateStamp: %s\n", get_time_str(hdr->TimeDateStamp));
    printf("Flags: %s\n", get_hexint64_str(hdr->Flags));

    if (!PRD(hdr->StreamDirectoryRva, hdr->NumberOfStreams * sizeof(MINIDUMP_DIRECTORY)))
    {
        printf("Corrupt file, can't read all minidump directories\n");
        return;
    }
    for (idx = 0; idx < hdr->NumberOfStreams; ++idx)
    {
        dir = PRD(hdr->StreamDirectoryRva + idx * sizeof(MINIDUMP_DIRECTORY), sizeof(*dir));

        stream = PRD(dir->Location.Rva, dir->Location.DataSize);
        if (!stream)
        {
            printf("Stream [%u]: corrupt file, stream is larger that file\n", idx);
            continue;
        }
        switch (dir->StreamType)
        {
        case UnusedStream:
            printf("Stream [%u]: Unused:\n", idx);
            break;
        case ThreadListStream:
            if (globals_dump_sect("thread"))
            {
                const MINIDUMP_THREAD_LIST *mtl = stream;
                const MINIDUMP_THREAD *mt = mtl->Threads;

                printf("Stream [%u]: Threads:\n", idx);
                printf("  NumberOfThreads: %u\n", mtl->NumberOfThreads);
                for (i = 0; i < mtl->NumberOfThreads; i++, mt++)
                {
                    printf("  Thread: #%d\n", i);
                    printf("    ThreadId: %#x\n", mt->ThreadId);
                    printf("    SuspendCount: %u\n", mt->SuspendCount);
                    printf("    PriorityClass: %u\n", mt->PriorityClass);
                    printf("    Priority: %u\n", mt->Priority);
                    printf("  Teb: %s\n", get_hexint64_str(mt->Teb));
                    printf("  Stack: %s +%#x\n", get_hexint64_str(mt->Stack.StartOfMemoryRange), mt->Stack.Memory.DataSize);
                    if (globals_dump_sect("content"))
                        dump_mdmp_data(&mt->Stack.Memory, "      ");
                    printf("    ThreadContext:\n");
                    dump_mdmp_data(&mt->ThreadContext, "      ");
                }
            }
            break;
        case ModuleListStream:
        case 0xFFF0:
            if (globals_dump_sect("module"))
            {
                const MINIDUMP_MODULE_LIST *mml = stream;
                const MINIDUMP_MODULE*      mm = mml->Modules;
                const char*                 p1;
                const char*                 p2;

                printf("Stream [%u]: Modules (%s):\n", idx,
                       dir->StreamType == ModuleListStream ? "PE" : "ELF");
                printf("  NumberOfModules: %u\n", mml->NumberOfModules);
                for (i = 0; i < mml->NumberOfModules; i++, mm++)
                {
                    printf("  Module #%d:\n", i);
                    printf("    BaseOfImage: %s\n", get_hexint64_str(mm->BaseOfImage));
                    printf("    SizeOfImage: %#x (%u)\n", mm->SizeOfImage, mm->SizeOfImage);
                    printf("    CheckSum: %#x (%u)\n", mm->CheckSum, mm->CheckSum);
                    printf("    TimeDateStamp: %s\n", get_time_str(mm->TimeDateStamp));
                    printf("    ModuleName: %s\n", get_mdmp_str(mm->ModuleNameRva));
                    printf("    VersionInfo:\n");
                    printf("      dwSignature: %x\n", (UINT)mm->VersionInfo.dwSignature);
                    printf("      dwStrucVersion: %x\n", (UINT)mm->VersionInfo.dwStrucVersion);
                    printf("      dwFileVersion: %d,%d,%d,%d\n",
                           HIWORD(mm->VersionInfo.dwFileVersionMS),
                           LOWORD(mm->VersionInfo.dwFileVersionMS),
                           HIWORD(mm->VersionInfo.dwFileVersionLS),
                           LOWORD(mm->VersionInfo.dwFileVersionLS));
                    printf("      dwProductVersion %d,%d,%d,%d\n",
                           HIWORD(mm->VersionInfo.dwProductVersionMS),
                           LOWORD(mm->VersionInfo.dwProductVersionMS),
                           HIWORD(mm->VersionInfo.dwProductVersionLS),
                           LOWORD(mm->VersionInfo.dwProductVersionLS));
                    printf("      dwFileFlagsMask: %x\n", (UINT)mm->VersionInfo.dwFileFlagsMask);
                    printf("      dwFileFlags: %s%s%s%s%s%s\n",
                           mm->VersionInfo.dwFileFlags & VS_FF_DEBUG ? "Debug " : "",
                           mm->VersionInfo.dwFileFlags & VS_FF_INFOINFERRED ? "Inferred " : "",
                           mm->VersionInfo.dwFileFlags & VS_FF_PATCHED ? "Patched " : "",
                           mm->VersionInfo.dwFileFlags & VS_FF_PRERELEASE ? "PreRelease " : "",
                           mm->VersionInfo.dwFileFlags & VS_FF_PRIVATEBUILD ? "PrivateBuild " : "",
                           mm->VersionInfo.dwFileFlags & VS_FF_SPECIALBUILD ? "SpecialBuild " : "");
                    if (mm->VersionInfo.dwFileOS)
                    {
                        switch (mm->VersionInfo.dwFileOS & 0x000F)
                        {
                        case VOS__BASE:     p1 = "_base"; break;
                        case VOS__WINDOWS16:p1 = "16 bit Windows"; break;
                        case VOS__PM16:     p1 = "16 bit Presentation Manager"; break;
                        case VOS__PM32:     p1 = "32 bit Presentation Manager"; break;
                        case VOS__WINDOWS32:p1 = "32 bit Windows"; break;
                        default:            p1 = "---"; break;
                        }
                        switch (mm->VersionInfo.dwFileOS & 0xF0000)
                        {
                        case VOS_UNKNOWN:   p2 = "unknown"; break;
                        case VOS_DOS:       p2 = "DOS"; break;
                        case VOS_OS216:     p2 = "16 bit OS/2"; break;
                        case VOS_OS232:     p2 = "32 bit OS/2"; break;
                        case VOS_NT:        p2 = "Windows NT"; break;
                        default:            p2 = "---"; break;
                        }
                        printf("      dwFileOS: %s running on %s\n", p1, p2);
                    }
                    else printf("      dwFileOS: 0\n");
                    switch (mm->VersionInfo.dwFileType)
                    {
                    case VFT_UNKNOWN:       p1 = "Unknown"; break;
                    case VFT_APP:           p1 = "Application"; break;
                    case VFT_DLL:           p1 = "DLL"; break;
                    case VFT_DRV:           p1 = "Driver"; break;
                    case VFT_FONT:          p1 = "Font"; break;
                    case VFT_VXD:           p1 = "VxD"; break;
                    case VFT_STATIC_LIB:    p1 = "Static Library"; break;
                    default:                p1 = "---"; break;
                    }
                    printf("      dwFileType: %s\n", p1);
                    printf("      dwFileSubtype: %u\n", (UINT)mm->VersionInfo.dwFileSubtype);
                    printf("      dwFileDate: %x%08x\n",
                           (UINT)mm->VersionInfo.dwFileDateMS, (UINT)mm->VersionInfo.dwFileDateLS);
                    printf("    CvRecord: <%u>\n", (UINT)mm->CvRecord.DataSize);
                    if (globals_dump_sect("content"))
                        dump_mdmp_data(&mm->CvRecord, "      ");
                    printf("    MiscRecord: <%u>\n", (UINT)mm->MiscRecord.DataSize);
                    if (globals_dump_sect("content"))
                        dump_mdmp_data(&mm->MiscRecord, "      ");
                    printf("    Reserved0: %s\n", get_hexint64_str(mm->Reserved0));
                    printf("    Reserved1: %s\n", get_hexint64_str(mm->Reserved1));
                }
            }
            break;
        case MemoryListStream:
            if (globals_dump_sect("memory"))
            {
                const MINIDUMP_MEMORY_LIST *mml = stream;
                const MINIDUMP_MEMORY_DESCRIPTOR*   mmd = mml->MemoryRanges;

                printf("Stream [%u]: Memory Ranges:\n", idx);
                printf("  NumberOfMemoryRanges: %u\n", mml->NumberOfMemoryRanges);
                for (i = 0; i < mml->NumberOfMemoryRanges; i++, mmd++)
                {
                    printf("  Memory Range #%d:\n", i);
                    printf("    Range: %s +%#x\n", get_hexint64_str(mmd->StartOfMemoryRange), mmd->Memory.DataSize);
                    if (globals_dump_sect("content"))
                        dump_mdmp_data(&mmd->Memory, "      ");
                }
            }
            break;
        case Memory64ListStream:
            if (globals_dump_sect("memory"))
            {
                const MINIDUMP_MEMORY64_LIST *mml = stream;
                const MINIDUMP_MEMORY_DESCRIPTOR64 *mmd = mml->MemoryRanges;
                ULONG64 i64, base_rva;
                printf("Stream [%u]: Memory64 Ranges:\n", idx);
                printf("  NumberOfMemoryRanges: %s\n", get_uint64_str(mml->NumberOfMemoryRanges));
                base_rva = mml->BaseRva;
                for (i64 = 0; i64 < mml->NumberOfMemoryRanges; i64++, mmd++)
                {
                    printf("  Memory Range #%s:\n", get_uint64_str(i64));
                    printf("    Range: %s +%s\n", get_hexint64_str(mmd->StartOfMemoryRange), get_hexint64_str(mmd->DataSize));
                    if (globals_dump_sect("content"))
                        dump_data(PRD(base_rva, mmd->DataSize), mmd->DataSize, "      ");
                    base_rva += mmd->DataSize;
                }
            }
            break;
        case SystemInfoStream:
            if (globals_dump_sect("info"))
            {
                const MINIDUMP_SYSTEM_INFO *msi = stream;
                const char*                 str;
                char                        tmp[128];

                printf("Stream [%u]: System Information:\n", idx);
                switch (msi->ProcessorArchitecture)
                {
                case PROCESSOR_ARCHITECTURE_UNKNOWN:
                    str = "Unknown";
                    break;
                case PROCESSOR_ARCHITECTURE_INTEL:
                    strcpy(tmp, "Intel ");
                    switch (msi->ProcessorLevel)
                    {
                    case  3: str = "80386"; break;
                    case  4: str = "80486"; break;
                    case  5: str = "Pentium"; break;
                    case  6: str = "Pentium Pro/II or AMD Athlon"; break;
                    case 15: str = "Pentium 4 or AMD Athlon64"; break;
                    case 23: str = "AMD Zen 1 or 2"; break;
                    case 25: str = "AMD Zen 3 or 4"; break;
                    case 26: str = "AMD Zen 5"; break;
                    default: str = "???"; break;
                    }
                    strcat(tmp, str);
                    strcat(tmp, " (");
                    if (msi->ProcessorLevel == 3 || msi->ProcessorLevel == 4)
                    {
                        if (HIBYTE(msi->ProcessorRevision) == 0xFF)
                            sprintf(tmp + strlen(tmp), "%c%d", 'A' + ((msi->ProcessorRevision>>4)&0xf)-0x0a, msi->ProcessorRevision&0xf);
                        else
                            sprintf(tmp + strlen(tmp), "%c%d", 'A' + HIBYTE(msi->ProcessorRevision), LOBYTE(msi->ProcessorRevision));
                    }
                    else sprintf(tmp + strlen(tmp), "%d.%d", HIBYTE(msi->ProcessorRevision), LOBYTE(msi->ProcessorRevision));
                    str = tmp;
                    break;
                case PROCESSOR_ARCHITECTURE_MIPS:
                    str = "Mips";
                    break;
                case PROCESSOR_ARCHITECTURE_ALPHA:
                    str = "Alpha";
                    break;
                case PROCESSOR_ARCHITECTURE_PPC:
                    str = "PowerPC";
                    break;
                case PROCESSOR_ARCHITECTURE_ARM:
                    str = "ARM";
                    break;
                case PROCESSOR_ARCHITECTURE_ARM64:
                    str = "ARM64";
                    break;
                case PROCESSOR_ARCHITECTURE_AMD64:
                    str = "X86_64";
                    break;
                case PROCESSOR_ARCHITECTURE_MSIL:
                    str = "MSIL";
                    break;
                case PROCESSOR_ARCHITECTURE_NEUTRAL:
                    str = "Neutral";
                    break;
                default:
                    str = "???";
                    break;
                }
                printf("  Processor: %s (#%d CPUs)\n", str, msi->NumberOfProcessors);
                switch (msi->MajorVersion)
                {
                case 3:
                    switch (msi->MinorVersion)
                    {
                    case 51: str = "NT 3.51"; break;
                    default: str = "3-????"; break;
                    }
                    break;
                case 4:
                    switch (msi->MinorVersion)
                    {
                    case 0: str = (msi->PlatformId == VER_PLATFORM_WIN32_NT) ? "NT 4.0" : "95"; break;
                    case 10: str = "98"; break;
                    case 90: str = "ME"; break;
                    default: str = "4-????"; break;
                    }
                    break;
                case 5:
                    switch (msi->MinorVersion)
                    {
                    case 0: str = "2000"; break;
                    case 1: str = "XP"; break;
                    case 2:
                        if (msi->ProductType == 1) str = "XP";
                        else if (msi->ProductType == 3) str = "Server 2003";
                        else str = "5-????";
                        break;
                    default: str = "5-????"; break;
                    }
                    break;
                case 6:
                    switch (msi->MinorVersion)
                    {
                    case 0:
                        if (msi->ProductType == 1) str = "Vista";
                        else if (msi->ProductType == 3) str = "Server 2008";
                        else str = "6-????";
                        break;
                    case 1:
                        if (msi->ProductType == 1) str = "Win7";
                        else if (msi->ProductType == 3) str = "Server 2008 R2";
                        else str = "6-????";
                    break;
                case 2:
                    if (msi->ProductType == 1) str = "Win8";
                    else if (msi->ProductType == 3) str = "Server 2012";
                    else str = "6-????";
                    break;
                case 3:
                    if (msi->ProductType == 1) str = "Win8.1";
                    else if (msi->ProductType == 3) str = "Server 2012 R2";
                    else str = "6-????";
                    break;
                default: str = "6-????"; break;
                    }
                    break;
                case 10:
                    switch (msi->MinorVersion)
                    {
                    case 0:
                        if (msi->ProductType == 1) str = "Win10";
                        else str = "10-????";
                        break;
                    default: str = "10-????"; break;
                    }
                    break;
                default: str = "???"; break;
                }
                printf("  Version: Windows %s (%u)\n", str, msi->BuildNumber);
                printf("  PlatformId: %u\n", msi->PlatformId);
                printf("  CSD: %s\n", get_mdmp_str(msi->CSDVersionRva));
                printf("  Reserved1: %u\n", msi->Reserved1);
                if (msi->ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
                {
                    printf("  x86.VendorId: %.12s\n",
                           (const char*)msi->Cpu.X86CpuInfo.VendorId);
                    printf("  x86.VersionInformation: %x\n", msi->Cpu.X86CpuInfo.VersionInformation);
                    printf("  x86.FeatureInformation: %x\n", msi->Cpu.X86CpuInfo.FeatureInformation);
                    printf("  x86.AMDExtendedCpuFeatures: %x\n", msi->Cpu.X86CpuInfo.AMDExtendedCpuFeatures);
                }
                if (sizeof(MINIDUMP_SYSTEM_INFO) + 4 > dir->Location.DataSize &&
                    msi->CSDVersionRva >= dir->Location.Rva + 4)
                {
                    const char*  code = PRD(dir->Location.Rva + sizeof(MINIDUMP_SYSTEM_INFO), 4);
                    const DWORD* wes;
                    if (code && code[0] == 'W' && code[1] == 'I' && code[2] == 'N' && code[3] == 'E' &&
                        *(wes = (const DWORD*)(code += 4)) >= 3)
                    {
                        /* assume we have wine extensions */
                        printf("  Wine details:\n");
                        printf("    build-id: %s\n", code + wes[1]);
                        printf("    system: %s\n", code + wes[2]);
                        printf("    release: %s\n", code + wes[3]);
                    }
                }
            }
            break;
        case MiscInfoStream:
            if (globals_dump_sect("info"))
            {
                const MINIDUMP_MISC_INFO_5 *mmi = stream;

                printf("Stream [%u]: Misc Information:\n", idx);
                printf("  Size: %u\n", mmi->SizeOfInfo);
                printf("  Flags: %#x\n", mmi->Flags1);
                if (mmi->SizeOfInfo >= sizeof(MINIDUMP_MISC_INFO) && mmi->Flags1 & MINIDUMP_MISC1_PROCESS_ID)
                    printf("  ProcessId: %u\n", mmi->ProcessId);
                if (mmi->SizeOfInfo >= sizeof(MINIDUMP_MISC_INFO) && mmi->Flags1 & MINIDUMP_MISC1_PROCESS_TIMES)
                {
                    printf("  ProcessCreateTime: %s\n", get_time_str(mmi->ProcessCreateTime));
                    printf("  ProcessUserTime: %u\n", mmi->ProcessUserTime);
                    printf("  ProcessKernelTime: %u\n", mmi->ProcessKernelTime);
                }
                if (mmi->SizeOfInfo >= sizeof(MINIDUMP_MISC_INFO_2) && mmi->Flags1 & MINIDUMP_MISC1_PROCESSOR_POWER_INFO)
                {
                    printf("  ProcessorMaxMhz: %u\n", mmi->ProcessorMaxMhz);
                    printf("  ProcessorCurrentMhz: %u\n", mmi->ProcessorCurrentMhz);
                    printf("  ProcessorMhzLimit: %u\n", mmi->ProcessorMhzLimit);
                    printf("  ProcessorMaxIdleState: %u\n", mmi->ProcessorMaxIdleState);
                    printf("  ProcessorCurrentIdleState: %u\n", mmi->ProcessorCurrentIdleState);
                }
                if (mmi->SizeOfInfo >= sizeof(MINIDUMP_MISC_INFO_3) && mmi->Flags1 & MINIDUMP_MISC3_PROCESS_INTEGRITY)
                    printf("  ProcessIntegrityLevel: %u\n", mmi->ProcessIntegrityLevel);
                if (mmi->SizeOfInfo >= sizeof(MINIDUMP_MISC_INFO_3) && mmi->Flags1 & MINIDUMP_MISC3_PROCESS_EXECUTE_FLAGS)
                    printf("  ProcessExecuteFlags: %u\n", mmi->ProcessExecuteFlags);
                if (mmi->SizeOfInfo >= sizeof(MINIDUMP_MISC_INFO_3) && mmi->Flags1 & MINIDUMP_MISC3_PROTECTED_PROCESS)
                    printf("  ProtectedProcess: %u\n", mmi->ProtectedProcess);
                if (mmi->SizeOfInfo >= sizeof(MINIDUMP_MISC_INFO_3) && mmi->Flags1 & MINIDUMP_MISC3_TIMEZONE)
                {
                    printf("  TimeZoneId: %u\n", mmi->TimeZoneId);
                    printf("  TimeZone:\n");
                    printf("    Bias: %d\n", (INT)mmi->TimeZone.Bias);
                    printf("    StandardName: %s\n", get_unicode_str(mmi->TimeZone.StandardName, -1));
                    printf("    StandardDate:\n");
                    dump_system_time(&mmi->TimeZone.StandardDate, "      ");
                    printf("    StandardBias: %d\n", (INT)mmi->TimeZone.StandardBias);
                    printf("    DaylightName: %s\n", get_unicode_str(mmi->TimeZone.DaylightName, -1));
                    printf("    DaylightDate:\n");
                    dump_system_time(&mmi->TimeZone.DaylightDate, "      ");
                    printf("    DaylightBias: %d\n", (INT)mmi->TimeZone.DaylightBias);
                }
                if (mmi->SizeOfInfo >= sizeof(MINIDUMP_MISC_INFO_4) && mmi->Flags1 & MINIDUMP_MISC4_BUILDSTRING)
                {
                    printf("  BuildString: %s\n", get_unicode_str(mmi->BuildString, -1));
                    printf("  DbgBldStr: %s\n", get_unicode_str(mmi->DbgBldStr, -1));
                }
                if (mmi->SizeOfInfo >= sizeof(MINIDUMP_MISC_INFO_5) && (mmi->Flags1 & MINIDUMP_MISC5_PROCESS_COOKIE))
                    printf("  ProcessCookie: %#x\n", mmi->ProcessCookie);
            }
            break;
        case ExceptionStream:
            if (globals_dump_sect("exception"))
            {
                const MINIDUMP_EXCEPTION_STREAM *mes = stream;

                printf("Stream [%u]: Exception:\n", idx);
                printf("  ThreadId: %#x\n", mes->ThreadId);
                printf("  ExceptionRecord:\n");
                printf("  ExceptionCode: %#x\n", mes->ExceptionRecord.ExceptionCode);
                printf("  ExceptionFlags: %#x\n", mes->ExceptionRecord.ExceptionFlags);
                printf("  ExceptionRecord: %s\n", get_hexint64_str( mes->ExceptionRecord.ExceptionRecord));
                printf("  ExceptionAddress: %s\n", get_hexint64_str( mes->ExceptionRecord.ExceptionAddress));
                printf("  ExceptionNumberParameters: %u\n", mes->ExceptionRecord.NumberParameters);
                for (i = 0; i < mes->ExceptionRecord.NumberParameters; i++)
                {
                    printf("    [%d] %s\n", i, get_hexint64_str(mes->ExceptionRecord.ExceptionInformation[i]));
                }
                printf("  ThreadContext:\n");
                dump_mdmp_data(&mes->ThreadContext, "    ");
            }
            break;
        case HandleDataStream:
            if (globals_dump_sect("handle"))
            {
                const MINIDUMP_HANDLE_DATA_STREAM *mhd = stream;

                printf("Stream [%u]: Handle data:\n", idx);
                printf("  SizeOfHeader: %u\n", mhd->SizeOfHeader);
                printf("  SizeOfDescriptor: %u\n", mhd->SizeOfDescriptor);
                printf("  NumberOfDescriptors: %u\n", mhd->NumberOfDescriptors);

                ptr = (BYTE *)mhd + sizeof(*mhd);
                for (i = 0; i < mhd->NumberOfDescriptors; ++i)
                {
                    const MINIDUMP_HANDLE_DESCRIPTOR_2 *hd = (void *)ptr;

                    printf("  Handle [%u]:\n", i);
                    printf("    Handle: %s\n", get_hexint64_str(hd->Handle));
                    printf("    TypeName: %s\n", get_mdmp_str(hd->TypeNameRva));
                    printf("    ObjectName: %s\n", get_mdmp_str(hd->ObjectNameRva));
                    printf("    Attributes: %#x\n", hd->Attributes);
                    printf("    GrantedAccess: %#x\n", hd->GrantedAccess);
                    printf("    HandleCount: %u\n", hd->HandleCount);
                    printf("    PointerCount: %#x\n", hd->PointerCount);

                    if (mhd->SizeOfDescriptor >= sizeof(MINIDUMP_HANDLE_DESCRIPTOR_2))
                    {
                        MINIDUMP_HANDLE_OBJECT_INFORMATION *obj_info;
                        unsigned link_count = 0;

                        printf("    ObjectInfo: %#x\n", (UINT)hd->ObjectInfoRva);
                        printf("    Reserved0: %#x\n", hd->Reserved0);

                        if (hd->ObjectInfoRva)
                        {
                            for (obj_info = (void*)PRD(hd->ObjectInfoRva, sizeof(*obj_info));
                                 obj_info;
                                 obj_info = obj_info->NextInfoRva ? (void*)PRD(obj_info->NextInfoRva, sizeof(*obj_info)) : NULL)
                            {
                                printf("    Information[%u]\n", link_count++);
                                printf("      NextInfoRva: %#x\n", (UINT)obj_info->NextInfoRva);
                                printf("      InfoType: %u\n", obj_info->InfoType);
                                printf("      SizeOfInfo: %u\n", obj_info->SizeOfInfo);
                                if (globals_dump_sect("content"))
                                    dump_data((const BYTE*)(obj_info + 1), obj_info->SizeOfInfo, "        ");
                            }
                        }
                    }

                    ptr += mhd->SizeOfDescriptor;
                }
            }
            break;
        case ThreadInfoListStream:
            if (globals_dump_sect("thread"))
            {
                const MINIDUMP_THREAD_INFO_LIST *til = stream;

                printf("Stream [%u]: Thread Info List:\n", idx);
                printf("  SizeOfHeader: %u\n", (UINT)til->SizeOfHeader);
                printf("  SizeOfEntry: %u\n", (UINT)til->SizeOfEntry);
                printf("  NumberOfEntries: %u\n", (UINT)til->NumberOfEntries);

                ptr = (BYTE *)til + sizeof(*til);
                for (i = 0; i < til->NumberOfEntries; ++i)
                {
                    const MINIDUMP_THREAD_INFO *ti = (void *)ptr;

                    printf("  Thread [%u]:\n", i);
                    printf("    ThreadId: %#x\n", ti->ThreadId);
                    printf("    DumpFlags: %#x\n", ti->DumpFlags);
                    printf("    DumpError: %u\n", ti->DumpError);
                    printf("    ExitStatus: %u\n", ti->ExitStatus);
                    printf("    CreateTime: %s\n", get_uint64_str(ti->CreateTime));
                    printf("    ExitTime: %s\n", get_hexint64_str(ti->ExitTime));
                    printf("    KernelTime: %s\n", get_uint64_str(ti->KernelTime));
                    printf("    UserTime: %s\n", get_uint64_str(ti->UserTime));
                    printf("    StartAddress: %s\n", get_hexint64_str(ti->StartAddress));
                    printf("    Affinity: %s\n", get_uint64_str(ti->Affinity));

                    ptr += til->SizeOfEntry;
                }
            }
            break;
        case UnloadedModuleListStream:
            if (globals_dump_sect("module"))
            {
                const MINIDUMP_UNLOADED_MODULE_LIST *uml = stream;

                printf("Stream [%u]: Unloaded module list:\n", idx);
                printf("  SizeOfHeader: %u\n", uml->SizeOfHeader);
                printf("  SizeOfEntry: %u\n", uml->SizeOfEntry);
                printf("  NumberOfEntries: %u\n", uml->NumberOfEntries);

                ptr = (BYTE *)uml + sizeof(*uml);
                for (i = 0; i < uml->NumberOfEntries; ++i)
                {
                    const MINIDUMP_UNLOADED_MODULE *mod = (void *)ptr;

                    printf("  Module [%u]:\n", i);
                    printf("    BaseOfImage: %s\n", get_hexint64_str(mod->BaseOfImage));
                    printf("    SizeOfImage: %u\n", mod->SizeOfImage);
                    printf("    CheckSum: %#x\n", mod->CheckSum);
                    printf("    TimeDateStamp: %s\n", get_time_str(mod->TimeDateStamp));
                    printf("    ModuleName: %s\n", get_mdmp_str(mod->ModuleNameRva));

                    ptr += uml->SizeOfEntry;
                }
            }
            break;
        case MemoryInfoListStream:
            if (globals_dump_sect("memory"))
            {
                const MINIDUMP_MEMORY_INFO_LIST *mil = stream;
                const MINIDUMP_MEMORY_INFO *mi;

                printf("Memory info list:\n");
                printf("  SizeOfHeader: %u\n", (UINT)mil->SizeOfHeader);
                printf("  SizeOfEntry: %u\n", (UINT)mil->SizeOfEntry);
                printf("  NumberOfEntries: %s\n", get_uint64_str(mil->NumberOfEntries));
                mi = (const MINIDUMP_MEMORY_INFO *)((BYTE *)mil + mil->SizeOfHeader);
                dump_mdmp_data(&dir->Location, "    ");
                for (i = 0; i < mil->NumberOfEntries; ++i, ++mi)
                {
                    printf("  Memory info [%u]:\n", i);
                    printf("    BaseAddress: %s\n", get_hexint64_str(mi->BaseAddress));
                    printf("    AllocationBase: %s\n", get_hexint64_str(mi->AllocationBase));
                    printf("    AllocationProtect: %#x\n", mi->AllocationProtect);
                    /* __alignment1 */
                    printf("    RegionSize: %s\n", get_hexint64_str(mi->RegionSize));
                    printf("    State: %x\n", mi->State);
                    printf("    Protect: %x\n", mi->Protect);
                    printf("    Type: %x\n", mi->Type);
                    /* __alignment2 */
                }
            }
            break;
        case SystemMemoryInfoStream:
            if (globals_dump_sect("system"))
            {
                const MINIDUMP_SYSTEM_MEMORY_INFO_1 *smi = stream;

                printf("Stream [%u]: System memory info:\n", idx);
                printf("  Revision: %u\n", smi->Revision);
                printf("  Flags: %#x\n", smi->Flags);
                printf("  Basic info:\n");
                printf("    TimerResolution: %u\n", (UINT)smi->BasicInfo.TimerResolution);
                printf("    PageSize: %u\n", (UINT)smi->BasicInfo.PageSize);
                printf("    NumberOfPhysicalPages: %u\n", (UINT)smi->BasicInfo.NumberOfPhysicalPages);
                printf("    LowestPhysicalPageNumber: %u\n", (UINT)smi->BasicInfo.LowestPhysicalPageNumber);
                printf("    HighestPhysicalPageNumber: %u\n", (UINT)smi->BasicInfo.HighestPhysicalPageNumber);
                printf("    AllocationGranularity: %u\n", (UINT)smi->BasicInfo.AllocationGranularity);
                printf("    MinimumUserModeAddress: %s\n", get_hexint64_str(smi->BasicInfo.MinimumUserModeAddress));
                printf("    MaximumUserModeAddress: %s\n", get_hexint64_str(smi->BasicInfo.MaximumUserModeAddress));
                printf("    ActiveProcessorsAffinityMask: %s\n", get_hexint64_str(smi->BasicInfo.ActiveProcessorsAffinityMask));
                printf("    NumberOfProcessors: %u\n", (UINT)smi->BasicInfo.NumberOfProcessors);
                printf("  File cache info:\n");
                printf("    CurrentSize: %s\n", get_hexint64_str(smi->FileCacheInfo.CurrentSize));
                printf("    PeakSize: %s\n", get_hexint64_str(smi->FileCacheInfo.PeakSize));
                printf("    PageFaultCount: %u\n", (UINT)smi->FileCacheInfo.PageFaultCount);
                printf("    MinimumWorkingSet: %s\n", get_hexint64_str(smi->FileCacheInfo.MinimumWorkingSet));
                printf("    MaximumWorkingSet: %s\n", get_hexint64_str(smi->FileCacheInfo.MaximumWorkingSet));
                printf("    CurrentSizeIncludingTransitionInPages: %s\n", get_hexint64_str(smi->FileCacheInfo.CurrentSizeIncludingTransitionInPages));
                printf("    PeakSizeIncludingTransitionInPages: %s\n", get_hexint64_str(smi->FileCacheInfo.PeakSizeIncludingTransitionInPages));
                if (smi->Flags & MINIDUMP_SYSMEMINFO1_FILECACHE_TRANSITIONREPURPOSECOUNT_FLAGS)
                    printf("    TransitionRePurposeCount: %u\n", (UINT)smi->FileCacheInfo.TransitionRePurposeCount);
                printf("    Flags: %u\n", (UINT)smi->FileCacheInfo.Flags);
                if (smi->Flags & MINIDUMP_SYSMEMINFO1_BASICPERF)
                {
                    printf("  Basic perf:\n");
                    printf("    AvailablePages: %s\n", get_uint64_str(smi->BasicPerfInfo.AvailablePages));
                    printf("    CommittedPages: %s\n", get_uint64_str(smi->BasicPerfInfo.CommittedPages));
                    printf("    CommitLimit: %s\n", get_uint64_str(smi->BasicPerfInfo.CommitLimit));
                    printf("    PeakCommitment: %s\n", get_uint64_str(smi->BasicPerfInfo.PeakCommitment));
                }
                printf("  Perf:\n");
                printf("    IdleProcessTime: %s\n", get_uint64_str(smi->PerfInfo.IdleProcessTime));
                printf("    IoReadTransferCount: %s\n", get_uint64_str(smi->PerfInfo.IoReadTransferCount));
                printf("    IoWriteTransferCount: %s\n", get_uint64_str(smi->PerfInfo.IoWriteTransferCount));
                printf("    IoOtherTransferCount: %s\n", get_uint64_str(smi->PerfInfo.IoOtherTransferCount));
                printf("    IoReadOperationCount: %u\n", (UINT)smi->PerfInfo.IoReadOperationCount);
                printf("    IoWriteOperationCount: %u\n", (UINT)smi->PerfInfo.IoWriteOperationCount);
                printf("    IoOtherOperationCount: %u\n", (UINT)smi->PerfInfo.IoOtherOperationCount);
                printf("    AvailablePages: %u\n", (UINT)smi->PerfInfo.AvailablePages);
                printf("    CommittedPages: %u\n", (UINT)smi->PerfInfo.CommittedPages);
                printf("    CommitLimit: %u\n", (UINT)smi->PerfInfo.CommitLimit);
                printf("    PeakCommitment: %u\n", (UINT)smi->PerfInfo.PeakCommitment);
                printf("    PageFaultCount: %u\n", (UINT)smi->PerfInfo.PageFaultCount);
                printf("    CopyOnWriteCount: %u\n", (UINT)smi->PerfInfo.CopyOnWriteCount);
                printf("    TransitionCount: %u\n", (UINT)smi->PerfInfo.TransitionCount);
                printf("    CacheTransitionCount: %u\n", (UINT)smi->PerfInfo.CacheTransitionCount);
                printf("    DemandZeroCount: %u\n", (UINT)smi->PerfInfo.DemandZeroCount);
                printf("    PageReadCount: %u\n", (UINT)smi->PerfInfo.PageReadCount);
                printf("    PageReadIoCount: %u\n", (UINT)smi->PerfInfo.PageReadIoCount);
                printf("    CacheReadCount: %u\n", (UINT)smi->PerfInfo.CacheReadCount);
                printf("    CacheIoCount: %u\n", (UINT)smi->PerfInfo.CacheIoCount);
                printf("    DirtyPagesWriteCount: %u\n", (UINT)smi->PerfInfo.DirtyPagesWriteCount);
                printf("    DirtyWriteIoCount: %u\n", (UINT)smi->PerfInfo.DirtyWriteIoCount);
                printf("    MappedPagesWriteCount: %u\n", (UINT)smi->PerfInfo.MappedPagesWriteCount);
                printf("    MappedWriteIoCount: %u\n", (UINT)smi->PerfInfo.MappedWriteIoCount);
                printf("    PagedPoolPages: %u\n", (UINT)smi->PerfInfo.PagedPoolPages);
                printf("    NonPagedPoolPages: %u\n", (UINT)smi->PerfInfo.NonPagedPoolPages);
                printf("    PagedPoolAllocs: %u\n", (UINT)smi->PerfInfo.PagedPoolAllocs);
                printf("    PagedPoolFrees: %u\n", (UINT)smi->PerfInfo.PagedPoolFrees);
                printf("    NonPagedPoolAllocs: %u\n", (UINT)smi->PerfInfo.NonPagedPoolAllocs);
                printf("    NonPagedPoolFrees: %u\n", (UINT)smi->PerfInfo.NonPagedPoolFrees);
                printf("    FreeSystemPtes: %u\n", (UINT)smi->PerfInfo.FreeSystemPtes);
                printf("    ResidentSystemCodePage: %u\n", (UINT)smi->PerfInfo.ResidentSystemCodePage);
                printf("    TotalSystemDriverPages: %u\n", (UINT)smi->PerfInfo.TotalSystemDriverPages);
                printf("    TotalSystemCodePages: %u\n", (UINT)smi->PerfInfo.TotalSystemCodePages);
                printf("    NonPagedPoolLookasideHits: %u\n", (UINT)smi->PerfInfo.NonPagedPoolLookasideHits);
                printf("    PagedPoolLookasideHits: %u\n", (UINT)smi->PerfInfo.PagedPoolLookasideHits);
                printf("    AvailablePagedPoolPages: %u\n", (UINT)smi->PerfInfo.AvailablePagedPoolPages);
                printf("    ResidentSystemCachePage: %u\n", (UINT)smi->PerfInfo.ResidentSystemCachePage);
                printf("    ResidentPagedPoolPage: %u\n", (UINT)smi->PerfInfo.ResidentPagedPoolPage);
                printf("    ResidentSystemDriverPage: %u\n", (UINT)smi->PerfInfo.ResidentSystemDriverPage);
                printf("    CcFastReadNoWait: %u\n", (UINT)smi->PerfInfo.CcFastReadNoWait);
                printf("    CcFastReadWait: %u\n", (UINT)smi->PerfInfo.CcFastReadWait);
                printf("    CcFastReadResourceMiss: %u\n", (UINT)smi->PerfInfo.CcFastReadResourceMiss);
                printf("    CcFastReadNotPossible: %u\n", (UINT)smi->PerfInfo.CcFastReadNotPossible);
                printf("    CcFastMdlReadNoWait: %u\n", (UINT)smi->PerfInfo.CcFastMdlReadNoWait);
                printf("    CcFastMdlReadWait: %u\n", (UINT)smi->PerfInfo.CcFastMdlReadWait);
                printf("    CcFastMdlReadResourceMiss: %u\n", (UINT)smi->PerfInfo.CcFastMdlReadResourceMiss);
                printf("    CcFastMdlReadNotPossible: %u\n", (UINT)smi->PerfInfo.CcFastMdlReadNotPossible);
                printf("    CcMapDataNoWait: %u\n", (UINT)smi->PerfInfo.CcMapDataNoWait);
                printf("    CcMapDataWait: %u\n", (UINT)smi->PerfInfo.CcMapDataWait);
                printf("    CcMapDataNoWaitMiss: %u\n", (UINT)smi->PerfInfo.CcMapDataNoWaitMiss);
                printf("    CcMapDataWaitMiss: %u\n", (UINT)smi->PerfInfo.CcMapDataWaitMiss);
                printf("    CcPinMappedDataCount: %u\n", (UINT)smi->PerfInfo.CcPinMappedDataCount);
                printf("    CcPinReadNoWait: %u\n", (UINT)smi->PerfInfo.CcPinReadNoWait);
                printf("    CcPinReadWait: %u\n", (UINT)smi->PerfInfo.CcPinReadWait);
                printf("    CcPinReadNoWaitMiss: %u\n", (UINT)smi->PerfInfo.CcPinReadNoWaitMiss);
                printf("    CcPinReadWaitMiss: %u\n", (UINT)smi->PerfInfo.CcPinReadWaitMiss);
                printf("    CcCopyReadNoWait: %u\n", (UINT)smi->PerfInfo.CcCopyReadNoWait);
                printf("    CcCopyReadWait: %u\n", (UINT)smi->PerfInfo.CcCopyReadWait);
                printf("    CcCopyReadNoWaitMiss: %u\n", (UINT)smi->PerfInfo.CcCopyReadNoWaitMiss);
                printf("    CcCopyReadWaitMiss: %u\n", (UINT)smi->PerfInfo.CcCopyReadWaitMiss);
                printf("    CcMdlReadNoWait: %u\n", (UINT)smi->PerfInfo.CcMdlReadNoWait);
                printf("    CcMdlReadWait: %u\n", (UINT)smi->PerfInfo.CcMdlReadWait);
                printf("    CcMdlReadNoWaitMiss: %u\n", (UINT)smi->PerfInfo.CcMdlReadNoWaitMiss);
                printf("    CcMdlReadWaitMiss: %u\n", (UINT)smi->PerfInfo.CcMdlReadWaitMiss);
                printf("    CcReadAheadIos: %u\n", (UINT)smi->PerfInfo.CcReadAheadIos);
                printf("    CcLazyWriteIos: %u\n", (UINT)smi->PerfInfo.CcLazyWriteIos);
                printf("    CcLazyWritePages: %u\n", (UINT)smi->PerfInfo.CcLazyWritePages);
                printf("    CcDataFlushes: %u\n", (UINT)smi->PerfInfo.CcDataFlushes);
                printf("    CcDataPages: %u\n", (UINT)smi->PerfInfo.CcDataPages);
                printf("    ContextSwitches: %u\n", (UINT)smi->PerfInfo.ContextSwitches);
                printf("    FirstLevelTbFills: %u\n", (UINT)smi->PerfInfo.FirstLevelTbFills);
                printf("    SecondLevelTbFills: %u\n", (UINT)smi->PerfInfo.SecondLevelTbFills);
                printf("    SystemCalls: %u\n", (UINT)smi->PerfInfo.SystemCalls);

                if (smi->Flags & MINIDUMP_SYSMEMINFO1_PERF_CCTOTALDIRTYPAGES_CCDIRTYPAGETHRESHOLD)
                {
                    printf("    CcTotalDirtyPages: %s\n", get_uint64_str(smi->PerfInfo.CcTotalDirtyPages));
                    printf("    CcDirtyPageThreshold: %s\n", get_uint64_str(smi->PerfInfo.CcDirtyPageThreshold));
                }
                if (smi->Flags & MINIDUMP_SYSMEMINFO1_PERF_RESIDENTAVAILABLEPAGES_SHAREDCOMMITPAGES)
                {
                    printf("    ResidentAvailablePages: %s\n", get_uint64_str(smi->PerfInfo.ResidentAvailablePages));
                    printf("    SharedCommittedPages: %s\n", get_uint64_str(smi->PerfInfo.SharedCommittedPages));
                }
            }
            break;
        case ProcessVmCountersStream:
            if (globals_dump_sect("system"))
            {
                const MINIDUMP_PROCESS_VM_COUNTERS_2 *pvm = stream;

                /* usage MINIDUMP_PROCESS_VM_COUNTERS to be asserted */
                /* Note: contrary to other structures, _1 isn't a subset of _2... */
                printf("Stream [%u]: Process VM counters:\n", idx);
                printf("  Revision: %u\n", pvm->Revision);
                if (pvm->Revision >= 2)
                    printf("  Flags: %#x\n", pvm->Flags);
                printf("  PageFaultCount: %u\n", (UINT)pvm->PageFaultCount);
                printf("  PeakWorkingSetSize: %s\n", get_hexint64_str(pvm->PeakWorkingSetSize));
                printf("  WorkingSetSize: %s\n", get_hexint64_str(pvm->WorkingSetSize));
                printf("  QuotaPeakPagedPoolUsage: %s\n", get_hexint64_str(pvm->QuotaPeakPagedPoolUsage));
                printf("  QuotaPagedPoolUsage: %s\n", get_hexint64_str(pvm->QuotaPagedPoolUsage));
                printf("  QuotaPeakNonPagedPoolUsage: %s\n", get_hexint64_str(pvm->QuotaPeakNonPagedPoolUsage));
                printf("  QuotaNonPagedPoolUsage: %s\n", get_hexint64_str(pvm->QuotaNonPagedPoolUsage));
                printf("  PagefileUsage: %s\n", get_hexint64_str(pvm->PagefileUsage));
                printf("  PeakPagefileUsage: %s\n", get_hexint64_str(pvm->PeakPagefileUsage));
                if (pvm->Revision == 1)
                    printf("  PrivateUsage: %s\n", get_hexint64_str(((const MINIDUMP_PROCESS_VM_COUNTERS_1 *)stream)->PrivateUsage));
                else
                {
                    if (pvm->Flags & MINIDUMP_PROCESS_VM_COUNTERS_VIRTUALSIZE)
                    {
                        printf("  PeakVirtualSize: %s\n", get_hexint64_str(pvm->PeakVirtualSize));
                        printf("  VirtualSize: %s\n", get_hexint64_str(pvm->VirtualSize));
                    }
                    if (pvm->Flags & MINIDUMP_PROCESS_VM_COUNTERS_EX)
                        printf("  PrivateUsage: %s\n", get_hexint64_str(pvm->PrivateUsage));
                    if (pvm->Flags & MINIDUMP_PROCESS_VM_COUNTERS_EX2)
                    {
                        printf("  PrivateWorkingSetSize: %s\n", get_hexint64_str(pvm->PrivateWorkingSetSize));
                        printf("  SharedCommitUsage: %s\n", get_hexint64_str(pvm->SharedCommitUsage));
                    }

                    if (pvm->Flags & MINIDUMP_PROCESS_VM_COUNTERS_JOB)
                    {
                        printf("  JobSharedCommitUsage: %s\n", get_hexint64_str(pvm->JobSharedCommitUsage));
                        printf("  JobPrivateCommitUsage: %s\n", get_hexint64_str(pvm->JobPrivateCommitUsage));
                        printf("  JobPeakPrivateCommitUsage: %s\n", get_hexint64_str(pvm->JobPeakPrivateCommitUsage));
                        printf("  JobPrivateCommitLimit: %s\n", get_hexint64_str(pvm->JobPrivateCommitLimit));
                        printf("  JobTotalCommitLimit: %s\n", get_hexint64_str(pvm->JobTotalCommitLimit));
                    }
                }
            }
            break;
        case TokenStream:
            if (globals_dump_sect("token"))
            {
                const MINIDUMP_TOKEN_INFO_LIST *til = stream;
                const MINIDUMP_TOKEN_INFO_HEADER *ti;

                printf("Stream [%u]: Token info list:\n", idx);
                printf("  TokenListSize: %u\n", til->TokenListSize);
                printf("  TokenListEntries: %u\n", til->TokenListEntries);
                printf("  ListHeaderSize: %u\n", til->ListHeaderSize);
                printf("  ElementHeaderSize: %u\n", til->ElementHeaderSize);

                ti = (const MINIDUMP_TOKEN_INFO_HEADER *)(til + 1);
                if (til->ListHeaderSize >= sizeof(*ti))
                {
                    for (i = 0; i < til->TokenListEntries; ++i)
                    {
                        printf("  Token #%u:\n", i);
                        printf("    TokenSize: %u\n", ti->TokenSize);
                        printf("    TokenId: %u\n", ti->TokenId);
                        printf("    TokenHandle:: %s\n", get_hexint64_str(ti->TokenHandle));
                        if (globals_dump_sect("content"))
                            dump_data((const BYTE *)ti + til->ListHeaderSize, ti->TokenSize - til->ListHeaderSize, "      ");
                        ti = (const MINIDUMP_TOKEN_INFO_HEADER *)((const BYTE *)ti + ti->TokenSize);
                    }
                }
                else printf("  ### bad token entry\n");
            }
            break;
        case ThreadNamesStream:
            if (globals_dump_sect("thread"))
            {
                const MINIDUMP_THREAD_NAME_LIST *tnl = stream;
                const MINIDUMP_THREAD_NAME *tn = tnl->ThreadNames;

                printf("Stream [%u]: Thread name list:\n", idx);
                printf("  NumberOfThreadNames: %u\n", (UINT)tnl->NumberOfThreadNames);
                for (i = 0; i < tnl->NumberOfThreadNames; i++, tn++)
                {
                    printf("  Thread #%u\n", i);
                    printf("    ThreadId: %#x\n", (UINT)tn->ThreadId);
                    printf("    ThreadName: %s\n", get_mdmp_str(tn->RvaOfThreadName));
                }
            }
            break;
        case CommentStreamA:
            {
                /* MSDN states an ANSI string, so stopping at first '\0' if any */
                const char *end = memchr(stream, '\0', dir->Location.DataSize);
                printf("Stream [%u]: CommentA\n", idx);
                printf("--- start of comment\n");
                write(1, stream, end ? end - (const char*)stream : dir->Location.DataSize);
                printf("---  end  of comment\n");
                if (globals_dump_sect("content"))
                    dump_mdmp_data(&dir->Location, "  ");
            }
            break;
        case CommentStreamW:
            {
                const WCHAR *ptr;

                /* MSDN states an UNICODE string, so stopping at first L'\0' if any */
                printf("Stream [%u]: CommentW\n", idx);
                printf("--- start of comment\n");
                for (ptr = stream; ptr + 1 <= (const WCHAR *)((const char *)stream + dir->Location.DataSize) && *ptr; ptr++)
                {
                    if (*ptr== L'\n' || *ptr== L'\r' || *ptr== L'\t' || (*ptr >= L' ' && *ptr < 127))
                        putchar((char)*ptr);
                    else printf("\\u%04x", *ptr);
                }
                printf("---  end  of comment\n");
                if (globals_dump_sect("content"))
                    dump_mdmp_data(&dir->Location, "  ");
            }
            break;
        default:
            printf("Stream [%u]: NIY %d\n", idx, dir->StreamType);
            printf("  RVA: %#x\n", (UINT)dir->Location.Rva);
            printf("  Size: %u\n", dir->Location.DataSize);
            if (globals_dump_sect("content"))
                dump_mdmp_data(&dir->Location, "    ");
            break;
        }
    }
}
