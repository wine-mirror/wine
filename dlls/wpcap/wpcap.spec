@ stub bpf_dump
@ stub bpf_filter
@ stub bpf_image
@ stub bpf_validate
@ stub endservent
@ stub eproto_db
@ stub getservent
@ stub install_bpf_program
@ cdecl pcap_breakloop(ptr) wine_pcap_breakloop
@ cdecl pcap_close(ptr) wine_pcap_close
@ cdecl pcap_compile(ptr ptr str long long) wine_pcap_compile
@ stub pcap_compile_nopcap
@ stub pcap_createsrcstr
@ cdecl pcap_datalink(ptr) wine_pcap_datalink
@ cdecl pcap_datalink_name_to_val(str) wine_pcap_datalink_name_to_val
@ cdecl pcap_datalink_val_to_description(long) wine_pcap_datalink_val_to_description
@ cdecl pcap_datalink_val_to_name(long) wine_pcap_datalink_val_to_name
@ cdecl pcap_dispatch(ptr long ptr ptr) wine_pcap_dispatch
@ cdecl pcap_dump(ptr ptr str) wine_pcap_dump
@ stub pcap_dump_close
@ stub pcap_dump_file
@ stub pcap_dump_flush
@ stub pcap_dump_ftell
@ cdecl pcap_dump_open(ptr str) wine_pcap_dump_open
@ stub pcap_file
@ stub pcap_fileno
@ cdecl pcap_findalldevs(ptr ptr) wine_pcap_findalldevs
@ cdecl pcap_findalldevs_ex(ptr ptr ptr ptr) wine_pcap_findalldevs_ex
@ cdecl pcap_freealldevs(ptr) wine_pcap_freealldevs
@ cdecl pcap_freecode(ptr) wine_pcap_freecode
@ cdecl pcap_get_airpcap_handle(ptr) wine_pcap_get_airpcap_handle
@ cdecl pcap_geterr(ptr) wine_pcap_geterr
@ stub pcap_getevent
@ cdecl pcap_getnonblock(ptr ptr) wine_pcap_getnonblock
@ stub pcap_is_swapped
@ cdecl pcap_lib_version() wine_pcap_lib_version
@ cdecl pcap_list_datalinks(ptr ptr) wine_pcap_list_datalinks
@ stub pcap_live_dump
@ stub pcap_live_dump_ended
@ cdecl pcap_lookupdev(ptr) wine_pcap_lookupdev
@ cdecl pcap_lookupnet(str ptr ptr ptr) wine_pcap_lookupnet
@ cdecl pcap_loop(ptr long ptr ptr) wine_pcap_loop
@ cdecl pcap_major_version(ptr) wine_pcap_major_version
@ cdecl pcap_minor_version(ptr) wine_pcap_minor_version
@ cdecl pcap_next(ptr ptr) wine_pcap_next
@ stub pcap_next_etherent
@ cdecl pcap_next_ex(ptr ptr ptr) wine_pcap_next_ex
@ stub pcap_offline_filter
@ stub pcap_offline_read
@ cdecl pcap_open(str long long long ptr ptr) wine_pcap_open
@ stub pcap_open_dead
@ cdecl pcap_open_live(str long long long ptr) wine_pcap_open_live
@ stub pcap_open_offline
@ cdecl pcap_parsesrcstr(str ptr ptr ptr ptr ptr) wine_pcap_parsesrcstr
@ stub pcap_perror
@ stub pcap_read
@ stub pcap_remoteact_accept
@ stub pcap_remoteact_cleanup
@ stub pcap_remoteact_close
@ stub pcap_remoteact_list
@ cdecl pcap_sendpacket(ptr ptr long) wine_pcap_sendpacket
@ stub pcap_sendqueue_alloc
@ stub pcap_sendqueue_destroy
@ stub pcap_sendqueue_queue
@ stub pcap_sendqueue_transmit
@ cdecl pcap_set_datalink(ptr long) wine_pcap_set_datalink
@ cdecl pcap_setbuff(ptr long) wine_pcap_setbuff
@ cdecl pcap_setfilter(ptr ptr) wine_pcap_setfilter
@ stub pcap_setmintocopy
@ stub pcap_setmode
@ cdecl pcap_setnonblock(ptr long ptr) wine_pcap_setnonblock
@ stub pcap_setsampling
@ stub pcap_setuserbuffer
@ cdecl pcap_snapshot(ptr) wine_pcap_snapshot
@ cdecl pcap_stats(ptr ptr) wine_pcap_stats
@ stub pcap_stats_ex
@ cdecl pcap_strerror(long) msvcrt.strerror
@ cdecl wsockinit() wine_wsockinit
