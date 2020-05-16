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

#include <fstream>
#include <iostream>
#include <string>
#include <assert.h>
#include <stdint.h>
#include <limits.h>


namespace zseb
{
namespace stream
{


constexpr uint32_t str2int(const char * store, const uint16_t num) noexcept
{
    uint32_t value = 0;
    for (uint16_t cnt = 0; cnt < num; ++cnt)
        value = (value << CHAR_BIT) ^ static_cast<uint8_t>(store[num - 1 - cnt]);
    return value;
}


constexpr void int2str(uint32_t value, char * store, const uint16_t num) noexcept
{
    for (uint16_t cnt = 0; cnt < num; ++cnt)
    {
        store[cnt] = static_cast<uint8_t>(value & UINT8_MAX);
        value = value >> CHAR_BIT;
    }
}


} // End of namespace stream


// Bitwise wrapper for ifstream
class ibstream
{
    public:

        ibstream(const std::string& smallfile) : data(0), ibit(0)
        {
            ifile.open(smallfile.c_str(), std::ios::in|std::ios::binary);
            if (!ifile.is_open())
            {
                std::cerr << "zseb: Unable to open " << smallfile << "." << std::endl;
                exit(255);
            }
        }

        ~ibstream()
        {
            if (ifile.is_open())
                ifile.close();
        }

        uint64_t pos()
        {
            return ifile.tellg();
        }

        void next_byte()
        {
            if (ibit != 0)
                read(ibit);
        }

        uint32_t read(const uint16_t nbits)
        {
            assert(nbits <= 24);
            while (ibit < nbits)
            {
                char toread;
                ifile.read(&toread, 1);
                const uint32_t toshift = static_cast<uint8_t>(toread);
                data = data ^ (toshift << ibit);
                ibit = ibit + CHAR_BIT;
            }

            const uint32_t fetch = data & ((1U << nbits) - 1);
            data = data >> nbits;
            ibit = ibit - nbits;

            return fetch;
        }

        void read(char * buffer, const uint32_t size)
        {
            assert(ibit == 0);
            ifile.read(buffer, size);
        }

    private:

        std::ifstream ifile;

        uint32_t data; // Not yet completed byte

        uint16_t ibit; // Number of bits in not yet completed byte

};


// Bitwise wrapper for ofstream
class obstream
{
    public:

        obstream(const std::string& smallfile) : data(0), ibit(0)
        {
            ofile.open(smallfile.c_str(), std::ios::out|std::ios::binary|std::ios::trunc);
        }

        ~obstream()
        {
            close();
        }

        void close()
        {
            if (ofile.is_open())
                ofile.close();
        }

        uint64_t pos()
        {
            return ofile.tellp();
        }

        void write(const uint32_t flush, const uint16_t nbits)
        {
            assert(flush == (flush & ((1U << nbits) - 1)));
            assert(ibit + nbits <= 32);
            data = data ^ (flush << ibit);
            ibit = ibit + nbits;

            while (ibit >= CHAR_BIT)
            {
                const char towrite = static_cast<uint8_t>(data & UINT8_MAX);
                ofile.write(&towrite, 1);
                data = data >> CHAR_BIT;
                ibit = ibit - CHAR_BIT;
            }
        }

        void write(const char * buffer, const uint32_t size)
        {
            assert(ibit == 0);
            ofile.write(buffer, size);
        }

        void flush()
        {
            while (ibit != 0)
            {
                const char towrite = static_cast<uint8_t>(data & UINT8_MAX);
                ofile.write(&towrite, 1);
                data = data >> CHAR_BIT;
                ibit = ibit > CHAR_BIT ? ibit - CHAR_BIT : 0;
            }
        }

    private:

        std::ofstream ofile;

        uint32_t data; // Not yet completed byte

        uint16_t ibit; // Number of bits in not yet completed byte

};


} // End of namespace zseb


