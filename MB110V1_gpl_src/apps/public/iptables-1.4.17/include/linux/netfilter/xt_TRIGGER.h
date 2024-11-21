#ifndef _XT_TRIGGER_H_target
#define _XT_TRIGGER_H_target

#include <linux/types.h>

#define TRIGGER_TIMEOUT 600	/* 600 secs */

enum xt_trigger_type
{
	XT_TRIGGER_DNAT = 1,
	XT_TRIGGER_IN = 2,
	XT_TRIGGER_OUT = 3
};

struct xt_trigger_ports {
	__u16 mport[2];	/* Related destination port range */
	__u16 rport[2];	/* Port range to map related destination port range to */
};

struct xt_trigger_info {
	enum xt_trigger_type type;
	__u16 proto;	/* Related protocol */
	struct xt_trigger_ports ports;
};

#endif /*_IPT_TRIGGER_H_target*/
