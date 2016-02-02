FROM ubuntu:trusty
MAINTAINER Benjamin Henrion <zoobab@gmail.com>

RUN DEBIAN_FRONTEND=noninteractive apt-get update -y -q
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y -q --force-yes uuid-dev build-essential git-core libtool autotools-dev autoconf automake pkg-config unzip libkrb5-dev cmake

RUN useradd -d /home/zmq -m -s /bin/bash zmq
RUN echo "zmq ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/zmq
RUN chmod 0440 /etc/sudoers.d/zmq

USER zmq

WORKDIR /home/zmq
RUN git clone https://github.com/jedisct1/libsodium.git
WORKDIR /home/zmq/libsodium
RUN ./autogen.sh
RUN ./configure
RUN make
RUN sudo make install
RUN sudo ldconfig

WORKDIR /home/zmq
RUN git clone https://github.com/zeromq/libzmq.git
WORKDIR /home/zmq/libzmq
RUN ./autogen.sh
RUN ./configure
RUN make
RUN sudo make install
RUN sudo ldconfig

WORKDIR /home/zmq
RUN git clone https://github.com/zeromq/czmq.git
WORKDIR /home/zmq/czmq
RUN ./autogen.sh
RUN ./configure
RUN make
RUN sudo make install
RUN sudo ldconfig

COPY . /home/zmq/malamute
RUN sudo chown -R zmq:zmq /home/zmq/malamute
WORKDIR /home/zmq/malamute
RUN ./autogen.sh
RUN ./configure
RUN make
RUN sudo make install
RUN sudo ldconfig

#  Custom commands for Dockerfile
EXPOSE 9999
CMD ["/usr/local/bin/malamute", "/home/zmq/malamute/src/malamute.cfg"]

