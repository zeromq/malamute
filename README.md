# Malamute

All the enterprise messaging patterns in one box.

![Malamute](https://github.com/malamute/malamute-core/blob/master/malamute.jpg)

[Read the whitepaper](MALAMUTE.md)

[Protocol wireframe](https://github.com/malamute/malamute-core/blob/master/src/mlm_msg.bnf)

[Stream protocol](STREAM.md)

## Building Malamute

To use or contribute to Malamute, build and install this stack:

    git clone git://github.com/jedisct1/libsodium.git
    git clone git://github.com/zeromq/libzmq.git
    git clone git://github.com/zeromq/czmq.git
    git clone git://github.com/zeromq/zyre.git
    git clone git://github.com/malamute/malamute.git
    for project in libsodium libzmq czmq zyre malamute; do
        cd $project
        ./autogen.sh
        ./configure && make check
        sudo make install
        sudo ldconfig
        cd ..
    done

To run Malamute, issue this command:

    malamute [name]

Where 'name' is the name of the Malamute instance, and must be unique on any given host. The default name is 'local'. To end the broker, send a TERM or INT signal (Ctrl-C).

## How to Help

1. Use Malamute in a real project.
2. Identify problems that you face, using it.
3. Work with us to fix the problems, or send us patches.

## Ownership and Contributing

The contributors are listed in AUTHORS. This project uses the MPL v2 license, see LICENSE.

The contribution policy is the standard ZeroMQ [C4.1 process](http://rfc.zeromq.org/spec:22). Please read this RFC if you have never contributed to a ZeroMQ project.
