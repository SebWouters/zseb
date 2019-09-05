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
#include <limits.h>
#include "zseb.h"

const unsigned char zseb::zseb::bit_len[ 29 ] = { 0, 0, 0, 0, 0, 0, 0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  3,  3,  3,  3,  4,  4,  4,   4,   5,   5,   5,   5,   5 };

const unsigned int  zseb::zseb::add_len[ 29 ] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };

const unsigned char zseb::zseb::bit_dist[ 30 ] = { 0, 0, 0, 0, 1, 1, 2,  2,  3,  3,  4,  4,  5,  5,   6,   6,   7,   7,   8,   8,    9,   9,    10,   10,   11,   11,   12,    12,    13,    13 };

const unsigned int  zseb::zseb::add_dist[ 30 ] = { 0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576 };

void zseb::zseb::__fill_map_dist__(){

   const unsigned char cutoff = 16;

   // Fill 0 - 255 with respective mappings of ( distance - 1 ) --> code
   for ( unsigned char code = 0; code < cutoff; code++ ){
      const unsigned int lower = add_dist[ code ];
      const unsigned int upper = lower + ( 1 << ( bit_dist[ code ] ) );
      for ( unsigned int n = lower; n < upper; n++ ){
         map_dist[ n ] = code;
      }
   }

   map_dist[ add_dist[ cutoff ]     ] = 255; // Error
   map_dist[ add_dist[ cutoff ] + 1 ] = 255; // Error

   // Fill 258 - 511 with respective mappings of ( distance - 1 ) / 128 --> code
   for ( unsigned char code = cutoff; code < 30; code++ ){
      const unsigned int lower = ( add_dist[ code ] >> bit_dist[ cutoff ] );
      const unsigned int upper = lower + ( 1 << ( bit_dist[ code ] - bit_dist[ cutoff ] ) );
      for ( unsigned int n = lower; n < upper; n++ ){
         map_dist[ add_dist[ cutoff ] + n ] = code;
      }
   }

   // shift = distance - 1; code = ( shift < 256 ) ? map_dist[ shift ] : map_dist[ 256 ^ ( shift >> 7 ) ];

}

void zseb::zseb::__fill_map_len__(){

   // Fill 0 - 255
   for ( unsigned char code = 0; code < 29; code++ ){
      const unsigned int lower = add_len[ code ] - add_len[ 0 ];
      const unsigned int upper = lower + ( 1 << bit_len[ code ] );
      for ( unsigned int n = lower; n < upper; n++ ){
         map_len[ n ] = code;
      }
   }

   // code = 257 + map_len[ length - 3 ];

}

zseb::zseb::zseb( std::string toread, std::string towrite, const char unzip ){

   assert( UINT_MAX >= ZSEB_SANITY_CHECK );
   // TODO: define zseb_hash_t and zseb_dist_t as minimal native types with a representation of a number > 2^24 (unsigned int?) and > 2^15 (unsigned short?) respectively
   assert( ( unzip == 'Z' ) || ( unzip == 'U' ) );

   modus = unzip;
   lzss = 0;
   zlib = 0;

   infile.open( toread.c_str(), std::ios::in|std::ios::binary|std::ios::ate );
   if ( infile.is_open() ){

      size = ( unsigned long )( infile.tellg() );
      infile.seekg( 0, std::ios::beg );
      std::cout << "Opened " << toread << " with size " << size << "." << std::endl;

    //outfile.open( towrite.c_str(), std::ios::out|std::ios::binary|std::ios::trunc );

      if ( unzip == 'Z' ){   __zip__(); }
    //if ( unzip == 'U' ){ __unzip__(); }

      std::cout << "LZSS encoding yields reduction of " << 100.0 * ( 1.0 * size - 0.125 * lzss ) / size << "%." << std::endl;

   } else {

      size = 0;
      std::cout << "zseb: Unable to open " << toread << "." << std::endl;
      readframe = NULL;
      rd_shift = 0;
      rd_end = 0;
      rd_current = 0;

      buffer = NULL;
      distance = NULL;
      wr_current = 0;
      stat_lit = NULL;
      stat_dist = NULL;

      hash_last = NULL;
      hash_ptrs = NULL;

   }

}

void zseb::zseb::__zip__(){

   readframe = new char[ ZSEB_BUFFER ];
   rd_shift = 0;
   rd_end = 0;
   rd_current = 0;

   buffer    = new unsigned char[ ZSEB_SHIFT ];
   distance  = new unsigned  int[ ZSEB_SHIFT ];
   stat_lit  = new unsigned  int[ ZSEB_HUF1 ];
   stat_dist = new unsigned  int[ ZSEB_HUF2 ];
   for ( unsigned int cnt = 0; cnt < ZSEB_HUF1; cnt++ ){ stat_lit [ cnt ] = 0; }
   for ( unsigned int cnt = 0; cnt < ZSEB_HUF2; cnt++ ){ stat_dist[ cnt ] = 0; }
   wr_current = 0;

   hash_last = new unsigned int[ ZSEB_HASH_SIZE ];
   hash_ptrs = new unsigned int[ ZSEB_BUFFER ];
   for ( unsigned int cnt = 0; cnt < ZSEB_HASH_SIZE; cnt++ ){ hash_last[ cnt ] = ZSEB_HASH_STOP; }
   for ( unsigned int cnt = 0; cnt < ZSEB_BUFFER;    cnt++ ){ hash_ptrs[ cnt ] = ZSEB_HASH_STOP; }

   __fill_map_len__();
   __fill_map_dist__();

   __readin__();
   __lzss_encode__();

}

