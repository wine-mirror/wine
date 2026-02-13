// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "uprefs.h"
#if U_PLATFORM_USES_ONLY_WIN32_API && UCONFIG_USE_WINDOWS_PREFERENCES_LIBRARY
#include "unicode/ustring.h"
#include "cmemory.h"
#include "charstr.h"
#include "cstring.h"
#include "cwchar.h"
#include <windows.h>

U_NAMESPACE_USE

// Older versions of the Windows SDK don’t have definitions for calendar types that were added later on.
// (For example, the Windows 7 SDK doesn’t have CAL_PERSIAN).
// So we’ll need to provide our own definitions for some of them.
// Note that on older versions of the OS these values won't ever be returned by the platform APIs, so providing our own definitions is fine.
#ifndef CAL_PERSIAN
#define CAL_PERSIAN                    22     // Persian (Solar Hijri) calendar
#endif

#define RETURN_FAILURE_STRING_WITH_STATUS_IF(condition, error, status)  \
    if (condition)                                                      \
    {                                                                   \
        *status = error;                                                \
        return CharString();                                            \
    }

#define RETURN_FAILURE_WITH_STATUS_IF(condition, error, status)         \
    if (condition)                                                      \
    {                                                                   \
        *status = error;                                                \
        return 0;                                                       \
    }

#define RETURN_VALUE_IF(condition, value)                               \
    if (condition)                                                      \
    {                                                                   \
        return value;                                                   \
    }

#define RETURN_WITH_ALLOCATION_ERROR_IF_FAILED(status)                  \
    if (U_FAILURE(*status))                                             \
    {                                                                   \
        *status = U_MEMORY_ALLOCATION_ERROR;                            \
        return CharString();                                            \
    }                                                                   \
// -------------------------------------------------------
// ----------------- MAPPING FUNCTIONS--------------------
// -------------------------------------------------------

// Maps from a NLS Calendar ID (CALID) to a BCP47 Unicode Extension calendar identifier.
//
// We map the NLS CALID from GetLocaleInfoEx to the calendar identifier
// used in BCP47 tag with Unicode Extensions.
//
// This does not return a full nor valid BCP47Tag, it only returns the option that the BCP47 tag
// would return after the "ca-" part
//
// For example:
//   CAL_GREGORIAN would return "gregory".
//   CAL_HIJRI would return "islamic".
//
// These could be used in a BCP47 tag like this: "en-US-u-ca-gregory".
// Note that there are some NLS calendars that are not supported with the BCP47 U extensions,
// and vice-versa.
//
// NLS CALID reference:https://docs.microsoft.com/en-us/windows/win32/intl/calendar-identifiers
CharString getCalendarBCP47FromNLSType(int32_t calendar, UErrorCode* status)
{
    switch(calendar)
    {
        case CAL_GREGORIAN:
        case CAL_GREGORIAN_US:
        case CAL_GREGORIAN_ME_FRENCH:
        case CAL_GREGORIAN_ARABIC:
        case CAL_GREGORIAN_XLIT_ENGLISH:
        case CAL_GREGORIAN_XLIT_FRENCH:
            return CharString("gregory", *status);

        case CAL_JAPAN:
            return CharString("japanese", *status);

        case CAL_TAIWAN:
            return CharString("roc", *status);

        case CAL_KOREA:
            return CharString("dangi", *status);

        case CAL_HIJRI:
            return CharString("islamic", *status);

        case CAL_THAI:
            return CharString("buddhist", *status);

        case CAL_HEBREW:
            return CharString("hebrew", *status);

        case CAL_PERSIAN:
            return CharString("persian", *status);

        case CAL_UMALQURA:
            return CharString("islamic-umalqura", *status);

        default:
            return CharString();
    }
}

