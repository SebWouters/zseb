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
#include <stdlib.h>
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

const zseb_08_t zseb::huffman::bit_ssq[ 19 ]     = {  0,  0,  0,  0,  0, 0, 0, 0,  0, 0,  0,  0,  0,  0,  0,  0,  2, 3,  7 };

const zseb_08_t zseb::huffman::ssq_sym2pos[ 19 ] = {  3, 17, 15, 13, 11, 9, 7, 5,  4, 6,  8, 10, 12, 14, 16, 18,  0, 1,  2 }; // sym = '0' at pos = 3

const zseb_08_t zseb::huffman::ssq_pos2sym[ 19 ] = { 16, 17, 18,  0,  8, 7, 9, 6, 10, 5, 11,  4, 12,  3, 13,  2, 14, 1, 15 }; // at pos = 0, sym = '16' (rep previous)

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

void zseb::huffman::__write__( zseb_stream &zipfile, const zseb_16_t flush, const zseb_16_t nbits ){

   /* Data ini : [ __ __ __ P5 P4 P3 P2 P1 ], ibit = 5
      Flush    = [ __ __ N1 N2 N3 N4 N5 N6 ], nbits = 6
      Data mid : [ __ __ __ __ __ N6 N5 N4 ][ N3 N2 N1 P5 P4 P3 P3 P1 ], ibit = 11
      Stream <<  [ N3 N2 N1 P5 P4 P3 P2 P1 ]
      Data end : [ __ __ __ __ __ N6 N5 N4 ], ibit = 3
   */

   /* Quote from RFC 1951:

          * Data elements are packed into bytes in order of
            increasing bit number within the byte, i.e., starting
            with the least-significant bit of the byte.
          * Data elements other than Huffman codes are packed
            starting with the least-significant bit of the data
            element.
          * Huffman codes are packed starting with the most-
            significant bit of the code.
   */

   //std::cout << "flush = " << flush << std::endl;
   //std::cout << "nbits = " << nbits << std::endl;

   zseb_16_t ibit = zipfile.ibit;
   zseb_32_t data = zipfile.data;
   //data = data & ( ( 1U << ibit ) - 1 ); // Mask last ibit bits

   //std::cout << "data = " << data << std::endl;
   //std::cout << "ibit = " << ibit << std::endl;

   for ( zseb_16_t bit = 0; bit < nbits; bit++ ){ // Append biggest first
      data |= ( ( ( flush >> ( nbits - 1 - bit ) ) & 1U ) << ( ibit + bit ) );
   }
   ibit = ibit + nbits;

   //std::cout << "data = " << data << std::endl;
   //std::cout << "ibit = " << ibit << std::endl;

   while ( ibit >= 8 ){
      zseb_08_t towrite = ( zseb_08_t )( data & 255U ); // Mask last 8 bits
      zipfile.file << towrite;
      data = ( data >> 8 );
      ibit = ibit - 8;
   }

   //std::cout << "data = " << data << std::endl;
   //std::cout << "ibit = " << ibit << std::endl;

   zipfile.ibit = ( zseb_08_t )( ibit );
   zipfile.data = ( zseb_08_t )( data );

}

zseb_16_t zseb::huffman::__read__( zseb_stream &zipfile, const zseb_16_t nbits ){

   /* Cfr. write example: P has been retrieved and removed
      Data ini : [ __ __ __ __ __ N3 N2 N1 ], ibit = 3
      nbits = 6 > ibit = 3
      Stream >>  [ XX XX XX XX XX N6 N5 N4 ]
      Data mid : [ __ __ __ __ __ XX XX XX ][ XX XX N6 N5 N4 N3 N2 N1 ], ibit = 11
      Fetch    = [ __ __ N1 N2 N3 N4 N5 N6 ], nbits = 6
      Data end : [ __ __ __ XX XX XX XX XX ], ibit = 5
   */

   zseb_16_t ibit = zipfile.ibit;
   zseb_32_t data = zipfile.data; // data ini

   while ( ibit < nbits ){
      zseb_08_t toread;
      zipfile.file >> toread;
      zseb_32_t toshift = toread;
      //std::cout << "huff RD: toread = " << toshift << std::endl;
      data = ( ( data ) | ( toshift << ibit ) ); // data mid
      ibit = ibit + 8;
   }

   zseb_16_t fetch = 0;
   for ( zseb_16_t bit = 0; bit < nbits; bit++ ){
      fetch = ( ( fetch << 1 ) | ( data & 1U ) );
      data  = ( data >> 1 ); // data end
   }
   ibit = ibit - nbits;

   zipfile.ibit = ( zseb_08_t )( ibit );
   zipfile.data = data;

   return fetch;

}








