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

   while ( last_block == false ){

      last_block = flate->deflate( llen_pack, dist_pack, ZSEB_PACK_TRIGGER, wr_current );
      huffman::pack( zipfile, llen_pack, dist_pack, wr_current, last_block );
      wr_current = 0;

   }

   size_zlib = ( zseb_64_t )( zipfile.file.tellg() ) - size_zlib; // Bytes

   // TODO: Write GZIP checksums

   const zseb_64_t size_file = flate->get_file_bytes();
   const zseb_64_t size_lzss = flate->get_lzss_bits();

   const double red_lzss = 100.0 * ( 1.0   * size_file - 0.125 * size_lzss ) / size_file;
   const double red_huff = 100.0 * ( 0.125 * size_lzss -   1.0 * size_zlib ) / size_file;

   std::cout << "zseb: reduction: LZSS  = " << red_lzss << "%." << std::endl;
   std::cout << "                 Huff  = " << red_huff << "%." << std::endl;
   std::cout << "                 Sum   = " << red_lzss + red_huff << "%." << std::endl;
   std::cout << "                 Ratio = " << size_file / ( 1.0 * size_zlib ) << std::endl;

}

