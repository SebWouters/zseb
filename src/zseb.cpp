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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "zseb.h"
#include "crc32.h"

zseb::zseb::zseb( std::string toread, std::string towrite, const char modus ){

   assert( sizeof( zseb_08_t ) == 1 );
   assert( sizeof( zseb_16_t ) == 2 );
   assert( sizeof( zseb_32_t ) == 4 );
   assert( sizeof( zseb_64_t ) == 8 );

   assert( ( modus == 'Z' ) || ( modus == 'U' ) );

   flate     = NULL;
   coder     = NULL;
   zipfile   = NULL;
   llen_pack = NULL;
   dist_pack = NULL;

   if ( modus == 'Z' ){

      flate   = new lzss( toread, modus );
      coder   = new huffman();
      zipfile = new stream( towrite, 'W' );

      llen_pack  = new zseb_08_t[ ZSEB_PACK_SIZE ];
      dist_pack  = new zseb_16_t[ ZSEB_PACK_SIZE ];
      wr_current = 0;

   }

   if ( modus == 'U' ){

      flate   = new lzss( towrite, modus );
      coder   = new huffman();
      zipfile = new stream( toread, 'R' );

      llen_pack  = new zseb_08_t[ ZSEB_UNPACK_SIZE ];
      dist_pack  = new zseb_16_t[ ZSEB_UNPACK_SIZE ];
      wr_current = 0;

   }

}

zseb::zseb::~zseb(){

   if ( flate != NULL ){ delete flate; }
   if ( coder != NULL ){ delete coder; }
   if ( zipfile != NULL ){ delete zipfile; }
   if ( llen_pack != NULL ){ delete [] llen_pack; }
   if ( dist_pack != NULL ){ delete [] dist_pack; }

}

void zseb::zseb::write_preamble( std::string bigfile ){

   /***  Fetch last modification time --> "time", 4 char  ***/
   struct stat info;
   const zseb_32_t result = stat( bigfile.c_str(), &info );
   assert( result == 0 );
   const zseb_32_t MTIME = ( zseb_32_t )( info.st_mtime );
   std::cout << "MTIME = " << MTIME << std::endl;
   char time[ 4 ];
   zseb_32_t temp = MTIME;
   for ( zseb_16_t cnt = 0; cnt < 4; cnt++ ){
      time[ cnt ] = ( zseb_08_t )( temp & 0xFF );
      temp = temp >> 8;
   }

   /***  CRC of header  ***/
   zseb_32_t crc16 = 0;

   /***  GZIP header  ***/
   char ID1 = ( zseb_08_t )( 0x1F ); zipfile->write( &ID1, 1 ); crc16 = crc32::update( crc16, &ID1, 1 );
   char ID2 = ( zseb_08_t )( 0x8B ); zipfile->write( &ID2, 1 ); crc16 = crc32::update( crc16, &ID2, 1 );
   char  CM = ( zseb_08_t )( 8 );    zipfile->write(  &CM, 1 ); crc16 = crc32::update( crc16,  &CM, 1 );
   char FLG = ( zseb_08_t )( 10 );   zipfile->write( &FLG, 1 ); crc16 = crc32::update( crc16, &FLG, 1 ); // (0, 0, 0, FCOMMENT=0, FNAME=1, FEXTRA=0, FHCRC=1, FTEXT=0 )
                                     zipfile->write( time, 4 ); crc16 = crc32::update( crc16, time, 4 );
   char XFL = ( zseb_08_t )( 2 );    zipfile->write( &XFL, 1 ); crc16 = crc32::update( crc16, &XFL, 1 ); // Compressor used maximum compression, slowest algorithm
   char  OS = ( zseb_08_t )( 255 );  zipfile->write(  &OS, 1 ); crc16 = crc32::update( crc16,  &OS, 1 ); // Unknown Operating System

   // Stuff if FLG.FEXTRA is set --> no
   // Stuff if FLG.FNAME  is set --> yes

   /***  Original filename, terminated by a zero byte block  ***/
   std::size_t prev = 0;
   std::size_t curr = bigfile.find( '/', prev );
   while ( curr != std::string::npos ){
      prev = curr;
      curr = bigfile.find( '/', prev + 1 );
   }
   std::string stripped = bigfile.substr( prev + 1, std::string::npos );
   const zseb_32_t length = ( zseb_32_t )( stripped.length() );
   const char * buffer = stripped.c_str();
   zipfile->write( buffer, length );
   crc16 = crc32::update( crc16, buffer, length );
   std::cout << "FNAME = [";
   for ( zseb_16_t cnt = 0; cnt < length; cnt++ ){
      std::cout << buffer[ cnt ];
   }
   std::cout << "]" << std::endl;
   char ZER = ( zseb_08_t )( 0 );
   zipfile->write( &ZER, 1 );
   crc16 = crc32::update( crc16, &ZER, 1 );

   // Stuff if FLG.FCOMMENT is set --> no
   // Stuff if FLG.FHCRC    is set --> yes

   /***  Two least significant bytes of all bytes preceding the CRC16  ***/
   char crc[ 2 ];
   crc[ 0 ] = ( zseb_08_t )( ( crc16      ) & 0xFF );
   crc[ 1 ] = ( zseb_08_t )( ( crc16 >> 8 ) & 0xFF );
   zipfile->write( crc, 2 );

}