int zseb::huffman::unpack( zseb_stream &zipfile, zseb_08_t * llen_pack, zseb_16_t * dist_pack, zseb_32_t &wr_current, const zseb_32_t maxsize_pack ){

   assert( wr_current == 0 ); // TODO: If not zero: last returned 2, want to continue: switch from purely static class to data --> reuse trees

   zseb_16_t * stat_comb = new zseb_16_t[ ZSEB_HUF_COMBI ];
   zseb_16_t * stat_ssq  = new zseb_16_t[ ZSEB_HUF_SSQ   ];
   zseb_node * tree_llen = new zseb_node[ ZSEB_HUF_TREE_LLEN ];
   zseb_node * tree_dist = new zseb_node[ ZSEB_HUF_TREE_DIST ];
   zseb_node * tree_ssq  = new zseb_node[ ZSEB_HUF_TREE_SSQ  ];
   bool      * work      = new      bool[ ZSEB_HUF_TREE_LLEN ];

   for ( zseb_16_t cnt = 0; cnt < ZSEB_HUF_COMBI; cnt++ ){ stat_comb[ cnt ] = 0; }
   for ( zseb_16_t cnt = 0; cnt < ZSEB_HUF_SSQ;   cnt++ ){ stat_ssq [ cnt ] = 0; }

   //std::cout << "huff unpack: RD BLK" << std::endl;

   const zseb_16_t HEAD  = __read__( zipfile, 3 );
   const zseb_16_t HLIT  = __read__( zipfile, 5 ) + 257;
   const zseb_16_t HDIST = __read__( zipfile, 5 ) + 1;
   const zseb_16_t HCLEN = __read__( zipfile, 4 ) + 4;
//   std::cout << "HEAD  = " << HEAD  << std::endl;
//   std::cout << "HLIT  = " << HLIT  << std::endl;
//   std::cout << "HDIST = " << HDIST << std::endl;
//   std::cout << "HCLEN = " << HCLEN << std::endl;
   assert( ( HEAD == 6 ) || ( HEAD == 2 ) ); // 6 means LAST BLOCK

   std::cout << "CCL of SSQ = [ ";
   for ( zseb_16_t idx = 0; idx < HCLEN; idx++ ){
      stat_ssq[ idx ] = __read__( zipfile, 3 ); // CCL of reshuffled RLE symbols
      std::cout << stat_ssq[ idx ] << " ; "; //std::endl;
   }
   std::cout << "]" << std::endl;

   // Build tree: on output tree[ idx ].( info, data ) = bit ( length, sequence ) of SSQ
   __build_tree__( stat_ssq, HCLEN, tree_ssq, work, 'I', ZSEB_MAX_BITS_SSQ );

   /*zseb_node * tree_test = new zseb_node[ ZSEB_HUF_TREE_SSQ  ];
   __build_tree__( stat_ssq, HCLEN, tree_test, work, 'O', ZSEB_MAX_BITS_SSQ );
   for ( zseb_16_t idx = 0; idx < HCLEN; idx++ ){
      const zseb_32_t codon = tree_test[ idx ].data;
      const zseb_32_t lengt = tree_test[ idx ].info;
      if ( lengt > 0 ){
         std::cout << "( idx, tree_out.info, tree_out.data ) = ( " << idx << " ; " << lengt << " ; " << codon << " )" << std::endl;
         zseb_16_t counter = 0;
         {
            zseb_16_t node = 0; // start at root
            zseb_16_t bit;

            while ( tree_ssq[ node ].child[ 0 ] != tree_ssq[ node ].child[ 1 ] ){ // not a leaf
               std::cout << "node = " << node << " with code = " << tree_ssq[ node ].data << " and bit length = " << tree_ssq[ node ].info << std::endl;
               bit = ( ( codon >> ( lengt - 1 - counter ) ) & 1U );
               node = tree_ssq[ node ].child[ bit ]; // goto child
               counter++;
            }

            node = tree_ssq[ node ].child[ 0 ]; // symbol the leaf represents
            std::cout << "symbol = " << node << std::endl;
            std::cout << "counter = " << counter << std::endl;

         }
      }
   }
   delete [] tree_test;*/

   // Quote from RFC 1951: all CL form a single sequence of HLIT + HDIST + 258 values
   __CL_unpack__( zipfile, tree_ssq, HLIT + HDIST, stat_comb );
   zseb_16_t * stat_dist = stat_comb + HLIT;

   // Build tree
   __build_tree__( stat_comb, HLIT , tree_llen, work, 'I', ZSEB_MAX_BITS_LLD );
   __build_tree__( stat_dist, HDIST, tree_dist, work, 'I', ZSEB_MAX_BITS_LLD );

   // Ready to read-in!
   zseb_16_t llen_code = 0;

   while ( ( llen_code != ZSEB_LITLEN ) && ( wr_current < maxsize_pack ) ){

      llen_code = __get_sym__( zipfile, tree_llen );

      if ( llen_code < ZSEB_LITLEN ){ // write literal

         llen_pack[ wr_current ] = llen_code;
         dist_pack[ wr_current ] = ZSEB_MAX_16T;
         wr_current += 1;

      }

      if ( llen_code > ZSEB_LITLEN ){ // write ( len, dist ) pair

         zseb_16_t len_shft = __len_base__( llen_code );
         zseb_16_t len_nbit = __len_bits__( llen_code );
         if ( len_nbit > 0 ){
            len_shft = len_shft + __read__( zipfile, len_nbit );
         }

         zseb_16_t dist_code = __get_sym__( zipfile, tree_dist );
         zseb_16_t dist_shft = add_dist[ dist_code ];
         zseb_16_t dist_nbit = bit_dist[ dist_code ];
         if ( dist_nbit > 0 ){
            dist_shft = dist_shft + __read__( zipfile, dist_nbit );
         }

         llen_pack[ wr_current ] = len_shft;
         dist_pack[ wr_current ] = dist_shft;
         wr_current += 1;

      }
   }

   if ( llen_code != ZSEB_LITLEN ){
      assert( wr_current == maxsize_pack );
      return 2; // buffers not large enough
   }

   return ( ( HEAD == 6 ) ? 1 : 0 );

   delete [] stat_comb;
   delete [] stat_ssq;
   delete [] tree_llen;
   delete [] tree_dist;
   delete [] tree_ssq;
   delete [] work;

}