// Maps from a NLS Alternate sorting system to a BCP47 U extension sorting system.
//
// We map the alternate sorting method from GetLocaleInfoEx to the sorting method
// used in BCP47 tag with Unicode Extensions.
//
// This does not return a full nor valid BCP47Tag, it only returns the option that the BCP47 tag
// would return after the "co-" part
//
// For example:
//   "phoneb" (parsed from "de-DE_phoneb") would return "phonebk".
//   "radstr" (parsed from "ja-JP_radstr") would return "unihan".
//
// These could be used in a BCP47 tag like this: "de-DE-u-co-phonebk".
// Note that there are some NLS Alternate sort methods that are not supported with the BCP47 U extensions,
// and vice-versa.
CharString getSortingSystemBCP47FromNLSType(const wchar_t* sortingSystem, UErrorCode* status)
{
    if (wcscmp(sortingSystem, L"phoneb") == 0) // Phonebook style ordering (such as in German)
    {
        return CharString("phonebk", *status);
    }
    else if (wcscmp(sortingSystem, L"tradnl") == 0) // Traditional style ordering (such as in Spanish)
    {
        return CharString("trad", *status);
    }
    else if (wcscmp(sortingSystem, L"stroke") == 0) // Pinyin ordering for Latin, stroke order for CJK characters (used in Chinese)
    {
        return CharString("stroke", *status);
    }
    else if (wcscmp(sortingSystem, L"radstr") == 0) // Pinyin ordering for Latin, Unihan radical-stroke ordering for CJK characters (used in Chinese)
    {
        return CharString("unihan", *status);
    }
    else if (wcscmp(sortingSystem, L"pronun") == 0) // Phonetic ordering (sorting based on pronunciation)
    {
        return CharString("phonetic", *status);
    }
    else
    {
        return CharString();
    }
}

// Maps from a NLS first day of week value to a BCP47 U extension first day of week.
//
// NLS defines:
// 0 -> Monday, 1 -> Tuesday, ...  5 -> Saturday, 6 -> Sunday
//
// We map the first day of week from GetLocaleInfoEx to the first day of week
// used in BCP47 tag with Unicode Extensions.
//
// This does not return a full nor valid BCP47Tag, it only returns the option that the BCP47 tag
// would return after the "fw-" part
//
// For example:
//   1 (Tuesday) would return "tue".
//   6 (Sunday) would return "sun".
//
// These could be used in a BCP47 tag like this: "en-US-u-fw-sun".
CharString getFirstDayBCP47FromNLSType(int32_t firstday, UErrorCode* status)
{
    switch(firstday)
    {
        case 0:
            return CharString("mon", *status);

        case 1:
            return CharString("tue", *status);

        case 2:
            return CharString("wed", *status);

        case 3:
            return CharString("thu", *status);

        case 4:
            return CharString("fri", *status);

        case 5:
            return CharString("sat", *status);

        case 6:
            return CharString("sun", *status);

        default:
            return CharString();
    }
}

// Maps from a NLS Measurement system to a BCP47 U extension measurement system.
//
// NLS defines:
// 0 -> Metric system, 1 -> U.S. System
//
// This does not return a full nor valid BCP47Tag, it only returns the option that the BCP47 tag
// would return after the "ms-" part
//
// For example:
//   0 (Metric) would return "metric".
//   6 (U.S. System) would return "ussystem".
//
// These could be used in a BCP47 tag like this: "en-US-u-ms-metric".
CharString getMeasureSystemBCP47FromNLSType(int32_t measureSystem, UErrorCode *status)
{
    switch(measureSystem)
    {
        case 0:
            return CharString("metric", *status);
        case 1:
            return CharString("ussystem", *status);
        default:
            return CharString();
    }
}

// -------------------------------------------------------
// --------------- END OF MAPPING FUNCTIONS --------------
// -------------------------------------------------------

// -------------------------------------------------------
// ------------------ HELPER FUCTIONS  -------------------
// -------------------------------------------------------

// Return the CLDR "h12" or "h23" format for the 12 or 24 hour clock.
// NLS only gives us a "time format" of a form similar to "h:mm:ss tt"
// The NLS "h" is 12 hour, and "H" is 24 hour, so we'll scan for the
// first h or H.
// Note that the NLS string could have sections escaped with single
// quotes, so be sure to skip those parts. Eg: "'Hours:' h:mm:ss"
// would skip the "H" in 'Hours' and use the h in the actual pattern.
CharString get12_or_24hourFormat(wchar_t* hourFormat, UErrorCode* status)
{
    bool isInEscapedString = false;
    const int32_t hourLength = static_cast<int32_t>(uprv_wcslen(hourFormat));
    for (int32_t i = 0; i < hourLength; i++)
    {
        // Toggle escaped flag if in ' quoted portion
        if (hourFormat[i] == L'\'')
        {
            isInEscapedString = !isInEscapedString;
        }

        if (!isInEscapedString)
        {
            // Check for both so we can escape early
            if (hourFormat[i] == L'H')
            {
                return CharString("h23", *status);
            }

            if (hourFormat[i] == L'h')
            {
                return CharString("h12", *status);
            }
        }
    }
    // default to a 24 hour clock as that's more common worldwide
    return CharString("h23", *status);
}

