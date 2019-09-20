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

#ifndef __ZSEB_STREAM__
#define __ZSEB_STREAM__

#include <fstream>
#include <iostream>
#include <string>

#include "dtypes.h"

namespace zseb{

   class stream{

      public:

         stream( std::string filename, const char modus );

         virtual ~stream();

         /***  WRITE FUNCTIONS  ***/

         void flush();

         void write( const zseb_32_t flush, const zseb_16_t nbits );

         void write( const char * buffer, const zseb_32_t size_out );

         static void int2str( const zseb_32_t value, char * store, const zseb_16_t num );

         /***  READ FUNCTION  ***/

         void nextbyte();

         zseb_32_t read( const zseb_16_t nbits );

         void read( char * buffer, const zseb_32_t size_in );

         static zseb_32_t str2int( const char * store, const zseb_16_t num );

         /***  FILE SIZE INFO  ***/

         zseb_64_t getsize() const; // Only relevant on 'R'

         zseb_64_t getpos();

      private:

         std::fstream file;

         char mode;      // 'R' or 'W'

         zseb_32_t data; // Not yet completed byte

         zseb_16_t ibit; // Number of bits in not yet completed byte

         zseb_64_t size; // File size variable

   };

}

#endif

