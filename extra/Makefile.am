## Process this file with automake to produce Makefile.in
# Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
# Free Software Foundation, Inc.
#
# Author: Nikos Mavrogiannopoulos
#
# This file is part of GnuTLS-EXTRA.
#
# GnuTLS-extra is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3 of the
# License, or (at your option) any later version.
#
# GnuTLS-extra is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GnuTLS-EXTRA; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

ACLOCAL_AMFLAGS = -I ../m4 -I ../gl/m4

AM_CFLAGS = $(WERROR_CFLAGS) $(WSTACK_CFLAGS) $(WARN_CFLAGS)
AM_CPPFLAGS = \
	-I$(srcdir)/../gl			\
	-I$(builddir)/../gl			\
	-I$(builddir)/../lib/includes	\
	-I$(srcdir)/../lib/includes	\
	-I$(srcdir)/includes		\
	-I$(builddir)/includes		\
	-I$(srcdir)/../lib		\
	-I$(builddir)/../lib

if ENABLE_MINITASN1
AM_CPPFLAGS += -I$(srcdir)/../lib/minitasn1
endif

SUBDIRS = includes

defexecdir = $(bindir)
defexec_DATA = 


# OpenSSL

libgnutls_openssl_la_LDFLAGS = -no-undefined

if ENABLE_OPENSSL
lib_LTLIBRARIES = libgnutls-openssl.la

libgnutls_openssl_la_SOURCES = gnutls_openssl.c openssl_compat.h	\
	openssl_compat.c

libgnutls_openssl_la_LIBADD = ../gl/libgnu.la $(LIBSOCKET) \
	../lib/libgnutls.la

libgnutls_openssl_la_LDFLAGS += -version-info $(LT_SSL_CURRENT):$(LT_SSL_REVISION):$(LT_SSL_AGE)

if ENABLE_MINITASN1
libgnutls_openssl_la_LIBADD += ../lib/minitasn1/libminitasn1.la
else
libgnutls_openssl_la_LDFLAGS += $(LTLIBTASN1)
endif

if HAVE_LD_OUTPUT_DEF
libgnutls_openssl_la_LDFLAGS += \
	-Wl,--output-def,libgnutls-openssl-$(DLL_VERSION).def
defexec_DATA += libgnutls-openssl-$(DLL_VERSION).def
endif
endif

DISTCLEANFILES = $(defexec_DATA)