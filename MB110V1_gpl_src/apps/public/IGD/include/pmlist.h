#ifndef _PMLIST_H_
	#define _PMLIST_H_

#define COMMAND_LEN 500
#define DEST_LEN 100

/* 0 :不删除FORWARD_UPNP链中的重复条目
 * 1:删除FORWARD_UPNP链中的重复条目
 */
#if 0 
#define PATCH_MUTI_FORWARD_UPNP_LIST
#endif

/* Record current entry count of port mapping.Add by Chen Zexian, 20121121 */
#define MAX_ENTRY_COUNT	64
#define IPSTR_LEN 16

extern int curEntryCount;
typedef enum
{
	ENTRY_OPT_ADD = 0,
	ENTRY_OPT_DEL = 1,
	ENTRY_OPT_FLUSH = 2,
}ENTRY_OPT;
/* End of add, Chen Zexian */

typedef struct ExpirationEvent {
  int eventId;
  char DevUDN[NAME_SIZE];
  char ServiceID[NAME_SIZE];
  struct portMap *mapping;
} expiration_event;

struct portMap
{
  int m_PortMappingEnabled;
  long int m_PortMappingLeaseDuration;
  char m_RemoteHost[16];
  char m_ExternalPort[6];
  char m_InternalPort[6];
  char m_PortMappingProtocol[4];
  char m_InternalClient[16];
  char m_PortMappingDescription[50];

  int expirationEventId;
  long int expirationTime;

  struct portMap* next;
  struct portMap* prev;
} *pmlist_Head, *pmlist_Tail, *pmlist_Current;

//struct portMap* pmlist_NewNode(void);
struct portMap* pmlist_NewNode(int enabled, long int duration, char *remoteHost,
			       char *externalPort, char *internalPort, 
			       char *protocol, char *internalClient, char *desc);

struct portMap* pmlist_Find(char *externalPort, char *proto, char *internalClient);
struct portMap* pmlist_FindByIndex(int index);
struct portMap* pmlist_FindSpecific(char *externalPort, char *protocol);
int pmlist_IsEmtpy(void);
int pmlist_Size(void);
int pmlist_FreeList(void);
int pmlist_PushBack(struct portMap* item);
int pmlist_Delete(struct portMap** item);
int pmlist_AddPortMapping (int enabled, char *protocol,
			   char *externalPort, char *internalClient, char *internalPort);
int pmlist_DeletePortMapping(int enabled, char *protocol, 
			     char *externalPort, char *internalClient, char *internalPort);
int pmlist_FlushPortMapping();
extern int getIpAddress(char *iface_name, char *ip_addr, int len);

#ifdef PATCH_MUTI_FORWARD_UPNP_LIST

int pmlist_DeletePortMapping_PeroutingList(int enabled, char *protocol, char *externalPort, char *internalClient, char *internalPort);
int pmlist_DeletePortMapping_ForwardList(int enabled, char *protocol, char *externalPort, char *internalClient, char *internalPort);

int pmlist_AddPortMapping_PeroutingList(int enabled, char *protocol, char *externalPort, char *internalClient, char *internalPort);
int pmlist_AddPortMapping_ForwardList(int enabled, char *protocol, char *externalPort, char *internalClient, char *internalPort);
int pmlist_MutiUser_IptablesForwardList(struct portMap * SearchFrom, int maxAssert, char *InternalPort, char *proto, char *internalClient);
#endif

#endif // _PMLIST_H_
