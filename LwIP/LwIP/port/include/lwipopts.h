/* Auto-generated config file lwipopts.h */
#ifndef LWIPOPTS_H
#define LWIPOPTS_H

// <<< Use Configuration Wizard in Context Menu >>>

// <h> Basic Configuration

// <q> Enable DHCP
// <id> lwip_dhcp
#ifndef LWIP_DHCP
#define LWIP_DHCP 1
#endif

// <q> NO RTOS
// <id> lwip_no_sys
#ifndef NO_SYS
#define NO_SYS 1
#endif

// <o> The size of the heap memory <0-100000>
// <i> Defines size of the heap memory
// <i> Default: 4096
// <id> lwip_mem_size
#ifndef MEM_SIZE
#define MEM_SIZE 4096
#endif

// <q> Enables TCP
// <id> lwip_tcp
#ifndef LWIP_TCP
#define LWIP_TCP 1
#endif

// <q> Enables UDP
// <id> lwip_udp
#ifndef LWIP_UDP
#define LWIP_UDP 1
#endif

// <q> Enables ICMP
// <id> lwip_icmp
#ifndef LWIP_ICMP
#define LWIP_ICMP 1
#endif

// <q> Enables AUTOIP
// <id> lwip_autoip
#ifndef LWIP_AUTOIP
#define LWIP_AUTOIP 1
#endif

// <q> Enables SNMP
// <id> lwip_snmp
#ifndef LWIP_SNMP
#define LWIP_SNMP 0
#endif

// <q> Enables IGMP
// <id> lwip_igmp
#ifndef LWIP_IGMP
#define LWIP_IGMP 0
#endif

// <q> Enables SLIP interface
// <id> lwip_have_slipif
#ifndef LWIP_HAVE_SLIPIF
#define LWIP_HAVE_SLIPIF 0
#endif

// <q> Enables DNS
// <id> lwip_dns
#ifndef LWIP_DNS
#define LWIP_DNS 1
#endif
// <q> Enable PPP
// <id> lwip_ppp_support
#ifndef PPP_SUPPORT
#define PPP_SUPPORT 0
#endif

// <q> Enable PPPoE
// <id> lwip_pppoe_support
#ifndef PPPOE_SUPPORT
#define PPPOE_SUPPORT 0
#endif

// <q> Enable PPPoS
// <id> lwip_pppos_support
#ifndef PPPOS_SUPPORT
#define PPPOS_SUPPORT 0
#endif

// </h>

// <e> Advanced Configuration
// <id> lwip_advanced_config
#ifndef LWIP_ADVANCED_CONFIG
#define LWIP_ADVANCED_CONFIG 0
#endif

// <o> TCP Maximum segment size<0-100000>
// <i> TCP_MSS
// <i> Default: 1460
// <id> lwip_tcp_mss
#ifndef TCP_MSS
#define TCP_MSS 1460
#endif

// <o> The size of a TCP window<0-100000>
// <i> multiple of TCP_MSS
// <i> Default: 4
// <id> lwip_tcp_wnd_mul
#ifndef TCP_WND_MUL
#define TCP_WND_MUL 4
#endif

#ifndef TCP_WND
#define TCP_WND (TCP_WND_MUL * TCP_MSS)
#endif

// <o> TCP sender buffer space (bytes)<0-100000>
// <i> multiple of TCP_MSS
// <i> Default: 2
// <id> lwip_tcp_snd_buf_mul
#ifndef TCP_SND_BUF_MUL
#define TCP_SND_BUF_MUL 2
#endif

#ifndef TCP_SND_BUF
#define TCP_SND_BUF (TCP_SND_BUF_MUL * TCP_MSS)
#endif

// <o> The number of simultaneously active TCP connections<0-1000>
// <i> The number of simultaneously active TCP connections
// <i> Default: 5
// <id> lwip_memp_num_tcp_pcb
#ifndef MEMP_NUM_TCP_PCB
#define MEMP_NUM_TCP_PCB 5
#endif

// <o> the number of listening TCP connections<0-1000>
// <i> the number of listening TCP connections
// <i> Default: 8
// <id> lwip_memp_num_tcp_pcb_listen
#ifndef MEMP_NUM_TCP_PCB_LISTEN
#define MEMP_NUM_TCP_PCB_LISTEN 8
#endif

// <o> the number of simultaneously queued TCP segments<0-1000>
// <i> the number of simultaneously queued TCP segments
// <i> Default: 16
// <id> lwip_memp_num_tcp_seg
#ifndef MEMP_NUM_TCP_SEG
#define MEMP_NUM_TCP_SEG 16
#endif

