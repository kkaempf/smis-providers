#
# 
# | 	SMIS-Providers
# | 	Copyright (c) [2008] Novell, Inc.
# | 	All rights reserved. 
# |
# | This program and the accompanying materials
# | are made available under the terms of the Eclipse Public License v1.0
# | which accompanies this distribution, and is available at
# | http://www.eclipse.org/legal/epl-v10.html 
# |
# |********************************************************************************
# |
# |	 OMC SMI-S Volume Management provider
# |
# |---------------------------------------------------------------------------
# |
# | $Id: 
# |
# |---------------------------------------------------------------------------
# | Description:
# |   Automake input file for the OMC SMI-S Array provider.
# |=============================================================================
# |EDIT THIS FILE AS APPROPRIATE TO BUILD THE PROVIDER, INSTALL THE PROVIDER
# |LIBRARIES, AND REGISTER THE NEW PROVIDER AND CLASSES TO THE CURRENT CIMOM.
# |=============================================================================*/

CC=@CXX@
lib=@LIBDIR@

#Set the pkgdatadir to the SFCB stage directory
pkgdatadir = /usr/share/mof/smis-providers
pkgdatadir_CONF = /etc
#pkgdatadir = /var/lib/sfcb/stage/
#pkgdatadir_MOFS = $(pkgdatadir)/mofs/root/cimv2
#pkgdatadir_REGS = $(pkgdatadir)/regs/

#Set etcdir to /etc/ where we copy the sample smsetup.conf file
#etcdir = /etc/

# Start with an empty extra distribution file list
EXTRA_DIST=

# Add the RPM spec file to the distribution file list
EXTRA_DIST+=$(top_srcdir)\$(PACKAGE).spec

# Remove CVS directories when building a distribution package
dist-hook:
	test -d "$(distdir)" && (find $(distdir) -type d -name CVS | xargs -i rm -rf {})


# -----------------------------------------------------------------------------
# Automake instructions for documentation
# -----------------------------------------------------------------------------

# MAKE CHANGES HERE TO INSTALL THE DOCUMENTATION FILES FOR YOUR PROVIDER
# docdir specifies where the documentation will be installed
docdir=$(datadir)/doc/$(PACKAGE)-$(VERSION)

# Add standard package doc files
#doc_DATA=README AUTHORS COPYING

# ADD ANY EXTRA DOCUMENTATION FILES HERE
#doc_DATA+=README.RPM README.DEBUG
EXTRA_DIST+=doc README.RPM README.DEBUG


# -----------------------------------------------------------------------------
# Automake instructions for source 
# -----------------------------------------------------------------------------
# MAKE CHANGES HERE TO COMPILE ALL THE SOURCE FILES FOR YOUR PROVIDER

# providerdir specifies where provider libraries will be installed.
# @PROVIDERDIR@ is set by the configure script.
providerdir=@PROVIDERDIR@

# LIST EACH PROVIDER LIBRARY, ITS SOURCE FILE(S), AND ANY LIBS REQUIRED 
# FOR LINKING HERE
provider_LTLIBRARIES = libomc_smi_array.la
libomc_smi_array_la_SOURCES = \
	ArrayProvider.c \
	StorageConfigurationService.c \
	StoragePool.c \
	StorageCapability.c \
	StorageExtent.c \
	StorageVolume.c \
	StorageSetting.c \
	Utils.c \
	CopyServicesExtrinsic.c \
	LvmCopyServices.c

####libomc_smi_array_la_LIBADD = -lomc_cmpi -lomc_cbase -levms
libomc_smi_array_la_LIBADD = -lsblim-cmpiutil -ly2storage

libomc_smi_array_la_LDFLAGS = \
	-avoid-version


# -----------------------------------------------------------------------------
# Automake instructions for schema
# -----------------------------------------------------------------------------

