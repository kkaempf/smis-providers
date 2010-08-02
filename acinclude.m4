dnl
dnl $Id: acinclude.m4,v 1.1.1.1 2005/04/28 21:05:10 bestorga-oss Exp $
dnl
 dnl 
 dnl (C) Copyright IBM Corp. 2004, 2005
 dnl
 dnl THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE
 dnl ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 dnl CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 dnl
 dnl You can obtain a current copy of the Common Public License from
 dnl  http://www.opensource.org/licenses/cpl1.0.php
 dnl
 dnl Author:       Konrad Rzeszutek <konradr@us.ibm.com>
 dnl Contributors: Viktor Mihajlovski <mihajlov@de.ibm.com>
 dnl Date  :	      09/20/2004
dnl
dnl
dnl CHECK_CMPI: Check for CMPI headers and set the CPPFLAGS
dnl with the -I<directory>
dnl 
dnl CHECK_PEGASUS_2_3_2: Check for Pegasus 2.3.2 and set
dnl the HAVE_PEGASUS_2_3_2 
dnl flag 
dnl

AC_DEFUN([CHECK_PEGASUS_2_3_2],
	[
	AC_MSG_CHECKING(for Pegasus 2.3.2)
	if which cimserver > /dev/null 2>&1 
	then
	   test_CIMSERVER=`cimserver -v`
	fi	
	if test "$test_CIMSERVER" == "2.3.2"; then
		AC_MSG_RESULT(yes)		
		AC_DEFINE_UNQUOTED(HAVE_PEGASUS_2_3_2,1,[Defined to 1 if Pegasus 2.3.2 is used])
	else
		AC_MSG_RESULT(no)

	fi
	]
)

dnl
dnl CHECK_PEGASUS_2_4: Check for Pegasus 2.4 and set the
dnl the -DPEGASUS_USE_EXPERIMENTAL_INTERFACES flag
dnl
AC_DEFUN([CHECK_PEGASUS_2_4],
	[
	AC_MSG_CHECKING(for Pegasus 2.4)
	if which cimserver > /dev/null 2>&1 
	then
	   test_CIMSERVER=`cimserver -v`
	fi	
	if test "$test_CIMSERVER" == "2.4"; then
		AC_MSG_RESULT(yes)		
		CPPFLAGS="$CPPFLAGS -DPEGASUS_USE_EXPERIMENTAL_INTERFACES"
		AC_DEFINE_UNQUOTED(HAVE_PEGASUS_2_4,1,[Defined to 1 if Pegasus 2.4 is used])
	else
		AC_MSG_RESULT(no)

	fi
	]
)
	
	
dnl
dnl Helper functions
dnl
AC_DEFUN([_CHECK_CMPI],
	[
	AC_MSG_CHECKING($1)
	AC_TRY_LINK(
	[
		#include <cmpimacs.h>
		#include <cmpidt.h>
		#include <cmpift.h>
	],
	[
		CMPIBroker broker;
		CMPIStatus status = {CMPI_RC_OK, NULL};
		CMPIString *s = CMNewString(&broker, "TEST", &status);
	],
	[
		have_CMPI=yes
		dnl AC_MSG_RESULT(yes)
	],
	[
		have_CMPI=no
		dnl AC_MSG_RESULT(no)
	])

])

AC_DEFUN([_CHECK_INDHELP_HEADER],
	[
	AC_MSG_CHECKING($1)
	AC_TRY_COMPILE(
	[
                #include <stdio.h>
		#include <ind_helper.h>
	],
	[
		CMPISelectExp *filter = NULL;
	        ind_set_select("/root/cimv2",NULL,filter);
	],
	[
		have_INDHELP=yes
		dnl AC_MSG_RESULT(yes)
	],
	[
		have_INDHELP=no
		dnl AC_MSG_RESULT(no)
	])

])

dnl
dnl The main function to check for CMPI headers
dnl Modifies the CPPFLAGS with the right include directory and sets
dnl the 'have_CMPI' to either 'no' or 'yes'
dnl

