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
#include <tuple>
#include "lzss.h"
#include "crc32.h"

namespace zseb
{
namespace lz77
{

// LZ77 parameters
constexpr const uint32_t MAX_MATCH    = 258;
constexpr const uint32_t LEN_SHIFT    = 3;
constexpr const uint32_t DIS_SHIFT    = 1;
constexpr const uint32_t HIST_SIZE    = 32768; // 2^15
constexpr const uint32_t HIST_MASK    = HIST_SIZE - 1;
constexpr const uint32_t HASH_SIZE    = 16777216; // 256^3
constexpr const uint32_t HASH_MASK    = HASH_SIZE - 1;
constexpr const uint32_t HASH_STOP    = 0;

// Memory-related parameters
constexpr const uint32_t DISK_TRIGGER = 2 * HIST_SIZE;
constexpr const uint32_t FRAME_SIZE   = DISK_TRIGGER + 272;

// Some GZIP parameters
constexpr const uint32_t GZIP_TOO_FAR = 4096;
constexpr const uint32_t GZIP_CHAIN   = 4096;
constexpr const uint32_t GZIP_GOOD    = 32;

constexpr std::pair<uint16_t, uint16_t> match(char * window, uint16_t ptr, const uint32_t current, const uint32_t runway, uint16_t * prev3, uint16_t chain_length) noexcept
{
    const uint16_t max_len = std::min(MAX_MATCH, runway);
    const uint16_t ptr_lim = pos_diff(current, HIST_SIZE);
    if (max_len < LEN_SHIFT)
        return { HASH_STOP, 1 };

    char * start  = window + current + 2;
    char * cutoff = window + current + max_len;

    uint16_t result_ptr = HASH_STOP;
    uint16_t result_len = 1;

    while ((ptr > ptr_lim) && (chain_length-- != 0))
    {
        if ((window[current + result_len] != window[ptr + result_len]) ||
            (window[current + result_len - 1U] != window[ptr + result_len - 1U]))
        {
            ptr = prev3[ptr & HIST_MASK];
            continue;
        }

        char * present = start;
        char * history = window + ptr + 2;

        do {} while( ( *(++history) == *(++present) ) && ( *(++history) == *(++present) ) &&
                     ( *(++history) == *(++present) ) && ( *(++history) == *(++present) ) &&
                     ( *(++history) == *(++present) ) && ( *(++history) == *(++present) ) &&
                     ( *(++history) == *(++present) ) && ( *(++history) == *(++present) ) &&
                     ( present < cutoff ) ); // FRAME_SIZE is a full 272 larger than DISK_TRIGGER

        const uint16_t length = static_cast<uint16_t>(present >= cutoff ? max_len : max_len - (cutoff - present));

        if (length > result_len)
        {
            result_len = length;
            result_ptr = ptr;
            if (result_len >= max_len)
                break;
        }
        ptr = prev3[ptr & HIST_MASK];
    }

    if ((result_len == LEN_SHIFT) && (result_ptr + GZIP_TOO_FAR < current))
        return { HASH_STOP, 1U };

    return { result_ptr, result_len };
}

} // End of namespace lz77
} // End of namespace zseb




zseb::lzss::lzss( std::string fullfile, const zseb_modus modus ){

   assert(modus != zseb_modus::undefined);

   size_lzss = 0;
   size_file = 0;
   checksum  = 0;
   rd_shift  = 0;
   rd_end    = 0;
   rd_current = 0;
   frame     = NULL;
   hash_head = NULL;
   hash_prv3 = NULL;

   if ( modus == zseb_modus::zip ){

      file.open( fullfile.c_str(), std::ios::in|std::ios::binary|std::ios::ate );

      if ( file.is_open() ){

         size_file = static_cast<uint64_t>( file.tellg() );
         file.seekg( 0, std::ios::beg );

         frame = new char[zseb::lz77::FRAME_SIZE];
         for (uint32_t cnt = 0; cnt < zseb::lz77::FRAME_SIZE; ++cnt){ frame[ cnt ] = 0U; } // Because match check can go up to 4 beyond rd_end

         hash_head = new uint64_t[ zseb::lz77::HASH_SIZE ];
         hash_prv3 = new uint16_t[ zseb::lz77::HIST_SIZE ];
         for (uint32_t cnt = 0; cnt < zseb::lz77::HASH_SIZE; ++cnt){ hash_head[ cnt ] = zseb::lz77::HASH_STOP; }
         for (uint32_t cnt = 0; cnt < zseb::lz77::HIST_SIZE; ++cnt){ hash_prv3[ cnt ] = zseb::lz77::HASH_STOP; }

         __readin__(); // Get ready for work

      } else {

         std::cerr << "zseb: Unable to open " << fullfile << "." << std::endl;
         exit( 255 );

      }
   }

   if ( modus == zseb_modus::unzip ){

      file.open( fullfile.c_str(), std::ios::out|std::ios::binary|std::ios::trunc );

      frame = new char[zseb::lz77::FRAME_SIZE];

   }

}