// <o> Number of bytes added before the ethernet header CPU<0-100000>
// <i> Ensure alignment of payload after that header
// <i> Default:  2 can speed up 32-bit-platforms
// <id> lwip_eth_pad_size
#ifndef ETH_PAD_SIZE
#define ETH_PAD_SIZE 0
#endif

// <o> Memory alignment(Byte) of the CPU<0-8>
// <i> Memory alignment(Byte)
// <i> Default: 4 byte alignment
// <id> lwip_mem_alignment
#ifndef MEM_ALIGNMENT
#define MEM_ALIGNMENT 4
#endif

// <q> Enables application layer to hook into the IP layer itself
// <id> lwip_raw
#ifndef LWIP_RAW
#define LWIP_RAW 1
#endif

// <q> Enable interface up/down status callback
// <id> lwip_netif_status_callback
#ifndef LWIP_NETIF_STATUS_CALLBACK
#define LWIP_NETIF_STATUS_CALLBACK 1
#endif

// <q> Support callback when a netif is removed
// <id> lwip_netif_remove_callback
#ifndef LWIP_NETIF_REMOVE_CALLBACK
#define LWIP_NETIF_REMOVE_CALLBACK 0
#endif

/**
 * SYS_LIGHTWEIGHT_PROT==1: if you want inter-task protection for certain
 * critical regions during buffer allocation, deallocation and memory
 * allocation and deallocation.
 */
// <q> Enable inter-task protection for certain critical regions during buffer/memory allocation etc.
// <id> lwip_sys_lightweight_prot
#ifndef SYS_LIGHTWEIGHT_PROT
#define SYS_LIGHTWEIGHT_PROT 0
#endif

// <q> Enables Netconn API(not available when using "NO_SYS")
// <id> lwip_netconn
#ifndef LWIP_NETCONN
#define LWIP_NETCONN 0
#endif

// <q> Enables TCP/IP timeout
// <id> lwip_tcpip_timeout
#ifndef LWIP_TCPIP_TIMEOUT
#define LWIP_TCPIP_TIMEOUT 0
#endif

// <q> Enables Socket functions(not available when using "NO_SYS")
// <id> lwip_socket
#ifndef LWIP_SOCKET
#define LWIP_SOCKET 0
#endif

// <q> Enables BSD-style sockets functions(not available when using "NO_SYS")
// <id> lwip_compat_sockets
#ifndef LWIP_COMPAT_SOCKETS
#define LWIP_COMPAT_SOCKETS 0
#endif

// <q> Enables the ability to forward IP packets
// <id> lwip_ip_forward
#ifndef IP_FORWARD
#define IP_FORWARD 0
#endif

// <o> Value for Time-To-Live used by transport layers<0-255>
// <i> IP TTL
// <i> Default: 255
// <id> lwip_ip_default_ttl
#ifndef IP_DEFAULT_TTL
#define IP_DEFAULT_TTL 255
#endif

// <o> the number of IP packets simultaneously queued<0-1000>
// <i> the number of IP packets simultaneously queued
// <i> Default: 5
// <id> lwip_memp_num_reassdata
#ifndef MEMP_NUM_REASSDATA
#define MEMP_NUM_REASSDATA 5
#endif

// <o> the number of IP fragments simultaneously sent<0-1000>
// <i> the number of IP fragments simultaneously sent
// <i> Default: 15
// <id> lwip_memp_num_frag_pbuf
#ifndef MEMP_NUM_FRAG_PBUF
#define MEMP_NUM_FRAG_PBUF 15
#endif

// <o> the number of simultaneously queued outgoing packets<0-1000>
// <i> Pbuf
// <i> Default: 30
// <id> lwip_memp_num_arp_queue
#ifndef MEMP_NUM_ARP_QUEUE
#define MEMP_NUM_ARP_QUEUE 30
#endif

// <o> the number of struct netbufs<0-1000>
// <i> the number of struct netbufs
// <i> Default: 2
// <id> lwip_memp_num_netbuf
#ifndef MEMP_NUM_NETBUF
#define MEMP_NUM_NETBUF 2
#endif

// <o> the number of struct netconns<0-1000>
// <i> the number of struct netconns
// <i> Default: 4
// <id> lwip_memp_num_netconn
#ifndef MEMP_NUM_NETCONN
#define MEMP_NUM_NETCONN 4
#endif

