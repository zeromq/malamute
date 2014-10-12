lib_LTLIBRARIES = libmlm.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmlm.pc

include_HEADERS = \
	../include/malamute.h \
	../include/mlm_msg.h \
	../include/mlm_server.h \
	../include/mlm_client.h

libmlm_la_SOURCES = \
	mlm_msg.c \
	mlm_server.c \
	mlm_client.c

AM_CFLAGS = -g
AM_CPPFLAGS = -I$(top_srcdir)/include
bin_PROGRAMS = malamute
noinst_PROGRAMS = mlm_selftest

malamute_LDADD = libmlm.la
mlm_selftest_LDADD = libmlm.la

libmlm_la_LDFLAGS = -version-info @LTVER@

TESTS = mlm_selftest

# Produce generated models; do this manually in src directory
code:
	gsl -q mlm_msg.xml
	gsl -q mlm_server.xml
	gsl -q mlm_client.xml
