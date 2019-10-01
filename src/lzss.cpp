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
#include "lzss.h"
#include "crc32.h"

zseb::lzss::lzss( std::string fullfile, const char modus ){

   assert( ( modus == 'Z' ) || ( modus == 'U' ) );

   assert( ZSEB_MMC_XVAL == 5 ); // Else insufficient repetitions in __longest_match__

   size_lzss = 0;
   size_file = 0;
   checksum  = 0;
   rd_shift  = 0;
   rd_end    = 0;
   rd_current = 0;
   frame     = NULL;
   hash_head = NULL;
   hash_prv3 = NULL;
   #ifndef ZSEB_GZIP_BEST
   hash_prvx = NULL;
   #endif

   if ( modus == 'Z' ){

      file.open( fullfile.c_str(), std::ios::in|std::ios::binary|std::ios::ate );

      if ( file.is_open() ){

         size_file = ( zseb_64_t )( file.tellg() );
         file.seekg( 0, std::ios::beg );

         frame = new char[ ZSEB_FRAME ];
         for ( zseb_32_t cnt = 0; cnt < ZSEB_FRAME; cnt++ ){ frame[ cnt ] = 0U; } // Because match check can go up to 4 beyond rd_end

         hash_head = new zseb_64_t[ ZSEB_HASH_SIZE ];
         hash_prv3 = new zseb_16_t[ ZSEB_HIST_SIZE ];
         #ifndef ZSEB_GZIP_BEST
         hash_prvx = new zseb_16_t[ ZSEB_HIST_SIZE ];
         #endif
         for ( zseb_32_t cnt = 0; cnt < ZSEB_HASH_SIZE; cnt++ ){ hash_head[ cnt ] = ZSEB_HASH_STOP; }
         for ( zseb_16_t cnt = 0; cnt < ZSEB_HIST_SIZE; cnt++ ){ hash_prv3[ cnt ] = ZSEB_HASH_STOP; }
         #ifndef ZSEB_GZIP_BEST
         for ( zseb_16_t cnt = 0; cnt < ZSEB_HIST_SIZE; cnt++ ){ hash_prvx[ cnt ] = ZSEB_HASH_STOP; }
         #endif

         __readin__(); // Get ready for work

      } else {

         std::cerr << "zseb: Unable to open " << fullfile << "." << std::endl;
         exit( 255 );

      }
   }

   if ( modus == 'U' ){

      file.open( fullfile.c_str(), std::ios::out|std::ios::binary|std::ios::trunc );

      frame = new char[ ZSEB_FRAME ];

   }

}

zseb::lzss::~lzss(){

   if ( file.is_open() ){ file.close(); }
   if ( frame     != NULL ){ delete [] frame;     }
   if ( hash_head != NULL ){ delete [] hash_head; }
   if ( hash_prv3 != NULL ){ delete [] hash_prv3; }
   #ifndef ZSEB_GZIP_BEST
   if ( hash_prvx != NULL ){ delete [] hash_prvx; }
   #endif

}

void zseb::lzss::flush(){

   file.write( frame, rd_current );
   checksum = crc32::update( checksum, frame, rd_current );
   rd_current = 0;
   size_file = ( zseb_64_t )( file.tellg() );

}

void zseb::lzss::copy( stream * zipfile, const zseb_16_t size_copy ){

   zseb_32_t nwrite = 0;
   if ( size_copy > ZSEB_HIST_SIZE ){
      nwrite = rd_current; // Write everything away, there will be still enough history
   } else { // ( ZSEB_HIST_SIZE - size_copy ) >= 0
      if ( rd_current > ( ZSEB_HIST_SIZE - size_copy ) ){
         nwrite = rd_current - ( ZSEB_HIST_SIZE - size_copy ); // rd_current + size_copy - nwrite = ZSEB_HIST_SIZE
      }
      // If rd_current + size_copy <= ZSEB_HIST_SIZE, no need to write
   }

   if ( nwrite > 0 ){

      assert( rd_current >= nwrite );

      file.write( frame, nwrite );
      checksum = crc32::update( checksum, frame, nwrite );

      for ( zseb_32_t cnt = 0; cnt < ( rd_current - nwrite ); cnt++ ){
         frame[ cnt ] = frame[ nwrite + cnt ];
      }

      rd_current -= nwrite;

   }

   zipfile->read( frame + rd_current, size_copy );
   rd_current += size_copy;

}

