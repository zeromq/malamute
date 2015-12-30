Format:         1.0
Source:         Malamute
Version:        0.0.0-1
Binary:         libmlm0, Malamute-dev
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
 Malamute-dev deb libdevel optional arch=any

