#
# This is experimental meson build script for malamute
# written by hand in order to learn as much of it to replace
# autotools backend
#
# TODO:
#  * unit test fail
#  * do not install pc files et all
#  * do not know concept of DRAFTS
#  * cannot add DRAFT macros to config.h
#  * do not install systemd files (which might be good BTW)
#  * you can't turn binaries on and off
#  * does not have optional dependencies
#  * don't have an ability to call (make code)
#  * no integration with Travis
#  * no memcheck target
#
project ('malamute', 'c')

# dependencies
zmqdep = dependency ('libzmq', version: '>=4.2.0')
czmqdep = dependency ('libczmq', version: '>=3.0.0')

LIBS = [
    zmqdep,
    czmqdep
    ]

# shared library sources
MLM_SRC = [
    'src/mlm_msg.c',
    'src/mlm_stream_simple.c',
    'src/mlm_mailbox_simple.c',
    'src/mlm_mailbox_bounded.c',
    'src/platform.h',
    'src/mlm_proto.c',
    'src/mlm_server.c',
    'src/mlm_client.c',
    'src/mlm_selftest.c',
    'src/mlm_private_selftest.c',
    ]

libmlm = shared_library ('mlm', MLM_SRC, version: '1.0.0', soversion: '0', dependencies: LIBS)

executable ('malamute', 'src/malamute.c', link_with: [libmlm], dependencies: LIBS)
mlm_selftest = executable ('mlm_selftest', 'src/mlm_selftest.c', link_with: [libmlm], dependencies: LIBS)
test ('mlm_selftest', mlm_selftest)
