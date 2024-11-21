#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <sys/file.h>
#include <fcntl.h>
#include <ixml.h>
#include <string.h>
#include <time.h>
#include <upnp.h>
#include <upnptools.h>
#include <TimerThread.h>
#include <arpa/inet.h>
#include "globals.h"
#include "gatedevice.h"
#include "pmlist.h"
#include "util.h"
#include "globals.h"

// IGD Device Globals
UpnpDevice_Handle deviceHandle = 0;
char *gateUDN;
long int startup_time;

// State Variables
char ConnectionType[50];
char PossibleConnectionTypes[50];
char ConnectionStatus[20];
long int StartupTime;
char LastConnectionError[35];
long int AutoDisconnectTime;
long int IdleDisconnectTime;
long int WarnDisconnectDelay;
int RSIPAvailable;
int NATEnabled;
char ExternalIPAddress[20];
int PortMappingNumberOfEntries;
int PortMappingEnabled;


static int defaultAdvrExpire = 300;
extern int upnp_alreadyRunning;

/* Added by LI CHENGLONG , 2011-Nov-11.*/
int exIntfChangedHandler(CMSG_BUFF *pCmsgBuff);
/* Ended by LI CHENGLONG , 2011-Nov-11.*/

//Definitions for mapping expiration timer thread
static TimerThread gExpirationTimerThread;
static ThreadPool gExpirationThreadPool;
extern TimerThread gTimerThread;

/*  by lcl, 05May11 */
static char L3F_DefaultConnectionService[50]={"WANIPConnection"}; 

// MUTEX for locking shared state variables whenver they are changed
ithread_mutex_t DevMutex = PTHREAD_MUTEX_INITIALIZER;

static char wanIPconnUUID[] = "uuid:9f0865b3-f5da-4ad5-85b7-7404637fdf39";
// Main event handler for callbacks from the SDK.  Determine type of event
// and dispatch to the appropriate handler (Note: Get Var Request deprecated
int EventHandler(Upnp_EventType EventType, void *Event, void *Cookie)
{
	switch (EventType)
	{
		case UPNP_EVENT_SUBSCRIPTION_REQUEST:
			HandleSubscriptionRequest((struct Upnp_Subscription_Request *) Event);
			break;
		// -- Deprecated --
		case UPNP_CONTROL_GET_VAR_REQUEST:
			HandleGetVarRequest((struct Upnp_State_Var_Request *) Event);
			break;
		case UPNP_CONTROL_ACTION_REQUEST:
			HandleActionRequest((struct Upnp_Action_Request *) Event);
			break;
		default:
			trace(1, "Error in EventHandler: Unknown event type %d",
						EventType);
	}
	return (0);
}

// Grab our UDN from the Description Document.  This may not be needed, 
// the UDN comes with the request, but we leave this for other device initializations
int StateTableInit(char *descDocUrl)
{
	int ret = UPNP_E_SUCCESS;
	gateUDN = wanIPconnUUID;
		
	// Initialize our linked list of port mappings.
	pmlist_Head = pmlist_Current = NULL;
	PortMappingNumberOfEntries = 0;

	return (ret);
}

// Handles subscription request for state variable notifications
int HandleSubscriptionRequest(struct Upnp_Subscription_Request *sr_event)
{
	IXML_Document *propSet = NULL;
	
	ithread_mutex_lock(&DevMutex);

	/*debug ,Added by LI CHENGLONG, 2012 Mar 22 10:44:36 AM*/
#if 0
	if (strcmp(sr_event->UDN, gateUDN) == 0)
	{
#endif 
		/* need fix for l3f and wfc by lcl, 05May11 */

		
		// WAN Common Interface Config Device Notifications
		if (strcmp(sr_event->ServiceId, "urn:upnp-org:serviceId:WANCommonIFC1") == 0)
		{
		        trace(3, "Recieved request to subscribe to WANCommonIFC1");
			UpnpAddToPropertySet(&propSet, "PhysicalLinkStatus", "Up");
			UpnpAcceptSubscriptionExt(deviceHandle, sr_event->UDN, sr_event->ServiceId,
						  propSet, sr_event->Sid);
			ixmlDocument_free(propSet);
		}
		// WAN IP Connection Device Notifications
		else if (strcmp(sr_event->ServiceId, "urn:upnp-org:serviceId:WANIPConn1") == 0)
		{
			GetIpAddressStr(ExternalIPAddress, g_vars.extInterfaceName);
			trace(3, "Received request to subscribe to WANIPConn1");
			UpnpAddToPropertySet(&propSet, "PossibleConnectionTypes","IP_Routed");
			UpnpAddToPropertySet(&propSet, "ConnectionStatus","Connected");
			UpnpAddToPropertySet(&propSet, "ExternalIPAddress", ExternalIPAddress);
			UpnpAddToPropertySet(&propSet, "PortMappingNumberOfEntries","0");
			UpnpAcceptSubscriptionExt(deviceHandle, sr_event->UDN, sr_event->ServiceId,
						  propSet, sr_event->Sid);
			ixmlDocument_free(propSet);
		}
#if 0
	}
#endif 

	ithread_mutex_unlock(&DevMutex);
	return(1);
}

int HandleGetVarRequest(struct Upnp_State_Var_Request *gv_request)
{
	// GET VAR REQUEST DEPRECATED FROM UPnP SPECIFICATIONS 
	// Report this in debug and ignore requests.  If anyone experiences problems
	// please let us know.
        trace(3, "Deprecated Get Variable Request received. Ignoring.");
	return 1;
}

int SetDefaultConnectionService(struct Upnp_Action_Request *ca_event)
{

	char   *conn_service=NULL;
	char resultStr[RESULT_LEN];
	
   if ((conn_service = GetFirstDocumentItem(ca_event->ActionRequest, "NewDefaultConnectionService")))
   	{
   		if (strncmp(L3F_DefaultConnectionService, conn_service, 50) == 0)
   		{
			ca_event->ErrCode = UPNP_E_SUCCESS;
			snprintf(resultStr, RESULT_LEN, "<u:%sResponse xmlns:u=\"%s\">\n%s\n</u:%sResponse>",
		ca_event->ActionName, "urn:schemas-upnp-org:service:L3Forwarding:1", "", ca_event->ActionName);
			ca_event->ActionResult = ixmlParseBuffer(resultStr);
		 }
		else
		{
   			  trace(1, "Failiure in SetDefaultConnectionService: Invalid Arguments!");	
   			  ca_event->ErrCode = 402;
			  strcpy(ca_event->ErrStr, "Invalid Args");
			  ca_event->ActionResult = NULL;
		}
	}
   else
   	{
   			  trace(1, "Failiure in SetDefaultConnectionService: Invalid Arguments!");	
   			  ca_event->ErrCode = 402;
			  strcpy(ca_event->ErrStr, "Invalid Args");
			  ca_event->ActionResult = NULL;
		
   	}

       if (conn_service) free(conn_service);

	return (ca_event->ErrCode);
		
  }

/*lcl*/
int GetDefaultConnectionService(struct Upnp_Action_Request *ca_event)
{
	char resultStr[RESULT_LEN];
	IXML_Document *result;

	snprintf(resultStr, RESULT_LEN,
		"<u:GetDefaultConnectionServiceResponse xmlns:u=\"urn:schemas-upnp-org:service:L3Forwarding:1\">\n"
		"<NewDefaultConnectionService>%s</NewDefaultConnectionService>\n"
		"</u:GetDefaultConnectionServiceResponse>",L3F_DefaultConnectionService);

   // Create a IXML_Document from resultStr and return with ca_event
   if ((result = ixmlParseBuffer(resultStr)) != NULL)
   {
      ca_event->ActionResult = result;
      ca_event->ErrCode = UPNP_E_SUCCESS;
   }
   else
   {
      trace(1, "Error parsing Response to GetDefaultConnectionService: %s", resultStr);
      ca_event->ActionResult = NULL;

      ca_event->ErrCode = 402;
   }

	return(ca_event->ErrCode);
}