zseb::lzss::~lzss()
{
    if (file.is_open()){ file.close(); }
    if (frame     != NULL){ delete [] frame;     }
    if (hash_head != NULL){ delete [] hash_head; }
    if (hash_prv3 != NULL){ delete [] hash_prv3; }
}

void zseb::lzss::flush()
{
    file.write(frame, rd_current);
    checksum = crc32::update(checksum, frame, rd_current);
    rd_current = 0;
    size_file = static_cast<uint64_t>(file.tellg());
}

void zseb::lzss::copy(stream * zipfile, const uint16_t size_copy)
{
    uint32_t nwrite = 0;
    if (size_copy > zseb::lz77::HIST_SIZE)
        nwrite = rd_current; // Write everything away, there will be still enough history
    else // ( zseb::lz77::HIST_SIZE - size_copy ) >= 0
    {
        if ( rd_current > ( zseb::lz77::HIST_SIZE - size_copy ) ){
         nwrite = rd_current - ( zseb::lz77::HIST_SIZE - size_copy ); // rd_current + size_copy - nwrite = zseb::lz77::HIST_SIZE
      }
      // If rd_current + size_copy <= zseb::lz77::HIST_SIZE, no need to write
    }

   if ( nwrite > 0 ){

      assert( rd_current >= nwrite );

      file.write( frame, nwrite );
      checksum = crc32::update( checksum, frame, nwrite );

      for ( uint32_t cnt = 0; cnt < ( rd_current - nwrite ); cnt++ ){
         frame[ cnt ] = frame[ nwrite + cnt ];
      }

      rd_current -= nwrite;
    }

    zipfile->read( frame + rd_current, size_copy );
    rd_current += size_copy;
}

void zseb::lzss::inflate(uint8_t * llen_pack, uint16_t * dist_pack, const uint32_t size_pack)
{
    for (uint32_t idx = 0U; idx < size_pack; ++idx)
    {
        if (dist_pack[idx] == ZSEB_MASK_16T) // write literal
        {
            size_lzss += CHAR_BIT + 1U; // 1-bit differentiator
            frame[rd_current] = static_cast<char>(llen_pack[idx]);
            ++rd_current;
            std::cout << static_cast<char>(llen_pack[idx]) << std::endl;
        }
        else // copy (lenth, distance)
        {
            size_lzss += ZSEB_HIST_BIT + CHAR_BIT + 1U; // 1-bit differentiator
            const uint16_t distance = dist_pack[idx] + zseb::lz77::DIS_SHIFT;
            const uint16_t length   = llen_pack[idx] + zseb::lz77::LEN_SHIFT;
            for (uint32_t cnt = 0U; cnt < length; ++cnt)
                frame[rd_current + cnt] = frame[rd_current - distance + cnt];
            rd_current += length;
            std::cout << "[" << distance << ", " << length << "]" << std::endl;
        }

        if (rd_current >= zseb::lz77::DISK_TRIGGER)
        {
            file.write(frame, zseb::lz77::HIST_SIZE);
            checksum = crc32::update(checksum, frame, zseb::lz77::HIST_SIZE);
            for (uint32_t cnt = 0; cnt < rd_current - zseb::lz77::HIST_SIZE; ++cnt)
                frame[cnt] = frame[zseb::lz77::HIST_SIZE + cnt];
            rd_current -= zseb::lz77::HIST_SIZE;
        }
    }
    std::cout << "==========================================================================================" << std::endl;
}

