# start: abi/ac-helpers/abi-fribidi.m4
# 
# Copyright (C) 2003 Francis James Franklin
# Copyright (C) 2003 AbiSource, Inc
# 
# This file is free software; you may copy and/or distribute it with
# or without modifications, as long as this notice is preserved.
# This software is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even
# the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE.
#
# The above license applies to THIS FILE ONLY, the abiword code
# itself may be copied and distributed under the terms of the GNU
# GPL, see COPYING for more details
#
# Usage: ABI_FRIBIDI

AC_DEFUN([ABI_GNOMEPRINT],[

dnl detects gnomeprint

GNOMEPRINT_CFLAGS=""
GNOMEPRINT_LIBS=""

PKG_CHECK_MODULES(GNOMEPRINT,[libgnomeprint-2.2 >= 2.2.1 libgnomeprintui-2.2 >= 2.2.1],[
	abi_gnomeprint=yes
],[	AC_MSG_ERROR([Error: libgnomeprint-2.2 >= 2.2.1 libgnomeprintui-2.2 >= 2.2.1 required])
])
GNOMEPRINT_CFLAGS="$GNOMEPRINT_CFLAGS"

AC_SUBST(GNOMEPRINT_CFLAGS)
AC_SUBST(GNOMEPRINT_LIBS)

])
# 
# end: abi/ac-helpers/abi-gnomeprint.m4
# 