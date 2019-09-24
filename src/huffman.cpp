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

const zseb_08_t zseb::huffman::map_ssq[ 19 ] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 }; // at pos = 0, sym = '16' (rep previous)

zseb::huffman::huffman(){

   stat_comb = new zseb_16_t[ ZSEB_HUF_COMBI ];
   stat_ssq  = new zseb_16_t[ ZSEB_HUF_SSQ   ];
   tree_llen = new zseb_node[ ZSEB_HUF_TREE_LLEN ];
   tree_dist = new zseb_node[ ZSEB_HUF_TREE_DIST ];
   tree_ssq  = new zseb_node[ ZSEB_HUF_TREE_SSQ  ];
   work      = new      bool[ ZSEB_HUF_TREE_LLEN ];

}

zseb::huffman::~huffman(){

   delete [] stat_comb;
   delete [] stat_ssq;
   delete [] tree_llen;
   delete [] tree_dist;
   delete [] tree_ssq;
   delete [] work;

}

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

void zseb::huffman::load_tree( stream * zipfile ){

   for ( zseb_16_t cnt = 0; cnt < ZSEB_HUF_COMBI; cnt++ ){ stat_comb[ cnt ] = 0; }
   for ( zseb_16_t cnt = 0; cnt < ZSEB_HUF_SSQ;   cnt++ ){ stat_ssq [ cnt ] = 0; }

   HLIT  = ( zseb_16_t )( zipfile->read( 5 ) ) + 257;
   HDIST = ( zseb_16_t )( zipfile->read( 5 ) ) + 1;
   HCLEN = ( zseb_16_t )( zipfile->read( 4 ) ) + 4;

   for ( zseb_16_t idx_pos = 0; idx_pos < HCLEN; idx_pos++ ){
      stat_ssq[ map_ssq[ idx_pos ] ] = ( zseb_16_t )( zipfile->read( 3 ) ); // CCL of RLE symbols; stat_ssq in idx_sym
   }

   // Build tree: on output tree[ idx ].( info, data ) = bit ( length, sequence ) of SSQ; tree_ssq in idx_sym
   __build_tree__( stat_ssq, ZSEB_HUF_SSQ, tree_ssq, work, 'I', ZSEB_MAX_BITS_SSQ );

   // Quote from RFC 1951: all CL form a single sequence of HLIT + HDIST + 258 values
   __CL_unpack__( zipfile, tree_ssq, HLIT + HDIST, stat_comb );
   zseb_16_t * stat_dist = stat_comb + HLIT;

   // Build trees
   __build_tree__( stat_comb, HLIT , tree_llen, work, 'I', ZSEB_MAX_BITS_LLD );
   __build_tree__( stat_dist, HDIST, tree_dist, work, 'I', ZSEB_MAX_BITS_LLD );

}

