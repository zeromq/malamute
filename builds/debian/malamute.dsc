Format:         1.0
Source:         malamute
Version:        0.1.0-1
Binary:         libmlm0, malamute-dev
Architecture:   any all
Maintainer:     John Doe <John.Doe@example.com>
Standards-Version: 3.9.5
Build-Depends: bison, debhelper (>= 8),
    pkg-config,
    automake,
    autoconf,
    libtool,
    libsodium-dev,
    libzmq4-dev,
    libczmq-dev,
    dh-autoreconf

Package-List:
 libmlm0 deb net optional arch=any
 malamute-dev deb libdevel optional arch=any

