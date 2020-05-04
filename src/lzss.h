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

#pragma once

#include <iostream>
#include <fstream>
#include <string>

#include "dtypes.h"
#include "stream.h"

namespace zseb{

   class lzss{

      public:

         lzss( std::string fullfile, const zseb_modus modus );

         virtual ~lzss();

         // Following returns "Last block?"; on input wr_current is reset to zero; on output wr_current is #elem in *_pack; max(wr_current) <= size_pack + 4
         bool deflate2( uint8_t * llen_pack, uint16_t * dist_pack, const uint32_t size_pack, uint32_t &wr_current );
         bool deflate3( uint8_t * llen_pack, uint16_t * dist_pack, const uint32_t size_pack, uint32_t &wr_current );

         bool deflate(uint8_t * llen_pack, uint16_t * dist_pack, const uint32_t size_pack, uint32_t& wr_current);

         void inflate( uint8_t * llen_pack, uint16_t * dist_pack, const uint32_t size_pack );

         void flush();

         void copy( stream * zipfile, const uint16_t size_copy );

         uint64_t get_lzss_bits() const{ return size_lzss; }

         uint64_t get_file_bytes() const{ return size_file; }

         uint32_t get_checksum() const{ return checksum; }

      private:

         /***  INPUT FILE  ***/

         std::fstream file;

         uint64_t size_file; // Number of bytes in file

         uint64_t size_lzss; // Number of bits with pure LZSS ( 1-bit diff + 8-bit lit OR 1-bit diff + 8-bit len_shift + 15-bit dist_shift )

         /***  Workspace  ***/

         char * frame; // Length ZSEB_FRAME; snippet from file --> [ rd_shift : rd_shift + rd_end ]

         uint64_t rd_shift;   // Initial position of frame with respect to file

         uint32_t rd_end;     // Current validly filled length of frame [ 0 : ZSEB_FRAME ]

         uint32_t rd_current; // Current position within frame; ergo current position within file = rd_shift + rd_current

         /***  Hash table  ***/

         uint64_t * hash_head; // hash_head['abc'] = hash_head[ c + 256 * ( b + 256 * a ) ] = file idx of latest encounter

         //uint16_t * hash_prv3; // hash_prv3[ idx ] = idx' = frame idx of encounter before idx with same 3 chars (length ZSEB_HIST_SIZE)
         uint32_t * prev;

         /***  Functions  ***/

         void __readin__();

         //inline void __move_hash__( uint32_t &hash_entry ); // Update hash_prv3, hash_head, rd_current and hash_entry

         //inline void __append_lit_encode__( uint8_t * llen_pack, uint16_t * dist_pack, uint32_t &wr_current );

         //inline void __append_len_encode__( uint8_t * llen_pack, uint16_t * dist_pack, uint32_t &wr_current, const uint16_t dist_shift, const uint8_t len_shift );

         /***  CRC32 ***/

         uint32_t checksum;

   };

}

