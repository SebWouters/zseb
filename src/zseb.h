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

#define zseb_08_t unsigned char      // c++ guarantees  8-bit
#define zseb_16_t unsigned short     // c++ guarantees 16-bit
#define zseb_32_t unsigned int       // c++ guarantees 16-bit, although 32_bit in practice
#define zseb_64_t unsigned long long // c++ guarantees 64-bit

#define ZSEB_READFRAME    131072U    // 2^17
#define ZSEB_SHIFT        65536U     // 2^16
#define ZSEB_HISTORY      32768U     // 2^15
#define ZSEB_HISTORY_BIT  15
#define ZSEB_LITLEN       256U       // 2^8; there is an alphabet of 256 literals / len_shifts
#define ZSEB_LITLEN_BIT   8
#define ZSEB_LENGTH_SHIFT 3          // length = ZSEB_LENGTH_SHIFT + len_shift
#define ZSEB_LENGTH_MAX   258U

#define ZSEB_READ_TRIGGER 130048U    // 2^17 - 2^10: important that 2^10 > max( length ) = 258 !!!

#define ZSEB_HASH_SIZE    16777216U  // ZSEB_LITLEN^3 = 2^24
#define ZSEB_HASH_MASK    16777215U  // ZSEB_LITLEN^3 - 1 = 2^24 - 1
#define ZSEB_HASH_STOP    ( ~( ( zseb_32_t )( 0 ) ) ) // pointers can be larger than 2^16
#define ZSEB_LTRL_READ    ( ~( ( zseb_16_t )( 0 ) ) ) // if ( dist_pack[ x ] == ZSEB_LTRL_READ ) then llen_pack[ x ] is a lit_code, else llen_pack[ x ] is a len_shift

#define ZSEB_HUF1         286U      // 0-255 lit; 256 stop; 257-285 len; 286 and 287 unused
#define ZSEB_HUF2         30U       // 0-29 dist

namespace zseb{

   class zseb{

      public:

         zseb( std::string toread, std::string towrite, const char unzip );

         virtual ~zseb();

      private:

         /***  HUFFMAN TREE STATIC CONSTANTS  ***/

         static const zseb_08_t map_len[ 256 ];  // code_length = 257 + map_len[ len_shift ]; ( len_shift = length - 3 )

         static const zseb_08_t bit_len[ 29 ];   // bits_length =       bit_len[ code_length - 257 ];

         static const zseb_08_t add_len[ 29 ];   // base_length =   3 + add_len[ code_length - 257 ];

         static const zseb_08_t map_dist[ 512 ]; // code_distance = ( shift < 256 ) ? map_dist[ shift ] : map_dist[ 256 ^ ( shift >> 7 ) ]; ( shift = dist - 1 )

         static const zseb_08_t bit_dist[ 30 ];  // bits_distance =     bit_dist[ code_distance ];

         static const zseb_16_t add_dist[ 30 ];  // base_distance = 1 + add_dist[ code_distance ];

         /***  INPUT FILE  ***/

         std::ifstream infile;

         zseb_64_t size;   // Number of bytes in infile

         char * readframe; // Length ZSEB_BUFFER; snippet from infile --> [ rd_shift : rd_shift + rd_end ]

         zseb_64_t rd_shift;   // Current starting point of readframe

         zseb_32_t rd_end;     // Current validly filled length of readframe [ 0 : ZSEB_READFRAME ]

         zseb_32_t rd_current; // Current position within readframe

         /***  OUTPUT FILE  ***/

         std::ofstream outfile;

         zseb_64_t lzss; // Number of bits with pure LZSS, header excluded ( 1-bit diff + 8-bit lit OR 1-bit diff + 8-bit len_shift + 15-bit dist_shift )

         zseb_64_t zlib; // Number of bits with GZIP, headed excluded: ( lzss - zlib ) is extra compression via huffman

         zseb_08_t * llen_pack; // Length ZSEB_SHIFT; contains lit_code OR len_shift ( packing happend later )

         zseb_16_t * dist_pack; // Length ZSEB_SHIFT; contains dist_shift [ 0 : 32767 ] with ZSEB_LTRL_READ means literal

         zseb_32_t wr_current;  // Currently validly filled length of distance & buffer

         zseb_32_t * stat_lit;  // Length ZSEB_HUF1 --> counts lit/len code encounters: TODO: if shift every ZSEB_SHIFT char: counts cannot exceed 2^16

         zseb_32_t * stat_dist; // Length ZSEB_HUF2 --> counts dist code encounters: TODO: if shift every ZSEB_SHIFT char: counts cannot exceed 2^16

         /***  Hash table  ***/

         zseb_32_t * hash_last; // hash_last['abc'] = hash_last[ c + 256 * ( b + 256 * a ) ] = index latest encounter (w.r.t. readframe); STOP = ZSEB_HASH_STOP

         zseb_32_t * hash_ptrs; // hash_ptrs[ index ] = index' = index of encounter before index ( length ZSEB_BUFFER )

         /***  Functions  ***/

         void __zip__();

         void __readin__();

         void __shift_left__();    // Shift readframe, hash_last and hash_ptrs

         inline void __move_up__( const zseb_32_t hash_entry ); // Add hash_entry & rd_current to hash; increment rd_current

         inline void __longest_match__( zseb_32_t &result_ptr, zseb_16_t &result_len, const zseb_32_t hash_entry, const zseb_32_t position ) const;

         inline void __append_lit_encode__();

         inline void __append_len_encode__( const zseb_16_t dist_shift, const zseb_08_t len_shift );

         void __lzss_encode__();

         void __flush_encode__( const bool lastblock );

   };

}

#endif
