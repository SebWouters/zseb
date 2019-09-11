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
#include "zseb.h"

zseb::lzss::lzss( std::string fullfile, const char modus ){

   assert( ( modus == 'Z' ) || ( modus == 'U' ) );

   size_lzss = 0;
   size_file = 0;
   frame     = NULL;
   hash_last = NULL;
   hash_ptrs = NULL;

   if ( modus == 'Z' ){

      file.open( fullfile.c_str(), std::ios::in|std::ios::binary|std::ios::ate );

      if ( file.is_open() ){

         size_file = ( zseb_64_t )( file.tellg() );
         file.seekg( 0, std::ios::beg );

         frame = new char[ ZSEB_FRAME ];
         rd_shift = 0;
         rd_end = 0;
         rd_current = 0;

         hash_last = new zseb_32_t[ ZSEB_HASH_SIZE ];
         hash_ptrs = new zseb_32_t[ ZSEB_FRAME ];
         for ( zseb_32_t cnt = 0; cnt < ZSEB_HASH_SIZE;  cnt++ ){ hash_last[ cnt ] = ZSEB_HASH_STOP; }
         for ( zseb_32_t cnt = 0; cnt < ZSEB_FRAME;      cnt++ ){ hash_ptrs[ cnt ] = ZSEB_HASH_STOP; }

         __readin__(); // Get ready for work

      } else {

         std::cerr << "zseb::lzss: Unable to open " << fullfile << "." << std::endl;

      }
   }

   if ( modus == 'U' ){

      file.open( fullfile.c_str(), std::ios::out|std::ios::binary|std::ios::trunc );

      frame = new char[ ZSEB_FRAME ];
      rd_shift = 0;
      rd_end = 0;
      rd_current = 0;

   }

}

zseb::lzss::~lzss(){

   if ( file.is_open() ){ file.close(); }
   if ( frame      != NULL ){ delete [] frame;     }
   if ( hash_ptrs  != NULL ){ delete [] hash_ptrs; }
   if ( hash_last  != NULL ){ delete [] hash_last; }

}

void zseb::lzss::flush(){

   file.write( frame, rd_current );
   rd_current = 0;
   size_file = ( zseb_64_t )( file.tellg() );

}

void zseb::lzss::inflate( zseb_08_t * llen_pack, zseb_16_t * dist_pack, const zseb_32_t size_pack ){

   for ( zseb_32_t idx = 0; idx < size_pack; idx++ ){

      if ( dist_pack[ idx ] == ZSEB_MAX_16T ){ // write LIT

         frame[ rd_current ] = llen_pack[ idx ];
         rd_current += 1;

      } else { // copy ( LEN, DIST )

         zseb_32_t distance = dist_pack[ idx ] + 1;
         zseb_32_t length   = llen_pack[ idx ];
                   length   = length + 3;

         for ( zseb_32_t cnt = 0; cnt < length; cnt++ ){
            frame[ rd_current + cnt ] = frame[ rd_current - distance + cnt ];
         }
         rd_current += length;

      }

      if ( rd_current > ZSEB_TRIGGER ){

         file.write( frame, ZSEB_SHIFT );
         for ( zseb_32_t cnt = 0; cnt < ( rd_current - ZSEB_SHIFT ); cnt++ ){
            frame[ cnt ] = frame[ ZSEB_SHIFT + cnt ];
         }
         rd_current -= ZSEB_SHIFT;

      }

   }

   size_lzss += size_pack;

}

