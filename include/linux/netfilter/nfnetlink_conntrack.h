#ifndef _IPCONNTRACK_NETLINK_H
#define _IPCONNTRACK_NETLINK_H
#include <linux/netfilter/nfnetlink.h>

enum cntl_msg_types {
	IPCTNL_MSG_CT_NEW,
	IPCTNL_MSG_CT_GET,
	IPCTNL_MSG_CT_DELETE,
	IPCTNL_MSG_CT_GET_CTRZERO,

	IPCTNL_MSG_MAX
};

enum ctnl_exp_msg_types {
	IPCTNL_MSG_EXP_NEW,
	IPCTNL_MSG_EXP_GET,
	IPCTNL_MSG_EXP_DELETE,

	IPCTNL_MSG_EXP_MAX
};


enum ctattr_type {
	CTA_UNSPEC,
	CTA_TUPLE_ORIG,
	CTA_TUPLE_REPLY,
	CTA_STATUS,
	CTA_PROTOINFO,
	CTA_HELP,
	CTA_NAT_SRC,
#define CTA_NAT	CTA_NAT_SRC	/* backwards compatibility */
	CTA_TIMEOUT,
	CTA_MARK,
	CTA_COUNTERS_ORIG,
	CTA_COUNTERS_REPLY,
	CTA_USE,
	CTA_ID,
	CTA_NAT_DST,
	CTA_TUPLE_MASTER,
	CTA_NAT_SEQ_ADJ_ORIG,
	CTA_NAT_SEQ_ADJ_REPLY,
	CTA_SECMARK,
	__CTA_MAX
};
#define CTA_MAX (__CTA_MAX - 1)

enum ctattr_tuple {
	CTA_TUPLE_UNSPEC,
	CTA_TUPLE_IP,
	CTA_TUPLE_PROTO,
	__CTA_TUPLE_MAX
};
#define CTA_TUPLE_MAX (__CTA_TUPLE_MAX - 1)

enum ctattr_ip {
	CTA_IP_UNSPEC,
	CTA_IP_V4_SRC,
	CTA_IP_V4_DST,
	CTA_IP_V6_SRC,
	CTA_IP_V6_DST,
	__CTA_IP_MAX
};
#define CTA_IP_MAX (__CTA_IP_MAX - 1)

enum ctattr_l4proto {
	CTA_PROTO_UNSPEC,
	CTA_PROTO_NUM,
	CTA_PROTO_SRC_PORT,
	CTA_PROTO_DST_PORT,
	CTA_PROTO_ICMP_ID,
	CTA_PROTO_ICMP_TYPE,
	CTA_PROTO_ICMP_CODE,
	CTA_PROTO_ICMPV6_ID,
	CTA_PROTO_ICMPV6_TYPE,
	CTA_PROTO_ICMPV6_CODE,
	__CTA_PROTO_MAX
};
#define CTA_PROTO_MAX (__CTA_PROTO_MAX - 1)

enum ctattr_protoinfo {
	CTA_PROTOINFO_UNSPEC,
	CTA_PROTOINFO_TCP,
	CTA_PROTOINFO_DCCP,
	CTA_PROTOINFO_SCTP,
	__CTA_PROTOINFO_MAX
};
#define CTA_PROTOINFO_MAX (__CTA_PROTOINFO_MAX - 1)

enum ctattr_protoinfo_tcp {
	CTA_PROTOINFO_TCP_UNSPEC,
	CTA_PROTOINFO_TCP_STATE,
	CTA_PROTOINFO_TCP_WSCALE_ORIGINAL,
	CTA_PROTOINFO_TCP_WSCALE_REPLY,
	CTA_PROTOINFO_TCP_FLAGS_ORIGINAL,
	CTA_PROTOINFO_TCP_FLAGS_REPLY,
	__CTA_PROTOINFO_TCP_MAX
};
#define CTA_PROTOINFO_TCP_MAX (__CTA_PROTOINFO_TCP_MAX - 1)

enum ctattr_protoinfo_dccp {
	CTA_PROTOINFO_DCCP_UNSPEC,
	CTA_PROTOINFO_DCCP_STATE,
	__CTA_PROTOINFO_DCCP_MAX,
};
#define CTA_PROTOINFO_DCCP_MAX (__CTA_PROTOINFO_DCCP_MAX - 1)

enum ctattr_protoinfo_sctp {
	CTA_PROTOINFO_SCTP_UNSPEC,
	CTA_PROTOINFO_SCTP_STATE,
	CTA_PROTOINFO_SCTP_VTAG_ORIGINAL,
	CTA_PROTOINFO_SCTP_VTAG_REPLY,
	__CTA_PROTOINFO_SCTP_MAX
};
#define CTA_PROTOINFO_SCTP_MAX (__CTA_PROTOINFO_SCTP_MAX - 1)

enum ctattr_counters {
	CTA_COUNTERS_UNSPEC,
	CTA_COUNTERS_PACKETS,		/* 64bit counters */
	CTA_COUNTERS_BYTES,		/* 64bit counters */
	CTA_COUNTERS32_PACKETS,		/* old 32bit counters, unused */
	CTA_COUNTERS32_BYTES,		/* old 32bit counters, unused */
	__CTA_COUNTERS_MAX
};
#define CTA_COUNTERS_MAX (__CTA_COUNTERS_MAX - 1)

enum ctattr_nat {
	CTA_NAT_UNSPEC,
	CTA_NAT_MINIP,
	CTA_NAT_MAXIP,
	CTA_NAT_PROTO,
	__CTA_NAT_MAX
};
#define CTA_NAT_MAX (__CTA_NAT_MAX - 1)

enum ctattr_protonat {
	CTA_PROTONAT_UNSPEC,
	CTA_PROTONAT_PORT_MIN,
	CTA_PROTONAT_PORT_MAX,
	__CTA_PROTONAT_MAX
};
#define CTA_PROTONAT_MAX (__CTA_PROTONAT_MAX - 1)

enum ctattr_natseq {
	CTA_NAT_SEQ_CORRECTION_POS,
	CTA_NAT_SEQ_OFFSET_BEFORE,
	CTA_NAT_SEQ_OFFSET_AFTER,
	__CTA_NAT_SEQ_MAX
};
#define CTA_NAT_SEQ_MAX (__CTA_NAT_SEQ_MAX - 1)

enum ctattr_expect {
	CTA_EXPECT_UNSPEC,
	CTA_EXPECT_MASTER,
	CTA_EXPECT_TUPLE,
	CTA_EXPECT_MASK,
	CTA_EXPECT_TIMEOUT,
	CTA_EXPECT_ID,
	CTA_EXPECT_HELP_NAME,
	__CTA_EXPECT_MAX
};
#define CTA_EXPECT_MAX (__CTA_EXPECT_MAX - 1)

enum ctattr_help {
	CTA_HELP_UNSPEC,
	CTA_HELP_NAME,
	__CTA_HELP_MAX
};
#define CTA_HELP_MAX (__CTA_HELP_MAX - 1)

#endif /* _IPCONNTRACK_NETLINK_H */
