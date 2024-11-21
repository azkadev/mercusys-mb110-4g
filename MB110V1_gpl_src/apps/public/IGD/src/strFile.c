/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		strFile.c
 * brief		
 * details	
 *
 * author	LI CHENGLONG
 * version	
 * date		02Nov11
 *
 * history 	\arg  1.0.0, 02Nov11, LI CHENGLONG, Create the file.	
 */
#include "globals.h"
#include "strFile.h"
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "util.h"
 
/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/
/*设备描述xml模板,更据oem厂商最终的设备描述文件会有变化, Added by LI CHENGLONG , 2011-Nov-02.*/
static char gateDescXmlTemple[] = 
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\r\n"
"  <specVersion>\r\n"
"    <major>1</major>\r\n"
"    <minor>0</minor>\r\n"
"  </specVersion>\r\n"
"  <device>\r\n"
"    <deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType>\r\n"
"    <presentationURL>%s</presentationURL>\r\n"
"    <friendlyName>%s</friendlyName>\r\n"
"    <manufacturer>%s</manufacturer>\r\n"
"    <manufacturerURL>%s</manufacturerURL>\r\n"
"    <modelDescription>%s</modelDescription>\r\n"
"    <modelName>%s</modelName>\r\n"
"    <modelNumber>%s</modelNumber>\r\n"
"    <modelURL>%s</modelURL>\r\n"
"    <serialNumber>1.0</serialNumber>\r\n"
"    <UDN>uuid:9f0865b3-f5da-4ad5-85b7-7404637fdf37</UDN>\r\n"
"    <serviceList>\r\n"
"      <service>\r\n"
"        <serviceType>urn:schemas-upnp-org:service:Layer3Forwarding:1</serviceType>\r\n"
"        <serviceId>urn:upnp-org:serviceId:L3Forwarding1</serviceId>\r\n"
"        <controlURL>" L3F_CTL_URL "</controlURL>\r\n"
"        <eventSubURL>" L3F_EVENT_URL "</eventSubURL>\r\n"
"        <SCPDURL>" L3F_SCPD_FILE_NAME "</SCPDURL>\r\n"
"      </service>\r\n"
"    </serviceList>\r\n"
"    <deviceList>\r\n"
"      <device>\r\n"
"        <deviceType>urn:schemas-upnp-org:device:WANDevice:1</deviceType>\r\n"
"        <friendlyName>%s</friendlyName>\r\n"
"        <manufacturer>%s</manufacturer>\r\n"
"        <manufacturerURL>%s</manufacturerURL>\r\n"
"        <modelDescription>%s</modelDescription>\r\n"
"        <modelName>%s</modelName>\r\n"
"        <modelNumber>%s</modelNumber>\r\n"
"        <modelURL>%s</modelURL>\r\n"
"        <serialNumber>1.0</serialNumber>\r\n"
"        <UDN>uuid:9f0865b3-f5da-4ad5-85b7-7404637fdf38</UDN>\r\n"
"        <serviceList>\r\n"
"          <service>\r\n"
"            <serviceType>urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1</serviceType>\r\n"
"            <serviceId>urn:upnp-org:serviceId:WANCommonIFC1</serviceId>\r\n"
"            <controlURL>" WCIFC_CTL_URL "</controlURL>\r\n"
"            <eventSubURL>" WCIFC_EVENT_URL "</eventSubURL>\r\n"
"            <SCPDURL>" WCIFC_SCPD_FILE_NAME "</SCPDURL>\r\n"
"          </service>\r\n"
"        </serviceList>\r\n"
"        <deviceList>\r\n"
"          <device>\r\n"
"            <deviceType>urn:schemas-upnp-org:device:WANConnectionDevice:1</deviceType>\r\n"
"            <friendlyName>%s</friendlyName>\r\n"
"            <manufacturer>%s</manufacturer>\r\n"
"            <manufacturerURL>%s</manufacturerURL>\r\n"
"            <modelDescription>%s</modelDescription>\r\n"
"            <modelName>%s</modelName>\r\n"
"            <modelNumber>%s</modelNumber>\r\n"
"            <modelURL>%s</modelURL>\r\n"
"            <serialNumber>1.0</serialNumber>\r\n"
"            <UDN>uuid:9f0865b3-f5da-4ad5-85b7-7404637fdf39</UDN>\r\n"
"            <serviceList>\r\n"
"              <service>\r\n"
"                <serviceType>urn:schemas-upnp-org:service:WANIPConnection:1</serviceType>\r\n"
"                <serviceId>urn:upnp-org:serviceId:WANIPConn1</serviceId>\r\n"
"                <controlURL>" WIPC_CTL_URL "</controlURL>\r\n"
"                <eventSubURL>" WIPC_EVENT_URL "</eventSubURL>\r\n"
"                <SCPDURL>" WIPC_SCPD_FILE_NAME "</SCPDURL>\r\n"
"              </service>\r\n"
"            </serviceList>\r\n"
"          </device>\r\n"
"        </deviceList>\r\n"
"      </device>\r\n"
"   </deviceList>\r\n"
" </device>\r\n"
"</root>\r\n"
;

