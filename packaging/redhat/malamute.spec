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

# To build with draft APIs, use "--with drafts" in rpmbuild for local builds or add
#   Macros:
#   %_with_drafts 1
# at the BOTTOM of the OBS prjconf
%bcond_with drafts
%if %{with drafts}
%define DRAFTS yes
%else
%define DRAFTS no
%endif

# build with python_cffi support enabled
%bcond_with python_cffi
%if %{with python_cffi}
%define py2_ver %(python2 -c "import sys; print ('%d.%d' % (sys.version_info.major, sys.version_info.minor))")
%define py3_ver %(python3 -c "import sys; print ('%d.%d' % (sys.version_info.major, sys.version_info.minor))")
%endif

Name:           malamute
Version:        1.1.0
Release:        1
Summary:        zeromq message broker
License:        MPL-2.0
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
%if %{with python_cffi}
BuildRequires:  python-cffi
BuildRequires:  python-devel
BuildRequires:  python-setuptools
BuildRequires:  python3-devel
BuildRequires:  python3-cffi
BuildRequires:  python3-setuptools
%endif
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
malamute zeromq message broker.

%package -n libmlm1
Group:          System/Libraries
Summary:        zeromq message broker shared library

%description -n libmlm1
This package contains shared library for malamute: zeromq message broker

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
zeromq message broker development tools
This package contains development files for malamute: zeromq message broker

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/libmlm.so
%{_libdir}/pkgconfig/libmlm.pc
%{_mandir}/man3/*
%{_mandir}/man7/*
# Install api files into /usr/local/share/zproject
%dir %{_datadir}/zproject/
%dir %{_datadir}/zproject/malamute
%{_datadir}/zproject/malamute/*.api

%if %{with python_cffi}
%package -n python2-malamute-cffi
Group:  Python
Summary:    Python CFFI bindings for malamute
Requires:  python = %{py2_ver}

%description -n python2-malamute-cffi
This package contains Python CFFI bindings for malamute

%files -n python2-malamute-cffi
%{_libdir}/python%{py2_ver}/site-packages/malamute_cffi/
%{_libdir}/python%{py2_ver}/site-packages/malamute_cffi-*-py%{py2_ver}.egg-info/

%package -n python3-malamute-cffi
Group:  Python
Summary:    Python 3 CFFI bindings for malamute
Requires:  python3 = %{py2_ver}

%description -n python3-malamute-cffi
This package contains Python 3 CFFI bindings for malamute

%files -n python3-malamute-cffi
%{_libdir}/python%{py3_ver}/site-packages/malamute_cffi/
%{_libdir}/python%{py3_ver}/site-packages/malamute_cffi-*-py%{py3_ver}.egg-info/
%endif

%prep
#FIXME: %{error:...} did not worked for me
%if %{with python_cffi}
%if %{without drafts}
echo "FATAL: python_cffi not yet supported w/o drafts"
exit 1
%endif
%endif

%setup -q

%build
sh autogen.sh
%{configure} --enable-drafts=%{DRAFTS} --with-systemd-units
make %{_smp_mflags}

%if %{with python_cffi}
# Problem: we need pkg-config points to built and not yet installed copy of malamute
# Solution: chicken-egg problem - let's make "fake" pkg-config file
sed -e "s@^libdir.*@libdir=`pwd`/src/.libs@" \
    -e "s@^includedir.*@includedir=`pwd`/include@" \
    src/libmlm.pc > bindings/python_cffi/libmlm.pc
cd bindings/python_cffi
export PKG_CONFIG_PATH=`pwd`
python2 setup.py build
python3 setup.py build
%endif

%install
make install DESTDIR=%{buildroot} %{?_smp_mflags}

# remove static libraries
find %{buildroot} -name '*.a' | xargs rm -f
find %{buildroot} -name '*.la' | xargs rm -f

%if %{with python_cffi}
cd bindings/python_cffi
export PKG_CONFIG_PATH=`pwd`
python2 setup.py install --root=%{buildroot} --skip-build --prefix %{_prefix}
python3 setup.py install --root=%{buildroot} --skip-build --prefix %{_prefix}
%endif

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
