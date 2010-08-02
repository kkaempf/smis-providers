#
# spec file for package smis-providers (Version 1.0.0)
#
# Copyright (c) 2009 SUSE LINUX Products GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#



Name:           smis-providers
License:        Other uncritical OpenSource License
Group:          System/Management
Version:        1.0.0
Release:        10
Summary:        SMI-S providers for Volume Management & Snapshot
Url:            http://www.novell.com
BuildRequires:  automake cmpi-provider-register dos2unix gcc-c++ libsblim-cmpiutil1 sblim-cmpi-base-devel sblim-cmpi-devel sblim-cmpiutil-devel sblim-sfcb
%if 0%{?suse_version} < 1120
BuildRequires:  yast2-storage-devel
%else
Patch0:         libstorage-splitt-off.patch
BuildRequires:  libstorage-devel
%endif
#BuildRequires:  autoconf automake gcc gcc-c++ sblim-sfcb libsblim-cmpiutil1 sblim-cmpi-base sblim-cmpi-devel sblim-cmpiutil-devel yast2-storage yast2-storage-devel
Patch1:         fix_indication_prototype.diff
Requires:       sblim-sfcb sblim-cmpi-base
Source:         %{name}.tar.bz2
AutoReqProv:    on
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
PreReq:         /sbin/ldconfig /usr/sbin/cmpi-provider-register

%description
The smis-providers contains the providers to instrument the Volumes &
Snapshots on the Linux filesystem. These are based on SNIA's SMI-S
Volume Management profile & Copy Services profile respectively



Authors:
--------
    rramkumar@novell.com
    rkaur@novell.com
    dvijayababu@novell.com

%prep
%setup -n smis-providers
%if 0%{?suse_version} >= 1120
%patch0 -p1
%endif
%patch1 -p1

%build
sh bootstrap.sh
dos2unix README AUTHORS COPYING
#export CFLAGS=${RPM_OPT_FLAGS}
./configure PROVIDERDIR=/usr/%{_lib}
make

%install
#%{__rm} -rf ${RPM_BUILD_ROOT}
dos2unix README AUTHORS COPYING
make DESTDIR=${RPM_BUILD_ROOT} install
#find ${RPM_BUILD_ROOT} -iname "*.so" | xargs --no-run-if-empty rm -v
find ${RPM_BUILD_ROOT} -iname "*.a" | xargs --no-run-if-empty rm -v
find ${RPM_BUILD_ROOT} -iname "*.la" | xargs --no-run-if-empty rm -v
#mkdir -p ${RPM_BUILD_ROOT}/usr/%_lib/
#mkdir -p ${RPM_BUILD_ROOT}/var/%_lib/
#mkdir -p ${RPM_BUILD_ROOT}/var/%_lib/sfcb/stage/mofs/root/
#mkdir -p ${RPM_BUILD_ROOT}/var/%_lib/sfcb/stage/mofs/root/cimv2/
#mkdir -p ${RPM_BUILD_ROOT}/var/%_lib/sfcb/stage/regs/
#
#
#install -p -m 644 src/providers/server-profile/.libs/libomc_smi_server.so ${RPM_BUILD_ROOT/}/usr/%_lib/
#install -p -m 644 src/providers/Array-Profile/.libs/libomc_smi_array.so ${RPM_BUILD_ROOT/}/usr/%_lib/
#install -p -m 644 src/providers/copy-services-profile/.libs/libomc_smi_snapshot.so ${RPM_BUILD_ROOT/}/usr/%_lib/
#install -d ${RPM_BUILD_ROOT}/var/%_lib/sfcb/stage/mofs/root/cimv2/
#install -p -m     src/providers/server-profile/mof ${RPM_BUILD_ROOT/}/var/%_lib/sfcb/stage/mofs/root/cimv2/
#install -p -m     src/providers/Array-Profile/mof ${RPM_BUILD_ROOT/}/var/%_lib/sfcb/stage/mofs/root/cimv2/
#install -p -m     src/providers/copy-services-profile/mof ${RPM_BUILD_ROOT/}/var/%_lib/sfcb/stage/mofs/root/cimv2/
#install -d ${RPM_BUILD_ROOT}/var/%_lib/sfcb/stage/regs/
#install -p -m     src/providers/server-profile/reg ${RPM_BUILD_ROOT/}/var/%_lib/sfcb/stage/regs
#install -p -m     src/providers/Array-Profile/reg ${RPM_BUILD_ROOT/}/var/%_lib/sfcb/stage/regs
#install -p -m     src/providers/copy-services-profile/reg ${RPM_BUILD_ROOT/}/var/%_lib/sfcb/stage/regs