/* brief: Added by LI CHENGLONG, 2011-Nov-11.
 *		  l3f 模板.
 */
static char l3fXmlTemple[] = 
"<?xml version=\"1.0\"?>\n"
"<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">\n"
"  <specVersion>\n"
"    <major>1</major>\n"
"    <minor>0</minor>\n"
"  </specVersion>n"
"  <actionList >\n"
"  	<action>\n"
"		<name>SetDefaultConnectionService</name>\n"
"		<argumentList>\n"
"			<argument>\n"
"				<name>NewDefaultConnectionService</name>\n"
"				<direction>in</direction>\n"
"				<relatedStateVariable>DefaultConnectionService</relatedStateVariable>\n"
"			</argument>\n"
"		</argumentList>\n"
"	</action>\n"
"	<action>\n"
"		<name>GetDefaultConnectionService</name>\n"
"		<argumentList>\n"
"			<argument>\n"
"				<name>NewDefaultConnectionService</name>\n"
"				<direction>out</direction>n"
"				<relatedStateVariable>DefaultConnectionService</relatedStateVariable>\n"
"			</argument>\n"
"		</argumentList>\n"
"	</action>\n"
"  </actionList>\n"
"  <serviceStateTable>\n"
"  	<stateVariable sendEvents=\"yes\">\n"
"		<name>DefaultConnectionService</name>\n"
"		<dataType>string</dataType>\n"
"	</stateVariable>\n"
"  </serviceStateTable>\n"
"</scpd>\n"
;

/* 
 * brief: Added by LI CHENGLONG, 2011-Nov-11.
 *		  WANConnectionDevice 描述xml模板.
 */