std::string zseb::zseb::strip_preamble(){

   /***  CRC of header  ***/
   zseb_32_t crc16 = 0;

   /***  GZIP header  ***/
   char ID1; zipfile->read( &ID1, 1 ); crc16 = crc32::update( crc16, &ID1, 1 ); assert( ( zseb_08_t )( ID1 ) == 0x1F );
   char ID2; zipfile->read( &ID2, 1 ); crc16 = crc32::update( crc16, &ID2, 1 ); assert( ( zseb_08_t )( ID2 ) == 0x8B );
   char  CM; zipfile->read(  &CM, 1 ); crc16 = crc32::update( crc16,  &CM, 1 ); assert( ( zseb_08_t )(  CM ) == 8    );
   char FLG; zipfile->read( &FLG, 1 ); crc16 = crc32::update( crc16, &FLG, 1 );
   zseb_08_t tmp = ( zseb_08_t )( FLG );
   assert( ( tmp & ( 1U << 7 ) ) == 0 );
   assert( ( tmp & ( 1U << 6 ) ) == 0 );
   assert( ( tmp & ( 1U << 5 ) ) == 0 );
   const bool FCOMMENT = ( ( ( tmp >> 4 ) & 1U ) == 1U );
   const bool FNAME    = ( ( ( tmp >> 3 ) & 1U ) == 1U );
   const bool FEXTRA   = ( ( ( tmp >> 2 ) & 1U ) == 1U );
   const bool FHCRC    = ( ( ( tmp >> 1 ) & 1U ) == 1U );
   const bool FTEXT    = ( ( ( tmp >> 0 ) & 1U ) == 1U );
   char time[ 4 ]; zipfile->read( time, 4 ); crc16 = crc32::update( crc16, time, 4 );
   char XFL;       zipfile->read( &XFL, 1 ); crc16 = crc32::update( crc16, &XFL, 1 );
   char  OS;       zipfile->read(  &OS, 1 ); crc16 = crc32::update( crc16,  &OS, 1 );

   /***  Print MTIME  ***/
   zseb_32_t MTIME = 0;
   for ( zseb_16_t cnt = 0; cnt < 4; cnt++ ){
      MTIME = ( MTIME << 8 ) ^ ( ( zseb_08_t )( time[ 3 - cnt ] ) );
   }
   std::cout << "MTIME = " << MTIME << std::endl;

   if ( FEXTRA ){
      char input[ 2 ];
      zipfile->read( input, 2 );
      crc16 = crc32::update( crc16, input, 2 );
      zseb_16_t size = ( zseb_08_t )( input[ 1 ] );
      size = ( size << 8 ) ^ ( ( zseb_08_t )( input[ 0 ] ) );
      char * buffer = new char[ size ];
      zipfile->read( buffer, size );
      crc16 = crc32::update( crc16, buffer, size );
      delete [] buffer;
   }

   std::string filename = "";
   if ( FNAME ){
      char input;
      bool proceed = true;
      while ( proceed ){
         zipfile->read( &input, 1 );
         crc16 = crc32::update( crc16, &input, 1 );
         proceed = ( ( zseb_08_t )( input ) != 0 );
         if ( proceed ){ filename += input; }
      }
      std::cout << "Original filename = " << filename << std::endl;
   }

   if ( FCOMMENT ){
      char input;
      bool proceed = true;
      while ( proceed ){
         zipfile->read( &input, 1 );
         crc16 = crc32::update( crc16, &input, 1 );
         proceed = ( ( zseb_08_t )( input ) != 0 );
      }
   }

   if ( FHCRC ){
      char crc[ 2 ];
      zipfile->read( crc, 2 );
      zseb_16_t checksum = ( zseb_08_t )( crc[ 1 ] );
                checksum = ( checksum << 8 ) ^ ( ( zseb_08_t )( crc[ 0 ] ) );
      crc16 = crc16 & 0xFFFF;
      std::cout << "Computed CRC16 = " << crc16 << " and read-in CRC16 = " << checksum << "." << std::endl;
      assert( checksum == crc16 );
   }

}