// <o> the number of buffers in the pbuf pool<0-1000>
// <i> the number of buffers in the pbuf pool
// <i> Default: 16
// <id> lwip_pbuf_pool_size
#ifndef PBUF_POOL_SIZE
#define PBUF_POOL_SIZE 16
#endif

// <o> the number of bytes that should be allocated for a link level header<0-1000>
// <i> The default is 14, the standard value for Ethernet.
// <i> Default: 14
// <id> lwip_pbuf_link_hlen_s
#ifndef PBUF_LINK_HLEN_S
#define PBUF_LINK_HLEN_S 14
#endif

#define PBUF_LINK_HLEN (PBUF_LINK_HLEN_S + ETH_PAD_SIZE)

// <o> Extra size of each pbuf in the pbuf pool<0-1000>
// <i> The default is designed to accommodate single full size TCP frame in one pbuf.
// <i> It will include TCP_MSS, IP header, and link header, plus an extra word
// <id> lwip_pbuf_pool_bufsize_added
#ifndef PBUF_POOL_BUFSIZE_ADDED
#define PBUF_POOL_BUFSIZE_ADDED 0
#endif

#define PBUF_POOL_BUFSIZE LWIP_MEM_ALIGN_SIZE(TCP_MSS + 40 + PBUF_LINK_HLEN + PBUF_POOL_BUFSIZE_ADDED)

// <o> the number of multicast groups<0-1000>
// <i> the number of multicast groups
// <i> Default: 8
// <id> lwip_memp_num_igmp_group
#ifndef MEMP_NUM_IGMP_GROUP
#define MEMP_NUM_IGMP_GROUP 8
#endif

// <q> Using malloc C-library function instead of the lwip internal allocator
// <id> lwip_mem_libc_malloc
#ifndef MEM_LIBC_MALLOC
#define MEM_LIBC_MALLOC 0
#endif

// <q> Using mem_malloc function instead of the lwip internal allocator
// <id> lwip_memp_mem_malloc
#ifndef MEMP_MEM_MALLOC
#define MEMP_MEM_MALLOC 0
#endif

// <o> The stack size used by the main tcpip thread<0-10000>
// <i> It is passed to sys_thread_new() when the thread is created.
// <i> The stack size value itself is platform-dependent
// <id> lwip_tcpip_thread_stacksize
#ifndef TCPIP_THREAD_STACKSIZE
#define TCPIP_THREAD_STACKSIZE 256
#endif

// <o> The priority assigned to the main tcpip thread<0-1000>
// <i> It is passed to sys_thread_new() when the thread is created.
// <i> The priority value itself is platform-dependent
// <id> lwip_tcpip_thread_prio
#ifndef TCPIP_THREAD_PRIO
#define TCPIP_THREAD_PRIO 1
#endif

// <o> The mailbox size for the tcpip thread messages<0-1000>
// <i> It is passed to sys_mbox_new() when tcpip_init is called.
// <i> The queue size value itself is platform-dependent
// <id> lwip_tcpip_mbox_size
#ifndef TCPIP_MBOX_SIZE
#define TCPIP_MBOX_SIZE 16
#endif
// <o> The mailbox size for the incoming packets on a NETCONN_RAW<0-100000>
// <i> It is passed to sys_mbox_new() when tcpip_init is called.
// <i> The queue size value itself is platform-dependent
// <id> lwip_default_raw_recvmbox_size
#ifndef DEFAULT_RAW_RECVMBOX_SIZE
#define DEFAULT_RAW_RECVMBOX_SIZE 16
#endif

// <o> The mailbox size for the incoming packets on a NETCONN_UDP<0-100000>
// <i> It is passed to sys_mbox_new() when tcpip_init is called.
// <i> The queue size value itself is platform-dependent
// <id> lwip_default_udp_recvmbox_size
#ifndef DEFAULT_UDP_RECVMBOX_SIZE
#define DEFAULT_UDP_RECVMBOX_SIZE 16
#endif

// <o> The mailbox size for the incoming packets on a NETCONN_TCP<0-1000>
// <i> It is passed to sys_mbox_new() when tcpip_init is called.
// <i> The queue size value itself is platform-dependent
// <id> lwip_default_tcp_recvmbox_size
#ifndef DEFAULT_TCP_RECVMBOX_SIZE
#define DEFAULT_TCP_RECVMBOX_SIZE 16
#endif

