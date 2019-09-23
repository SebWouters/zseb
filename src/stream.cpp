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
#include "stream.h"

zseb::stream::stream( std::string filename, const char modus ){

   assert( ( modus == 'R' ) || ( modus == 'W' ) );

   data = 0;
   ibit = 0;
   size = 0;
   mode = modus;

   if ( mode == 'W' ){

      file.open( filename.c_str(), std::ios::out|std::ios::binary|std::ios::trunc );

   }

   if ( mode == 'R' ){

      file.open( filename.c_str(), std::ios::in|std::ios::binary|std::ios::ate );

      if ( file.is_open() ){

         size = ( zseb_64_t )( file.tellg() );
         file.seekg( 0, std::ios::beg );

      } else {

         std::cerr << "zseb::stream: Unable to open " << filename << "." << std::endl;

      }

   }

}

zseb::stream::~stream(){

   if ( mode == 'W' ){ if ( ibit > 0 ){ flush(); } }
   if ( file.is_open() ){ file.close(); }

}

zseb_64_t zseb::stream::getsize() const{

   assert( mode == 'R' );
   return size;

}

zseb_64_t zseb::stream::getpos(){

   const zseb_64_t pos = ( zseb_64_t )( file.tellg() );
   return pos;

}

void zseb::stream::write( const zseb_32_t flush, const zseb_16_t nbits ){

   /* Data ini : [ __ __ __ P5 P4 P3 P2 P1 ], ibit = 5
      Flush    = [ __ __ N6 N5 N4 N3 N2 N1 ], nbits = 6
      Data mid : [ __ __ __ __ __ N6 N5 N4 ][ N3 N2 N1 P5 P4 P3 P2 P1 ], ibit = 11
      Stream <<  [ N3 N2 N1 P5 P4 P3 P2 P1 ]
      Data end : [ __ __ __ __ __ N6 N5 N4 ], ibit = 3
   */

   // Regular data = regular order: LSB of flush nearest to LSB of data
   // Huffman codes are read one bit at a time: reverse code upon creation of Huffman tree with modus 'O'utput

   data = data ^ ( flush << ibit ); // data mid
   ibit = ibit + nbits;

   while ( ibit >= 8 ){
      const char towrite = ( zseb_08_t )( data & 0xFFU ); // Mask last 8 bits
      file.write( &towrite, 1 );
      data = ( data >> 8 );
      ibit = ibit - 8;
   }

}

void zseb::stream::write( const char * buffer, const zseb_32_t size_out ){

   assert( ibit == 0 );

   file.write( buffer, size_out );

}

void zseb::stream::flush(){

   while ( ibit > 0 ){
      const char towrite = ( zseb_08_t )( data & 0xFFU ); // Mask last 8 bits
      file.write( &towrite, 1 );
      data  = ( data >> 8 );
      ibit  = ( ( ibit > 8 ) ? ( ibit - 8 ) : 0 );
   }

}

zseb_32_t zseb::stream::read( const zseb_16_t nbits ){

   /* Cfr. write example: P has been retrieved and removed
      Data ini : [ __ __ __ __ __ N3 N2 N1 ], ibit = 3
      nbits = 6 > ibit = 3
      Stream >>  [ XX XX XX XX XX N6 N5 N4 ]
      Data mid : [ __ __ __ __ __ XX XX XX ][ XX XX N6 N5 N4 N3 N2 N1 ], ibit = 11
      Fetch    = [ __ __ N6 N5 N4 N3 N2 N1 ], nbits = 6
      Data end : [ __ __ __ XX XX XX XX XX ], ibit = 5
   */

   while ( ibit < nbits ){
      char toread;
      file.read( &toread, 1 );
      const zseb_32_t toshift = ( zseb_08_t )( toread );
      data = data ^ ( toshift << ibit );
      ibit = ibit + 8;
   }

   const zseb_32_t fetch = ( data & ( ( 1U << nbits ) - 1 ) ); // mask last nbits bits
   data = ( data >> nbits );
   ibit = ibit - nbits;

   return fetch;

}

void zseb::stream::nextbyte(){

   // For block mode X00, you need to move to next byte for read
   if ( ibit > 0 ){
      zseb_16_t temp = read( ibit );
   }

}

void zseb::stream::read( char * buffer, const zseb_32_t size_in ){

   assert( ibit == 0 );

   file.read( buffer, size_in );

}

zseb_32_t zseb::stream::str2int( const char * store, const zseb_16_t num ){

   zseb_32_t value = 0;
   for ( zseb_16_t cnt = 0; cnt < num; cnt++ ){
      value = ( value << 8 ) ^ ( ( zseb_08_t )( store[ num - 1 - cnt ] ) );
   }
   return value;

}

void zseb::stream::int2str( const zseb_32_t value, char * store, const zseb_16_t num ){

   zseb_32_t temp = value;
   for ( zseb_16_t cnt = 0; cnt < num; cnt++ ){
      store[ cnt ] = ( zseb_08_t )( temp & 0xFFU );
      temp = temp >> 8;
   }

}


