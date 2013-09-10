#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/vfs.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#define INT_TO_ADDR(_addr) \
(_addr & 0xFF), \
(_addr >> 8 & 0xFF), \
(_addr >> 16 & 0xFF), \
(_addr >> 24 & 0xFF)

#define ADDR_TO_INT(a0, a1, a2, a3) \
((a0 & 0xFF        ) | \
 ((a1 & 0xFF) << 8 ) | \
 ((a2 & 0xFF) << 16) | \
 ((a3 & 0xFF) << 24))

#define AR_TO_HWADDR(_ar) \
(int)(_ar[0] & 0xFF), \
(int)(_ar[1] & 0xFF), \
(int)(_ar[2] & 0xFF), \
(int)(_ar[3] & 0xFF), \
(int)(_ar[4] & 0xFF), \
(int)(_ar[5] & 0xFF)

static const int kBufSize = 8192;

static int msg_seq = 0;
static int pid;

static int EthRouteReceive(int hndl, char *buf, int buf_size, int rpid) {
	struct nlmsghdr *t_msg    = (struct nlmsghdr*)buf;
  int              read_len = 0;
  int              msg_len  = 0;
  do {
		if ((read_len = recv(hndl, buf, buf_size - msg_len, 0)) < 0) {
			printf("Read From Socket Failed...\n");
			return -1;
		}
		if (NLMSG_OK(t_msg, read_len) == 0) {
			printf("Error while reading: NLMSG_OK(t_msg, %d)\n", read_len);
			return -1;
		}
	  printf("  receiving : msg_len: %d;\n", t_msg->nlmsg_len);
		if (t_msg->nlmsg_type == NLMSG_ERROR) {
			struct nlmsgerr *err = (struct nlmsgerr*)NLMSG_DATA(t_msg);
			if (err->error == 0) {
				printf("Got ACK!\n");
				break;
			}
			printf("Error while reading: %d\n", err->error);
			return err->error;
		}
		msg_len += read_len;
		buf     += read_len;
    if (t_msg->nlmsg_type == NLMSG_DONE ||
        (t_msg->nlmsg_flags & NLM_F_MULTI) == 0)
			break;
	} while ((t_msg->nlmsg_seq != msg_seq) || (t_msg->nlmsg_pid != rpid));
	printf("Receiving finished: %d\n", read_len);
	return read_len;
}

static int EthRouteParse(int hndl, struct nlmsghdr *t_msg, int msg_len) {
	struct nlmsghdr *f_msg = t_msg;
	for (; NLMSG_OK(t_msg, msg_len); t_msg = NLMSG_NEXT(t_msg, msg_len)) {
		struct rtmsg *rt_msg = (struct rtmsg*)NLMSG_DATA(t_msg);
		if ((rt_msg->rtm_family != AF_INET) ||
		    (rt_msg->rtm_table  != RT_TABLE_MAIN))
			continue;
		/* get the rtattr field */
		struct rtattr *rt_attr   = (struct rtattr *)RTM_RTA(rt_msg);
		int            rt_len    = RTM_PAYLOAD(t_msg);
		int            dest_addr = 0;
		char           ifname[IF_NAMESIZE];
//		printf("\t from: %p; offs: %p; msg_len: %d;\n", (void*)f_msg, (void*)t_msg, msg_len);
		for (; RTA_OK(rt_attr, rt_len); rt_attr = RTA_NEXT(rt_attr, rt_len)) {
//			printf("\t type: %d\n", rt_attr->rta_type);
			switch (rt_attr->rta_type) {
				case RTA_OIF:
					if (dest_addr == 0) {
						const int kOIf = *(int *)RTA_DATA(rt_attr);
						if_indextoname(kOIf, ifname);
						printf("\t iface  : %s [%d]\n", ifname, kOIf);
					}
					break;
				case RTA_GATEWAY:
					if (dest_addr == 0)
						printf("\t gateway: %d.%d.%d.%d\n", INT_TO_ADDR(*(u_int *)RTA_DATA(rt_attr)) );
					break;
				case RTA_PREFSRC:
//					printf("\t sorce  : %d.%d.%d.%d\n", INT_TO_ADDR(*(u_int *)RTA_DATA(rt_attr)) );
					break;
				case RTA_DST:
					dest_addr = *(u_int *)RTA_DATA(rt_attr);
//					printf("\t dest   : %d.%d.%d.%d\n", INT_TO_ADDR(dest_addr));
					break;
				default: break;
			}
		}
	}
}