int HandleActionRequest(struct Upnp_Action_Request *ca_event)
{
	int result = 0;

	ithread_mutex_lock(&DevMutex);

	if (0 == upnp_alreadyRunning)/* by Chen Zexian, 20121126 */
	{
		ithread_mutex_unlock(&DevMutex);
		trace(1, "IGD has not been in service!\n");
		return 0;
	}
		
#if 0
	if (strcmp(ca_event->DevUDN, gateUDN) == 0)
	{
#endif 
		// Common debugging info, hopefully gets removed soon.
		
		/*  by lcl, 05May11 */
		if (strcmp(ca_event->ServiceID, "urn:upnp-org:serviceId:L3Forwarding1") == 0)
		{
			if (strcmp(ca_event->ActionName, "SetDefaultConnectionService") == 0)
			{
				result = SetDefaultConnectionService(ca_event);	
			}
			else if (strcmp(ca_event->ActionName, "GetDefaultConnectionService") == 0)
			{
				result = GetDefaultConnectionService(ca_event);
			}
			else 
			{
				trace(1, "Invalid Action Request : %s",ca_event->ActionName);
				result = InvalidAction(ca_event);
			}
		}
		/*  by lcl, 05May11 */
		else if (strcmp(ca_event->ServiceID, "urn:upnp-org:serviceId:WANIPConn1") == 0)
		{
			if (strcmp(ca_event->ActionName,"GetConnectionTypeInfo") == 0)
			  result = GetConnectionTypeInfo(ca_event);
			else if (strcmp(ca_event->ActionName,"GetNATRSIPStatus") == 0)
			  result = GetNATRSIPStatus(ca_event);
			else if (strcmp(ca_event->ActionName,"SetConnectionType") == 0)
			  result = SetConnectionType(ca_event);
			else if (strcmp(ca_event->ActionName,"RequestConnection") == 0)
			  result = RequestConnection(ca_event);
			else if (strcmp(ca_event->ActionName,"AddPortMapping") == 0)
			  result = AddPortMapping(ca_event);
			else if (strcmp(ca_event->ActionName,"GetGenericPortMappingEntry") == 0)
			  result = GetGenericPortMappingEntry(ca_event);
			else if (strcmp(ca_event->ActionName,"GetSpecificPortMappingEntry") == 0)
			  result = GetSpecificPortMappingEntry(ca_event);
			else if (strcmp(ca_event->ActionName,"GetExternalIPAddress") == 0)
			  result = GetExternalIPAddress(ca_event);
			else if (strcmp(ca_event->ActionName,"DeletePortMapping") == 0)
			  result = DeletePortMapping(ca_event);
			else if (strcmp(ca_event->ActionName,"GetStatusInfo") == 0)
			  result = GetStatusInfo(ca_event);
	
			// Intentionally Non-Implemented Functions -- To be added later
			/*else if (strcmp(ca_event->ActionName,"RequestTermination") == 0)
				result = RequestTermination(ca_event);
			else if (strcmp(ca_event->ActionName,"ForceTermination") == 0)
				result = ForceTermination(ca_event);
			else if (strcmp(ca_event->ActionName,"SetAutoDisconnectTime") == 0)
				result = SetAutoDisconnectTime(ca_event);
			else if (strcmp(ca_event->ActionName,"SetIdleDisconnectTime") == 0)
				result = SetIdleDisconnectTime(ca_event);
			else if (strcmp(ca_event->ActionName,"SetWarnDisconnectDelay") == 0)
				result = SetWarnDisconnectDelay(ca_event);
			else if (strcmp(ca_event->ActionName,"GetAutoDisconnectTime") == 0)
				result = GetAutoDisconnectTime(ca_event);
			else if (strcmp(ca_event->ActionName,"GetIdleDisconnectTime") == 0)
				result = GetIdleDisconnectTime(ca_event);
			else if (strcmp(ca_event->ActionName,"GetWarnDisconnectDelay") == 0)
				result = GetWarnDisconnectDelay(ca_event);*/
			else result = InvalidAction(ca_event);
		}
		else if (strcmp(ca_event->ServiceID,"urn:upnp-org:serviceId:WANCommonIFC1") == 0)
		{
			if (strcmp(ca_event->ActionName,"GetTotalBytesSent") == 0)
				result = GetTotal(ca_event, STATS_TX_BYTES);
			else if (strcmp(ca_event->ActionName,"GetTotalBytesReceived") == 0)
				result = GetTotal(ca_event, STATS_RX_BYTES);
			else if (strcmp(ca_event->ActionName,"GetTotalPacketsSent") == 0)
				result = GetTotal(ca_event, STATS_TX_PACKETS);
			else if (strcmp(ca_event->ActionName,"GetTotalPacketsReceived") == 0)
				result = GetTotal(ca_event, STATS_RX_PACKETS);
			else if (strcmp(ca_event->ActionName,"GetCommonLinkProperties") == 0)
				result = GetCommonLinkProperties(ca_event);
			else 
			{
				trace(1, "Invalid Action Request : %s",ca_event->ActionName);
				result = InvalidAction(ca_event);
			}
		} 
#if 0
	}
#endif 
	
	ithread_mutex_unlock(&DevMutex);

	return (result);
}

// Default Action when we receive unknown Action Requests
int InvalidAction(struct Upnp_Action_Request *ca_event)
{
        ca_event->ErrCode = 401;
        strcpy(ca_event->ErrStr, "Invalid Action");
        ca_event->ActionResult = NULL;
        return (ca_event->ErrCode);
}

// As IP_Routed is the only relevant Connection Type for Linux-IGD
// we respond with IP_Routed as both current type and only type
int GetConnectionTypeInfo (struct Upnp_Action_Request *ca_event)
{
	char resultStr[RESULT_LEN];
	IXML_Document *result;

	snprintf(resultStr, RESULT_LEN,
		"<u:GetConnectionTypeInfoResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">\n"
		"<NewConnectionType>IP_Routed</NewConnectionType>\n"
		"<NewPossibleConnectionTypes>IP_Routed</NewPossibleConnectionTypes>"
		"</u:GetConnectionTypeInfoResponse>");

   // Create a IXML_Document from resultStr and return with ca_event
   if ((result = ixmlParseBuffer(resultStr)) != NULL)
   {
      ca_event->ActionResult = result;
      ca_event->ErrCode = UPNP_E_SUCCESS;
   }
   else
   {
      trace(1, "Error parsing Response to GetConnectionTypeinfo: %s", resultStr);
      ca_event->ActionResult = NULL;
      ca_event->ErrCode = 402;
   }

	return(ca_event->ErrCode);
}





// Linux-IGD does not support RSIP.  However NAT is of course
// so respond with NewNATEnabled = 1
int GetNATRSIPStatus(struct Upnp_Action_Request *ca_event)
{
   char resultStr[RESULT_LEN];
	IXML_Document *result;

   snprintf(resultStr, RESULT_LEN, "<u:GetNATRSIPStatusResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">\n"
      							"<NewRSIPAvailable>0</NewRSIPAvailable>\n"
									"<NewNATEnabled>1</NewNATEnabled>\n"
								"</u:GetNATRSIPStatusResponse>");

	// Create a IXML_Document from resultStr and return with ca_event
	if ((result = ixmlParseBuffer(resultStr)) != NULL)
	{
		ca_event->ActionResult = result;
		ca_event->ErrCode = UPNP_E_SUCCESS;	
	}
   else
	{
	        trace(1, "Error parsing Response to GetNATRSIPStatus: %s", resultStr);
		ca_event->ActionResult = NULL;
		ca_event->ErrCode = 402;
	}

	return(ca_event->ErrCode);
}


// Connection Type is a Read Only Variable as linux-igd is only
// a device that supports a NATing IP router (not an Ethernet
// bridge).  Possible other uses may be explored.
int SetConnectionType(struct Upnp_Action_Request *ca_event)
{
	// Ignore requests
	ca_event->ActionResult = NULL;
	ca_event->ErrCode = UPNP_E_SUCCESS;
	return ca_event->ErrCode;
}

// This function should set the state variable ConnectionStatus to
// connecting, and then return synchronously, firing off a thread
// asynchronously to actually change the status to connected.  However, here we
// assume that the external WAN device is configured and connected
// outside of linux igd.
int RequestConnection(struct Upnp_Action_Request *ca_event)
{
	
	IXML_Document *propSet = NULL;
	
	//Immediatley Set connectionstatus to connected, and lastconnectionerror to none.
	strcpy(ConnectionStatus,"Connected");
	strcpy(LastConnectionError, "ERROR_NONE");
	trace(2, "RequestConnection recieved ... Setting Status to %s.", ConnectionStatus);

	// Build DOM Document with state variable connectionstatus and event it
	UpnpAddToPropertySet(&propSet, "ConnectionStatus", ConnectionStatus);
	
	// Send off notifications of state change
	UpnpNotifyExt(deviceHandle, ca_event->DevUDN, ca_event->ServiceID, propSet);

	/* bugFixed: memory leak. Chen Zexian, 20130909 */
	ixmlDocument_free(propSet);

	ca_event->ErrCode = UPNP_E_SUCCESS;
	return ca_event->ErrCode;
}


