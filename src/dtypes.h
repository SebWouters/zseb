/*
   zseb: Zipping Sequences of Encountered Bytes
   Copyright (C) 2019 Sebastian Wouters

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __ZSEB_DTYPES__
#define __ZSEB_DTYPES__

#define zseb_08_t unsigned char      // c++ guarantees  8-bit at least
#define zseb_16_t unsigned short     // c++ guarantees 16-bit at least
#define zseb_32_t unsigned int       // c++ guarantees 16-bit at least, but often 32-bit in practice (ILP32, LLP64, LP64)
#define zseb_64_t unsigned long long // c++ guarantees 64-bit at least

#define ZSEB_HIST_SIZE    32768U     // 2^15
#define ZSEB_HIST_BIT     15
#define ZSEB_CHARBIT      8
#define ZSEB_LITLEN       ( 1U << ZSEB_CHARBIT )
#define ZSEB_LENGTH_SHIFT 3          // length = ZSEB_LENGTH_SHIFT + len_shift
#define ZSEB_LENGTH_MAX   258U
#define ZSEB_DIST_SHIFT   1          // distance = ZSEB_DIST_SHIFT + dist_shift

#define ZSEB_256_16T      ( ( zseb_16_t )( 256U ) )

#define ZSEB_MASK_08T     0xffU
#define ZSEB_MASK_16T     0xffffU
#define ZSEB_MASK_32T     0xffffffffUL

#endif