static void EthRouteGet(int hndl) {
	printf("Getting of routing table:\n");
	char             buf[kBufSize];
	char             rbuf[kBufSize];
  struct nlmsghdr *t_msg  = (struct nlmsghdr*)buf;
  struct nlmsghdr *rx_msg = (struct nlmsghdr*)rbuf;
  struct rtmsg    *rt_msg = (struct rtmsg*)NLMSG_DATA(t_msg);
    
  memset(buf,  0, kBufSize);
  memset(rbuf, 0, kBufSize);
  
  t_msg->nlmsg_len   = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
  t_msg->nlmsg_type  = RTM_GETROUTE;                       // Get the routes from kernel routing table .
  t_msg->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;         // The message is a request for dump.
  t_msg->nlmsg_seq   = ++msg_seq;                          // Sequence of the message packet.
  t_msg->nlmsg_pid   = pid;                                // PID of process sending the request.

	rt_msg->rtm_dst_len  = 0;
	rt_msg->rtm_src_len  = 0;
  rt_msg->rtm_family   = AF_INET;
  rt_msg->rtm_table    = RT_TABLE_MAIN;
  rt_msg->rtm_protocol = RTPROT_BOOT;
  rt_msg->rtm_scope    = RT_SCOPE_UNIVERSE;
  rt_msg->rtm_type     = RTN_UNICAST;
    
	if (send(hndl, t_msg, t_msg->nlmsg_len, 0) < 0) {
    printf("Write To Socket Failed...\n");
    return;
  }
	const int kMsgLen = EthRouteReceive(hndl, rbuf, kBufSize, pid);
	if (kMsgLen < 1)
		return;
	EthRouteParse(hndl, rx_msg, kMsgLen);
}

int AddAttr32(struct nlmsghdr *t_msg, int type, __u32 data) {
	const int kDataSize = sizeof(data);
	const int kLen      = sizeof(struct rtattr) + kDataSize;
  struct rtmsg  *rt_msg = (struct rtmsg*)NLMSG_DATA(t_msg);
  struct rtattr *rta    = (struct rtattr*)RTM_RTA(rt_msg);
//  struct rtattr *rta    = (struct rtattr*)(t_msg + t_msg->nlmsg_len);
//  printf("  attr: %d: offs: %p; size: %d;\n", type, (void*)rta, kLen);
  rta->rta_type = type;
  rta->rta_len  = kLen;
  memcpy(RTA_DATA(rta), &data, kDataSize);
  t_msg->nlmsg_len += NLMSG_ALIGN(kLen);
//  printf("  attr: msg_len: %d;\n", t_msg->nlmsg_len);
  return 0;
}

