@ cdecl UCNV_FROM_U_CALLBACK_ESCAPE(ptr ptr ptr long long long ptr) icu.UCNV_FROM_U_CALLBACK_ESCAPE
@ cdecl UCNV_FROM_U_CALLBACK_SKIP(ptr ptr ptr long long long ptr) icu.UCNV_FROM_U_CALLBACK_SKIP
@ cdecl UCNV_FROM_U_CALLBACK_STOP(ptr ptr ptr long long long ptr) icu.UCNV_FROM_U_CALLBACK_STOP
@ cdecl UCNV_FROM_U_CALLBACK_SUBSTITUTE(ptr ptr ptr long long long ptr) icu.UCNV_FROM_U_CALLBACK_SUBSTITUTE
@ cdecl UCNV_TO_U_CALLBACK_ESCAPE(ptr ptr str long long ptr) icu.UCNV_TO_U_CALLBACK_ESCAPE
@ cdecl UCNV_TO_U_CALLBACK_SKIP(ptr ptr str long long ptr) icu.UCNV_TO_U_CALLBACK_SKIP
@ cdecl UCNV_TO_U_CALLBACK_STOP(ptr ptr str long long ptr) icu.UCNV_TO_U_CALLBACK_STOP
@ cdecl UCNV_TO_U_CALLBACK_SUBSTITUTE(ptr ptr str long long ptr) icu.UCNV_TO_U_CALLBACK_SUBSTITUTE
@ cdecl u_UCharsToChars(ptr str long) icu.u_UCharsToChars
@ cdecl u_austrcpy(str ptr) icu.u_austrcpy
@ cdecl u_austrncpy(str ptr long) icu.u_austrncpy
@ cdecl u_catclose(long) icu.u_catclose
@ cdecl u_catgets(long long long ptr ptr ptr) icu.u_catgets
@ cdecl u_catopen(str str ptr) icu.u_catopen
@ cdecl u_charAge(long long) icu.u_charAge
@ cdecl u_charDigitValue(long) icu.u_charDigitValue
@ cdecl u_charDirection(long) icu.u_charDirection
@ cdecl u_charFromName(long str ptr) icu.u_charFromName
@ cdecl u_charMirror(long) icu.u_charMirror
@ cdecl u_charName(long long str long ptr) icu.u_charName
@ cdecl u_charType(long) icu.u_charType
@ cdecl u_charsToUChars(str ptr long) icu.u_charsToUChars
@ cdecl u_cleanup() icu.u_cleanup
@ cdecl u_countChar32(ptr long) icu.u_countChar32
@ cdecl u_digit(long long) icu.u_digit
@ cdecl u_enumCharNames(long long ptr ptr long ptr) icu.u_enumCharNames
@ cdecl u_enumCharTypes(ptr ptr) icu.u_enumCharTypes
@ cdecl u_errorName(long) icu.u_errorName
@ cdecl u_foldCase(long long) icu.u_foldCase
@ cdecl u_forDigit(long long) icu.u_forDigit
@ cdecl u_getBidiPairedBracket(long) icu.u_getBidiPairedBracket
@ cdecl u_getCombiningClass(long) icu.u_getCombiningClass
@ cdecl u_getDataVersion(long ptr) icu.u_getDataVersion
@ cdecl u_getFC_NFKC_Closure(long ptr long ptr) icu.u_getFC_NFKC_Closure
@ cdecl u_getIntPropertyMaxValue(long) icu.u_getIntPropertyMaxValue
@ cdecl u_getIntPropertyMinValue(long) icu.u_getIntPropertyMinValue
@ cdecl u_getIntPropertyValue(long long) icu.u_getIntPropertyValue
@ cdecl u_getNumericValue(long) icu.u_getNumericValue
@ cdecl u_getPropertyEnum(str) icu.u_getPropertyEnum
@ cdecl u_getPropertyName(long long) icu.u_getPropertyName
@ cdecl u_getPropertyValueEnum(long str) icu.u_getPropertyValueEnum
@ cdecl u_getPropertyValueName(long long long) icu.u_getPropertyValueName
@ cdecl u_getUnicodeVersion(long) icu.u_getUnicodeVersion
@ cdecl u_getVersion(long) icu.u_getVersion
@ cdecl u_hasBinaryProperty(long long) icu.u_hasBinaryProperty
@ cdecl u_init(ptr) icu.u_init
@ cdecl u_isIDIgnorable(long) icu.u_isIDIgnorable
@ cdecl u_isIDPart(long) icu.u_isIDPart
@ cdecl u_isIDStart(long) icu.u_isIDStart
@ cdecl u_isISOControl(long) icu.u_isISOControl
@ cdecl u_isJavaIDPart(long) icu.u_isJavaIDPart
@ cdecl u_isJavaIDStart(long) icu.u_isJavaIDStart
@ cdecl u_isJavaSpaceChar(long) icu.u_isJavaSpaceChar
@ cdecl u_isMirrored(long) icu.u_isMirrored
@ cdecl u_isUAlphabetic(long) icu.u_isUAlphabetic
@ cdecl u_isULowercase(long) icu.u_isULowercase
@ cdecl u_isUUppercase(long) icu.u_isUUppercase
@ cdecl u_isUWhiteSpace(long) icu.u_isUWhiteSpace
@ cdecl u_isWhitespace(long) icu.u_isWhitespace
@ cdecl u_isalnum(long) icu.u_isalnum
@ cdecl u_isalpha(long) icu.u_isalpha
@ cdecl u_isbase(long) icu.u_isbase
@ cdecl u_isblank(long) icu.u_isblank
@ cdecl u_iscntrl(long) icu.u_iscntrl
@ cdecl u_isdefined(long) icu.u_isdefined
@ cdecl u_isdigit(long) icu.u_isdigit
@ cdecl u_isgraph(long) icu.u_isgraph
@ cdecl u_islower(long) icu.u_islower
@ cdecl u_isprint(long) icu.u_isprint
@ cdecl u_ispunct(long) icu.u_ispunct
@ cdecl u_isspace(long) icu.u_isspace
@ cdecl u_istitle(long) icu.u_istitle
@ cdecl u_isupper(long) icu.u_isupper
@ cdecl u_isxdigit(long) icu.u_isxdigit
@ cdecl u_memcasecmp(ptr ptr long long) icu.u_memcasecmp
@ cdecl u_memchr(ptr long long) icu.u_memchr
@ cdecl u_memchr32(ptr long long) icu.u_memchr32
@ cdecl u_memcmp(ptr ptr long) icu.u_memcmp
@ cdecl u_memcmpCodePointOrder(ptr ptr long) icu.u_memcmpCodePointOrder
@ cdecl u_memcpy(ptr ptr long) icu.u_memcpy
@ cdecl u_memmove(ptr ptr long) icu.u_memmove
@ cdecl u_memrchr(ptr long long) icu.u_memrchr
@ cdecl u_memrchr32(ptr long long) icu.u_memrchr32
@ cdecl u_memset(ptr long long) icu.u_memset
@ cdecl u_setMemoryFunctions(ptr ptr ptr ptr ptr) icu.u_setMemoryFunctions
@ cdecl u_shapeArabic(ptr long ptr long long ptr) icu.u_shapeArabic
@ cdecl u_strCaseCompare(ptr long ptr long long ptr) icu.u_strCaseCompare
@ cdecl u_strCompare(ptr long ptr long long) icu.u_strCompare
@ cdecl u_strCompareIter(ptr ptr long) icu.u_strCompareIter
@ cdecl u_strFindFirst(ptr long ptr long) icu.u_strFindFirst
@ cdecl u_strFindLast(ptr long ptr long) icu.u_strFindLast
@ cdecl u_strFoldCase(ptr long ptr long long ptr) icu.u_strFoldCase
@ cdecl u_strFromJavaModifiedUTF8WithSub(ptr long ptr str long long ptr ptr) icu.u_strFromJavaModifiedUTF8WithSub
@ cdecl u_strFromUTF32(ptr long ptr ptr long ptr) icu.u_strFromUTF32
@ cdecl u_strFromUTF32WithSub(ptr long ptr ptr long long ptr ptr) icu.u_strFromUTF32WithSub
@ cdecl u_strFromUTF8(ptr long ptr str long ptr) icu.u_strFromUTF8
@ cdecl u_strFromUTF8Lenient(ptr long ptr str long ptr) icu.u_strFromUTF8Lenient
@ cdecl u_strFromUTF8WithSub(ptr long ptr str long long ptr ptr) icu.u_strFromUTF8WithSub
@ cdecl u_strFromWCS(ptr long ptr wstr long ptr) icu.u_strFromWCS
@ cdecl u_strHasMoreChar32Than(ptr long long) icu.u_strHasMoreChar32Than
@ cdecl u_strToJavaModifiedUTF8(str long ptr ptr long ptr) icu.u_strToJavaModifiedUTF8
@ cdecl u_strToLower(ptr long ptr long str ptr) icu.u_strToLower
@ cdecl u_strToTitle(ptr long ptr long ptr str ptr) icu.u_strToTitle
@ cdecl u_strToUTF32(ptr long ptr ptr long ptr) icu.u_strToUTF32
@ cdecl u_strToUTF32WithSub(ptr long ptr ptr long long ptr ptr) icu.u_strToUTF32WithSub
@ cdecl u_strToUTF8(str long ptr ptr long ptr) icu.u_strToUTF8
@ cdecl u_strToUTF8WithSub(str long ptr ptr long long ptr ptr) icu.u_strToUTF8WithSub
@ cdecl u_strToUpper(ptr long ptr long str ptr) icu.u_strToUpper
@ cdecl u_strToWCS(wstr long ptr ptr long ptr) icu.u_strToWCS
@ cdecl u_strcasecmp(ptr ptr long) icu.u_strcasecmp
@ cdecl u_strcat(ptr ptr) icu.u_strcat
@ cdecl u_strchr(ptr long) icu.u_strchr
@ cdecl u_strchr32(ptr long) icu.u_strchr32
@ cdecl u_strcmp(ptr ptr) icu.u_strcmp
@ cdecl u_strcmpCodePointOrder(ptr ptr) icu.u_strcmpCodePointOrder
@ cdecl u_strcpy(ptr ptr) icu.u_strcpy
@ cdecl u_strcspn(ptr ptr) icu.u_strcspn
@ cdecl u_strlen(ptr) icu.u_strlen
@ cdecl u_strncasecmp(ptr ptr long long) icu.u_strncasecmp
@ cdecl u_strncat(ptr ptr long) icu.u_strncat
@ cdecl u_strncmp(ptr ptr long) icu.u_strncmp
@ cdecl u_strncmpCodePointOrder(ptr ptr long) icu.u_strncmpCodePointOrder
@ cdecl u_strncpy(ptr ptr long) icu.u_strncpy
@ cdecl u_strpbrk(ptr ptr) icu.u_strpbrk
@ cdecl u_strrchr(ptr long) icu.u_strrchr
@ cdecl u_strrchr32(ptr long) icu.u_strrchr32
@ cdecl u_strrstr(ptr ptr) icu.u_strrstr
@ cdecl u_strspn(ptr ptr) icu.u_strspn
@ cdecl u_strstr(ptr ptr) icu.u_strstr
@ cdecl u_strtok_r(ptr ptr ptr) icu.u_strtok_r
@ cdecl u_tolower(long) icu.u_tolower
@ cdecl u_totitle(long) icu.u_totitle
@ cdecl u_toupper(long) icu.u_toupper
@ cdecl u_uastrcpy(ptr str) icu.u_uastrcpy
@ cdecl u_uastrncpy(ptr str long) icu.u_uastrncpy
@ cdecl u_unescape(str ptr long) icu.u_unescape
@ cdecl u_unescapeAt(long ptr long ptr) icu.u_unescapeAt
@ cdecl u_versionFromString(long str) icu.u_versionFromString
@ cdecl u_versionFromUString(long ptr) icu.u_versionFromUString
@ cdecl u_versionToString(long str) icu.u_versionToString
@ cdecl ubidi_close(ptr) icu.ubidi_close
@ cdecl ubidi_countParagraphs(ptr) icu.ubidi_countParagraphs
@ cdecl ubidi_countRuns(ptr ptr) icu.ubidi_countRuns
@ cdecl ubidi_getBaseDirection(ptr long) icu.ubidi_getBaseDirection
@ cdecl ubidi_getClassCallback(ptr ptr ptr) icu.ubidi_getClassCallback
@ cdecl ubidi_getCustomizedClass(ptr long) icu.ubidi_getCustomizedClass
@ cdecl ubidi_getDirection(ptr) icu.ubidi_getDirection
@ cdecl ubidi_getLength(ptr) icu.ubidi_getLength
@ cdecl ubidi_getLevelAt(ptr long) icu.ubidi_getLevelAt
@ cdecl ubidi_getLevels(ptr ptr) icu.ubidi_getLevels
@ cdecl ubidi_getLogicalIndex(ptr long ptr) icu.ubidi_getLogicalIndex
@ cdecl ubidi_getLogicalMap(ptr ptr ptr) icu.ubidi_getLogicalMap
@ cdecl ubidi_getLogicalRun(ptr long ptr ptr) icu.ubidi_getLogicalRun
@ cdecl ubidi_getParaLevel(ptr) icu.ubidi_getParaLevel
@ cdecl ubidi_getParagraph(ptr long ptr ptr ptr ptr) icu.ubidi_getParagraph
@ cdecl ubidi_getParagraphByIndex(ptr long ptr ptr ptr ptr) icu.ubidi_getParagraphByIndex
@ cdecl ubidi_getProcessedLength(ptr) icu.ubidi_getProcessedLength
@ cdecl ubidi_getReorderingMode(ptr) icu.ubidi_getReorderingMode
@ cdecl ubidi_getReorderingOptions(ptr) icu.ubidi_getReorderingOptions
@ cdecl ubidi_getResultLength(ptr) icu.ubidi_getResultLength
@ cdecl ubidi_getText(ptr) icu.ubidi_getText
@ cdecl ubidi_getVisualIndex(ptr long ptr) icu.ubidi_getVisualIndex
@ cdecl ubidi_getVisualMap(ptr ptr ptr) icu.ubidi_getVisualMap
@ cdecl ubidi_getVisualRun(ptr long ptr ptr) icu.ubidi_getVisualRun
@ cdecl ubidi_invertMap(ptr ptr long) icu.ubidi_invertMap
@ cdecl ubidi_isInverse(ptr) icu.ubidi_isInverse
@ cdecl ubidi_isOrderParagraphsLTR(ptr) icu.ubidi_isOrderParagraphsLTR
@ cdecl ubidi_open() icu.ubidi_open
@ cdecl ubidi_openSized(long long ptr) icu.ubidi_openSized
@ cdecl ubidi_orderParagraphsLTR(ptr long) icu.ubidi_orderParagraphsLTR
@ cdecl ubidi_reorderLogical(ptr long ptr) icu.ubidi_reorderLogical
@ cdecl ubidi_reorderVisual(ptr long ptr) icu.ubidi_reorderVisual
@ cdecl ubidi_setClassCallback(ptr ptr ptr ptr ptr ptr) icu.ubidi_setClassCallback
@ cdecl ubidi_setContext(ptr ptr long ptr long ptr) icu.ubidi_setContext
@ cdecl ubidi_setInverse(ptr long) icu.ubidi_setInverse
@ cdecl ubidi_setLine(ptr long long ptr ptr) icu.ubidi_setLine
@ cdecl ubidi_setPara(ptr ptr long long ptr ptr) icu.ubidi_setPara
@ cdecl ubidi_setReorderingMode(ptr long) icu.ubidi_setReorderingMode
@ cdecl ubidi_setReorderingOptions(ptr long) icu.ubidi_setReorderingOptions
@ cdecl ubidi_writeReordered(ptr ptr long long ptr) icu.ubidi_writeReordered
@ cdecl ubidi_writeReverse(ptr long ptr long long ptr) icu.ubidi_writeReverse
@ cdecl ubiditransform_close(ptr) icu.ubiditransform_close
@ cdecl ubiditransform_open(ptr) icu.ubiditransform_open
@ cdecl ubiditransform_transform(ptr ptr long ptr long long long long long long long ptr) icu.ubiditransform_transform
@ cdecl ublock_getCode(long) icu.ublock_getCode
@ cdecl ubrk_close(ptr) icu.ubrk_close
@ cdecl ubrk_countAvailable() icu.ubrk_countAvailable
@ cdecl ubrk_current(ptr) icu.ubrk_current
@ cdecl ubrk_first(ptr) icu.ubrk_first
@ cdecl ubrk_following(ptr long) icu.ubrk_following
@ cdecl ubrk_getAvailable(long) icu.ubrk_getAvailable
@ cdecl ubrk_getBinaryRules(ptr ptr long ptr) icu.ubrk_getBinaryRules
@ cdecl ubrk_getLocaleByType(ptr long ptr) icu.ubrk_getLocaleByType
@ cdecl ubrk_getRuleStatus(ptr) icu.ubrk_getRuleStatus
@ cdecl ubrk_getRuleStatusVec(ptr ptr long ptr) icu.ubrk_getRuleStatusVec
@ cdecl ubrk_isBoundary(ptr long) icu.ubrk_isBoundary
@ cdecl ubrk_last(ptr) icu.ubrk_last
@ cdecl ubrk_next(ptr) icu.ubrk_next
@ cdecl ubrk_open(long str ptr long ptr) icu.ubrk_open
@ cdecl ubrk_openBinaryRules(ptr long ptr long ptr) icu.ubrk_openBinaryRules
@ cdecl ubrk_openRules(ptr long ptr long ptr ptr) icu.ubrk_openRules
@ cdecl ubrk_preceding(ptr long) icu.ubrk_preceding
@ cdecl ubrk_previous(ptr) icu.ubrk_previous
@ cdecl ubrk_refreshUText(ptr ptr ptr) icu.ubrk_refreshUText
@ cdecl ubrk_safeClone(ptr ptr ptr ptr) icu.ubrk_safeClone
@ cdecl ubrk_setText(ptr ptr long ptr) icu.ubrk_setText
@ cdecl ubrk_setUText(ptr ptr ptr) icu.ubrk_setUText
@ cdecl ucasemap_close(ptr) icu.ucasemap_close
@ cdecl ucasemap_getBreakIterator(ptr) icu.ucasemap_getBreakIterator
@ cdecl ucasemap_getLocale(ptr) icu.ucasemap_getLocale
@ cdecl ucasemap_getOptions(ptr) icu.ucasemap_getOptions
@ cdecl ucasemap_open(str long ptr) icu.ucasemap_open
@ cdecl ucasemap_setBreakIterator(ptr ptr ptr) icu.ucasemap_setBreakIterator
@ cdecl ucasemap_setLocale(ptr str ptr) icu.ucasemap_setLocale
@ cdecl ucasemap_setOptions(ptr long ptr) icu.ucasemap_setOptions
@ cdecl ucasemap_toTitle(ptr ptr long ptr long ptr) icu.ucasemap_toTitle
@ cdecl ucasemap_utf8FoldCase(ptr str long str long ptr) icu.ucasemap_utf8FoldCase
@ cdecl ucasemap_utf8ToLower(ptr str long str long ptr) icu.ucasemap_utf8ToLower
@ cdecl ucasemap_utf8ToTitle(ptr str long str long ptr) icu.ucasemap_utf8ToTitle
@ cdecl ucasemap_utf8ToUpper(ptr str long str long ptr) icu.ucasemap_utf8ToUpper
@ cdecl ucnv_cbFromUWriteBytes(ptr str long long ptr) icu.ucnv_cbFromUWriteBytes
@ cdecl ucnv_cbFromUWriteSub(ptr long ptr) icu.ucnv_cbFromUWriteSub
@ cdecl ucnv_cbFromUWriteUChars(ptr ptr ptr long ptr) icu.ucnv_cbFromUWriteUChars
@ cdecl ucnv_cbToUWriteSub(ptr long ptr) icu.ucnv_cbToUWriteSub
@ cdecl ucnv_cbToUWriteUChars(ptr ptr long long ptr) icu.ucnv_cbToUWriteUChars
@ cdecl ucnv_close(ptr) icu.ucnv_close
@ cdecl ucnv_compareNames(str str) icu.ucnv_compareNames
@ cdecl ucnv_convert(str str str long str long ptr) icu.ucnv_convert
@ cdecl ucnv_convertEx(ptr ptr ptr str ptr str ptr ptr ptr ptr long long ptr) icu.ucnv_convertEx
@ cdecl ucnv_countAliases(str ptr) icu.ucnv_countAliases
@ cdecl ucnv_countAvailable() icu.ucnv_countAvailable
@ cdecl ucnv_countStandards() icu.ucnv_countStandards
@ cdecl ucnv_detectUnicodeSignature(str long ptr ptr) icu.ucnv_detectUnicodeSignature
@ cdecl ucnv_fixFileSeparator(ptr ptr long) icu.ucnv_fixFileSeparator
@ cdecl ucnv_flushCache() icu.ucnv_flushCache
@ cdecl ucnv_fromAlgorithmic(ptr long str long str long ptr) icu.ucnv_fromAlgorithmic
@ cdecl ucnv_fromUChars(ptr str long ptr long ptr) icu.ucnv_fromUChars
@ cdecl ucnv_fromUCountPending(ptr ptr) icu.ucnv_fromUCountPending
@ cdecl ucnv_fromUnicode(ptr ptr str ptr ptr ptr long ptr) icu.ucnv_fromUnicode
@ cdecl ucnv_getAlias(str long ptr) icu.ucnv_getAlias
@ cdecl ucnv_getAliases(str ptr ptr) icu.ucnv_getAliases
@ cdecl ucnv_getAvailableName(long) icu.ucnv_getAvailableName
@ cdecl ucnv_getCCSID(ptr ptr) icu.ucnv_getCCSID
@ cdecl ucnv_getCanonicalName(str str ptr) icu.ucnv_getCanonicalName
@ cdecl ucnv_getDefaultName() icu.ucnv_getDefaultName
@ cdecl ucnv_getDisplayName(ptr str ptr long ptr) icu.ucnv_getDisplayName
@ cdecl ucnv_getFromUCallBack(ptr ptr ptr) icu.ucnv_getFromUCallBack
@ cdecl ucnv_getInvalidChars(ptr str ptr ptr) icu.ucnv_getInvalidChars
@ cdecl ucnv_getInvalidUChars(ptr ptr ptr ptr) icu.ucnv_getInvalidUChars
@ cdecl ucnv_getMaxCharSize(ptr) icu.ucnv_getMaxCharSize
@ cdecl ucnv_getMinCharSize(ptr) icu.ucnv_getMinCharSize
@ cdecl ucnv_getName(ptr ptr) icu.ucnv_getName
@ cdecl ucnv_getNextUChar(ptr ptr str ptr) icu.ucnv_getNextUChar
@ cdecl ucnv_getPlatform(ptr ptr) icu.ucnv_getPlatform
@ cdecl ucnv_getStandard(long ptr) icu.ucnv_getStandard
@ cdecl ucnv_getStandardName(str str ptr) icu.ucnv_getStandardName
@ cdecl ucnv_getStarters(ptr long ptr) icu.ucnv_getStarters
@ cdecl ucnv_getSubstChars(ptr str ptr ptr) icu.ucnv_getSubstChars
@ cdecl ucnv_getToUCallBack(ptr ptr ptr) icu.ucnv_getToUCallBack
@ cdecl ucnv_getType(ptr) icu.ucnv_getType
@ cdecl ucnv_getUnicodeSet(ptr ptr long ptr) icu.ucnv_getUnicodeSet
@ cdecl ucnv_isAmbiguous(ptr) icu.ucnv_isAmbiguous
@ cdecl ucnv_isFixedWidth(ptr ptr) icu.ucnv_isFixedWidth
@ cdecl ucnv_open(str ptr) icu.ucnv_open
@ cdecl ucnv_openAllNames(ptr) icu.ucnv_openAllNames
@ cdecl ucnv_openCCSID(long long ptr) icu.ucnv_openCCSID
@ cdecl ucnv_openPackage(str str ptr) icu.ucnv_openPackage
@ cdecl ucnv_openStandardNames(str str ptr) icu.ucnv_openStandardNames
@ cdecl ucnv_openU(ptr ptr) icu.ucnv_openU
@ cdecl ucnv_reset(ptr) icu.ucnv_reset
@ cdecl ucnv_resetFromUnicode(ptr) icu.ucnv_resetFromUnicode
@ cdecl ucnv_resetToUnicode(ptr) icu.ucnv_resetToUnicode
@ cdecl ucnv_safeClone(ptr ptr ptr ptr) icu.ucnv_safeClone
@ cdecl ucnv_setDefaultName(str) icu.ucnv_setDefaultName
@ cdecl ucnv_setFallback(ptr long) icu.ucnv_setFallback
@ cdecl ucnv_setFromUCallBack(ptr long ptr ptr ptr ptr) icu.ucnv_setFromUCallBack
@ cdecl ucnv_setSubstChars(ptr str long ptr) icu.ucnv_setSubstChars
@ cdecl ucnv_setSubstString(ptr ptr long ptr) icu.ucnv_setSubstString
@ cdecl ucnv_setToUCallBack(ptr long ptr ptr ptr ptr) icu.ucnv_setToUCallBack
@ cdecl ucnv_toAlgorithmic(long ptr str long str long ptr) icu.ucnv_toAlgorithmic
@ cdecl ucnv_toUChars(ptr ptr long str long ptr) icu.ucnv_toUChars
@ cdecl ucnv_toUCountPending(ptr ptr) icu.ucnv_toUCountPending
@ cdecl ucnv_toUnicode(ptr ptr ptr ptr str ptr long ptr) icu.ucnv_toUnicode
@ cdecl ucnv_usesFallback(ptr) icu.ucnv_usesFallback
@ cdecl ucnvsel_close(ptr) icu.ucnvsel_close
@ cdecl ucnvsel_open(str long ptr long ptr) icu.ucnvsel_open
@ cdecl ucnvsel_openFromSerialized(ptr long ptr) icu.ucnvsel_openFromSerialized
@ cdecl ucnvsel_selectForString(ptr ptr long ptr) icu.ucnvsel_selectForString
@ cdecl ucnvsel_selectForUTF8(ptr str long ptr) icu.ucnvsel_selectForUTF8
@ cdecl ucnvsel_serialize(ptr ptr long ptr) icu.ucnvsel_serialize
@ cdecl ucurr_countCurrencies(str long ptr) icu.ucurr_countCurrencies
@ cdecl ucurr_forLocale(str ptr long ptr) icu.ucurr_forLocale
@ cdecl ucurr_forLocaleAndDate(str long long ptr long ptr) icu.ucurr_forLocaleAndDate
@ cdecl ucurr_getDefaultFractionDigits(ptr ptr) icu.ucurr_getDefaultFractionDigits
@ cdecl ucurr_getDefaultFractionDigitsForUsage(ptr long ptr) icu.ucurr_getDefaultFractionDigitsForUsage
@ cdecl ucurr_getKeywordValuesForLocale(str str long ptr) icu.ucurr_getKeywordValuesForLocale
@ cdecl ucurr_getName(ptr str long ptr ptr ptr) icu.ucurr_getName
@ cdecl ucurr_getNumericCode(ptr) icu.ucurr_getNumericCode
@ cdecl ucurr_getPluralName(ptr str ptr str ptr ptr) icu.ucurr_getPluralName
@ cdecl ucurr_getRoundingIncrement(ptr ptr) icu.ucurr_getRoundingIncrement
@ cdecl ucurr_getRoundingIncrementForUsage(ptr long ptr) icu.ucurr_getRoundingIncrementForUsage
@ cdecl ucurr_isAvailable(ptr long long ptr) icu.ucurr_isAvailable
@ cdecl ucurr_openISOCurrencies(long ptr) icu.ucurr_openISOCurrencies
@ cdecl ucurr_register(ptr str ptr) icu.ucurr_register
@ cdecl ucurr_unregister(long ptr) icu.ucurr_unregister
@ cdecl uenum_close(ptr) icu.uenum_close
@ cdecl uenum_count(ptr ptr) icu.uenum_count
@ cdecl uenum_next(ptr ptr ptr) icu.uenum_next
@ cdecl uenum_openCharStringsEnumeration(str long ptr) icu.uenum_openCharStringsEnumeration
@ cdecl uenum_openUCharStringsEnumeration(ptr long ptr) icu.uenum_openUCharStringsEnumeration
@ cdecl uenum_reset(ptr ptr) icu.uenum_reset
@ cdecl uenum_unext(ptr ptr ptr) icu.uenum_unext
@ cdecl uidna_close(ptr) icu.uidna_close
@ cdecl uidna_labelToASCII(ptr ptr long ptr long ptr ptr) icu.uidna_labelToASCII
@ cdecl uidna_labelToASCII_UTF8(ptr str long str long ptr ptr) icu.uidna_labelToASCII_UTF8
@ cdecl uidna_labelToUnicode(ptr ptr long ptr long ptr ptr) icu.uidna_labelToUnicode
@ cdecl uidna_labelToUnicodeUTF8(ptr str long str long ptr ptr) icu.uidna_labelToUnicodeUTF8
@ cdecl uidna_nameToASCII(ptr ptr long ptr long ptr ptr) icu.uidna_nameToASCII
@ cdecl uidna_nameToASCII_UTF8(ptr str long str long ptr ptr) icu.uidna_nameToASCII_UTF8
@ cdecl uidna_nameToUnicode(ptr ptr long ptr long ptr ptr) icu.uidna_nameToUnicode
@ cdecl uidna_nameToUnicodeUTF8(ptr str long str long ptr ptr) icu.uidna_nameToUnicodeUTF8
@ cdecl uidna_openUTS46(long ptr) icu.uidna_openUTS46
@ cdecl uiter_current32(ptr) icu.uiter_current32
@ cdecl uiter_getState(ptr) icu.uiter_getState
@ cdecl uiter_next32(ptr) icu.uiter_next32
@ cdecl uiter_previous32(ptr) icu.uiter_previous32
@ cdecl uiter_setState(ptr long ptr) icu.uiter_setState
@ cdecl uiter_setString(ptr ptr long) icu.uiter_setString
@ cdecl uiter_setUTF16BE(ptr str long) icu.uiter_setUTF16BE
@ cdecl uiter_setUTF8(ptr str long) icu.uiter_setUTF8
@ cdecl uldn_close(ptr) icu.uldn_close
@ cdecl uldn_getContext(ptr long ptr) icu.uldn_getContext
@ cdecl uldn_getDialectHandling(ptr) icu.uldn_getDialectHandling
@ cdecl uldn_getLocale(ptr) icu.uldn_getLocale
@ cdecl uldn_keyDisplayName(ptr str ptr long ptr) icu.uldn_keyDisplayName
@ cdecl uldn_keyValueDisplayName(ptr str str ptr long ptr) icu.uldn_keyValueDisplayName
@ cdecl uldn_languageDisplayName(ptr str ptr long ptr) icu.uldn_languageDisplayName
@ cdecl uldn_localeDisplayName(ptr str ptr long ptr) icu.uldn_localeDisplayName
@ cdecl uldn_open(str long ptr) icu.uldn_open
@ cdecl uldn_openForContext(str ptr long ptr) icu.uldn_openForContext
@ cdecl uldn_regionDisplayName(ptr str ptr long ptr) icu.uldn_regionDisplayName
@ cdecl uldn_scriptCodeDisplayName(ptr long ptr long ptr) icu.uldn_scriptCodeDisplayName
@ cdecl uldn_scriptDisplayName(ptr str ptr long ptr) icu.uldn_scriptDisplayName
@ cdecl uldn_variantDisplayName(ptr str ptr long ptr) icu.uldn_variantDisplayName
@ cdecl ulistfmt_close(ptr) icu.ulistfmt_close
@ cdecl ulistfmt_format(ptr ptr ptr long ptr long ptr) icu.ulistfmt_format
@ cdecl ulistfmt_open(str ptr) icu.ulistfmt_open
@ cdecl uloc_acceptLanguage(str long ptr ptr long ptr ptr) icu.uloc_acceptLanguage
@ cdecl uloc_acceptLanguageFromHTTP(str long ptr str ptr ptr) icu.uloc_acceptLanguageFromHTTP
@ cdecl uloc_addLikelySubtags(str str long ptr) icu.uloc_addLikelySubtags
@ cdecl uloc_canonicalize(str str long ptr) icu.uloc_canonicalize
@ cdecl uloc_countAvailable() icu.uloc_countAvailable
@ cdecl uloc_forLanguageTag(str str long ptr ptr) icu.uloc_forLanguageTag
@ cdecl uloc_getAvailable(long) icu.uloc_getAvailable
@ cdecl uloc_getBaseName(str str long ptr) icu.uloc_getBaseName
@ cdecl uloc_getCharacterOrientation(str ptr) icu.uloc_getCharacterOrientation
@ cdecl uloc_getCountry(str str long ptr) icu.uloc_getCountry
@ cdecl uloc_getDefault() icu.uloc_getDefault
@ cdecl uloc_getDisplayCountry(str str ptr long ptr) icu.uloc_getDisplayCountry
@ cdecl uloc_getDisplayKeyword(str str ptr long ptr) icu.uloc_getDisplayKeyword
@ cdecl uloc_getDisplayKeywordValue(str str str ptr long ptr) icu.uloc_getDisplayKeywordValue
@ cdecl uloc_getDisplayLanguage(str str ptr long ptr) icu.uloc_getDisplayLanguage
@ cdecl uloc_getDisplayName(str str ptr long ptr) icu.uloc_getDisplayName
@ cdecl uloc_getDisplayScript(str str ptr long ptr) icu.uloc_getDisplayScript
@ cdecl uloc_getDisplayVariant(str str ptr long ptr) icu.uloc_getDisplayVariant
@ cdecl uloc_getISO3Country(str) icu.uloc_getISO3Country
@ cdecl uloc_getISO3Language(str) icu.uloc_getISO3Language
@ cdecl uloc_getISOCountries() icu.uloc_getISOCountries
@ cdecl uloc_getISOLanguages() icu.uloc_getISOLanguages
@ cdecl uloc_getKeywordValue(str str str long ptr) icu.uloc_getKeywordValue
@ cdecl uloc_getLCID(str) icu.uloc_getLCID
@ cdecl uloc_getLanguage(str str long ptr) icu.uloc_getLanguage
@ cdecl uloc_getLineOrientation(str ptr) icu.uloc_getLineOrientation
@ cdecl uloc_getLocaleForLCID(long str long ptr) icu.uloc_getLocaleForLCID
@ cdecl uloc_getName(str str long ptr) icu.uloc_getName
@ cdecl uloc_getParent(str str long ptr) icu.uloc_getParent
@ cdecl uloc_getScript(str str long ptr) icu.uloc_getScript
@ cdecl uloc_getVariant(str str long ptr) icu.uloc_getVariant
@ cdecl uloc_isRightToLeft(str) icu.uloc_isRightToLeft
@ cdecl uloc_minimizeSubtags(str str long ptr) icu.uloc_minimizeSubtags
@ cdecl uloc_openKeywords(str ptr) icu.uloc_openKeywords
@ cdecl uloc_setDefault(str ptr) icu.uloc_setDefault
@ cdecl uloc_setKeywordValue(str str str long ptr) icu.uloc_setKeywordValue
@ cdecl uloc_toLanguageTag(str str long long ptr) icu.uloc_toLanguageTag
@ cdecl uloc_toLegacyKey(str) icu.uloc_toLegacyKey
@ cdecl uloc_toLegacyType(str str) icu.uloc_toLegacyType
@ cdecl uloc_toUnicodeLocaleKey(str) icu.uloc_toUnicodeLocaleKey
@ cdecl uloc_toUnicodeLocaleType(str str) icu.uloc_toUnicodeLocaleType
@ cdecl unorm2_append(ptr ptr long long ptr long ptr) icu.unorm2_append
@ cdecl unorm2_close(ptr) icu.unorm2_close
@ cdecl unorm2_composePair(ptr long long) icu.unorm2_composePair
@ cdecl unorm2_getCombiningClass(ptr long) icu.unorm2_getCombiningClass
@ cdecl unorm2_getDecomposition(ptr long ptr long ptr) icu.unorm2_getDecomposition
@ cdecl unorm2_getInstance(str str long ptr) icu.unorm2_getInstance
@ cdecl unorm2_getNFCInstance(ptr) icu.unorm2_getNFCInstance
@ cdecl unorm2_getNFDInstance(ptr) icu.unorm2_getNFDInstance
@ cdecl unorm2_getNFKCCasefoldInstance(ptr) icu.unorm2_getNFKCCasefoldInstance
@ cdecl unorm2_getNFKCInstance(ptr) icu.unorm2_getNFKCInstance
@ cdecl unorm2_getNFKDInstance(ptr) icu.unorm2_getNFKDInstance
@ cdecl unorm2_getRawDecomposition(ptr long ptr long ptr) icu.unorm2_getRawDecomposition
@ cdecl unorm2_hasBoundaryAfter(ptr long) icu.unorm2_hasBoundaryAfter
@ cdecl unorm2_hasBoundaryBefore(ptr long) icu.unorm2_hasBoundaryBefore
@ cdecl unorm2_isInert(ptr long) icu.unorm2_isInert
@ cdecl unorm2_isNormalized(ptr ptr long ptr) icu.unorm2_isNormalized
@ cdecl unorm2_normalize(ptr ptr long ptr long ptr) icu.unorm2_normalize
@ cdecl unorm2_normalizeSecondAndAppend(ptr ptr long long ptr long ptr) icu.unorm2_normalizeSecondAndAppend
@ cdecl unorm2_openFiltered(ptr ptr ptr) icu.unorm2_openFiltered
@ cdecl unorm2_quickCheck(ptr ptr long ptr) icu.unorm2_quickCheck
@ cdecl unorm2_spanQuickCheckYes(ptr ptr long ptr) icu.unorm2_spanQuickCheckYes
@ cdecl unorm_compare(ptr long ptr long long ptr) icu.unorm_compare
@ cdecl ures_close(ptr) icu.ures_close
@ cdecl ures_getBinary(ptr ptr ptr) icu.ures_getBinary
@ cdecl ures_getByIndex(ptr long ptr ptr) icu.ures_getByIndex
@ cdecl ures_getByKey(ptr str ptr ptr) icu.ures_getByKey
@ cdecl ures_getInt(ptr ptr) icu.ures_getInt
@ cdecl ures_getIntVector(ptr ptr ptr) icu.ures_getIntVector
@ cdecl ures_getKey(ptr) icu.ures_getKey
@ cdecl ures_getLocaleByType(ptr long ptr) icu.ures_getLocaleByType
@ cdecl ures_getNextResource(ptr ptr ptr) icu.ures_getNextResource
@ cdecl ures_getNextString(ptr ptr ptr ptr) icu.ures_getNextString
@ cdecl ures_getSize(ptr) icu.ures_getSize
@ cdecl ures_getString(ptr ptr ptr) icu.ures_getString
@ cdecl ures_getStringByIndex(ptr long ptr ptr) icu.ures_getStringByIndex
@ cdecl ures_getStringByKey(ptr str ptr ptr) icu.ures_getStringByKey
@ cdecl ures_getType(ptr) icu.ures_getType
@ cdecl ures_getUInt(ptr ptr) icu.ures_getUInt
@ cdecl ures_getUTF8String(ptr str ptr long ptr) icu.ures_getUTF8String
@ cdecl ures_getUTF8StringByIndex(ptr long str ptr long ptr) icu.ures_getUTF8StringByIndex
@ cdecl ures_getUTF8StringByKey(ptr str str ptr long ptr) icu.ures_getUTF8StringByKey
@ cdecl ures_getVersion(ptr long) icu.ures_getVersion
@ cdecl ures_hasNext(ptr) icu.ures_hasNext
@ cdecl ures_open(str str ptr) icu.ures_open
@ cdecl ures_openAvailableLocales(str ptr) icu.ures_openAvailableLocales
@ cdecl ures_openDirect(str str ptr) icu.ures_openDirect
@ cdecl ures_openU(ptr str ptr) icu.ures_openU
@ cdecl ures_resetIterator(ptr) icu.ures_resetIterator
@ cdecl uscript_breaksBetweenLetters(long) icu.uscript_breaksBetweenLetters
@ cdecl uscript_getCode(str ptr long ptr) icu.uscript_getCode
@ cdecl uscript_getName(long) icu.uscript_getName
@ cdecl uscript_getSampleString(long ptr long ptr) icu.uscript_getSampleString
@ cdecl uscript_getScript(long ptr) icu.uscript_getScript
@ cdecl uscript_getScriptExtensions(long ptr long ptr) icu.uscript_getScriptExtensions
@ cdecl uscript_getShortName(long) icu.uscript_getShortName
@ cdecl uscript_getUsage(long) icu.uscript_getUsage
@ cdecl uscript_hasScript(long long) icu.uscript_hasScript
@ cdecl uscript_isCased(long) icu.uscript_isCased
@ cdecl uscript_isRightToLeft(long) icu.uscript_isRightToLeft
@ cdecl uset_add(ptr long) icu.uset_add
@ cdecl uset_addAll(ptr ptr) icu.uset_addAll
@ cdecl uset_addAllCodePoints(ptr ptr long) icu.uset_addAllCodePoints
@ cdecl uset_addRange(ptr long long) icu.uset_addRange
@ cdecl uset_addString(ptr ptr long) icu.uset_addString
@ cdecl uset_applyIntPropertyValue(ptr long long ptr) icu.uset_applyIntPropertyValue
@ cdecl uset_applyPattern(ptr ptr long long ptr) icu.uset_applyPattern
@ cdecl uset_applyPropertyAlias(ptr ptr long ptr long ptr) icu.uset_applyPropertyAlias
@ cdecl uset_charAt(ptr long) icu.uset_charAt
@ cdecl uset_clear(ptr) icu.uset_clear
@ cdecl uset_clone(ptr) icu.uset_clone
@ cdecl uset_cloneAsThawed(ptr) icu.uset_cloneAsThawed
@ cdecl uset_close(ptr) icu.uset_close
@ cdecl uset_closeOver(ptr long) icu.uset_closeOver
@ cdecl uset_compact(ptr) icu.uset_compact
@ cdecl uset_complement(ptr) icu.uset_complement
@ cdecl uset_complementAll(ptr ptr) icu.uset_complementAll
@ cdecl uset_contains(ptr long) icu.uset_contains
@ cdecl uset_containsAll(ptr ptr) icu.uset_containsAll
@ cdecl uset_containsAllCodePoints(ptr ptr long) icu.uset_containsAllCodePoints
@ cdecl uset_containsNone(ptr ptr) icu.uset_containsNone
@ cdecl uset_containsRange(ptr long long) icu.uset_containsRange
@ cdecl uset_containsSome(ptr ptr) icu.uset_containsSome
@ cdecl uset_containsString(ptr ptr long) icu.uset_containsString
@ cdecl uset_equals(ptr ptr) icu.uset_equals
@ cdecl uset_freeze(ptr) icu.uset_freeze
@ cdecl uset_getItem(ptr long ptr ptr ptr long ptr) icu.uset_getItem
@ cdecl uset_getItemCount(ptr) icu.uset_getItemCount
@ cdecl uset_getSerializedRange(ptr long ptr ptr) icu.uset_getSerializedRange
@ cdecl uset_getSerializedRangeCount(ptr) icu.uset_getSerializedRangeCount
@ cdecl uset_getSerializedSet(ptr ptr long) icu.uset_getSerializedSet
@ cdecl uset_indexOf(ptr long) icu.uset_indexOf
@ cdecl uset_isEmpty(ptr) icu.uset_isEmpty
@ cdecl uset_isFrozen(ptr) icu.uset_isFrozen
@ cdecl uset_open(long long) icu.uset_open
@ cdecl uset_openEmpty() icu.uset_openEmpty
@ cdecl uset_openPattern(ptr long ptr) icu.uset_openPattern
@ cdecl uset_openPatternOptions(ptr long long ptr) icu.uset_openPatternOptions
@ cdecl uset_remove(ptr long) icu.uset_remove
@ cdecl uset_removeAll(ptr ptr) icu.uset_removeAll
@ cdecl uset_removeAllStrings(ptr) icu.uset_removeAllStrings
@ cdecl uset_removeRange(ptr long long) icu.uset_removeRange
@ cdecl uset_removeString(ptr ptr long) icu.uset_removeString
@ cdecl uset_resemblesPattern(ptr long long) icu.uset_resemblesPattern
@ cdecl uset_retain(ptr long long) icu.uset_retain
@ cdecl uset_retainAll(ptr ptr) icu.uset_retainAll
@ cdecl uset_serialize(ptr ptr long ptr) icu.uset_serialize
@ cdecl uset_serializedContains(ptr long) icu.uset_serializedContains
@ cdecl uset_set(ptr long long) icu.uset_set
@ cdecl uset_setSerializedToOne(ptr long) icu.uset_setSerializedToOne
@ cdecl uset_size(ptr) icu.uset_size
@ cdecl uset_span(ptr ptr long long) icu.uset_span
@ cdecl uset_spanBack(ptr ptr long long) icu.uset_spanBack
@ cdecl uset_spanBackUTF8(ptr str long long) icu.uset_spanBackUTF8
@ cdecl uset_spanUTF8(ptr str long long) icu.uset_spanUTF8
@ cdecl uset_toPattern(ptr ptr long long ptr) icu.uset_toPattern
@ cdecl usprep_close(ptr) icu.usprep_close
@ cdecl usprep_open(str str ptr) icu.usprep_open
@ cdecl usprep_openByType(long ptr) icu.usprep_openByType
@ cdecl usprep_prepare(ptr ptr long ptr long long ptr ptr) icu.usprep_prepare
@ cdecl utext_char32At(ptr long) icu.utext_char32At
@ cdecl utext_clone(ptr ptr long long ptr) icu.utext_clone
@ cdecl utext_close(ptr) icu.utext_close
@ cdecl utext_copy(ptr long long long long ptr) icu.utext_copy
@ cdecl utext_current32(ptr) icu.utext_current32
@ cdecl utext_equals(ptr ptr) icu.utext_equals
@ cdecl utext_extract(ptr long long ptr long ptr) icu.utext_extract
@ cdecl utext_freeze(ptr) icu.utext_freeze
@ cdecl utext_getNativeIndex(ptr) icu.utext_getNativeIndex
@ cdecl utext_getPreviousNativeIndex(ptr) icu.utext_getPreviousNativeIndex
@ cdecl utext_hasMetaData(ptr) icu.utext_hasMetaData
@ cdecl utext_isLengthExpensive(ptr) icu.utext_isLengthExpensive
@ cdecl utext_isWritable(ptr) icu.utext_isWritable
@ cdecl utext_moveIndex32(ptr long) icu.utext_moveIndex32
@ cdecl utext_nativeLength(ptr) icu.utext_nativeLength
@ cdecl utext_next32(ptr) icu.utext_next32
@ cdecl utext_next32From(ptr long) icu.utext_next32From
@ cdecl utext_openUChars(ptr ptr long ptr) icu.utext_openUChars
@ cdecl utext_openUTF8(ptr str long ptr) icu.utext_openUTF8
@ cdecl utext_previous32(ptr) icu.utext_previous32
@ cdecl utext_previous32From(ptr long) icu.utext_previous32From
@ cdecl utext_replace(ptr long long ptr long ptr) icu.utext_replace
@ cdecl utext_setNativeIndex(ptr long) icu.utext_setNativeIndex
@ cdecl utext_setup(ptr long ptr) icu.utext_setup
@ cdecl utf8_appendCharSafeBody(ptr long long long ptr) icu.utf8_appendCharSafeBody
@ cdecl utf8_back1SafeBody(ptr long long) icu.utf8_back1SafeBody
@ cdecl utf8_nextCharSafeBody(ptr ptr long long long) icu.utf8_nextCharSafeBody
@ cdecl utf8_prevCharSafeBody(ptr long ptr long long) icu.utf8_prevCharSafeBody
@ varargs utrace_format(str long long str) icu.utrace_format
@ cdecl utrace_functionName(long) icu.utrace_functionName
@ cdecl utrace_getFunctions(ptr ptr ptr ptr) icu.utrace_getFunctions
@ cdecl utrace_getLevel() icu.utrace_getLevel
@ cdecl utrace_setFunctions(ptr ptr ptr ptr) icu.utrace_setFunctions
@ cdecl utrace_setLevel(long) icu.utrace_setLevel
@ cdecl utrace_vformat(str long long str long) icu.utrace_vformat
