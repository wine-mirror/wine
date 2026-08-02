/* include/config.h.  Generated from config.h.in by configure.  */
/* include/config.h.in.  Generated from configure.ac by autoheader.  */

#ifndef __WINE_CONFIG_H
#define __WINE_CONFIG_H
#ifndef WINE_CROSSTEST

/* Define to a function attribute for Microsoft hotpatch assembly prefix. */
#define DECLSPEC_HOTPATCH __attribute__((__ms_hook_prologue__))

/* Define to the file extension for executables. */
#define EXEEXT ""

/* Define to 1 if you have the <alias.h> header file. */
/* #undef HAVE_ALIAS_H */

/* Define to 1 if you have the <alsa/asoundlib.h> header file. */
/* #undef HAVE_ALSA_ASOUNDLIB_H */

/* Define to 1 if you have the <AL/al.h> header file. */
/* #undef HAVE_AL_AL_H */

/* Define to 1 if you have the <ApplicationServices/ApplicationServices.h>
   header file. */
/* #undef HAVE_APPLICATIONSERVICES_APPLICATIONSERVICES_H */

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the <arpa/nameser.h> header file. */
#define HAVE_ARPA_NAMESER_H 1

/* Define to 1 if you have the `asctime_r' function. */
#define HAVE_ASCTIME_R 1

/* Define to 1 if you have the <asm/types.h> header file. */
#define HAVE_ASM_TYPES_H 1

/* Define to 1 if you have the <AudioToolbox/AudioConverter.h> header file. */
/* #undef HAVE_AUDIOTOOLBOX_AUDIOCONVERTER_H */

/* Define to 1 if you have the <AudioUnit/AudioComponent.h> header file. */
/* #undef HAVE_AUDIOUNIT_AUDIOCOMPONENT_H */

/* Define to 1 if you have the <AudioUnit/AudioUnit.h> header file. */
/* #undef HAVE_AUDIOUNIT_AUDIOUNIT_H */

/* Define to 1 if you have the `AUGraphAddNode' function. */
/* #undef HAVE_AUGRAPHADDNODE */

/* Define to 1 if you have the <capi20.h> header file. */
/* #undef HAVE_CAPI20_H */

/* Define to 1 if you have the <Carbon/Carbon.h> header file. */
/* #undef HAVE_CARBON_CARBON_H */

/* Define to 1 if you have the `chsize' function. */
/* #undef HAVE_CHSIZE */

/* Define to 1 if you have the `clock_gettime' function. */
#define HAVE_CLOCK_GETTIME 1

/* Define to 1 if you have the <CL/cl.h> header file. */
/* #undef HAVE_CL_CL_H */

/* Define to 1 if you have the <CoreAudio/CoreAudio.h> header file. */
/* #undef HAVE_COREAUDIO_COREAUDIO_H */

/* Define to 1 if you have the <CoreServices/CoreServices.h> header file. */
/* #undef HAVE_CORESERVICES_CORESERVICES_H */

/* Define to 1 if you have the <cups/cups.h> header file. */
/* #undef HAVE_CUPS_CUPS_H */

/* Define to 1 if you have the <curses.h> header file. */
#define HAVE_CURSES_H 1

/* Define if you have the daylight variable */
#define HAVE_DAYLIGHT 1

/* Define to 1 if you have the <direct.h> header file. */
/* #undef HAVE_DIRECT_H */

/* Define to 1 if you have the <dirent.h> header file. */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <DiskArbitration/DiskArbitration.h> header
   file. */
/* #undef HAVE_DISKARBITRATION_DISKARBITRATION_H */

/* Define to 1 if you have the `dladdr' function. */
#define HAVE_DLADDR 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the `dlopen' function. */
#define HAVE_DLOPEN 1

/* Define to 1 if you have the <elf.h> header file. */
#define HAVE_ELF_H 1

/* Define to 1 if you have the `epoll_create' function. */
#define HAVE_EPOLL_CREATE 1

/* Define to 1 if you have the `fallocate' function. */
#define HAVE_FALLOCATE 1

/* Define to 1 if you have the `ffs' function. */
#define HAVE_FFS 1

/* Define to 1 if you have the `finite' function. */
#define HAVE_FINITE 1

/* Define to 1 if you have the <float.h> header file. */
#define HAVE_FLOAT_H 1

/* Define to 1 if you have the `fnmatch' function. */
#define HAVE_FNMATCH 1

/* Define to 1 if you have the <fnmatch.h> header file. */
#define HAVE_FNMATCH_H 1

/* Define to 1 if you have the <fontconfig/fontconfig.h> header file. */
/* #undef HAVE_FONTCONFIG_FONTCONFIG_H */

/* Define to 1 if you have the `fork' function. */
#define HAVE_FORK 1

/* Define to 1 if you have the `fpclass' function. */
/* #undef HAVE_FPCLASS */

/* Define if FreeType 2 is installed */
/* #undef HAVE_FREETYPE */

/* Define to 1 if the system has the type `fsblkcnt_t'. */
#define HAVE_FSBLKCNT_T 1

/* Define to 1 if the system has the type `fsfilcnt_t'. */
#define HAVE_FSFILCNT_T 1

/* Define to 1 if you have the `fstatfs' function. */
#define HAVE_FSTATFS 1

/* Define to 1 if you have the `fstatvfs' function. */
#define HAVE_FSTATVFS 1

/* Define to 1 if you have the <ft2build.h> header file. */
/* #undef HAVE_FT2BUILD_H */

/* Define to 1 if you have the `ftruncate' function. */
#define HAVE_FTRUNCATE 1

/* Define to 1 if the system has the type `FT_TrueTypeEngineType'. */
/* #undef HAVE_FT_TRUETYPEENGINETYPE */

/* Define to 1 if you have the `futimens' function. */
#define HAVE_FUTIMENS 1

/* Define to 1 if you have the `futimes' function. */
#define HAVE_FUTIMES 1