AC_DEFUN([CHECK_CMPI],
	[
	AC_MSG_CHECKING(for CMPI headers)
	dnl Check just with the standard include paths
	CMPI_CPP_FLAGS="$CPPFLAGS"
	_CHECK_CMPI(standard)
	if test "$have_CMPI" == "yes"; then
		dnl The standard include paths worked.
		AC_MSG_RESULT(yes)
	else
	  _DIRS_="/usr/include/cmpi \
                  /usr/local/include/cmpi \
		  $PEGASUS_ROOT/src/Pegasus/Provider/CMPI \
		  /opt/tog-pegasus/include/Pegasus/Provider/CMPI \
		  /usr/include/Pegasus/Provider/CMPI \
		  /usr/include/openwbem \
		  /usr/sniacimom/include"
	  for _DIR_ in $_DIRS_
	  do
		 _cppflags=$CPPFLAGS
		 _include_CMPI="$_DIR_"
		 CPPFLAGS="$CPPFLAGS -I$_include_CMPI"
		 _CHECK_CMPI($_DIR_)
		 if test "$have_CMPI" == "yes"; then
		  	dnl Found it
		  	AC_MSG_RESULT(yes)
			dnl Save the new -I parameter  
			CMPI_CPP_FLAGS="$CPPFLAGS"
			break
		 fi
	         CPPFLAGS=$_cppflags
	  done
	fi	
	CPPFLAGS="$CMPI_CPP_FLAGS"	
	if test "$have_CMPI" == "no"; then
		AC_MSG_ERROR(no. Sorry cannot find CMPI headers files.)
	fi
	]
)

dnl
dnl The main function to check for the indication_helper header.
dnl Modifies the CPPFLAGS with the right include directory and sets
dnl the 'have_INDHELP' to either 'no' or 'yes'
dnl

AC_DEFUN([CHECK_INDHELP_HEADER],
	[
        INDHELP_CPP_FLAGS="$CPPFLAGS"
	AC_MSG_CHECKING(for indication helper header)
	dnl Check just with the standard include paths
	_CHECK_INDHELP_HEADER(standard)
	if test "$have_INDHELP" == "yes"; then
		dnl The standard include paths worked.
		AC_MSG_RESULT(yes)
	else
	  _DIRS_="/usr/include/sblim \
                  /usr/local/include/sblim"
	  for _DIR_ in $_DIRS_
	  do
		 _cppflags=$CPPFLAGS
		 _include_INDHELP="$_DIR_"
		 CPPFLAGS="$CPPFLAGS -I$_include_INDHELP"
		 _CHECK_INDHELP_HEADER($_DIR_)
		 if test "$have_INDHELP" == "yes"; then
		  	dnl Found it
		  	AC_MSG_RESULT(yes)
			dnl Save the new -I parameter  
			INDHELP_CPP_FLAGS="$CPPFLAGS"
			break
		 fi
	         CPPFLAGS=$_cppflags
	  done
	fi	
	CPPFLAGS="$INDHELP_CPP_FLAGS"	
	if test "$have_INDHELP" == "no"; then
		AC_MSG_RESULT(no)
        fi
	]
)

dnl
dnl The check for the CMPI provider directory
dnl Sets the PROVIDERDIR  variable.
dnl

AC_DEFUN([CHECK_PROVIDERDIR],
	[
	AC_MSG_CHECKING(for CMPI provider directory)
	_DIRS="$libdir/cmpi"
	save_exec_prefix=${exec_prefix}
	save_prefix=${prefix}
	if test xNONE == x${prefix}; then
		prefix=/usr/local
	fi
	if test xNONE == x${exec_prefix}; then
		exec_prefix=$prefix
	fi
	for _dir in $_DIRS
	do
		_xdir=`eval echo $_dir`
		AC_MSG_CHECKING( $_dir )
		if test -d $_xdir ; then
		  dnl Found it
		  AC_MSG_RESULT(yes)
		  if test x"$PROVIDERDIR" == x ; then
			PROVIDERDIR=$_dir
		  fi
		  break
		fi
        done
	if test x"$PROVIDERDIR" == x ; then
		PROVIDERDIR="$libdir"/cmpi
		AC_MSG_RESULT(implied: $PROVIDERDIR)
	fi
	exec_prefix=$save_exec_prefix
	prefix=$save_prefix
	]
)

dnl
dnl The "check" for the CIM server type
dnl Sets the CIMSERVER variable.
dnl