UErrorCode getUErrorCodeFromLastError()
{
    DWORD error = GetLastError();
    switch(error)
    {
        case ERROR_INSUFFICIENT_BUFFER:
            return U_BUFFER_OVERFLOW_ERROR;

        case ERROR_INVALID_FLAGS:
        case ERROR_INVALID_PARAMETER:
            return U_ILLEGAL_ARGUMENT_ERROR;

        case ERROR_OUTOFMEMORY:
            return U_MEMORY_ALLOCATION_ERROR;

        default:
            return U_INTERNAL_PROGRAM_ERROR;
    }
}

int32_t GetLocaleInfoExWrapper(LPCWSTR lpLocaleName, LCTYPE LCType, LPWSTR lpLCData, int cchData, UErrorCode* status)
{
    RETURN_VALUE_IF(U_FAILURE(*status), 0);

#ifndef UPREFS_TEST
    *status = U_ZERO_ERROR;
    int32_t result = GetLocaleInfoEx(lpLocaleName, LCType, lpLCData, cchData);

    if (result == 0)
    {
        *status = getUErrorCodeFromLastError();
    }
    return result;
#else
    #include "uprefstest.h"
    UPrefsTest prefTests;
    return prefTests.MockGetLocaleInfoEx(lpLocaleName, LCType, lpLCData, cchData, status);
#endif
}

// Copies a string to a buffer if its size allows it and returns the size.
// The returned needed buffer size includes the terminating \0 null character.
// If the buffer's size is set to 0, the needed buffer size is returned before copying the string.
int32_t checkBufferCapacityAndCopy(const char* uprefsString, char* uprefsBuffer, int32_t bufferSize, UErrorCode* status)
{
    int32_t neededBufferSize = static_cast<int32_t>(uprv_strlen(uprefsString) + 1);

    RETURN_VALUE_IF(bufferSize == 0, neededBufferSize);
    RETURN_FAILURE_WITH_STATUS_IF(neededBufferSize > bufferSize, U_BUFFER_OVERFLOW_ERROR, status);

    uprv_strcpy(uprefsBuffer, uprefsString);

    return neededBufferSize;
}

CharString getLocaleBCP47Tag_impl(UErrorCode* status, bool getSorting)
{
    // First part of a bcp47 tag looks like an NLS user locale, so we get the NLS user locale.
    int32_t neededBufferSize = GetLocaleInfoExWrapper(LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME, nullptr, 0, status);

    RETURN_VALUE_IF(U_FAILURE(*status), CharString());

    MaybeStackArray<wchar_t, LOCALE_NAME_MAX_LENGTH> NLSLocale(neededBufferSize, *status);
    RETURN_WITH_ALLOCATION_ERROR_IF_FAILED(status);

    int32_t result = GetLocaleInfoExWrapper(LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME, NLSLocale.getAlias(), neededBufferSize, status);

    RETURN_VALUE_IF(U_FAILURE(*status), CharString());

    if (getSorting) //We determine if we want the locale (for example, en-US) or the sorting method (for example, phonebk)
    {
        // We use LOCALE_SNAME to get the sorting method (if any). So we need to keep
        // only the sorting bit after the _, removing the locale name.
        // Example: from "de-DE_phoneb" we only want "phoneb"
        const wchar_t * startPosition = wcschr(NLSLocale.getAlias(), L'_');

        // Note: not finding a "_" is not an error, it means the user has not selected an alternate sorting method, which is fine.
        if (startPosition != nullptr)
        {
            CharString sortingSystem = getSortingSystemBCP47FromNLSType(startPosition + 1, status);

            if (sortingSystem.length() == 0)
            {
                *status = U_UNSUPPORTED_ERROR;
                return CharString();
            }
            return sortingSystem;
        }
    }
    else
    {
        // The NLS locale may include a non-default sort, such as de-DE_phoneb. We only want the locale name before the _.
        wchar_t * position = wcschr(NLSLocale.getAlias(), L'_');
        if (position != nullptr)
        {
            *position = L'\0';
        }

        CharString languageTag;
        int32_t resultCapacity = 0;
        languageTag.getAppendBuffer(neededBufferSize, neededBufferSize, resultCapacity, *status);
        RETURN_WITH_ALLOCATION_ERROR_IF_FAILED(status);

        int32_t unitsWritten = 0;
        u_strToUTF8(languageTag.data(), neededBufferSize, &unitsWritten, reinterpret_cast<UChar*>(NLSLocale.getAlias()), neededBufferSize, status);
        RETURN_VALUE_IF(U_FAILURE(*status), CharString());
        return languageTag;
    }

    return CharString();
}

