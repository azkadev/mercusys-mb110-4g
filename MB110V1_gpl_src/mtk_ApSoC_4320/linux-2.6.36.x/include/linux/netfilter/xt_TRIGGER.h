#ifndef _XT_TRIGGER_H_target
#define _XT_TRIGGER_H_target

#define TRIGGER_TIMEOUT 600	/* 600 secs */

enum xt_trigger_type
{
	XT_TRIGGER_DNAT = 1,
	XT_TRIGGER_IN = 2,
	XT_TRIGGER_OUT = 3
};

struct xt_trigger_ports {
	u_int16_t mport[2];	/* Related destination port range */
	u_int16_t rport[2];	/* Port range to map related destination port range to */
};

struct xt_trigger_info {
	enum xt_trigger_type type;
	u_int16_t proto;	/* Related protocol */
	struct xt_trigger_ports ports;
};

#endif /*_IPT_TRIGGER_H_target*/
