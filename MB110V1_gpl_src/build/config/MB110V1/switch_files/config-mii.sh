#!/bin/sh

# $Id: //WIFI_SOC/MP/SDK_4_3_0_0/RT288x_SDK/source/user/rt2880_app/scripts/config-vlan.sh#2 $
#
# usage: config-vlan.sh <switch_type> <vlan_type>
#   switch_type: 0=IC+, 1=vtss
#   vlan_type: 0=no_vlan, 1=vlan, LLLLW=wan_4, WLLLL=wan_0
#

resetMiiPortV()
{
	mii_mgr -s -p 0 -r 0 -v 3900
	mii_mgr -s -p 1 -r 0 -v 3900
	mii_mgr -s -p 2 -r 0 -v 3900
	mii_mgr -s -p 3 -r 0 -v 3900
	mii_mgr -s -p 4 -r 0 -v 3900
	mii_mgr -s -p 0 -r 0 -v 3300
	mii_mgr -s -p 1 -r 0 -v 3300
	mii_mgr -s -p 2 -r 0 -v 3300
	mii_mgr -s -p 3 -r 0 -v 3300
	mii_mgr -s -p 4 -r 0 -v 3300

	echo "resetMiiPortV over." > /dev/console

	mii_mgr -s -p 0 -r 4 -v 1e1
	mii_mgr -s -p 0 -r 0 -v 3300
	mii_mgr -s -p 1 -r 4 -v 1e1
	mii_mgr -s -p 1 -r 0 -v 3300
	mii_mgr -s -p 2 -r 4 -v 1e1
	mii_mgr -s -p 2 -r 0 -v 3300
	mii_mgr -s -p 3 -r 4 -v 1e1
	mii_mgr -s -p 3 -r 0 -v 3300
	mii_mgr -s -p 4 -r 4 -v 1e1
	mii_mgr -s -p 4 -r 0 -v 3300

	echo "turn off flow control over." > /dev/console
}

resetMiiPortV