CharString getCalendarSystem_impl(UErrorCode* status)
{
    int32_t NLSCalendar = 0;

    GetLocaleInfoExWrapper(LOCALE_NAME_USER_DEFAULT, LOCALE_ICALENDARTYPE | LOCALE_RETURN_NUMBER, reinterpret_cast<PWSTR>(&NLSCalendar), sizeof(NLSCalendar) / sizeof(wchar_t), status);

    RETURN_VALUE_IF(U_FAILURE(*status), CharString());

    CharString calendar(getCalendarBCP47FromNLSType(NLSCalendar, status), *status);
    RETURN_FAILURE_STRING_WITH_STATUS_IF(calendar.length() == 0, U_UNSUPPORTED_ERROR, status);

    return calendar;
}

CharString getCurrencyCode_impl(UErrorCode* status)
{
    int32_t neededBufferSize = GetLocaleInfoExWrapper(LOCALE_NAME_USER_DEFAULT, LOCALE_SINTLSYMBOL, nullptr, 0, status);

    RETURN_VALUE_IF(U_FAILURE(*status), CharString());

    MaybeStackArray<wchar_t, 10> NLScurrencyData(neededBufferSize, *status);
    RETURN_WITH_ALLOCATION_ERROR_IF_FAILED(status);

    int32_t result = GetLocaleInfoExWrapper(LOCALE_NAME_USER_DEFAULT, LOCALE_SINTLSYMBOL, NLScurrencyData.getAlias(), neededBufferSize, status);

    RETURN_VALUE_IF(U_FAILURE(*status), CharString());

    MaybeStackArray<char, 10> currency(neededBufferSize, *status);
    RETURN_WITH_ALLOCATION_ERROR_IF_FAILED(status);

    int32_t unitsWritten = 0;
    u_strToUTF8(currency.getAlias(), neededBufferSize, &unitsWritten, reinterpret_cast<UChar*>(NLScurrencyData.getAlias()), neededBufferSize, status);
    RETURN_VALUE_IF(U_FAILURE(*status), CharString());

    if (unitsWritten == 0)
    {
        *status = U_INTERNAL_PROGRAM_ERROR;
        return CharString();
    }

    // Since we retreived the currency code in caps, we need to make it lowercase for it to be in CLDR BCP47 U extensions format.
    T_CString_toLowerCase(currency.getAlias());

    return CharString(currency.getAlias(), neededBufferSize, *status);
}

CharString getFirstDayOfWeek_impl(UErrorCode* status)
{
    int32_t NLSfirstDay = 0;
    GetLocaleInfoExWrapper(LOCALE_NAME_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK | LOCALE_RETURN_NUMBER, reinterpret_cast<PWSTR>(&NLSfirstDay), sizeof(NLSfirstDay) / sizeof(wchar_t), status);

    RETURN_VALUE_IF(U_FAILURE(*status), CharString());

    CharString firstDay = getFirstDayBCP47FromNLSType(NLSfirstDay, status);
    RETURN_FAILURE_STRING_WITH_STATUS_IF(firstDay.length() == 0, U_UNSUPPORTED_ERROR, status);

    return firstDay;
}

CharString getHourCycle_impl(UErrorCode* status)
{
    int32_t neededBufferSize = GetLocaleInfoExWrapper(LOCALE_NAME_USER_DEFAULT, LOCALE_STIMEFORMAT, nullptr, 0, status);

    RETURN_VALUE_IF(U_FAILURE(*status), CharString());

    MaybeStackArray<wchar_t, 40> NLShourCycle(neededBufferSize, *status);
    RETURN_WITH_ALLOCATION_ERROR_IF_FAILED(status);

    int32_t result = GetLocaleInfoExWrapper(LOCALE_NAME_USER_DEFAULT, LOCALE_STIMEFORMAT, NLShourCycle.getAlias(), neededBufferSize, status);

    RETURN_VALUE_IF(U_FAILURE(*status), CharString());

    CharString hourCycle = get12_or_24hourFormat(NLShourCycle.getAlias(), status);
    if (hourCycle.length() == 0)
    {
        *status = U_INTERNAL_PROGRAM_ERROR;
        return CharString();
    }
    return hourCycle;
}

