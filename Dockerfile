FROM ubuntu:trusty

MAINTAINER Trevor Bernard <trevor.bernard@gmail.com>

RUN apt-get update && \
    apt-get install -y uuid-dev build-essential git-core libtool autotools-dev autoconf automake pkg-config unzip libkrb5-dev && \
    mkdir -p /tmp/malamute && \
    cd /tmp/malamute && \
    git clone git://github.com/jedisct1/libsodium.git && \
    ( cd libsodium; ./autogen.sh; ./configure; make check; make install; ldconfig; cd .. ) && \
    git clone git://github.com/zeromq/libzmq.git && \
    ( cd libzmq; ./autogen.sh; ./configure; make check; make install; ldconfig; cd .. ) && \
    git clone git://github.com/zeromq/czmq.git && \
    ( cd czmq; ./autogen.sh; ./configure; make check; make install; ldconfig; cd .. ) && \
    git clone git://github.com/zeromq/malamute.git && \
    ( cd malamute; ./autogen.sh; ./configure; make check; make install; ldconfig; cd .. ) && \

    rm -rf /tmp/malamute

CMD ["malamute"]
