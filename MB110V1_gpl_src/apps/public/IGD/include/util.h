#ifndef _UTIL_H_
#define _UTIL_H_

int get_sockfd(void);
void getRandomPrefix(char *mac, char *prefix);
int getMacAddress(char *ifname, char *mac);
int GetIpAddressStr(char *address, char *ifname);
void trace(int debuglevel, const char *format, ...);

#endif //_UTIL_H_