void zseb::huffman::calc_tree( zseb_08_t * llen_pack, zseb_16_t * dist_pack, const zseb_32_t size ){

   size_X1 = 3; //   Fixed Huffman tree: X01
   size_X2 = 3; // Dynamic Huffman tree: X10

   zseb_16_t * stat_llen = stat_comb;
   zseb_16_t * stat_dist = stat_llen + ZSEB_HUF_LLEN;

   for ( zseb_16_t cnt = 0; cnt < ZSEB_HUF_COMBI; cnt++ ){ stat_comb[ cnt ] = 0; }
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

   // Fixed Huffman tree contributions 'pack' function
   for ( zseb_16_t cnt = 0;   cnt < 144; cnt++ ){ size_X1 += (   8                         * stat_llen[ cnt ] ); } // '8' x 144
   for ( zseb_16_t cnt = 144; cnt < 256; cnt++ ){ size_X1 += (   9                         * stat_llen[ cnt ] ); } // '9' x 112
                                                { size_X1 += (   7                         * stat_llen[ 256 ] ); } // '7' x 1 (STOP CODON)
   for ( zseb_16_t cnt = 257; cnt < 280; cnt++ ){ size_X1 += ( ( 7 + __len_bits__( cnt ) ) * stat_llen[ cnt ] ); } // '7' x 23
   for ( zseb_16_t cnt = 280; cnt < 286; cnt++ ){ size_X1 += ( ( 8 + __len_bits__( cnt ) ) * stat_llen[ cnt ] ); } // '8' x 6 (286, 287 not encountered)
   for ( zseb_16_t cnt = 0;   cnt < 30;  cnt++ ){ size_X1 += ( ( 5 +     bit_dist[ cnt ] ) * stat_dist[ cnt ] ); } // All dist CL 5 (30, 31 not encountered)

   // Huffman CL: input(stat) = freq; output(stat) = CL; output(tree)[ pack < num ].{info, data} = {bit length, frequency}
   const zseb_16_t num_llen = __prefix_lengths__( stat_llen, ZSEB_HUF_LLEN, tree_llen, ZSEB_MAX_BITS_LLD );
   const zseb_16_t num_dist = __prefix_lengths__( stat_dist, ZSEB_HUF_DIST, tree_dist, ZSEB_MAX_BITS_LLD );

   // Dynamic Huffman tree contributions 'pack' function
   for ( zseb_16_t cnt = 0; cnt < num_llen; cnt++ ){
      const zseb_16_t len_code = tree_llen[ cnt ].child[ 0 ];
      const zseb_16_t len_nbit = ( ( len_code > ZSEB_LITLEN ) ? __len_bits__( len_code ) : 0 );
      size_X2 += ( ( tree_llen[ cnt ].info + len_nbit ) * tree_llen[ cnt ].data );
   }
   for ( zseb_16_t cnt = 0; cnt < num_dist; cnt++ ){
      const zseb_16_t dist_code = tree_dist[ cnt ].child[ 0 ];
      const zseb_16_t dist_nbit = bit_dist[ dist_code ];
      size_X2 += ( ( tree_dist[ cnt ].info + dist_nbit ) * tree_dist[ cnt ].data );
   }

   // Retrieve the length of the non-zero CL
   HLIT  = ZSEB_HUF_LLEN; while ( stat_llen[ HLIT  - 1 ] == 0 ){ HLIT  -= 1; }
   HDIST = ZSEB_HUF_DIST; while ( stat_dist[ HDIST - 1 ] == 0 ){ HDIST -= 1; }

   // Build tree: on output tree[ idx ].( info, data ) = bit ( length, inverse[sequence] )
   __build_tree__( stat_llen, HLIT , tree_llen, work, 'O', ZSEB_MAX_BITS_LLD );
   __build_tree__( stat_dist, HDIST, tree_dist, work, 'O', ZSEB_MAX_BITS_LLD );

   // Quote from RFC 1951: all code lengths form a single sequence of HLIT + HDIST + 258 values
   for ( zseb_16_t count = 0; count < HDIST; count++ ){
      stat_comb[ HLIT + count ] = stat_comb[ ZSEB_HUF_LLEN + count ];
   }

   // Create the RLE for the CL
   size_ssq = __ssq_creation__( stat_comb, HLIT + HDIST );

   // Gather statistics on RLE: If sym >= 16, a number to be represented in bit sequence follows; stat_ssq frequencies in idx_sym
   for ( zseb_16_t count = 0; count < size_ssq; count++ ){ stat_ssq[ stat_comb[ count ] ] += 1; if ( stat_comb[ count ] >= 16 ){ count++; } }

   // Huffman CCL: input(stat) = freq; output(stat) = CCL; output(tree)[ pack < num ].{info, data} = {bit length, frequency}; stat_ssq in idx_sym
   const zseb_16_t num_ssq = __prefix_lengths__( stat_ssq, ZSEB_HUF_SSQ, tree_ssq, ZSEB_MAX_BITS_SSQ );

   // Retrieve the length of the non-zero CCL; stat_ssq in idx_sym and HCLEN in idx_pos
   HCLEN = ZSEB_HUF_SSQ; while ( stat_ssq[ map_ssq[ HCLEN - 1 ] ] == 0 ){ HCLEN -= 1; }

   // Dynamic Huffman tree contributions 'write_tree' function
   size_X2 += ( 5 + 5 + 4 ); // HLIT, HDIST, HCLEN
   size_X2 += ( HCLEN * 3 ); // stat_ssq
   for ( zseb_16_t cnt = 0; cnt < num_ssq; cnt++ ){ // stat_comb
      const zseb_16_t ssq_code = tree_ssq[ cnt ].child[ 0 ];
      const zseb_16_t ssq_nbit = ( ( ssq_code >= 16 ) ? ( ( ssq_code == 16 ) ? 2 : ( ( ssq_code == 17 ) ? 3 : 7 ) ) : 0 );
      size_X2 += ( ( tree_ssq[ cnt ].info + ssq_nbit ) * tree_ssq[ cnt ].data );
   }

   // Build tree: on output tree[ idx ].( info, data ) = bit ( length, reverse[sequence] ); tree_ssq in idx_sym
   __build_tree__( stat_ssq, ZSEB_HUF_SSQ, tree_ssq, work, 'O', ZSEB_MAX_BITS_SSQ );

}