AC_DEFUN([CHECK_CIMSERVER],
	[
	AC_MSG_CHECKING(for CIM servers)
	_SERVERS="sfcbd cimserver owcimomd"
	for _name in $_SERVERS
	do
	 	AC_MSG_CHECKING( $_name )
	        which $_name > /dev/null 2>&1
		if test $? == 0 ; then
		  dnl Found it
		  AC_MSG_RESULT(yes)
		  if test x"$CIMSERVER" == x ; then
			case $_name in
			   sfcbd) CIMSERVER=sfcb;;
			   cimserver) CIMSERVER=pegasus;;
			   owcimomd) CIMSERVER=openwbem;;
			esac
		  fi
		  break;
		fi
        done
	if test x"$CIMSERVER" == x ; then
		CIMSERVER=sfcb
		AC_MSG_RESULT(implied: $CIMSERVER)
	fi
	]
)

dnl
dnl The check for the OMC SMI test suite
dnl Sets the TESTSUITEDIR variable and the TESTSUITE conditional
dnl

AC_DEFUN([CHECK_TESTSUITE],
	[
	AC_MSG_CHECKING(for OMC SMI testsuite)
	_DIRS="$datadir/omc-smi-testsuite"
	save_exec_prefix=${exec_prefix}
	save_prefix=${prefix}
	if test xNONE == x${prefix}; then
		prefix=/usr/local
	fi
	if test xNONE == x${exec_prefix}; then
		exec_prefix=$prefix
	fi
	for _name in $_DIRS
	do
	 	AC_MSG_CHECKING( $_name )
		_xname=`eval echo $_name`
		if test -x $_xname/run.sh ; then
		  dnl Found it
		  AC_MSG_RESULT(yes)
		  if test x"$TESTSUITEDIR" == x; then
		  	TESTSUITEDIR=$_name
		  fi
		  AC_SUBST(TESTSUITEDIR)
		  break;
		fi
        done
	if test x"$TESTSUITEDIR" == x ; then
		AC_MSG_RESULT(no)
	fi
	AM_CONDITIONAL(TESTSUITE,[test x"$TESTSUITEDIR" != x])
	exec_prefix=$save_exec_prefix
	prefix=$save_prefix
	]
)

dnl
dnl The main function to check for the cmpi-base common header
dnl Modifies the CPPFLAGS with the right include directory and sets
dnl the 'have_SBLIMBASE' to either 'no' or 'yes'
dnl

AC_DEFUN([CHECK_SBLIM_BASE],
	[
	AC_MSG_CHECKING(for SBLIM Base)
        SBLIMBASE_CPP_FLAGS="$CPPFLAGS"
	dnl Check just with the standard include paths
	_CHECK_SBLIM_BASE(standard)
	if test "$have_SBLIMBASE" == "yes"; then
		dnl The standard include paths worked.
		AC_MSG_RESULT(yes)
	else
	  _DIRS_="/usr/include/sblim \
                  /usr/local/include/sblim"
	  for _DIR_ in $_DIRS_
	  do
		 _cppflags=$CPPFLAGS
		 _include_SBLIMBASE="$_DIR_"
		 CPPFLAGS="$CPPFLAGS -I$_include_SBLIMBASE"
		 _CHECK_SBLIM_BASE($_DIR_)
		 if test "$have_SBLIMBASE" == "yes"; then
		  	dnl Found it
		  	AC_MSG_RESULT(yes)
			dnl Save the new -I parameter  
			SBLIMBASE_CPP_FLAGS="$CPPFLAGS"
			LIBSBLIMBASE=-lcmpiOSBase_Common	
			break
		 fi
	         CPPFLAGS=$_cppflags
	  done
	fi	
	CPPFLAGS=$SBLIMBASE_CPP_FLAGS
	AC_SUBST(LIBSBLIMBASE)
	if test "$have_SBLIMBASE" == "no"; then
		AC_MSG_ERROR(no. The required SBLIM Base package is missing.)
        fi
	]
)

dnl
dnl The main function to check for EVMS headers
dnl Modifies the CPPFLAGS with the right include directory and sets
dnl the 'have_EVMS' to either 'no' or 'yes'
dnl

AC_DEFUN([CHECK_EVMS],
	[
	AC_MSG_CHECKING(for EVMS headers)
	CPPFLAGS="$CPPFLAGS -I/usr/include/evms"
	]
)

dnl
dnl The main function to check for YaST2 headers
dnl Modifies the CPPFLAGS with the right include directory and sets
dnl the 'have_YaST2' to either 'no' or 'yes'
dnl

AC_DEFUN([CHECK_YAST2],
	[
	AC_MSG_CHECKING(for Yast2 Storage)
	CPPFLAGS="$CPPFLAGS -I/usr/include/YaST2"
	]
)
