##############################################################################
# $Id:
# =============================================================================
#  Copyright (C) 2007 Novell, Inc. All rights reserved.
# 
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
# 
#   - Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
# 
#   - Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
# 
#   - Neither the name of Novell, Inc. nor the names of its
#     contributors may be used to endorse or promote products derived from this
#     software without specific prior written permission.
# 
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL Novell, Inc. OR THE CONTRIBUTORS
#  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#
# Author:       Tom Lee, <tlee@novell.com>
# Contributors: 
# Last Updated:
# Description:
#    Automake input file for the OMC SMI-S server profile provider.
# =============================================================================
# EDIT THIS FILE AS APPROPRIATE TO BUILD THE PROVIDER, INSTALL THE PROVIDER
# LIBRARIES, AND REGISTER THE NEW PROVIDER AND CLASSES TO THE CURRENT CIMOM.
# =============================================================================
##############################################################################

lib=@LIBDIR@

#Set the pkgdatadir to the SFCB stage directory
pkgdatadir = /usr/share/mof/smis-providers
#pkgdatadir = /var/lib/sfcb/stage/
#pkgdatadir_MOFS = $(pkgdatadir)/mofs/root/cimv2
#pkgdatadir_MOFS_INTEROP = $(pkgdatadir)/mofs/root/interop
#pkgdatadir_REGS = $(pkgdatadir)/regs/

# Start with an empty extra distribution file list
EXTRA_DIST=

# Add the RPM spec file to the distribution file list
EXTRA_DIST+=$(top_srcdir)/$(PACKAGE).spec

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
provider_LTLIBRARIES = libomc_smi_server.la
libomc_smi_server_la_SOURCES = \
	ServerProvider.c \
	RegisteredProfile.c \
	SoftwareIdentity.c

###libomc_smi_server_la_LIBADD = -lomc_cmpi -lomc_cbase
libomc_smi_server_la_LIBADD = -lsblim-cmpiutil 

libomc_smi_server_la_LDFLAGS = \
	-avoid-version -module

# -----------------------------------------------------------------------------
# Automake instructions for schema
# -----------------------------------------------------------------------------

# LIST ALL THE CLASS DEFINITION FILES (MOFS) HERE
MOFS= \
	mof/OMC_RegisteredSMIProfile.mof \
	mof/OMC_RegisteredSMISubProfile.mof \
	mof/OMC_SMIVolumeManagementSoftware.mof \
	mof/OMC_SMIArraySoftware.mof \
	mof/OMC_ElementConformsToSMIProfile.mof \
	mof/OMC_SMIElementSoftwareIdentity.mof \
	mof/OMC_SMISubProfileRequiresProfile.mof \
	mof/deploy.mof

# LIST ALL THE CLASS CIMOM REGISTRATION FILES HERE
REGS= \
	regs/OMC_Server.sfcb.reg

# Un/Register the providers and class definitions from/to the current CIMOM.
# @CIMSERVER@ is set by the configure script
postinstall:
	sh provider-register.sh -t @CIMSERVER@ -r $(REGS) -m $(MOFS)

preuninstall:
	sh provider-register.sh -d -t @CIMSERVER@ -r $(REGS) -m $(MOFS)

# Add the schema files to the distribution file list
pkgdata_MOFDATA=$(MOFS)
pkgdata_REGDATA=$(REGS)

#pkgdata_SCRIPTS=provider-register.sh
EXTRA_DIST+=schema $(pkgdata_DATA) $(pkgdata_SCRIPTS) 


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
#	install -d $(DESTDIR)/$(pkgdatadir_MOFS)
#	install -d $(DESTDIR)/$(pkgdatadir_MOFS_INTEROP)
#	install -d $(DESTDIR)/$(pkgdatadir_REGS)
#	install -m 644 $(MOFS) $(DESTDIR)/$(pkgdatadir_MOFS)
#	install -m 644 $(MOFS) $(DESTDIR)/$(pkgdatadir_MOFS_INTEROP)
#	install -m 644 $(REGS) $(DESTDIR)/$(pkgdatadir_REGS)
#	install -m 644 /dev/null $(DESTDIR)/$(pkgdatadir_CONF)/smsetup.conf
	install -m 644 $(MOFS) $(REGS) $(DESTDIR)/$(pkgdatadir)
