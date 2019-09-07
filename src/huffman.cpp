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

#include <assert.h>
#include "huffman.h"

const zseb_08_t zseb::huffman::bit_len[ 29 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 1,  1,  1,  1,  2,  2,  2,  2,  3,  3,  3,  3,  4,  4,  4,   4,   5,   5,   5,   5,   0 };

const zseb_08_t zseb::huffman::add_len[ 29 ] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 255 };

const zseb_08_t zseb::huffman::map_len[ 256 ] = {  0,  1,  2,  3,  4,  5,  6,  7,  8,  8,  9,  9, 10, 10, 11, 11,
                                                  12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15,
                                                  16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17,
                                                  18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19,
                                                  20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                                                  21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
                                                  22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
                                                  23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
                                                  24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
                                                  24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
                                                  25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
                                                  25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
                                                  26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
                                                  26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
                                                  27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
                                                  27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28 };

const zseb_08_t zseb::huffman::bit_dist[ 30 ] = { 0, 0, 0, 0, 1, 1, 2,  2,  3,  3,  4,  4,  5,  5,   6,   6,   7,   7,   8,   8,    9,   9,    10,   10,   11,   11,   12,    12,    13,    13 };

const zseb_16_t zseb::huffman::add_dist[ 30 ] = { 0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576 };

const zseb_08_t zseb::huffman::map_dist[ 512 ] = {  0,   1,   2,   3,   4,   4,   5,   5,   6,   6,   6,   6,   7,   7,   7,   7,
                                                    8,   8,   8,   8,   8,   8,   8,   8,   9,   9,   9,   9,   9,   9,   9,   9,
                                                   10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
                                                   11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,
                                                   12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,
                                                   12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,
                                                   13,  13,  13,  13,  13,  13,  13,  13,  13,  13,  13,  13,  13,  13,  13,  13,
                                                   13,  13,  13,  13,  13,  13,  13,  13,  13,  13,  13,  13,  13,  13,  13,  13,
                                                   14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,
                                                   14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,
                                                   14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,
                                                   14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,
                                                   15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,
                                                   15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,
                                                   15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,
                                                   15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,
                                                  255, 255,  16,  17,  18,  18,  19,  19,  20,  20,  20,  20,  21,  21,  21,  21,
                                                   22,  22,  22,  22,  22,  22,  22,  22,  23,  23,  23,  23,  23,  23,  23,  23,
                                                   24,  24,  24,  24,  24,  24,  24,  24,  24,  24,  24,  24,  24,  24,  24,  24,
                                                   25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,
                                                   26,  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,
                                                   26,  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,
                                                   27,  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,
                                                   27,  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,
                                                   28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,
                                                   28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,
                                                   28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,
                                                   28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,
                                                   29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,
                                                   29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,
                                                   29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,
                                                   29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29 };

const zseb_08_t zseb::huffman::rle_order[ 19 ] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

zseb_16_t zseb::huffman::__len_code__( const zseb_08_t len_shft ){

   return ( ZSEB_256_16T ^ ( 1 + map_len[ len_shft ] ) );

}

zseb_08_t zseb::huffman::__len_bits__( const zseb_16_t len_code ){

   return bit_len[ ZSEB_256_16T ^ ( len_code - 1 ) ];

}

zseb_08_t zseb::huffman::__len_base__( const zseb_16_t len_code ){

   return add_len[ ZSEB_256_16T ^ ( len_code - 1 ) ];

}

zseb_08_t zseb::huffman::__dist_code__( const zseb_16_t dist_shft ){

   return ( ( dist_shft < ZSEB_256_16T ) ? map_dist[ dist_shft ] : map_dist[ ZSEB_256_16T ^ ( dist_shft >> 7 ) ] );

}

zseb_08_t zseb::huffman::__dist_bits__( const zseb_08_t dist_code ){

   return bit_dist[ dist_code ];

}

zseb_16_t zseb::huffman::__dist_base__( const zseb_08_t dist_code ){

   return add_dist[ dist_code ];

}

zseb_64_t zseb::huffman::flush( std::ofstream &outfile, zseb_08_t * llen_pack, zseb_16_t * dist_pack, const zseb_16_t size, const bool last_blk ){

   zseb_16_t * stat_llen = new zseb_16_t[ ZSEB_HUF_LLEN ];
   zseb_16_t * stat_dist = new zseb_16_t[ ZSEB_HUF_DIST ];
   zseb_node * tree_llen = new zseb_node[ ZSEB_HUF_TREE_LLEN ];
   zseb_node * tree_dist = new zseb_node[ ZSEB_HUF_TREE_DIST ];

   for ( zseb_16_t cnt = 0; cnt < ZSEB_HUF_LLEN; cnt++ ){ stat_llen[ cnt ] = 0; }
   for ( zseb_16_t cnt = 0; cnt < ZSEB_HUF_DIST; cnt++ ){ stat_dist[ cnt ] = 0; }

   // Account for stop codon occurring once (also needs a huffman code)
   stat_llen[ ZSEB_HUF_STOP_CODE ] = 1;

   // Gather statistics
   for ( zseb_16_t count = 0; count < size; count++ ){
      if ( dist_pack[ count ] == ZSEB_MAX_16T ){
         stat_llen[ llen_pack[ count ] ] += 1; // lit_code == lit
      } else {
         stat_llen[  __len_code__( llen_pack[ count ] ) ] += 1;
         stat_dist[ __dist_code__( dist_pack[ count ] ) ] += 1;
      }
   }

   // Huffman prefix lengths: stat input = freq; stat output = HCL
   zseb_64_t zlib_part = 0;
   zlib_part += __prefix_lengths__( stat_dist, ZSEB_HUF_DIST, tree_dist );
   zlib_part += __prefix_lengths__( stat_llen, ZSEB_HUF_LLEN, tree_llen );

   // Create headers

   //std::cout << "zseb: Flush blk zlib_part = " << zlib_part << std::endl;

   delete [] stat_llen;
   delete [] stat_dist;
   delete [] tree_llen;
   delete [] tree_dist;

   return zlib_part;

}