CharString getMeasureSystem_impl(UErrorCode* status)
{
    int32_t NLSmeasureSystem = 0;
    GetLocaleInfoExWrapper(LOCALE_NAME_USER_DEFAULT, LOCALE_IMEASURE | LOCALE_RETURN_NUMBER, reinterpret_cast<PWSTR>(&NLSmeasureSystem), sizeof(NLSmeasureSystem) / sizeof(wchar_t), status);

    RETURN_VALUE_IF(U_FAILURE(*status), CharString());

    CharString measureSystem = getMeasureSystemBCP47FromNLSType(NLSmeasureSystem, status);
    RETURN_FAILURE_STRING_WITH_STATUS_IF(measureSystem.length() == 0, U_UNSUPPORTED_ERROR, status);

    return measureSystem;
}

void appendIfDataNotEmpty(CharString& dest, const char* firstData, const char* secondData, bool& warningGenerated, UErrorCode* status)
{
    if (*status == U_UNSUPPORTED_ERROR)
    {
        warningGenerated = true;
        *status = U_ZERO_ERROR;
    }

    if (uprv_strlen(secondData) != 0)
    {
        dest.append(firstData, *status);
        dest.append(secondData, *status);
    }
}
// -------------------------------------------------------
// --------------- END OF HELPER FUNCTIONS ---------------
// -------------------------------------------------------


// -------------------------------------------------------
// ---------------------- APIs ---------------------------
// -------------------------------------------------------

// Gets the valid and canonical BCP47 tag with the user settings for Language, Calendar, Sorting, Currency,
// First day of week, Hour cycle, and Measurement system.
// Calls all of the other APIs
// Returns the needed buffer size for the BCP47 Tag.
int32_t uprefs_getBCP47Tag(char* uprefsBuffer, int32_t bufferSize, UErrorCode* status)
{
    RETURN_FAILURE_WITH_STATUS_IF(uprefsBuffer == nullptr && bufferSize != 0, U_ILLEGAL_ARGUMENT_ERROR, status);

    *status = U_ZERO_ERROR;
    CharString BCP47Tag;
    bool warningGenerated = false;

    CharString languageTag = getLocaleBCP47Tag_impl(status, false);
    RETURN_VALUE_IF(U_FAILURE(*status), 0);
    BCP47Tag.append(languageTag.data(), *status);
    BCP47Tag.append("-u", *status);

    CharString calendar = getCalendarSystem_impl(status);
    RETURN_VALUE_IF(U_FAILURE(*status) && *status != U_UNSUPPORTED_ERROR, 0);
    appendIfDataNotEmpty(BCP47Tag, "-ca-", calendar.data(), warningGenerated, status);

    CharString sortingSystem = getLocaleBCP47Tag_impl(status, true);
    RETURN_VALUE_IF(U_FAILURE(*status) && *status != U_UNSUPPORTED_ERROR, 0);
    appendIfDataNotEmpty(BCP47Tag, "-co-", sortingSystem.data(), warningGenerated, status);

    CharString currency = getCurrencyCode_impl(status);
    RETURN_VALUE_IF(U_FAILURE(*status) && *status != U_UNSUPPORTED_ERROR, 0);
    appendIfDataNotEmpty(BCP47Tag, "-cu-", currency.data(), warningGenerated, status);

    CharString firstDay = getFirstDayOfWeek_impl(status);
    RETURN_VALUE_IF(U_FAILURE(*status) && *status != U_UNSUPPORTED_ERROR, 0);
    appendIfDataNotEmpty(BCP47Tag, "-fw-", firstDay.data(), warningGenerated, status);

    CharString hourCycle = getHourCycle_impl(status);
    RETURN_VALUE_IF(U_FAILURE(*status) && *status != U_UNSUPPORTED_ERROR, 0);
    appendIfDataNotEmpty(BCP47Tag, "-hc-", hourCycle.data(), warningGenerated, status);

    CharString measureSystem = getMeasureSystem_impl(status);
    RETURN_VALUE_IF(U_FAILURE(*status) && *status != U_UNSUPPORTED_ERROR, 0);
    appendIfDataNotEmpty(BCP47Tag, "-ms-", measureSystem.data(), warningGenerated, status);

    if (warningGenerated)
    {
        *status = U_USING_FALLBACK_WARNING;
    }

    return checkBufferCapacityAndCopy(BCP47Tag.data(), uprefsBuffer, bufferSize, status);
}

// -------------------------------------------------------
// ---------------------- END OF APIs --------------------
// -------------------------------------------------------

#endif // U_PLATFORM_USES_ONLY_WIN32_API && UCONFIG_USE_WINDOWS_PREFERENCES_LIBRARY
