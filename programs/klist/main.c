/*
 * Copyright 2023 Maxim Karasev <mxkrsv@etersoft.ru>
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

#define WIN32_LEAN_AND_MEAN
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <ntsecapi.h>

#include "wine/debug.h"

#include "resources.h"

WINE_DEFAULT_DEBUG_CHANNEL(klist);

static WCHAR msg_buf[1024];
static const WCHAR *load_resource(UINT id)
{
    LoadStringW(GetModuleHandleW(NULL), id, msg_buf, ARRAY_SIZE(msg_buf));
    return msg_buf;
}

static const WCHAR *get_etype_text(LONG encryption_type)
{
    switch (encryption_type) {
#define EXPAND_ETYPE(x) case KERB_ETYPE_##x: return L ## #x
    EXPAND_ETYPE(NULL);
    EXPAND_ETYPE(DES_CBC_CRC);
    EXPAND_ETYPE(DES_CBC_MD4);
    EXPAND_ETYPE(DES_CBC_MD5);
    EXPAND_ETYPE(AES128_CTS_HMAC_SHA1_96);
    EXPAND_ETYPE(AES256_CTS_HMAC_SHA1_96);
    EXPAND_ETYPE(RC4_MD4);
    EXPAND_ETYPE(RC4_PLAIN2);
    EXPAND_ETYPE(RC4_LM);
    EXPAND_ETYPE(RC4_SHA);
    EXPAND_ETYPE(DES_PLAIN);
    EXPAND_ETYPE(RC4_HMAC_OLD);
    EXPAND_ETYPE(RC4_PLAIN_OLD);
    EXPAND_ETYPE(RC4_HMAC_OLD_EXP);
    EXPAND_ETYPE(RC4_PLAIN_OLD_EXP);
    EXPAND_ETYPE(RC4_PLAIN);
    EXPAND_ETYPE(RC4_PLAIN_EXP);
    EXPAND_ETYPE(AES128_CTS_HMAC_SHA1_96_PLAIN);
    EXPAND_ETYPE(AES256_CTS_HMAC_SHA1_96_PLAIN);
    EXPAND_ETYPE(DSA_SHA1_CMS);
    EXPAND_ETYPE(RSA_MD5_CMS);
    EXPAND_ETYPE(RSA_SHA1_CMS);
    EXPAND_ETYPE(RC2_CBC_ENV);
    EXPAND_ETYPE(RSA_ENV);
    EXPAND_ETYPE(RSA_ES_OEAP_ENV);
    EXPAND_ETYPE(DES_EDE3_CBC_ENV);
    EXPAND_ETYPE(DSA_SIGN);
    EXPAND_ETYPE(DES3_CBC_MD5);
    EXPAND_ETYPE(DES3_CBC_SHA1);
    EXPAND_ETYPE(DES3_CBC_SHA1_KD);
    EXPAND_ETYPE(DES_CBC_MD5_NT);
    EXPAND_ETYPE(RC4_HMAC_NT);
    EXPAND_ETYPE(RC4_HMAC_NT_EXP);
#undef EXPAND_ETYPE
    default: return NULL;
    }
}

static BOOL get_process_logon_id(LUID *logon_id)
{
    HANDLE token_handle;
    TOKEN_STATISTICS token_statistics;
    DWORD token_statistics_len;
    BOOL err;

    err = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token_handle);
    if (!err)
    {
        ERR("OpenProcessToken failed\n");
        return FALSE;
    }

    err = GetTokenInformation(token_handle, TokenStatistics, &token_statistics,
            sizeof(token_statistics), &token_statistics_len);
    if (!err)
    {
        ERR("GetTokenInformation failed\n");
        return FALSE;
    }

    *logon_id = token_statistics.AuthenticationId;

    return TRUE;
}

static void format_dates_and_times(const FILETIME *const filetimes[],
        const WCHAR *output_strings[], ULONG count)
{
    ULONG i;

    for (i = 0; i < count; i++)
    {
        SYSTEMTIME system_time;
        int date_len;
        int time_len;
        WCHAR *date;

        FileTimeToSystemTime(filetimes[i], &system_time);
        SystemTimeToTzSpecificLocalTime(NULL, &system_time, &system_time);

        date_len = GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE, &system_time, NULL, NULL, 0, NULL);
        time_len = GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &system_time, NULL, NULL, 0);

        date = malloc((date_len + time_len) * sizeof(date[0]));

        GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE, &system_time, NULL, date, date_len, NULL);
        wcscat(date, L" ");
        GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &system_time, NULL, date + wcslen(date), time_len);

        output_strings[i] = date;
    }
}

static int tickets(void)
{
    LUID logon_id;
    BOOL status;
    HANDLE lsa_handle;
    NTSTATUS err;
    ULONG i;
    LSA_STRING package_name = {
        .Buffer = (char*)MICROSOFT_KERBEROS_NAME_A,
        .Length = strlen(MICROSOFT_KERBEROS_NAME_A),
        .MaximumLength = strlen(MICROSOFT_KERBEROS_NAME_A),
    };
    ULONG kerberos_package;

    KERB_QUERY_TKT_CACHE_REQUEST kerberos_cache_request = {
        .MessageType = KerbQueryTicketCacheMessage,
        .LogonId = {0},
    };
    KERB_QUERY_TKT_CACHE_RESPONSE *kerberos_cache;
    ULONG kerberos_cache_size;
    NTSTATUS kerberos_call_status;

    status = get_process_logon_id(&logon_id);
    if (!status)
    {
        wprintf(load_resource(STRING_UNKNOWN_ERROR));
        return 1;
    }

    err = LsaConnectUntrusted(&lsa_handle);
    if (err != STATUS_SUCCESS)
    {
        wprintf(load_resource(STRING_UNKNOWN_ERROR));
        ERR("LsaConnectUntrusted NTSTATUS %lX\n", err);
        return 1;
    }

    err = LsaLookupAuthenticationPackage(lsa_handle, &package_name, &kerberos_package);
    if (err != STATUS_SUCCESS)
    {
        wprintf(load_resource(STRING_UNKNOWN_ERROR));
        ERR("LsaLookupAuthenticationPackage NTSTATUS %lX\n", err);
        return 1;
    }

    TRACE("Kerberos LSA package: %lu\n", kerberos_package);

    err = LsaCallAuthenticationPackage(lsa_handle, kerberos_package, &kerberos_cache_request,
            sizeof(kerberos_cache_request), (PVOID*)&kerberos_cache, &kerberos_cache_size,
            &kerberos_call_status);
    if (err != STATUS_SUCCESS)
    {
        wprintf(load_resource(STRING_UNKNOWN_ERROR));
        ERR("LsaCallAuthenticationPackage NTSTATUS %lX\n", err);
        return 1;
    }

    wprintf(L"\n");
    wprintf(L"%ls %ld:0x%lx\n", load_resource(STRING_LOGON_ID), logon_id.HighPart, logon_id.LowPart);
    wprintf(L"\n");
    wprintf(L"%ls: (%d)\n", load_resource(STRING_CACHED_TICKETS), kerberos_cache->CountOfTickets);

    for (i = 0; i < kerberos_cache->CountOfTickets; i++)
    {
        KERB_TICKET_CACHE_INFO ticket = kerberos_cache->Tickets[i];
        const FILETIME *const filetimes[] = { (FILETIME*)&ticket.StartTime,
            (FILETIME*)&ticket.EndTime, (FILETIME*)&ticket.RenewTime };
        const WCHAR *dates[3];
        const WCHAR *etype_text;

        format_dates_and_times(filetimes, dates, 3);

        wprintf(L"\n");
        wprintf(L"#%ld>", i);

        wprintf(L"     %ls: %.*ls @ %.*ls\n", load_resource(STRING_SERVER),
                ticket.ServerName.Length / sizeof(WCHAR), ticket.ServerName.Buffer,
                ticket.RealmName.Length / sizeof(WCHAR), ticket.RealmName.Buffer);

        etype_text = get_etype_text(ticket.EncryptionType);
        wprintf(L"        %ls: ", load_resource(STRING_ENCRYPTION_TYPE));
        if (etype_text)
        {
            wprintf(L"%s\n", etype_text);
        }
        else
        {
            wprintf(L"%ld\n", ticket.EncryptionType);
        }

        wprintf(L"        %ls: 0x%lx ->", load_resource(STRING_TICKET_FLAGS), ticket.TicketFlags);
#define EXPAND_TICKET_FLAG(x) if (ticket.TicketFlags & KERB_TICKET_FLAGS_##x) wprintf(L" %ls", L ## #x)
        EXPAND_TICKET_FLAG(forwardable);
        EXPAND_TICKET_FLAG(forwarded);
        EXPAND_TICKET_FLAG(proxiable);
        EXPAND_TICKET_FLAG(proxy);
        EXPAND_TICKET_FLAG(may_postdate);
        EXPAND_TICKET_FLAG(postdated);
        EXPAND_TICKET_FLAG(invalid);
        EXPAND_TICKET_FLAG(renewable);
        EXPAND_TICKET_FLAG(initial);
        EXPAND_TICKET_FLAG(pre_authent);
        EXPAND_TICKET_FLAG(hw_authent);
        EXPAND_TICKET_FLAG(ok_as_delegate);
        EXPAND_TICKET_FLAG(name_canonicalize);
        EXPAND_TICKET_FLAG(cname_in_pa_data);
#undef EXPAND_TICKET_FLAG
        wprintf(L"\n");

        wprintf(L"        %ls: %ls (local)\n",   load_resource(STRING_START_TIME), dates[0]);
        wprintf(L"        %ls:   %ls (local)\n", load_resource(STRING_END_TIME),   dates[1]);
        wprintf(L"        %ls: %ls (local)\n",   load_resource(STRING_RENEW_TIME), dates[2]);
    }
    LsaFreeReturnBuffer(kerberos_cache);
    LsaDeregisterLogonProcess(lsa_handle);
    return 0;
}

static int tgt(void)
{
    FIXME("stub\n");

    return 0;
}

static int purge(void)
{
    FIXME("stub\n");

    return 0;
}

static int get(const WCHAR *principal)
{
    FIXME("stub\n");

    return 0;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    if (argc < 2)
    {
        return tickets();
    }

    if (!wcscmp(argv[1], L"tickets"))
    {
        return tickets();
    }
    else if (!wcscmp(argv[1], L"tgt"))
    {
        return tgt();
    }
    else if (!wcscmp(argv[1], L"purge"))
    {
        return purge();
    }
    else if (!wcscmp(argv[1], L"get"))
    {
        if (argc < 3)
        {
            wprintf(load_resource(STRING_USAGE));
            return 1;
        }

        return get(argv[2]);
    }
    else
    {
        wprintf(load_resource(STRING_USAGE));
        return 1;
    }
}