static char gateconnSCPDXmlTemple[] = 
"<?xml version=\"1.0\"?>\n"
"<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">\n"
"  <specVersion>\n"
"    <major>1</major>\n"
"    <minor>0</minor>\n"
"  </specVersion>\n"
"  <actionList>\n"
"    <action>\n"
"      <name>SetConnectionType</name>\n"
"      <argumentList>\n"
"        <argument>\n"
"          <name>NewConnectionType</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>ConnectionType</relatedStateVariable>\n"
"        </argument>\n"
"      </argumentList>\n"
"    </action>\n"
"    <action>\n"
"    <name>GetConnectionTypeInfo</name>\n"
"      <argumentList>\n"
"        <argument>\n"
"          <name>NewConnectionType</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>ConnectionType</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewPossibleConnectionTypes</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>PossibleConnectionTypes</relatedStateVariable>\n"
"        </argument>\n"
"      </argumentList>\n"
"    </action>\n"
"    <action>\n"
"      <name>RequestConnection</name>\n"
"    </action>\n"
"    <action>\n"
"      <name>ForceTermination</name>\n"
"    </action>\n"
"    <action>\n"
"     <name>GetStatusInfo</name>\n"
"      <argumentList>\n"
"        <argument>\n"
"          <name>NewConnectionStatus</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>ConnectionStatus</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewLastConnectionError</name>\n"
"		  <direction>out</direction>\n"
"          <relatedStateVariable>LastConnectionError</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewUptime</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>Uptime</relatedStateVariable>\n"
"        </argument>\n"
"      </argumentList>\n"
"    </action>\n"
"    <action>\n"
"      <name>GetNATRSIPStatus</name>\n"
"      <argumentList>\n"
"        <argument>\n"
"          <name>NewRSIPAvailable</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>RSIPAvailable</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewNATEnabled</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>NATEnabled</relatedStateVariable>\n"
"        </argument>\n"
"      </argumentList>\n"
"    </action>\n"
"    <action>\n"
"      <name>GetGenericPortMappingEntry</name>\n"
"      <argumentList>\n"
"        <argument>\n"
"          <name>NewPortMappingIndex</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>PortMappingNumberOfEntries</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewRemoteHost</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>RemoteHost</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewExternalPort</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>ExternalPort</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewProtocol</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>PortMappingProtocol</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewInternalPort</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>InternalPort</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewInternalClient</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>InternalClient</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewEnabled</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>PortMappingEnabled</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewPortMappingDescription</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>PortMappingDescription</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewLeaseDuration</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>PortMappingLeaseDuration</relatedStateVariable>\n"
"        </argument>\n"
"      </argumentList>\n"
"    </action>\n"
"    <action>\n"
"      <name>GetSpecificPortMappingEntry</name>\n"
"      <argumentList>\n"
"        <argument>\n"
"          <name>NewRemoteHost</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>RemoteHost</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewExternalPort</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>ExternalPort</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewProtocol</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>PortMappingProtocol</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewInternalPort</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>InternalPort</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewInternalClient</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>InternalClient</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewEnabled</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>PortMappingEnabled</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewPortMappingDescription</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>PortMappingDescription</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewLeaseDuration</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>PortMappingLeaseDuration</relatedStateVariable>\n"
"        </argument>\n"
"      </argumentList>\n"
"    </action>\n"
"    <action>\n"
"      <name>AddPortMapping</name>\n"
"      <argumentList>\n"
"        <argument>\n"
"          <name>NewRemoteHost</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>RemoteHost</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewExternalPort</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>ExternalPort</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewProtocol</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>PortMappingProtocol</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewInternalPort</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>InternalPort</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewInternalClient</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>InternalClient</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewEnabled</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>PortMappingEnabled</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewPortMappingDescription</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>PortMappingDescription</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewLeaseDuration</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>PortMappingLeaseDuration</relatedStateVariable>\n"
"        </argument>\n"
"      </argumentList>\n"
"    </action>\n"
"    <action>\n"
"      <name>DeletePortMapping</name>\n"
"      <argumentList>\n"
"        <argument>\n"
"          <name>NewRemoteHost</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>RemoteHost</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewExternalPort</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>ExternalPort</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewProtocol</name>\n"
"          <direction>in</direction>\n"
"          <relatedStateVariable>PortMappingProtocol</relatedStateVariable>\n"
"        </argument>\n"
"      </argumentList>\n"
"    </action>\n"
"    <action>\n"
"    <name>GetExternalIPAddress</name>\n"
"      <argumentList>\n"
"        <argument>\n"
"          <name>NewExternalIPAddress</name>\n"
"          <direction>out</direction>\n"
"        <relatedStateVariable>ExternalIPAddress</relatedStateVariable>\n"
"        </argument>\n"
"      </argumentList>\n"
"    </action>\n"
"  </actionList>\n"
"  <serviceStateTable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>ConnectionType</name>\n"
"      <dataType>string</dataType>\n"
"      <defaultValue>Unconfigured</defaultValue>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"yes\">\n"
"      <name>PossibleConnectionTypes</name>\n"
"      <dataType>string</dataType>\n"
"      <allowedValueList>\n"
"        <allowedValue>Unconfigured</allowedValue>\n"
"        <allowedValue>IP_Routed</allowedValue>\n"
"        <allowedValue>IP_Bridged</allowedValue>\n"
"      </allowedValueList>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"yes\">\n"
"      <name>ConnectionStatus</name>\n"
"      <dataType>string</dataType>\n"
"      <defaultValue>Unconfigured</defaultValue>\n"
"      <allowedValueList>\n"
"        <allowedValue>Unconfigured</allowedValue>\n"
"    	  <allowedValue>Connecting</allowedValue>\n"
"    	  <allowedValue>Authenticating</allowedValue>\n"
"        <allowedValue>PendingDisconnect</allowedValue>\n"
"        <allowedValue>Disconnecting</allowedValue>\n"
"        <allowedValue>Disconnected</allowedValue>\n"
"        <allowedValue>Connected</allowedValue>\n"
"      </allowedValueList>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>Uptime</name>\n"
"      <dataType>ui4</dataType>\n"
"      <defaultValue>0</defaultValue>\n"
"      <allowedValueRange>\n"
"        <minimum>0</minimum>\n"
"        <maximum></maximum>\n"
"        <step>1</step>\n"
"      </allowedValueRange>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>RSIPAvailable</name>\n"
"      <dataType>boolean</dataType>\n"
"      <defaultValue>0</defaultValue>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>NATEnabled</name>\n"
"      <dataType>boolean</dataType>\n"
"      <defaultValue>1</defaultValue>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>LastConnectionError</name>\n"
"      <dataType>string</dataType>\n"
"      <defaultValue>ERROR_NONE</defaultValue>\n"
"      <allowedValueList>\n"
"        <allowedValue>ERROR_NONE</allowedValue>\n"
"    	<allowedValue>ERROR_ISP_TIME_OUT</allowedValue>\n"
"        <allowedValue>ERROR_COMMAND_ABORTED</allowedValue>\n"
"        <allowedValue>ERROR_NOT_ENABLED_FOR_INTERNET</allowedValue>\n"
"        <allowedValue>ERROR_BAD_PHONE_NUMBER</allowedValue>\n"
"        <allowedValue>ERROR_USER_DISCONNECT</allowedValue>\n"
"        <allowedValue>ERROR_ISP_DISCONNECT</allowedValue>\n"
"        <allowedValue>ERROR_IDLE_DISCONNECT</allowedValue>\n"
"        <allowedValue>ERROR_FORCED_DISCONNECT</allowedValue>\n"
"        <allowedValue>ERROR_SERVER_OUT_OF_RESOURCES</allowedValue>\n"
"        <allowedValue>ERROR_RESTRICTED_LOGON_HOURS</allowedValue>\n"
"        <allowedValue>ERROR_ACCOUNT_DISABLED</allowedValue>\n"
"        <allowedValue>ERROR_ACCOUNT_EXPIRED</allowedValue>\n"
"        <allowedValue>ERROR_PASSWORD_EXPIRED</allowedValue>\n"
"        <allowedValue>ERROR_AUTHENTICATION_FAILURE</allowedValue>\n"
"        <allowedValue>ERROR_NO_DIALTONE</allowedValue>\n"
"        <allowedValue>ERROR_NO_CARRIER</allowedValue>\n"
"        <allowedValue>ERROR_NO_ANSWER</allowedValue>\n"
"	    <allowedValue>ERROR_LINE_BUSY</allowedValue>\n"
"	    <allowedValue>ERROR_UNSUPPORTED_BITSPERSECOND</allowedValue>\n"
"	    <allowedValue>ERROR_TOO_MANY_LINE_ERRORS</allowedValue>\n"
"	    <allowedValue>ERROR_IP_CONFIGURATION</allowedValue>\n"
"	    <allowedValue>ERROR_UNKNOWN</allowedValue>\n"
"      </allowedValueList>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"yes\">\n"
"      <name>ExternalIPAddress</name>\n"
"      <dataType>string</dataType>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>RemoteHost</name>\n"
"      <dataType>string</dataType>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>ExternalPort</name>\n"
"      <dataType>ui2</dataType>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>InternalPort</name>\n"
"      <dataType>ui2</dataType>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>PortMappingProtocol</name>\n"
"      <dataType>string</dataType>\n"
"      <allowedValueList>\n"
"        <allowedValue>TCP</allowedValue>\n"
"        <allowedValue>UDP</allowedValue>\n"
"      </allowedValueList>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>InternalClient</name>\n"
"      <dataType>string</dataType>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>PortMappingDescription</name>\n"
"      <dataType>string</dataType>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>PortMappingEnabled</name>\n"
"      <dataType>boolean</dataType>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>PortMappingLeaseDuration</name>\n"
"      <dataType>ui4</dataType>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"yes\">\n"
"      <name>PortMappingNumberOfEntries</name>\n"
"      <dataType>ui2</dataType>\n"
"    </stateVariable>\n"
"  </serviceStateTable>\n"
"</scpd>\n"
;