void zseb::lzss::inflate( zseb_08_t * llen_pack, zseb_16_t * dist_pack, const zseb_32_t size_pack ){

   for ( zseb_32_t idx = 0; idx < size_pack; idx++ ){

      if ( dist_pack[ idx ] == ZSEB_MASK_16T ){ // write LIT

         size_lzss += ( ZSEB_CHARBIT + 1 ); // 8-bit literal [ 0 : 255 ] + 1-bit differentiator

         frame[ rd_current ] = ( char )( llen_pack[ idx ] );
         rd_current += 1;

      } else { // copy ( LEN, DIST )

         size_lzss += ( ZSEB_HIST_BIT + ZSEB_CHARBIT + 1 ); // 15-bit dist_shift [ 0 : 32767 ] + 8-bit len_shift [ 0 : 255 ] + 1-bit differentiator

         const zseb_16_t distance =                  dist_pack[ idx ]     + ( ( zseb_16_t )( ZSEB_DIST_SHIFT   ) );
         const zseb_16_t length   = ( ( zseb_16_t )( llen_pack[ idx ] ) ) + ( ( zseb_16_t )( ZSEB_LENGTH_SHIFT ) );

         for ( zseb_16_t cnt = 0; cnt < length; cnt++ ){
            frame[ rd_current + cnt ] = frame[ rd_current - distance + cnt ];
         }
         rd_current += length;

      }

      if ( rd_current >= ZSEB_TRIGGER ){

         file.write( frame, ZSEB_HIST_SIZE );
         checksum = crc32::update( checksum, frame, ZSEB_HIST_SIZE );
         for ( zseb_32_t cnt = 0; cnt < ( rd_current - ZSEB_HIST_SIZE ); cnt++ ){
            frame[ cnt ] = frame[ ZSEB_HIST_SIZE + cnt ];
         }
         rd_current -= ZSEB_HIST_SIZE;

      }

   }

}

