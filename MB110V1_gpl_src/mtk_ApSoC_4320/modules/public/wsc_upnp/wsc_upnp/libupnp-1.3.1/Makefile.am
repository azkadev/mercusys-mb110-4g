# $Id: //WIFI_SOC/TRUNK/RT288x_SDK/source/user/wsc_upnp/libupnp-1.3.1/Makefile.am#1 $
#
# Top-level "Makefile.am" for libupnp
#
# Copyright (C) 2005 Rémi Turboult <r3mi@users.sourceforge.net>
#

ACLOCAL_AMFLAGS = -I m4

SUBDIRS 	= ixml threadutil upnp


EXTRA_DIST	= libupnp.pc.in LICENSE


# This variable must have 'exec' in its name, in order to be installed 
# by 'install-exec' target (instead of default 'install-data')
pkgconfigexecdir 	= $(libdir)/pkgconfig
pkgconfigexec_DATA 	= libupnp.pc

$(pkgconfigexec_DATA): config.status


CLEANFILES	= IUpnpErrFile.txt IUpnpInfoFile.txt