static void EthRouteAdd(int hndl, int gw) {
	printf("Adding to routing table: %d.%d.%d.%d\n", INT_TO_ADDR(gw));
  const int kDst = 0;
  const int kSrc = 0;
  
 	char             buf[kBufSize];
	char             rbuf[kBufSize];
  struct nlmsghdr *t_msg  = (struct nlmsghdr *)buf;
  struct rtmsg    *rt_msg = (struct rtmsg*)NLMSG_DATA(t_msg);

  memset(buf,  0, kBufSize);
  memset(rbuf, 0, kBufSize);
  
  t_msg->nlmsg_len   = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
  t_msg->nlmsg_type  = RTM_NEWROUTE;                       // Set the routes to kernel routing table .
  t_msg->nlmsg_flags = NLM_F_REQUEST | NLM_F_REPLACE/* | NLM_F_ACK*/;
  t_msg->nlmsg_seq   = ++msg_seq;                          // Sequence of the message packet.
  t_msg->nlmsg_pid   = 0;                                  // PID of process sending the request.

//	rt_msg->rtm_flags    = 0;
	rt_msg->rtm_dst_len  = 0;
	rt_msg->rtm_src_len  = 0;
  rt_msg->rtm_family   = AF_INET;
  rt_msg->rtm_table    = RT_TABLE_MAIN;
  rt_msg->rtm_protocol = RTPROT_BOOT;
  rt_msg->rtm_scope    = RT_SCOPE_UNIVERSE;
  rt_msg->rtm_type     = RTN_UNICAST;

//  AddAttr32(t_msg, RTA_DST,     kDst);
//  AddAttr32(t_msg, RTA_SRC,     kSrc);
  AddAttr32(t_msg, RTA_GATEWAY, gw);
  
	if (send(hndl, t_msg, t_msg->nlmsg_len, 0) < 0) {
    printf("Write To Socket Failed...\n");
    return;
  }
//	EthRouteReceive(hndl, rbuf, kBufSize, 0);
}

static void EthRouteDel(int hndl, int gw) {
	printf("Deleting from routing table: %d.%d.%d.%d\n", INT_TO_ADDR(gw));
  const int kDst = 0;
  const int kSrc = 0;
  
 	char             buf[kBufSize];
 	char             rbuf[kBufSize];
  struct nlmsghdr *t_msg  = (struct nlmsghdr *)buf;
  struct rtmsg    *rt_msg = (struct rtmsg*)NLMSG_DATA(t_msg);
  
  memset(buf,  0, kBufSize);
  memset(rbuf, 0, kBufSize);
  
  t_msg->nlmsg_len   = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
  t_msg->nlmsg_type  = RTM_DELROUTE;                       // Set the routes to kernel routing table .
  t_msg->nlmsg_flags = NLM_F_REQUEST/* | NLM_F_ACK*/;
  t_msg->nlmsg_seq   = ++msg_seq;                          // Sequence of the message packet.
  t_msg->nlmsg_pid   = 0;                                  // PID of process sending the request.

//	rt_msg->rtm_flags    = 0;
	rt_msg->rtm_dst_len  = 0;
	rt_msg->rtm_src_len  = 0;
//	rt_msg->rtm_tos      = 0;
  rt_msg->rtm_family   = AF_INET;
  rt_msg->rtm_table    = RT_TABLE_MAIN;
  rt_msg->rtm_protocol = RTPROT_BOOT;
  rt_msg->rtm_scope    = RT_SCOPE_UNIVERSE;
  rt_msg->rtm_type     = RTN_UNICAST;

//  AddAttr32(t_msg, RTA_DST,     kDst);
//  AddAttr32(t_msg, RTA_SRC,     kSrc);
  AddAttr32(t_msg, RTA_GATEWAY, gw);
  
	if (send(hndl, t_msg, t_msg->nlmsg_len, 0) < 0) {
    printf("Write To Socket Failed...\n");
    return;
  }
//	EthRouteReceive(hndl, rbuf, kBufSize, 0);
}

static void EthRoute(const char *ifname) {
	int hndl = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE); // route table
	if (hndl < 1) {
  	printf("Socket error: %d\n", hndl);
  	return;
  }
  int gw_0 = 0x0505A8C0;
  int gw_1 = 0x0105A8C0;
//  int gw = 0x0100000A;
//  EthRouteDel(hndl, gw_0);
//  EthRouteGet(hndl);
  EthRouteGet(hndl);
//  EthRouteAdd(hndl, gw_0);
//  EthRouteAdd(hndl, gw_0);
//  EthRouteGet(hndl);
  /*
  EthRouteDel(hndl, gw_1);
  EthRouteGet(hndl);
  EthRouteAdd(hndl, gw_0);
  EthRouteGet(hndl);
  */
	close(hndl);
}