zseb_32_t zseb::lzss::deflate( zseb_08_t * llen_pack, zseb_16_t * dist_pack, const zseb_32_t size_pack, zseb_32_t &wr_current ){

   zseb_16_t longest_ptr0;
   zseb_16_t longest_len0 = 3; // No reuse of ( ptr1, len1 ) data initially
   zseb_16_t longest_ptr1;
   zseb_16_t longest_len1;
   zseb_32_t hash_entry = 0;

   if ( ( rd_current + ZSEB_LENGTH_SHIFT ) <= rd_end ){
      hash_entry =                                  ( zseb_08_t )( frame[ rd_current     ] );
      hash_entry = ( hash_entry << ZSEB_CHARBIT ) | ( zseb_08_t )( frame[ rd_current + 1 ] );
      hash_entry = ( hash_entry << ZSEB_CHARBIT ) | ( zseb_08_t )( frame[ rd_current + 2 ] );
   }

   // Obtain an upper_limit for rd_current, so that the current processed block does not exceed 32K
   const zseb_32_t limit1 = ( ( ( rd_shift + rd_current ) == 0 ) ? ZSEB_HIST_SIZE : ZSEB_TRIGGER );
   const zseb_32_t limit2 = rd_end;
   const zseb_32_t upper_limit = ( ( limit2 < limit1 ) ? limit2 : limit1 );

   while ( rd_current < upper_limit ){ // rd_current can be at the absolute most 65535 --> hence any ptr should fit in zseb_16_t

      if ( longest_len0 == 1 ){
         longest_ptr0 = longest_ptr1;
         longest_len0 = longest_len1;
      } else {
         longest_ptr0 = ( ( hash_head[ hash_entry ] > rd_shift ) ? ( zseb_16_t )( hash_head[ hash_entry ] - rd_shift ) : 0 );
         __longest_match__( frame + rd_current, longest_ptr0, longest_len0, longest_ptr0, rd_current, ( zseb_16_t )( upper_limit - rd_current ), hash_prv3,
            #ifdef ZSEB_GZIP_BEST
            ZSEB_MAX_CHAIN
            #else
            hash_prvx
            #endif
            );
      }

      __move_hash__( hash_entry ); // Update of hash_prv3, hash_head, rd_current, hash_entry
      { // Lazy evaluation candidate
         longest_ptr1 = ( ( hash_head[ hash_entry ] > rd_shift ) ? ( zseb_16_t )( hash_head[ hash_entry ] - rd_shift ) : 0 );
         __longest_match__( frame + rd_current, longest_ptr1, longest_len1, longest_ptr1, rd_current, ( zseb_16_t )( upper_limit - rd_current ), hash_prv3,
            #ifdef ZSEB_GZIP_BEST
            ( ( longest_len0 >= ZSEB_GOOD_MATCH ) ? ( ZSEB_MAX_CHAIN >> 2 ) : ZSEB_MAX_CHAIN )
            #else
            hash_prvx
            #endif
            );
      }

      if ( ( longest_ptr0 == ZSEB_HASH_STOP ) || ( longest_len1 > longest_len0 ) ){ // lazy evaluation
         __append_lit_encode__( llen_pack, dist_pack, wr_current );
         longest_len0 = 1;
      } else {
         const zseb_16_t dist_shft = ( zseb_16_t )( rd_current - ( 1 + longest_ptr0 + ZSEB_DIST_SHIFT ) );
         const zseb_08_t  len_shft = ( zseb_08_t )( longest_len0 - ZSEB_LENGTH_SHIFT );
         __append_len_encode__( llen_pack, dist_pack, wr_current, dist_shft, len_shft );
      }

      // hash_prvx[ pos & ZSEB_HIST_MASK ] and hash_prvx[ ( pos + 1 ) & ZSEB_HIST_MASK ] updated via __longest_match__, with pos = rd_current
      #ifndef ZSEB_GZIP_BEST
      for ( zseb_16_t cnt = 1; cnt < ( longest_len0 - 1 ); cnt++ ){ hash_prvx[ ( rd_current + cnt ) & ZSEB_HIST_MASK ] = ZSEB_HASH_STOP; }
      #endif
      for ( zseb_16_t cnt = 1; cnt < longest_len0; cnt++ ){ __move_hash__( hash_entry ); }

   }

   if ( rd_current >= ZSEB_TRIGGER ){

      for ( zseb_32_t cnt = 0; cnt < ( rd_end - ZSEB_HIST_SIZE ); cnt++ ){
         frame[ cnt ] = frame[ ZSEB_HIST_SIZE + cnt ];
      }
      rd_shift   += ZSEB_HIST_SIZE;
      rd_end     -= ZSEB_HIST_SIZE;
      rd_current -= ZSEB_HIST_SIZE;

      __readin__();

      for ( zseb_16_t cnt = 0; cnt < ZSEB_HIST_SIZE; cnt++ ){ hash_prv3[ cnt ] = ( ( hash_prv3[ cnt ] > ZSEB_HIST_SIZE ) ? ( hash_prv3[ cnt ] ^ ZSEB_HIST_SIZE ) : 0 ); }
      #ifndef ZSEB_GZIP_BEST
      for ( zseb_16_t cnt = 0; cnt < ZSEB_HIST_SIZE; cnt++ ){ hash_prvx[ cnt ] = ( ( hash_prvx[ cnt ] > ZSEB_HIST_SIZE ) ? ( hash_prvx[ cnt ] ^ ZSEB_HIST_SIZE ) : 0 ); }
      #endif
      /* Faster with rd_shift shifts
      for ( zseb_32_t cnt = 0; cnt < ZSEB_HASH_SIZE; cnt++ ){ hash_head[ cnt ] = ( ( hash_head[ cnt ] > ZSEB_HIST_SIZE ) ? ( hash_head[ cnt ] - ZSEB_HIST_SIZE ) : 0 ); } */

   }

   if ( rd_end > rd_current ){ // Not yet processed until rd_end

      assert( wr_current <= size_pack );
      return 0; // Not yet last block

   }

   return 1; // Last block

}

void zseb::lzss::__readin__(){

   const zseb_32_t current_read = ( ( rd_shift + ZSEB_FRAME > size_file ) ? ( size_file - rd_shift - rd_end ) : ( ZSEB_FRAME - rd_end ) );
   file.read( frame + rd_end, current_read );
   checksum = crc32::update( checksum, frame + rd_end, current_read );
   rd_end += current_read;

}

void zseb::lzss::__move_hash__( zseb_32_t &hash_entry ){

   hash_prv3[ rd_current & ZSEB_HIST_MASK ] = ( ( hash_head[ hash_entry ] > rd_shift ) ? ( zseb_16_t )( hash_head[ hash_entry ] - rd_shift ) : 0 );
   hash_head[ hash_entry ] = rd_shift + rd_current; // rd_current always < 2**16 upon set
   rd_current += 1;
   hash_entry = ( ( zseb_08_t )( frame[ rd_current + 2 ] ) ) | ( ( hash_entry << ZSEB_CHARBIT ) & ZSEB_HASH_MASK );

}

