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

#ifndef __ZSEB_LZSS__
#define __ZSEB_LZSS__

#include <iostream>
#include <fstream>
#include <string>

#include "dtypes.h"
#include "stream.h"

     // ZSEB_HIST_SIZE    32768U                          // 2^15 ( data format, defined in dtypes )
#define ZSEB_HIST_MASK    ( ZSEB_HIST_SIZE - 1U )
#define ZSEB_TRIGGER      ( ZSEB_HIST_SIZE << 1 )         // If ( rd_current >= ZSEB_TRIGGER ) --> shift
#define ZSEB_FRAME        ( ZSEB_TRIGGER + 1024U )        // Important that 1024 > max( length ) = 258 !!!

#define ZSEB_HASH_SIZE    16777216U  // ZSEB_LITLEN^3 = 2^24
#define ZSEB_HASH_MASK    ( ZSEB_HASH_SIZE - 1U )
#define ZSEB_HASH_STOP    0U

#define ZSEB_GZIP_BEST
#define ZSEB_MAX_CHAIN    4096
#define ZSEB_GOOD_MATCH   32

namespace zseb{

   class lzss{

      public:

         lzss( std::string fullfile, const char unzip );

         virtual ~lzss();

         // Following returns "Last block?"; on input wr_current is reset to zero; on output wr_current is #elem in *_pack; max(wr_current) <= size_pack + 4
         zseb_32_t deflate( zseb_08_t * llen_pack, zseb_16_t * dist_pack, const zseb_32_t size_pack, zseb_32_t &wr_current );

         void inflate( zseb_08_t * llen_pack, zseb_16_t * dist_pack, const zseb_32_t size_pack );

         void flush();

         void copy( stream * zipfile, const zseb_16_t size_copy );

         zseb_64_t get_lzss_bits() const{ return size_lzss; }

         zseb_64_t get_file_bytes() const{ return size_file; }

         zseb_32_t get_checksum() const{ return checksum; }

      private:

         /***  INPUT FILE  ***/

         std::fstream file;

         zseb_64_t size_file; // Number of bytes in file

         zseb_64_t size_lzss; // Number of bits with pure LZSS ( 1-bit diff + 8-bit lit OR 1-bit diff + 8-bit len_shift + 15-bit dist_shift )

         /***  Workspace  ***/

         char * frame; // Length ZSEB_FRAME; snippet from file --> [ rd_shift : rd_shift + rd_end ]

         zseb_64_t rd_shift;   // Initial position of frame with respect to file

         zseb_32_t rd_end;     // Current validly filled length of frame [ 0 : ZSEB_FRAME ]

         zseb_32_t rd_current; // Current position within frame; ergo current position within file = rd_shift + rd_current

         /***  Hash table  ***/

         zseb_64_t * hash_head; // hash_head['abc'] = hash_head[ c + 256 * ( b + 256 * a ) ] = file idx of latest encounter

         zseb_32_t * hash_prv3; // hash_prv3[ idx ] = idx' = frame idx of encounter before idx with same 3 chars ( length ZSEB_HIST_SIZE )

         zseb_32_t * hash_prv4; // hash_prv4[ idx ] = idx' = frame idx of encounter before idx with same 4 chars ( length ZSEB_HIST_SIZE )

         /***  Functions  ***/

         void __readin__();

#ifndef ZSEB_GZIP_BEST
         void __longest_match__( zseb_32_t &result_ptr, zseb_16_t &result_len, zseb_32_t ptr, const zseb_32_t curr );
#else
         void __longest_match__( zseb_32_t &result_ptr, zseb_16_t &result_len, zseb_32_t ptr, const zseb_32_t curr, zseb_16_t chain_length );
#endif

         inline void __move_hash__( const zseb_32_t hash_entry ); // Add hash_entry & rd_current to hash; increment rd_current

         inline void __append_lit_encode__( zseb_08_t * llen_pack, zseb_16_t * dist_pack, zseb_32_t &wr_current );

         inline void __append_len_encode__( zseb_08_t * llen_pack, zseb_16_t * dist_pack, zseb_32_t &wr_current, const zseb_16_t dist_shift, const zseb_08_t len_shift );

         /***  CRC32 ***/

         zseb_32_t checksum;

   };

}

#endif
