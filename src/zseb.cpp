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
#include <sys/time.h>
#include <unistd.h>

#include "huffman.h"
#include "zseb.h"

zseb::zseb::zseb( std::string toread, std::string towrite, const char modus ){

   assert( sizeof( zseb_08_t ) == 1 );
   assert( sizeof( zseb_16_t ) == 2 );
   assert( sizeof( zseb_32_t ) == 4 );
   assert( sizeof( zseb_64_t ) == 8 );

   assert( ( modus == 'Z' ) || ( modus == 'U' ) );

   flate     = NULL;
   llen_pack = NULL;
   dist_pack = NULL;

   if ( modus == 'Z' ){

      flate = new lzss( toread, modus );

      zipfile.file.open( towrite.c_str(), std::ios::out|std::ios::binary|std::ios::trunc );
      zipfile.data = 0;
      zipfile.ibit = 0;

      llen_pack  = new zseb_08_t[ ZSEB_PACK_SIZE ];
      dist_pack  = new zseb_16_t[ ZSEB_PACK_SIZE ];
      wr_current = 0;

   }

   if ( modus == 'U' ){

      flate = new lzss( towrite, modus );

      zipfile.file.open( toread.c_str(), std::ios::in|std::ios::binary ); // At begin
      zipfile.data = 0;
      zipfile.ibit = 0;

      llen_pack  = new zseb_08_t[ ZSEB_UNPACK_SIZE ];
      dist_pack  = new zseb_16_t[ ZSEB_UNPACK_SIZE ];
      wr_current = 0;

      if ( zipfile.file.is_open() == false ){ std::cerr << "zseb::zseb: Unable to open " << toread << "." << std::endl; }

   }

}

zseb::zseb::~zseb(){

   if ( zipfile.file.is_open() ){ zipfile.file.close(); }
   if ( flate != NULL ){ delete flate; }
   if ( llen_pack != NULL ){ delete [] llen_pack; }
   if ( dist_pack != NULL ){ delete [] dist_pack; }

}

void zseb::zseb::zip(){

   // TODO: Write GZIP preamble

   zseb_64_t size_zlib = ( zseb_64_t )( zipfile.file.tellg() );

   bool last_block = false;

   struct timeval start, end;
   double time_lzss = 0;
   double time_huff = 0;

   while ( last_block == false ){

      gettimeofday( &start, NULL );
      last_block = flate->deflate( llen_pack, dist_pack, ZSEB_PACK_TRIGGER, wr_current );
      gettimeofday( &end, NULL );
      time_lzss += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

      gettimeofday( &start, NULL );
      huffman::pack( zipfile, llen_pack, dist_pack, wr_current, last_block );
      gettimeofday( &end, NULL );
      time_huff += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

      wr_current = 0;

   }

   huffman::flush( zipfile );
   size_zlib = ( zseb_64_t )( zipfile.file.tellg() ) - size_zlib; // Bytes

   // TODO: Write GZIP checksums

   const zseb_32_t chcksm = flate->get_checksum();
   std::cout << "CRC32 = " << chcksm << std::endl;
   zipfile.file.close();

   const zseb_64_t size_file = flate->get_file_bytes();
   const zseb_64_t size_lzss = flate->get_lzss_bits();

   const double red_lzss = 100.0 * ( 1.0   * size_file - 0.125 * size_lzss ) / size_file;
   const double red_huff = 100.0 * ( 0.125 * size_lzss -   1.0 * size_zlib ) / size_file;

   std::cout << "zseb: zip: LZSS  = " << red_lzss << "%." << std::endl;
   std::cout << "           Huff  = " << red_huff << "%." << std::endl;
   std::cout << "           Sum   = " << red_lzss + red_huff << "%." << std::endl;
   std::cout << "           Ratio = " << size_file / ( 1.0 * size_zlib ) << std::endl;
   std::cout << "zseb: zip: LZSS  = " << time_lzss << "s." << std::endl;
   std::cout << "           Huff  = " << time_huff << "s." << std::endl;

}

void zseb::zseb::unzip(){

   // TODO: Read GZIP preamble

   int last_block = 0;

   struct timeval start, end;
   double time_lzss = 0;
   double time_huff = 0;

   while ( last_block == 0 ){

      gettimeofday( &start, NULL );
      last_block = huffman::unpack( zipfile, llen_pack, dist_pack, wr_current, ZSEB_UNPACK_SIZE );
      assert( last_block < 2 ); // TODO: If last_block 2, then create longer llen_pack and dist_pack and copy previous wr_current data
      gettimeofday( &end, NULL );
      time_huff += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

      gettimeofday( &start, NULL );
      flate->inflate( llen_pack, dist_pack, wr_current );
      gettimeofday( &end, NULL );
      time_lzss += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

      wr_current = 0;

   }

   flate->flush();

   const zseb_32_t chcksm = flate->get_checksum();
   std::cout << "CRC32 = " << chcksm << std::endl;

   // TODO: Verify GZIP checksums

   std::cout << "zseb: unzip: LZSS  = " << time_lzss << "s." << std::endl;
   std::cout << "             Huff  = " << time_huff << "s." << std::endl;

}