int GetCommonLinkProperties(struct Upnp_Action_Request *ca_event)
{
   char resultStr[RESULT_LEN];
	IXML_Document *result;
        
	ca_event->ErrCode = UPNP_E_SUCCESS;
	snprintf(resultStr, RESULT_LEN,
		"<u:GetCommonLinkPropertiesResponse xmlns:u=\"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1\">\n"
		"<NewWANAccessType>Cable</NewWANAccessType>\n"
		"<NewLayer1UpstreamMaxBitRate>%s</NewLayer1UpstreamMaxBitRate>\n"
		"<NewLayer1DownstreamMaxBitRate>%s</NewLayer1DownstreamMaxBitRate>\n"
		"<NewPhysicalLinkStatus>Up</NewPhysicalLinkStatus>\n"
		"</u:GetCommonLinkPropertiesResponse>",g_vars.upstreamBitrate,g_vars.downstreamBitrate);

   // Create a IXML_Document from resultStr and return with ca_event
   if ((result = ixmlParseBuffer(resultStr)) != NULL)
   {
      ca_event->ActionResult = result;
      ca_event->ErrCode = UPNP_E_SUCCESS;
   }
   else
   {
      trace(1, "Error parsing Response to GetCommonLinkProperties: %s", resultStr);
      ca_event->ActionResult = NULL;
      ca_event->ErrCode = 402;
   }

	return(ca_event->ErrCode);
}

/* get specified statistic from /proc/net/dev */
int GetTotal(struct Upnp_Action_Request *ca_event, stats_t stat)
{
	char dev[IFNAMSIZ], resultStr[RESULT_LEN];
	const char *methods[STATS_LIMIT] =
		{ "BytesSent", "BytesReceived", "PacketsSent", "PacketsReceived" };
	unsigned long stats[STATS_LIMIT];
	FILE *proc;
	IXML_Document *result;
	int read;
	
	proc = fopen("/proc/net/dev", "r");
	if (!proc)
	{
		fprintf(stderr, "failed to open\n");
		return 0;
	}

	/* skip first two lines */
	fscanf(proc, "%*[^\n]\n%*[^\n]\n");

	/* parse stats */
	do
		read = fscanf(proc, "%[^:]:%lu %lu %*u %*u %*u %*u %*u %*u %lu %lu %*u %*u %*u %*u %*u %*u\n", dev, &stats[STATS_RX_BYTES], &stats[STATS_RX_PACKETS], &stats[STATS_TX_BYTES], &stats[STATS_TX_PACKETS]);
	while (read != EOF && (read == 5 && strncmp(dev, g_vars.extInterfaceName, IFNAMSIZ) != 0));

	fclose(proc);

	snprintf(resultStr, RESULT_LEN,
		"<u:GetTotal%sResponse xmlns:u=\"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1\">\n"
		"<NewTotal%s>%lu</NewTotal%s>\n"
		"</u:GetTotal%sResponse>", 
		methods[stat], methods[stat], stats[stat], methods[stat], methods[stat]);

	// Create a IXML_Document from resultStr and return with ca_event
	if ((result = ixmlParseBuffer(resultStr)) != NULL)
	{
		ca_event->ActionResult = result;
		ca_event->ErrCode = UPNP_E_SUCCESS;
	}
	else
	{
		trace(1, "Error parsing response to GetTotal: %s", resultStr);
		ca_event->ActionResult = NULL;
		ca_event->ErrCode = 402;
	}

	return (ca_event->ErrCode);
}

// Returns connection status related information to the control points
int GetStatusInfo(struct Upnp_Action_Request *ca_event)
{
   long int uptime;
   char resultStr[RESULT_LEN];
	IXML_Document *result = NULL;

   uptime = (time(NULL) - startup_time);
   
	snprintf(resultStr, RESULT_LEN,
		"<u:GetStatusInfoResponse xmlns:u=\"urn:schemas-upnp-org:service:GetStatusInfo:1\">\n"
		"<NewConnectionStatus>Connected</NewConnectionStatus>\n"
		"<NewLastConnectionError>ERROR_NONE</NewLastConnectionError>\n"
		"<NewUptime>%li</NewUptime>\n"
		"</u:GetStatusInfoResponse>", 
		uptime);
   
	// Create a IXML_Document from resultStr and return with ca_event
   if ((result = ixmlParseBuffer(resultStr)) != NULL)
   {
      ca_event->ActionResult = result;
      ca_event->ErrCode = UPNP_E_SUCCESS;
   }
   else
   {
     trace(1, "Error parsing Response to GetStatusInfo: %s", resultStr);
      ca_event->ActionResult = NULL;
      ca_event->ErrCode = 402;
   }

   return(ca_event->ErrCode);
}

