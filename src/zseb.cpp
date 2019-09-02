/*
   zseb: compression
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

zseb::zseb::zseb( std::string toread ){

   assert( UINT_MAX >= ZSEB_SANITY_CHECK );

   infile.open( toread.c_str(), std::ios::in|std::ios::binary|std::ios::ate );
   if ( infile.is_open() ){

      size = ( unsigned long )( infile.tellg() );
      infile.seekg( 0, std::ios::beg );
      std::cout << "Opened " << toread << " with size " << size << "." << std::endl;
      assert( size > 8UL );

      readframe = new char[ ZSEB_BUFFER ];
      rd_shift = 0;
      rd_end = 0;
      rd_current = 0;

      hash_last = new unsigned int[ ZSEB_HASH_SIZE ];
      hash_ptrs = new unsigned int[ ZSEB_BUFFER ];
      for ( unsigned int cnt = 0; cnt < ZSEB_HASH_SIZE; cnt++ ){ hash_last[ cnt ] = ZSEB_HASH_STOP; }
      for ( unsigned int cnt = 0; cnt < ZSEB_BUFFER;    cnt++ ){ hash_ptrs[ cnt ] = ZSEB_HASH_STOP; }

      __readin__();
      __write_infile_test__();

   } else {

      size = 0;
      std::cout << "Unable to open " << toread << "." << std::endl;
      readframe = NULL;
      rd_shift = 0;
      rd_end = 0;
      rd_current = 0;
      hash_last = NULL;
      hash_ptrs = NULL;

   }

}

zseb::zseb::~zseb(){

   if ( infile.is_open() ){ infile.close(); }
   if ( readframe != NULL ){ delete [] readframe; }
   if ( hash_ptrs != NULL ){ delete [] hash_ptrs; }
   if ( hash_last != NULL ){ delete [] hash_last; }

}

void zseb::zseb::__shift_left__(){

   assert( rd_current <  rd_end );
   assert( ZSEB_SHIFT <= rd_current );

   /* item = rd_current, pointer_to_keep
      ZSEB_SHIFT = 2^16 <= item < ZSEB_BUFFER = 2^17,
      hence: item - ZSEB_SHIFT = item ^ ZSEB_SHIFT ( XOR, bit toggle 2^16 ) */

   for ( unsigned int cnt = 0; cnt < ( rd_end - ZSEB_SHIFT ); cnt++ ){ // cnt < ZSEB_SHIFT
      readframe[ cnt ] = readframe[ cnt ^ ZSEB_SHIFT ]; // cnt + ZSEB_SHIFT
   }
   for ( unsigned int cnt = 0; cnt < ( rd_end - ZSEB_SHIFT ); cnt++ ){ // cnt < ZSEB_SHIFT
      unsigned int temp = hash_ptrs[ cnt ^ ZSEB_SHIFT ]; // cnt + ZSEB_SHIFT
      hash_ptrs[ cnt ] = ( ( ( temp == ZSEB_HASH_STOP ) || ( temp < ZSEB_SHIFT ) ) ? ( ZSEB_HASH_STOP ) : ( temp ^ ZSEB_SHIFT ) ); // temp - ZSEB_SHIFT
   }
   for ( unsigned int cnt = 0; cnt < ZSEB_HASH_SIZE; cnt++ ){
      unsigned int temp = hash_last[ cnt ];
      hash_last[ cnt ] = ( ( ( temp == ZSEB_HASH_STOP ) || ( temp < ZSEB_SHIFT ) ) ? ( ZSEB_HASH_STOP ) : ( temp ^ ZSEB_SHIFT ) ); // temp - ZSEB_SHIFT
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
   while ( ( pointer != ZSEB_HASH_STOP ) && ( pointer + ZSEB_HISTORY >= position ) ){ // >= because ( position - pointer - 1 ) stored?
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

void zseb::zseb::__write_infile_test__(){

   unsigned int longest_ptr0;
   unsigned int longest_len0 = 3; // No reuse of ( ptr1, len1 ) data initially
   unsigned int longest_ptr1;
   unsigned int longest_len1;
   unsigned int hash_entry;
   unsigned int next_entry;

   hash_entry =                                       ( unsigned char )( readframe[ rd_current     ] );
   hash_entry = ( hash_entry << ZSEB_NUM_CHAR_BIT ) | ( unsigned char )( readframe[ rd_current + 1 ] );
   hash_entry = ( hash_entry << ZSEB_NUM_CHAR_BIT ) | ( unsigned char )( readframe[ rd_current + 2 ] );

   while ( rd_current < rd_end - 3 ) {

      next_entry = ( ( unsigned char )( readframe[ rd_current + 3 ] ) ) | ( ( hash_entry << ZSEB_NUM_CHAR_BIT ) & ZSEB_HASH_MASK );
      if ( longest_len0 == 1 ){
         longest_ptr0 = longest_ptr1;
         longest_len0 = longest_len1;
      } else {
      __longest_match__( &longest_ptr0, &longest_len0, hash_entry, rd_current     ); }
      __longest_match__( &longest_ptr1, &longest_len1, next_entry, rd_current + 1 );

      if ( ( longest_ptr0 == ZSEB_HASH_STOP ) || ( longest_len1 > longest_len0 ) ){ // lazy evaluation
         //if ( longest_len0 > 1 ){ std::cout << "[Len " << longest_len0 << " --> " << longest_len1 << "]"; }
         std::cout << readframe[ rd_current ];
         longest_len0 = 1;
      } else {
         for ( unsigned int cnt = 0; cnt < longest_len0; cnt++ ){ std::cout << readframe[ longest_ptr0 + cnt ]; }
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

   while ( rd_current < rd_end ){
      std::cout << readframe[ rd_current ];
      rd_current += 1;
   }

}

