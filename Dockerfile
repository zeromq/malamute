FROM ubuntu:trusty

MAINTAINER Benjamin Henrion <zoobab@gmail.com>

RUN apt-get update && \
    apt-get install -y uuid-dev build-essential git-core libtool unzip && \
    apt-get install -y autotools-dev autoconf automake pkg-config libkrb5-dev && \
    mkdir -p /tmp/malamute && \
    cd /tmp/malamute && \
    git clone https://github.com/jedisct1/libsodium && \
    ( cd libsodium; ./autogen.sh; ./configure; make check; make install; ldconfig; cd .. ) && \
    git clone https://github.com/zeromq/libzmq && \
    ( cd libzmq; ./autogen.sh; ./configure; make check; make install; ldconfig; cd .. ) && \
    git clone https://github.com/zeromq/czmq && \
    ( cd czmq; ./autogen.sh; ./configure; make check; make install; ldconfig; cd .. ) && \
    git clone git://github.com/zeromq/malamute.git && \
    ( cd malamute; ./autogen.sh; ./configure; make check; make install; ldconfig; cd .. ) && \
    cd ../.. && \
    rm -rf /tmp/malamute

CMD ["malamute"]