// Add New Port Map to the IGD
int AddPortMapping(struct Upnp_Action_Request *ca_event)
{
	char *remote_host=NULL;
	char *ext_port=NULL;
	char *proto=NULL;
	char *int_port=NULL;
	char *int_ip=NULL;
	char *int_duration=NULL;
	char *bool_enabled=NULL;
	char *desc=NULL;
  	struct portMap *ret, *new;
	int result = 0;
	char num[5]; // Maximum number of port mapping entries 9999
	IXML_Document *propSet = NULL;
	int action_succeeded = 0;
	char resultStr[RESULT_LEN];
	int ext_port_num=0;
	int ftp_ctrl_port=0;
	int n=0;
	FILE *fp=NULL;
	char bufFile[16];
	struct OpenVpnStr openvpnStr;
	struct CwmpStr cwmpStr;
	if (curEntryCount >= MAX_ENTRY_COUNT)
	{
		trace(1, "Entry count has been maximum.curEntryCount=%d\n", curEntryCount);
		return (ca_event->ErrCode = UPNP_E_OUTOF_HANDLE);
	}
	
	if ( (ext_port = GetFirstDocumentItem(ca_event->ActionRequest, "NewExternalPort") )
	     && (proto = GetFirstDocumentItem(ca_event->ActionRequest, "NewProtocol") )
	     && (int_port = GetFirstDocumentItem(ca_event->ActionRequest, "NewInternalPort") )
	     && (int_ip = GetFirstDocumentItem(ca_event->ActionRequest, "NewInternalClient") )
	     && (int_duration = GetFirstDocumentItem(ca_event->ActionRequest, "NewLeaseDuration") )
	     && (bool_enabled = GetFirstDocumentItem(ca_event->ActionRequest, "NewEnabled") )
	     && (desc = GetFirstDocumentItem(ca_event->ActionRequest, "NewPortMappingDescription") ))
	{
		/* Add by Chen Zexian to validate parametres , 20130627 */
		/* validate the IP address */
		struct in_addr req_addr;
		int a = 0, b = 0, c = 0, d = 0;
		if (0 == inet_aton(int_ip, &req_addr))
		{
			ca_event->ErrCode = UPNP_E_INTERNAL_ERROR;
			strcpy(ca_event->ErrStr, "Internal Error");
	  		ca_event->ActionResult = NULL;
			goto end;
		}
		a = req_addr.s_addr >> 24;
		b = (req_addr.s_addr  << 8) >> 24;
		c = (req_addr.s_addr  << 16) >> 24;
		d = (req_addr.s_addr  << 24) >> 24;
		if (a > 255 || b > 255 || c > 255 || d > 255 ||a < 0 || b < 0 || c < 0 || d < 0)
		{
			ca_event->ErrCode = UPNP_E_INVALID_PARAM;
			strcpy(ca_event->ErrStr, "Invalid Args");
	  		ca_event->ActionResult = NULL;
			goto end;
		}

		/* validate the ports */
		a = atoi(ext_port);
		b = atoi(int_port);
		if (a > 65535 || b > 65535 ||a < 1 || b < 1)
		{
			ca_event->ErrCode = UPNP_E_INVALID_PARAM;
			strcpy(ca_event->ErrStr, "Invalid Args");
	  		ca_event->ActionResult = NULL;
			goto end;
		}
		
		/* truncate description if it's too long */
		if (strlen(desc) >= 32)
		{	
			desc[31] = 0;
		}
		/* End of add */
	
		#ifdef INCLUDE_USB_FTP_SERVER
		/* Add by chz to avoid port conflict with FTP. 2012-12-21 */
		ext_port_num = atoi(ext_port);
		fp = fopen(FTP_PORT_FILE, "r");
		if (fp == NULL)
		{
			trace(1, "Failiure in GateDeviceAddPortMapping: Open file error:%d", errno);
			ca_event->ErrCode = errno;
		    strcpy(ca_event->ErrStr, strerror(errno));
		    ca_event->ActionResult = NULL;

			goto end;
		}

		memset(bufFile, 0, sizeof(bufFile));
		n = fread(bufFile, 1, sizeof(bufFile), fp);
		fclose(fp);
		if (n == 0)
		{
			trace(1, "Failiure in GateDeviceAddPortMapping: Read file error:%d", errno);
			ca_event->ErrCode = errno;
		    strcpy(ca_event->ErrStr, strerror(errno));
		    ca_event->ActionResult = NULL;

			goto end;
		}

		ftp_ctrl_port = atoi(bufFile + 3);
		if ((!strncmp(bufFile, "11", 2) && (ftp_ctrl_port == ext_port_num)) ||
			((59990 <= ext_port_num) && (ext_port_num <= 59999)))
		{
			trace(1, "Failiure in GateDeviceAddPortMapping: Port conflict with FTP!");
		    trace(1, "  ExtPort: %s Proto: %s IntPort: %s IntIP: %s Dur: %s Ena: %s Desc: %s",
			ext_port, proto, int_port, int_ip, int_duration, bool_enabled, desc);
		    ca_event->ErrCode = 402;
		    strcpy(ca_event->ErrStr, "Invalid Args");
		    ca_event->ActionResult = NULL;

			goto end;
		}
		/* end add */
		#endif

 	/* To check port conflict with OpenVpn Server. Linzijian add */
		ext_port_num = atoi(ext_port);
		fp = fopen(OPEN_VPN_FILE, "r");
		if (fp != NULL)   //if fp == NULL  , we will not do this check ;
		{
			memset(openvpnStr.OpenvpnProto, 0, sizeof(openvpnStr.OpenvpnProto));
			if(fread(&openvpnStr, sizeof(openvpnStr), 1, fp) == 1)//will only be one openvpn obj
			{
				if (openvpnStr.OpenvpnEnable == 1 && openvpnStr.OpenvpnPort == ext_port_num &&
					!strncmp(openvpnStr.OpenvpnProto, proto, sizeof(openvpnStr.OpenvpnProto)-1))
				{
					trace(1, "Failiure in GateDeviceAddPortMapping: Port conflict with Open VPN!");
				    trace(1, "  ExtPort: %s Proto: %s IntPort: %s IntIP: %s Dur: %s Ena: %s Desc: %s",
					ext_port, proto, int_port, int_ip, int_duration, bool_enabled, desc);
				    ca_event->ErrCode = 402;
				    strcpy(ca_event->ErrStr, "Invalid Args");
				    ca_event->ActionResult = NULL;

					goto end;
				}
			}
			fclose(fp);
		}

	/* To check port conflict with PptpVpn Server. */
		fp = fopen(PPTP_VPN_FILE, "r");
		if(fp != NULL)
		{
			unsigned char Enable = 0;	//whether PPTPVpn Server is enable
			if(fread(&Enable, sizeof(char), 1, fp) == 1)
			{
				if(Enable == 1 && ext_port_num == PPTP_VPN_PORT && !strncmp(proto, "TCP", sizeof("TCP")))
				{
					trace(1, "Failiure in GateDeviceAddPortMapping: Port conflict with Open VPN!");
					trace(1, "  ExtPort: %s Proto: %s IntPort: %s IntIP: %s Dur: %s Ena: %s Desc: %s",
					ext_port, proto, int_port, int_ip, int_duration, bool_enabled, desc);
					ca_event->ErrCode = 402;
					strcpy(ca_event->ErrStr, "Invalid Args");
				 	ca_event->ActionResult = NULL;
					goto end;
				}
			}
			fclose(fp);
		}
	/* add end. by Linzijian */

	/* To check port conflict with cwmp. */
		fp = fopen(CWMP_FILE, "r");
		if(fp != NULL)
		{
			if(fread(&cwmpStr, sizeof(cwmpStr), 1, fp) == 1)
			{
				if(cwmpStr.CwmpEnable == 1 && ext_port_num ==cwmpStr.CwmpPort)
				{
					trace(1, "Failiure in GateDeviceAddPortMapping: Port conflict with CWMP!");
					trace(1, "  ExtPort: %s Proto: %s IntPort: %s IntIP: %s Dur: %s Ena: %s Desc: %s",
					ext_port, proto, int_port, int_ip, int_duration, bool_enabled, desc);
					ca_event->ErrCode = 402;
					strcpy(ca_event->ErrStr, "Invalid Args");
				 	ca_event->ActionResult = NULL;
					goto end;
				}
			}
			fclose(fp);
		}
		
	  remote_host = GetFirstDocumentItem(ca_event->ActionRequest, "NewRemoteHost");
		// If port map with the same External Port, Protocol, and Internal Client exists
		// then, as per spec, we overwrite it (for simplicity, we delete and re-add at end of list)
		// Note: This may cause problems with GetGernericPortMappingEntry if a CP expects the overwritten
		// to be in the same place.
		if ((ret = pmlist_Find(ext_port, proto, int_ip)) != NULL)
		{
			trace(3, "Found port map to already exist.  Replacing(ext_port:%s, proto:%s, int_ip:%s)\n", ext_port, proto, int_ip);
				pmlist_Delete(&ret);
		}
		/* If the external port with the same protocol has been ocupied by other client PCs,return 718(conflict). Chen Zexian, 20130819 */
		else if ((ret = pmlist_FindSpecific(ext_port, proto)) != NULL)
		{
			trace(1,"Conflict with other mapping entries: RemoteHost: %s Prot:%s ExtPort: %s Int: %s.%s\n",
							    remote_host, proto, ext_port, int_ip, int_port);
			ca_event->ErrCode = 718;
			strcpy(ca_event->ErrStr, "ConflictInMappingEntry");
			ca_event->ActionResult = NULL;
			goto end;
		}
 
		new = pmlist_NewNode(atoi(bool_enabled), atol(int_duration), "", ext_port, int_port, proto, int_ip, desc); 

		if (new != NULL)
		{
			result = pmlist_PushBack(new);

			if (result==1)
			{
			    ScheduleMappingExpiration(new,ca_event->DevUDN,ca_event->ServiceID);
				sprintf(num, "%d", pmlist_Size());
				trace(3, "PortMappingNumberOfEntries: %d", pmlist_Size());
				UpnpAddToPropertySet(&propSet, "PortMappingNumberOfEntries", num);				
				UpnpNotifyExt(deviceHandle, ca_event->DevUDN, ca_event->ServiceID, propSet);
				ixmlDocument_free(propSet);
				trace(2, "AddPortMap: DevUDN: %s ServiceID: %s RemoteHost: %s Prot: %s ExtPort: %s Int: %s.%s",
						    ca_event->DevUDN,ca_event->ServiceID,remote_host, proto, ext_port, int_ip, int_port);
				action_succeeded = 1;
			}
			else
			{
				free(new);
			}
	 	}
		else
				{
			trace(1,"Failure to alloc memory!\n");
			ca_event->ErrCode = 501;
			strcpy(ca_event->ErrStr, "Action failed");
					ca_event->ActionResult = NULL;
			goto end;
		}
	}
	else
	{
	  
	  trace(1, "Failiure in GateDeviceAddPortMapping: Invalid Arguments!");
	  trace(1, "  ExtPort: %s Proto: %s IntPort: %s IntIP: %s Dur: %s Ena: %s Desc: %s",
		ext_port, proto, int_port, int_ip, int_duration, bool_enabled, desc);
	  ca_event->ErrCode = 402;
	  strcpy(ca_event->ErrStr, "Invalid Args");
	  ca_event->ActionResult = NULL;
	}

	if (action_succeeded)
	{
		ca_event->ErrCode = UPNP_E_SUCCESS;
		snprintf(resultStr, RESULT_LEN, "<u:%sResponse xmlns:u=\"%s\">\n%s\n</u:%sResponse>",
			ca_event->ActionName, "urn:schemas-upnp-org:service:WANIPConnection:1", "", ca_event->ActionName);
		ca_event->ActionResult = ixmlParseBuffer(resultStr);
	}
	else if (result != 718)
	{
		ca_event->ErrCode = UPNP_E_INTERNAL_ERROR;
		strcpy(ca_event->ErrStr, "Internal Error");
		ca_event->ActionResult = NULL;
	}

end:
	if (ext_port) free(ext_port);
	if (int_port) free(int_port);
	if (proto) free(proto);
	if (int_ip) free(int_ip);
	if (int_duration) free(int_duration);
	if (bool_enabled) free(bool_enabled);
	if (desc) free(desc);
	if (remote_host) free(remote_host);

	return(ca_event->ErrCode);
}

