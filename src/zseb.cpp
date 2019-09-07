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

zseb::zseb::zseb( std::string toread, std::string towrite, const char unzip ){

   assert( sizeof( zseb_08_t ) == 1 );
   assert( sizeof( zseb_16_t ) == 2 );
   assert( sizeof( zseb_32_t ) == 4 );
   assert( sizeof( zseb_64_t ) == 8 );

   assert( ( unzip == 'Z' ) || ( unzip == 'U' ) );

   lzss = 0;
   zlib = 0;

   infile.open( toread.c_str(), std::ios::in|std::ios::binary|std::ios::ate );
   if ( infile.is_open() ){

      size = ( zseb_64_t )( infile.tellg() );
      infile.seekg( 0, std::ios::beg );

      outfile.open( towrite.c_str(), std::ios::out|std::ios::binary|std::ios::trunc );

      if ( unzip == 'Z' ){   __zip__(); }
    //if ( unzip == 'U' ){ __unzip__(); }

   } else {

      size = 0;
      std::cout << "zseb: Unable to open " << toread << "." << std::endl;
      readframe = NULL;
      rd_shift = 0;
      rd_end = 0;
      rd_current = 0;

      flushframe = NULL;
      flsh_ptr   = 0;
      llen_pack  = NULL;
      dist_pack  = NULL;
      wr_current = 0;

      hash_last = NULL;
      hash_ptrs = NULL;

   }

}

void zseb::zseb::__zip__(){

   readframe = new char[ ZSEB_READ_FRAME ];
   rd_shift = 0;
   rd_end = 0;
   rd_current = 0;

   flushframe = new char[ ZSEB_FLUSH_FRAME ];
   flsh_ptr   = 0;
   llen_pack  = new zseb_08_t[ ZSEB_WR_FRAME ];
   dist_pack  = new zseb_16_t[ ZSEB_WR_FRAME ];
   wr_current = 0;
   for ( zseb_32_t cnt = 0; cnt < ZSEB_FLUSH_FRAME; cnt++ ){ flushframe[ cnt ] = 0; }

   hash_last = new zseb_32_t[ ZSEB_HASH_SIZE ];
   hash_ptrs = new zseb_32_t[ ZSEB_READ_FRAME ];
   for ( zseb_32_t cnt = 0; cnt < ZSEB_HASH_SIZE;  cnt++ ){ hash_last[ cnt ] = ZSEB_HASH_STOP; }
   for ( zseb_32_t cnt = 0; cnt < ZSEB_READ_FRAME; cnt++ ){ hash_ptrs[ cnt ] = ZSEB_HASH_STOP; }

   __readin__();
   __lzss_encode__();

   std::cout << "zseb: reduction: LZSS    = " << 100.0 * ( 1.0 * size - 0.125 * lzss ) / size << "%." << std::endl;
   std::cout << "                 Huffman = " <<  12.5 * ( 1.0 * lzss -   1.0 * zlib ) / size << "%." << std::endl;
   std::cout << "                 Total   = " << 100.0 * ( 1.0 * size - 0.125 * zlib ) / size << "%." << std::endl;

}

zseb::zseb::~zseb(){

   if (  infile.is_open() ){  infile.close(); }
   if ( outfile.is_open() ){ outfile.close(); }
   if ( readframe  != NULL ){ delete [] readframe;  }
   if ( flushframe != NULL ){ delete [] flushframe; }
   if ( llen_pack  != NULL ){ delete [] llen_pack;  }
   if ( dist_pack  != NULL ){ delete [] dist_pack;  }
   if ( hash_ptrs  != NULL ){ delete [] hash_ptrs;  }
   if ( hash_last  != NULL ){ delete [] hash_last;  }

}

