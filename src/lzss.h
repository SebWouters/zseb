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

     // ZSEB_HISTORY      32768U     // 2^15 ( data format, defined in dtypes )
#define ZSEB_SHIFT        65536U     // 2^16
#define ZSEB_TRIGGER      98304U     // ZSEB_SHIFT + ZSEB_HISTORY: if ( rd_current > ZSEB_SHIFT + ZSEB_HISTORY ) --> shift --> rd_current >= 2^15 + 1
#define ZSEB_FRAME        99328U     // ZSEB_TRIGGER + 2^10: important that 2^10 > max( length ) = 258 !!!

#define ZSEB_HASH_SIZE    16777216U  // ZSEB_LITLEN^3 = 2^24
#define ZSEB_HASH_MASK    16777215U  // ZSEB_LITLEN^3 - 1 = 2^24 - 1
#define ZSEB_HASH_STOP    ( ~( ( zseb_32_t )( 0U ) ) ) // pointers can be larger than 2^16

namespace zseb{

   class lzss{

      public:

         lzss( std::string fullfile, const char unzip );

         virtual ~lzss();

         // Following returns "Last block?"; on input wr_current is reset to zero; on output wr_current is #elem in *_pack; max(wr_current) <= size_pack + 4
         bool deflate( zseb_08_t * llen_pack, zseb_16_t * dist_pack, const zseb_32_t size_pack, zseb_32_t &wr_current );

         void inflate( zseb_08_t * llen_pack, zseb_16_t * dist_pack, const zseb_32_t size_pack );

         void flush();

         zseb_64_t get_lzss_bits() const;

         zseb_64_t get_file_bytes() const;

         //inflate: Idea is to unpack pack blocks of given length for  fullfile

      private:

         /***  INPUT FILE  ***/

         std::fstream file;

         zseb_64_t size_file; // Number of bytes in file

         zseb_64_t size_lzss; // Number of bits with pure LZSS ( 1-bit diff + 8-bit lit OR 1-bit diff + 8-bit len_shift + 15-bit dist_shift )

         char * frame; // Length ZSEB_FRAME; snippet from file --> [ rd_shift : rd_shift + rd_end ]

         zseb_64_t rd_shift;   // Current starting point of readframe

         zseb_32_t rd_end;     // Current validly filled length of readframe [ 0 : ZSEB_READFRAME ]

         zseb_32_t rd_current; // Current position within readframe

         /***  Hash table  ***/

         zseb_32_t * hash_last; // hash_last['abc'] = hash_last[ c + 256 * ( b + 256 * a ) ] = index latest encounter (w.r.t. frame); STOP = ZSEB_HASH_STOP

         zseb_32_t * hash_ptrs; // hash_ptrs[ index ] = index' = index of encounter before index ( length ZSEB_FRAME )

         /***  Functions  ***/

         void __readin__();

         void __shift_left__(); // Shift readframe, hash_last and hash_ptrs

         inline void __move_hash__( const zseb_32_t hash_entry ); // Add hash_entry & rd_current to hash; increment rd_current

         inline void __longest_match__( zseb_32_t &result_ptr, zseb_16_t &result_len, const zseb_32_t hash_entry, const zseb_32_t position ) const;

         inline void __append_lit_encode__( zseb_08_t * llen_pack, zseb_16_t * dist_pack, zseb_32_t &wr_current );

         inline void __append_len_encode__( zseb_08_t * llen_pack, zseb_16_t * dist_pack, zseb_32_t &wr_current, const zseb_16_t dist_shift, const zseb_08_t len_shift );

   };

}

#endif
