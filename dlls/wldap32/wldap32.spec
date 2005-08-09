@ stub LdapGetLastError
@ cdecl LdapMapErrorToWin32(long)
@ cdecl LdapUnicodeToUTF8(wstr long ptr long)
@ cdecl LdapUTF8ToUnicode(str long ptr long)
@ cdecl ber_alloc_t(long)
@ cdecl ber_bvdup(ptr)
@ cdecl ber_bvecfree(ptr)
@ cdecl ber_bvfree(ptr)
@ cdecl ber_first_element(ptr ptr ptr)
@ cdecl ber_flatten(ptr ptr)
@ cdecl ber_free(ptr long)
@ cdecl ber_init(ptr)
@ cdecl ber_next_element(ptr ptr ptr)
@ cdecl ber_peek_tag(ptr ptr)
@ varargs ber_printf(ptr str)
@ varargs ber_scanf(ptr str)
@ cdecl ber_skip_tag(ptr ptr)
@ cdecl ldap_bind(ptr str str long) ldap_bindA
@ cdecl ldap_bindA(ptr str str long)
@ cdecl ldap_bindW(ptr wstr wstr long)
@ cdecl ldap_bind_s(ptr str str long) ldap_bind_sA
@ cdecl ldap_bind_sA(ptr str str long)
@ cdecl ldap_bind_sW(ptr wstr wstr long)
@ cdecl ldap_compare(ptr str str str) ldap_compareA
@ cdecl ldap_compareA(ptr str str str)
@ cdecl ldap_compareW(ptr wstr wstr wstr)
@ cdecl ldap_compare_ext(ptr str str str ptr ptr ptr ptr) ldap_compare_extA
@ cdecl ldap_compare_extA(ptr str str str ptr ptr ptr ptr)
@ cdecl ldap_compare_extW(ptr wstr wstr wstr ptr ptr ptr ptr)
@ cdecl ldap_compare_ext_s(ptr str str str ptr ptr ptr) ldap_compare_ext_sA
@ cdecl ldap_compare_ext_sA(ptr str str str ptr ptr ptr)
@ cdecl ldap_compare_ext_sW(ptr wstr wstr wstr ptr ptr ptr)
@ cdecl ldap_compare_s(ptr str str str) ldap_compare_sA
@ cdecl ldap_compare_sA(ptr str str str)
@ cdecl ldap_compare_sW(ptr wstr wstr wstr)
@ cdecl ldap_dn2ufn(str) ldap_dn2ufnA
@ cdecl ldap_dn2ufnA(str)
@ cdecl ldap_dn2ufnW(wstr)
@ cdecl ldap_err2string(long) ldap_err2stringA
@ cdecl ldap_err2stringA(long)
@ cdecl ldap_err2stringW(long)
@ cdecl ldap_explode_dn(str long) ldap_explode_dnA
@ cdecl ldap_explode_dnA(str long)
@ cdecl ldap_explode_dnW(wstr long)
@ cdecl ldap_get_dn(ptr ptr) ldap_get_dnA
@ cdecl ldap_get_dnA(ptr ptr)
@ cdecl ldap_get_dnW(ptr ptr)
@ cdecl ldap_get_option(ptr long ptr) ldap_get_optionA
@ cdecl ldap_get_optionA(ptr long ptr)
@ cdecl ldap_get_optionW(ptr long ptr)
@ cdecl ldap_init(str long) ldap_initA
@ cdecl ldap_initA(str long)
@ cdecl ldap_initW(wstr long)
@ cdecl ldap_memfree(ptr) ldap_memfreeA
@ cdecl ldap_memfreeA(ptr)
@ cdecl ldap_memfreeW(ptr)
@ cdecl ldap_open(str long) ldap_openA
@ cdecl ldap_openA(str long)
@ cdecl ldap_openW(wstr long)
@ cdecl ldap_perror(ptr ptr) WLDAP32_ldap_perror
@ cdecl ldap_result2error(ptr ptr long) WLDAP32_ldap_result2error
@ cdecl ldap_sasl_bindA(ptr str str ptr ptr ptr ptr)
@ cdecl ldap_sasl_bindW(ptr wstr wstr ptr ptr ptr ptr)
@ cdecl ldap_sasl_bind_sA(ptr str str ptr ptr ptr ptr)
@ cdecl ldap_sasl_bind_sW(ptr wstr wstr ptr ptr ptr ptr)
@ cdecl ldap_search(ptr str long str ptr long) ldap_searchA
@ cdecl ldap_searchA(ptr str long str ptr long)
@ cdecl ldap_searchW(ptr wstr long wstr ptr long)
@ cdecl ldap_search_ext(ptr str long str ptr long ptr ptr long long ptr) ldap_search_extA
@ cdecl ldap_search_extA(ptr str long str ptr long ptr ptr long long ptr)
@ cdecl ldap_search_extW(ptr wstr long wstr ptr long ptr ptr long long ptr)
@ cdecl ldap_search_ext_s(ptr str long str ptr long ptr ptr ptr long ptr) ldap_search_ext_sA
@ cdecl ldap_search_ext_sA(ptr str long str ptr long ptr ptr ptr long ptr)
@ cdecl ldap_search_ext_sW(ptr wstr long wstr ptr long ptr ptr ptr long ptr)
@ cdecl ldap_search_s(ptr str long str ptr long ptr) ldap_search_sA
@ cdecl ldap_search_sA(ptr str long str ptr long ptr)
@ cdecl ldap_search_sW(ptr wstr long wstr ptr long ptr)
@ cdecl ldap_search_st(ptr str long str ptr long ptr ptr) ldap_search_stA
@ cdecl ldap_search_stA(ptr str long str ptr long ptr ptr)
@ cdecl ldap_search_stW(ptr wstr long wstr ptr long ptr ptr)
@ cdecl ldap_set_option(ptr long ptr) ldap_set_optionA
@ cdecl ldap_set_optionA(ptr long ptr)
@ cdecl ldap_set_optionW(ptr long ptr)
@ cdecl ldap_simple_bind(ptr str str) ldap_simple_bindA
@ cdecl ldap_simple_bindA(ptr str str)
@ cdecl ldap_simple_bindW(ptr wstr wstr)
@ cdecl ldap_simple_bind_s(ptr str str) ldap_simple_bind_sA
@ cdecl ldap_simple_bind_sA(ptr str str)
@ cdecl ldap_simple_bind_sW(ptr wstr wstr)
@ cdecl ldap_start_tls_sA(ptr ptr ptr ptr ptr)
@ cdecl ldap_start_tls_sW(ptr ptr ptr ptr ptr)
@ cdecl ldap_unbind(ptr) WLDAP32_ldap_unbind
@ cdecl ldap_unbind_s(ptr) WLDAP32_ldap_unbind_s
@ cdecl ldap_ufn2dn(str ptr) ldap_ufn2dnA
@ cdecl ldap_ufn2dnA(str ptr)
@ cdecl ldap_ufn2dnW(wstr ptr)
@ cdecl ldap_value_free(ptr) ldap_value_freeA
@ cdecl ldap_value_freeA(ptr)
@ cdecl ldap_value_freeW(ptr)