int GetGenericPortMappingEntry(struct Upnp_Action_Request *ca_event)
{
	char *mapindex = NULL;
	struct portMap *temp;
	char result_param[RESULT_LEN];
	char resultStr[RESULT_LEN];
	int action_succeeded = 0;

	if ((mapindex = GetFirstDocumentItem(ca_event->ActionRequest, "NewPortMappingIndex")))
	{
		temp = pmlist_FindByIndex(atoi(mapindex));
		if (temp)
		{
			snprintf(result_param, RESULT_LEN, "<NewRemoteHost>%s</NewRemoteHost><NewExternalPort>%s</NewExternalPort><NewProtocol>%s</NewProtocol><NewInternalPort>%s</NewInternalPort><NewInternalClient>%s</NewInternalClient><NewEnabled>%d</NewEnabled><NewPortMappingDescription>%s</NewPortMappingDescription><NewLeaseDuration>%li</NewLeaseDuration>", temp->m_RemoteHost, temp->m_ExternalPort, temp->m_PortMappingProtocol, temp->m_InternalPort, temp->m_InternalClient, temp->m_PortMappingEnabled, temp->m_PortMappingDescription, temp->m_PortMappingLeaseDuration);
			action_succeeded = 1;
		}
      if (action_succeeded)
      {
         ca_event->ErrCode = UPNP_E_SUCCESS;
                   snprintf(resultStr, RESULT_LEN, "<u:%sResponse xmlns:u=\"%s\">\n%s\n</u:%sResponse>", ca_event->ActionName,
                           "urn:schemas-upnp-org:service:WANIPConnection:1",result_param, ca_event->ActionName);
                   ca_event->ActionResult = ixmlParseBuffer(resultStr);
      }
      else
      {
         ca_event->ErrCode = 713;
			strcpy(ca_event->ErrStr, "SpecifiedArrayIndexInvalid");
			ca_event->ActionResult = NULL;
      }

   }
   else
   {
            trace(1, "Failure in GateDeviceGetGenericPortMappingEntry: Invalid Args");
            ca_event->ErrCode = 402;
                 strcpy(ca_event->ErrStr, "Invalid Args");
                 ca_event->ActionResult = NULL;
   }
	if (mapindex) free (mapindex);
	return (ca_event->ErrCode);
 	
}

int GetSpecificPortMappingEntry(struct Upnp_Action_Request *ca_event)
{
   char *ext_port=NULL;
   char *proto=NULL;
   char result_param[RESULT_LEN];
   char resultStr[RESULT_LEN];
   int action_succeeded = 0;
	struct portMap *temp;

   if ((ext_port = GetFirstDocumentItem(ca_event->ActionRequest, "NewExternalPort"))
      && (proto = GetFirstDocumentItem(ca_event->ActionRequest,"NewProtocol")))
   {
      if ((strcmp(proto, "TCP") == 0) || (strcmp(proto, "UDP") == 0))
      {
			temp = pmlist_FindSpecific (ext_port, proto);
			if (temp)
			{
				snprintf(result_param, RESULT_LEN, "<NewInternalPort>%s</NewInternalPort><NewInternalClient>%s</NewInternalClient><NewEnabled>%d</NewEnabled><NewPortMappingDescription>%s</NewPortMappingDescription><NewLeaseDuration>%li</NewLeaseDuration>",
            temp->m_InternalPort,
            temp->m_InternalClient,
            temp->m_PortMappingEnabled,
				temp->m_PortMappingDescription,
            temp->m_PortMappingLeaseDuration);
            action_succeeded = 1;
			}
         if (action_succeeded)
         {
            ca_event->ErrCode = UPNP_E_SUCCESS;
	    snprintf(resultStr, RESULT_LEN, "<u:%sResponse xmlns:u=\"%s\">\n%s\n</u:%sResponse>", ca_event->ActionName,
		    "urn:schemas-upnp-org:service:WANIPConnection:1",result_param, ca_event->ActionName);
	    ca_event->ActionResult = ixmlParseBuffer(resultStr);
         }
         else
         {
            trace(2, "GateDeviceGetSpecificPortMappingEntry: PortMapping Doesn't Exist...");
	    ca_event->ErrCode = 714;
	    strcpy(ca_event->ErrStr, "NoSuchEntryInArray");
	    ca_event->ActionResult = NULL;
         }
      }
      else
      {
          trace(1, "Failure in GateDeviceGetSpecificPortMappingEntry: Invalid NewProtocol=%s\n",proto);
	      ca_event->ErrCode = 402;
	      strcpy(ca_event->ErrStr, "Invalid Args");
	      ca_event->ActionResult = NULL;
      }
   }
   else
   {
      trace(1, "Failure in GateDeviceGetSpecificPortMappingEntry: Invalid Args");
      ca_event->ErrCode = 402;
      strcpy(ca_event->ErrStr, "Invalid Args");
      ca_event->ActionResult = NULL;
   }

   if (ext_port) free(ext_port);
   if (proto) free(proto);

   return (ca_event->ErrCode);


}

int GetExternalIPAddress(struct Upnp_Action_Request *ca_event)
{
   char resultStr[RESULT_LEN];
	IXML_Document *result = NULL;

   ca_event->ErrCode = UPNP_E_SUCCESS;
   GetIpAddressStr(ExternalIPAddress, g_vars.extInterfaceName);
   snprintf(resultStr, RESULT_LEN, "<u:GetExternalIPAddressResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">\n"
										"<NewExternalIPAddress>%s</NewExternalIPAddress>\n"
								"</u:GetExternalIPAddressResponse>", ExternalIPAddress);

   // Create a IXML_Document from resultStr and return with ca_event
   if ((result = ixmlParseBuffer(resultStr)) != NULL)
   {
      ca_event->ActionResult = result;
      ca_event->ErrCode = UPNP_E_SUCCESS;
   }
   else
   {
      trace(1, "Error parsing Response to ExternalIPAddress: %s", resultStr);
      ca_event->ActionResult = NULL;
      ca_event->ErrCode = 402;
   }

   return(ca_event->ErrCode);
}

int DeletePortMapping(struct Upnp_Action_Request *ca_event)
{
   char *ext_port=NULL;
   char *proto=NULL;
   int result=0;
   char num[5];
   char resultStr[RESULT_LEN];
   IXML_Document *propSet= NULL;
   int action_succeeded = 0;
	struct portMap *temp;

   if (((ext_port = GetFirstDocumentItem(ca_event->ActionRequest, "NewExternalPort")) &&
      (proto = GetFirstDocumentItem(ca_event->ActionRequest, "NewProtocol"))))
   {
     if ((strcmp(proto, "TCP") == 0) || (strcmp(proto, "UDP") == 0))
     {
       if ((temp = pmlist_FindSpecific(ext_port, proto)))
		 result = pmlist_Delete(&temp);

         if (result==1)
         {
            trace(2, "DeletePortMap: Proto:%s Port:%s\n",proto, ext_port);
            sprintf(num,"%d",pmlist_Size());
            UpnpAddToPropertySet(&propSet,"PortMappingNumberOfEntries", num);
            UpnpNotifyExt(deviceHandle, ca_event->DevUDN,ca_event->ServiceID,propSet);
            ixmlDocument_free(propSet);
            action_succeeded = 1;
         }
         else
         {
            trace(1, "Failure in GateDeviceDeletePortMapping: DeletePortMap: Proto:%s Port:%s\n",proto, ext_port);
            ca_event->ErrCode = 714;
            strcpy(ca_event->ErrStr, "NoSuchEntryInArray");
            ca_event->ActionResult = NULL;
         }
      }
      else
      {
         trace(1, "Failure in GateDeviceDeletePortMapping: Invalid NewProtocol=%s\n",proto);
         ca_event->ErrCode = 402;
			strcpy(ca_event->ErrStr, "Invalid Args");
			ca_event->ActionResult = NULL;
      }
   }
   else
   {
		trace(1, "Failiure in GateDeviceDeletePortMapping: Invalid Arguments!");
		ca_event->ErrCode = 402;
		strcpy(ca_event->ErrStr, "Invalid Args");
		ca_event->ActionResult = NULL;
   }

   if (action_succeeded)
   {
      ca_event->ErrCode = UPNP_E_SUCCESS;
      snprintf(resultStr, RESULT_LEN, "<u:%sResponse xmlns:u=\"%s\">\n%s\n</u:%sResponse>",
         ca_event->ActionName, "urn:schemas-upnp-org:service:WANIPConnection:1", "", ca_event->ActionName);
      ca_event->ActionResult = ixmlParseBuffer(resultStr);
   }

   if (ext_port) free(ext_port);
   if (proto) free(proto);

   return(ca_event->ErrCode);
}