/* Define to 1 if you have the `futimesat' function. */
#define HAVE_FUTIMESAT 1

/* Define to 1 if you have the `getaddrinfo' function. */
#define HAVE_GETADDRINFO 1

/* Define to 1 if you have the `getattrlist' function. */
/* #undef HAVE_GETATTRLIST */

/* Define to 1 if you have the `getdirentries' function. */
#define HAVE_GETDIRENTRIES 1

/* Define to 1 if you have the `getnameinfo' function. */
#define HAVE_GETNAMEINFO 1

/* Define to 1 if you have the `getnetbyname' function. */
#define HAVE_GETNETBYNAME 1

/* Define to 1 if you have the <getopt.h> header file. */
#define HAVE_GETOPT_H 1

/* Define to 1 if you have the `getopt_long_only' function. */
#define HAVE_GETOPT_LONG_ONLY 1

/* Define to 1 if you have the `getprotobyname' function. */
#define HAVE_GETPROTOBYNAME 1

/* Define to 1 if you have the `getprotobynumber' function. */
#define HAVE_GETPROTOBYNUMBER 1

/* Define to 1 if you have the `getpwuid' function. */
#define HAVE_GETPWUID 1

/* Define to 1 if you have the `getservbyport' function. */
#define HAVE_GETSERVBYPORT 1

/* Define to 1 if you have the <gettext-po.h> header file. */
/* #undef HAVE_GETTEXT_PO_H */

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the `getuid' function. */
#define HAVE_GETUID 1

/* Define if we have the libgphoto2 development environment */
/* #undef HAVE_GPHOTO2 */

/* Define if we have the libgphoto2_port development environment */
/* #undef HAVE_GPHOTO2_PORT */

/* Define to 1 if you have the <grp.h> header file. */
#define HAVE_GRP_H 1

/* Define to 1 if you have the <gsm/gsm.h> header file. */
/* #undef HAVE_GSM_GSM_H */

/* Define to 1 if you have the <gsm.h> header file. */
/* #undef HAVE_GSM_H */

/* Define to 1 if you have the <ieeefp.h> header file. */
/* #undef HAVE_IEEEFP_H */

/* Define to 1 if you have the <ifaddrs.h> header file. */
#define HAVE_IFADDRS_H 1

/* Define to 1 if you have the <inet/mib2.h> header file. */
/* #undef HAVE_INET_MIB2_H */

/* Define to 1 if you have the `inet_network' function. */
#define HAVE_INET_NETWORK 1

/* Define to 1 if you have the `inet_ntop' function. */
#define HAVE_INET_NTOP 1

/* Define to 1 if you have the `inet_pton' function. */
#define HAVE_INET_PTON 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `IOHIDManagerCreate' function. */
/* #undef HAVE_IOHIDMANAGERCREATE */

/* Define to 1 if you have the <IOKit/hid/IOHIDLib.h> header file. */
/* #undef HAVE_IOKIT_HID_IOHIDLIB_H */

/* Define to 1 if you have the <IOKit/IOKitLib.h> header file. */
/* #undef HAVE_IOKIT_IOKITLIB_H */

/* Define to 1 if you have the <io.h> header file. */
/* #undef HAVE_IO_H */

/* Define to 1 if you have the `isinf' function. */
#define HAVE_ISINF 1

/* Define to 1 if you have the `isnan' function. */
#define HAVE_ISNAN 1

/* Define to 1 if you have the <jpeglib.h> header file. */
/* #undef HAVE_JPEGLIB_H */

/* Define to 1 if you have the `kqueue' function. */
/* #undef HAVE_KQUEUE */

/* Define to 1 if you have the <kstat.h> header file. */
/* #undef HAVE_KSTAT_H */

/* Define to 1 if you have the <lber.h> header file. */
/* #undef HAVE_LBER_H */

/* Define if you have the LittleCMS development environment */
/* #undef HAVE_LCMS2 */

/* Define to 1 if you have the <lcms2.h> header file. */
/* #undef HAVE_LCMS2_H */

/* Define if you have the OpenLDAP development environment */
/* #undef HAVE_LDAP */

/* Define to 1 if you have the `ldap_count_references' function. */
/* #undef HAVE_LDAP_COUNT_REFERENCES */

/* Define to 1 if you have the `ldap_first_reference' function. */
/* #undef HAVE_LDAP_FIRST_REFERENCE */

/* Define to 1 if you have the <ldap.h> header file. */
/* #undef HAVE_LDAP_H */

/* Define to 1 if you have the `ldap_next_reference' function. */
/* #undef HAVE_LDAP_NEXT_REFERENCE */

/* Define to 1 if you have the `ldap_parse_reference' function. */
/* #undef HAVE_LDAP_PARSE_REFERENCE */

/* Define to 1 if you have the `ldap_parse_sortresponse_control' function. */
/* #undef HAVE_LDAP_PARSE_SORTRESPONSE_CONTROL */

/* Define to 1 if you have the `ldap_parse_sort_control' function. */
/* #undef HAVE_LDAP_PARSE_SORT_CONTROL */

/* Define to 1 if you have the `ldap_parse_vlvresponse_control' function. */
/* #undef HAVE_LDAP_PARSE_VLVRESPONSE_CONTROL */

/* Define to 1 if you have the `ldap_parse_vlv_control' function. */
/* #undef HAVE_LDAP_PARSE_VLV_CONTROL */

/* Define to 1 if you have the `gettextpo' library (-lgettextpo). */
/* #undef HAVE_LIBGETTEXTPO */

/* Define to 1 if you have the `i386' library (-li386). */
/* #undef HAVE_LIBI386 */

/* Define to 1 if you have the `kstat' library (-lkstat). */
/* #undef HAVE_LIBKSTAT */

/* Define to 1 if you have the `ossaudio' library (-lossaudio). */
/* #undef HAVE_LIBOSSAUDIO */