/* 
 * brief: Added by LI CHENGLONG, 2011-Nov-11.
 *		  WANCommonInterfaceConfig 描述xml.
 */
static char gateicfgSCPDXmlTemple[] = 
"<?xml version=\"1.0\"?>\n"
"<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">\n"
"	<specVersion>\n"
"		<major>1</major>\n"
"		<minor>0</minor>\n"
"	</specVersion>\n"
"	<actionList>\n"
"    <action>\n"
"      <name>GetCommonLinkProperties</name>\n"
"      <argumentList>\n"
"        <argument>\n"
"          <name>NewWANAccessType</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>WANAccessType</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewLayer1UpstreamMaxBitRate</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>Layer1UpstreamMaxBitRate</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewLayer1DownstreamMaxBitRate</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>Layer1DownstreamMaxBitRate</relatedStateVariable>\n"
"        </argument>\n"
"        <argument>\n"
"          <name>NewPhysicalLinkStatus</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>PhysicalLinkStatus</relatedStateVariable>\n"
"        </argument>\n"
"      </argumentList>\n"
"    </action>\n"
"    <action>\n"
"    <name>GetTotalBytesSent</name>\n"
"      <argumentList>\n"
"        <argument>\n"
"          <name>NewTotalBytesSent</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>TotalBytesSent</relatedStateVariable>\n"
"        </argument>\n"
"      </argumentList>\n"
"    </action>\n"
"    <action>\n"
"    <name>GetTotalBytesReceived</name>\n"
"      <argumentList>\n"
"        <argument>\n"
"          <name>NewTotalBytesReceived</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>TotalBytesReceived</relatedStateVariable>\n"
"        </argument>\n"
"      </argumentList>\n"
"    </action>\n"
"    <action>\n"
"    <name>GetTotalPacketsSent</name>\n"
"      <argumentList>\n"
"        <argument>\n"
"          <name>NewTotalPacketsSent</name>\n"
"          <direction>out</direction>\n"
"          <relatedStateVariable>TotalPacketsSent</relatedStateVariable>\n"
"        </argument>\n"
"      </argumentList>\n"
"    </action>\n"
"    <action>\n"
"    <name>GetTotalPacketsReceived</name>\n"
"      <argumentList>\n"
"        <argument>\n"
"          <name>NewTotalPacketsReceived</name>\n"
"          <direction>out</direction>\n"
"         <relatedStateVariable>TotalPacketsReceived</relatedStateVariable>\n"
"        </argument>\n"
"      </argumentList>\n"
"    </action>\n"
"	</actionList>\n"
"	<serviceStateTable>\n"
"		<stateVariable sendEvents=\"no\">\n"
"			<name>WANAccessType</name>\n"
"			<dataType>string</dataType>\n"
"			<allowedValueList>\n"
"				<allowedValue>DSL</allowedValue>\n"
"				<allowedValue>POTS</allowedValue>\n"
"				<allowedValue>Cable</allowedValue>\n"
"				<allowedValue>Ethernet</allowedValue>\n"
"				<allowedValue>Other</allowedValue>\n"
"			</allowedValueList>\n"
"		</stateVariable>\n"
"		<stateVariable sendEvents=\"no\">\n"
"			<name>Layer1UpstreamMaxBitRate</name>\n"
"			<dataType>ui4</dataType>\n"
"		</stateVariable>\n"
"		<stateVariable sendEvents=\"no\">\n"
"			<name>Layer1DownstreamMaxBitRate</name>\n"
"			<dataType>ui4</dataType>\n"
"		</stateVariable>\n"
"		<stateVariable sendEvents=\"yes\">\n"
"			<name>PhysicalLinkStatus</name>\n"
"			<dataType>string</dataType>\n"
"	  <allowedValueList>\n"
"        <allowedValue>Up</allowedValue>\n"
"        <allowedValue>Down</allowedValue>\n"
"        <allowedValue>Initializing</allowedValue>\n"
"        <allowedValue>Unavailable</allowedValue>\n"
"      </allowedValueList>\n"
"		</stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>WANAccessProvider</name>\n"
"      <dataType>string</dataType>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>MaximumActiveConnections</name>\n"
"      <dataType>ui2</dataType>\n"
"      <allowedValueRange>\n"
"        <minimum>1</minimum>\n"
"        <maximum></maximum>\n"
"        <step>1</step>\n"
"      </allowedValueRange>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>TotalBytesSent</name>\n"
"      <dataType>ui4</dataType>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>TotalBytesReceived</name>\n"
"      <dataType>ui4</dataType>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>TotalPacketsSent</name>\n"
"      <dataType>ui4</dataType>\n"
"    </stateVariable>\n"
"    <stateVariable sendEvents=\"no\">\n"
"      <name>TotalPacketsReceived</name>\n"
"      <dataType>ui4</dataType>\n"
"    </stateVariable>\n"
"	</serviceStateTable>\n"
"</scpd>\n"
;

