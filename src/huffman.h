/*
   zseb: Zipping Sequences of Encountered Bytes
   Copyright (C) 2019, 2020 Sebastian Wouters

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
#include "stream.h"

#define ZSEB_HUF_LLEN       288      // 0-255 lit; 256 stop; 257-285 len; 286 and 287 unused
#define ZSEB_HUF_DIST       32       // 0-29 dist; 30 and 31 unused
#define ZSEB_HUF_SSQ        19       // 0-15 prefix length; 16 prev rep; 17 small zero seq; 18 large zero seq
#define ZSEB_HUF_COMBI      ( ZSEB_HUF_LLEN + ZSEB_HUF_DIST )
#define ZSEB_HUF_TREE_LLEN  ( 2 * ZSEB_HUF_LLEN - 1 )
#define ZSEB_HUF_TREE_DIST  ( 2 * ZSEB_HUF_DIST - 1 )
#define ZSEB_HUF_TREE_SSQ   ( 2 * ZSEB_HUF_SSQ  - 1 )

#define ZSEB_MAX_BITS_LLD   15
#define ZSEB_MAX_BITS_SSQ   7

namespace zseb{

   typedef struct zseb_node {

      uint16_t child[ 2 ];
      uint16_t data;       // freq   OR bit sequence
      uint16_t info;       // parent OR bit length

   } zseb_node;

   class huffman{

      public:

         huffman();

         virtual ~huffman();

         /***  TREES  ***/

         void load_tree( stream * zipfile );

         void calc_tree( uint8_t * llen_pack, uint16_t * dist_pack, const uint32_t size );

         void write_tree( stream * zipfile ) const;

         void fixed_tree( const char modus );

         /***  (UN)PACK  ***/

         void pack( stream * zipfile, uint8_t * llen_pack, uint16_t * dist_pack, const uint32_t size );

         bool unpack( stream * zipfile, uint8_t * llen_pack, uint16_t * dist_pack, uint32_t &wr_current, const uint32_t maxsize_pack );

         /***  Get sizes of fixed / dynamic trees  ***/

         uint32_t get_size_X1() const{ return size_X1; }

         uint32_t get_size_X2() const{ return size_X2; }

      private:

         /***  DATA: advantage of switching to data class, is that when buffers are too short, the trees are still in memory :-)  ***/

         uint16_t * stat_comb; // Length ZSEB_HUF_COMBI: for stat_llen = stat_comb + 0 & stat_dist = stat_comb + HLIT

         uint16_t * stat_ssq;  // Length ZSEB_HUF_SSQ

         zseb_node * tree_llen; // Length ZSEB_HUF_TREE_LLEN

         zseb_node * tree_dist; // Length ZSEB_HUF_TREE_DIST

         zseb_node * tree_ssq;  // Length ZSEB_HUF_TREE_SSQ

         uint16_t HLIT;

         uint16_t HDIST;

         uint16_t HCLEN;

         uint16_t size_ssq;

         bool * work; // Length ZSEB_HUF_TREE_LLEN; purely for combinations of zseb_node's in __build_tree__ modus 'I'

         uint32_t size_X1;

         uint32_t size_X2;

         /***  HELPER FUNCTIONS  ***/

         static inline uint16_t __bit_reverse__( uint16_t code, const uint16_t nbits );

         static uint16_t __prefix_lengths__( uint16_t * stat, const uint16_t size, zseb_node * tree, bool * temp, const uint16_t ZSEB_MAX_BITS );

         static void __build_tree__( uint16_t * stat, const uint16_t size, zseb_node * tree, bool * temp, const char option, const uint16_t ZSEB_MAX_BITS );

         static uint16_t __ssq_creation__( uint16_t * stat, const uint16_t size );

         static void __CL_unpack__( stream * zipfile, zseb_node * tree, const uint16_t size, uint16_t * stat );

         static uint16_t __get_sym__( stream * zipfile, zseb_node * tree );

         /***  HUFFMAN TREE STATIC CONSTANTS  ***/

         static const uint8_t map_len[ 256 ];  // code_length = 257 + map_len[ len_shift ]; ( len_shift = length - 3 )

         static const uint8_t bit_len[ 29 ];   // bits_length =       bit_len[ code_length - 257 ];

         static const uint8_t add_len[ 29 ];   // base_length =   3 + add_len[ code_length - 257 ];

         static const uint8_t map_dist[ 512 ]; // code_distance = ( shift < 256 ) ? map_dist[ shift ] : map_dist[ 256 ^ ( shift >> 7 ) ]; ( shift = dist - 1 )

         static const uint8_t bit_dist[ 30 ];  // bits_distance =     bit_dist[ code_distance ];

         static const uint16_t add_dist[ 30 ];  // base_distance = 1 + add_dist[ code_distance ];

         static const uint8_t map_ssq[ 19 ];

         /***  CONVERSIONS  ***/

         static inline uint16_t  __len_code__( const uint8_t  len_shft ); // [ 257 : 285 ]

         static inline uint8_t  __len_bits__( const uint16_t  len_code ); // [ 0 : 5 ]

         static inline uint8_t  __len_base__( const uint16_t  len_code ); // [ 0 : 255 ]

         static inline uint8_t __dist_code__( const uint16_t dist_shft ); // [ 0 :  29 ]

   };

}

#endif

