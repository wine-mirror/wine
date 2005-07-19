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
@ stub ldap_open
@ stub ldap_openA
@ stub ldap_openW
