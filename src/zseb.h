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

#ifndef __ZSEB__
#define __ZSEB__

#include <iostream>
#include <fstream>
#include <string>

#define ZSEB_BUFFER       131072U   // 2^ZSEB_BUFFER_BIT
#define ZSEB_BUFFER_BIT   17
#define ZSEB_SHIFT        65536U    // 2^ZSEB_SHIFT_BIT
#define ZSEB_SHIFT_BIT    16        // ZSEB_SHIFT_BIT == ZSEB_BUFFER_BIT - 1
#define ZSEB_HISTORY      32768U    // 2^ZSEB_HISTORY_BIT
#define ZSEB_HISTORY_BIT  15        // ZSEB_HISTORY_BIT < ZSEB_SHIFT_BIT
#define ZSEB_NUM_CHAR     256U      // 2^ZSEB_NUM_CHAR_BIT; there is an alphabet of 256 characters
#define ZSEB_NUM_CHAR_BIT 8

#define ZSEB_READ_TRIGGER 130048U   // 2^ZSEB_BUFFER_BIT - 1024

#define ZSEB_HASH_SIZE    16777216U // ZSEB_NUM_CHAR ** 3
#define ZSEB_HASH_MASK    16777215U // ZSEB_NUM_CHAR ** 3 - 1
#define ZSEB_HASH_STOP    ( ~( ( unsigned int )( 0 ) ) )

#define ZSEB_STR_LEN      258U      // [ 0 : 2 ** ZSEB_STR_LEN_BIT ] + ZSEB_STR_LEN_SHFT
#define ZSEB_STR_LEN_BIT  8
#define ZSEB_STR_LEN_SHFT 3         // LZSS: Only if length >= 3 string is replaced

#define ZSEB_SANITY_CHECK ( 1UL << 24 )

namespace zseb{

   class zseb{

      public:

         zseb( std::string toread, std::string towrite, const char unzip );

         virtual ~zseb();

      private:

         char modus;

         void __zip__();

         /***  File to compress  ***/

         std::ifstream infile;

         unsigned long size;       // Number of bytes in infile

         char * readframe;         // Length ZSEB_BUFFER; snippet from infile --> [ rd_shift : rd_shift + rd_end ]

         unsigned long rd_shift;   // Current starting point of readframe

         unsigned int  rd_end;     // Current validly filled length of readframe

         unsigned int  rd_current; // Current position within readframe

         /***  Hash table  ***/

         unsigned int * hash_last; // hash_last['abc'] = hash_last[ c + 256 * ( b + 256 * a ) ] = index latest encounter (w.r.t. readframe); STOP = ZSEB_HASH_STOP

         unsigned int * hash_ptrs; // hash_ptrs[ index ] = index' = index of encounter before index ( length ZSEB_BUFFER )

         /***  Functions  ***/

         void __readin__();

         void __shift_left__();    // Shift readframe, hash_last and hash_ptrs

         void __move_up__( unsigned int hash_entry ); // Add hash_entry & rd_current to hash; increment rd_current

         void __longest_match__( unsigned int * l_ptr, unsigned int * l_len, const unsigned int hash_entry, const unsigned int position ) const;

         void __write_infile_test__();

   };

}

#endif