void zseb::huffman::pack( zseb_stream &zipfile, zseb_08_t * llen_pack, zseb_16_t * dist_pack, const zseb_32_t size, const bool last_blk ){

   zseb_16_t * stat_llen = new zseb_16_t[ ZSEB_HUF_COMBI ];
   zseb_16_t * stat_dist = stat_llen + ZSEB_HUF_LLEN;
   zseb_16_t * stat_ssq  = new zseb_16_t[ ZSEB_HUF_SSQ   ];
   zseb_node * tree_llen = new zseb_node[ ZSEB_HUF_TREE_LLEN ];
   zseb_node * tree_dist = new zseb_node[ ZSEB_HUF_TREE_DIST ];
   zseb_node * tree_ssq  = new zseb_node[ ZSEB_HUF_TREE_SSQ  ];
   bool      * work      = new      bool[ ZSEB_HUF_TREE_LLEN ];

   for ( zseb_16_t cnt = 0; cnt < ZSEB_HUF_COMBI; cnt++ ){ stat_llen[ cnt ] = 0; }
   for ( zseb_16_t cnt = 0; cnt < ZSEB_HUF_SSQ;   cnt++ ){ stat_ssq [ cnt ] = 0; }

   // Stop codon also needs a huffman code
   stat_llen[ ZSEB_LITLEN ] = 1;

   // Gather statistics
   for ( zseb_32_t count = 0; count < size; count++ ){
      if ( dist_pack[ count ] == ZSEB_MAX_16T ){
         stat_llen[ llen_pack[ count ] ] += 1; // lit_code == lit
      } else {
         stat_llen[  __len_code__( llen_pack[ count ] ) ] += 1;
         stat_dist[ __dist_code__( dist_pack[ count ] ) ] += 1;
      }
   }

   // Huffman CL: input(stat) = freq; output(stat) = CL
   __prefix_lengths__( stat_llen, ZSEB_HUF_LLEN, tree_llen, ZSEB_MAX_BITS_LLD );
   __prefix_lengths__( stat_dist, ZSEB_HUF_DIST, tree_dist, ZSEB_MAX_BITS_LLD );

   // Retrieve the length of the non-zero CL
   zseb_16_t HLIT  = ZSEB_HUF_LLEN; while ( stat_llen[ HLIT  - 1 ] == 0 ){ HLIT  -= 1; } assert( HLIT  >= 257 );
   zseb_16_t HDIST = ZSEB_HUF_DIST; while ( stat_dist[ HDIST - 1 ] == 0 ){ HDIST -= 1; } assert( HDIST >=   1 );

   // Build tree: on output tree[ idx ].( info, data ) = bit ( length, sequence )
   __build_tree__( stat_llen, HLIT , tree_llen, work, 'O', ZSEB_MAX_BITS_LLD );
   __build_tree__( stat_dist, HDIST, tree_dist, work, 'O', ZSEB_MAX_BITS_LLD );

   // Quote from RFC 1951: all code lengths form a single sequence of HLIT + HDIST + 258 values
   for ( zseb_16_t count = 0; count < HDIST; count++ ){
      stat_llen[ HLIT + count ] = stat_llen[ ZSEB_HUF_LLEN + count ];
   }

   std::cout << "CL of pack = [ "; for ( zseb_16_t cnt = 0; cnt < HLIT + HDIST; cnt++ ){ std::cout << stat_llen[ cnt ] << " ; "; } std::cout << "]" << std::endl;

   // Create the RLE for the CL
   const zseb_16_t size_ssq = __ssq_creation__( stat_llen, HLIT + HDIST );

   std::cout << "SSQ of pack = [ "; for ( zseb_16_t cnt = 0; cnt < size_ssq; cnt++ ){ std::cout << stat_llen[ cnt ] << " ; "; } std::cout << "]" << std::endl;

   // Gather statistics on RLE: If sym >= 16, a number to be represented in bit sequence follows
   for ( zseb_16_t count = 0; count < size_ssq; count++ ){ stat_ssq[ ssq_sym2pos[ stat_llen[ count ] ] ] += 1; if ( stat_llen[ count ] >= 16 ){ count++; } }

   // Huffman CCL: input(stat) = freq; output(stat) = CCL
   __prefix_lengths__( stat_ssq, ZSEB_HUF_SSQ, tree_ssq, ZSEB_MAX_BITS_SSQ ); // Header info size depends on ( HLIT, HDIST, HCLEN )

   // Retrieve the length of the non-zero CCL
   zseb_16_t HCLEN = ZSEB_HUF_SSQ; while ( stat_ssq[ HCLEN - 1 ] == 0 ){ HCLEN -= 1; } assert( HCLEN >= 4 );

   // Build tree: on output tree[ idx ].( info, data ) = bit ( length, sequence )
   __build_tree__( stat_ssq, HCLEN, tree_ssq, work, 'O', ZSEB_MAX_BITS_SSQ );

   std::cout << "CCL of SSQ = [ "; for ( zseb_16_t cnt = 0; cnt < HCLEN; cnt++ ){ std::cout << stat_ssq[ cnt ] << " ; "; } std::cout << "]" << std::endl;

   __write__( zipfile, ( ( last_blk ) ? 6 : 2 ), 3 ); // Dynamic header: 110 for last_blk; 010 for non-last_block
   __write__( zipfile, HLIT  - 257, 5 ); // HLIT
   __write__( zipfile, HDIST - 1,   5 ); // HDIST
   __write__( zipfile, HCLEN - 4,   4 ); // HCLEN

   //std::cout << "HEAD  = " << ( ( last_blk ) ? 6 : 2 ) << std::endl; // SAME
   //std::cout << "HLIT  = " << HLIT << std::endl;  // SAME
   //std::cout << "HDIST = " << HDIST << std::endl; // SAME
   //std::cout << "HCLEN = " << HCLEN << std::endl; // SAME

   //exit( 0 ); 

   for ( zseb_16_t idx = 0; idx < HCLEN; idx++ ){
      __write__( zipfile, stat_ssq[ idx ], 3 ); // CCL of reshuffled RLE symbols
   }

   for ( zseb_16_t idx = 0; idx < size_ssq; idx++ ){
      const zseb_16_t idx_sym = stat_llen[ idx ];
      const zseb_16_t idx_pos = ssq_sym2pos[ idx_sym ];
      __write__( zipfile, tree_ssq[ idx_pos ].data, tree_ssq[ idx_pos ].info ); // RLE symbols in CCL codons
      if ( idx_sym >= 16 ){
         __write__( zipfile, stat_llen[ idx + 1 ], bit_ssq[ idx_sym ] ); // Shifts
         idx++;
      }
   }

   for ( zseb_32_t idx = 0; idx < size; idx++ ){
      if ( dist_pack[ idx ] == ZSEB_MAX_16T ){
         const zseb_16_t lit_code = llen_pack[ idx ];
         __write__( zipfile, tree_llen[ lit_code ].data, tree_llen[ lit_code ].info ); // Literal
      } else {
         const zseb_16_t len_code = __len_code__( llen_pack[ idx ] );
         const zseb_16_t len_nbit = __len_bits__( len_code );
         __write__( zipfile, tree_llen[ len_code ].data, tree_llen[ len_code ].info ); // Length codon
         if ( len_nbit > 0 ){
            const zseb_16_t len_plus = llen_pack[ idx ] - __len_base__( len_code );
            __write__( zipfile, len_plus, len_nbit ); // Shifts
         }
         const zseb_16_t dist_code = __dist_code__( dist_pack[ idx ] );
         const zseb_16_t dist_nbit = bit_dist[ dist_code ];
         __write__( zipfile, tree_dist[ dist_code ].data, tree_dist[ dist_code ].info ); // Dist codon
         if ( dist_nbit > 0 ){
            const zseb_16_t dist_plus = dist_pack[ idx ] - add_dist[ dist_code ];
            __write__( zipfile, dist_plus, dist_nbit ); // Shifts
         }
      }
   }
   __write__( zipfile, tree_llen[ ZSEB_LITLEN ].data, tree_llen[ ZSEB_LITLEN ].info ); // Stop codon

   delete [] stat_llen;
   delete [] stat_ssq;
   delete [] tree_llen;
   delete [] tree_dist;
   delete [] tree_ssq;
   delete [] work;

}

