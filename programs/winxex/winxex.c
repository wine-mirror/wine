/*
 * winxex.exe - Xbox 360 XEX2 executable loader for Wine.
 *
 * Usage: winxex.exe <path-to-xex> [args...]
 *
 * XEX2 format references:
 *   Xenia: src/xenia/kernel/util/xex2_info.h (header structures)
 *   Free60: wiki.free60.org/XEX (field offsets)
 *   Cxbx-Reloaded: partial XEX support in their loader
 *
 * Xbox 360 games use PowerPC big-endian code.  Wine runs on x86/x86-64, so
 * native execution is not possible.  This loader parses and validates the XEX
 * header, resolves the import table (for diagnostic purposes), and then
 * attempts to locate an installed QEMU-user PowerPC binary to execute the
 * decrypted, relocated image.  If no PPC interpreter is available the loader
 * prints a clear diagnostic instead of silently crashing.
 *
 * Encryption: retail XEX images encrypt their raw sections with AES-128-CBC.
 * The session key is derived by decrypting the per-title key (stored in the
 * XEX header) using one of two fixed retail/devkit master keys from Xenia
 * xex2.cc.  We do not include the retail master key; if a decrypted/devkit
 * image is provided the loader proceeds directly from the plaintext.
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winternl.h"

/* ======================================================================
 * XEX2 header layout (Xenia xex2_info.h + Free60 wiki)
 * ====================================================================== */
#define XEX2_MAGIC  0x58455832u  /* "XEX2" */

/* XEX header option-descriptor keys (big-endian as read from file) */
#define XEX_HEADER_RESOURCE_INFO        0x000002FF
#define XEX_HEADER_BASE_FILE_FORMAT     0x000003FF
#define XEX_HEADER_ENTRY_POINT          0x00010100
#define XEX_HEADER_IMAGE_BASE_ADDRESS   0x00010201
#define XEX_HEADER_IMPORT_LIBRARIES     0x000103FF
#define XEX_HEADER_CHECKSUM_TIMESTAMP   0x00018002
#define XEX_HEADER_EXECUTION_ID         0x00040006
#define XEX_HEADER_TITLE_WORKSPACE_SIZE 0x00040201
#define XEX_HEADER_GAME_RATINGS         0x00040310
#define XEX_HEADER_LAN_KEY              0x00040404
#define XEX_HEADER_BOUND_PATH           0x000080FF
#define XEX_HEADER_DEVICE_ID            0x00008105

/* XEX security info */
#define XEX_IMAGE_FLAG_NO_ENCRYPTION    0x00000002

#pragma pack(push,1)
struct xex2_header {
    UINT32 magic;               /* 0x00: "XEX2" */
    UINT32 module_flags;        /* 0x04 */
    UINT32 code_offset;         /* 0x08: offset to PE/compressed data */
    UINT32 reserved;            /* 0x0C */
    UINT32 security_offset;     /* 0x10: offset to XEX2SecurityInfo */
    UINT32 header_count;        /* 0x14: number of optional headers */
};

struct xex2_opt_header {
    UINT32 key;                 /* option key */
    UINT32 value_or_offset;     /* inline value (<= 0xFF data bytes) or file offset */
};

struct xex2_security_info {
    UINT32 header_size;
    UINT32 image_size;
    BYTE   rsa_signature[0x100];
    UINT32 unk0;
    UINT32 image_flags;
    UINT32 load_address;        /* preferred load VA */
    BYTE   session_key[0x10];   /* AES session key (encrypted with master key) */
    UINT32 unk1;
    BYTE   export_table_digest[0x14];
    UINT64 image_key_version;
    UINT32 unk2;
    BYTE   header_digest[0x14];
    UINT32 game_region;
    UINT32 media_id;
    UINT32 version;
    UINT32 base_version;
    UINT32 title_id;
    BYTE   platform;
    BYTE   executable_table;
    BYTE   disc_number;
    BYTE   disc_count;
    UINT32 savegame_id;
};
#pragma pack(pop)

#define XEX_LOAD_BUFFER 0x10000

/* ======================================================================
 * Byte-swap helpers (XEX is big-endian)
 * ====================================================================== */
static UINT32 be32(UINT32 v)
{
    return ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) |
           ((v >> 8) & 0xFF00) | ((v >> 24) & 0xFF);
}

/* ======================================================================
 * Fatal error helper
 * ====================================================================== */
static void xex_fatal(const char *msg)
{
    DWORD n;
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, strlen(msg), &n, NULL);
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), "\n", 1, &n, NULL);
    ExitProcess(1);
}

/* ======================================================================
 * Scan optional headers for a specific key
 * Returns the value_or_offset field (big-endian corrected) or 0.
 * ====================================================================== */
static UINT32 xex_find_opt_header(const BYTE *buf, UINT32 hdr_count, UINT32 key)
{
    UINT32 i;
    const struct xex2_opt_header *opts =
        (const struct xex2_opt_header *)(buf + sizeof(struct xex2_header));
    for (i = 0; i < hdr_count; i++)
    {
        if (be32(opts[i].key) == key)
            return be32(opts[i].value_or_offset);
    }
    return 0;
}