%clean
%{__rm} -rf ${RPM_BUILD_ROOT}

%pre
if [ $1 -gt 1 ]; then
/usr/sbin/cmpi-provider-register -r -x -d /usr/share/mof/smis-providers
fi

%post
sfcbrepos -f
/sbin/ldconfig
/usr/sbin/cmpi-provider-register -d /usr/share/mof/smis-providers

%postun 
/sbin/ldconfig

%preun
if [ "$1" = "0" ] ; then
/usr/sbin/cmpi-provider-register -r -d /usr/share/mof/smis-providers
fi

%files
%defattr(-,root,root)
%doc README AUTHORS COPYING
%dir %attr(755,root,root)
#%dir /usr/%_lib/
/usr/%_lib/libomc_smi_server.so
/usr/%_lib/libomc_smi_array.so
/usr/%_lib/libomc_smi_snapshot.so
%dir /usr/share/mof/
%dir /usr/share/mof/smis-providers/
/usr/share/mof/smis-providers/deploy.mof
/usr/share/mof/smis-providers/*
%config /etc/smsetup.conf
#%dir /var/lib/
#%dir /var/lib/sfcb/
#%dir /var/lib/sfcb/stage/
#%dir /var/lib/sfcb/stage/mofs/
#%dir /var/lib/sfcb/stage/mofs/root/
#%dir /var/lib/sfcb/stage/mofs/root/cimv2
#%dir /var/lib/sfcb/stage/mofs/root/interop
#%dir /var/lib/sfcb/stage/regs/
#/var/lib/sfcb/stage/mofs/root/cimv2/*
#/var/lib/sfcb/stage/mofs/root/interop/*
#/usr/%_lib/libomc_smi_array.a
#/usr/%_lib/libomc_smi_array.la
#/usr/%_lib/libomc_smi_array.so.1
#/usr/%_lib/libomc_smi_array.so.1.0.0
#/usr/%_lib/libomc_smi_server.a
#/usr/%_lib/libomc_smi_server.la
#/usr/%_lib/libomc_smi_server.so.1
#/usr/%_lib/libomc_smi_server.so.1.0.0
#/usr/%_lib/libomc_smi_snapshot.a
#/usr/%_lib/libomc_smi_snapshot.la
#/usr/%_lib/libomc_smi_snapshot.so.1
#/usr/%_lib/libomc_smi_snapshot.so.1.0.0
#/var/lib/sfcb/stage/regs/Array.reg
#/var/lib/sfcb/stage/regs/CopyServices.reg
#/var/lib/sfcb/stage/regs/Server.reg

%changelog
* Fri Jan 30 2009 rkaur@suse.de
- Added missing dependencies to the SPEC file
* Fri Jan 23 2009 rkaur@suse.de
- Added fix for Bug 467346 which takes care of ignoring association calls from other libraries.
* Wed Jan 21 2009 rkaur@suse.de
- Added a fix for bug 467974 to allow unregistering the provider during uninstallation.
* Fri Oct 24 2008 rkaur@suse.de
- Added fixes for some bugs and changed the spec file to copy the sample CONF file to the DOC folder
* Mon Sep 15 2008 rkaur@suse.de
- Made changes to SPEC file to resolve dependencies mentioned in Bug : 426344
* Fri Sep 12 2008 rkaur@suse.de
- Changed the Makefile.am to accomodate the generation of only the .so and not versioned shared objects
* Fri Sep 12 2008 rkaur@suse.de
-Initial checkin