// <o> The mailbox size for  the incoming connections<0-1000>
// <i> It is passed to sys_mbox_new() when tcpip_init is called.
// <i> The queue size value itself is platform-dependent
// <id> lwip_default_acceptmbox_size
#ifndef DEFAULT_ACCEPTMBOX_SIZE
#define DEFAULT_ACCEPTMBOX_SIZE 16
#endif

// <q> Enables loop interface (127.0.0.1)
// <id> lwip_loopif
#ifndef LWIP_HAVE_LOOPIF
#define LWIP_HAVE_LOOPIF 0
#endif

// <q> Enables statistics collection in lwip_stats
// <id> lwip_stats
#ifndef LWIP_STATS
#define LWIP_STATS 0
#endif

// <q> Compile in the statistics output functions
// <id> lwip_stats_display
#ifndef LWIP_STATS_DISPLAY
#define LWIP_STATS_DISPLAY 0
#endif

// <q> Compile in the statistics output functions
// <id> lwip_link_stats
#ifndef LINK_STATS
#define LINK_STATS 0
#endif

// <q> Enable etharp stats
// <id> lwip_etharp_stats
#ifndef ETHARP_STATS
#define ETHARP_STATS 0
#endif

// <q> Enable IP stats
// <id> lwip_ip_stats
#ifndef IP_STATS
#define IP_STATS 0
#endif

// <q> Enable IP fragmentation stats
// <id> lwip_ipfrag_stats
#ifndef IPFRAG_STATS
#define IPFRAG_STATS 0
#endif

// <q> Enable ICMP stats
// <id> lwip_icmp_stats
#ifndef ICMP_STATS
#define ICMP_STATS 0
#endif

// <q> Enable IGMP stats
// <id> lwip_igmp_stats
#ifndef IGMP_STATS
#define IGMP_STATS 0
#endif

// <q> Enable UDP stats
// <id> lwip_udp_stats
#ifndef UDP_STATS
#define UDP_STATS 0
#endif

// <q> Enable TCP stats
// <id> lwip_tcp_stats
#ifndef TCP_STATS
#define TCP_STATS 0
#endif

// <q> Enable memp.c stats
// <id> lwip_memp_stats
#ifndef MEMP_STATS
#define MEMP_STATS 0
#endif

// <q> Enable system stats
// <id> lwip_sys_stats
#ifndef SYS_STATS
#define SYS_STATS 0
#endif

// <q> Disable LwIP Assert
// <id> lwip_assert
#ifndef LWIP_NOASSERT
#define LWIP_NOASSERT 0
#endif

// <y> Debug level
//    <LWIP_DBG_LEVEL_ALL"> All
//    <LWIP_DBG_LEVEL_WARNING"> warning
//    <LWIP_DBG_LEVEL_SERIOUS"> serious
//    <LWIP_DBG_LEVEL_SEVERE"> severe
// <i> Default LWIP_DBG_LEVEL_ALL
// <id> lwip_dbg_min_level
#ifndef LWIP_DBG_MIN_LEVEL
#define LWIP_DBG_MIN_LEVEL LWIP_DBG_LEVEL_ALL
#endif

// <y> ARP Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_etharp_debug
#ifndef ETHARP_DEBUG
#define ETHARP_DEBUG LWIP_DBG_OFF
#endif

// <y> Netif Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_netif_debug
#ifndef NETIF_DEBUG
#define NETIF_DEBUG LWIP_DBG_OFF
#endif

// <y> Pbuf Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_api_lib_debug
#ifndef API_LIB_DEBUG
#define API_LIB_DEBUG LWIP_DBG_OFF
#endif

// <y> Pbuf Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_pbuf_debug
#ifndef PBUF_DEBUG
#define PBUF_DEBUG LWIP_DBG_OFF
#endif

// <y> API message Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_api_msg_debug
#ifndef API_MSG_DEBUG
#define API_MSG_DEBUG LWIP_DBG_OFF
#endif

// <y> Socket Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_sockets_debug
#ifndef SOCKETS_DEBUG
#define SOCKETS_DEBUG LWIP_DBG_OFF
#endif

// <y> ICMP Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_icmp_debug
#ifndef ICMP_DEBUG
#define ICMP_DEBUG LWIP_DBG_OFF
#endif

// <y> IGMP message Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_igmp_debug
#ifndef IGMP_DEBUG
#define IGMP_DEBUG LWIP_DBG_OFF
#endif

// <y> INET message Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_inet_debug
#ifndef INET_DEBUG
#define INET_DEBUG LWIP_DBG_OFF
#endif

