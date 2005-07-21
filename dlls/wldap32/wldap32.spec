@ stub LdapGetLastError
@ stub LdapMapErrorToWin32
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
@ cdecl ldap_init(str long) ldap_initA
@ cdecl ldap_initA(str long)
@ cdecl ldap_initW(wstr long)
@ cdecl ldap_open(str long) ldap_openA
@ cdecl ldap_openA(str long)
@ cdecl ldap_openW(wstr long)
@ cdecl ldap_simple_bind(ptr str str) ldap_simple_bindA
@ cdecl ldap_simple_bindA(ptr str str)
@ cdecl ldap_simple_bindW(ptr wstr wstr)
@ cdecl ldap_simple_bind_s(ptr str str) ldap_simple_bind_sA
@ cdecl ldap_simple_bind_sA(ptr str str)
@ cdecl ldap_simple_bind_sW(ptr wstr wstr)
@ cdecl ldap_unbind(ptr) WLDAP32_ldap_unbind
@ cdecl ldap_unbind_s(ptr) WLDAP32_ldap_unbind_s