/* Define to 1 if you have the <libv4l1.h> header file. */
/* #undef HAVE_LIBV4L1_H */

/* Define if you have the libxml2 library */
#define HAVE_LIBXML2 1

/* Define to 1 if you have the <libxml/parser.h> header file. */
#define HAVE_LIBXML_PARSER_H 1

/* Define to 1 if you have the <libxml/SAX2.h> header file. */
#define HAVE_LIBXML_SAX2_H 1

/* Define to 1 if you have the <libxml/xmlsave.h> header file. */
#define HAVE_LIBXML_XMLSAVE_H 1

/* Define if you have the X Shape extension */
/* #undef HAVE_LIBXSHAPE */

/* Define to 1 if you have the <libxslt/pattern.h> header file. */
/* #undef HAVE_LIBXSLT_PATTERN_H */

/* Define to 1 if you have the <libxslt/transform.h> header file. */
/* #undef HAVE_LIBXSLT_TRANSFORM_H */

/* Define if you have the X Shm extension */
/* #undef HAVE_LIBXXSHM */

/* Define to 1 if you have the <link.h> header file. */
#define HAVE_LINK_H 1

/* Define if <linux/joystick.h> defines the Linux 2.2 joystick API */
#define HAVE_LINUX_22_JOYSTICK_API 1

/* Define to 1 if you have the <linux/capi.h> header file. */
#define HAVE_LINUX_CAPI_H 1

/* Define to 1 if you have the <linux/cdrom.h> header file. */
#define HAVE_LINUX_CDROM_H 1

/* Define to 1 if you have the <linux/compiler.h> header file. */
/* #undef HAVE_LINUX_COMPILER_H */

/* Define to 1 if you have the <linux/filter.h> header file. */
#define HAVE_LINUX_FILTER_H 1

/* Define if Linux-style gethostbyname_r and gethostbyaddr_r are available */
#define HAVE_LINUX_GETHOSTBYNAME_R_6 1

/* Define to 1 if you have the <linux/hdreg.h> header file. */
#define HAVE_LINUX_HDREG_H 1

/* Define to 1 if you have the <linux/input.h> header file. */
#define HAVE_LINUX_INPUT_H 1

/* Define to 1 if you have the <linux/ioctl.h> header file. */
#define HAVE_LINUX_IOCTL_H 1

/* Define to 1 if you have the <linux/ipx.h> header file. */
/* #undef HAVE_LINUX_IPX_H */

/* Define to 1 if you have the <linux/irda.h> header file. */
/* #undef HAVE_LINUX_IRDA_H */

/* Define to 1 if you have the <linux/joystick.h> header file. */
#define HAVE_LINUX_JOYSTICK_H 1

/* Define to 1 if you have the <linux/major.h> header file. */
#define HAVE_LINUX_MAJOR_H 1

/* Define to 1 if you have the <linux/param.h> header file. */
#define HAVE_LINUX_PARAM_H 1

/* Define to 1 if you have the <linux/serial.h> header file. */
#define HAVE_LINUX_SERIAL_H 1

/* Define to 1 if you have the <linux/types.h> header file. */
#define HAVE_LINUX_TYPES_H 1

/* Define to 1 if you have the <linux/ucdrom.h> header file. */
/* #undef HAVE_LINUX_UCDROM_H */

/* Define to 1 if you have the <linux/videodev2.h> header file. */
#define HAVE_LINUX_VIDEODEV2_H 1

/* Define to 1 if you have the <linux/videodev.h> header file. */
/* #undef HAVE_LINUX_VIDEODEV_H */

/* Define to 1 if the system has the type `long long'. */
#define HAVE_LONG_LONG 1

/* Define to 1 if you have the `lstat' function. */
#define HAVE_LSTAT 1

/* Define to 1 if you have the <lwp.h> header file. */
/* #undef HAVE_LWP_H */

/* Define to 1 if you have the <machine/cpu.h> header file. */
/* #undef HAVE_MACHINE_CPU_H */

/* Define to 1 if you have the <machine/limits.h> header file. */
/* #undef HAVE_MACHINE_LIMITS_H */

/* Define to 1 if you have the <machine/sysarch.h> header file. */
/* #undef HAVE_MACHINE_SYSARCH_H */

/* Define to 1 if you have the <mach/machine.h> header file. */
/* #undef HAVE_MACH_MACHINE_H */

/* Define to 1 if you have the <mach/mach.h> header file. */
/* #undef HAVE_MACH_MACH_H */

/* Define to 1 if you have the <mach-o/dyld_images.h> header file. */
/* #undef HAVE_MACH_O_DYLD_IMAGES_H */

/* Define to 1 if you have the <mach-o/loader.h> header file. */
/* #undef HAVE_MACH_O_LOADER_H */

/* Define to 1 if you have the <mach-o/nlist.h> header file. */
/* #undef HAVE_MACH_O_NLIST_H */

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mmap' function. */
#define HAVE_MMAP 1

/* Define to 1 if you have the <mntent.h> header file. */
#define HAVE_MNTENT_H 1

/* Define to 1 if the system has the type `mode_t'. */
#define HAVE_MODE_T 1

/* Define to 1 if you have the `mousemask' function. */
#define HAVE_MOUSEMASK 1

/* Define to 1 if you have the <mpg123.h> header file. */
/* #undef HAVE_MPG123_H */

/* Define to 1 if you have the <ncurses.h> header file. */
#define HAVE_NCURSES_H 1

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/icmp_var.h> header file. */
/* #undef HAVE_NETINET_ICMP_VAR_H */

/* Define to 1 if you have the <netinet/if_ether.h> header file. */
#define HAVE_NETINET_IF_ETHER_H 1

/* Define to 1 if you have the <netinet/if_inarp.h> header file. */
/* #undef HAVE_NETINET_IF_INARP_H */

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <netinet/in_pcb.h> header file. */
/* #undef HAVE_NETINET_IN_PCB_H */

