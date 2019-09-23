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

      llen_pack  = new zseb_08_t[ ZSEB_PACK_SIZE ];
      dist_pack  = new zseb_16_t[ ZSEB_PACK_SIZE ];
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

   /***  Variables  ***/
   zseb_32_t crc16 = 0;
   char var;
   char temp[ 4 ];

   /***  Fetch last modification time  ***/
   struct stat info;
   const zseb_32_t result = stat( bigfile.c_str(), &info );
   assert( result == 0 );
   const zseb_32_t MTIME = ( zseb_32_t )( info.st_mtime );
   std::cout << "MTIME = " << MTIME << std::endl;
   stream::int2str( MTIME, temp, 4 );

   /***  GZIP header  ***/
   /* ID1 */ var = ( zseb_08_t )( 0x1F ); zipfile->write( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 );
   /* ID2 */ var = ( zseb_08_t )( 0x8B ); zipfile->write( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 );
   /* CM  */ var = ( zseb_08_t )( 8 );    zipfile->write( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 );
   /* FLG */ var = ( zseb_08_t )( 10 );   zipfile->write( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 ); // (0, 0, 0, FCOMMENT=0, FNAME=1, FEXTRA=0, FHCRC=1, FTEXT=0 )
   /* MTIME */                            zipfile->write( temp, 4 ); crc16 = crc32::update( crc16, temp, 4 );
   /* XFL */ var = ( zseb_08_t )( 0 );    zipfile->write( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 );
   /* OS  */ var = ( zseb_08_t )( 255 );  zipfile->write( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 ); // Unknown Operating System

   // FLG.FEXTRA --> no
   // FLG.FNAME  --> yes

   /***  Original filename, terminated by a zero byte block  ***/
   std::size_t prev = std::string::npos;
   std::size_t curr = bigfile.find( '/', 0 );
   while ( curr != std::string::npos ){
      prev = curr;
      curr = bigfile.find( '/', prev + 1 );
   }
   std::string stripped = ( ( prev == std::string::npos ) ? bigfile : bigfile.substr( prev + 1, std::string::npos ) );
   const zseb_32_t length = ( zseb_32_t )( stripped.length() );
   const char * buffer = stripped.c_str();
   zipfile->write( buffer, length );
   crc16 = crc32::update( crc16, buffer, length );
   std::cout << "FNAME = [";
   for ( zseb_16_t cnt = 0; cnt < length; cnt++ ){
      std::cout << buffer[ cnt ];
   }
   std::cout << "]" << std::endl;
   /* ZER */ var = ( zseb_08_t )( 0 );    zipfile->write( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 );

   // FLG.FCOMMENT --> no
   // FLG.FHCRC    --> yes

   /***  Two least significant bytes of all bytes preceding the CRC16  ***/
   stream::int2str( crc16, temp, 2 );
   zipfile->write( temp, 2 );

}