/* 
 * brief: Added by LI CHENGLONG, 2011-Nov-11.
 *		  upnpd 的配置文件.
 */
static char upnpdOptionFile[BUFLEN_2048] =
"# To change the interfaces used edit:\n"
"#   /etc/sysconfig/upnpd\n"
"\n"
"#\n"
"# The full path and name of the iptables executable,\n"
"# (enclosed in quotes).\n"
"#\n"
"iptables_location = \"/usr/bin/iptables\"\n"
"\n"
"#\n"
"# Daemon debug level. Messages are logged via syslog to debug.\n"
"# 0 - no debug messages\n"
"# 1 - log errors\n"
"# 2 - log errors and basic info\n"
"# 3 - log errors and verbose info\n"
"# default = 0\n"
"debug_mode = 0\n"
"\n"
"#\n"
"# Should the daemon insert rules in the forward chain\n"
"# This is necessary if your firewall has a drop or reject\n"
"# policy in your forward chain.\n"
"# allowed values: yes,no\n"
"# default = no\n"
"insert_forward_rules = yes\n"
"\n"
"#\n"
"# The name of the chain to put the forward rules in.\n"
"# This directive is only activ if \"insert_forward_rules = yes\"\n"
"# above.\n"
"# allowed values: a-z, A-Z, _, -\n"
"# default = FORWARD\n"
"#\n"
"forward_chain_name = FORWARD\n"
"\n"
"#\n"
"# The name of the chain to put prerouting rules in.\n"
"# allowed values: a-z, A-Z, _, -\n"
"# default = PREROUTING\n"
"prerouting_chain_name = PREROUTING\n"
"\n"
"#\n"
"# The internet line upstream bit rate reported from\n"
"# the daemon. Value in bits per second\n"
"# default = 0\n"
"upstream_bitrate = 512000\n"
"\n"
"#\n"
"# The internet line downstream bit rate reported from\n"
"# the daemon. Value in bits per second\n"
"# default = 0\n"
"downstream_bitrate = 512000\n"
"\n"
"#\n"
"# The default duration of port mappings, used when the client\n"
"# doesn't specify a duration\n"
"# Can have the following values:\n"
"# 0 - no default duration specified\n"
"# seconds | HH:MM - duration from the time of addition\n"
"# @seconds | @HH:MM - expire mapping at the specified time of day\n"
"# default = 0\n"
"duration = 0 # One day\n"
"\n"
"# The name of the igd device xml description document\n"
"# default = gatedesc.xml\n"
"description_document_name = gatedesc.xml\n"
"\n"
"# The path to the xml documents\n"
"# Do not include the trailing \"/\"\n"
"# default = /etc/linuxigd\n"
"# WARNING! The make install does put the xml files\n"
"# in /etc/linuxigd, if you change this variable\n"
"# you have to make sure the xml docs are in the\n"
"# right place\n"
"xml_document_path = /var/tmp/upnpd\n"
;


