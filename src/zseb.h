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

//#define ZSEB_ARRAY_SIZE 65536U
#define ZSEB_BLOCK_SIZE 32767U // GZIP packs in blocks of 32767
#define ZSEB_ARRAY_SIZE 98304U

namespace zseb
{
namespace tools
{
    uint32_t write_header(const std::string& bigfile, stream * zipfile);
    std::pair<std::string, uint32_t> read_header(stream * zipfile);
    void set_time(std::string filename, const uint32_t mtime);
    void zip(lzss * flate, stream * zipfile, const bool print);
    void unzip(lzss * flate, stream * zipfile, const bool print);
}
/*
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

         stream * zipfile;

         bool print;

         uint32_t mtime;

   };
*/
}