zseb_16_t zseb::huffman::__get_sym__( zseb_stream &zipfile, zseb_node * tree ){

   zseb_16_t idx = 0; // start at root
   zseb_16_t bit;

   while ( tree[ idx ].child[ 0 ] != tree[ idx ].child[ 1 ] ){ // not a leaf
      bit = __read__( zipfile, 1 ); // read-in bit
      idx = tree[ idx ].child[ bit ]; // goto child
   }

   idx = tree[ idx ].child[ 0 ]; // symbol the leaf represents
   return idx;

}

void zseb::huffman::__CL_unpack__( zseb_stream &zipfile, zseb_node * tree_ssq, const zseb_16_t size, zseb_16_t * stat ){

   std::cout << "SSQ un-pack = [ ";

   zseb_16_t bit;
   zseb_16_t size_part = 0;
   zseb_16_t idx_pos;
   zseb_16_t idx_sym;

   while ( size_part < size ){
      idx_pos = __get_sym__( zipfile, tree_ssq );
      idx_sym = ssq_pos2sym[ idx_pos ];

      if ( idx_sym < 16 ){
         stat[ size_part ] = idx_sym;
         std::cout << stat[ size_part ] << " ; ";
         size_part += 1;
      }

      if ( idx_sym == 16 ){
         assert( size_part > 0 );
         bit = 3 + __read__( zipfile, 2 ); // num repeats
         std::cout << bit << " ; ";
         while ( bit > 0 ){
            stat[ size_part ] = stat[ size_part - 1 ];
            size_part += 1;
            bit -= 1;
         }
      }

      if ( idx_sym == 17 ){
         bit = 3 + __read__( zipfile, 3 ); // num repeats
         std::cout << bit << " ; ";
         while ( bit > 0 ){
            stat[ size_part ] = 0;
            size_part += 1;
            bit -= 1;
         }
      }

      if ( idx_sym == 18 ){
         bit = 11 + __read__( zipfile, 7 ); // num repeats
         std::cout << bit << " ; ";
         while ( bit > 0 ){
            stat[ size_part ] = 0;
            size_part += 1;
            bit -= 1;
         }
      }

   }
   std::cout << "]" << std::endl;

   std::cout << "CL un-pack = ["; for ( zseb_16_t cnt = 0; cnt < size_part; cnt++ ){ std::cout << stat[ cnt ] << " ; "; } std::cout << " ]" << std::endl;

   assert( size == size_part );

}