// From sampleutil.c included with libupnp 
char* GetFirstDocumentItem( IN IXML_Document * doc,
                                 IN const char *item )
{
    IXML_NodeList *nodeList = NULL;
    IXML_Node *textNode = NULL;
    IXML_Node *tmpNode = NULL;

    char *ret = NULL;

    nodeList = ixmlDocument_getElementsByTagName( doc, ( char * )item );

    if( nodeList ) {
        if( ( tmpNode = ixmlNodeList_item( nodeList, 0 ) ) ) {
            textNode = ixmlNode_getFirstChild( tmpNode );
       if (textNode != NULL)
       {
      ret = strdup( ixmlNode_getNodeValue( textNode ) );
       }
        }
    }

    if( nodeList )
        ixmlNodeList_free( nodeList );
    return ret;
}

int ExpirationTimerThreadInit(void)
{
  int retVal;
  ThreadPoolAttr attr;
  TPAttrInit( &attr );
  TPAttrSetMaxThreads( &attr, MAX_THREADS );
  TPAttrSetMinThreads( &attr, MIN_THREADS );
  TPAttrSetJobsPerThread( &attr, JOBS_PER_THREAD );
  TPAttrSetIdleTime( &attr, THREAD_IDLE_TIME );

#if (EXCLUDE_G_EXPIRATIONTHREADPOOL == 0)
  if( ThreadPoolInit( &gExpirationThreadPool, &attr ) != UPNP_E_SUCCESS ) {
    return UPNP_E_INIT_FAILED;
  }

  if( ( retVal = TimerThreadInit( &gExpirationTimerThread,
				  &gExpirationThreadPool ) ) !=
      UPNP_E_SUCCESS ) {
    return retVal;
  }
#endif /* (EXCLUDE_G_EXPIRATIONTHREADPOOL == 0) */

 return 0;
}

int ExpirationTimerThreadShutdown(void)
{
#if (EXCLUDE_G_EXPIRATIONTHREADPOOL == 0)	/*let gTimerThread control this.*/
  	return TimerThreadShutdown(&gExpirationTimerThread);
#endif /* (EXCLUDE_G_EXPIRATIONTHREADPOOL == 0) */

	return 0;
}


void free_expiration_event(expiration_event *event)
{
  if (event->mapping!=NULL)
    event->mapping->expirationEventId = -1;
  free(event);
}

void ExpireMapping(void *input)
{
  char num[5]; // Maximum number of port mapping entries 9999
  IXML_Document *propSet = NULL;
  expiration_event *event = ( expiration_event * ) input;

  ithread_mutex_lock(&DevMutex);

  trace(2, "ExpireMapping: Proto:%s Port:%s\n",
		      event->mapping->m_PortMappingProtocol, event->mapping->m_ExternalPort);

  //reset the event id before deleting the mapping so that pmlist_Delete
  //will not call CancelMappingExpiration
  event->mapping->expirationEventId = -1;
  if (1 == pmlist_Delete(&(event->mapping)))
  {
  	trace(2, "ExpireMapping success.\n");
  }
  else
  {
  	trace(1, "Entry has been deleted before expiring!!But this should not been an error.\n");
  }
  sprintf(num, "%d", pmlist_Size());
  UpnpAddToPropertySet(&propSet, "PortMappingNumberOfEntries", num);
  UpnpNotifyExt(deviceHandle, event->DevUDN, event->ServiceID, propSet);
  ixmlDocument_free(propSet);
  trace(3, "ExpireMapping: UpnpNotifyExt(deviceHandle,%s,%s,propSet)\n  PortMappingNumberOfEntries: %s",
		      event->DevUDN, event->ServiceID, num);
  
  free_expiration_event(event);
  
  ithread_mutex_unlock(&DevMutex);
}

int ScheduleMappingExpiration(struct portMap *mapping, char *DevUDN, char *ServiceID)
{
  int retVal = 0;
  ThreadPoolJob job;
  expiration_event *event;
  time_t curtime = time(NULL);
	
  if (mapping->m_PortMappingLeaseDuration > 0) {
    mapping->expirationTime = curtime + mapping->m_PortMappingLeaseDuration;
  }
  else {
    //client did not provide a duration, so use the default duration
    if (g_vars.duration==0) {
      return 1; //no default duration set
    }
    else if (g_vars.duration>0) {
      //relative duration
      mapping->expirationTime = curtime+g_vars.duration;
    }
    else { //g_vars.duration < 0
      //absolute daily expiration time
      long int expclock = -1*g_vars.duration;
      struct tm *loctime = localtime(&curtime);
      long int curclock = loctime->tm_hour*3600 + loctime->tm_min*60 + loctime->tm_sec;
      long int diff = expclock-curclock;
      if (diff<60) //if exptime is in less than a minute (or in the past), schedule it in 24 hours instead
	diff += 24*60*60;
      mapping->expirationTime = curtime+diff;
    }
  }

  event = ( expiration_event * ) malloc( sizeof( expiration_event ) );
  if( event == NULL ) {
    return 0;
  }
  event->mapping = mapping;
  if (strlen(DevUDN) < sizeof(event->DevUDN)) strcpy(event->DevUDN, DevUDN);
  else strcpy(event->DevUDN, "");
  if (strlen(ServiceID) < sizeof(event->ServiceID)) strcpy(event->ServiceID, ServiceID);
  else strcpy(event->ServiceID, "");
  
  TPJobInit( &job, ( start_routine ) ExpireMapping, event );
  TPJobSetFreeFunction( &job, ( free_routine ) free_expiration_event );
 	/*use gTimerThread instead, Added by LI CHENGLONG , 2011-Nov-16.*/
#if (EXCLUDE_G_EXPIRATIONTHREADPOOL == 0)
  if( ( retVal = TimerThreadSchedule( &gExpirationTimerThread,
				      mapping->expirationTime,
				      ABS_SEC, &job, SHORT_TERM,
				      &( event->eventId ) ) )
      != UPNP_E_SUCCESS ) {
    free( event );
    mapping->expirationEventId = -1;
    return 0;
  }

#else
	if( ( retVal = TimerThreadSchedule( &gTimerThread,
						mapping->expirationTime,
						ABS_SEC, &job, SHORT_TERM,
						&( event->eventId ) ) )
		!= UPNP_E_SUCCESS ) {
	  free( event );
	  mapping->expirationEventId = -1;
	  return 0;
	}
#endif /* (EXCLUDE_G_EXPIRATIONTHREADPOOL == 0) */

 	/* Ended by LI CHENGLONG , 2011-Nov-16.*/
  mapping->expirationEventId = event->eventId;

  trace(3,"ScheduleMappingExpiration: DevUDN: %s ServiceID: %s Proto: %s ExtPort: %s Int: %s.%s at: %s eventId: %d",event->DevUDN,event->ServiceID,mapping->m_PortMappingProtocol, mapping->m_ExternalPort, mapping->m_InternalClient, mapping->m_InternalPort, ctime(&(mapping->expirationTime)), event->eventId);

  return event->eventId;
}