zseb::zseb::~zseb(){

   if (  infile.is_open() ){  infile.close(); }
 //if ( outfile.is_open() ){ outfile.close(); }
   if ( readframe != NULL ){ delete [] readframe; }
   if ( buffer    != NULL ){ delete [] buffer; }
   if ( distance  != NULL ){ delete [] distance; }
   if ( stat_lit  != NULL ){ delete [] stat_lit; }
   if ( stat_dist != NULL ){ delete [] stat_dist; }
   if ( hash_ptrs != NULL ){ delete [] hash_ptrs; }
   if ( hash_last != NULL ){ delete [] hash_last; }

}

void zseb::zseb::__shift_left__(){

   assert( rd_current <  rd_end );
   assert( ZSEB_SHIFT <= rd_current );

   /* ZSEB_SHIFT = 2^16 <= temp < ZSEB_BUFFER = 2^17: temp - ZSEB_SHIFT = temp ^ ZSEB_SHIFT ( XOR, bit toggle 2^16 )
                      0 <= cnt  < ZSEB_SHIFT  = 2^16: cnt  + ZSEB_SHIFT = cnt  ^ ZSEB_SHIFT ( XOR, bit toggle 2^16 ) */

   for ( unsigned int cnt = 0; cnt < ( rd_end - ZSEB_SHIFT ); cnt++ ){ // cnt < ZSEB_SHIFT
      readframe[ cnt ] = readframe[ cnt ^ ZSEB_SHIFT ]; // cnt + ZSEB_SHIFT
   }
   for ( unsigned int cnt = 0; cnt < ( rd_end - ZSEB_SHIFT ); cnt++ ){ // cnt < ZSEB_SHIFT
      unsigned int temp = hash_ptrs[ cnt ^ ZSEB_SHIFT ]; // cnt + ZSEB_SHIFT
      hash_ptrs[ cnt ] = ( ( ( temp == ZSEB_HASH_STOP ) || ( temp < ZSEB_SHIFT ) ) ? ( ZSEB_HASH_STOP ) : ( temp ^ ZSEB_SHIFT ) ); // temp - ZSEB_SHIFT
   }
   for ( unsigned int abc = 0; abc < ZSEB_HASH_SIZE; abc++ ){
      unsigned int temp = hash_last[ abc ];
      hash_last[ abc ] = ( ( ( temp == ZSEB_HASH_STOP ) || ( temp < ZSEB_SHIFT ) ) ? ( ZSEB_HASH_STOP ) : ( temp ^ ZSEB_SHIFT ) ); // temp - ZSEB_SHIFT
   }
   rd_shift   += ZSEB_SHIFT;
   rd_end     -= ZSEB_SHIFT;
   rd_current -= ZSEB_SHIFT;

}

void zseb::zseb::__readin__(){

   const unsigned int current_read = ( ( rd_shift + ZSEB_BUFFER > size ) ? ( size - rd_shift - rd_end ) : ( ZSEB_BUFFER - rd_end ) );
   infile.read( readframe + rd_end, current_read );
   rd_end += current_read;

}

void zseb::zseb::__move_up__( unsigned int hash_entry ){

   hash_ptrs[ rd_current ] = hash_last[ hash_entry ];
   hash_last[ hash_entry ] = rd_current;
   rd_current += 1;

}

void zseb::zseb::__longest_match__( unsigned int * l_ptr, unsigned int * l_len, const unsigned int hash_entry, const unsigned int position ) const{

   unsigned int pointer = hash_last[ hash_entry ];
   l_ptr[ 0 ] = ZSEB_HASH_STOP;
   l_len[ 0 ] = 1;
   while ( ( pointer != ZSEB_HASH_STOP ) && ( pointer + ZSEB_HISTORY >= position ) ){ // >= because dist - 1 = position - pointer - 1 < ZSEB_HISTORY is stored
      unsigned int length = 3;
      bool match = true;
      while ( ( position + length < rd_end ) && ( length < ZSEB_STR_LEN ) && ( match ) ){
         if ( readframe[ pointer + length ] == readframe[ position + length ] ){
            length += 1;
         } else {
            match = false;
         }
      }
      if ( length > l_len[ 0 ] ){
         l_len[ 0 ] = length;
         l_ptr[ 0 ] = pointer;
      }
      pointer = hash_ptrs[ pointer ];
   }

}