zseb_16_t zseb::huffman::__ssq_creation__( zseb_16_t * stat, const zseb_16_t size ){

   zseb_16_t idx_stat  = 0;
   zseb_16_t idx_first = 0;
   zseb_16_t idx_last;
   zseb_16_t number;

   while ( idx_first < size ){

      // Find idx_last for which the bit length (.info) is the same
      idx_last = idx_first;
      bool same = true;
      while ( ( same ) && ( idx_last + 1 < size ) ){
         if ( stat[ idx_first ] == stat[ idx_last + 1 ] ){
            idx_last += 1;
         } else {
            same = false;
         }
      }

      number = idx_last - idx_first + 1;

      while ( number > 0 ){

         if ( stat[ idx_first ] == 0 ){ // HCL == 0

            if ( number <= 2 ){ // idx_stat increases same as idx_first
               while ( number > 0 ){
                  stat[ idx_stat ] = stat[ idx_first ];
                  idx_stat  += 1;
                  idx_first += 1;
                  number    -= 1;
               }
            }

            if ( ( number >= 3 ) && ( number <= 10 ) ){ // idx_stat increases less than idx_first
               stat[ idx_stat     ] = 17;
               stat[ idx_stat + 1 ] = number - 3; // 3 bit encoding [ 3 : 10 ] --> [ 0 : 7 ]
               idx_stat  += 2;
               idx_first += number;
               number     = 0;
            }

            if ( number >= 11 ){ // idx_stat increases less than idx_first
               const zseb_16_t out = ( ( number > 138 ) ? 138 : number );
               stat[ idx_stat     ] = 18;
               stat[ idx_stat + 1 ] = out - 11; // 7 bit encoding [ 11 : 138 ] --> [ 0 : 127 ]
               idx_stat  += 2;
               idx_first += out;
               number    -= out;
            }

         } else { // 1 <= HCL <= 15

            // In any case: write out the bit length --> idx_stat increases same as idx_first
            stat[ idx_stat ] = stat[ idx_first ];
            idx_stat  += 1;
            idx_first += 1;
            number    -= 1;

            while ( number >= 3 ){ // idx_stat increases less than idx_first
               const zseb_16_t out = ( ( number > 6 ) ? 6 : number );
               stat[ idx_stat     ] = 16;
               stat[ idx_stat + 1 ] = out - 3; // 2 bit encoding [ 3 : 6 ] --> [ 0 : 3 ]
               idx_stat  += 2;
               idx_first += out;
               number    -= out;

            }

         }
      }
   }

   return idx_stat;

}