/* Define to 1 if you have the <netinet/in_systm.h> header file. */
#define HAVE_NETINET_IN_SYSTM_H 1

/* Define to 1 if you have the <netinet/ip.h> header file. */
#define HAVE_NETINET_IP_H 1

/* Define to 1 if you have the <netinet/ip_icmp.h> header file. */
#define HAVE_NETINET_IP_ICMP_H 1

/* Define to 1 if you have the <netinet/ip_var.h> header file. */
/* #undef HAVE_NETINET_IP_VAR_H */

/* Define to 1 if you have the <netinet/tcp_fsm.h> header file. */
/* #undef HAVE_NETINET_TCP_FSM_H */

/* Define to 1 if you have the <netinet/tcp.h> header file. */
#define HAVE_NETINET_TCP_H 1

/* Define to 1 if you have the <netinet/tcp_timer.h> header file. */
/* #undef HAVE_NETINET_TCP_TIMER_H */

/* Define to 1 if you have the <netinet/tcp_var.h> header file. */
/* #undef HAVE_NETINET_TCP_VAR_H */

/* Define to 1 if you have the <netinet/udp.h> header file. */
#define HAVE_NETINET_UDP_H 1

/* Define to 1 if you have the <netinet/udp_var.h> header file. */
/* #undef HAVE_NETINET_UDP_VAR_H */

/* Define to 1 if you have the <netipx/ipx.h> header file. */
#define HAVE_NETIPX_IPX_H 1

/* Define to 1 if you have the <net/if_arp.h> header file. */
#define HAVE_NET_IF_ARP_H 1

/* Define to 1 if you have the <net/if_dl.h> header file. */
/* #undef HAVE_NET_IF_DL_H */

/* Define to 1 if you have the <net/if.h> header file. */
#define HAVE_NET_IF_H 1

/* Define to 1 if you have the <net/if_types.h> header file. */
/* #undef HAVE_NET_IF_TYPES_H */

/* Define to 1 if you have the <net/route.h> header file. */
#define HAVE_NET_ROUTE_H 1

/* Define to 1 if `_msg_ptr' is a member of `ns_msg'. */
#define HAVE_NS_MSG__MSG_PTR 1

/* Define to 1 if the system has the type `off_t'. */
#define HAVE_OFF_T 1

/* Define if mkdir takes only one argument */
/* #undef HAVE_ONE_ARG_MKDIR */

/* Define to 1 if OpenAL is available */
/* #undef HAVE_OPENAL */

/* Define to 1 if you have the <OpenAL/al.h> header file. */
/* #undef HAVE_OPENAL_AL_H */

/* Define to 1 if you have the <OpenCL/opencl.h> header file. */
/* #undef HAVE_OPENCL_OPENCL_H */

/* Define to 1 if `numaudioengines' is a member of `oss_sysinfo'. */
/* #undef HAVE_OSS_SYSINFO_NUMAUDIOENGINES */

/* Define to 1 if you have the `pclose' function. */
#define HAVE_PCLOSE 1

/* Define to 1 if the system has the type `pid_t'. */
#define HAVE_PID_T 1

/* Define to 1 if you have the `pipe2' function. */
#define HAVE_PIPE2 1

/* Define to 1 if you have the <png.h> header file. */
/* #undef HAVE_PNG_H */

/* Define to 1 if libpng has the png_set_expand_gray_1_2_4_to_8 function. */
/* #undef HAVE_PNG_SET_EXPAND_GRAY_1_2_4_TO_8 */

/* Define to 1 if you have the `poll' function. */
#define HAVE_POLL 1

/* Define to 1 if you have the <poll.h> header file. */
#define HAVE_POLL_H 1

/* Define to 1 if you have the `popen' function. */
#define HAVE_POPEN 1

/* Define to 1 if you have the `port_create' function. */
/* #undef HAVE_PORT_CREATE */

/* Define to 1 if you have the <port.h> header file. */
/* #undef HAVE_PORT_H */

/* Define if we can use ppdev.h for parallel port access */
#define HAVE_PPDEV 1

/* Define to 1 if you have the `prctl' function. */
#define HAVE_PRCTL 1

/* Define to 1 if you have the `pread' function. */
#define HAVE_PREAD 1

/* Define to 1 if you have the <process.h> header file. */
/* #undef HAVE_PROCESS_H */

/* Define to 1 if you have the `pthread_attr_get_np' function. */
/* #undef HAVE_PTHREAD_ATTR_GET_NP */

/* Define to 1 if you have the `pthread_getattr_np' function. */
#define HAVE_PTHREAD_GETATTR_NP 1

/* Define to 1 if you have the `pthread_getthreadid_np' function. */
/* #undef HAVE_PTHREAD_GETTHREADID_NP */

/* Define to 1 if you have the `pthread_get_stackaddr_np' function. */
/* #undef HAVE_PTHREAD_GET_STACKADDR_NP */

/* Define to 1 if you have the `pthread_get_stacksize_np' function. */
/* #undef HAVE_PTHREAD_GET_STACKSIZE_NP */

/* Define to 1 if you have the <pthread.h> header file. */
#define HAVE_PTHREAD_H 1

/* Define to 1 if you have the <pthread_np.h> header file. */
/* #undef HAVE_PTHREAD_NP_H */

/* Define to 1 if you have the <pwd.h> header file. */
#define HAVE_PWD_H 1

/* Define to 1 if you have the `pwrite' function. */
#define HAVE_PWRITE 1

/* Define to 1 if you have the <QuickTime/ImageCompression.h> header file. */
/* #undef HAVE_QUICKTIME_IMAGECOMPRESSION_H */

/* Define to 1 if you have the `readdir' function. */
#define HAVE_READDIR 1

/* Define to 1 if you have the `readlink' function. */
#define HAVE_READLINK 1

/* Define to 1 if the system has the type `request_sense'. */
/* #undef HAVE_REQUEST_SENSE */

