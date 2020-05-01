/*
   zseb: Zipping Sequences of Encountered Bytes
   Copyright (C) 2019, 2020 Sebastian Wouters

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
#include <time.h>
#include <utime.h>
#include <sys/stat.h>
#include <unistd.h>

#include "zseb.h"
#include "crc32.h"

zseb::zseb::zseb( std::string packfile, const zseb_modus modus, const bool verbose ){

   assert(modus != zseb_modus::undefined);

   print     = verbose;
   flate     = NULL;
   coder     = new huffman();
   zipfile   = new stream( packfile, ( ( modus == zseb_modus::zip ) ? 'W' : 'R' ) );
   llen_pack = new uint8_t[ ZSEB_PACK_SIZE ];
   dist_pack = new uint16_t[ ZSEB_PACK_SIZE ];
   wr_current = 0;
   mtime     = 0;

}

zseb::zseb::~zseb(){

   if ( flate != NULL ){ delete flate; }
   if ( coder != NULL ){ delete coder; }
   if ( zipfile != NULL ){ delete zipfile; }
   if ( llen_pack != NULL ){ delete [] llen_pack; }
   if ( dist_pack != NULL ){ delete [] dist_pack; }

}

void zseb::zseb::setup_flate( std::string bigfile, const zseb_modus modus ){

   flate = new lzss( bigfile, modus );

}

void zseb::zseb::set_time( std::string filename ){

   struct utimbuf overwrite;
   overwrite.actime  = time( NULL ); // Present
   overwrite.modtime = mtime;        // Modication time original file
   utime( filename.c_str(), &overwrite );

}

void zseb::zseb::write_preamble( std::string bigfile ){

   /***  Variables  ***/
   uint32_t crc16 = 0;
   char var;
   char temp[ 4 ];

   /***  Fetch last modification time  ***/
   struct stat info;
   if ( stat( bigfile.c_str(), &info ) != 0 ){
      std::cerr << "zseb: Unable to open " << bigfile << "." << std::endl;
      exit( 255 );
   }
   mtime = static_cast<uint32_t>( info.st_mtime );
   stream::int2str( mtime, temp, 4 );

   /***  GZIP header  ***/
   /* ID1 */ var = static_cast<uint8_t>( 0x1f ); zipfile->write( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 );
   /* ID2 */ var = static_cast<uint8_t>( 0x8b ); zipfile->write( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 );
   /* CM  */ var = static_cast<uint8_t>( 8 );    zipfile->write( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 );
   /* FLG */ var = static_cast<uint8_t>( 10 );   zipfile->write( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 ); // (0, 0, 0, FCOMMENT=0, FNAME=1, FEXTRA=0, FHCRC=1, FTEXT=0 )
   /* MTIME */                                   zipfile->write( temp, 4 ); crc16 = crc32::update( crc16, temp, 4 );
   /* XFL */ var = static_cast<uint8_t>( 0 );    zipfile->write( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 );
   /* OS  */ var = static_cast<uint8_t>( 255 );  zipfile->write( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 ); // Unknown Operating System

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
   const uint32_t length = static_cast<uint32_t>( stripped.length() );
   const char * buffer = stripped.c_str();
   zipfile->write( buffer, length );
   crc16 = crc32::update( crc16, buffer, length );
   /* ZER */ var = static_cast<uint8_t>( 0 );    zipfile->write( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 );

   // FLG.FCOMMENT --> no
   // FLG.FHCRC    --> yes

   /***  Two least significant bytes of all bytes preceding the CRC16  ***/
   stream::int2str( crc16, temp, 2 );
   zipfile->write( temp, 2 );

}

