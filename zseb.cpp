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
//#include <math.h>

#include "zseb.h"

//int jans::big_int::NUM_BLOCK = 0;

//bool jans::big_int::nb_set = false;

zseb::zseb::zseb( std::string toread ){

   infile.open( toread.c_str(), std::ios::in|std::ios::binary|std::ios::ate );
   if ( infile.is_open() ){

      size = ( unsigned long )( infile.tellg() );
      infile.seekg( 0, std::ios::beg );
      std::cout << "Opened " << toread << " with size " << size << "." << std::endl;
      //std::cout << "ZSEB_HSH_STOP = " << ZSEB_HSH_STOP << std::endl;

      readframe = new char[ ZSEB_RD_SIZE ];
      rd_shift = 0;
      rd_end = 0;
      rd_current = 0;

      hash_last = new unsigned int[ ZSEB_HSH_SIZE ];
      hash_ptrs = new unsigned int[ ZSEB_RD_SIZE ];
      for ( unsigned long cnt = 0; cnt < ZSEB_HSH_SIZE; cnt++ ){ hash_last[ cnt ] = ZSEB_HSH_STOP; }
      for ( unsigned int  cnt = 0; cnt < ZSEB_RD_SIZE;  cnt++ ){ hash_ptrs[ cnt ] = ZSEB_HSH_STOP; }

      __readin__( 0 );
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

void zseb::zseb::__readin__( const unsigned int shift_left ){

   assert( rd_current <= rd_end );
   assert( shift_left <= rd_current );

   // Shift readframe down
   for ( unsigned int cnt = 0; cnt < ( rd_end - shift_left ); cnt++ ){
      readframe[ cnt ] = readframe[ cnt + shift_left ];
   }
   for ( unsigned int cnt = 0; cnt < ( rd_end - shift_left ); cnt++ ){
      hash_ptrs[ cnt ] = ( ( ( hash_ptrs[ cnt + shift_left ] == ZSEB_HSH_STOP ) || ( hash_ptrs[ cnt + shift_left ] < shift_left ) )
                       ? ( ZSEB_HSH_STOP ) : ( hash_ptrs[ cnt + shift_left ] - shift_left ) );
   }
   for ( unsigned long cnt = 0; cnt < ZSEB_HSH_SIZE; cnt++ ){
      hash_last[ cnt ] = ( ( ( hash_last[ cnt ] == ZSEB_HSH_STOP ) || ( hash_last[ cnt ] < shift_left ) )
                       ? ( ZSEB_HSH_STOP ) : ( hash_last[ cnt ] - shift_left ) );
   }
   rd_shift   += shift_left;
   rd_end     -= shift_left;
   rd_current -= shift_left;

   // Fill readframe[ rd_end : ZSEB_RD_SIZE ] if still possible, else less
   const unsigned int current_read = ( ( rd_shift + ZSEB_RD_SIZE > size ) ? ( size - rd_shift - rd_end ) : ( ZSEB_RD_SIZE - rd_end ) );
   infile.read( readframe + rd_end, current_read );
   rd_end += current_read;

}

void zseb::zseb::__write_infile_test__(){

   while ( rd_current < rd_end - 2 ) {

      //std::cout << "[readframe(" << rd_shift << ") = " << readframe[ rd_current ] << readframe[ rd_current + 1 ] << readframe[ rd_current + 2 ] << "]" << std::endl;

      //std::cout << readframe[ rd_current ];
      unsigned long entry = ( unsigned char )( readframe[ rd_current + 2 ] );
      entry = ( entry << ZSEB_NCHR_BIT ) + ( unsigned char )( readframe[ rd_current + 1 ] );
      entry = ( entry << ZSEB_NCHR_BIT ) + ( unsigned char )( readframe[ rd_current ] );

      unsigned int pointer = hash_last[ entry ];
      unsigned int longest_ptr = ZSEB_HSH_STOP;
      unsigned int longest_len = 1;
      while ( pointer != ZSEB_HSH_STOP ){
         unsigned int length = 3;
         bool match = true;
         while ( ( rd_current + length < rd_end ) && ( length < ZSEB_HEAD ) && ( match ) ){
            if ( readframe[ pointer + length ] == readframe[ rd_current + length ] ){
               length += 1;
            } else {
               match = false;
            }
         }
         if ( length > longest_len ){
            longest_len = length;
            longest_ptr = pointer;
         }
         //std::cout << "[readframe(" << pointer << ") = " << readframe[ pointer ] << readframe[ pointer + 1 ] << readframe[ pointer + 2 ] << " ]" << std::endl;
         pointer = hash_ptrs[ pointer ];
      }

      if ( longest_ptr != ZSEB_HSH_STOP ){
         //std::cout << "[" << rd_current - longest_ptr << ";" << longest_len << "]";
         for ( unsigned int cnt = 0; cnt < longest_len; cnt++ ){
            std::cout << readframe[ longest_ptr + cnt ];
         }
      } else {
         std::cout << readframe[ rd_current ];
      }

      hash_ptrs[ rd_current ] = hash_last[ entry ];
      hash_last[ entry ] = rd_current;
      rd_current += 1;

      unsigned int counter = 1;
      while ( counter < longest_len ){
         unsigned long temp = ( unsigned char )( readframe[ rd_current + 2 ] );
         entry = ( entry >> ZSEB_NCHR_BIT ) + ( temp << ( 2 * ZSEB_NCHR_BIT ) );
         hash_ptrs[ rd_current ] = hash_last[ entry ];
         hash_last[ entry ] = rd_current;
         rd_current += 1;
         counter += 1;
      }

      if ( ( rd_end == ZSEB_RD_SIZE ) && ( rd_current > ( ZSEB_HIST + ZSEB_READ_FWD ) ) ){
         __readin__( ZSEB_READ_FWD );
         //std::cout << "[shift_left " << ZSEB_READ_FWD << "]";
      }

   }

   while ( rd_current < rd_end ){
      std::cout << readframe[ rd_current ];
      rd_current += 1;
   }

}

