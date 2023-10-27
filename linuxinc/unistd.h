/* Copyright (C) 1991-2016 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */


#ifndef	_UNISTD_H
#define	_UNISTD_H	1

#define basename(x) x

#define CP_UTF8 65001
#define __USE_MISC 1
#define __BIG_ENDIAN 0x0100
#define __LITTLE_ENDIAN 0x0001
#define __BYTE_ORDER __LITTLE_ENDIAN
#define __extension__

#define u_int32_t uint32_t
#define u_int16_t uint16_t
#define u_int8_t uint8_t

int getline(char **lineptr, size_t *n, FILE *stream);	

#endif /* unistd.h  */