bool zseb::lzss::deflate(uint8_t * llen_pack, uint16_t * dist_pack, const uint32_t size_pack, uint32_t &wr_current)
{
    uint16_t longest_ptr0;
    uint16_t longest_len0 = 3; // No reuse of ( ptr1, len1 ) data initially
    uint16_t longest_ptr1;
    uint16_t longest_len1;
    uint32_t hash_entry = 0;

    if (rd_current + zseb::lz77::LEN_SHIFT <= rd_end)
    {
        hash_entry =                            static_cast<uint8_t>(frame[rd_current    ]);
        hash_entry = (hash_entry << CHAR_BIT) | static_cast<uint8_t>(frame[rd_current + 1]);
        hash_entry = (hash_entry << CHAR_BIT) | static_cast<uint8_t>(frame[rd_current + 2]);
    }

    // Obtain an upper_limit for rd_current, so that the current processed block does not exceed 32K
    //const uint32_t limit1 = rd_shift + rd_current == 0 ? zseb::lz77::HIST_SIZE : zseb::lz77::DISK_TRIGGER;
    const uint32_t limit1 = zseb::lz77::DISK_TRIGGER;
    const uint32_t limit2 = rd_end;
    const uint32_t upper_limit = limit2 < limit1 ? limit2 : limit1;

    while (rd_current < upper_limit) // Inside: rd_current < zseb::lz77::DISK_TRIGGER = 65536
    {
        if (longest_len0 == 1)
        {
            longest_ptr0 = longest_ptr1;
            longest_len0 = longest_len1;
        }
        else
        {
            longest_ptr0 = pos_diff(hash_head[hash_entry], rd_shift); // hash_head[hash_entry] > rd_shift ? static_cast<uint16_t>(hash_head[hash_entry] - rd_shift) : 0;
            std::tie(longest_ptr0, longest_len0) = zseb::lz77::match(frame, longest_ptr0, rd_current, /*upper_limit*/rd_end - rd_current, hash_prv3, zseb::lz77::GZIP_CHAIN);
        }

        __move_hash__(hash_entry); // Update of hash_prv3, hash_head, rd_current, hash_entry
        { // Lazy evaluation candidate
            longest_ptr1 = pos_diff(hash_head[hash_entry], rd_shift); // hash_head[hash_entry] > rd_shift ? static_cast<uint16_t>(hash_head[ hash_entry ] - rd_shift) : 0;
            std::tie(longest_ptr1, longest_len1) = zseb::lz77::match(frame, longest_ptr1, rd_current, /*upper_limit*/rd_end - rd_current, hash_prv3,
                (longest_len0 >= zseb::lz77::GZIP_GOOD) ? zseb::lz77::GZIP_CHAIN >> 2 : zseb::lz77::GZIP_CHAIN);
        }

        if ((longest_ptr0 == zseb::lz77::HASH_STOP) || (longest_len1 > longest_len0))
        { // lazy evaluation
            __append_lit_encode__(llen_pack, dist_pack, wr_current);
            longest_len0 = 1;
        }
        else
        {
            const uint16_t dist_shft = static_cast<uint16_t>(rd_current - (1 + longest_ptr0 + zseb::lz77::DIS_SHIFT));
            const uint8_t   len_shft = static_cast<uint8_t>(longest_len0 - zseb::lz77::LEN_SHIFT);
            __append_len_encode__( llen_pack, dist_pack, wr_current, dist_shft, len_shft );
        }

        for (uint16_t cnt = 1; cnt < longest_len0; ++cnt){ __move_hash__(hash_entry); }
    }

    if ( rd_current >= zseb::lz77::DISK_TRIGGER )
    {
        for (uint32_t cnt = 0U; cnt < rd_end - zseb::lz77::HIST_SIZE; ++cnt)
            frame[ cnt ] = frame[ zseb::lz77::HIST_SIZE + cnt ];
        rd_shift   += zseb::lz77::HIST_SIZE;
        rd_end     -= zseb::lz77::HIST_SIZE;
        rd_current -= zseb::lz77::HIST_SIZE;

        __readin__();

        for (uint16_t cnt = 0; cnt < zseb::lz77::HIST_SIZE; ++cnt)
            hash_prv3[cnt] = hash_prv3[cnt] > zseb::lz77::HIST_SIZE ? hash_prv3[cnt] ^ zseb::lz77::HIST_SIZE : 0;
    }

    if (rd_end > rd_current)
    {
        assert(wr_current <= size_pack);
        return false; // Not yet last block
    }
    return true; // Last block
}

void zseb::lzss::__readin__()
{
    const uint32_t current_read = rd_shift + zseb::lz77::FRAME_SIZE > size_file ? size_file - rd_shift - rd_end : zseb::lz77::FRAME_SIZE - rd_end;
    file.read(frame + rd_end, current_read);
    checksum = crc32::update( checksum, frame + rd_end, current_read );
    rd_end += current_read;
}

void zseb::lzss::__move_hash__(uint32_t &hash_entry)
{
    hash_prv3[rd_current & zseb::lz77::HIST_MASK] = hash_head[hash_entry] > rd_shift ? static_cast<uint16_t>(hash_head[hash_entry] - rd_shift) : 0U;
    hash_head[hash_entry] = rd_shift + rd_current; // rd_current always < 2**16 upon set
    ++rd_current;
    hash_entry = ( static_cast<uint8_t>( frame[ rd_current + 2 ] ) ) | ( ( hash_entry << CHAR_BIT ) & zseb::lz77::HASH_MASK );
}

void zseb::lzss::__append_lit_encode__(uint8_t * llen_pack, uint16_t * dist_pack, uint32_t &wr_current)
{
    size_lzss += CHAR_BIT + 1; // 1-bit differentiator

    llen_pack[wr_current] = static_cast<uint8_t>(frame[rd_current - 1]); // [ 0 : 255 ]
    dist_pack[wr_current] = UINT16_MAX;

    ++wr_current;
}

void zseb::lzss::__append_len_encode__(uint8_t * llen_pack, uint16_t * dist_pack, uint32_t &wr_current, const uint16_t dist_shift, const uint8_t len_shift)
{
    size_lzss += ZSEB_HIST_BIT + CHAR_BIT + 1U; // 1-bit differentiator

    llen_pack[wr_current] = len_shift;  // [ 0 : 255 ]
    dist_pack[wr_current] = dist_shift; // [ 0 : 32767 ]

    ++wr_current;
}