std::string zseb::zseb::strip_preamble(){

   /***  Variables  ***/
   uint32_t crc16 = 0;
   char var;
   char temp[ 4 ];

   /***  GZIP header  ***/
   /* ID1 */ zipfile->read( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 ); if ( static_cast<uint8_t>( var ) != 0x1f ){ std::cerr << "zseb: Incompatible ID1." << std::endl; exit( 255 ); }
   /* ID2 */ zipfile->read( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 ); if ( static_cast<uint8_t>( var ) != 0x8b ){ std::cerr << "zseb: Incompatible ID2." << std::endl; exit( 255 ); }
   /* CM  */ zipfile->read( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 ); if ( static_cast<uint8_t>( var ) != 8    ){ std::cerr << "zseb: Incompatible CM."  << std::endl; exit( 255 ); }
   /* FLG */ zipfile->read( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 ); const uint8_t FLG = static_cast<uint8_t>( var );
   if ( ( ( FLG >> 7 ) & 1U ) == 1U ){ std::cerr << "zseb: Reserved bit is non-zero." << std::endl; exit( 255 ); }
   if ( ( ( FLG >> 6 ) & 1U ) == 1U ){ std::cerr << "zseb: Reserved bit is non-zero." << std::endl; exit( 255 ); }
   if ( ( ( FLG >> 5 ) & 1U ) == 1U ){ std::cerr << "zseb: Reserved bit is non-zero." << std::endl; exit( 255 ); }
   const bool FCOMMENT = ( ( ( FLG >> 4 ) & 1U ) == 1U );
   const bool FNAME    = ( ( ( FLG >> 3 ) & 1U ) == 1U );
   const bool FEXTRA   = ( ( ( FLG >> 2 ) & 1U ) == 1U );
   const bool FHCRC    = ( ( ( FLG >> 1 ) & 1U ) == 1U );
   const bool FTEXT    = ( (   FLG        & 1U ) == 1U );
   /* MTIME */ zipfile->read( temp, 4 ); crc16 = crc32::update( crc16, temp, 4 ); mtime = stream::str2int( temp, 4 );
   /* XFL   */ zipfile->read( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 );
   /* OS    */ zipfile->read( &var, 1 ); crc16 = crc32::update( crc16, &var, 1 );

   if ( FEXTRA ){
      zipfile->read( temp, 2 );
      crc16 = crc32::update( crc16, temp, 2 );
      const uint16_t XLEN = static_cast<uint16_t>( stream::str2int( temp, 2 ) );
      for ( uint16_t cnt = 0; cnt < XLEN; cnt++ ){
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
         proceed = ( static_cast<uint8_t>( var ) != 0 );
         if ( proceed ){ filename += var; }
      }
   }

   if ( FCOMMENT ){
      bool proceed = true;
      while ( proceed ){
         zipfile->read( &var, 1 );
         crc16 = crc32::update( crc16, &var, 1 );
         proceed = ( static_cast<uint8_t>( var ) != 0 );
      }
   }

   if ( FHCRC ){
      zipfile->read( temp, 2 );
      const uint32_t checksum = stream::str2int( temp, 2 );
      crc16 = crc16 & ZSEB_MASK_16T;
      if ( checksum != crc16 ){
         std::cerr << "zseb: Computed CRC16 = " << crc16 << " if different from read-in CRC16 = " << checksum << "." << std::endl;
         exit( 255 );
      }
   }

   return filename;

}

void zseb::zseb::zip(){

   uint64_t size_zlib = zipfile->getpos(); // Preamble are full bytes

   uint32_t last_block = 0;
   uint32_t block_form;

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
      const uint32_t size_X1 = coder->get_size_X1();
      const uint32_t size_X2 = coder->get_size_X2();
      gettimeofday( &end, NULL );
      time_huff += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

      // What is the minimal output?
      block_form = ( ( size_X2 < size_X1 ) ? 2 : 1 );
      zipfile->write( last_block, 1 );
      zipfile->write( block_form, 2 );

      // Write out
      gettimeofday( &start, NULL );
      if ( block_form == 2 ){
         coder->write_tree( zipfile );
      } else {
         coder->fixed_tree( 'O' );
      }
      coder->pack( zipfile, llen_pack, dist_pack, wr_current );
      gettimeofday( &end, NULL );
      time_huff += ( end.tv_sec - start.tv_sec ) + 1e-6 * ( end.tv_usec - start.tv_usec );

      wr_current = 0;

   }

   zipfile->flush();
   size_zlib = zipfile->getpos() - size_zlib; // Bytes after flush

   const uint32_t chcksm    = flate->get_checksum();
   const uint64_t size_file = flate->get_file_bytes();
   const uint64_t size_lzss = flate->get_lzss_bits();

   char temp[ 4 ];
   // Write CRC32
   stream::int2str( chcksm, temp, 4 );
   zipfile->write( temp, 4 );
   // Write ISIZE = size_file mod 2^32
   const uint32_t ISIZE = static_cast<uint32_t>( size_file & ZSEB_MASK_32T );
   stream::int2str( ISIZE, temp, 4 );
   zipfile->write( temp, 4 );

   delete zipfile; // So that file closes...
   zipfile = NULL;

   delete flate; // So that file closes...
   flate = NULL;

   if ( print ){
      std::cout << "zseb: zip: comp(lzss)  = " << size_file / ( 0.125 * size_lzss ) << std::endl;
      std::cout << "           comp(total) = " << size_file / ( 1.0 * size_zlib ) << std::endl;
      std::cout << "           time(lzss)  = " << time_lzss << " seconds" << std::endl;
      std::cout << "           time(huff)  = " << time_huff << " seconds" << std::endl;
   }

}

