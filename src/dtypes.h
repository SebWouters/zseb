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

#include <fstream>

#define zseb_08_t unsigned char      // c++ guarantees  8-bit at least
#define zseb_16_t unsigned short     // c++ guarantees 16-bit at least
#define zseb_32_t unsigned int       // c++ guarantees 16-bit at least, but often 32-bit in practice (ILP32, LLP64, LP64)
#define zseb_64_t unsigned long long // c++ guarantees 64-bit at least

#define ZSEB_256_16T    ( ( zseb_16_t )( 256 ) )
#define ZSEB_MAX_16T    65535

namespace zseb{

   typedef struct zseb_stream{

      std::fstream file;
      zseb_16_t    ibit;
      zseb_32_t    data;

   } zseb_stream;

}

#endif