int CancelMappingExpiration(int expirationEventId)
{
  ThreadPoolJob job;
  if (expirationEventId<0)
    return 1;
  trace(3,"CancelMappingExpiration: eventId: %d",expirationEventId);
  
#if (EXCLUDE_G_EXPIRATIONTHREADPOOL == 0)
  if (TimerThreadRemove(&gExpirationTimerThread,expirationEventId,&job)==0) {
    free_expiration_event((expiration_event *)job.arg);
  }

#else

	/*use gTimerThread instead.*/
  if (TimerThreadRemove(&gTimerThread, expirationEventId,&job)==0) {
    free_expiration_event((expiration_event *)job.arg);
  }
  else {
    trace(1,"  TimerThreadRemove failed!");
  }
#endif /* (EXCLUDE_G_EXPIRATIONTHREADPOOL == 0)*/

  
  return 1;
}

void DeleteAllPortMappings_locked(void)
{
  IXML_Document *propSet = NULL;
  pmlist_FreeList();

  UpnpAddToPropertySet(&propSet, "PortMappingNumberOfEntries", "0");
  UpnpNotifyExt(deviceHandle, gateUDN, "urn:upnp-org:serviceId:WANIPConn1", propSet);
  ixmlDocument_free(propSet);
  trace(2, "DeleteAllPortMappings: UpnpNotifyExt(deviceHandle,%s,%s,propSet)\n  PortMappingNumberOfEntries: %s",
	gateUDN, "urn:upnp-org:serviceId:WANIPConn1", "0");
}

void DeleteAllPortMappings(void)
{

  ithread_mutex_lock(&DevMutex);

 DeleteAllPortMappings_locked();
 
  ithread_mutex_unlock(&DevMutex);
}

#define RENEW_ENTRY
#ifdef RENEW_ENTRY
/* 
 * fn		int pmDelFromKernel()
 * brief	将pmlist_Head 链中的所有portmapping从内核netfilter中删除掉.
 * details	将pmlist_Head 链中的所有portmapping从内核netfilter中删除掉.	
 *
 * return	int		
 * retval	0		成功
 *			-1		失败
 *
 * note	written by  03Nov11, LI CHENGLONG	
 */
int pmDelFromKernel()
{
	struct portMap *curr;

	curr = pmlist_Head;
	while (curr)
	{

#ifndef PATCH_MUTI_FORWARD_UPNP_LIST
		pmlist_DeletePortMapping(curr->m_PortMappingEnabled,
								 curr->m_PortMappingProtocol,
								 curr->m_ExternalPort,
			     				 curr->m_InternalClient,
			     				 curr->m_InternalPort
			     				);
#else

		pmlist_DeletePortMapping_PeroutingList(curr->m_PortMappingEnabled,
								 curr->m_PortMappingProtocol,
								 curr->m_ExternalPort,
			     				 curr->m_InternalClient,
			     				 curr->m_InternalPort
			     				);

		/*wanggankun@2012-11-10, may be , iptables: No chain/target/match by that name, but have no effect */
		pmlist_DeletePortMapping_ForwardList(curr->m_PortMappingEnabled,
								 curr->m_PortMappingProtocol,
								 curr->m_ExternalPort,
			     				 curr->m_InternalClient,
			     				 curr->m_InternalPort
			     				);
#endif
		curr = curr->next;
	}

	return 0;
	
}

/* 
 * fn		int pmAddToKernel()
 * brief	将pmlist_Head 链中的所有portmapping添加到内核netfilter中使之生效.	
 * details	将pmlist_Head 链中的所有portmapping添加到内核netfilter中使之生效.	
 *
 * return	int	
 * retval	0		成功
 *			-1		失败
 *
 * note	written by  03Nov11, LI CHENGLONG	
 */
int pmAddToKernel()
{
	struct portMap *curr;

	curr = pmlist_Head;
	while (curr)
	{
#ifndef PATCH_MUTI_FORWARD_UPNP_LIST
		pmlist_AddPortMapping(curr->m_PortMappingEnabled,
							  curr->m_PortMappingProtocol,
							  curr->m_ExternalPort,
		     				  curr->m_InternalClient,
		     				  curr->m_InternalPort
			     			 );
#else
		pmlist_AddPortMapping_PeroutingList(curr->m_PortMappingEnabled,
							  curr->m_PortMappingProtocol,
							  curr->m_ExternalPort,
		     				  curr->m_InternalClient,
		     				  curr->m_InternalPort
			     			 );

		if (NULL != curr->next)
		{
			/*if from next, it is no same protocol, internal port, internal client, we should add it*/
			if (pmlist_MutiUser_IptablesForwardList(curr->next, 1, curr->m_InternalPort, curr->m_PortMappingProtocol, curr->m_InternalClient) == 0)
			{
				pmlist_AddPortMapping_ForwardList(curr->m_PortMappingEnabled,
							  curr->m_PortMappingProtocol,
							  curr->m_ExternalPort,
		     				  curr->m_InternalClient,
		     				  curr->m_InternalPort
			     			 );
			}
		}
		else
		{
			/*wanggankun@2012-11-10, the last one, must be added*/
			pmlist_AddPortMapping_ForwardList(curr->m_PortMappingEnabled,
							  curr->m_PortMappingProtocol,
							  curr->m_ExternalPort,
		     				  curr->m_InternalClient,
		     				  curr->m_InternalPort
			     			 );
		}
#endif		
		curr = curr->next;
	}

	return 0;	
}
#endif /* RENEW_ENTRY */

/* 
 * fn		int exIntfChangedHandler(CMSG_BUFF *pCmsgBuff)
 * brief	外部接口改变时需要更新内核netfilter中的规则.	
 * details	外部接口改变时需要更新内核netfilter中的规则,这个函数处理更新细节.	
 *
 * param[in]	外部接口改变的消息.
 *
 * return		int
 * retval		0		成功
 *				-1		失败
 *
 * note	written by  03Nov11, LI CHENGLONG	
 * 		外部接口改变时，直接用-F 删除原有规则，尽量避免与cmm冲突.Chen Zexian,20121115
 */
int exIntfChangedHandler(CMSG_BUFF *pCmsgBuff)
{
	UPNP_DEFAULT_GW_CH_MSG *pMsg = (UPNP_DEFAULT_GW_CH_MSG *)pCmsgBuff->content;

	
	/* 
	 * brief: Added by Li Chenglong, 11-11-11.
	 *		  对默认路由添加和更新时进行处理.
	 */	
	if(pMsg->gwName[0] == '\0')
	{
#ifdef RENEW_ENTRY
		/*从旧的WAN接口上删除 Added  by  Li Chenglong , 11-11-11.*/
		pmDelFromKernel();
#else /* RENEW_ENTRY */
		DeleteAllPortMappings();
#endif /* RENEW_ENTRY */
		/*更新新的接口, Added  by  Li Chenglong , 11-11-11.*/
		memset(g_vars.extInterfaceName, 0, IFNAMSIZ);
		g_vars.natEnabled = (pMsg->natEnabled) ? TRUE:FALSE;
#ifdef RENEW_ENTRY
		pmAddToKernel();
#endif /* RENEW_ENTRY */
	}
	else if (pMsg->gwName[0] != '\0')
	{
#ifdef RENEW_ENTRY
		pmDelFromKernel();
#else /* RENEW_ENTRY */
		DeleteAllPortMappings();
#endif /* RENEW_ENTRY */

		/*更新新的接口, Added  by  Li Chenglong , 11-11-11.*/
		snprintf(g_vars.extInterfaceName, IFNAMSIZ, "%s", pMsg->gwName);
		g_vars.natEnabled = (pMsg->natEnabled) ? TRUE:FALSE;

#ifdef RENEW_ENTRY
		/*将规则添加到新的WAN接口中   by Li Chenglong*/
		pmAddToKernel();
#endif /* RENEW_ENTRY */
	}


	
	return 0;
}

/* 
 * fn		int SyncMappingDatabase()
 * brief	定时更新pm.db文件.	
 * details	注册一个定时执行函数,更新pm.db.	
 *
 *
 * return		int
 * retval		0		成功
 *				-1		失败
 *
 * note	written by  10Nov11, LI CHENGLONG	
 * 		增加互斥锁,Chen Zexian, 20121115
 */