void zseb::zseb::unzip(){

   uint64_t size_zlib = zipfile->getpos(); // Preamble are full Bytes

   uint32_t last_block = 0;
   uint32_t block_form;

   struct timeval start, end;
   double time_lzss = 0;
   double time_huff = 0;

   while ( last_block == 0 ){

      last_block = zipfile->read( 1 );
      block_form = zipfile->read( 2 ); // '10'_b dyn trees, '01'_b fixed trees, '00'_b uncompressed, '11'_b error
      if ( block_form == 3 ){
         std::cerr << "zseb: X11 is not a valid block mode." << std::endl;
         exit( 255 );
      }

      if ( block_form == 0 ){

         zipfile->nextbyte();
         char vals[ 2 ];
         zipfile->read( vals, 2 ); const uint16_t  LEN = static_cast<uint16_t>( stream::str2int( vals, 2 ) );
         zipfile->read( vals, 2 ); const uint16_t NLEN = static_cast<uint16_t>( stream::str2int( vals, 2 ) );
         const uint16_t NLEN2 = ( ~LEN );
         if ( NLEN != NLEN2 ){
            std::cerr << "zseb: Block type X00: NLEN != ( ~LEN )" << std::endl;
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

         bool remain = true;

         while ( remain ){

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
   size_zlib = zipfile->getpos() - size_zlib; // Bytes after nextbyte

   const uint32_t chcksm    = flate->get_checksum();
   const uint64_t size_file = flate->get_file_bytes(); // Set on flush
   const uint64_t size_lzss = flate->get_lzss_bits();

   char temp[ 4 ];
   // Read CRC32
   zipfile->read( temp, 4 );
   const uint32_t rd_chk = stream::str2int( temp, 4 );
   if ( chcksm != rd_chk ){
      std::cerr << "zseb: Computed CRC32 = " << chcksm << " is different from read-in CRC32 = " << rd_chk << "." << std::endl;
      exit( 255 );
   }
   // Read ISIZE
   zipfile->read( temp, 4 );
   const uint32_t rd_isz = stream::str2int( temp, 4 );
   const uint32_t ISIZE  = static_cast<uint32_t>( size_file & ZSEB_MASK_32T );
   if ( ISIZE != rd_isz ){
      std::cerr << "zseb: Computed ISIZE = " << ISIZE << " is different from read-in ISIZE = " << rd_isz << "." << std::endl;
      exit( 255 );
   }

   delete zipfile;
   zipfile = NULL;

   delete flate;
   flate = NULL;

   if ( print ){
      std::cout << "zseb: unzip: comp(lzss)  = " << size_file / ( 0.125 * size_lzss ) << std::endl;
      std::cout << "             comp(total) = " << size_file / ( 1.0 * size_zlib ) << std::endl;
      std::cout << "             time(lzss)  = " << time_lzss << " seconds" << std::endl;
      std::cout << "             time(huff)  = " << time_huff << " seconds" << std::endl;
   }

}