void zseb::zseb::__shift_left__(){

   assert( rd_current <  rd_end );
   assert( ZSEB_SHIFT <= rd_current );

   /* ZSEB_SHIFT = 2^16 <= temp < ZSEB_READFRAME = 2^17: temp - ZSEB_SHIFT = temp ^ ZSEB_SHIFT ( XOR, bit toggle 2^16 )
                      0 <= cnt  < ZSEB_SHIFT     = 2^16: cnt  + ZSEB_SHIFT = cnt  ^ ZSEB_SHIFT ( XOR, bit toggle 2^16 ) */

   for ( zseb_32_t cnt = 0; cnt < ( rd_end - ZSEB_SHIFT ); cnt++ ){ // cnt < ZSEB_SHIFT
      readframe[ cnt ] = readframe[ cnt ^ ZSEB_SHIFT ]; // cnt + ZSEB_SHIFT
   }
   for ( zseb_32_t cnt = 0; cnt < ( rd_end - ZSEB_SHIFT ); cnt++ ){ // cnt < ZSEB_SHIFT
      zseb_32_t temp = hash_ptrs[ cnt ^ ZSEB_SHIFT ]; // cnt + ZSEB_SHIFT
      hash_ptrs[ cnt ] = ( ( ( temp == ZSEB_HASH_STOP ) || ( temp < ZSEB_SHIFT ) ) ? ( ZSEB_HASH_STOP ) : ( temp ^ ZSEB_SHIFT ) ); // temp - ZSEB_SHIFT
   }
   for ( zseb_32_t abc = 0; abc < ZSEB_HASH_SIZE; abc++ ){
      zseb_32_t temp = hash_last[ abc ];
      hash_last[ abc ] = ( ( ( temp == ZSEB_HASH_STOP ) || ( temp < ZSEB_SHIFT ) ) ? ( ZSEB_HASH_STOP ) : ( temp ^ ZSEB_SHIFT ) ); // temp - ZSEB_SHIFT
   }
   rd_shift   += ZSEB_SHIFT;
   rd_end     -= ZSEB_SHIFT;
   rd_current -= ZSEB_SHIFT;

}

void zseb::zseb::__readin__(){

   const zseb_32_t current_read = ( ( rd_shift + ZSEB_READ_FRAME > size ) ? ( size - rd_shift - rd_end ) : ( ZSEB_READ_FRAME - rd_end ) );
   infile.read( readframe + rd_end, current_read );
   rd_end += current_read;

}

void zseb::zseb::__writeout__( const bool last ){

   const zseb_32_t nchar = ( flsh_ptr >> 3 );
   //std::cout << "zseb:nchar = " << nchar << std::endl;
   outfile.write( flushframe, nchar );
   flsh_ptr = ( flsh_ptr & 7U );
   flushframe[ 0 ] = flushframe[ nchar ];
   for ( zseb_32_t cnt = 1; cnt < ZSEB_FLUSH_FRAME; cnt++ ){ flushframe[ cnt ] = 0; }

   if ( ( last ) && ( flsh_ptr > 0 ) ){ outfile.write( flushframe, 1 ); }

}

void zseb::zseb::__move_up__( const zseb_32_t hash_entry ){

   hash_ptrs[ rd_current ] = hash_last[ hash_entry ];
   hash_last[ hash_entry ] = rd_current;
   rd_current += 1;

}