/* 
 * brief: Added by LI CHENGLONG, 2011-Nov-11.
 *		  rootdevice 设备描述.
 */
static char gateDescXml[BUFLEN_4096];

/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
/**************************************************************************************************/

/* 
 * fn		UBOOL32 initConfigFile()
 * brief	初始化upnpd的配置文件.	
 * details	生成配置文件保存到配置文件目录下.
 *
 * return		UBOOL32
 * retval		TRUE	设置成功
 *				FALSE	设置失败
 *
 * note	written by  08Nov11, LI CHENGLONG	
 */
UBOOL32 initConfigFile()
{
	char cmd[BUFLEN_32];
	int ret;
	FILE *pFile = NULL;
	int count = 0;
	char path[BUFLEN_128];
	int fd;

	snprintf(cmd, BUFLEN_32, "mkdir -p %s", UPNPD_CONFIG_FILE_PATH);
	ret = system(cmd);
	if (-1 == ret)
	{
		return FALSE;
	}

	
	fd = creat(UPNPD_PM_LIST_DB_FILE, S_IRWXU|S_IRWXG|S_IRWXO);
	if (fd >=0)
	{
		close(fd);
 	}

	/*write to dummy.xml, Added by LI CHENGLONG , 2011-Nov-08.*/
	snprintf(path, BUFLEN_128, "%s/%s%s", UPNPD_CONFIG_FILE_PATH, L3F_SCPD_NAME, UPNP_XML_SUFFIX);
	pFile = fopen(path, "w+");
	if (pFile == NULL)
	{
		trace(1, "can't open file %s", path);
		return FALSE;
	}
	count = fwrite(l3fXmlTemple, sizeof(char), strlen(l3fXmlTemple), pFile);
	if (count != strlen(l3fXmlTemple))
	{
		fclose(pFile);
		return FALSE;
	}
	fclose(pFile);
 	/* Ended by LI CHENGLONG , 2011-Nov-08.*/
	
 	/*write to gateconnSCPD.xml, Added by LI CHENGLONG , 2011-Nov-08.*/
	snprintf(path, BUFLEN_128, "%s/%s%s", UPNPD_CONFIG_FILE_PATH, WIPC_SCPD_NAME, UPNP_XML_SUFFIX);
	pFile = fopen(path, "w+");
	if (pFile == NULL)
	{
		trace(1, "can't open file %s\n", path);
		return FALSE;
	}
	count = fwrite(gateconnSCPDXmlTemple, sizeof(char), strlen(gateconnSCPDXmlTemple), pFile);
	if (count != strlen(gateconnSCPDXmlTemple))
	{
		fclose(pFile);
		return FALSE;
	}
	fclose(pFile);
 	/* Ended by LI CHENGLONG , 2011-Nov-08.*/

 	/*write to gateicfgSCPD.xml, Added by LI CHENGLONG , 2011-Nov-08.*/
	snprintf(path, BUFLEN_128, "%s/%s%s", UPNPD_CONFIG_FILE_PATH, WCIFC_SCPD_NAME, UPNP_XML_SUFFIX);
	pFile = fopen(path, "w+");
	if (pFile == NULL)
	{
		trace(1, "can't open file %s\n", path);
		return FALSE;
	}
	count = fwrite(gateicfgSCPDXmlTemple, sizeof(char), strlen(gateicfgSCPDXmlTemple), pFile);
	if (count != strlen(gateicfgSCPDXmlTemple))
	{
		fclose(pFile);
		return FALSE;
	}
	fclose(pFile);
 	/* Ended by LI CHENGLONG , 2011-Nov-08.*/


 	/*write to upnp.conf Added by LI CHENGLONG , 2011-Nov-08.*/
	snprintf(path, BUFLEN_128, "%s/%s", UPNPD_CONFIG_FILE_PATH, CONF_FILE_NAME);
	pFile = fopen(path, "w+");
	if (pFile == NULL)
	{
		trace(1, "can't open file %s\n", path);
		return FALSE;
	}
	count = fwrite(upnpdOptionFile, sizeof(char), strlen(upnpdOptionFile), pFile);
	if (count != strlen(upnpdOptionFile))
	{
		fclose(pFile);
		return FALSE;
	}
	fclose(pFile);
 	/* Ended by LI CHENGLONG , 2011-Nov-08.*/

	
	return TRUE;
	
}