bool zseb::lzss::deflate( zseb_08_t * llen_pack, zseb_16_t * dist_pack, const zseb_32_t size_pack, zseb_32_t &wr_current ){

   zseb_32_t longest_ptr0;
   zseb_16_t longest_len0 = 3; // No reuse of ( ptr1, len1 ) data initially
   zseb_32_t longest_ptr1;
   zseb_16_t longest_len1;
   zseb_32_t hash_entry = 0;
   zseb_32_t next_entry;

   if ( rd_current <= rd_end - 3 ){
      hash_entry =                                     ( zseb_08_t )( frame[ rd_current     ] );
      hash_entry = ( hash_entry << ZSEB_LITLEN_BIT ) | ( zseb_08_t )( frame[ rd_current + 1 ] );
      hash_entry = ( hash_entry << ZSEB_LITLEN_BIT ) | ( zseb_08_t )( frame[ rd_current + 2 ] );
   }

   wr_current = 0;

   while ( ( rd_current < rd_end - 3 ) && ( wr_current < size_pack ) ){

      next_entry = ( ( zseb_08_t )( frame[ rd_current + 3 ] ) ) | ( ( hash_entry << ZSEB_LITLEN_BIT ) & ZSEB_HASH_MASK );
      if ( longest_len0 == 1 ){
         longest_ptr0 = longest_ptr1;
         longest_len0 = longest_len1;
      } else {
      __longest_match__( longest_ptr0, longest_len0, hash_entry, rd_current     ); }
      __longest_match__( longest_ptr1, longest_len1, next_entry, rd_current + 1 );

      if ( ( longest_ptr0 == ZSEB_HASH_STOP ) || ( longest_len1 > longest_len0 ) ){ // lazy evaluation
         __append_lit_encode__( llen_pack, dist_pack, wr_current );         
         longest_len0 = 1;
      } else {
         __append_len_encode__( llen_pack, dist_pack, wr_current, rd_current - longest_ptr0 - 1, longest_len0 - ZSEB_LENGTH_SHIFT );
      }

      __move_hash__( hash_entry );
      hash_entry = next_entry;
      for ( zseb_16_t cnt = 1; cnt < longest_len0; cnt++ ){
         __move_hash__( hash_entry );
         hash_entry = ( ( zseb_08_t )( frame[ rd_current + 2 ] ) ) | ( ( hash_entry << ZSEB_LITLEN_BIT ) & ZSEB_HASH_MASK );
      }

      if ( ( rd_end == ZSEB_FRAME ) && ( rd_current > ZSEB_TRIGGER ) ){
         __shift_left__();
         if ( longest_ptr1 != ZSEB_HASH_STOP ){ longest_ptr1 = longest_ptr1 ^ ZSEB_SHIFT; } // longest_ptr1 >= rd_current + 1 - ZSEB_HISTORY >= ZSEB_SHIFT
         __readin__();
      }

   }

   if ( wr_current >= size_pack ){

      assert( wr_current == size_pack );
      return false; // Not yet last block

   } else {

      if ( rd_current == rd_end - 3 ){ // frame[ rd_end - 3, rd_end - 2, rd_end - 1 ] may be last triplet

         longest_ptr0 = hash_last[ hash_entry ];
         if ( ( longest_ptr0 != ZSEB_HASH_STOP ) && ( longest_ptr0 + ZSEB_HISTORY >= rd_current ) ){
            longest_len0 = 3;
            __append_len_encode__( llen_pack, dist_pack, wr_current, rd_current - longest_ptr0 - 1, longest_len0 - ZSEB_LENGTH_SHIFT );
            // No longer update the hash_ptrs and hash_last
            rd_current += longest_len0;
         }
      }

      while ( rd_current < rd_end ){
         __append_lit_encode__( llen_pack, dist_pack, wr_current );
         rd_current += 1;
      }

      return true; // Last block

   }

   return true; // Last block

}

zseb_64_t zseb::lzss::get_lzss_bits() const{ return size_lzss; }

zseb_64_t zseb::lzss::get_file_bytes() const{ return size_file; }

