#ifndef RA_MAC_H
#define RA_MAC_H

void ra2880stop(END_DEVICE *ei_local);
void ra2880MacAddressSet(unsigned char p[6]);
void ra2880Mac2AddressSet(unsigned char p[6]);
void ethtool_init(struct net_device *dev);

void ra2880EnableInterrupt(void);

void dump_qos(void);
void dump_reg(struct seq_file *s);
void dump_cp0(void);

int debug_proc_init(void);
void debug_proc_exit(void);

#if defined (CONFIG_RALINK_RT6855) || defined(CONFIG_RALINK_RT6855A) || \
           defined (CONFIG_RALINK_MT7620) || defined(CONFIG_RALINK_MT7621)
void enable_auto_negotiate(int unused);
#else
void enable_auto_negotiate(int ge);
#endif

void rt2880_gmac_hard_reset(void);

int TsoLenUpdate(int tso_len);
int NumOfTxdUpdate(int num_of_txd);

#ifdef CONFIG_RAETH_LRO
int LroStatsUpdate(struct net_lro_mgr *lro_mgr, bool all_flushed);
#endif
int getnext(const char *src, int separator, char *dest);
int str_to_ip(unsigned int *ip, const char *str);
#endif
