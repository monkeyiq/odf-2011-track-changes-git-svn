/* AbiWord
 * Copyright (C) 1998 AbiSource, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */ 


#include <windows.h>

#include <stdio.h>
#include <stdarg.h>

#include "ut_debugmsg.h"

// TODO aaaaagh!  This is Win32-specific

void _UT_OutputMessage(char *s, ...)
{
	char sBuf[1024];
	va_list marker;

	va_start(marker, s);

	vsprintf(sBuf, s, marker);

	OutputDebugString(sBuf);
}