void zseb::lzss::__shift_left__(){

   assert( rd_current <  rd_end );
   assert( ZSEB_SHIFT <= rd_current );

   /* ZSEB_SHIFT = 2^16 <= temp < ZSEB_FRAME < 2^17: temp - ZSEB_SHIFT = temp ^ ZSEB_SHIFT ( XOR, bit toggle 2^16 )
                      0 <= cnt  < ZSEB_SHIFT = 2^16: cnt  + ZSEB_SHIFT = cnt  ^ ZSEB_SHIFT ( XOR, bit toggle 2^16 ) */

   for ( zseb_32_t cnt = 0; cnt < ( rd_end - ZSEB_SHIFT ); cnt++ ){
      frame[ cnt ] = frame[ cnt + ZSEB_SHIFT ];
   }
   for ( zseb_32_t cnt = 0; cnt < ( rd_end - ZSEB_SHIFT ); cnt++ ){ // cnt < ZSEB_SHIFT
      const zseb_32_t temp = hash_ptrs[ cnt + ZSEB_SHIFT ];
      hash_ptrs[ cnt ] = ( ( ( temp == ZSEB_HASH_STOP ) || ( temp < ZSEB_SHIFT ) ) ? ( ZSEB_HASH_STOP ) : ( temp ^ ZSEB_SHIFT ) ); // temp - ZSEB_SHIFT
   }
   for ( zseb_32_t abc = 0; abc < ZSEB_HASH_SIZE; abc++ ){
      const zseb_32_t temp = hash_last[ abc ];
      hash_last[ abc ] = ( ( ( temp == ZSEB_HASH_STOP ) || ( temp < ZSEB_SHIFT ) ) ? ( ZSEB_HASH_STOP ) : ( temp ^ ZSEB_SHIFT ) ); // temp - ZSEB_SHIFT
   }
   rd_shift   += ZSEB_SHIFT;
   rd_end     -= ZSEB_SHIFT;
   rd_current -= ZSEB_SHIFT;

}

void zseb::lzss::__readin__(){

   const zseb_32_t current_read = ( ( rd_shift + ZSEB_FRAME > size_file ) ? ( size_file - rd_shift - rd_end ) : ( ZSEB_FRAME - rd_end ) );
   file.read( frame + rd_end, current_read );
   rd_end += current_read;

}

void zseb::lzss::__move_hash__( const zseb_32_t hash_entry ){

   hash_ptrs[ rd_current ] = hash_last[ hash_entry ];
   hash_last[ hash_entry ] = rd_current;
   rd_current += 1;

}

void zseb::lzss::__longest_match__( zseb_32_t &result_ptr, zseb_16_t &result_len, const zseb_32_t hash_entry, const zseb_32_t position ) const{

   zseb_32_t pointer = hash_last[ hash_entry ];
   result_ptr = ZSEB_HASH_STOP;
   result_len = 1;
   while ( ( pointer != ZSEB_HASH_STOP ) &&
           ( pointer + ZSEB_HISTORY >= position ) && // >= because dist - 1 = position - pointer - 1 < ZSEB_HISTORY is stored
           ( result_len < ZSEB_LENGTH_MAX ) ){
      zseb_16_t length = 3;
      bool match = true;
      while ( ( position + length < rd_end ) && ( length < ZSEB_LENGTH_MAX ) && ( match ) ){
         if ( frame[ pointer + length ] == frame[ position + length ] ){
            length += 1;
         } else {
            match = false;
         }
      }
      if ( length > result_len ){
         result_len = length;
         result_ptr = pointer;
      }
      pointer = hash_ptrs[ pointer ];
   }

}

void zseb::lzss::__append_lit_encode__( zseb_08_t * llen_pack, zseb_16_t * dist_pack, zseb_32_t &wr_current ){

   size_lzss += ( ZSEB_LITLEN_BIT + 1 ); // 8-bit literal [ 0 : 255 ] + 1-bit differentiator

   llen_pack[ wr_current ] = ( zseb_08_t )( frame[ rd_current ] ); // [ 0 : 255 ]
   dist_pack[ wr_current ] = ZSEB_MAX_16T; // 65535

   wr_current += 1;

}

void zseb::lzss::__append_len_encode__( zseb_08_t * llen_pack, zseb_16_t * dist_pack, zseb_32_t &wr_current, const zseb_16_t dist_shift, const zseb_08_t len_shift ){

   size_lzss += ( ZSEB_HISTORY_BIT + ZSEB_LITLEN_BIT + 1 ); // 15-bit dist_shift [ 0 : 32767 ] + 8-bit len_shift [ 0 : 255 ] + 1-bit differentiator

   llen_pack[ wr_current ] = len_shift;  // [ 0 : 255 ]
   dist_pack[ wr_current ] = dist_shift; // [ 0 : 32767 ]

   wr_current += 1;

}