/* 
 * fn		UBOOL32 upnpDescInit(char *devManufacturerUrl,
 *								 char *devManufacturer,
 *								 char *devModelName,
 *								 char *devModelVer,
 *								 char *devName,
 *								 char *description,
 *								 UINT32 webPort,
 *								 UINT32 ip
 *			);
 * brief	根据设备信息更新设备描述文件gatedesc.xml	
 * details	根据设备信息更新设备描述文件gatedesc.xml	
 *
 * param[in]	devManufacturerUrl			upnp设备厂商url
 * param[in]	devManufacturer				upnp设备厂商名
 * param[in]	devModelName				upnp设备model名
 * param[in]	devModelVer					upnp设备model版本
 * param[in]	devName						upnp设备名
 * param[in]   description					upnp设备描述
 * param[in]	webPort						webport default 80
 * param[in]	ip							lan端ip default br0's ip
 *
 * return		UBOOL32
 * retval		TRUE	更新成功
 *				FALSE	更新失败
 *
 * note	written by  02Nov11, LI CHENGLONG	
 */
UBOOL32 upnpDescInit(char *devManufacturerUrl,
					 char *devManufacturer,
					 char *devModelName,
					 char *devModelVer,
					 char *devName,
					 char *description,
					 UINT32 webPort,
					 UINT32 ip
)
{
	FILE *pFile = NULL;
	int  count = 0;
	char path[BUFLEN_128];
	char modelDescription[BUFLEN_256];
	char presentationURL[BUFLEN_256];



	snprintf(presentationURL, sizeof(STRING_URL) - 1, "http://%d.%d.%d.%d:%d/",
			(ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff, webPort);

	snprintf(modelDescription, BUFLEN_256 - 1,  "%s", description);

	snprintf(gateDescXml,
			 BUFLEN_4096,
			 gateDescXmlTemple, 
			 presentationURL,
			 devName,
			 devManufacturer,
			 devManufacturerUrl,
			 modelDescription,
			 devModelName,
			 devModelVer,
			 presentationURL,
             g_vars.randomPrefix,
             g_vars.randomPrefix,
			 devName,
			 devManufacturer,
			 devManufacturerUrl,
			 modelDescription,
			 devModelName,
			 devModelVer,
			 presentationURL,	
             g_vars.randomPrefix,
             g_vars.randomPrefix,
			 devName,		
			 devManufacturer,
			 devManufacturerUrl,
			 modelDescription,
			 devModelName,	
			 devModelVer,		
			 presentationURL,
             g_vars.randomPrefix,
             g_vars.randomPrefix
			);
	

	snprintf(path, BUFLEN_128, "%s/%s/%s", g_vars.xmlPath, g_vars.randomPrefix, g_vars.descDocName);
	pFile = fopen(path, "w+");
	if (pFile == NULL)
	{
		trace(1, "can't open file %s\n", path);
		return FALSE;
	}

	count = fwrite(gateDescXml, sizeof(char), strlen(gateDescXml), pFile);
	
	if (count != strlen(gateDescXml))
	{
		fclose(pFile);
		return FALSE;
	}
	
	fclose(pFile);
	return TRUE;
}