void zseb::zseb::__longest_match__( zseb_32_t &result_ptr, zseb_16_t &result_len, const zseb_32_t hash_entry, const zseb_32_t position ) const{

   zseb_32_t pointer = hash_last[ hash_entry ];
   result_ptr = ZSEB_HASH_STOP;
   result_len = 1;
   while ( ( pointer != ZSEB_HASH_STOP ) &&
           ( pointer + ZSEB_HISTORY >= position ) && // >= because dist - 1 = position - pointer - 1 < ZSEB_HISTORY is stored
           ( result_len < ZSEB_LENGTH_MAX ) ) // TODO: check magic 'long enough' numbers gzip
   {
      zseb_16_t length = 3;
      bool match = true;
      while ( ( position + length < rd_end ) && ( length < ZSEB_LENGTH_MAX ) && ( match ) ){
         if ( readframe[ pointer + length ] == readframe[ position + length ] ){
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

void zseb::zseb::__append_lit_encode__(){

   lzss += ( ZSEB_LITLEN_BIT + 1 ); // 8-bit literal [ 0 : 255 ] + 1-bit differentiator

//   std::cout << readframe[ rd_current ];

   llen_pack[ wr_current ] = ( zseb_08_t )( readframe[ rd_current ] ); // [ 0 : 255 ]
   dist_pack[ wr_current ] = ZSEB_MAX_16T; // 65535

   std::cout << llen_pack[ wr_current ];

   wr_current += 1;

}

void zseb::zseb::__append_len_encode__( const zseb_16_t dist_shift, const zseb_08_t len_shift ){

   lzss += ( ZSEB_HISTORY_BIT + ZSEB_LITLEN_BIT + 1 ); // 15-bit dist_shift [ 0 : 32767 ] + 8-bit len_shift [ 0 : 255 ] + 1-bit differentiator

   for ( zseb_16_t cnt = 0; cnt < ( ZSEB_LENGTH_SHIFT + len_shift ); cnt++ ){ std::cout << readframe[ rd_current - ( dist_shift + 1 ) + cnt ]; }

   llen_pack[ wr_current ] = len_shift;  // [ 0 : 255 ]
   dist_pack[ wr_current ] = dist_shift; // [ 0 : 32767 ]
   wr_current += 1;

}

void zseb::zseb::__lzss_encode__(){ // TODO: Move to LZSS class

   zseb_32_t longest_ptr0;
   zseb_16_t longest_len0 = 3; // No reuse of ( ptr1, len1 ) data initially
   zseb_32_t longest_ptr1;
   zseb_16_t longest_len1;
   zseb_32_t hash_entry = 0;
   zseb_32_t next_entry;

   if ( rd_current <= rd_end - 3 ){
      hash_entry =                                     ( zseb_08_t )( readframe[ rd_current     ] );
      hash_entry = ( hash_entry << ZSEB_LITLEN_BIT ) | ( zseb_08_t )( readframe[ rd_current + 1 ] );
      hash_entry = ( hash_entry << ZSEB_LITLEN_BIT ) | ( zseb_08_t )( readframe[ rd_current + 2 ] );
   }

   while ( rd_current < rd_end - 3 ) {

      next_entry = ( ( zseb_08_t )( readframe[ rd_current + 3 ] ) ) | ( ( hash_entry << ZSEB_LITLEN_BIT ) & ZSEB_HASH_MASK );
      if ( longest_len0 == 1 ){
         longest_ptr0 = longest_ptr1;
         longest_len0 = longest_len1;
      } else {
      __longest_match__( longest_ptr0, longest_len0, hash_entry, rd_current     ); }
      __longest_match__( longest_ptr1, longest_len1, next_entry, rd_current + 1 );

      if ( ( longest_ptr0 == ZSEB_HASH_STOP ) || ( longest_len1 > longest_len0 ) ){ // lazy evaluation
         __append_lit_encode__();         
         longest_len0 = 1;
      } else {
         __append_len_encode__( rd_current - longest_ptr0 - 1, longest_len0 - ZSEB_LENGTH_SHIFT );
      }

      __move_up__( hash_entry );
      hash_entry = next_entry;
      for ( zseb_16_t cnt = 1; cnt < longest_len0; cnt++ ){
         __move_up__( hash_entry );
         hash_entry = ( ( zseb_08_t )( readframe[ rd_current + 2 ] ) ) | ( ( hash_entry << ZSEB_LITLEN_BIT ) & ZSEB_HASH_MASK );
      }

      if ( ( rd_end == ZSEB_READ_FRAME ) && ( rd_current > ZSEB_READ_TRIGGER ) ){
         __shift_left__();
         if ( longest_ptr1 != ZSEB_HASH_STOP ){ longest_ptr1 = longest_ptr1 ^ ZSEB_SHIFT; } // longest_ptr1 >= rd_current + 1 - ZSEB_HISTORY >= ZSEB_SHIFT
         __readin__();
         //std::cout << "zseb:trigger read" << std::endl;
      }

      if ( wr_current >= ZSEB_WR_TRIGGER ){
         zseb_64_t flsh_begin = flsh_ptr;
         huffman::pack( flushframe, flsh_ptr, llen_pack, dist_pack, wr_current, false );
         zlib += ( flsh_ptr - flsh_begin );
         __writeout__( false );
         wr_current = 0;
         //std::cout << "zseb:trigger write" << std::endl;
      }

   }

   if ( rd_current == rd_end - 3 ){ // mem[ rd_end - 3, rd_end - 2, rd_end - 1 ] may be last triplet

      longest_ptr0 = hash_last[ hash_entry ];
      if ( ( longest_ptr0 != ZSEB_HASH_STOP ) && ( longest_ptr0 + ZSEB_HISTORY >= rd_current ) ){
         longest_len0 = 3;
         __append_len_encode__( rd_current - longest_ptr0 - 1, longest_len0 - ZSEB_LENGTH_SHIFT );
         // No longer update the hash_ptrs and hash_last
         rd_current += longest_len0;
      }
   }

   while ( rd_current < rd_end ){
      __append_lit_encode__();
      rd_current += 1;
   }

   zseb_64_t flsh_begin = flsh_ptr;
   huffman::pack( flushframe, flsh_ptr, llen_pack, dist_pack, wr_current, true );
   zlib += ( flsh_ptr - flsh_begin );
   __writeout__( true );
   wr_current = 0;

}