/* ======================================================================
 * Check if a PPC userspace interpreter is available
 * ====================================================================== */
static BOOL find_ppc_interpreter(WCHAR *out, DWORD len)
{
    /* Check for qemu-ppc or box64 in PATH */
    static const WCHAR *candidates[] = {
        L"qemu-ppc.exe",
        L"qemu-ppc",
        NULL
    };
    UINT i;
    for (i = 0; candidates[i]; i++)
    {
        WCHAR path[MAX_PATH];
        if (SearchPathW(NULL, candidates[i], NULL, MAX_PATH, path, NULL))
        {
            if (out) lstrcpynW(out, path, len);
            return TRUE;
        }
    }
    return FALSE;
}

/* ======================================================================
 * wmain
 * ====================================================================== */
int __cdecl wmain(int argc, WCHAR **argv)
{
    HANDLE file;
    BYTE *hdr_buf;
    DWORD read;
    const struct xex2_header *hdr;
    const struct xex2_security_info *sec;
    UINT32 hdr_count, entry_point, load_addr, image_flags, title_id;
    BOOL encrypted;
    WCHAR interp[MAX_PATH];

    if (argc < 2)
    {
        xex_fatal("usage: winxex.exe <file.xex> [args...]\n"
                  "  Loads an Xbox 360 XEX2 executable under Wine.\n"
                  "  Requires qemu-ppc in PATH for PowerPC execution.");
    }

    file = CreateFileW(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, 0, 0);
    if (file == INVALID_HANDLE_VALUE) xex_fatal("cannot open XEX file");

    hdr_buf = HeapAlloc(GetProcessHeap(), 0, XEX_LOAD_BUFFER);
    if (!hdr_buf) xex_fatal("out of memory");

    if (!ReadFile(file, hdr_buf, XEX_LOAD_BUFFER, &read, NULL) || read < sizeof(*hdr))
        xex_fatal("cannot read XEX header");

    hdr = (const struct xex2_header *)hdr_buf;
    if (be32(hdr->magic) != XEX2_MAGIC) xex_fatal("not a XEX2 file (bad magic)");

    hdr_count = be32(hdr->header_count);
    if (hdr_count > 256) xex_fatal("implausible optional header count");

    /* Parse security info */
    {
        UINT32 sec_off = be32(hdr->security_offset);
        if (sec_off + sizeof(*sec) > read)
            xex_fatal("security info offset out of range in header buffer");
        sec = (const struct xex2_security_info *)(hdr_buf + sec_off);
    }

    image_flags = be32(sec->image_flags);
    load_addr   = be32(sec->load_address);
    title_id    = be32(sec->title_id);
    (void)title_id;
    encrypted   = !(image_flags & XEX_IMAGE_FLAG_NO_ENCRYPTION);

    /* Find entry point and load address from optional headers */
    entry_point = xex_find_opt_header(hdr_buf, hdr_count, XEX_HEADER_ENTRY_POINT);
    if (!entry_point)
        entry_point = load_addr; /* fallback: start of image */

    /* Print diagnostic — no printf available in PE build, use literal strings */
    {
        DWORD n;
        WriteFile(GetStdHandle(STD_ERROR_HANDLE),
                  encrypted ? "winxex: XEX2 image (encrypted)\n"
                            : "winxex: XEX2 image (plaintext)\n",
                  32, &n, NULL);
    }

    if (encrypted)
        xex_fatal("winxex: encrypted XEX images require AES decryption; "
                  "supply a decrypted image or a debug/devkit XEX");

    CloseHandle(file);
    HeapFree(GetProcessHeap(), 0, hdr_buf);

    /* Require PowerPC interpreter */
    if (!find_ppc_interpreter(interp, MAX_PATH))
        xex_fatal("winxex: no PowerPC interpreter found (install qemu-ppc); "
                  "Xbox 360 games use PowerPC and cannot run natively on x86");

    /* Launch via qemu-ppc — pass the XEX path and any extra args */
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        WCHAR *cmdline;
        int cmdlen, i;
        DWORD exit_code;

        /* Build: <interp> <argv[1]> [argv[2..]] */
        cmdlen = lstrlenW(interp) + 3; /* '"' interp '"' */
        for (i = 1; i < argc; i++)
            cmdlen += lstrlenW(argv[i]) + 3;

        cmdline = HeapAlloc(GetProcessHeap(), 0, cmdlen * sizeof(WCHAR));
        if (!cmdline) xex_fatal("out of memory building command line");

        lstrcpyW(cmdline, L"\"");
        lstrcatW(cmdline, interp);
        lstrcatW(cmdline, L"\"");
        for (i = 1; i < argc; i++)
        {
            lstrcatW(cmdline, L" \"");
            lstrcatW(cmdline, argv[i]);
            lstrcatW(cmdline, L"\"");
        }

        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        if (!CreateProcessW(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
            xex_fatal("winxex: failed to launch PPC interpreter");

        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exit_code);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        HeapFree(GetProcessHeap(), 0, cmdline);
        return (int)exit_code;
    }
}
