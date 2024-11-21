#ifndef _IPT_TRIGGER_H
#define _IPT_TRIGGER_H

#include <linux/netfilter/xt_TRIGGER.h>

enum ipt_trigger_type
{
	IPT_TRIGGER_DNAT = 1,
	IPT_TRIGGER_IN = 2,
	IPT_TRIGGER_OUT = 3
};

#define ipt_trigger_info xt_trigger_info

#endif /*_IPT_TRIGGER_H*/
