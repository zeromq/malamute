#
#    malamute - ZeroMQ Message Broker
#
#    Copyright (c) the Contributors as noted in the AUTHORS file.       
#    This file is part of the Malamute Project.                         
#                                                                       
#    This Source Code Form is subject to the terms of the Mozilla Public
#    License, v. 2.0. If a copy of the MPL was not distributed with this
#    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
#

Name:           malamute
Version:        1.1.0
Release:        1
Summary:        zeromq message broker
License:        MIT
URL:            https://github.com/zeromq/malamute
Source0:        %{name}-%{version}.tar.gz
Group:          System/Libraries
# Note: ghostscript is required by graphviz which is required by
#       asciidoc. On Fedora 24 the ghostscript dependencies cannot
#       be resolved automatically. Thus add working dependency here!
BuildRequires:  ghostscript
BuildRequires:  asciidoc
BuildRequires:  automake
BuildRequires:  autoconf
BuildRequires:  libtool
BuildRequires:  pkgconfig
BuildRequires:  systemd-devel
BuildRequires:  systemd
%{?systemd_requires}
BuildRequires:  xmlto
BuildRequires:  zeromq-devel
BuildRequires:  czmq-devel
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
malamute zeromq message broker.

%package -n libmlm1
Group:          System/Libraries
Summary:        zeromq message broker

%description -n libmlm1
malamute zeromq message broker.
This package contains shared library.

%post -n libmlm1 -p /sbin/ldconfig
%postun -n libmlm1 -p /sbin/ldconfig

%files -n libmlm1
%defattr(-,root,root)
%{_libdir}/libmlm.so.*

%package devel
Summary:        zeromq message broker
Group:          System/Libraries
Requires:       libmlm1 = %{version}
Requires:       zeromq-devel
Requires:       czmq-devel

%description devel
malamute zeromq message broker.
This package contains development files.

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/libmlm.so
%{_libdir}/pkgconfig/libmlm.pc
%{_mandir}/man3/*
%{_datadir}/zproject/
%{_datadir}/zproject/malamute/

%prep
%setup -q

%build
sh autogen.sh
%{configure} --with-systemd-units
make %{_smp_mflags}

%install
make install DESTDIR=%{buildroot} %{?_smp_mflags}

# remove static libraries
find %{buildroot} -name '*.a' | xargs rm -f
find %{buildroot} -name '*.la' | xargs rm -f

%files
%defattr(-,root,root)
%doc README.md
%{_bindir}/malamute
%{_mandir}/man1/malamute*
%{_bindir}/mshell
%{_mandir}/man1/mshell*
%{_bindir}/mlm_tutorial
%{_mandir}/man1/mlm_tutorial*
%{_bindir}/mlm_perftest
%{_mandir}/man1/mlm_perftest*
%config(noreplace) %{_sysconfdir}/malamute/malamute.cfg
/usr/lib/systemd/system/malamute.service
%dir %{_sysconfdir}/malamute
%if 0%{?suse_version} > 1315
%post
%systemd_post malamute.service
%preun
%systemd_preun malamute.service
%postun
%systemd_postun_with_restart malamute.service
%endif

%changelog