/* Define if you have the resolver library and header */
#define HAVE_RESOLV 1

/* Define to 1 if you have the <resolv.h> header file. */
#define HAVE_RESOLV_H 1

/* Define to 1 if you have the <sched.h> header file. */
#define HAVE_SCHED_H 1

/* Define to 1 if you have the `sched_setaffinity' function. */
#define HAVE_SCHED_SETAFFINITY 1

/* Define to 1 if you have the `sched_yield' function. */
#define HAVE_SCHED_YIELD 1

/* Define to 1 if `cmd' is a member of `scsireq_t'. */
/* #undef HAVE_SCSIREQ_T_CMD */

/* Define to 1 if you have the <scsi/scsi.h> header file. */
#define HAVE_SCSI_SCSI_H 1

/* Define to 1 if you have the <scsi/scsi_ioctl.h> header file. */
#define HAVE_SCSI_SCSI_IOCTL_H 1

/* Define to 1 if you have the <scsi/sg.h> header file. */
#define HAVE_SCSI_SG_H 1

/* Define to 1 if you have the <Security/Security.h> header file. */
/* #undef HAVE_SECURITY_SECURITY_H */

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the `sendmsg' function. */
#define HAVE_SENDMSG 1

/* Define to 1 if you have the `setproctitle' function. */
/* #undef HAVE_SETPROCTITLE */

/* Define to 1 if you have the `setrlimit' function. */
#define HAVE_SETRLIMIT 1

/* Define to 1 if you have the `settimeofday' function. */
#define HAVE_SETTIMEOFDAY 1

/* Define to 1 if `interface_id' is a member of `sg_io_hdr_t'. */
#define HAVE_SG_IO_HDR_T_INTERFACE_ID 1

/* Define if sigaddset is supported */
#define HAVE_SIGADDSET 1

/* Define to 1 if you have the `sigaltstack' function. */
#define HAVE_SIGALTSTACK 1

/* Define to 1 if `si_fd' is a member of `siginfo_t'. */
#define HAVE_SIGINFO_T_SI_FD 1

/* Define to 1 if you have the `sigprocmask' function. */
#define HAVE_SIGPROCMASK 1

/* Define to 1 if the system has the type `sigset_t'. */
#define HAVE_SIGSET_T 1

/* Define to 1 if the system has the type `size_t'. */
#define HAVE_SIZE_T 1

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the `socketpair' function. */
#define HAVE_SOCKETPAIR 1

/* Define to 1 if the system has the type `ssize_t'. */
#define HAVE_SSIZE_T 1

/* Define to 1 if you have the `SSLCopyPeerCertificates' function. */
/* #undef HAVE_SSLCOPYPEERCERTIFICATES */

/* Define to 1 if you have the `statfs' function. */
#define HAVE_STATFS 1

/* Define to 1 if you have the `statvfs' function. */
#define HAVE_STATVFS 1

/* Define to 1 if you have the <stdbool.h> header file. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strncasecmp' function. */
#define HAVE_STRNCASECMP 1

/* Define to 1 if you have the <stropts.h> header file. */
/* #undef HAVE_STROPTS_H */

/* Define to 1 if you have the `strtold' function. */
#define HAVE_STRTOLD 1

/* Define to 1 if you have the `strtoll' function. */
#define HAVE_STRTOLL 1

/* Define to 1 if you have the `strtoull' function. */
#define HAVE_STRTOULL 1

/* Define to 1 if `d_reclen' is a member of `struct dirent'. */
#define HAVE_STRUCT_DIRENT_D_RECLEN 1

/* Define to 1 if `direction' is a member of `struct ff_effect'. */
#define HAVE_STRUCT_FF_EFFECT_DIRECTION 1

/* Define to 1 if `icps_inhist' is a member of `struct icmpstat'. */
/* #undef HAVE_STRUCT_ICMPSTAT_ICPS_INHIST */

/* Define to 1 if `icps_outhist' is a member of `struct icmpstat'. */
/* #undef HAVE_STRUCT_ICMPSTAT_ICPS_OUTHIST */

/* Define to 1 if `ifr_hwaddr' is a member of `struct ifreq'. */
#define HAVE_STRUCT_IFREQ_IFR_HWADDR 1

/* Define to 1 if `ips_total' is a member of `struct ipstat'. */
/* #undef HAVE_STRUCT_IPSTAT_IPS_TOTAL */

/* Define to 1 if `ips_total' is a member of `struct ip_stats'. */
/* #undef HAVE_STRUCT_IP_STATS_IPS_TOTAL */

/* Define to 1 if `msg_accrights' is a member of `struct msghdr'. */
/* #undef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */

/* Define to 1 if `mt_blkno' is a member of `struct mtget'. */
#define HAVE_STRUCT_MTGET_MT_BLKNO 1

/* Define to 1 if `mt_blksiz' is a member of `struct mtget'. */
/* #undef HAVE_STRUCT_MTGET_MT_BLKSIZ */

/* Define to 1 if `mt_gstat' is a member of `struct mtget'. */
#define HAVE_STRUCT_MTGET_MT_GSTAT 1

/* Define to 1 if `name' is a member of `struct option'. */
#define HAVE_STRUCT_OPTION_NAME 1

/* Define to 1 if `sin6_scope_id' is a member of `struct sockaddr_in6'. */
#define HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID 1

/* Define to 1 if `sa_len' is a member of `struct sockaddr'. */
/* #undef HAVE_STRUCT_SOCKADDR_SA_LEN */

/* Define to 1 if `sun_len' is a member of `struct sockaddr_un'. */
/* #undef HAVE_STRUCT_SOCKADDR_UN_SUN_LEN */

/* Define to 1 if `f_bavail' is a member of `struct statfs'. */
#define HAVE_STRUCT_STATFS_F_BAVAIL 1