std::string zseb::zseb::strip_preamble(){

   /***  Variables  ***/
   zseb_32_t crc16 = 0;
   char var;
   char temp[ 4 ];

   /***  GZIP header  ***/
   /* ID1 */ zipfile->read( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 ); if ( ( zseb_08_t )( var ) != 0x1F ){ std::cerr << "Incompatible ID1." << std::endl; exit( 255 ); }
   /* ID2 */ zipfile->read( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 ); if ( ( zseb_08_t )( var ) != 0x8B ){ std::cerr << "Incompatible ID2." << std::endl; exit( 255 ); }
   /* CM  */ zipfile->read( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 ); if ( ( zseb_08_t )( var ) != 8    ){ std::cerr << "Incompatible CM."  << std::endl; exit( 255 ); }
   /* FLG */ zipfile->read( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 ); const zseb_08_t FLG = ( zseb_08_t )( var );
   if ( ( ( FLG >> 7 ) & 1U ) == 1U ){ std::cerr << "Reserved bit is non-zero." << std::endl; exit( 255 ); }
   if ( ( ( FLG >> 6 ) & 1U ) == 1U ){ std::cerr << "Reserved bit is non-zero." << std::endl; exit( 255 ); }
   if ( ( ( FLG >> 5 ) & 1U ) == 1U ){ std::cerr << "Reserved bit is non-zero." << std::endl; exit( 255 ); }
   const bool FCOMMENT = ( ( ( FLG >> 4 ) & 1U ) == 1U );
   const bool FNAME    = ( ( ( FLG >> 3 ) & 1U ) == 1U );
   const bool FEXTRA   = ( ( ( FLG >> 2 ) & 1U ) == 1U );
   const bool FHCRC    = ( ( ( FLG >> 1 ) & 1U ) == 1U );
   const bool FTEXT    = ( (   FLG        & 1U ) == 1U );
   /* MTIME */ zipfile->read( temp, 4 ); crc16 = crc32::update( crc16, temp, 4 ); const zseb_32_t MTIME = stream::str2int( temp, 4 );
   /* XFL   */ zipfile->read( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 );
   /* OS    */ zipfile->read( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 );

   /***  Print MTIME  ***/
   std::cout << "MTIME = " << MTIME << std::endl;

   if ( FEXTRA ){
      zipfile->read( temp, 2 );
      crc16 = crc32::update( crc16, temp, 2 );
      const zseb_16_t XLEN = ( zseb_16_t )( stream::str2int( temp, 2 ) );
      for ( zseb_16_t cnt = 0; cnt < XLEN; cnt++ ){
         zipfile->read( &var, 1 );
         crc16 = crc32::update( crc16, &var, 1 );
      }
   }

   std::string filename = "";
   if ( FNAME ){
      bool proceed = true;
      while ( proceed ){
         zipfile->read( &var, 1 );
         crc16 = crc32::update( crc16, &var, 1 );
         proceed = ( ( zseb_08_t )( var ) != 0 );
         if ( proceed ){ filename += var; }
      }
      std::cout << "FNAME = [" << filename << "]" << std::endl;
   }

   if ( FCOMMENT ){
      bool proceed = true;
      while ( proceed ){
         zipfile->read( &var, 1 );
         crc16 = crc32::update( crc16, &var, 1 );
         proceed = ( ( zseb_08_t )( var ) != 0 );
      }
   }

   if ( FHCRC ){
      zipfile->read( temp, 2 );
      const zseb_32_t checksum = stream::str2int( temp, 2 );
      crc16 = crc16 & 0xFFFFU;
      if ( checksum != crc16 ){
         std::cerr << "Computed CRC16 = " << crc16 << " and read-in CRC16 = " << checksum << "." << std::endl;
         exit( 255 );
      }
   }

}

