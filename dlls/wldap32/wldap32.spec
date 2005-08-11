@ stub LdapGetLastError
@ cdecl LdapMapErrorToWin32(long)
@ cdecl LdapUTF8ToUnicode(str long ptr long)
@ cdecl LdapUnicodeToUTF8(wstr long ptr long)
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
@ stub cldap_open
@ stub cldap_openA
@ stub cldap_openW
@ stub ldap_abandon
@ cdecl ldap_add(ptr str ptr) ldap_addA
@ cdecl ldap_addA(ptr str ptr)
@ cdecl ldap_addW(ptr wstr ptr)
@ cdecl ldap_add_ext(ptr str ptr ptr ptr ptr) ldap_add_extA
@ cdecl ldap_add_extA(ptr str ptr ptr ptr ptr)
@ cdecl ldap_add_extW(ptr wstr ptr ptr ptr ptr)
@ cdecl ldap_add_ext_s(ptr str ptr ptr ptr) ldap_add_ext_sA
@ cdecl ldap_add_ext_sA(ptr str ptr ptr ptr)
@ cdecl ldap_add_ext_sW(ptr str ptr ptr ptr)
@ cdecl ldap_add_s(ptr str ptr) ldap_add_sA
@ cdecl ldap_add_sA(ptr str ptr)
@ cdecl ldap_add_sW(ptr wstr ptr)
@ cdecl ldap_bind(ptr str str long) ldap_bindA
@ cdecl ldap_bindA(ptr str str long)
@ cdecl ldap_bindW(ptr wstr wstr long)
@ cdecl ldap_bind_s(ptr str str long) ldap_bind_sA
@ cdecl ldap_bind_sA(ptr str str long)
@ cdecl ldap_bind_sW(ptr wstr wstr long)
@ stub ldap_check_filterA
@ stub ldap_check_filterW
@ stub ldap_cleanup
@ stub ldap_close_extended_op
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
@ stub ldap_conn_from_msg
@ stub ldap_connect
@ stub ldap_control_free
@ stub ldap_control_freeA
@ stub ldap_control_freeW
@ stub ldap_controls_free
@ stub ldap_controls_freeA
@ stub ldap_controls_freeW
@ stub ldap_count_entries
@ stub ldap_count_references
@ stub ldap_count_values
@ stub ldap_count_valuesA
@ stub ldap_count_valuesW
@ stub ldap_count_values_len
@ stub ldap_create_page_control
@ stub ldap_create_page_controlA
@ stub ldap_create_page_controlW
@ stub ldap_create_sort_control
@ stub ldap_create_sort_controlA
@ stub ldap_create_sort_controlW
@ stub ldap_create_vlv_controlA
@ stub ldap_create_vlv_controlW
@ cdecl ldap_delete(ptr str) ldap_deleteA
@ cdecl ldap_deleteA(ptr str)
@ cdecl ldap_deleteW(ptr wstr)
@ cdecl ldap_delete_ext(ptr str ptr ptr ptr) ldap_delete_extA
@ cdecl ldap_delete_extA(ptr str ptr ptr ptr)
@ cdecl ldap_delete_extW(ptr wstr ptr ptr ptr)
@ cdecl ldap_delete_ext_s(ptr str ptr ptr) ldap_delete_ext_sA
@ cdecl ldap_delete_ext_sA(ptr str ptr ptr)
@ cdecl ldap_delete_ext_sW(ptr wstr ptr ptr)
@ cdecl ldap_delete_s(ptr str) ldap_delete_sA
@ cdecl ldap_delete_sA(ptr str)
@ cdecl ldap_delete_sW(ptr wstr)
@ cdecl ldap_dn2ufn(str) ldap_dn2ufnA
@ cdecl ldap_dn2ufnA(str)
@ cdecl ldap_dn2ufnW(wstr)
@ stub ldap_encode_sort_controlA
@ stub ldap_encode_sort_controlW
@ cdecl ldap_err2string(long) ldap_err2stringA
@ cdecl ldap_err2stringA(long)
@ cdecl ldap_err2stringW(long)
@ stub ldap_escape_filter_element
@ stub ldap_escape_filter_elementA
@ stub ldap_escape_filter_elementW
@ cdecl ldap_explode_dn(str long) ldap_explode_dnA
@ cdecl ldap_explode_dnA(str long)
@ cdecl ldap_explode_dnW(wstr long)
@ stub ldap_extended_operation
@ stub ldap_extended_operationA
@ stub ldap_extended_operationW
@ stub ldap_extended_operation_sA
@ stub ldap_extended_operation_sW
@ stub ldap_first_attribute
@ stub ldap_first_attributeA
@ stub ldap_first_attributeW
@ stub ldap_first_entry
@ stub ldap_first_reference
@ stub ldap_free_controls
@ stub ldap_free_controlsA
@ stub ldap_free_controlsW
@ cdecl ldap_get_dn(ptr ptr) ldap_get_dnA
@ cdecl ldap_get_dnA(ptr ptr)
@ cdecl ldap_get_dnW(ptr ptr)
@ stub ldap_get_next_page
@ stub ldap_get_next_page_s
@ cdecl ldap_get_option(ptr long ptr) ldap_get_optionA
@ cdecl ldap_get_optionA(ptr long ptr)
@ cdecl ldap_get_optionW(ptr long ptr)
@ stub ldap_get_paged_count
@ stub ldap_get_values
@ stub ldap_get_valuesA
@ stub ldap_get_valuesW
@ stub ldap_get_values_len
@ stub ldap_get_values_lenA
@ stub ldap_get_values_lenW
@ cdecl ldap_init(str long) ldap_initA
@ cdecl ldap_initA(str long)
@ cdecl ldap_initW(wstr long)
@ cdecl ldap_memfree(ptr) ldap_memfreeA
@ cdecl ldap_memfreeA(ptr)
@ cdecl ldap_memfreeW(ptr)
@ stub ldap_modify
@ stub ldap_modifyA
@ stub ldap_modifyW
@ stub ldap_modify_ext
@ stub ldap_modify_extA
@ stub ldap_modify_extW
@ stub ldap_modify_ext_s
@ stub ldap_modify_ext_sA
@ stub ldap_modify_ext_sW
@ stub ldap_modify_s
@ stub ldap_modify_sA
@ stub ldap_modify_sW
@ stub ldap_modrdn
@ stub ldap_modrdn2
@ stub ldap_modrdn2A
@ stub ldap_modrdn2W
@ stub ldap_modrdn2_s
@ stub ldap_modrdn2_sA
@ stub ldap_modrdn2_sW
@ stub ldap_modrdnA
@ stub ldap_modrdnW
@ stub ldap_modrdn_s
@ stub ldap_modrdn_sA
@ stub ldap_modrdn_sW
@ stub ldap_msgfree
@ stub ldap_next_attribute
@ stub ldap_next_attributeA
@ stub ldap_next_attributeW
@ stub ldap_next_entry
@ stub ldap_next_reference
@ cdecl ldap_open(str long) ldap_openA
@ cdecl ldap_openA(str long)
@ cdecl ldap_openW(wstr long)
@ stub ldap_parse_extended_resultA
@ stub ldap_parse_extended_resultW
@ stub ldap_parse_page_control
@ stub ldap_parse_page_controlA
@ stub ldap_parse_page_controlW
@ stub ldap_parse_reference
@ stub ldap_parse_referenceA
@ stub ldap_parse_referenceW
@ stub ldap_parse_result
@ stub ldap_parse_resultA
@ stub ldap_parse_resultW
@ stub ldap_parse_sort_control
@ stub ldap_parse_sort_controlA
@ stub ldap_parse_sort_controlW
@ stub ldap_parse_vlv_controlA
@ stub ldap_parse_vlv_controlW
@ cdecl ldap_perror(ptr ptr) WLDAP32_ldap_perror
@ stub ldap_rename_ext
@ stub ldap_rename_extA
@ stub ldap_rename_extW
@ stub ldap_rename_ext_s
@ stub ldap_rename_ext_sA
@ stub ldap_rename_ext_sW
@ stub ldap_result
@ cdecl ldap_result2error(ptr ptr long) WLDAP32_ldap_result2error
@ cdecl ldap_sasl_bindA(ptr str str ptr ptr ptr ptr)
@ cdecl ldap_sasl_bindW(ptr wstr wstr ptr ptr ptr ptr)
@ cdecl ldap_sasl_bind_sA(ptr str str ptr ptr ptr ptr)
@ cdecl ldap_sasl_bind_sW(ptr wstr wstr ptr ptr ptr ptr)
@ cdecl ldap_search(ptr str long str ptr long) ldap_searchA
@ cdecl ldap_searchA(ptr str long str ptr long)
@ cdecl ldap_searchW(ptr wstr long wstr ptr long)
@ stub ldap_search_abandon_page
@ cdecl ldap_search_ext(ptr str long str ptr long ptr ptr long long ptr) ldap_search_extA
@ cdecl ldap_search_extA(ptr str long str ptr long ptr ptr long long ptr)
@ cdecl ldap_search_extW(ptr wstr long wstr ptr long ptr ptr long long ptr)
@ cdecl ldap_search_ext_s(ptr str long str ptr long ptr ptr ptr long ptr) ldap_search_ext_sA
@ cdecl ldap_search_ext_sA(ptr str long str ptr long ptr ptr ptr long ptr)
@ cdecl ldap_search_ext_sW(ptr wstr long wstr ptr long ptr ptr ptr long ptr)
@ stub ldap_search_init_page
@ stub ldap_search_init_pageA
@ stub ldap_search_init_pageW
@ cdecl ldap_search_s(ptr str long str ptr long ptr) ldap_search_sA
@ cdecl ldap_search_sA(ptr str long str ptr long ptr)
@ cdecl ldap_search_sW(ptr wstr long wstr ptr long ptr)
@ cdecl ldap_search_st(ptr str long str ptr long ptr ptr) ldap_search_stA
@ cdecl ldap_search_stA(ptr str long str ptr long ptr ptr)
@ cdecl ldap_search_stW(ptr wstr long wstr ptr long ptr ptr)
@ stub ldap_set_dbg_flags
@ stub ldap_set_dbg_routine
@ cdecl ldap_set_option(ptr long ptr) ldap_set_optionA
@ cdecl ldap_set_optionA(ptr long ptr)
@ cdecl ldap_set_optionW(ptr long ptr)
@ cdecl ldap_simple_bind(ptr str str) ldap_simple_bindA
@ cdecl ldap_simple_bindA(ptr str str)
@ cdecl ldap_simple_bindW(ptr wstr wstr)
@ cdecl ldap_simple_bind_s(ptr str str) ldap_simple_bind_sA
@ cdecl ldap_simple_bind_sA(ptr str str)
@ cdecl ldap_simple_bind_sW(ptr wstr wstr)
@ stub ldap_sslinit
@ stub ldap_sslinitA
@ stub ldap_sslinitW
@ cdecl ldap_start_tls_sA(ptr ptr ptr ptr ptr)
@ cdecl ldap_start_tls_sW(ptr ptr ptr ptr ptr)
@ stub ldap_startup
@ stub ldap_stop_tls_s
@ cdecl ldap_ufn2dn(str ptr) ldap_ufn2dnA
@ cdecl ldap_ufn2dnA(str ptr)
@ cdecl ldap_ufn2dnW(wstr ptr)
@ cdecl ldap_unbind(ptr) WLDAP32_ldap_unbind
@ cdecl ldap_unbind_s(ptr) WLDAP32_ldap_unbind_s
@ cdecl ldap_value_free(ptr) ldap_value_freeA
@ cdecl ldap_value_freeA(ptr)
@ cdecl ldap_value_freeW(ptr)
@ stub ldap_value_free_len