void zseb::zseb::__append_lit_encode__(){

   lzss += ( ZSEB_NUM_CHAR_BIT + 1 ); // 8-bit literal [0 - 255] + 1-bit lit/len differentiator

   std::cout << readframe[ rd_current ];

   const unsigned char code = ( unsigned char )( readframe[ rd_current ] );
   //buffer  [ wr_current ] = code;
   //distance[ wr_current ] = ZSEB_HASH_STOP;
   stat_lit[ code ] += 1; // [ 0 - 255 ]
   //wr_current += 1;

}

void zseb::zseb::__append_len_encode__( const unsigned int dist_shift, const unsigned int len_shift ){

   lzss += ( ZSEB_HISTORY_BIT + ZSEB_STR_LEN_BIT + 1 ); // 15-bit distance [1 - 32768] + 8-bit length [3 - 258] + 1-bit lit/len differentiator

   for ( unsigned int cnt = 0; cnt < ZSEB_STR_LEN_SHFT + len_shift; cnt++ ){ std::cout << readframe[ rd_current - dist_shift - 1 + cnt ]; }

   const unsigned char len_code  = ZSEB_NUM_CHAR ^ ( 1 + map_len[ len_shift ] ); // code = 257 + map_len[ length - 3 ];
   const unsigned char dist_code = ( ( dist_shift < 256 ) ? map_dist[ dist_shift ] : map_dist[ 256 ^ ( dist_shift >> 7 ) ] );
   //buffer  [ wr_current ] = len_shift;  // [ 0 - 255 ]
   //distance[ wr_current ] = dist_shift; // [ 0 - 32767 ]
   stat_lit [ len_code  ] += 1;
   stat_dist[ dist_code ] += 1;
   //wr_current += 1;

}

void zseb::zseb::__lzss_encode__(){

   unsigned int longest_ptr0;
   unsigned int longest_len0 = 3; // No reuse of ( ptr1, len1 ) data initially
   unsigned int longest_ptr1;
   unsigned int longest_len1;
   unsigned int hash_entry = 0;
   unsigned int next_entry;

   if ( rd_current <= rd_end - 3 ){
      hash_entry =                                       ( unsigned char )( readframe[ rd_current     ] );
      hash_entry = ( hash_entry << ZSEB_NUM_CHAR_BIT ) | ( unsigned char )( readframe[ rd_current + 1 ] );
      hash_entry = ( hash_entry << ZSEB_NUM_CHAR_BIT ) | ( unsigned char )( readframe[ rd_current + 2 ] );
   }

   while ( rd_current < rd_end - 3 ) {

      next_entry = ( ( unsigned char )( readframe[ rd_current + 3 ] ) ) | ( ( hash_entry << ZSEB_NUM_CHAR_BIT ) & ZSEB_HASH_MASK );
      if ( longest_len0 == 1 ){
         longest_ptr0 = longest_ptr1;
         longest_len0 = longest_len1;
      } else {
      __longest_match__( &longest_ptr0, &longest_len0, hash_entry, rd_current     ); }
      __longest_match__( &longest_ptr1, &longest_len1, next_entry, rd_current + 1 );

      if ( ( longest_ptr0 == ZSEB_HASH_STOP ) || ( longest_len1 > longest_len0 ) ){ // lazy evaluation
         __append_lit_encode__();         
         longest_len0 = 1;
      } else {
         __append_len_encode__( rd_current - longest_ptr0 - 1, longest_len0 - ZSEB_STR_LEN_SHFT );
      }

      __move_up__( hash_entry );
      hash_entry = next_entry;
      for ( unsigned int cnt = 1; cnt < longest_len0; cnt++ ){
         __move_up__( hash_entry );
         hash_entry = ( ( unsigned char )( readframe[ rd_current + 2 ] ) ) | ( ( hash_entry << ZSEB_NUM_CHAR_BIT ) & ZSEB_HASH_MASK );
      }

      if ( ( rd_end == ZSEB_BUFFER ) && ( rd_current > ZSEB_READ_TRIGGER ) ){
         __shift_left__();
         longest_ptr1 = longest_ptr1 ^ ZSEB_SHIFT; // longest_ptr1 >= rd_current + 1 - ZSEB_HISTORY >= ZSEB_SHIFT
         __readin__();
      }

   }

   if ( rd_current == rd_end - 3 ){ // mem[ rd_end - 3, rd_end - 2, rd_end - 1 ] may be last triplet

      longest_ptr0 = hash_last[ hash_entry ];
      if ( ( longest_ptr0 != ZSEB_HASH_STOP ) && ( longest_ptr0 + ZSEB_HISTORY >= rd_current ) ){
         longest_len0 = 3;
         __append_len_encode__( rd_current - longest_ptr0 - 1, longest_len0 - ZSEB_STR_LEN_SHFT );
         // No longer update the hash_ptrs and hash_last
         rd_current += longest_len0;
      }
   }

   while ( rd_current < rd_end ){
      __append_lit_encode__();
      rd_current += 1;
   }

}

