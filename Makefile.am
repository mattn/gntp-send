ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -Wall -Wextra -Wno-missing-field-initializers -pedantic -I include
AM_CXXFLAGS =

nodist_include_HEADERS = include/growl.h

lib_LTLIBRARIES = libgrowl.la

# libgrowl.la
libgrowl_la_CFLAGS = $(AM_CFLAGS) -DGROWL_STATIC -Wno-format-zero-length
libgrowl_la_SOURCES = src/growl.c \
					  src/tcp.c \
					  src/md5.c

bin_PROGRAMS = gntp-send
noinst_HEADERS = include/growl.h \
				 include/md5.h \
				 include/tcp.h

# gntp-send
gntp_send_CFLAGS = $(AM_CFLAGS)
gntp_send_SOURCES = src/gntp-send.c
gntp_send_LDADD = libgrowl.la

EXTRA_DIST = VERSION README.md

# vim: noexpandtab sw=4 ts=4
