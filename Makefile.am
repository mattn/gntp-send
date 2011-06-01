ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -Wall -Wextra -pedantic -I headers
AM_CXXFLAGS =

nodist_include_HEADERS = headers/growl.h \
						 headers/growl++.hpp

lib_LTLIBRARIES = libgrowl.la libgrowl++.la

# libgrowl.la
libgrowl_la_CFLAGS = $(AM_CFLAGS) -DGROWL_STATIC -Wno-format-zero-length
libgrowl_la_SOURCES = source/growl.c \
					  source/tcp.c \
					  source/md5.c

# libgrowl++.la
libgrowl___la_CFLAGS = $(AM_CFLAGS) -DGROWL_STATIC -Wno-format-zero-length
libgrowl___la_CXXFLAGS = $(AM_CFLAGS) $(AM_CXXFLAGS) -DGROWL_STATIC
libgrowl___la_SOURCES = source/growl++.cpp
libgrowl___la_LIBADD = libgrowl.la

bin_PROGRAMS = gntp-send gntp-send++

# gntp-send
gntp_send_CFLAGS = $(AM_CFLAGS)
gntp_send_SOURCES = source/gntp-send.c
gntp_send_LDADD = libgrowl.la

# gntp-send++
gntp_send___CFLAGS = $(AM_CFLAGS)
gntp_send___CXXFLAGS = $(AM_CFLAGS) $(AM_CXXFLAGS)
gntp_send___SOURCES = source/gntp-send++.cpp
gntp_send___LDADD = libgrowl++.la

# vim: noexpandtab sw=4 ts=4