int syncMappingDatabase()
{
	int ret = 0;
	ThreadPoolJob job;

	/*文件锁,读者,写者间同步, Added by LI CHENGLONG , 2011-Nov-09.*/
	FILE *pFWriter;
	int pmIndex;
	struct portMap *pPMEntry;
	char buff[BUFLEN_1024];
	int count;
	/* Ended by LI CHENGLONG , 2011-Nov-09.*/
	
	/* 
	* brief: Added by LI CHENGLONG, 2011-Nov-09.
	*		  pm.db 文件用于在进程间共享数据(即是实时获取端口映射列表).
	*		  该文件的结构如下:
	*
	*
	*	INDEX=0
	*	{
	*		EXTERNALPORT=6578
	*		PROTOCOL=TCP
	*		INTERNALPORT=4536
	*		INTERNALIP=192.168.1.111
	*		STATUS=1
	*		DESCRIPTION=thunder
	*	}
	*	INDEX=1
	*	{
	*		EXTERNALPORT=7637
	*		PROTOCOL=TCP
	*		INTERNALPORT=4342
	*		INTERNALIP=192.168.1.111
	*		STATUS=1
	*		DESCRIPTION=emule
	*	}
	*  ...
	*
	*  
	*/
	
	ithread_mutex_lock(&DevMutex);

	
	/*每3秒更新一次文件/var/tmp/upnpd/pm.db(UPNPD_PM_LIST_DB_FILE),
	 *Added by LI CHENGLONG , 2011-Nov-09.*/

	/* 打开文件不可用w+，因为此时还没有获得文件锁，可能有其他进程随后进行读。Chen Zexian, 20121121 */
	pFWriter = fopen(UPNPD_PM_LIST_DB_FILE, "a+");

	if (NULL != pFWriter)
	{
		flock(fileno(pFWriter), LOCK_EX);

		truncate(UPNPD_PM_LIST_DB_FILE, 0); /* 获得文件锁后才把文件截短为0。 Chen Zexian, 20121121 */
		
		pmIndex = 0;
		pPMEntry = NULL;

		while((pPMEntry = pmlist_FindByIndex(pmIndex)))
		{
			
			/*write to pm.db, Added by LI CHENGLONG , 2011-Nov-09.*/
			snprintf(buff,
					 BUFLEN_1024,
					 "INDEX=%d\n"\
					 "{\n"\
					 "EXTERNALPORT=%s\n"\
					 "PROTOCOL=%s\n"\
					 "INTERNALPORT=%s\n"\
					 "INTERNALIP=%s\n"\
					 "STATUS=%d\n"\
					 "DESCRIPTION=%s\n"\
					 "}\n",
					 pmIndex,
					 pPMEntry->m_ExternalPort,
					 pPMEntry->m_PortMappingProtocol,
					 pPMEntry->m_InternalPort,
					 pPMEntry->m_InternalClient,
					 pPMEntry->m_PortMappingEnabled > 0 ? 1 : 0,
					 pPMEntry->m_PortMappingDescription
					 );
			
			count = fwrite(buff, strlen(buff), 1, pFWriter);
			if (count != 1)
			{
				trace(1, "in syncMappingDatabase fwrite error!");
			}
			
			pmIndex++;
		}

		flock(fileno(pFWriter), LOCK_UN);
		fclose(pFWriter);
	}
	else
	{
		trace(1, "%s not exist\n", UPNPD_PM_LIST_DB_FILE);
	}

	/* Ended by LI CHENGLONG , 2011-Nov-09.*/
	
	ithread_mutex_unlock(&DevMutex);
	
	TPJobInit( &job, ( start_routine ) syncMappingDatabase, NULL);
	TPJobSetFreeFunction( &job, ( free_routine )NULL);
	TPJobSetPriority( &job, MED_PRIORITY );

	ret = TimerThreadSchedule(&gTimerThread,
							  MAPPING_DATA_BASE_SYNC_PERIOD,
							  REL_SEC,
							  &job,
							  SHORT_TERM,
							  NULL);
	
	if (ret != UPNP_E_SUCCESS)
	{
		return -1;
	}

	return 0;
}


/* 
 * fn		UBOOL32 switchOnUpnp(char *descDocUrl)
 * brief	置upnp功能为enable.	
 * details	开启upnp功能向libupnp库中注册rootdevice.
 *
 * param[in]	descDocUrl		客户端访问rootdevice描述文件的url.
 *
 * return		UBOOL32
 * retval		TRUE	开启upnp功能成功.
 *				FALSE	开启upnp功能失败.
 *
 * note	written by  11Nov11, LI CHENGLONG	
 */
UBOOL32 switchOnUpnp(char *descDocUrl)
{
	int ret;
	char path[BUFLEN_128];
	ithread_mutex_lock(&DevMutex);

#if (EXCLUDE_G_EXPIRATIONTHREADPOOL == 0)	
	/*don't use gExpirationThreadPool and gExpirationTimerThread.*/
	//initialize the timer thread for expiration of mappings
	if (ExpirationTimerThreadInit()!=0) {
	  trace(1,"ExpirationTimerInit failed");
	  UpnpFinish();
	  ithread_mutex_unlock(&DevMutex);
	  return FALSE;
	}
#endif /* (EXCLUDE_G_EXPIRATIONTHREADPOOL == 0) */


	// Form the Description Doc URL to pass to RegisterRootDevice
	/*    return miniSocket->miniServerPort；即是49152或以上端口...miniserver已经在
	监听该端口   by LiChenglong*/
	snprintf(descDocUrl, DESC_DOC_URL_LENGTH, "http://%s:%d/%s/%s", UpnpGetServerIpAddress(),
				UpnpGetServerPort(), g_vars.randomPrefix, g_vars.descDocName);

	snprintf(path, BUFLEN_128, "%s/%s/%s", g_vars.xmlPath, g_vars.randomPrefix, g_vars.descDocName);
	

		// Register our IGD as a valid UPnP Root device
	trace(3, "Registering the root device with descDocUrl %s", descDocUrl);

		
	while ((ret = UpnpRegisterRootDevice5(descDocUrl, EventHandler, &deviceHandle,
					   &deviceHandle, path)) == UPNP_E_TIMEDOUT)
	{
		trace(1, "  UpnpRegisterRootDevice returned %d(UPNP_E_TIMEDOUT).Try again.", ret);
		usleep(500);
	}

	if ( ret != UPNP_E_SUCCESS )
	{
		trace(1, "Error registering the root device with descDocUrl: %s", descDocUrl);
		trace(1, "  UpnpRegisterRootDevice returned %d", ret);
		UpnpFinish();
		ithread_mutex_unlock(&DevMutex);
		return FALSE;
	}

	
	trace(2, "IGD root device successfully registered.");
	
	// Initialize the state variable table.
	if ( (ret = StateTableInit(descDocUrl)) != UPNP_E_SUCCESS )
	{
		UpnpFinish();
		ithread_mutex_unlock(&DevMutex);
		return FALSE;
	}
	
	// Record the startup time, for uptime
	startup_time = time(NULL);
	
	// Send out initial advertisements of our device's services with timeouts of 30 minutes
	if ( (ret = UpnpSendAdvertisement(deviceHandle, defaultAdvrExpire) != UPNP_E_SUCCESS ))
	{
		trace(1, "Error Sending Advertisements.  Exiting ...");
		UpnpFinish();
		ithread_mutex_unlock(&DevMutex);
		return FALSE;
	}
	trace(2, "Advertisements Sent.  Listening for requests ... ");

	upnp_alreadyRunning = 1;

	ithread_mutex_unlock(&DevMutex);
	return TRUE;
}


/* 
 * fn		UBOOL32 switchOffUpnp()
 * brief	置upnp功能为disabled.	
 *
 * return	UBOOL32
 * retval	TRUE	disable成功
 *			FALSE	disable失败
 *
 * note	written by  11Nov11, LI CHENGLONG	
 */
UBOOL32 switchOffUpnp()
{
	FILE *pFile;
	ithread_mutex_lock(&DevMutex);
	// Cleanup UPnP SDK and free memory

	DeleteAllPortMappings_locked();
	
	ExpirationTimerThreadShutdown();

	UpnpUnRegisterRootDevice(deviceHandle);
		

 	/*将pm.db文件清空. Added by LI CHENGLONG , 2011-Nov-11.*/
	pFile = fopen(UPNPD_PM_LIST_DB_FILE, "w+");
	if (NULL != pFile)
	{
		fclose(pFile);
	}
	else
	{
		remove(UPNPD_PM_LIST_DB_FILE);
	}
 	/* Ended by LI CHENGLONG , 2011-Nov-11.*/
	upnp_alreadyRunning = 0;
	
	ithread_mutex_unlock(&DevMutex);
	return TRUE;
}