void zseb::huffman::write_tree( stream * zipfile ) const{

   assert( HLIT  >= 257 ); zipfile->write( HLIT  - 257, 5 );
   assert( HDIST >= 1 );   zipfile->write( HDIST - 1,   5 );
   assert( HCLEN >= 4 );   zipfile->write( HCLEN - 4,   4 );

   for ( zseb_16_t idx_pos = 0; idx_pos < HCLEN; idx_pos++ ){
      zipfile->write( stat_ssq[ map_ssq[ idx_pos ] ], 3 ); // CCL of RLE in idx_pos
   }

   for ( zseb_16_t idx = 0; idx < size_ssq; idx++ ){
      const zseb_16_t idx_sym = stat_comb[ idx ];
      zipfile->write( tree_ssq[ idx_sym ].data, tree_ssq[ idx_sym ].info ); // RLE symbols in CCL codons; tree_ssq in idx_sym
      if ( idx_sym >= 16 ){
         const zseb_16_t num_bits = ( ( idx_sym == 16 ) ? 2 : ( ( idx_sym == 17 ) ? 3 : 7 ) );
         zipfile->write( stat_comb[ idx + 1 ], num_bits ); // Shifts
         idx++;
      }
   }

}

void zseb::huffman::fixed_tree( const char modus ){

   assert( ( modus == 'I' ) || ( modus == 'O' ) );

   zseb_16_t * stat_llen = stat_comb;
   zseb_16_t * stat_dist = stat_llen + ZSEB_HUF_LLEN;

   for ( zseb_16_t cnt = 0;   cnt < 144; cnt++ ){ stat_llen[ cnt ] = 8; } // '8' x 144
   for ( zseb_16_t cnt = 144; cnt < 256; cnt++ ){ stat_llen[ cnt ] = 9; } // '9' x 112
   for ( zseb_16_t cnt = 256; cnt < 280; cnt++ ){ stat_llen[ cnt ] = 7; } // '7' x 24
   for ( zseb_16_t cnt = 280; cnt < 288; cnt++ ){ stat_llen[ cnt ] = 8; } // '8' x 8
   for ( zseb_16_t cnt = 0;   cnt < 32;  cnt++ ){ stat_dist[ cnt ] = 5; } // All dist CL 5

   // Build trees
   __build_tree__( stat_llen, ZSEB_HUF_LLEN, tree_llen, work, modus, ZSEB_MAX_BITS_LLD );
   __build_tree__( stat_dist, ZSEB_HUF_DIST, tree_dist, work, modus, ZSEB_MAX_BITS_LLD );

}

