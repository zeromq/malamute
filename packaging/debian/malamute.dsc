Format: 3.0 (quilt)
Binary: malamute
Source: malamute
Version: 1.2.0-0.1
Maintainer: malamute Developers <zeromq-dev@lists.zeromq.org>
Architecture: any
Build-Depends: debhelper (>= 9),
    pkg-config,
    libzmq3-dev,
    libczmq-dev,
    systemd,
    dh-python <!nopython>,
    python3-all-dev <!nopython>, python3-cffi <!nopython>, python3-setuptools <!nopython>,
    asciidoc-base | asciidoc, xmlto,
    dh-autoreconf

Files:
 7697688bf65a35bc33ae2db51ebb0e3b 818110 malamute.tar.gz
