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
    assert(modus == zseb_modus::unzip);

    size_lzss = 0;
    size_file = 0;
    checksum  = 0;
    rd_shift  = 0;
    rd_end    = 0;
    rd_current = 0;
    frame = nullptr;
    head  = nullptr;
    prev  = nullptr;

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
    uint32_t idx_pack = 0;

    while (idx_pack != size_pack)
    {
        std::tuple<uint32_t, uint32_t, uint32_t> result = zseb::lz77::inflate(frame, rd_current, zseb::lz77::DISK_TRIGGER, llen_pack, dist_pack, idx_pack, size_pack);
        rd_current = std::get<0>(result);
        idx_pack   = std::get<1>(result);
        size_lzss += std::get<2>(result);

        if (rd_current >= zseb::lz77::DISK_TRIGGER)
        {
            file.write(frame, zseb::lz77::DISK_SHIFT);
            checksum = crc32::update(checksum, frame, zseb::lz77::DISK_SHIFT);
            for (uint32_t cnt = 0; cnt < rd_current - zseb::lz77::DISK_SHIFT; ++cnt)
                frame[cnt] = frame[zseb::lz77::DISK_SHIFT + cnt];
            rd_current -= zseb::lz77::DISK_SHIFT;
        }
    }

}


