@ stub bpf_dump
@ stub bpf_filter
@ stub bpf_image
@ stub bpf_validate
@ stub eproto_db
@ cdecl pcap_activate(ptr)
@ cdecl pcap_bufsize(ptr)
@ cdecl pcap_breakloop(ptr)
@ cdecl pcap_can_set_rfmon(ptr)
@ cdecl pcap_close(ptr)
@ cdecl pcap_compile(ptr ptr str long long)
@ stub pcap_compile_nopcap
@ cdecl pcap_create(str ptr)
@ stub pcap_createsrcstr
@ stub pcap_datalink_ext
@ cdecl pcap_datalink(ptr)
@ cdecl pcap_datalink_name_to_val(str)
@ cdecl pcap_datalink_val_to_description(long)
@ stub pcap_datalink_val_to_description_or_dlt
@ cdecl pcap_datalink_val_to_name(long)
@ cdecl pcap_dispatch(ptr long ptr ptr)
@ cdecl pcap_dump(ptr ptr str)
@ cdecl pcap_dump_close(ptr)
@ stub pcap_dump_file
@ stub pcap_dump_flush
@ stub pcap_dump_ftell
@ stub pcap_dump_ftell64
@ stub pcap_dump_hopen
@ cdecl pcap_dump_open(ptr str)
@ stub pcap_dump_open_append
@ stub pcap_ether_aton
@ stub pcap_ether_hostton
@ stub pcap_file
@ stub pcap_fileno
@ cdecl pcap_findalldevs(ptr ptr)
@ cdecl pcap_findalldevs_ex(ptr ptr ptr ptr)
@ cdecl pcap_free_datalinks(ptr)
@ cdecl pcap_free_tstamp_types(ptr)
@ cdecl pcap_freealldevs(ptr)
@ cdecl pcap_freecode(ptr)
@ cdecl pcap_get_airpcap_handle(ptr)
@ cdecl pcap_get_tstamp_precision(ptr)
@ cdecl pcap_geterr(ptr)
@ stub pcap_getevent
@ cdecl pcap_getnonblock(ptr ptr)
@ stub pcap_hopen_offline
@ stub pcap_hopen_offline_with_tstamp_precision
@ cdecl pcap_init(long ptr)
@ stub pcap_inject
@ stub pcap_is_swapped
@ cdecl pcap_lib_version()
@ cdecl pcap_list_datalinks(ptr ptr)
@ cdecl pcap_list_tstamp_types(ptr ptr)
@ stub pcap_live_dump
@ stub pcap_live_dump_ended
@ cdecl pcap_lookupdev(ptr)
@ cdecl pcap_lookupnet(str ptr ptr ptr)
@ cdecl pcap_loop(ptr long ptr ptr)
@ cdecl pcap_major_version(ptr)
@ cdecl pcap_minor_version(ptr)
@ cdecl pcap_next(ptr ptr)
@ stub pcap_next_etherent
@ cdecl pcap_next_ex(ptr ptr ptr)
@ stub pcap_offline_filter
@ stub pcap_oid_get_request
@ stub pcap_oid_set_request
@ cdecl pcap_open(str long long long ptr ptr)
@ stub pcap_open_dead
@ stub pcap_open_dead_with_tstamp_precision
@ cdecl pcap_open_live(str long long long ptr)
@ stub pcap_open_offline
@ stub pcap_open_offline_with_tstamp_precision
@ cdecl pcap_parsesrcstr(str ptr ptr ptr ptr ptr)
@ stub pcap_perror
@ stub pcap_remoteact_accept
@ stub pcap_remoteact_accept_ex
@ stub pcap_remoteact_cleanup
@ stub pcap_remoteact_close
@ stub pcap_remoteact_list
@ cdecl pcap_sendpacket(ptr ptr long)
@ stub pcap_sendqueue_alloc
@ stub pcap_sendqueue_destroy
@ stub pcap_sendqueue_queue
@ stub pcap_sendqueue_transmit
@ cdecl pcap_set_buffer_size(ptr long)
@ cdecl pcap_set_datalink(ptr long)
@ cdecl pcap_set_immediate_mode(ptr long)
@ cdecl pcap_set_promisc(ptr long)
@ cdecl pcap_set_rfmon(ptr long)
@ cdecl pcap_set_snaplen(ptr long)
@ cdecl pcap_set_timeout(ptr long)
@ cdecl pcap_set_tstamp_precision(ptr long)
@ cdecl pcap_set_tstamp_type(ptr long)
@ cdecl pcap_setbuff(ptr long)
@ stub pcap_setdirection
@ cdecl pcap_setfilter(ptr ptr)
@ stub pcap_setmintocopy
@ stub pcap_setmode
@ cdecl pcap_setnonblock(ptr long ptr)
@ stub pcap_setsampling
@ stub pcap_setuserbuffer
@ cdecl pcap_snapshot(ptr)
@ cdecl pcap_stats(ptr ptr)
@ stub pcap_stats_ex
@ cdecl pcap_statustostr(long)
@ cdecl pcap_strerror(long) msvcrt.strerror
@ cdecl pcap_tstamp_type_name_to_val(str)
@ cdecl pcap_tstamp_type_val_to_description(long)
@ cdecl pcap_tstamp_type_val_to_name(long)
@ stub pcap_version
@ cdecl pcap_wsockinit()
