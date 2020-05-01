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
#include "lzss.h"
#include "huffman.h"

#define ZSEB_PACK_SIZE      ( ZSEB_HIST_SIZE )
//#define ZSEB_PACK_SIZE (1U << 16U)

namespace zseb{

   class zseb{

      public:

         zseb( std::string packfile, const zseb_modus modus, const bool verbose );

         virtual ~zseb();

         void setup_flate( std::string bigfile, const zseb_modus modus );

         void zip();

         void unzip();

         void write_preamble( std::string bigfile );

         std::string strip_preamble();

         void set_time( std::string filename );

      private:

         lzss * flate;

         huffman * coder;

         stream * zipfile;

         uint8_t * llen_pack; // Length ZSEB_WR_FRAME; contains lit_code OR len_shift ( packing happens later )

         uint16_t * dist_pack; // Length ZSEB_WR_FRAME; contains dist_shift [ 0 : 32767 ] with 65535 means literal

         uint32_t wr_current;  // Currently validly filled length of llen_pack and dist_pack

         bool print;

         uint32_t mtime;

   };

}