/* Define to 1 if `f_bfree' is a member of `struct statfs'. */
#define HAVE_STRUCT_STATFS_F_BFREE 1

/* Define to 1 if `f_favail' is a member of `struct statfs'. */
/* #undef HAVE_STRUCT_STATFS_F_FAVAIL */

/* Define to 1 if `f_ffree' is a member of `struct statfs'. */
#define HAVE_STRUCT_STATFS_F_FFREE 1

/* Define to 1 if `f_frsize' is a member of `struct statfs'. */
#define HAVE_STRUCT_STATFS_F_FRSIZE 1

/* Define to 1 if `f_namelen' is a member of `struct statfs'. */
#define HAVE_STRUCT_STATFS_F_NAMELEN 1

/* Define to 1 if `f_blocks' is a member of `struct statvfs'. */
#define HAVE_STRUCT_STATVFS_F_BLOCKS 1

/* Define to 1 if `st_atim' is a member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_ATIM 1

/* Define to 1 if `st_atimespec' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_ATIMESPEC */

/* Define to 1 if `st_birthtim' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_BIRTHTIM */

/* Define to 1 if `st_birthtime' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_BIRTHTIME */

/* Define to 1 if `st_birthtimespec' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC */

/* Define to 1 if `st_blocks' is a member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_BLOCKS 1

/* Define to 1 if `st_ctim' is a member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_CTIM 1

/* Define to 1 if `st_ctimespec' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_CTIMESPEC */

/* Define to 1 if `st_mtim' is a member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_MTIM 1

/* Define to 1 if `st_mtimespec' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_MTIMESPEC */

/* Define to 1 if `__st_birthtim' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT___ST_BIRTHTIM */

/* Define to 1 if `__st_birthtime' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT___ST_BIRTHTIME */

/* Define to 1 if `tcps_connattempt' is a member of `struct tcpstat'. */
/* #undef HAVE_STRUCT_TCPSTAT_TCPS_CONNATTEMPT */

/* Define to 1 if `tcps_connattempt' is a member of `struct tcp_stats'. */
/* #undef HAVE_STRUCT_TCP_STATS_TCPS_CONNATTEMPT */

/* Define to 1 if `udps_ipackets' is a member of `struct udpstat'. */
/* #undef HAVE_STRUCT_UDPSTAT_UDPS_IPACKETS */

/* Define to 1 if the system has the type `struct xinpgen'. */
/* #undef HAVE_STRUCT_XINPGEN */

/* Define to 1 if you have the `symlink' function. */
#define HAVE_SYMLINK 1

/* Define to 1 if you have the <syscall.h> header file. */
#define HAVE_SYSCALL_H 1

/* Define to 1 if you have the <sys/asoundlib.h> header file. */
/* #undef HAVE_SYS_ASOUNDLIB_H */

/* Define to 1 if you have the <sys/attr.h> header file. */
/* #undef HAVE_SYS_ATTR_H */

/* Define to 1 if you have the <sys/cdio.h> header file. */
/* #undef HAVE_SYS_CDIO_H */

/* Define to 1 if you have the <sys/elf32.h> header file. */
/* #undef HAVE_SYS_ELF32_H */

/* Define to 1 if you have the <sys/epoll.h> header file. */
#define HAVE_SYS_EPOLL_H 1

/* Define to 1 if you have the <sys/event.h> header file. */
/* #undef HAVE_SYS_EVENT_H */

/* Define to 1 if you have the <sys/exec_elf.h> header file. */
/* #undef HAVE_SYS_EXEC_ELF_H */

/* Define to 1 if you have the <sys/filio.h> header file. */
/* #undef HAVE_SYS_FILIO_H */

/* Define to 1 if you have the <sys/inotify.h> header file. */
#define HAVE_SYS_INOTIFY_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/ipc.h> header file. */
#define HAVE_SYS_IPC_H 1

/* Define to 1 if you have the <sys/limits.h> header file. */
/* #undef HAVE_SYS_LIMITS_H */

/* Define to 1 if you have the <sys/link.h> header file. */
/* #undef HAVE_SYS_LINK_H */

/* Define to 1 if you have the <sys/mman.h> header file. */
#define HAVE_SYS_MMAN_H 1

/* Define to 1 if you have the <sys/modem.h> header file. */
/* #undef HAVE_SYS_MODEM_H */

/* Define to 1 if you have the <sys/mount.h> header file. */
#define HAVE_SYS_MOUNT_H 1

/* Define to 1 if you have the <sys/msg.h> header file. */
#define HAVE_SYS_MSG_H 1

/* Define to 1 if you have the <sys/mtio.h> header file. */
#define HAVE_SYS_MTIO_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/poll.h> header file. */
#define HAVE_SYS_POLL_H 1

/* Define to 1 if you have the <sys/prctl.h> header file. */
#define HAVE_SYS_PRCTL_H 1

/* Define to 1 if you have the <sys/protosw.h> header file. */
/* #undef HAVE_SYS_PROTOSW_H */

/* Define to 1 if you have the <sys/ptrace.h> header file. */
#define HAVE_SYS_PTRACE_H 1

/* Define to 1 if you have the <sys/resource.h> header file. */
#define HAVE_SYS_RESOURCE_H 1

/* Define to 1 if you have the <sys/scsiio.h> header file. */
/* #undef HAVE_SYS_SCSIIO_H */

/* Define to 1 if you have the <sys/shm.h> header file. */
#define HAVE_SYS_SHM_H 1

/* Define to 1 if you have the <sys/signal.h> header file. */
#define HAVE_SYS_SIGNAL_H 1

/* Define to 1 if you have the <sys/socketvar.h> header file. */
#define HAVE_SYS_SOCKETVAR_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/sockio.h> header file. */
/* #undef HAVE_SYS_SOCKIO_H */

/* Define to 1 if you have the <sys/statfs.h> header file. */
#define HAVE_SYS_STATFS_H 1