static void EthInfo() {
	struct ifconf ifc;
  struct ifreq  ifr[10];
  int hndl = socket(PF_INET, SOCK_DGRAM, 0); // common info
  if (hndl < 1) {
  	printf("socket error: %d\n", hndl);
  	return;
  }
	ifc.ifc_len           = sizeof(ifr);
  ifc.ifc_ifcu.ifcu_buf = (caddr_t)ifr;
  ioctl(hndl, SIOCGIFCONF, &ifc);
  int ifc_num = ifc.ifc_len / sizeof(struct ifreq);
  int id = 0;
  for (; id < ifc_num; id++) {
		int addr    = ((struct sockaddr_in *)(&ifr[id].ifr_addr))->sin_addr.s_addr;
		// HW adddr
		ioctl(hndl, SIOCGIFHWADDR, &ifr[id]);
		char hwaddr[14];
		memcpy(hwaddr, ((struct sockaddr *)(&ifr[id].ifr_hwaddr))->sa_data, sizeof(char) * 14);
		// Broadcast
		ioctl(hndl, SIOCGIFBRDADDR, &ifr[id]);
		int bcast   = ((struct sockaddr_in *)(&ifr[id].ifr_broadaddr))->sin_addr.s_addr;
		// Net mask
		ioctl(hndl, SIOCGIFNETMASK, &ifr[id]);
		int mask    = ((struct sockaddr_in *)(&ifr[id].ifr_netmask))->sin_addr.s_addr;
		// Destination addr
		ioctl(hndl, SIOCGIFDSTADDR, &ifr[id]);
		int dstaddr = ((struct sockaddr_in *)(&ifr[id].ifr_dstaddr))->sin_addr.s_addr;
		// Net addr
		int net     = addr & mask;
/*
		printf("DEBUG: id: %d; name: %s; hw: %X:%X:%X:%X:%X:%X; addr: %d.%d.%d.%d; bcast: %d.%d.%d.%d;\
 mask: %d.%d.%d.%d; net: %d.%d.%d.%d; dst: %d.%d.%d.%d;\n", id,
	                                       ifr[id].ifr_name,
	                                       AR_TO_HWADDR(hwaddr),
		                                     INT_TO_ADDR(addr),
		                                     INT_TO_ADDR(bcast),
		                                     INT_TO_ADDR(mask),
		                                     INT_TO_ADDR(net),
		                                     INT_TO_ADDR(dstaddr));
*/
		printf("DEBUG: id: %d; name: %s; hw: %X:%X:%X:%X:%X:%X; addr: %d.%d.%d.%d; " \
           "mask: %d.%d.%d.%d; net: %d.%d.%d.%d;\n", id,
	                                                   ifr[id].ifr_name,
	                                                   AR_TO_HWADDR(hwaddr),
		                                                 INT_TO_ADDR(addr),
		                                                 INT_TO_ADDR(mask),
		                                                 INT_TO_ADDR(net));
  }
  printf("DEBUG: setting up new IP address: 192.168.5.222\n");
  ((struct sockaddr_in *)(&ifr[1].ifr_addr))->sin_addr.s_addr = ADDR_TO_INT(192, 168, 5, 222);
	ioctl(hndl, SIOCSIFADDR, &ifr[1]);
  close(hndl);
}

static void FsInfo(char *path) {
	struct statfs info;
	if (statfs(path, &info) != 0)
		return;
		
	printf("DEBUG: f_bsize: %d; f_blocks: %d; f_bfree: %d; f_bavail: %d\n",
	       info.f_bsize,
	       info.f_blocks,
	       info.f_bfree,
	       info.f_bavail);
}

int main(int argc, char *argv[]) {
	pid = getpid();
	/*if (argc != 2) {
		fprintf(stderr, "Usage: %s <pathname>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if (stat(argv[1], &sb) == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	}
	FsInfo(argv[1]);
	*/
	EthRoute("eth0");
	EthInfo();
	return 0;
}