# LIST ALL THE CLASS DEFINITION FILES (MOFS) HERE
MOFS= \
	mof/OMC_CompositeExtent.mof \
	mof/OMC_LogicalDisk.mof \
	mof/OMC_StorageCapabilities.mof \
	mof/OMC_StorageConfigurationCapabilities.mof \
	mof/OMC_StorageConfigurationService.mof \
	mof/OMC_StorageExtent.mof \
	mof/OMC_StoragePool.mof \
	mof/OMC_StorageSettingWithHints.mof \
	mof/OMC_SystemStorageCapabilities.mof \
	mof/OMC_AllocatedFromStoragePool.mof \
	mof/OMC_BasedOn.mof \
	mof/OMC_CompositeExtentBasedOn.mof \
	mof/OMC_HostedStorageConfigurationService.mof \
	mof/OMC_HostedStoragePool.mof \
	mof/OMC_LogicalDiskDevice.mof \
	mof/OMC_StorageConfigurationElementCapabilities.mof \
	mof/OMC_StorageElementCapabilities.mof \
	mof/OMC_StorageElementSettingData.mof \
	mof/OMC_AssociatedComponentExtent.mof \
	mof/OMC_AssociatedRemainingExtent.mof \
	mof/OMC_StorageSettingsGeneratedFromCapabilities.mof \
	mof/OMC_StorageVolume.mof \
	mof/OMC_StorageVolumeDevice.mof

# LIST ALL THE CLASS CIMOM REGISTRATION FILES HERE
REGS= \
	regs/OMC_Array.sfcb.reg

# Un/Register the providers and class definitions from/to the current CIMOM.
# @CIMSERVER@ is set by the configure script
postinstall:
	sh provider-register.sh -t @CIMSERVER@ -r $(REGS) -m $(MOFS)

preuninstall:
	sh provider-register.sh -d -t @CIMSERVER@ -r $(REGS) -m $(MOFS)

# Add the schema files to the distribution file list
pkgdata_MOFDATA=$(MOFS)
pkgdata_REGDATA=$(REGS)

#pkgdata_DATA=$(pkgdata_REGDATA) $(pkgdata_MOFDATA)

#pkgdata_SCRIPTS=provider-register.sh
#EXTRA_DIST+=schema $(pkgdata_DATA) $(pkgdata_SCRIPTS) 
EXTRA_DIST+=schema $(pkgdata_DATA) 


# -----------------------------------------------------------------------------
# Automake instructions for tests
# -----------------------------------------------------------------------------

#if TESTSUITE
#testsuitedir=@TESTSUITEDIR@
#testsuitesystemdir=@TESTSUITEDIR@/system/linux
#testsuitecimdir=@TESTSUITEDIR@/cim

# LIST ALL THE PROVIDER TEST SCRIPTS AND DATA FILES HERE
#testsuite_SCRIPTS=test/test-@PACKAGE@.sh
#testsuitesystem_DATA=test/system/linux/CWS_Process.system
#testsuitesystem_SCRIPTS=test/system/linux/CWS_Process.pl
#testsuitecim_DATA=test/cim/CWS_Process.cim

# Run the package test script 
#runtest: install
#	cd $(DESTDIR)$(TESTSUITEDIR) && sh test-@PACKAGE@.sh
#endif

# Add the test files to the distribution file list
#doc_DATA+=README.TEST
EXTRA_DIST+=test README.TEST

install-data-local:
	install -d $(DESTDIR)/$(pkgdatadir)
	install -d $(DESTDIR)/$(pkgdatadir_CONF)
#	install -d $(DESTDIR)/$(pkgdatadir_MOFS)
#	install -d $(DESTDIR)/$(pkgdatadir_REGS)
#	install -m 644 $(MOFS) $(DESTDIR)/$(pkgdatadir_MOFS)
#	install -m 644 $(REGS) $(DESTDIR)/$(pkgdatadir_REGS)
	install -m 644 $(top_srcdir)/etc/smsetup.conf $(DESTDIR)/$(pkgdatadir_CONF)
	install -m 644 $(MOFS) $(REGS) $(DESTDIR)/$(pkgdatadir)