/* Define to 1 if you have the <sys/statvfs.h> header file. */
#define HAVE_SYS_STATVFS_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/strtio.h> header file. */
/* #undef HAVE_SYS_STRTIO_H */

/* Define to 1 if you have the <sys/syscall.h> header file. */
#define HAVE_SYS_SYSCALL_H 1

/* Define to 1 if you have the <sys/sysctl.h> header file. */
/* #undef HAVE_SYS_SYSCTL_H */

/* Define to 1 if you have the <sys/thr.h> header file. */
/* #undef HAVE_SYS_THR_H */

/* Define to 1 if you have the <sys/tihdr.h> header file. */
/* #undef HAVE_SYS_TIHDR_H */

/* Define to 1 if you have the <sys/timeout.h> header file. */
/* #undef HAVE_SYS_TIMEOUT_H */

/* Define to 1 if you have the <sys/times.h> header file. */
#define HAVE_SYS_TIMES_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/uio.h> header file. */
#define HAVE_SYS_UIO_H 1

/* Define to 1 if you have the <sys/un.h> header file. */
#define HAVE_SYS_UN_H 1

/* Define to 1 if you have the <sys/user.h> header file. */
#define HAVE_SYS_USER_H 1

/* Define to 1 if you have the <sys/utsname.h> header file. */
#define HAVE_SYS_UTSNAME_H 1

/* Define to 1 if you have the <sys/vfs.h> header file. */
#define HAVE_SYS_VFS_H 1

/* Define to 1 if you have the <sys/vm86.h> header file. */
/* #undef HAVE_SYS_VM86_H */

/* Define to 1 if you have the <sys/wait.h> header file. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the `tcdrain' function. */
#define HAVE_TCDRAIN 1

/* Define to 1 if you have the <termios.h> header file. */
#define HAVE_TERMIOS_H 1

/* Define to 1 if you have the `thr_kill2' function. */
/* #undef HAVE_THR_KILL2 */

/* Define to 1 if you have the <tiffio.h> header file. */
/* #undef HAVE_TIFFIO_H */

/* Define to 1 if you have the `timegm' function. */
#define HAVE_TIMEGM 1

/* Define if you have the timezone variable */
#define HAVE_TIMEZONE 1

/* Define to 1 if you have the <ucontext.h> header file. */
#define HAVE_UCONTEXT_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `usleep' function. */
#define HAVE_USLEEP 1

/* Define to 1 if you have the <utime.h> header file. */
#define HAVE_UTIME_H 1

/* Define to 1 if you have the <valgrind/memcheck.h> header file. */
/* #undef HAVE_VALGRIND_MEMCHECK_H */

/* Define to 1 if you have the <valgrind/valgrind.h> header file. */
/* #undef HAVE_VALGRIND_VALGRIND_H */

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the <X11/extensions/shape.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_SHAPE_H */

/* Define to 1 if you have the <X11/extensions/Xcomposite.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XCOMPOSITE_H */

/* Define to 1 if you have the <X11/extensions/xf86vmode.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XF86VMODE_H */

/* Define to 1 if you have the <X11/extensions/xf86vmproto.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XF86VMPROTO_H */

/* Define to 1 if you have the <X11/extensions/Xinerama.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XINERAMA_H */

/* Define to 1 if you have the <X11/extensions/XInput2.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XINPUT2_H */

/* Define to 1 if you have the <X11/extensions/XInput.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XINPUT_H */

/* Define to 1 if you have the <X11/extensions/Xrandr.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XRANDR_H */

/* Define to 1 if you have the <X11/extensions/Xrender.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XRENDER_H */

/* Define to 1 if you have the <X11/extensions/XShm.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XSHM_H */

/* Define to 1 if you have the <X11/Xcursor/Xcursor.h> header file. */
/* #undef HAVE_X11_XCURSOR_XCURSOR_H */

/* Define to 1 if you have the <X11/XKBlib.h> header file. */
/* #undef HAVE_X11_XKBLIB_H */

/* Define to 1 if you have the <X11/Xlib.h> header file. */
/* #undef HAVE_X11_XLIB_H */

/* Define to 1 if you have the <X11/Xutil.h> header file. */
/* #undef HAVE_X11_XUTIL_H */

/* Define to 1 if `xcookie' is a member of `XEvent'. */
/* #undef HAVE_XEVENT_XCOOKIE */

/* Define to 1 if `callback' is a member of `XICCallback'. */
/* #undef HAVE_XICCALLBACK_CALLBACK */

/* Define if you have the XKB extension */
/* #undef HAVE_XKB */

/* Define if libxml2 has the xmlDocProperties enum */
#define HAVE_XMLDOC_PROPERTIES 1

/* Define if libxml2 has the xmlFirstElementChild function */
#define HAVE_XMLFIRSTELEMENTCHILD 1

/* Define if libxml2 has the xmlNewDocPI function */
#define HAVE_XMLNEWDOCPI 1

/* Define if libxml2 has the xmlReadMemory function */
#define HAVE_XMLREADMEMORY 1

/* Define if libxml2 has the xmlSchemaSetParserStructuredErrors function */
#define HAVE_XMLSCHEMASSETPARSERSTRUCTUREDERRORS 1

/* Define if libxml2 has the xmlSchemaSetValidStructuredErrors function */
#define HAVE_XMLSCHEMASSETVALIDSTRUCTUREDERRORS 1

/* Define if Xrender has the XRenderCreateLinearGradient function */
/* #undef HAVE_XRENDERCREATELINEARGRADIENT */

/* Define if Xrender has the XRenderSetPictureTransform function */
/* #undef HAVE_XRENDERSETPICTURETRANSFORM */

/* Define if Xrandr has the XRRGetScreenResources function */
/* #undef HAVE_XRRGETSCREENRESOURCES */

/* Define to 1 if you have the `z' library (-lz). */
#define HAVE_ZLIB 1

/* Define to 1 if you have the <zlib.h> header file. */
#define HAVE_ZLIB_H 1