void zseb::lzss::__longest_match__( char * present, zseb_16_t &result_ptr, zseb_16_t &result_len, zseb_16_t ptr, const zseb_32_t curr, const zseb_16_t runway, zseb_16_t * prev3,
   #ifdef ZSEB_GZIP_BEST
   zseb_16_t chain_length
   #else
   zseb_16_t * prevx
   #endif
){

   result_ptr = ZSEB_HASH_STOP;
   result_len = 1;

   #ifndef ZSEB_GZIP_BEST
   zseb_16_t formx = ( zseb_16_t )( curr & ZSEB_HIST_MASK );
   prevx[ formx ] = ZSEB_HASH_STOP; // Update accounted for, also if immediate return
   #endif

   const zseb_16_t max_len = ( ( ZSEB_LENGTH_MAX > runway ) ? runway : ZSEB_LENGTH_MAX );
   if ( max_len < ZSEB_LENGTH_SHIFT ){ return; }

   char * start  = present + 2;
   char * cutoff = present + max_len;

   const zseb_16_t ptr_lim = ( ( curr > ZSEB_HIST_SIZE ) ? ( zseb_16_t )( curr - ZSEB_HIST_SIZE ) : 0 ); // curr < stop <= 64K because max_len >= 3

   zseb_16_t * chain = prev3;

   while ( ( ptr > ptr_lim )
      #ifdef ZSEB_GZIP_BEST
      && ( chain_length-- != 0 )
      #endif
      ){

      char * current = start;
      char * history = ( start + ptr ) - curr;

      do {} while( ( *(++history) == *(++current) ) && ( *(++history) == *(++current) ) &&
                   ( *(++history) == *(++current) ) && ( *(++history) == *(++current) ) &&
                   ( *(++history) == *(++current) ) && ( *(++history) == *(++current) ) &&
                   ( *(++history) == *(++current) ) && ( *(++history) == *(++current) ) &&
                   ( current < cutoff ) ); // ZSEB_FRAME is a full 1024 larger than ZSEB_TRIGGER

      const zseb_16_t length = ( zseb_16_t )( ( ( current >= cutoff ) ? cutoff : current ) - present );

      #ifndef ZSEB_GZIP_BEST
      if ( chain == prev3 ){
         if ( length >= ZSEB_MMC_XVAL ){
            prevx[ formx ] = ptr;
            formx = ( ptr & ZSEB_HIST_MASK );
            if ( prevx[ formx ] > ZSEB_HASH_STOP ){ // Trace on prevx is picked up: switch chain & quit updating
               start += 1;
               chain = prevx;
            }
         }
      }
      #endif

      if ( length > result_len ){
         result_len = length;
         result_ptr = ptr;

         if ( result_len >= max_len ){

            /*
            prevx meets max_len for one string, before properly connecting.
            For another string, with different suffix / max_len, the search on prevx then prematurely ends.
            The following assumes proper connection.
            */

            #ifndef ZSEB_GZIP_BEST
            if ( true ){

               ptr = chain[ ptr & ZSEB_HIST_MASK ];

               while ( ( chain == prev3 ) && ( ptr > ptr_lim ) ){

                  current = start;
                  history = ( start + ptr ) - curr;

                  #if ZSEB_MMC_XVAL == 5
                  if ( ( *(++history) == *(++current) ) && ( *(++history) == *(++current) ) ) // ( ZSEB_MMC_XVAL - 3 ) times
                  #else
                  #error "__longest_match__: adjust number of checks"
                  #endif
                  {

                     prevx[ formx ] = ptr;
                     formx = ( ptr & ZSEB_HIST_MASK );
                     if ( prevx[ formx ] > ZSEB_HASH_STOP ){ chain = prevx; }

                  }

                  ptr = chain[ ptr & ZSEB_HIST_MASK ]; 

               }
            }
            #endif

            break;

         }
      }

      ptr = chain[ ptr & ZSEB_HIST_MASK ];

   }

}

void zseb::lzss::__append_lit_encode__( zseb_08_t * llen_pack, zseb_16_t * dist_pack, zseb_32_t &wr_current ){

   size_lzss += ( ZSEB_CHARBIT + 1 ); // 8-bit literal [ 0 : 255 ] + 1-bit differentiator

   llen_pack[ wr_current ] = ( zseb_08_t )( frame[ rd_current - 1 ] ); // [ 0 : 255 ]
   dist_pack[ wr_current ] = ZSEB_MASK_16T;

   wr_current += 1;

}

void zseb::lzss::__append_len_encode__( zseb_08_t * llen_pack, zseb_16_t * dist_pack, zseb_32_t &wr_current, const zseb_16_t dist_shift, const zseb_08_t len_shift ){

   size_lzss += ( ZSEB_HIST_BIT + ZSEB_CHARBIT + 1 ); // 15-bit dist_shift [ 0 : 32767 ] + 8-bit len_shift [ 0 : 255 ] + 1-bit differentiator

   llen_pack[ wr_current ] = len_shift;  // [ 0 : 255 ]
   dist_pack[ wr_current ] = dist_shift; // [ 0 : 32767 ]

   wr_current += 1;

}

