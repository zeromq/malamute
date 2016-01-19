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
Version:        0.0.0
Release:        1
Summary:        zeromq message broker
License:        MIT
URL:            http://example.com/
Source0:        %{name}-%{version}.tar.gz
Group:          System/Libraries
BuildRequires:  automake
BuildRequires:  autoconf
BuildRequires:  libtool
BuildRequires:  pkg-config
BuildRequires:  systemd-devel
BuildRequires:  libsodium-devel
BuildRequires:  zeromq-devel
BuildRequires:  czmq-devel
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
malamute zeromq message broker.

%package -n libmlm0
Group:          System/Libraries
Summary:        zeromq message broker

%description -n libmlm0
malamute zeromq message broker.
This package contains shared library.

%post -n libmlm0 -p /sbin/ldconfig
%postun -n libmlm0 -p /sbin/ldconfig

%files -n libmlm0
%defattr(-,root,root)
%doc COPYING
%{_libdir}/libmlm.so.*

%package devel
Summary:        zeromq message broker
Group:          System/Libraries
Requires:       libmlm0 = %{version}
Requires:       libsodium-devel
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

%prep
%setup -q

%build
sh autogen.sh
%{configure} --with-systemd
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
%{_bindir}/mshell
%{_bindir}/mlm_tutorial
%{_bindir}/mlm_perftest
%{_prefix}/lib/systemd/system/malamute*.service


%changelog