/* Define to 1 if you have the `_finite' function. */
/* #undef HAVE__FINITE */

/* Define to 1 if you have the `_isnan' function. */
/* #undef HAVE__ISNAN */

/* Define to 1 if you have the `_pclose' function. */
/* #undef HAVE__PCLOSE */

/* Define to 1 if you have the `_popen' function. */
/* #undef HAVE__POPEN */

/* Define to 1 if you have the `_snprintf' function. */
/* #undef HAVE__SNPRINTF */

/* Define to 1 if you have the `_spawnvp' function. */
/* #undef HAVE__SPAWNVP */

/* Define to 1 if you have the `_strdup' function. */
/* #undef HAVE__STRDUP */

/* Define to 1 if you have the `_stricmp' function. */
/* #undef HAVE__STRICMP */

/* Define to 1 if you have the `_strnicmp' function. */
/* #undef HAVE__STRNICMP */

/* Define to 1 if you have the `_strtoi64' function. */
/* #undef HAVE__STRTOI64 */

/* Define to 1 if you have the `_strtoui64' function. */
/* #undef HAVE__STRTOUI64 */

/* Define to 1 if you have the `_vsnprintf' function. */
/* #undef HAVE__VSNPRINTF */

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "wine-devel@winehq.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "Wine"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "Wine 1.6.2"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "wine"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://www.winehq.org"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.6.2"

/* Define to the soname of the libcapi20 library. */
/* #undef SONAME_LIBCAPI20 */

/* Define to the soname of the libcups library. */
/* #undef SONAME_LIBCUPS */

/* Define to the soname of the libcurses library. */
/* #undef SONAME_LIBCURSES */

/* Define to the soname of the libdbus-1 library. */
/* #undef SONAME_LIBDBUS_1 */

/* Define to the soname of the libfontconfig library. */
/* #undef SONAME_LIBFONTCONFIG */

/* Define to the soname of the libfreetype library. */
/* #undef SONAME_LIBFREETYPE */

/* Define to the soname of the libGL library. */
/* #undef SONAME_LIBGL */

/* Define to the soname of the libgnutls library. */
/* #undef SONAME_LIBGNUTLS */

/* Define to the soname of the libgsm library. */
/* #undef SONAME_LIBGSM */

/* Define to the soname of the libhal library. */
/* #undef SONAME_LIBHAL */

/* Define to the soname of the libjpeg library. */
/* #undef SONAME_LIBJPEG */

/* Define to the soname of the libncurses library. */
#define SONAME_LIBNCURSES "libncurses.so.6"

/* Define to the soname of the libodbc library. */
#define SONAME_LIBODBC "libodbc.so"

/* Define to the soname of the libopenal library. */
/* #undef SONAME_LIBOPENAL */

/* Define to the soname of the libOSMesa library. */
/* #undef SONAME_LIBOSMESA */

/* Define to the soname of the libpng library. */
/* #undef SONAME_LIBPNG */

/* Define to the soname of the libsane library. */
/* #undef SONAME_LIBSANE */

/* Define to the soname of the libtiff library. */
/* #undef SONAME_LIBTIFF */

/* Define to the soname of the libv4l1 library. */
/* #undef SONAME_LIBV4L1 */

/* Define to the soname of the libX11 library. */
/* #undef SONAME_LIBX11 */

/* Define to the soname of the libXcomposite library. */
/* #undef SONAME_LIBXCOMPOSITE */

/* Define to the soname of the libXcursor library. */
/* #undef SONAME_LIBXCURSOR */

/* Define to the soname of the libXext library. */
/* #undef SONAME_LIBXEXT */

/* Define to the soname of the libXi library. */
/* #undef SONAME_LIBXI */

/* Define to the soname of the libXinerama library. */
/* #undef SONAME_LIBXINERAMA */

/* Define to the soname of the libXrandr library. */
/* #undef SONAME_LIBXRANDR */

/* Define to the soname of the libXrender library. */
/* #undef SONAME_LIBXRENDER */

/* Define to the soname of the libxslt library. */
/* #undef SONAME_LIBXSLT */

/* Define to the soname of the libXxf86vm library. */
/* #undef SONAME_LIBXXF86VM */

/* Define to 1 if the `S_IS*' macros in <sys/stat.h> do not work properly. */
/* #undef STAT_MACROS_BROKEN */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if the X Window System is missing or not being used. */
#define X_DISPLAY_MISSING 1

/* Enable large inode numbers on Mac OS X 10.5.  */
#ifndef _DARWIN_USE_64_BIT_INODE
# define _DARWIN_USE_64_BIT_INODE 1
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to a macro to output a .cfi assembly pseudo-op */
#define __ASM_CFI(str) str

/* Define to a macro to define an assembly function */
#define __ASM_DEFINE_FUNC(name,suffix,code) asm(".text\n\t.align 4\n\t.globl " #name suffix "\n\t.type " #name suffix ",@function\n" #name suffix ":\n\t.cfi_startproc\n\t" code "\n\t.cfi_endproc\n\t.previous");

/* Define to a macro to generate an assembly function directive */
#define __ASM_FUNC(name) ".type " __ASM_NAME(name) ",@function"

/* Define to a macro to generate an assembly function with C calling
   convention */
#define __ASM_GLOBAL_FUNC(name,code) __ASM_DEFINE_FUNC(name,"",code)

/* Define to a macro to generate an assembly name from a C symbol */
#define __ASM_NAME(name) name

/* Define to a macro to generate an stdcall suffix */
#define __ASM_STDCALL(args) ""

/* Define to a macro to generate an assembly function with stdcall calling
   convention */
#define __ASM_STDCALL_FUNC(name,args,code) __ASM_DEFINE_FUNC(name,__ASM_STDCALL(args),code)

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

#endif /* WINE_CROSSTEST */
#endif /* __WINE_CONFIG_H */