bool zseb::huffman::unpack( stream * zipfile, zseb_08_t * llen_pack, zseb_16_t * dist_pack, zseb_32_t &wr_current, const zseb_32_t maxsize_pack ){

   assert( wr_current == 0 );

   // Ready to read-in!
   zseb_16_t llen_code = 0;

   while ( ( llen_code != ZSEB_LITLEN ) && ( wr_current < maxsize_pack ) ){

      llen_code = __get_sym__( zipfile, tree_llen );

      if ( llen_code < ZSEB_LITLEN ){ // unpack literal

         llen_pack[ wr_current ] = llen_code;
         dist_pack[ wr_current ] = ZSEB_MAX_16T;
         wr_current += 1;

      }

      if ( llen_code > ZSEB_LITLEN ){ // unpack ( len, dist ) pair

         zseb_16_t len_shft = __len_base__( llen_code );
         zseb_16_t len_nbit = __len_bits__( llen_code );
         if ( len_nbit > 0 ){
            len_shft = len_shft + ( zseb_16_t )( zipfile->read( len_nbit ) );
         }

         zseb_16_t dist_code = __get_sym__( zipfile, tree_dist );
         zseb_16_t dist_shft = add_dist[ dist_code ];
         zseb_16_t dist_nbit = bit_dist[ dist_code ];
         if ( dist_nbit > 0 ){
            dist_shft = dist_shft + ( zseb_16_t )( zipfile->read( dist_nbit ) );
         }

         llen_pack[ wr_current ] = len_shft;
         dist_pack[ wr_current ] = dist_shft;
         wr_current += 1;

      }
   }

   if ( llen_code != ZSEB_LITLEN ){
      assert( wr_current == maxsize_pack );
      return true;
   }

   return false;

}

void zseb::huffman::pack( stream * zipfile, zseb_08_t * llen_pack, zseb_16_t * dist_pack, const zseb_32_t size ){

   for ( zseb_32_t idx = 0; idx < size; idx++ ){
      if ( dist_pack[ idx ] == ZSEB_MAX_16T ){
         const zseb_16_t lit_code = llen_pack[ idx ];
         zipfile->write( tree_llen[ lit_code ].data, tree_llen[ lit_code ].info ); // Literal
      } else {
         const zseb_16_t len_code = __len_code__( llen_pack[ idx ] );
         const zseb_16_t len_nbit = __len_bits__( len_code );
         zipfile->write( tree_llen[ len_code ].data, tree_llen[ len_code ].info ); // Length codon
         if ( len_nbit > 0 ){
            const zseb_16_t len_plus = llen_pack[ idx ] - __len_base__( len_code );
            zipfile->write( len_plus, len_nbit ); // Shifts
         }
         const zseb_16_t dist_code = __dist_code__( dist_pack[ idx ] );
         const zseb_16_t dist_nbit = bit_dist[ dist_code ];
         zipfile->write( tree_dist[ dist_code ].data, tree_dist[ dist_code ].info ); // Dist codon
         if ( dist_nbit > 0 ){
            const zseb_16_t dist_plus = dist_pack[ idx ] - add_dist[ dist_code ];
            zipfile->write( dist_plus, dist_nbit ); // Shifts
         }
      }
   }
   zipfile->write( tree_llen[ ZSEB_LITLEN ].data, tree_llen[ ZSEB_LITLEN ].info ); // Stop codon

}

zseb_16_t zseb::huffman::__get_sym__( stream * zipfile, zseb_node * tree ){

   zseb_16_t idx = 0; // start at root
   zseb_32_t bit;

   while ( tree[ idx ].child[ 0 ] != tree[ idx ].child[ 1 ] ){ // not a leaf
      bit = zipfile->read( 1 ); // read-in bit
      idx = tree[ idx ].child[ bit ]; // goto child
   }

   idx = tree[ idx ].child[ 0 ]; // symbol the leaf represents
   return idx;

}