void zseb::huffman::__build_tree__( zseb_16_t * stat, const zseb_16_t size, zseb_node * tree, bool * work, const char option, const zseb_16_t ZSEB_MAX_BITS ){

   // Paragraph 3.2.2 RFC 1951

   zseb_16_t  bl_count[ ZSEB_MAX_BITS + 1 ];
   zseb_16_t next_code[ ZSEB_MAX_BITS + 1 ];

   for ( zseb_16_t nbits = 0; nbits <= ZSEB_MAX_BITS; nbits++ ){ bl_count[ nbits ] = 0; }
   for ( zseb_16_t   idx = 0; idx < size; idx++ ){ bl_count[ stat[ idx ] ] += 1; }

   next_code[ 0 ] = 0;
    bl_count[ 0 ] = 0;
   zseb_16_t code = 0;
   for ( zseb_08_t nbits = 1; nbits <= ZSEB_MAX_BITS; nbits++ ){
      code = ( code + bl_count[ nbits - 1 ] ) << 1;
      next_code[ nbits ] = code;
   }

   assert( ( option == 'I' ) || ( option == 'O' ) ); // In or Out

   if ( option == 'O' ){ // Allow for quick data access: idx = llen_code OR dist_code
      for ( zseb_16_t idx = 0; idx < size; idx++ ){
         tree[ idx ].child[ 0 ] = idx;   // llen_code or dist_code
         tree[ idx ].child[ 1 ] = idx;   // llen_code or dist_code
         tree[ idx ].info = stat[ idx ]; // Bit length
         tree[ idx ].data = next_code[ stat[ idx ] ]; // Bit sequence
         next_code[ stat[ idx ] ] += 1;  // If stat[ idx ] > 0: OK, if stat[ idx ] == 0: never accessed, also OK
      }
   }

   if ( option == 'I' ){ // Need to build the entire tree

      // Number of symbols with non-zero frequency
      zseb_16_t num = 0;
      for ( zseb_08_t nbits = 1; nbits <= ZSEB_MAX_BITS; nbits++ ){ num += bl_count[ nbits ]; }

      const zseb_16_t num_nonzero = num;
      const zseb_16_t total       = 2 * num_nonzero - 1;

      num = num_nonzero - 1;
      for ( zseb_16_t idx = 0; idx < size; idx++ ){
         if ( stat[ idx ] != 0 ){
            tree[ num ].child[ 0 ] = idx;
            tree[ num ].child[ 1 ] = idx; // Same code: leaf node & symbol llen_code or dist_code
            tree[ num ].info = stat[ idx ]; // Bit length
            tree[ num ].data = next_code[ stat[ idx ] ]; // Bit sequence
            next_code[ stat[ idx ] ] += 1;
            num += 1;
         }
      }
      assert( num == total );

      for ( zseb_16_t idx = 0; idx < total; idx++ ){ work[ idx ] = false; }

      num = num_nonzero - 1;
      while ( num != 0 ){
         for ( zseb_16_t idx1 = num; idx1 < total; idx1++ ){
            if ( work[ idx1 ] == false ){
               for ( zseb_16_t idx2 = idx1 + 1; idx2 < total; idx2++ ){
                  if ( work[ idx2 ] == false ){
                     if ( ( tree[ idx1 ].info == tree[ idx2 ].info ) && ( tree[ idx1 ].data == ( 1U ^ ( tree[ idx2 ].data ) ) ) ){ // Same bit length & only different in last bit
                        //std::cout << "( idx1, info, data ) = ( " << idx1 << " ; " << tree[ idx1 ].info << " ; " << tree[ idx1 ].data << " )" << std::endl;
                        //std::cout << "( idx2, info, data ) = ( " << idx2 << " ; " << tree[ idx2 ].info << " ; " << tree[ idx2 ].data << " )" << std::endl;
                        num -= 1;
                        if ( ( ( tree[ idx1 ].data ) & 1U ) == 0 ){ // Last bit of tree[ idx1 ].data is a zero
                           tree[ num ].child[ 0 ] = idx1;
                           tree[ num ].child[ 1 ] = idx2; // Different code: intermediate node
                        } else { // Last bit of tree[ idx1 ].data is a one
                           tree[ num ].child[ 0 ] = idx2;
                           tree[ num ].child[ 1 ] = idx1; // Different code: intermediate node
                        }
                        tree[ num ].info = tree[ idx1 ].info - 1; // Bit length one less
                        tree[ num ].data = ( ( tree[ idx1 ].data ) >> 1 ); // Remove last bit
                        //std::cout << "-->( num, info, data ) = ( " << num << " ; " << tree[ num ].info << " ; " <<  tree[ num ].data << " )" << std::endl;
                        work[ idx1 ] = true;
                        work[ idx2 ] = true;
                        idx2 = total; // Escape inner for loop
                     }
                  }
               }
            }
         }
      }
   }

}

void zseb::huffman::__prefix_lengths__( zseb_16_t * stat, const zseb_16_t size, zseb_node * tree, const zseb_16_t ZSEB_MAX_BITS ){

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

   // Decrement lengths > ZSEB_MAX_BITS
   zseb_16_t tomove = 0;
   for ( zseb_16_t pack = 0; pack < num; pack++ ){
      if ( tree[ pack ].info > ZSEB_MAX_BITS ){ // Bit length
         tomove += ( tree[ pack ].info - ZSEB_MAX_BITS ); // Excess bit length
         tree[ pack ].info = ZSEB_MAX_BITS; // Bit length
      }
   }

   // Move decrements: Least harm? Lengths < ZSEB_MAX_BITS with smallest associated frequency
   while ( tomove > 0 ){
      zseb_16_t rare = ZSEB_MAX_16T;
      for ( zseb_16_t pack = 0; pack < num; pack++ ){
         if ( tree[ pack ].info < ZSEB_MAX_BITS ){ // Bit length allows for increment
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
   for ( zseb_16_t pack = 0; pack < num; pack++ ){
      const zseb_16_t code = tree[ pack ].child[ 0 ];
      stat[ code ] = tree[ pack ].info; // Bit length
   }

}



