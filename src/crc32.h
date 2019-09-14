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

#ifndef __ZSEB_CRC32__
#define __ZSEB_CRC32__

#include "dtypes.h"

#define ZSEB_CRC32_REVERSE  0xEDB88320L
#define ZSEB_CRC32_BITFLIP  0xFFFFFFFFL
#define ZSEB_CRC32_TABLE    256
#define ZSEB_CRC32_MASK     0xFF
#define ZSEB_CRC32_CHARBIT  8

namespace zseb{

   class crc32{

      public:

         static zseb_32_t update( const zseb_32_t crc, char * data, const zseb_32_t length );

         static zseb_32_t entry( const zseb_16_t idx );

      private:

         static const zseb_32_t table[ ZSEB_CRC32_TABLE ];

   };

}

#endif

