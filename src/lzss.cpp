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

#include "lz77.hpp"

#include <assert.h>
//#include <tuple>
#include "lzss.h"
#include "crc32.hpp"
//#include "lz77.hpp"

namespace zseb
{
namespace lz77
{

// Memory-related parameters
// Note that GZIP works with a frame of 65536 and shifts over 32768 whenever insufficient lookahead (MIN_LOOKAHEAD = 258 + 3 + 1)
constexpr const uint32_t DISK_TRIGGER = 4 * HIST_SIZE;
constexpr const uint32_t FRAME_SIZE   = DISK_TRIGGER + 272;
constexpr const uint32_t DISK_SHIFT   = DISK_TRIGGER - HIST_SIZE;

}
}



zseb::lzss::lzss(std::string fullfile, const zseb_modus modus)
{
    assert(modus != zseb_modus::undefined);

    size_lzss = 0;
    size_file = 0;
    checksum  = 0;
    rd_shift  = 0;
    rd_end    = 0;
    rd_current = 0;
    frame = nullptr;
    head  = nullptr;
    prev  = nullptr;

   if ( modus == zseb_modus::zip ){

      file.open( fullfile.c_str(), std::ios::in|std::ios::binary|std::ios::ate );

      if ( file.is_open() ){

         size_file = static_cast<uint64_t>( file.tellg() );
         file.seekg( 0, std::ios::beg );

            frame = new char[zseb::lz77::FRAME_SIZE];
            for (uint32_t cnt = 0; cnt < zseb::lz77::FRAME_SIZE; ++cnt){ frame[ cnt ] = 0U; } // Because match check can go up to 4 beyond rd_end

         head = new uint32_t[zseb::lz77::HASH_SIZE];
         prev = new uint32_t[zseb::lz77::HIST_SIZE];
         for (uint32_t cnt = 0; cnt < zseb::lz77::HASH_SIZE; ++cnt){ head[cnt] = zseb::lz77::HASH_STOP; }
         for (uint32_t cnt = 0; cnt < zseb::lz77::HIST_SIZE; ++cnt){ prev[cnt] = zseb::lz77::HASH_STOP; }

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
    if (head != nullptr) { delete [] head; }
    if (prev != nullptr) { delete [] prev; }
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

    if (nwrite > 0)
    {
        assert( rd_current >= nwrite );

        file.write( frame, nwrite );
        checksum = crc32::update( checksum, frame, nwrite );

        for (uint32_t cnt = 0; cnt < rd_current - nwrite; ++cnt)
            frame[ cnt ] = frame[ nwrite + cnt ];
     
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
            //std::cout << static_cast<char>(llen_pack[idx]) << std::endl;
        }
        else // copy (lenth, distance)
        {
            size_lzss += ZSEB_HIST_BIT + CHAR_BIT + 1U; // 1-bit differentiator
            const uint16_t distance = dist_pack[idx] + zseb::lz77::DIS_SHIFT;
            const uint16_t length   = llen_pack[idx] + zseb::lz77::LEN_SHIFT;
            for (uint32_t cnt = 0U; cnt < length; ++cnt)
                frame[rd_current + cnt] = frame[rd_current - distance + cnt];
            rd_current += length;
            //std::cout << "[" << distance << ", " << length << "]" << std::endl;
        }

        if (rd_current >= zseb::lz77::DISK_TRIGGER)
        {
            file.write(frame, zseb::lz77::DISK_SHIFT);
            checksum = crc32::update(checksum, frame, zseb::lz77::DISK_SHIFT);
            for (uint32_t cnt = 0; cnt < rd_current - zseb::lz77::DISK_SHIFT; ++cnt)
                frame[cnt] = frame[zseb::lz77::DISK_SHIFT + cnt];
            rd_current -= zseb::lz77::DISK_SHIFT;
        }
    }
    //std::cout << "==========================================================================================" << std::endl;
}

bool zseb::lzss::deflate(uint8_t * llen_pack, uint16_t * dist_pack, const uint32_t size_pack, uint32_t &wr_current)
{
    const uint32_t start = (rd_shift + rd_current == 0) ? 0 : zseb::lz77::HIST_SIZE;
    const uint32_t limit = std::min(rd_end, zseb::lz77::DISK_TRIGGER);
    std::pair<uint32_t, uint32_t> result = zseb::lz77::deflate(frame, start, limit, prev, head, llen_pack + wr_current, dist_pack + wr_current);
    rd_current = limit;
    if ( rd_current == zseb::lz77::DISK_TRIGGER )
    {
        for (uint32_t cnt = 0U; cnt < rd_end - zseb::lz77::DISK_SHIFT; ++cnt)
            frame[ cnt ] = frame[ zseb::lz77::DISK_SHIFT + cnt ];
        //std::copy(frame + zseb::lz77::HIST_SIZE, frame + rd_end, frame); // std::copy not guaranteed for overlapping pieces
        rd_shift   += zseb::lz77::DISK_SHIFT;
        rd_end     -= zseb::lz77::DISK_SHIFT;
        rd_current -= zseb::lz77::DISK_SHIFT;

        __readin__();
    }

    wr_current += result.first;
    assert(wr_current <= size_pack);
    size_lzss += result.second;

    return rd_shift + rd_current == size_file;

}

void zseb::lzss::__readin__()
{
    const uint32_t current_read = rd_shift + zseb::lz77::FRAME_SIZE > size_file ? size_file - rd_shift - rd_end : zseb::lz77::FRAME_SIZE - rd_end;
    file.read(frame + rd_end, current_read);
    checksum = crc32::update( checksum, frame + rd_end, current_read );
    rd_end += current_read;
}


