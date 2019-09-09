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

#define ZSEB_HUF_LLEN       286      // 0-255 lit; 256 stop; 257-285 len; 286 and 287 unused
#define ZSEB_HUF_DIST       30       // 0-29 dist
#define ZSEB_HUF_COMBI      316      // ZSEB_HUF_LLEN + ZSEB_HUF_DIST
#define ZSEB_HUF_SSQ        19       // 0-15 prefix length; 16 prev rep; 17 small zero seq; 18 large zero seq
#define ZSEB_HUF_TREE_LLEN  575      // 2 * ZSEB_HUF_LLEN - 1: WCS each llen_code encountered at least once
#define ZSEB_HUF_TREE_DIST  59       // 2 * ZSEB_HUF_DIST - 1: WCS each dist_code encountered at least once
#define ZSEB_HUF_TREE_SSQ   37       // 2 * ZSEB_HUF_SSQ  - 1: WCS each  ssq_code encountered at least once

#define ZSEB_MAX_BITS_LLD   15
#define ZSEB_MAX_BITS_SSQ   7

namespace zseb{

   typedef struct zseb_node {

      zseb_16_t child[ 2 ];
      zseb_16_t data;       // freq   OR bit sequence
      zseb_16_t info;       // parent OR bit length

   } zseb_node;

   class huffman{

      public:

         static void  pack( zseb_stream &zipfile, zseb_08_t * llen_pack, zseb_16_t * dist_pack, const zseb_32_t size, const bool last_blk );

         static int unpack( zseb_stream &zipfile, zseb_08_t * llen_pack, zseb_16_t * dist_pack, zseb_32_t &wr_current, const zseb_32_t maxsize_pack );

      private:

         /***  HELPER FUNCTIONS  ***/

         static inline void __write__( zseb_stream &zipfile, const zseb_16_t data, const zseb_16_t nbits );

         static inline zseb_16_t __read__( zseb_stream &zipfile, const zseb_16_t nbits );

         static void __prefix_lengths__( zseb_16_t * stat, const zseb_16_t size, zseb_node * tree, const zseb_16_t ZSEB_MAX_BITS );

         static void __build_tree__( zseb_16_t * stat, const zseb_16_t size, zseb_node * tree, bool * work, const char option, const zseb_16_t ZSEB_MAX_BITS );

         static zseb_16_t __ssq_creation__( zseb_16_t * stat, const zseb_16_t size );

         static void __CL_unpack__( zseb_stream &zipfile, zseb_node * tree_ssq, const zseb_16_t size, zseb_16_t * stat );

         static zseb_16_t __get_sym__( zseb_stream &zipfile, zseb_node * tree );

         /***  HUFFMAN TREE STATIC CONSTANTS  ***/

         static const zseb_08_t map_len[ 256 ];  // code_length = 257 + map_len[ len_shift ]; ( len_shift = length - 3 )

         static const zseb_08_t bit_len[ 29 ];   // bits_length =       bit_len[ code_length - 257 ];

         static const zseb_08_t add_len[ 29 ];   // base_length =   3 + add_len[ code_length - 257 ];

         static const zseb_08_t map_dist[ 512 ]; // code_distance = ( shift < 256 ) ? map_dist[ shift ] : map_dist[ 256 ^ ( shift >> 7 ) ]; ( shift = dist - 1 )

         static const zseb_08_t bit_dist[ 30 ];  // bits_distance =     bit_dist[ code_distance ];

         static const zseb_16_t add_dist[ 30 ];  // base_distance = 1 + add_dist[ code_distance ];

         static const zseb_08_t bit_ssq[ 19 ];

         static const zseb_08_t ssq_sym2pos[ 19 ];

         static const zseb_08_t ssq_pos2sym[ 19 ];

         /***  CONVERSIONS  ***/

         static inline zseb_16_t  __len_code__( const zseb_08_t  len_shft ); // [ 257 : 285 ]

         static inline zseb_08_t  __len_bits__( const zseb_16_t  len_code ); // [ 0 : 5 ]

         static inline zseb_08_t  __len_base__( const zseb_16_t  len_code ); // [ 0 : 255 ]

         static inline zseb_08_t __dist_code__( const zseb_16_t dist_shft ); // [ 0 :  29 ]

   };

}

#endif