// <y> IP Frag message Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_ip_reass_debug
#ifndef IP_REASS_DEBUG
#define IP_REASS_DEBUG LWIP_DBG_OFF
#endif

// <y> IP message Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_ip_debug
#ifndef IP_DEBUG
#define IP_DEBUG LWIP_DBG_OFF
#endif

// <y> RAW message Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_raw_debug
#ifndef RAW_DEBUG
#define RAW_DEBUG LWIP_DBG_OFF
#endif

// <y> MEM message Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_mem_debug
#ifndef MEM_DEBUG
#define MEM_DEBUG LWIP_DBG_OFF
#endif

// <y> MEMP message Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_memp_debug
#ifndef MEMP_DEBUG
#define MEMP_DEBUG LWIP_DBG_OFF
#endif

// <y> System message Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_sys_debug
#ifndef SYS_DEBUG
#define SYS_DEBUG LWIP_DBG_OFF
#endif

// <y> Timer message Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_timers_debug
#ifndef TIMERS_DEBUG
#define TIMERS_DEBUG LWIP_DBG_OFF
#endif

// <y> TCP message Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_tcp_debug
#ifndef TCP_DEBUG
#define TCP_DEBUG LWIP_DBG_OFF
#endif

// <y> TCP input Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_tcp_input_debug
#ifndef TCP_INPUT_DEBUG
#define TCP_INPUT_DEBUG LWIP_DBG_OFF
#endif

// <y> TCP in for fast retransmit Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_tcp_fr_debug
#ifndef TCP_FR_DEBUG
#define TCP_FR_DEBUG LWIP_DBG_OFF
#endif

// <y> TCP for retransmit Debug option
//     <LWIP_DBG_ON"> Enable debug message
//     <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_tcp_rto_debug
#ifndef TCP_RTO_DEBUG
#define TCP_RTO_DEBUG LWIP_DBG_OFF
#endif

// <y> TCP congestion window Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_tcp_cwnd_debug
#ifndef TCP_CWND_DEBUG
#define TCP_CWND_DEBUG LWIP_DBG_OFF
#endif

// <y> TCP window updating Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_tcp_wnd_debug
#ifndef TCP_WND_DEBUG
#define TCP_WND_DEBUG LWIP_DBG_OFF
#endif

// <y> TCP out Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_tcp_output_debug
#ifndef TCP_OUTPUT_DEBUG
#define TCP_OUTPUT_DEBUG LWIP_DBG_OFF
#endif

// <y> TCP RST Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_tcp_rst_debug
#ifndef TCP_RST_DEBUG
#define TCP_RST_DEBUG LWIP_DBG_OFF
#endif

// <y> TCP queue lengths Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_tcp_qlen_debug
#ifndef TCP_QLEN_DEBUG
#define TCP_QLEN_DEBUG LWIP_DBG_OFF
#endif

// <y> UDP Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_udp_debug
#ifndef UDP_DEBUG
#define UDP_DEBUG LWIP_DBG_OFF
#endif

// <y> TCP/ip Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_tcpip_debug
#ifndef TCPIP_DEBUG
#define TCPIP_DEBUG LWIP_DBG_OFF
#endif

// <y> PPP Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_ppp_debug
#ifndef PPP_DEBUG
#define PPP_DEBUG LWIP_DBG_OFF
#endif

// <y> SLIP Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_slip_debug
#ifndef SLIP_DEBUG
#define SLIP_DEBUG LWIP_DBG_OFF
#endif

// <y> DHCP Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_dhcp_debug
#ifndef DHCP_DEBUG
#define DHCP_DEBUG LWIP_DBG_OFF
#endif

// <y> AUTOIP Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_autoip_debug
#ifndef AUTOIP_DEBUG
#define AUTOIP_DEBUG LWIP_DBG_OFF
#endif

// <y> SNMP Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_snmp_msg_debug
#ifndef SNMP_MSG_DEBUG
#define SNMP_MSG_DEBUG LWIP_DBG_OFF
#endif

// <y> SNMP MIBs Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_snmp_mib_debug
#ifndef SNMP_MIB_DEBUG
#define SNMP_MIB_DEBUG LWIP_DBG_OFF
#endif

// <y> DNS Debug option
//    <LWIP_DBG_ON"> Enable debug message
//    <LWIP_DBG_OFF"> Disable debug message
// <i> Default LWIP_DBG_OFF
// <id> lwip_dns_debug
#ifndef DNS_DEBUG
#define DNS_DEBUG LWIP_DBG_OFF
#endif

// </e>

// <<< end of configuration section >>>

#endif // LWIPOPTS_H
