@ varargs u_formatMessage(str ptr long ptr long ptr) icu.u_formatMessage
@ varargs u_formatMessageWithError(str ptr long ptr long ptr ptr) icu.u_formatMessageWithError
@ varargs u_parseMessage(str ptr long ptr long ptr) icu.u_parseMessage
@ varargs u_parseMessageWithError(str ptr long ptr long ptr ptr) icu.u_parseMessageWithError
@ cdecl u_vformatMessage(str ptr long ptr long long ptr) icu.u_vformatMessage
@ cdecl u_vformatMessageWithError(str ptr long ptr long ptr long ptr) icu.u_vformatMessageWithError
@ cdecl u_vparseMessage(str ptr long ptr long long ptr) icu.u_vparseMessage
@ cdecl u_vparseMessageWithError(str ptr long ptr long long ptr ptr) icu.u_vparseMessageWithError
@ cdecl ucal_add(ptr long long ptr) icu.ucal_add
@ cdecl ucal_clear(ptr) icu.ucal_clear
@ cdecl ucal_clearField(ptr long) icu.ucal_clearField
@ cdecl ucal_clone(ptr ptr) icu.ucal_clone
@ cdecl ucal_close(ptr) icu.ucal_close
@ cdecl ucal_countAvailable() icu.ucal_countAvailable
@ cdecl ucal_equivalentTo(ptr ptr) icu.ucal_equivalentTo
@ cdecl ucal_get(ptr long ptr) icu.ucal_get
@ cdecl ucal_getAttribute(ptr long) icu.ucal_getAttribute
@ cdecl ucal_getAvailable(long) icu.ucal_getAvailable
@ cdecl ucal_getCanonicalTimeZoneID(ptr long ptr long ptr ptr) icu.ucal_getCanonicalTimeZoneID
@ cdecl ucal_getDSTSavings(ptr ptr) icu.ucal_getDSTSavings
@ cdecl ucal_getDayOfWeekType(ptr long ptr) icu.ucal_getDayOfWeekType
@ cdecl ucal_getDefaultTimeZone(ptr long ptr) icu.ucal_getDefaultTimeZone
@ cdecl ucal_getFieldDifference(ptr long long ptr) icu.ucal_getFieldDifference
@ cdecl ucal_getGregorianChange(ptr ptr) icu.ucal_getGregorianChange
@ cdecl ucal_getKeywordValuesForLocale(str str long ptr) icu.ucal_getKeywordValuesForLocale
@ cdecl ucal_getLimit(ptr long long ptr) icu.ucal_getLimit
@ cdecl ucal_getLocaleByType(ptr long ptr) icu.ucal_getLocaleByType
@ cdecl ucal_getMillis(ptr ptr) icu.ucal_getMillis
@ cdecl ucal_getNow() icu.ucal_getNow
@ cdecl ucal_getTZDataVersion(ptr) icu.ucal_getTZDataVersion
@ cdecl ucal_getTimeZoneDisplayName(ptr long str ptr long ptr) icu.ucal_getTimeZoneDisplayName
@ cdecl ucal_getTimeZoneID(ptr ptr long ptr) icu.ucal_getTimeZoneID
@ cdecl ucal_getTimeZoneIDForWindowsID(ptr long str ptr long ptr) icu.ucal_getTimeZoneIDForWindowsID
@ cdecl ucal_getTimeZoneTransitionDate(ptr long ptr ptr) icu.ucal_getTimeZoneTransitionDate
@ cdecl ucal_getType(ptr ptr) icu.ucal_getType
@ cdecl ucal_getWeekendTransition(ptr long ptr) icu.ucal_getWeekendTransition
@ cdecl ucal_getWindowsTimeZoneID(ptr long ptr long ptr) icu.ucal_getWindowsTimeZoneID
@ cdecl ucal_inDaylightTime(ptr ptr) icu.ucal_inDaylightTime
@ cdecl ucal_isSet(ptr long) icu.ucal_isSet
@ cdecl ucal_isWeekend(ptr long ptr) icu.ucal_isWeekend
@ cdecl ucal_open(ptr long str long ptr) icu.ucal_open
@ cdecl ucal_openCountryTimeZones(str ptr) icu.ucal_openCountryTimeZones
@ cdecl ucal_openTimeZoneIDEnumeration(long str ptr ptr) icu.ucal_openTimeZoneIDEnumeration
@ cdecl ucal_openTimeZones(ptr) icu.ucal_openTimeZones
@ cdecl ucal_roll(ptr long long ptr) icu.ucal_roll
@ cdecl ucal_set(ptr long long) icu.ucal_set
@ cdecl ucal_setAttribute(ptr long long) icu.ucal_setAttribute
@ cdecl ucal_setDate(ptr long long long ptr) icu.ucal_setDate
@ cdecl ucal_setDateTime(ptr long long long long long long ptr) icu.ucal_setDateTime
@ cdecl ucal_setDefaultTimeZone(ptr ptr) icu.ucal_setDefaultTimeZone
@ cdecl ucal_setGregorianChange(ptr long ptr) icu.ucal_setGregorianChange
@ cdecl ucal_setMillis(ptr long ptr) icu.ucal_setMillis
@ cdecl ucal_setTimeZone(ptr ptr long ptr) icu.ucal_setTimeZone
@ cdecl ucol_cloneBinary(ptr ptr long ptr) icu.ucol_cloneBinary
@ cdecl ucol_close(ptr) icu.ucol_close
@ cdecl ucol_closeElements(ptr) icu.ucol_closeElements
@ cdecl ucol_countAvailable() icu.ucol_countAvailable
@ cdecl ucol_equal(ptr ptr long ptr long) icu.ucol_equal
@ cdecl ucol_getAttribute(ptr long ptr) icu.ucol_getAttribute
@ cdecl ucol_getAvailable(long) icu.ucol_getAvailable
@ cdecl ucol_getBound(ptr long long long ptr long ptr) icu.ucol_getBound
@ cdecl ucol_getContractionsAndExpansions(ptr ptr ptr long ptr) icu.ucol_getContractionsAndExpansions
@ cdecl ucol_getDisplayName(str str ptr long ptr) icu.ucol_getDisplayName
@ cdecl ucol_getEquivalentReorderCodes(long ptr long ptr) icu.ucol_getEquivalentReorderCodes
@ cdecl ucol_getFunctionalEquivalent(str long str str ptr ptr) icu.ucol_getFunctionalEquivalent
@ cdecl ucol_getKeywordValues(str ptr) icu.ucol_getKeywordValues
@ cdecl ucol_getKeywordValuesForLocale(str str long ptr) icu.ucol_getKeywordValuesForLocale
@ cdecl ucol_getKeywords(ptr) icu.ucol_getKeywords
@ cdecl ucol_getLocaleByType(ptr long ptr) icu.ucol_getLocaleByType
@ cdecl ucol_getMaxExpansion(ptr long) icu.ucol_getMaxExpansion
@ cdecl ucol_getMaxVariable(ptr) icu.ucol_getMaxVariable
@ cdecl ucol_getOffset(ptr) icu.ucol_getOffset
@ cdecl ucol_getReorderCodes(ptr ptr long ptr) icu.ucol_getReorderCodes
@ cdecl ucol_getRules(ptr ptr) icu.ucol_getRules
@ cdecl ucol_getRulesEx(ptr long ptr long) icu.ucol_getRulesEx
@ cdecl ucol_getSortKey(ptr ptr long ptr long) icu.ucol_getSortKey
@ cdecl ucol_getStrength(ptr) icu.ucol_getStrength
@ cdecl ucol_getTailoredSet(ptr ptr) icu.ucol_getTailoredSet
@ cdecl ucol_getUCAVersion(ptr long) icu.ucol_getUCAVersion
@ cdecl ucol_getVariableTop(ptr ptr) icu.ucol_getVariableTop
@ cdecl ucol_getVersion(ptr long) icu.ucol_getVersion
@ cdecl ucol_greater(ptr ptr long ptr long) icu.ucol_greater
@ cdecl ucol_greaterOrEqual(ptr ptr long ptr long) icu.ucol_greaterOrEqual
@ cdecl ucol_keyHashCode(ptr long) icu.ucol_keyHashCode
@ cdecl ucol_mergeSortkeys(ptr long ptr long ptr long) icu.ucol_mergeSortkeys
@ cdecl ucol_next(ptr ptr) icu.ucol_next
@ cdecl ucol_nextSortKeyPart(ptr ptr long ptr long ptr) icu.ucol_nextSortKeyPart
@ cdecl ucol_open(str ptr) icu.ucol_open
@ cdecl ucol_openAvailableLocales(ptr) icu.ucol_openAvailableLocales
@ cdecl ucol_openBinary(ptr long ptr ptr) icu.ucol_openBinary
@ cdecl ucol_openElements(ptr ptr long ptr) icu.ucol_openElements
@ cdecl ucol_openRules(ptr long long long ptr ptr) icu.ucol_openRules
@ cdecl ucol_previous(ptr ptr) icu.ucol_previous
@ cdecl ucol_primaryOrder(long) icu.ucol_primaryOrder
@ cdecl ucol_reset(ptr) icu.ucol_reset
@ cdecl ucol_safeClone(ptr ptr ptr ptr) icu.ucol_safeClone
@ cdecl ucol_secondaryOrder(long) icu.ucol_secondaryOrder
@ cdecl ucol_setAttribute(ptr long long ptr) icu.ucol_setAttribute
@ cdecl ucol_setMaxVariable(ptr long ptr) icu.ucol_setMaxVariable
@ cdecl ucol_setOffset(ptr long ptr) icu.ucol_setOffset
@ cdecl ucol_setReorderCodes(ptr ptr long ptr) icu.ucol_setReorderCodes
@ cdecl ucol_setStrength(ptr long) icu.ucol_setStrength
@ cdecl ucol_setText(ptr ptr long ptr) icu.ucol_setText
@ cdecl ucol_strcoll(ptr ptr long ptr long) icu.ucol_strcoll
@ cdecl ucol_strcollIter(ptr ptr ptr ptr) icu.ucol_strcollIter
@ cdecl ucol_strcollUTF8(ptr str long str long ptr) icu.ucol_strcollUTF8
@ cdecl ucol_tertiaryOrder(long) icu.ucol_tertiaryOrder
@ cdecl ucsdet_close(ptr) icu.ucsdet_close
@ cdecl ucsdet_detect(ptr ptr) icu.ucsdet_detect
@ cdecl ucsdet_detectAll(ptr ptr ptr) icu.ucsdet_detectAll
@ cdecl ucsdet_enableInputFilter(ptr long) icu.ucsdet_enableInputFilter
@ cdecl ucsdet_getAllDetectableCharsets(ptr ptr) icu.ucsdet_getAllDetectableCharsets
@ cdecl ucsdet_getConfidence(ptr ptr) icu.ucsdet_getConfidence
@ cdecl ucsdet_getLanguage(ptr ptr) icu.ucsdet_getLanguage
@ cdecl ucsdet_getName(ptr ptr) icu.ucsdet_getName
@ cdecl ucsdet_getUChars(ptr ptr long ptr) icu.ucsdet_getUChars
@ cdecl ucsdet_isInputFilterEnabled(ptr) icu.ucsdet_isInputFilterEnabled
@ cdecl ucsdet_open(ptr) icu.ucsdet_open
@ cdecl ucsdet_setDeclaredEncoding(ptr str long ptr) icu.ucsdet_setDeclaredEncoding
@ cdecl ucsdet_setText(ptr str long ptr) icu.ucsdet_setText
@ cdecl udat_adoptNumberFormat(ptr ptr) icu.udat_adoptNumberFormat
@ cdecl udat_adoptNumberFormatForFields(ptr ptr ptr ptr) icu.udat_adoptNumberFormatForFields
@ cdecl udat_applyPattern(ptr long ptr long) icu.udat_applyPattern
@ cdecl udat_clone(ptr ptr) icu.udat_clone
@ cdecl udat_close(ptr) icu.udat_close
@ cdecl udat_countAvailable() icu.udat_countAvailable
@ cdecl udat_countSymbols(ptr long) icu.udat_countSymbols
@ cdecl udat_format(ptr long ptr long ptr ptr) icu.udat_format
@ cdecl udat_formatCalendar(ptr ptr ptr long ptr ptr) icu.udat_formatCalendar
@ cdecl udat_formatCalendarForFields(ptr ptr ptr long ptr ptr) icu.udat_formatCalendarForFields
@ cdecl udat_formatForFields(ptr long ptr long ptr ptr) icu.udat_formatForFields
@ cdecl udat_get2DigitYearStart(ptr ptr) icu.udat_get2DigitYearStart
@ cdecl udat_getAvailable(long) icu.udat_getAvailable
@ cdecl udat_getBooleanAttribute(ptr long ptr) icu.udat_getBooleanAttribute
@ cdecl udat_getCalendar(ptr) icu.udat_getCalendar
@ cdecl udat_getContext(ptr long ptr) icu.udat_getContext
@ cdecl udat_getLocaleByType(ptr long ptr) icu.udat_getLocaleByType
@ cdecl udat_getNumberFormat(ptr) icu.udat_getNumberFormat
@ cdecl udat_getNumberFormatForField(ptr long) icu.udat_getNumberFormatForField
@ cdecl udat_getSymbols(ptr long long ptr long ptr) icu.udat_getSymbols
@ cdecl udat_isLenient(ptr) icu.udat_isLenient
@ cdecl udat_open(long long str ptr long ptr long ptr) icu.udat_open
@ cdecl udat_parse(ptr ptr long ptr ptr) icu.udat_parse
@ cdecl udat_parseCalendar(ptr ptr ptr long ptr ptr) icu.udat_parseCalendar
@ cdecl udat_set2DigitYearStart(ptr long ptr) icu.udat_set2DigitYearStart
@ cdecl udat_setBooleanAttribute(ptr long long ptr) icu.udat_setBooleanAttribute
@ cdecl udat_setCalendar(ptr ptr) icu.udat_setCalendar
@ cdecl udat_setContext(ptr long ptr) icu.udat_setContext
@ cdecl udat_setLenient(ptr long) icu.udat_setLenient
@ cdecl udat_setNumberFormat(ptr ptr) icu.udat_setNumberFormat
@ cdecl udat_setSymbols(ptr long long ptr long ptr) icu.udat_setSymbols
@ cdecl udat_toCalendarDateField(long) icu.udat_toCalendarDateField
@ cdecl udat_toPattern(ptr long ptr long ptr) icu.udat_toPattern
@ cdecl udatpg_addPattern(ptr ptr long long ptr long ptr ptr) icu.udatpg_addPattern
@ cdecl udatpg_clone(ptr ptr) icu.udatpg_clone
@ cdecl udatpg_close(ptr) icu.udatpg_close
@ cdecl udatpg_getAppendItemFormat(ptr long ptr) icu.udatpg_getAppendItemFormat
@ cdecl udatpg_getAppendItemName(ptr long ptr) icu.udatpg_getAppendItemName
@ cdecl udatpg_getBaseSkeleton(ptr ptr long ptr long ptr) icu.udatpg_getBaseSkeleton
@ cdecl udatpg_getBestPattern(ptr ptr long ptr long ptr) icu.udatpg_getBestPattern
@ cdecl udatpg_getBestPatternWithOptions(ptr ptr long long ptr long ptr) icu.udatpg_getBestPatternWithOptions
@ cdecl udatpg_getDateTimeFormat(ptr ptr) icu.udatpg_getDateTimeFormat
@ cdecl udatpg_getDecimal(ptr ptr) icu.udatpg_getDecimal
@ cdecl udatpg_getPatternForSkeleton(ptr ptr long ptr) icu.udatpg_getPatternForSkeleton
@ cdecl udatpg_getSkeleton(ptr ptr long ptr long ptr) icu.udatpg_getSkeleton
@ cdecl udatpg_open(str ptr) icu.udatpg_open
@ cdecl udatpg_openBaseSkeletons(ptr ptr) icu.udatpg_openBaseSkeletons
@ cdecl udatpg_openEmpty(ptr) icu.udatpg_openEmpty
@ cdecl udatpg_openSkeletons(ptr ptr) icu.udatpg_openSkeletons
@ cdecl udatpg_replaceFieldTypes(ptr ptr long ptr long ptr long ptr) icu.udatpg_replaceFieldTypes
@ cdecl udatpg_replaceFieldTypesWithOptions(ptr ptr long ptr long long ptr long ptr) icu.udatpg_replaceFieldTypesWithOptions
@ cdecl udatpg_setAppendItemFormat(ptr long ptr long) icu.udatpg_setAppendItemFormat
@ cdecl udatpg_setAppendItemName(ptr long ptr long) icu.udatpg_setAppendItemName
@ cdecl udatpg_setDateTimeFormat(ptr ptr long) icu.udatpg_setDateTimeFormat
@ cdecl udatpg_setDecimal(ptr ptr long) icu.udatpg_setDecimal
@ cdecl udtitvfmt_close(ptr) icu.udtitvfmt_close
@ cdecl udtitvfmt_format(ptr long long ptr long ptr ptr) icu.udtitvfmt_format
@ cdecl udtitvfmt_open(str ptr long ptr long ptr) icu.udtitvfmt_open
@ cdecl ufieldpositer_close(ptr) icu.ufieldpositer_close
@ cdecl ufieldpositer_next(ptr ptr ptr) icu.ufieldpositer_next
@ cdecl ufieldpositer_open(ptr) icu.ufieldpositer_open
@ cdecl ufmt_close(ptr) icu.ufmt_close
@ cdecl ufmt_getArrayItemByIndex(ptr long ptr) icu.ufmt_getArrayItemByIndex
@ cdecl ufmt_getArrayLength(ptr ptr) icu.ufmt_getArrayLength
@ cdecl ufmt_getDate(ptr ptr) icu.ufmt_getDate
@ cdecl ufmt_getDecNumChars(ptr ptr ptr) icu.ufmt_getDecNumChars
@ cdecl ufmt_getDouble(ptr ptr) icu.ufmt_getDouble
@ cdecl ufmt_getInt64(ptr ptr) icu.ufmt_getInt64
@ cdecl ufmt_getLong(ptr ptr) icu.ufmt_getLong
@ cdecl ufmt_getObject(ptr ptr) icu.ufmt_getObject
@ cdecl ufmt_getType(ptr ptr) icu.ufmt_getType
@ cdecl ufmt_getUChars(ptr ptr ptr) icu.ufmt_getUChars
@ cdecl ufmt_isNumeric(ptr) icu.ufmt_isNumeric
@ cdecl ufmt_open(ptr) icu.ufmt_open
@ cdecl ugender_getInstance(str ptr) icu.ugender_getInstance
@ cdecl ugender_getListGender(ptr ptr long ptr) icu.ugender_getListGender
@ cdecl ulocdata_close(ptr) icu.ulocdata_close
@ cdecl ulocdata_getCLDRVersion(long ptr) icu.ulocdata_getCLDRVersion
@ cdecl ulocdata_getDelimiter(ptr long ptr long ptr) icu.ulocdata_getDelimiter
@ cdecl ulocdata_getExemplarSet(ptr ptr long long ptr) icu.ulocdata_getExemplarSet
@ cdecl ulocdata_getLocaleDisplayPattern(ptr ptr long ptr) icu.ulocdata_getLocaleDisplayPattern
@ cdecl ulocdata_getLocaleSeparator(ptr ptr long ptr) icu.ulocdata_getLocaleSeparator
@ cdecl ulocdata_getMeasurementSystem(str ptr) icu.ulocdata_getMeasurementSystem
@ cdecl ulocdata_getNoSubstitute(ptr) icu.ulocdata_getNoSubstitute
@ cdecl ulocdata_getPaperSize(str ptr ptr ptr) icu.ulocdata_getPaperSize
@ cdecl ulocdata_open(str ptr) icu.ulocdata_open
@ cdecl ulocdata_setNoSubstitute(ptr long) icu.ulocdata_setNoSubstitute
@ cdecl umsg_applyPattern(ptr ptr long ptr ptr) icu.umsg_applyPattern
@ cdecl umsg_autoQuoteApostrophe(ptr long ptr long ptr) icu.umsg_autoQuoteApostrophe
@ cdecl umsg_clone(ptr ptr) icu.umsg_clone
@ cdecl umsg_close(ptr) icu.umsg_close
@ varargs umsg_format(ptr ptr long ptr) icu.umsg_format
@ cdecl umsg_getLocale(ptr) icu.umsg_getLocale
@ cdecl umsg_open(ptr long str ptr ptr) icu.umsg_open
@ varargs umsg_parse(ptr ptr long ptr ptr) icu.umsg_parse
@ cdecl umsg_setLocale(ptr str) icu.umsg_setLocale
@ cdecl umsg_toPattern(ptr ptr long ptr) icu.umsg_toPattern
@ cdecl umsg_vformat(ptr ptr long long ptr) icu.umsg_vformat
@ cdecl umsg_vparse(ptr ptr long ptr long ptr) icu.umsg_vparse
@ cdecl unum_applyPattern(ptr long ptr long ptr ptr) icu.unum_applyPattern
@ cdecl unum_clone(ptr ptr) icu.unum_clone
@ cdecl unum_close(ptr) icu.unum_close
@ cdecl unum_countAvailable() icu.unum_countAvailable
@ cdecl unum_format(ptr long ptr long ptr ptr) icu.unum_format
@ cdecl unum_formatDecimal(ptr str long ptr long ptr ptr) icu.unum_formatDecimal
@ cdecl unum_formatDouble(ptr double ptr long ptr ptr) icu.unum_formatDouble
@ cdecl unum_formatDoubleCurrency(ptr double ptr ptr long ptr ptr) icu.unum_formatDoubleCurrency
@ cdecl unum_formatDoubleForFields(ptr double ptr long ptr ptr) icu.unum_formatDoubleForFields
@ cdecl unum_formatInt64(ptr long ptr long ptr ptr) icu.unum_formatInt64
@ cdecl unum_formatUFormattable(ptr ptr ptr long ptr ptr) icu.unum_formatUFormattable
@ cdecl unum_getAttribute(ptr long) icu.unum_getAttribute
@ cdecl unum_getAvailable(long) icu.unum_getAvailable
@ cdecl unum_getContext(ptr long ptr) icu.unum_getContext
@ cdecl unum_getDoubleAttribute(ptr long) icu.unum_getDoubleAttribute
@ cdecl unum_getLocaleByType(ptr long ptr) icu.unum_getLocaleByType
@ cdecl unum_getSymbol(ptr long ptr long ptr) icu.unum_getSymbol
@ cdecl unum_getTextAttribute(ptr long ptr long ptr) icu.unum_getTextAttribute
@ cdecl unum_open(long ptr long str ptr ptr) icu.unum_open
@ cdecl unum_parse(ptr ptr long ptr ptr) icu.unum_parse
@ cdecl unum_parseDecimal(ptr ptr long ptr str long ptr) icu.unum_parseDecimal
@ cdecl unum_parseDouble(ptr ptr long ptr ptr) icu.unum_parseDouble
@ cdecl unum_parseDoubleCurrency(ptr ptr long ptr ptr ptr) icu.unum_parseDoubleCurrency
@ cdecl unum_parseInt64(ptr ptr long ptr ptr) icu.unum_parseInt64
@ cdecl unum_parseToUFormattable(ptr ptr ptr long ptr ptr) icu.unum_parseToUFormattable
@ cdecl unum_setAttribute(ptr long long) icu.unum_setAttribute
@ cdecl unum_setContext(ptr long ptr) icu.unum_setContext
@ cdecl unum_setDoubleAttribute(ptr long double) icu.unum_setDoubleAttribute
@ cdecl unum_setSymbol(ptr long ptr long ptr) icu.unum_setSymbol
@ cdecl unum_setTextAttribute(ptr long ptr long ptr) icu.unum_setTextAttribute
@ cdecl unum_toPattern(ptr long ptr long ptr) icu.unum_toPattern
@ cdecl unumsys_close(ptr) icu.unumsys_close
@ cdecl unumsys_getDescription(ptr ptr long ptr) icu.unumsys_getDescription
@ cdecl unumsys_getName(ptr) icu.unumsys_getName
@ cdecl unumsys_getRadix(ptr) icu.unumsys_getRadix
@ cdecl unumsys_isAlgorithmic(ptr) icu.unumsys_isAlgorithmic
@ cdecl unumsys_open(str ptr) icu.unumsys_open
@ cdecl unumsys_openAvailableNames(ptr) icu.unumsys_openAvailableNames
@ cdecl unumsys_openByName(str ptr) icu.unumsys_openByName
@ cdecl uplrules_close(ptr) icu.uplrules_close
@ cdecl uplrules_getKeywords(ptr ptr) icu.uplrules_getKeywords
@ cdecl uplrules_open(str ptr) icu.uplrules_open
@ cdecl uplrules_openForType(str long ptr) icu.uplrules_openForType
@ cdecl uplrules_select(ptr double ptr long ptr) icu.uplrules_select
@ cdecl uregex_appendReplacement(ptr ptr long ptr ptr ptr) icu.uregex_appendReplacement
@ cdecl uregex_appendReplacementUText(ptr ptr ptr ptr) icu.uregex_appendReplacementUText
@ cdecl uregex_appendTail(ptr ptr ptr ptr) icu.uregex_appendTail
@ cdecl uregex_appendTailUText(ptr ptr ptr) icu.uregex_appendTailUText
@ cdecl uregex_clone(ptr ptr) icu.uregex_clone
@ cdecl uregex_close(ptr) icu.uregex_close
@ cdecl uregex_end(ptr long ptr) icu.uregex_end
@ cdecl uregex_end64(ptr long ptr) icu.uregex_end64
@ cdecl uregex_find(ptr long ptr) icu.uregex_find
@ cdecl uregex_find64(ptr long ptr) icu.uregex_find64
@ cdecl uregex_findNext(ptr ptr) icu.uregex_findNext
@ cdecl uregex_flags(ptr ptr) icu.uregex_flags
@ cdecl uregex_getFindProgressCallback(ptr ptr ptr ptr) icu.uregex_getFindProgressCallback
@ cdecl uregex_getMatchCallback(ptr ptr ptr ptr) icu.uregex_getMatchCallback
@ cdecl uregex_getStackLimit(ptr ptr) icu.uregex_getStackLimit
@ cdecl uregex_getText(ptr ptr ptr) icu.uregex_getText
@ cdecl uregex_getTimeLimit(ptr ptr) icu.uregex_getTimeLimit
@ cdecl uregex_getUText(ptr ptr ptr) icu.uregex_getUText
@ cdecl uregex_group(ptr long ptr long ptr) icu.uregex_group
@ cdecl uregex_groupCount(ptr ptr) icu.uregex_groupCount
@ cdecl uregex_groupNumberFromCName(ptr str long ptr) icu.uregex_groupNumberFromCName
@ cdecl uregex_groupNumberFromName(ptr ptr long ptr) icu.uregex_groupNumberFromName
@ cdecl uregex_groupUText(ptr long ptr ptr ptr) icu.uregex_groupUText
@ cdecl uregex_hasAnchoringBounds(ptr ptr) icu.uregex_hasAnchoringBounds
@ cdecl uregex_hasTransparentBounds(ptr ptr) icu.uregex_hasTransparentBounds
@ cdecl uregex_hitEnd(ptr ptr) icu.uregex_hitEnd
@ cdecl uregex_lookingAt(ptr long ptr) icu.uregex_lookingAt
@ cdecl uregex_lookingAt64(ptr long ptr) icu.uregex_lookingAt64
@ cdecl uregex_matches(ptr long ptr) icu.uregex_matches
@ cdecl uregex_matches64(ptr long ptr) icu.uregex_matches64
@ cdecl uregex_open(ptr long long ptr ptr) icu.uregex_open
@ cdecl uregex_openC(str long ptr ptr) icu.uregex_openC
@ cdecl uregex_openUText(ptr long ptr ptr) icu.uregex_openUText
@ cdecl uregex_pattern(ptr ptr ptr) icu.uregex_pattern
@ cdecl uregex_patternUText(ptr ptr) icu.uregex_patternUText
@ cdecl uregex_refreshUText(ptr ptr ptr) icu.uregex_refreshUText
@ cdecl uregex_regionEnd(ptr ptr) icu.uregex_regionEnd
@ cdecl uregex_regionEnd64(ptr ptr) icu.uregex_regionEnd64
@ cdecl uregex_regionStart(ptr ptr) icu.uregex_regionStart
@ cdecl uregex_regionStart64(ptr ptr) icu.uregex_regionStart64
@ cdecl uregex_replaceAll(ptr ptr long ptr long ptr) icu.uregex_replaceAll
@ cdecl uregex_replaceAllUText(ptr ptr ptr ptr) icu.uregex_replaceAllUText
@ cdecl uregex_replaceFirst(ptr ptr long ptr long ptr) icu.uregex_replaceFirst
@ cdecl uregex_replaceFirstUText(ptr ptr ptr ptr) icu.uregex_replaceFirstUText
@ cdecl uregex_requireEnd(ptr ptr) icu.uregex_requireEnd
@ cdecl uregex_reset(ptr long ptr) icu.uregex_reset
@ cdecl uregex_reset64(ptr long ptr) icu.uregex_reset64
@ cdecl uregex_setFindProgressCallback(ptr ptr ptr ptr) icu.uregex_setFindProgressCallback
@ cdecl uregex_setMatchCallback(ptr ptr ptr ptr) icu.uregex_setMatchCallback
@ cdecl uregex_setRegion(ptr long long ptr) icu.uregex_setRegion
@ cdecl uregex_setRegion64(ptr long long ptr) icu.uregex_setRegion64
@ cdecl uregex_setRegionAndStart(ptr long long long ptr) icu.uregex_setRegionAndStart
@ cdecl uregex_setStackLimit(ptr long ptr) icu.uregex_setStackLimit
@ cdecl uregex_setText(ptr ptr long ptr) icu.uregex_setText
@ cdecl uregex_setTimeLimit(ptr long ptr) icu.uregex_setTimeLimit
@ cdecl uregex_setUText(ptr ptr ptr) icu.uregex_setUText
@ cdecl uregex_split(ptr ptr long ptr ptr long ptr) icu.uregex_split
@ cdecl uregex_splitUText(ptr ptr long ptr) icu.uregex_splitUText
@ cdecl uregex_start(ptr long ptr) icu.uregex_start
@ cdecl uregex_start64(ptr long ptr) icu.uregex_start64
@ cdecl uregex_useAnchoringBounds(ptr long ptr) icu.uregex_useAnchoringBounds
@ cdecl uregex_useTransparentBounds(ptr long ptr) icu.uregex_useTransparentBounds
@ cdecl uregion_areEqual(ptr ptr) icu.uregion_areEqual
@ cdecl uregion_contains(ptr ptr) icu.uregion_contains
@ cdecl uregion_getAvailable(long ptr) icu.uregion_getAvailable
@ cdecl uregion_getContainedRegions(ptr ptr) icu.uregion_getContainedRegions
@ cdecl uregion_getContainedRegionsOfType(ptr long ptr) icu.uregion_getContainedRegionsOfType
@ cdecl uregion_getContainingRegion(ptr) icu.uregion_getContainingRegion
@ cdecl uregion_getContainingRegionOfType(ptr long) icu.uregion_getContainingRegionOfType
@ cdecl uregion_getNumericCode(ptr) icu.uregion_getNumericCode
@ cdecl uregion_getPreferredValues(ptr ptr) icu.uregion_getPreferredValues
@ cdecl uregion_getRegionCode(ptr) icu.uregion_getRegionCode
@ cdecl uregion_getRegionFromCode(str ptr) icu.uregion_getRegionFromCode
@ cdecl uregion_getRegionFromNumericCode(long ptr) icu.uregion_getRegionFromNumericCode
@ cdecl uregion_getType(ptr) icu.uregion_getType
@ cdecl ureldatefmt_close(ptr) icu.ureldatefmt_close
@ cdecl ureldatefmt_combineDateAndTime(ptr ptr long ptr long ptr long ptr) icu.ureldatefmt_combineDateAndTime
@ cdecl ureldatefmt_format(ptr double long ptr long ptr) icu.ureldatefmt_format
@ cdecl ureldatefmt_formatNumeric(ptr double long ptr long ptr) icu.ureldatefmt_formatNumeric
@ cdecl ureldatefmt_open(str ptr long long ptr) icu.ureldatefmt_open
@ cdecl usearch_close(ptr) icu.usearch_close
@ cdecl usearch_first(ptr ptr) icu.usearch_first
@ cdecl usearch_following(ptr long ptr) icu.usearch_following
@ cdecl usearch_getAttribute(ptr long) icu.usearch_getAttribute
@ cdecl usearch_getBreakIterator(ptr) icu.usearch_getBreakIterator
@ cdecl usearch_getCollator(ptr) icu.usearch_getCollator
@ cdecl usearch_getMatchedLength(ptr) icu.usearch_getMatchedLength
@ cdecl usearch_getMatchedStart(ptr) icu.usearch_getMatchedStart
@ cdecl usearch_getMatchedText(ptr ptr long ptr) icu.usearch_getMatchedText
@ cdecl usearch_getOffset(ptr) icu.usearch_getOffset
@ cdecl usearch_getPattern(ptr ptr) icu.usearch_getPattern
@ cdecl usearch_getText(ptr ptr) icu.usearch_getText
@ cdecl usearch_last(ptr ptr) icu.usearch_last
@ cdecl usearch_next(ptr ptr) icu.usearch_next
@ cdecl usearch_open(ptr long ptr long str ptr ptr) icu.usearch_open
@ cdecl usearch_openFromCollator(ptr long ptr long ptr ptr ptr) icu.usearch_openFromCollator
@ cdecl usearch_preceding(ptr long ptr) icu.usearch_preceding
@ cdecl usearch_previous(ptr ptr) icu.usearch_previous
@ cdecl usearch_reset(ptr) icu.usearch_reset
@ cdecl usearch_setAttribute(ptr long long ptr) icu.usearch_setAttribute
@ cdecl usearch_setBreakIterator(ptr ptr ptr) icu.usearch_setBreakIterator
@ cdecl usearch_setCollator(ptr ptr ptr) icu.usearch_setCollator
@ cdecl usearch_setOffset(ptr long ptr) icu.usearch_setOffset
@ cdecl usearch_setPattern(ptr ptr long ptr) icu.usearch_setPattern
@ cdecl usearch_setText(ptr ptr long ptr) icu.usearch_setText
@ cdecl uspoof_areConfusable(ptr ptr long ptr long ptr) icu.uspoof_areConfusable
@ cdecl uspoof_areConfusableUTF8(ptr str long str long ptr) icu.uspoof_areConfusableUTF8
@ cdecl uspoof_check(ptr ptr long ptr ptr) icu.uspoof_check
@ cdecl uspoof_check2(ptr ptr long ptr ptr) icu.uspoof_check2
@ cdecl uspoof_check2UTF8(ptr str long ptr ptr) icu.uspoof_check2UTF8
@ cdecl uspoof_checkUTF8(ptr str long ptr ptr) icu.uspoof_checkUTF8
@ cdecl uspoof_clone(ptr ptr) icu.uspoof_clone
@ cdecl uspoof_close(ptr) icu.uspoof_close
@ cdecl uspoof_closeCheckResult(ptr) icu.uspoof_closeCheckResult
@ cdecl uspoof_getAllowedChars(ptr ptr) icu.uspoof_getAllowedChars
@ cdecl uspoof_getAllowedLocales(ptr ptr) icu.uspoof_getAllowedLocales
@ cdecl uspoof_getCheckResultChecks(ptr ptr) icu.uspoof_getCheckResultChecks
@ cdecl uspoof_getCheckResultNumerics(ptr ptr) icu.uspoof_getCheckResultNumerics
@ cdecl uspoof_getCheckResultRestrictionLevel(ptr ptr) icu.uspoof_getCheckResultRestrictionLevel
@ cdecl uspoof_getChecks(ptr ptr) icu.uspoof_getChecks
@ cdecl uspoof_getInclusionSet(ptr) icu.uspoof_getInclusionSet
@ cdecl uspoof_getRecommendedSet(ptr) icu.uspoof_getRecommendedSet
@ cdecl uspoof_getRestrictionLevel(ptr) icu.uspoof_getRestrictionLevel
@ cdecl uspoof_getSkeleton(ptr long ptr long ptr long ptr) icu.uspoof_getSkeleton
@ cdecl uspoof_getSkeletonUTF8(ptr long str long str long ptr) icu.uspoof_getSkeletonUTF8
@ cdecl uspoof_open(ptr) icu.uspoof_open
@ cdecl uspoof_openCheckResult(ptr) icu.uspoof_openCheckResult
@ cdecl uspoof_openFromSerialized(ptr long ptr ptr) icu.uspoof_openFromSerialized
@ cdecl uspoof_openFromSource(str long str long ptr ptr ptr) icu.uspoof_openFromSource
@ cdecl uspoof_serialize(ptr ptr long ptr) icu.uspoof_serialize
@ cdecl uspoof_setAllowedChars(ptr ptr ptr) icu.uspoof_setAllowedChars
@ cdecl uspoof_setAllowedLocales(ptr str ptr) icu.uspoof_setAllowedLocales
@ cdecl uspoof_setChecks(ptr long ptr) icu.uspoof_setChecks
@ cdecl uspoof_setRestrictionLevel(ptr long) icu.uspoof_setRestrictionLevel
@ cdecl utmscale_fromInt64(long long ptr) icu.utmscale_fromInt64
@ cdecl utmscale_getTimeScaleValue(long long ptr) icu.utmscale_getTimeScaleValue
@ cdecl utmscale_toInt64(long long ptr) icu.utmscale_toInt64
@ cdecl utrans_clone(ptr ptr) icu.utrans_clone
@ cdecl utrans_close(ptr) icu.utrans_close
@ cdecl utrans_countAvailableIDs() icu.utrans_countAvailableIDs
@ cdecl utrans_getSourceSet(ptr long ptr ptr) icu.utrans_getSourceSet
@ cdecl utrans_getUnicodeID(ptr ptr) icu.utrans_getUnicodeID
@ cdecl utrans_openIDs(ptr) icu.utrans_openIDs
@ cdecl utrans_openInverse(ptr ptr) icu.utrans_openInverse
@ cdecl utrans_openU(ptr long long ptr long ptr ptr) icu.utrans_openU
@ cdecl utrans_register(ptr ptr) icu.utrans_register
@ cdecl utrans_setFilter(ptr ptr long ptr) icu.utrans_setFilter
@ cdecl utrans_toRules(ptr long ptr long ptr) icu.utrans_toRules
@ cdecl utrans_trans(ptr ptr ptr long ptr ptr) icu.utrans_trans
@ cdecl utrans_transIncremental(ptr ptr ptr ptr ptr) icu.utrans_transIncremental
@ cdecl utrans_transIncrementalUChars(ptr ptr ptr long ptr ptr) icu.utrans_transIncrementalUChars
@ cdecl utrans_transUChars(ptr ptr ptr long long ptr ptr) icu.utrans_transUChars
@ cdecl utrans_unregisterID(ptr long) icu.utrans_unregisterID