zseb_64_t zseb::huffman::__prefix_lengths__( zseb_16_t * stat, const zseb_16_t size, zseb_node * tree ){

   // Find codes with non-zero frequencies
   zseb_16_t num = 0;
   for ( zseb_16_t idx = 0; idx < size; idx++ ){
      if ( stat[ idx ] != 0 ){
         tree[ num ].child[ 0 ] = idx;
         tree[ num ].child[ 1 ] = idx;
         tree[ num ].info = ZSEB_MAX_16T; // parent not yet set
         tree[ num ].data = stat[ idx ];  // frequency
         num += 1;
      }
   }
   assert( num != 0 );

   // Construct Huffman tree: after ( num - 1 ) steps of removing 2 nodes and adding 1 parent node, only the root remains
   for ( zseb_16_t extra = 0; extra < ( num - 1 ); extra++ ){

      const zseb_16_t next = num + extra;
      tree[ next ].info = ZSEB_MAX_16T; // parent not yet set
      tree[ next ].data = 0;            // frequency

      for ( zseb_16_t chld = 0; chld < 2; chld++ ){ // Find two children for next
         zseb_16_t rare = ZSEB_MAX_16T;
         for ( zseb_16_t sch = 0; sch < next; sch++ ){
            if ( tree[ sch ].info == ZSEB_MAX_16T ){ // parent not yet set
               if ( rare == ZSEB_MAX_16T ){
                  rare = sch; // Assign first relevant encounter
               } else {
                  if ( tree[ sch ].data < tree[ rare ].data ){ // smaller frequency
                     rare = sch; // Assign even rarer encounter
                  }
               }
            }
         }
         tree[ rare ].info          = next; // assign parent to child
         tree[ next ].child[ chld ] = rare; // assign child to parent
         tree[ next ].data         += tree[ rare ].data; // child frequency contributes to parent frequency
         assert( rare != ZSEB_MAX_16T );
      }
   }

   // Determine bit lengths --> root has none
   const zseb_16_t root = 2 * num - 2; // Tree contains 2 * num - 1 nodes
   tree[ root ].info = 0; // Bit length
 //tree[ root ].data = 0; // Bit code
   for ( zseb_16_t extra = root; extra >= num; extra-- ){ // Only non-leafs
      for ( zseb_16_t chld = 0; chld < 2; chld++ ){
         tree[ tree[ extra ].child[ chld ] ].info = tree[ extra ].info + 1; // Bit length
       //tree[ tree[ extra ].child[ chld ] ].data = tree[ extra ].data | ( chld << tree[ extra ].info ); // Bit code
      }
   }

   // Decrement lengths > ZSEB_HUF_PREF_MAX
   zseb_16_t tomove = 0;
   for ( zseb_16_t pack = 0; pack < num; pack++ ){
      if ( tree[ pack ].info > ZSEB_HUF_PREF_MAX ){ // Bit length
         tomove += ( tree[ pack ].info - ZSEB_HUF_PREF_MAX ); // Excess bit length
         tree[ pack ].info = ZSEB_HUF_PREF_MAX; // Bit length
      }
   }

   // Move decrements: Least harm? Lengths < PREFIX_MAX with smallest associated frequency
   while ( tomove > 0 ){
      zseb_16_t rare = ZSEB_MAX_16T;
      for ( zseb_16_t pack = 0; pack < num; pack++ ){
         if ( tree[ pack ].info < ZSEB_HUF_PREF_MAX ){ // Bit length allows for increment
            if ( rare == ZSEB_MAX_16T ){
               rare = pack; // Assign first relevant encounter
            } else {
               if ( tree[ pack ].data < tree[ rare ].data ){ // smaller frequency
             //if ( stat[ tree[ pack ].child[ 0 ] ] < stat[ tree[ rare ].child[ 0 ] ] ){ // In case Bit code would have been overwritten above!
                  rare = pack; // Assign even rarer encounter
               }
            }
         }
      }
      tree[ rare ].info += 1; // Bit length increment
      tomove -= 1;
   }

   // Repack to stat
   zseb_64_t zlib_part = 0; // Compute partial zlib size
   for ( zseb_16_t pack = 0; pack < num; pack++ ){
      const zseb_16_t code = tree[ pack ].child[ 0 ];
      const zseb_08_t nbit = ( ( size == ZSEB_HUF_DIST ) ? __dist_bits__( code ) : ( ( code > 256 ) ? __len_bits__( code ) : 0 ) );
      zlib_part += ( tree[ pack ].info + nbit ) * stat[ code ];
      stat[ code ] = tree[ pack ].info; // Bit length
   }

   return zlib_part;

}



