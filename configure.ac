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
# Last Updated: January 2, 2007
# Description:
#    Autoconf input file for the OMC SMI-S volume management provider.
#    Process this file with autoconf to produce a configure script.
#
##############################################################################

# Package information
#AC_INIT(<PACKAGE_NAME>, <PACKAGE_VERSION>, <PACKAGE_BUGREPORT>, <PACKAGE_TARBALL>)
AC_INIT(smis-providers, 1.0.0, rkaur@novell.com, smis-providers)

# Identify the source directory 
# SET THIS TO THE RELATIVE PATHNAME OF *ONE* OF THE SOURCE FILES
AC_CONFIG_SRCDIR([src])

# Create any necessary files from "<file>.in" templates
# SET THIS TO THE NAME OF THE PACKAGE RPM SPEC FILE
AC_CONFIG_FILES([smis-providers.spec])

# Check for required programs; e.g. commands used in system() calls or
# special additional build tools (lex, yacc, etc)
# ADD CHECKS FOR ANY REQUIRED PROGRAMS HERE
#AC_CHECK_PROG(PS,ps,[ps -e --no-headers])

# Check for required header files
# ADD CHECKS FOR ANY REQUIRED NON-CMPI HEADER FILES HERE
AC_CHECK_HEADERS([stdarg.h signal.h errno.h])

# Check for required library functions
# ADD CHECKS FOR ANY REQUIRED NON_CMPI LIBRARY FUNCTIONS HERE
AC_CHECK_FUNCS([popen pclose fgets kill vfprintf fprintf sprintf sscanf])

CPPFLAGS="$CPPFLAGS -DCMPI_VERSION=90"

# Configure command line options
AC_ARG_VAR([CIMSERVER],[the target CIM server (pegasus|sfcb|openwbem|sniacimom).])
AC_ARG_VAR([PROVIDERDIR],[the directory where the CMPI providers will be installed.])
AC_ARG_VAR([TESTSUITEDIR],[the directory where the SBLIM testsuite is installed.])

# Autogenerate the autoconf header file to store build settings
AC_CONFIG_HEADER([config.h])

# Autogenerate the Makefile
AC_CONFIG_FILES([Makefile
	src/Makefile
	src/lib/Makefile
	src/include/Makefile
	src/providers/Makefile
	src/providers/server-profile/Makefile
	src/providers/Array-Profile/Makefile
	src/providers/copy-services-profile/Makefile
])
AM_INIT_AUTOMAKE

# Check for the required CPP compiler
AC_PROG_CXX([g++])

# Check for the required C compiler
AC_PROG_CC

# Check if the C compiler supports 'const'
AC_C_CONST

# Check for the required libtool
AC_PROG_LIBTOOL

# Check for the required install program 
AC_PROG_INSTALL

# Check for some common required headers
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Check if the system headers conform to ANSI C
AC_HEADER_STDC

# Check for the required CMPI header files (this macro is defined in acinclude.m4)
CHECK_CMPI

# Check for the required EVMS header files (this macro is defined in acinclude.m4)
#CHECK_EVMS

# Check for the required YAST2 header files (this macro is defined in acinclude.m4)
CHECK_YAST2

# Check for where to install CMPI libraries (this macro is defined in acinclude.m4)
CHECK_PROVIDERDIR

# Check for presense of a CIM server (this macro is defined in acinclude.m4)
CHECK_CIMSERVER

# Check for the OMC-SMI testsuite (this macro is defined in acinclude.m4)
CHECK_TESTSUITE
if test x"$TESTSUITEDIR" == x; then
   TESTSUITE_SUPPORT=Disabled
else
   TESTSUITE_SUPPORT=Enabled
fi

case $host_cpu in
	x86_64)
		LIBDIR='lib64'
	;;
	amd64)
		LIBDIR='lib64'
	;;
	*)
		LIBDIR='lib'
	;;
esac

AC_SUBST(LIBDIR)

# Display configuration options
echo "----------------------------------------------------------"
echo "Configuration for $PACKAGE complete."
echo ""
echo "The following configuration options have been selected:"
echo "   CIMSERVER: " $CIMSERVER
echo "   PROVIDERDIR: " $PROVIDERDIR
echo "   TESTSUITE_SUPPORT: " $TESTSUITE_SUPPORT
if test x"$TESTSUITEDIR" != x; then
   echo "   TESTSUITEDIR: " $TESTSUITEDIR
fi
echo "   CPPFLAGS:" $CPPFLAGS
echo
echo "You can override these values by setting the corresponding"
echo "configure command-line option(s); e.g."
echo "   ./configure PROVIDERDIR=/home/me/lib/omc/"
echo "----------------------------------------------------------"

# Generate configure scripts for the Makefile
AC_OUTPUT

echo "You may now run make"

