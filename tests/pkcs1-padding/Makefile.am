## Process this file with automake to produce Makefile.in
# Copyright (C) 2006, 2008, 2010, 2012 Free Software Foundation, Inc.
#
# Author: Simon Josefsson
#
# This file is part of GnuTLS.
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This file is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this file; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

# For pkcs1-pad
EXTRA_DIST = pkcs1-pad-ok.pem pkcs1-pad-broken.pem \
	pkcs1-pad-ok2.pem pkcs1-pad-broken2.pem \
	pkcs1-pad-broken3.pem

dist_check_SCRIPTS = pkcs1-pad

if !WINDOWS
TESTS = pkcs1-pad
endif

TESTS_ENVIRONMENT = EXEEXT=$(EXEEXT)
