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

#define ZSEB_HASH_SIZE    16777216U // ZSEB_NUM_CHAR ^ 3
#define ZSEB_HASH_MASK    16777215U // ZSEB_NUM_CHAR ^ 3 - 1
#define ZSEB_HASH_STOP    ( ~( ( unsigned int )( 0 ) ) )

#define ZSEB_STR_LEN      258U      // [ 0 : 2 ^ ZSEB_STR_LEN_BIT ] + ZSEB_STR_LEN_SHFT
#define ZSEB_STR_LEN_BIT  8
#define ZSEB_STR_LEN_SHFT 3         // LZSS: Only if length >= 3 string is replaced

#define ZSEB_HUF1         286U      // 0-255 lit; 256 stop; 257-285 len; 286 and 287 unused
#define ZSEB_HUF2         30U       // 0-29 dist

#define ZSEB_SANITY_CHECK ( 1UL << 24 )

namespace zseb{

   class zseb{

      public:

         zseb( std::string toread, std::string towrite, const char unzip );

         virtual ~zseb();

      private:

         char modus;

         void __zip__();

         /***  HUFFMAN TREE STATICS  ***/

         unsigned char map_len[ 256 ]; // For zipping: code = 257 + map_len[ length - 3 ];

         void __fill_map_len__();

         static const unsigned char bit_len[ 29 ]; // For unzipping

         static const unsigned int  add_len[ 29 ]; // For unzipping

         unsigned char map_dist[ 512 ]; // For zipping: shift = distance - 1; code = ( shift < 256 ) ? map_dist[ shift ] : map_dist[ 256 ^ ( shift >> 7 ) ];

         void __fill_map_dist__();

         static const unsigned char bit_dist[ 30 ]; // For unzipping

         static const unsigned int  add_dist[ 30 ]; // For unzipping

         /***  INPUT FILE  ***/

         std::ifstream infile;

         unsigned long size;       // Number of bytes in infile

         unsigned long long lzss;  // Number of bits with pure LZSS, header excluded ( 1-bit diff + 8-bit lit OR 1-bit diff + 8-bit len + 15-bit dist )

         char * readframe;         // Length ZSEB_BUFFER; snippet from infile --> [ rd_shift : rd_shift + rd_end ]

         unsigned long rd_shift;   // Current starting point of readframe

         unsigned int  rd_end;     // Current validly filled length of readframe

         unsigned int  rd_current; // Current position within readframe

         /***  OUTPUT FILE  ***/

         std::ofstream outfile;

         unsigned long long zlib;  // Number of bits with GZIP encoding, headed excluded

         unsigned char * buffer;   // Length ZSEB_SHIFT; contains lit OR len_shift = length - 3

         unsigned int * distance;  // Length ZSEB_SHIFT; contains dist_shift, where ZSEB_HASH_STOP means lit

         unsigned int wr_current;  // Currently validly filled length of distance & buffer

         unsigned int * stat_lit;  // Length ZSEB_HUF1 --> counts lit/len code encounters

         unsigned int * stat_dist; // Length ZSEB_HUF2 --> counts dist code encounters

         /***  Hash table  ***/

         unsigned int * hash_last; // hash_last['abc'] = hash_last[ c + 256 * ( b + 256 * a ) ] = index latest encounter (w.r.t. readframe); STOP = ZSEB_HASH_STOP

         unsigned int * hash_ptrs; // hash_ptrs[ index ] = index' = index of encounter before index ( length ZSEB_BUFFER )

         /***  Functions  ***/

         void __readin__();

         void __shift_left__();    // Shift readframe, hash_last and hash_ptrs

         inline void __move_up__( unsigned int hash_entry ); // Add hash_entry & rd_current to hash; increment rd_current

         inline void __longest_match__( unsigned int * l_ptr, unsigned int * l_len, const unsigned int hash_entry, const unsigned int position ) const;

         inline void __append_lit_encode__();

         inline void __append_len_encode__( const unsigned int dist_shift, const unsigned int len_shift );

         void __lzss_encode__();

   };

}

#endif