void zseb::huffman::__CL_unpack__( stream * zipfile, zseb_node * tree, const zseb_16_t size, zseb_16_t * stat ){

   zseb_32_t bit;
   zseb_16_t size_part = 0;
   zseb_16_t idx_sym;

   while ( size_part < size ){
      idx_sym = __get_sym__( zipfile, tree );

      if ( idx_sym < 16 ){
         stat[ size_part ] = idx_sym;
         size_part += 1;
      }

      if ( idx_sym == 16 ){
         assert( size_part > 0 );
         bit = 3 + zipfile->read( 2 ); // num repeats
         while ( bit > 0 ){
            stat[ size_part ] = stat[ size_part - 1 ];
            size_part += 1;
            bit -= 1;
         }
      }

      if ( idx_sym == 17 ){
         bit = 3 + zipfile->read( 3 ); // num repeats
         while ( bit > 0 ){
            stat[ size_part ] = 0;
            size_part += 1;
            bit -= 1;
         }
      }

      if ( idx_sym == 18 ){
         bit = 11 + zipfile->read( 7 ); // num repeats
         while ( bit > 0 ){
            stat[ size_part ] = 0;
            size_part += 1;
            bit -= 1;
         }
      }

   }

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
   assert( idx_first == size );

   return idx_stat;

}

zseb_16_t zseb::huffman::__bit_reverse__( zseb_16_t code, const zseb_16_t nbits ){

   zseb_16_t result = 0;

   for ( zseb_16_t bit = 0; bit < nbits; bit++ ){
      result = ( result << 1 ) | ( code & 1U ); // append LSB of code
      code   = ( code >> 1 ); // remove LSB from code
   }

   return result;

}

void zseb::huffman::__build_tree__( zseb_16_t * stat, const zseb_16_t size, zseb_node * tree, bool * temp, const char option, const zseb_16_t ZSEB_MAX_BITS ){

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
         tree[ idx ].data = __bit_reverse__( next_code[ stat[ idx ] ], stat[ idx ] ); // !!! Bit sequence in REVERSE !!! ( LSB read in first in __read__ )
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
            tree[ num ].data = next_code[ stat[ idx ] ]; // !!! Bit sequence NOT in reverse !!! ( tree[ x ].data never read during 'I'nput )
            next_code[ stat[ idx ] ] += 1;
            num += 1;
         }
      }
      assert( num == total );

      for ( zseb_16_t idx = 0; idx < total; idx++ ){ temp[ idx ] = false; }

      num = num_nonzero - 1;
      while ( num != 0 ){
         for ( zseb_16_t idx1 = num; idx1 < total; idx1++ ){
            if ( temp[ idx1 ] == false ){
               for ( zseb_16_t idx2 = idx1 + 1; idx2 < total; idx2++ ){
                  if ( temp[ idx2 ] == false ){
                     if ( ( tree[ idx1 ].info == tree[ idx2 ].info ) && ( tree[ idx1 ].data == ( 1U ^ ( tree[ idx2 ].data ) ) ) ){ // Same bit length & only different in last bit
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
                        temp[ idx1 ] = true;
                        temp[ idx2 ] = true;
                        idx2 = total; // Escape inner for loop
                     }
                  }
               }
            }
         }
      }
      assert( tree[ 0 ].info == 0 );
      for ( zseb_16_t idx1 = 1; idx1 < total; idx1++ ){ assert( temp[ idx1 ] == true ); }
   }

}

zseb_16_t zseb::huffman::__prefix_lengths__( zseb_16_t * stat, const zseb_16_t size, zseb_node * tree, const zseb_16_t ZSEB_MAX_BITS ){

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

   if ( num > 0 ){ // For very small chunks, there may be no DIST codes, for example

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
      for ( zseb_16_t extra = root; extra >= num; extra-- ){ // Only non-leafs
         for ( zseb_16_t chld = 0; chld < 2; chld++ ){
            tree[ tree[ extra ].child[ chld ] ].info = tree[ extra ].info + 1; // Bit length
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
                     rare = pack; // Assign even rarer encounter
                  }
               }
            }
         }
         tree[ rare ].info += 1; // Bit length increment
         tomove -= 1;
      }

      // Repack to stat: on output: tree[ pack < num ].{info, data} = {bit length, frequency}
      for ( zseb_16_t pack = 0; pack < num; pack++ ){
         const zseb_16_t code = tree[ pack ].child[ 0 ];
         stat[ code ] = tree[ pack ].info; // Bit length
      }

   }

   return num;

}



