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

#ifndef __ZSEB_HUFFMAN__
#define __ZSEB_HUFFMAN__

#include <iostream>
#include <fstream>
#include <string>

#include "dtypes.h"

#define ZSEB_HUF_LLEN       288      // 0-255 lit; 256 stop; 257-285 len; 286 and 287 unused
#define ZSEB_HUF_DIST       30       // 0-29 dist
#define ZSEB_HUF_TREE_LLEN  575      // 2 * ZSEB_HUF_LLEN - 1: WCS each llen_code encountered at least once
#define ZSEB_HUF_TREE_DIST  59       // 2 * ZSEB_HUF_DIST - 1: WCS each dist_code encountered at least once
#define ZSEB_HUF_STOP_CODE  256      // stop code
#define ZSEB_HUF_PREF_MAX   15

namespace zseb{

   typedef struct zseb_node {
      zseb_16_t child[ 2 ];
      zseb_16_t data; // frequency OR bit representation
      zseb_16_t info; // parent pointer OR number of bits in code
   } zseb_node;

   class huffman{

      public:

         static zseb_64_t flush( std::ofstream &outfile, zseb_08_t * llen_pack, zseb_16_t * dist_pack, const zseb_16_t size, const bool last_blk );

      private:

         /***  HELPER FUNCTIONS  ***/

         static zseb_64_t __prefix_lengths__( zseb_16_t * stat, const zseb_16_t size, zseb_node * tree );

         /***  HUFFMAN TREE STATIC CONSTANTS  ***/

         static const zseb_08_t map_len[ 256 ];  // code_length = 257 + map_len[ len_shift ]; ( len_shift = length - 3 )

         static const zseb_08_t bit_len[ 29 ];   // bits_length =       bit_len[ code_length - 257 ];

         static const zseb_08_t add_len[ 29 ];   // base_length =   3 + add_len[ code_length - 257 ];

         static const zseb_08_t map_dist[ 512 ]; // code_distance = ( shift < 256 ) ? map_dist[ shift ] : map_dist[ 256 ^ ( shift >> 7 ) ]; ( shift = dist - 1 )

         static const zseb_08_t bit_dist[ 30 ];  // bits_distance =     bit_dist[ code_distance ];

         static const zseb_16_t add_dist[ 30 ];  // base_distance = 1 + add_dist[ code_distance ];

         static const zseb_08_t rle_order[ 19 ];

         /***  CONVERSIONS  ***/
         /*
                 -->  len_shft =  length   - 3;                 [ 0 : 255 ]
                 --> dist_shft =  distance - 1;                 [ 0 : 32767 ]
                 -->  len_shft =  len_base + readin(  len_bits )
                 --> dist_shft = dist_base + readin( dist_bits )             
         */

         static inline zseb_16_t  __len_code__( const zseb_08_t  len_shft ); // [ 257 : 285 ]

         static inline zseb_08_t  __len_bits__( const zseb_16_t  len_code ); // [ 0 : 5 ]

         static inline zseb_08_t  __len_base__( const zseb_16_t  len_code ); // [ 0 : 255 ]

         static inline zseb_08_t __dist_code__( const zseb_16_t dist_shft ); // [ 0 :  29 ]

         static inline zseb_08_t __dist_bits__( const zseb_08_t dist_code ); // [ 0 : 13 ]

         static inline zseb_16_t __dist_base__( const zseb_08_t dist_code ); // [ 0 : 24576 ]

   };

}

#endif