void zseb::zseb::zip( const bool debug_test ){

   zseb_64_t size_zlib = zipfile->getpos();

   zseb_32_t last_block = 0;
   zseb_32_t  cnt_block = 0;

   struct timeval start, end;
   double time_lzss = 0;
   double time_huff = 0;

   while ( last_block == 0 ){

      // LZSS a block
      gettimeofday( &start, NULL );
      last_block = flate->deflate( llen_pack, dist_pack, ZSEB_PACK_SIZE, wr_current );
      gettimeofday( &end, NULL );
      time_lzss += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

      // Compute dynamic Huffman trees & X01 and X10 sizes
      gettimeofday( &start, NULL );
      coder->calc_tree( llen_pack, dist_pack, wr_current );
      const zseb_32_t size_X1 = coder->get_size_X1();
      const zseb_32_t size_X2 = coder->get_size_X2();
      gettimeofday( &end, NULL );
      time_huff += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

      // What is the minimal output?
      const zseb_16_t block_form = ( ( debug_test ) ? ( cnt_block % 3 ) : ( ( size_X2 < size_X1 ) ? 2 : 1 ) );
      zipfile->write( last_block, 1 );
      zipfile->write( block_form, 2 );

      // Write out
      if ( block_form == 0 ){

         zipfile->flush();
         char vals[ 2 ];
         const zseb_16_t  LEN = ( zseb_16_t )( flate->get_LEN() );
         const zseb_16_t NLEN = ( ~LEN );
         stream::int2str(  LEN, vals, 2 ); zipfile->write( vals, 2 );
         stream::int2str( NLEN, vals, 2 ); zipfile->write( vals, 2 );
         gettimeofday( &start, NULL );
         flate->uncompressed( zipfile );
         gettimeofday( &end, NULL );
         time_lzss += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

      } else {

         gettimeofday( &start, NULL );
         if ( block_form == 1 ){ // No need to write trees
            coder->fixed_tree( 'O' );
         } else {
            coder->write_tree( zipfile );
         }
         coder->pack( zipfile, llen_pack, dist_pack, wr_current );
         gettimeofday( &end, NULL );
         time_huff += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

      }

      cnt_block += 1;
      wr_current = 0;

   }

   zipfile->flush();
   size_zlib = zipfile->getpos() - size_zlib; // Bytes

   const zseb_32_t chcksm    = flate->get_checksum();
   const zseb_64_t size_file = flate->get_file_bytes();
   const zseb_64_t size_lzss = flate->get_lzss_bits();

   char temp[ 4 ];
   // Write CRC32
   stream::int2str( chcksm, temp, 4 );
   zipfile->write( temp, 4 );
   // Write ISIZE = size_file mod 2^32
   const zseb_32_t ISIZE = ( zseb_32_t )( size_file & 0xFFFFFFFFU );
   stream::int2str( ISIZE, temp, 4 );
   zipfile->write( temp, 4 );

   delete zipfile; // So that file closes...
   zipfile = NULL;

   std::cout << "zseb: zip: comp(lzss)  = " << size_file / ( 0.125 * size_lzss ) << std::endl;
   std::cout << "           comp(total) = " << size_file / ( 1.0 * size_zlib ) << std::endl;
   std::cout << "           time(lzss)  = " << time_lzss << " seconds" << std::endl;
   std::cout << "           time(huff)  = " << time_huff << " seconds" << std::endl;

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
         char vals[ 2 ];
         zipfile->read( vals, 2 ); const zseb_16_t  LEN = ( zseb_16_t )( stream::str2int( vals, 2 ) );
         zipfile->read( vals, 2 ); const zseb_16_t NLEN = ( zseb_16_t )( stream::str2int( vals, 2 ) );
         const zseb_16_t NLEN2 = ( ~LEN );
         if ( NLEN != NLEN2 ){
            std::cerr << "Block type X00: NLEN != ( ~LEN )" << std::endl;
            exit( 255 );
         }
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
            remain = coder->unpack( zipfile, llen_pack, dist_pack, wr_current, ZSEB_PACK_SIZE );
            gettimeofday( &end, NULL );
            time_huff += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

            if ( wr_current > 0 ){
               gettimeofday( &start, NULL );
               flate->inflate( llen_pack, dist_pack, wr_current );
               gettimeofday( &end, NULL );
               time_lzss += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );
            }

            wr_current = 0;

         }
      }
   }

   flate->flush();
   zipfile->nextbyte();

   const zseb_32_t chcksm    = flate->get_checksum();
   const zseb_64_t size_file = flate->get_file_bytes(); // Set on flush

   char temp[ 4 ];
   // Read CRC32
   zipfile->read( temp, 4 );
   const zseb_32_t rd_chk = stream::str2int( temp, 4 );
   if ( chcksm != rd_chk ){
      std::cerr << "Computed CRC32 = " << chcksm << " and read-in CRC32 = " << rd_chk << "." << std::endl;
      exit( 255 );
   }
   // Read ISIZE
   zipfile->read( temp, 4 );
   const zseb_32_t rd_isz = stream::str2int( temp, 4 );
   const zseb_32_t ISIZE  = ( zseb_32_t )( size_file & 0xFFFFFFFFU );
   if ( ISIZE != rd_isz ){
      std::cerr << "Computed ISIZE = " << ISIZE << " and read-in ISIZE = " << rd_isz << "." << std::endl;
      exit( 255 );
   }

   std::cout << "zseb: unzip: time(lzss)  = " << time_lzss << " seconds" << std::endl;
   std::cout << "             time(huff)  = " << time_huff << " seconds" << std::endl;

}