void zseb::zseb::zip(){

   zseb_64_t size_zlib = zipfile->getpos();

   zseb_32_t last_block = 0;

   struct timeval start, end;
   double time_lzss = 0;
   double time_huff = 0;

   while ( last_block == 0 ){

      gettimeofday( &start, NULL );
      last_block = flate->deflate( llen_pack, dist_pack, ZSEB_PACK_TRIGGER, wr_current );
      gettimeofday( &end, NULL );
      time_lzss += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

      gettimeofday( &start, NULL );
      zipfile->write( last_block, 1 );
      zipfile->write( 2, 2 ); // Dynamic Huffman trees
      coder->calc_write_tree( zipfile, llen_pack, dist_pack, wr_current );
      coder->pack( zipfile, llen_pack, dist_pack, wr_current );
      gettimeofday( &end, NULL );
      time_huff += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

      wr_current = 0;

   }

   zipfile->flush();
   size_zlib = zipfile->getpos() - size_zlib; // Bytes

   const zseb_32_t chcksm    = flate->get_checksum();
   const zseb_64_t size_file = flate->get_file_bytes();
   const zseb_64_t size_lzss = flate->get_lzss_bits();

   char writeout[ 4 ];
   // Write CRC32
   zseb_32_t temp = chcksm;
   for ( zseb_16_t cnt = 0; cnt < 4; cnt++ ){
      writeout[ cnt ] = ( zseb_08_t )( temp & 0xFF );
      temp = temp >> 8;
   }
   zipfile->write( writeout, 4 );
   // Write ISIZE = size_file mod 2^32
   temp = ( zseb_32_t )( size_file & 0xFFFFFFFF );
   for ( zseb_16_t cnt = 0; cnt < 4; cnt++ ){
      writeout[ cnt ] = ( zseb_08_t )( temp & 0xFF );
      temp = temp >> 8;
   }
   zipfile->write( writeout, 4 );

   delete zipfile; // So that file closes...
   zipfile = NULL;

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

   zseb_32_t last_block = 0;
   zseb_32_t block_form;

   struct timeval start, end;
   double time_lzss = 0;
   double time_huff = 0;

   while ( last_block == 0 ){

      last_block = zipfile->read( 1 );
      block_form = zipfile->read( 2 ); // '10'_b dyn trees, '01'_b fixed trees, '00'_b uncompressed, '11'_b error
      assert( block_form < 3 );

      if ( block_form == 0 ){

         zipfile->nextbyte();
         char size_blk[ 4 ];
         zipfile->read( size_blk, 4 );
         zseb_16_t LEN = ( zseb_08_t )( size_blk[ 0 ] );
         LEN = ( LEN << 8 ) ^ ( ( zseb_08_t )( size_blk[ 1 ] ) );
         zseb_16_t NLEN = ( zseb_08_t )( size_blk[ 2 ] );
         NLEN = ( NLEN << 8 ) ^ ( ( zseb_08_t )( size_blk[ 3 ] ) );
         assert( LEN == ( ~ ( NLEN ) ) );
         flate->copy( zipfile, LEN );

      } else {

         gettimeofday( &start, NULL );
         if ( block_form == 2 ){ // Dynamic trees
            coder->load_tree( zipfile );
         } else { // block_form == 1 ---> Fixed trees
            coder->fixed_tree( 'I' );
         }
         gettimeofday( &end, NULL );
         time_huff += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

         zseb_32_t remain = 666;

         while ( remain > 0 ){

            gettimeofday( &start, NULL );
            remain = coder->unpack( zipfile, llen_pack, dist_pack, wr_current, ZSEB_UNPACK_SIZE );
            gettimeofday( &end, NULL );
            time_huff += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

            gettimeofday( &start, NULL );
            if ( wr_current > 0 ){
               flate->inflate( llen_pack, dist_pack, wr_current );
            }
            gettimeofday( &end, NULL );
            time_lzss += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

            wr_current = 0;

         }
      }
   }

   flate->flush();

   const zseb_32_t chcksm    = flate->get_checksum();
   const zseb_64_t size_file = flate->get_file_bytes(); // Set on flush

   char readin[ 4 ];

   // Read CRC32
   zipfile->read( readin, 4 );
   zseb_32_t temp = 0;
   for ( zseb_16_t cnt = 0; cnt < 4; cnt++ ){
      temp = ( temp << 8 ) ^ ( ( zseb_08_t )( readin[ 3 - cnt ] ) );
   }
   std::cout << "Computed CRC32 = " << chcksm << " and read-in CRC32 = " << temp << "." << std::endl;
   assert( chcksm == temp );

   // Read ISIZE
   zipfile->read( readin, 4 );
   temp = 0;
   for ( zseb_16_t cnt = 0; cnt < 4; cnt++ ){
      temp = ( temp << 8 ) ^ ( ( zseb_08_t )( readin[ 3 - cnt ] ) );
   }
   std::cout << "Computed ISIZE = " << ( size_file & 0xFFFFFFFF ) << " and read-in ISIZE = " << temp << "." << std::endl;
   assert( ( size_file & 0xFFFFFFFF ) == temp );

   std::cout << "zseb: unzip: LZSS  = " << time_lzss << "s." << std::endl;
   std::cout << "             Huff  = " << time_huff << "s." << std::endl;

}

