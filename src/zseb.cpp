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
#include <sys/types.h>
#include <utime.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <chrono>

#include "zseb.h"
#include "stream.h"
#include "crc32.hpp"
#include "lz77.hpp"

namespace zseb
{
namespace tools
{

// Note that GZIP works with a frame of 65536 and shifts over 32768 whenever insufficient lookahead (MIN_LOOKAHEAD = 258 + 3 + 1)
constexpr const uint32_t BATCH_SIZE   = 4 * lz77::HIST_SIZE;
constexpr const uint32_t DISK_TRIGGER = BATCH_SIZE + lz77::HIST_SIZE;
constexpr const uint32_t FRAME_EXTRA  = 272;

constexpr const uint32_t ZSEB_BLOCK_SIZE = 32767; // GZIP packs in blocks of 32767
constexpr const uint32_t ZSEB_ARRAY_SIZE = 98304;

uint32_t write_header(const std::string& bigfile, stream * zipfile)
{
    /***  Variables  ***/
    uint32_t crc16 = 0;
    char var;
    char temp[4];

    /***  Fetch last modification time  ***/
    struct stat info;
    if (stat(bigfile.c_str(), &info) != 0)
    {
        std::cerr << "zseb: Unable to open " << bigfile << "." << std::endl;
        exit(255);
    }
    const uint32_t mtime = static_cast<uint32_t>(info.st_mtime);
    stream::int2str(mtime, temp, 4);

    /***  GZIP header  ***/
    /* ID1 */ var = static_cast<uint8_t>(0x1f); zipfile->write(&var, 1); crc16 = crc32::update(crc16, &var, 1);
    /* ID2 */ var = static_cast<uint8_t>(0x8b); zipfile->write(&var, 1); crc16 = crc32::update(crc16, &var, 1);
    /* CM  */ var = static_cast<uint8_t>(8);    zipfile->write(&var, 1); crc16 = crc32::update(crc16, &var, 1);
    /* FLG */ var = static_cast<uint8_t>(10);   zipfile->write(&var, 1); crc16 = crc32::update(crc16, &var, 1); // (0, 0, 0, FCOMMENT=0, FNAME=1, FEXTRA=0, FHCRC=1, FTEXT=0)
    /* MTIME */                                 zipfile->write(temp, 4); crc16 = crc32::update(crc16, temp, 4);
    /* XFL */ var = static_cast<uint8_t>(0);    zipfile->write(&var, 1); crc16 = crc32::update(crc16, &var, 1);
    /* OS  */ var = static_cast<uint8_t>(255);  zipfile->write(&var, 1); crc16 = crc32::update(crc16, &var, 1); // Unknown Operating System

    // FLG.FEXTRA --> no
    // FLG.FNAME  --> yes

    /***  Original filename, terminated by a zero byte block  ***/
    size_t prev = std::string::npos;
    size_t curr = bigfile.find('/', 0);
    while (curr != std::string::npos)
    {
        prev = curr;
        curr = bigfile.find('/', prev + 1);
    }
    std::string stripped = prev == std::string::npos ? bigfile : bigfile.substr(prev + 1, std::string::npos);
    const uint32_t length = static_cast<uint32_t>(stripped.length());
    const char * buffer = stripped.c_str();
    zipfile->write(buffer, length);
    crc16 = crc32::update(crc16, buffer, length);
    /* ZER */ var = static_cast<uint8_t>(0);    zipfile->write(&var, 1); crc16 = crc32::update(crc16, &var, 1);

    // FLG.FCOMMENT --> no
    // FLG.FHCRC    --> yes

    /***  Two least significant bytes of all bytes preceding the CRC16  ***/
    stream::int2str(crc16, temp, 2);
    zipfile->write(temp, 2);

    return mtime;
}


std::pair<std::string, uint32_t> read_header(stream * zipfile)
{
    /***  Variables  ***/
    uint32_t crc16 = 0;
    char var;
    char temp[4];

    /***  GZIP header  ***/
    /* ID1 */ zipfile->read(&var, 1); crc16 = crc32::update(crc16, &var, 1); if (static_cast<uint8_t>(var) != 0x1f){ std::cerr << "zseb: Incompatible ID1." << std::endl; exit(255); }
    /* ID2 */ zipfile->read(&var, 1); crc16 = crc32::update(crc16, &var, 1); if (static_cast<uint8_t>(var) != 0x8b){ std::cerr << "zseb: Incompatible ID2." << std::endl; exit(255); }
    /* CM  */ zipfile->read(&var, 1); crc16 = crc32::update(crc16, &var, 1); if (static_cast<uint8_t>(var) != 8   ){ std::cerr << "zseb: Incompatible CM."  << std::endl; exit(255); }
    /* FLG */ zipfile->read(&var, 1); crc16 = crc32::update(crc16, &var, 1); const uint8_t FLG = static_cast<uint8_t>(var);
    if (((FLG >> 7) & 1U) == 1U){ std::cerr << "zseb: Reserved bit is non-zero." << std::endl; exit(255); }
    if (((FLG >> 6) & 1U) == 1U){ std::cerr << "zseb: Reserved bit is non-zero." << std::endl; exit(255); }
    if (((FLG >> 5) & 1U) == 1U){ std::cerr << "zseb: Reserved bit is non-zero." << std::endl; exit(255); }
    const bool FCOMMENT = (((FLG >> 4) & 1U) == 1U);
    const bool FNAME    = (((FLG >> 3) & 1U) == 1U);
    const bool FEXTRA   = (((FLG >> 2) & 1U) == 1U);
    const bool FHCRC    = (((FLG >> 1) & 1U) == 1U);
  //const bool FTEXT    = (( FLG       & 1U) == 1U);
    /* MTIME */ zipfile->read(temp, 4); crc16 = crc32::update(crc16, temp, 4); const uint32_t mtime = stream::str2int(temp, 4);
    /* XFL   */ zipfile->read(&var, 1); crc16 = crc32::update(crc16, &var, 1);
    /* OS    */ zipfile->read(&var, 1); crc16 = crc32::update(crc16, &var, 1);

    if (FEXTRA)
    {
        zipfile->read(temp, 2);
        crc16 = crc32::update(crc16, temp, 2);
        const uint16_t XLEN = static_cast<uint16_t>(stream::str2int(temp, 2));
        for (uint16_t cnt = 0; cnt < XLEN; ++cnt)
        {
            zipfile->read(&var, 1);
            crc16 = crc32::update(crc16, &var, 1);
        }
    }

    std::string filename = "";
    if (FNAME)
    {
        bool proceed = true;
        while (proceed)
        {
            zipfile->read(&var, 1);
            crc16 = crc32::update(crc16, &var, 1);
            proceed = static_cast<uint8_t>(var) != 0;
            if (proceed){ filename += var; }
        }
    }

    if (FCOMMENT)
    {
        bool proceed = true;
        while (proceed)
        {
            zipfile->read(&var, 1);
            crc16 = crc32::update(crc16, &var, 1);
            proceed = static_cast<uint8_t>(var) != 0;
        }
    }

    if (FHCRC)
    {
        zipfile->read(temp, 2);
        const uint32_t checksum = stream::str2int(temp, 2);
        crc16 = crc16 & UINT16_MAX;
        if (checksum != crc16)
        {
            std::cerr << "zseb: Computed CRC16 = " << crc16 << " if different from read-in CRC16 = " << checksum << "." << std::endl;
            exit(255);
        }
    }

    return { filename, mtime };
}


void set_time(const std::string& filename, const uint32_t mtime)
{
    struct utimbuf overwrite;
    overwrite.actime  = time(NULL); // Present
    overwrite.modtime = mtime;      // Modication time original file
    utime(filename.c_str(), &overwrite);
}




void zip(const std::string& bigfile, const std::string& smallfile, const bool print)
{
    stream * zipfile = new stream(smallfile, 'W');
    const uint32_t mtime = write_header(bigfile, zipfile);
    std::ifstream origfile;
    origfile.open(bigfile.c_str(), std::ios::in|std::ios::binary|std::ios::ate);
    if (!origfile.is_open())
    {
        std::cerr << "zseb: Unable to open " << bigfile << "." << std::endl;
        exit(255);
    }
    uint64_t size_lzss = 0;
    const uint64_t size_file = static_cast<uint64_t>(origfile.tellg());
    origfile.seekg(0, std::ios::beg);

    char * frame = new char[DISK_TRIGGER + FRAME_EXTRA];
    for (uint32_t cnt = 0; cnt < DISK_TRIGGER + FRAME_EXTRA; ++cnt){ frame[cnt] = 0; }
    uint32_t * head = new uint32_t[lz77::HASH_SIZE];
    uint32_t * prev = new uint32_t[lz77::HIST_SIZE];
    uint8_t  * llen_pack = new  uint8_t[ZSEB_ARRAY_SIZE];
    uint16_t * dist_pack = new uint16_t[ZSEB_ARRAY_SIZE];
    huffman * coder = new huffman();

    uint32_t checksum   = 0;
    uint64_t rd_shift   = 0;
    uint32_t rd_current = 0;

    uint32_t rd_end = BATCH_SIZE > size_file ? size_file : BATCH_SIZE;
    origfile.read(frame, rd_end);
    checksum = crc32::update(checksum, frame, rd_end);

    uint64_t size_zlib = zipfile->getpos(); // Preamble are full bytes
    uint32_t wr_current = 0;

    bool last_block = false;
    uint32_t block_form;

    uint64_t time_lzss = 0.0;
    uint64_t time_huff = 0.0;

    while ((!last_block) || (wr_current > 0))
    {
        // LZSS a block: gzip packs (llen_pack, dist_pack) blocks of size 32767
        auto start = std::chrono::steady_clock::now();
        while ((!last_block) && (wr_current < ZSEB_BLOCK_SIZE))
        {
            // first run: rd_current = 0         & rd_shift = 0 & rd_end = BATCH_SIZE   (except if eof)
            // other run: rd_current = HIST_SIZE & rd_shift > 0 & rd_end = DISK_TRIGGER (except if eof)
            std::pair<uint32_t, uint32_t> result = zseb::lz77::deflate(frame, rd_current, rd_end, prev, head, llen_pack + wr_current, dist_pack + wr_current);
            const uint32_t upper = rd_shift + rd_current == 0 ? BATCH_SIZE : DISK_TRIGGER;
            rd_current = rd_end;

            if (rd_end == upper)
            {
                const uint32_t shift = upper - lz77::HIST_SIZE;
                if (shift != 0)
                {
                    assert(rd_end <= 2 * shift); // Requirement for std::copy (which does not allow overlapping pieces)
                    std::copy(frame + shift, frame + rd_end, frame);
                }
                rd_shift  += shift;
                rd_end     = lz77::HIST_SIZE;
                rd_current = lz77::HIST_SIZE;

                const uint32_t current_read = rd_shift + DISK_TRIGGER > size_file ? size_file - rd_shift - lz77::HIST_SIZE : BATCH_SIZE;
                origfile.read(frame + lz77::HIST_SIZE, current_read);
                checksum = crc32::update(checksum, frame + lz77::HIST_SIZE, current_read);
                rd_end += current_read;
            }

            wr_current += result.first;
            assert(wr_current <= ZSEB_ARRAY_SIZE);
            size_lzss += result.second;

            last_block = rd_shift + rd_current == size_file;
        }

        const uint32_t huffman_size = std::min(wr_current, ZSEB_BLOCK_SIZE);
        auto end = std::chrono::steady_clock::now();
        time_lzss += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        // Compute dynamic Huffman trees & X01 and X10 sizes
        start = std::chrono::steady_clock::now();
        coder->calc_tree( llen_pack, dist_pack, huffman_size );
        const uint32_t size_X1 = coder->get_size_X1();
        const uint32_t size_X2 = coder->get_size_X2();
        end = std::chrono::steady_clock::now();
        time_huff += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        // What is the minimal output?
        block_form = size_X2 < size_X1 ? 2 : 1;
        zipfile->write(last_block ? 1 : 0, 1);
        zipfile->write(block_form, 2);

        // Write out
        start = std::chrono::steady_clock::now();
        if (block_form == 2)
            coder->write_tree(zipfile);
        else
            coder->fixed_tree('O');
        coder->pack(zipfile, llen_pack, dist_pack, huffman_size);
        end = std::chrono::steady_clock::now();
        time_huff += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        //Shift the non-used tail of llen_pack & dist_pack down
        wr_current = wr_current - huffman_size;
        if (wr_current > 0)
        {
            for (uint32_t cnt = 0; cnt < wr_current; ++cnt)
                llen_pack[cnt] = llen_pack[huffman_size + cnt];
            for (uint32_t cnt = 0; cnt < wr_current; ++cnt)
                dist_pack[cnt] = dist_pack[huffman_size + cnt];
        }
    }

    delete coder;
    delete [] llen_pack;
    delete [] dist_pack;
    delete [] frame;
    delete [] head;
    delete [] prev;

    if (origfile.is_open()){ origfile.close(); }

    zipfile->flush();
    size_zlib = zipfile->getpos() - size_zlib; // Bytes after flush

    char temp[4];
    // Write CRC32
    stream::int2str(checksum, temp, 4);
    zipfile->write(temp, 4);
    // Write ISIZE = size_file mod 2^32
    const uint32_t ISIZE = static_cast<uint32_t>(size_file & UINT32_MAX);
    stream::int2str(ISIZE, temp, 4);
    zipfile->write(temp, 4);

    delete zipfile;
    set_time(smallfile, mtime);

    if (print)
    {
        std::cout << "zseb: zip: comp(lzss)  = " << size_file / (0.125 * size_lzss) << std::endl;
        std::cout << "           comp(total) = " << size_file / (1.0 * size_zlib) << std::endl;
        std::cout << "           time(lzss)  = " << 1e-6 * time_lzss << " seconds" << std::endl;
        std::cout << "           time(huff)  = " << 1e-6 * time_huff << " seconds" << std::endl;
    }
}


void unzip(const std::string& smallfile, std::string& bigfile, const bool name, const bool print)
{
    stream * zipfile = new stream(smallfile, 'R');
    std::pair<std::string, uint32_t> orignametime = read_header(zipfile);
    if (name){ bigfile = orignametime.first; }
    std::ofstream origfile;
    origfile.open(bigfile.c_str(), std::ios::out|std::ios::binary|std::ios::trunc );

    uint32_t checksum  = 0;
    uint64_t size_lzss = 0;
    uint64_t size_zlib = zipfile->getpos(); // Preamble are full Bytes

    uint32_t last_block = 0;

    uint64_t time_lzss = 0.0;
    uint64_t time_huff = 0.0;

    std::vector<char> frame; frame.reserve(DISK_TRIGGER + FRAME_EXTRA);
    std::vector<uint8_t>  llen_pack; llen_pack.reserve(ZSEB_ARRAY_SIZE);
    std::vector<uint16_t> dist_pack; dist_pack.reserve(ZSEB_ARRAY_SIZE);
    huffman * coder = new huffman();

    while (last_block == 0)
    {
        last_block = zipfile->read(1);
        uint32_t block_form = zipfile->read(2); // '10'_b dyn trees, '01'_b fixed trees, '00'_b uncompressed, '11'_b error
        if (block_form == 3)
        {
            std::cerr << "zseb: X11 is not a valid block mode." << std::endl;
            exit(255);
        }

        if (block_form == 0)
        {
            zipfile->nextbyte();
            char vals[2];
            zipfile->read(vals, 2); const uint16_t  LEN = static_cast<uint16_t>(stream::str2int(vals, 2));
            zipfile->read(vals, 2); const uint16_t NLEN = static_cast<uint16_t>(stream::str2int(vals, 2));
            const uint16_t NLEN2 = ~LEN;
            if (NLEN != NLEN2)
            {
                std::cerr << "zseb: Block type X00: NLEN != ( ~LEN )" << std::endl;
                exit(255);
            }

            const uint32_t write_size = LEN > lz77::HIST_SIZE ? static_cast<uint32_t>(frame.size()) : std::max(static_cast<uint32_t>(frame.size()) + LEN, lz77::HIST_SIZE) - lz77::HIST_SIZE;
            if (write_size != 0)
            {
                assert(frame.size() >= write_size);
                origfile.write(&frame[0], write_size);
                checksum = crc32::update(checksum, &frame[0], write_size);
                frame.erase(frame.begin(), frame.begin() + write_size);
            }
            frame.insert(frame.end(), LEN, 0);
            zipfile->read(&frame[frame.size() - LEN], LEN);
        }
        else
        {
            auto start = std::chrono::steady_clock::now();
            if (block_form == 2) // Dynamic trees
                coder->load_tree(zipfile);
            else // Fixed trees
                coder->fixed_tree('I');

            coder->unpack(zipfile, llen_pack, dist_pack);
            auto end = std::chrono::steady_clock::now();
            time_huff += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

            if (llen_pack.size() != 0)
            {
                start = std::chrono::steady_clock::now();
                for (size_t idx = 0; idx != llen_pack.size(); ++idx)
                {
                    size_lzss += lz77::inflate(frame, llen_pack[idx], dist_pack[idx]);

                    if (frame.size() >= DISK_TRIGGER)
                    {
                        origfile.write(&frame[0], BATCH_SIZE);
                        checksum = crc32::update(checksum, &frame[0], BATCH_SIZE);
                        frame.erase(frame.begin(), frame.begin() + BATCH_SIZE);
                    }
                }
                llen_pack.clear();
                dist_pack.clear();
                end = std::chrono::steady_clock::now();
                time_lzss += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            }
        }
    }

    // Flush
    origfile.write(&frame[0], frame.size());
    checksum = crc32::update(checksum, &frame[0], frame.size());
    frame.clear();
    const uint64_t size_file = static_cast<uint64_t>(origfile.tellp());

    delete coder;
    if (origfile.is_open()){ origfile.close(); }

    zipfile->nextbyte();
    size_zlib = zipfile->getpos() - size_zlib; // Bytes after nextbyte

    char temp[4];
    // Read CRC32
    zipfile->read(temp, 4);
    const uint32_t checksum_read = stream::str2int(temp, 4);
    if (checksum != checksum_read)
    {
        std::cerr << "zseb: Computed CRC32 = " << checksum << " is different from read-in CRC32 = " << checksum_read << "." << std::endl;
        exit(255);
    }
    // Read ISIZE
    zipfile->read(temp, 4);
    const uint32_t isize_read = stream::str2int(temp, 4);
    const uint32_t isize = static_cast<uint32_t>(size_file & UINT32_MAX);
    if (isize != isize_read)
    {
        std::cerr << "zseb: Computed ISIZE = " << isize << " is different from read-in ISIZE = " << isize_read << "." << std::endl;
        exit(255);
    }

    delete zipfile;
    zseb::tools::set_time(bigfile, orignametime.second);

    if (print)
    {
        std::cout << "zseb: unzip: comp(lzss)  = " << size_file / (0.125 * size_lzss) << std::endl;
        std::cout << "             comp(total) = " << size_file / (1.0 * size_zlib) << std::endl;
        std::cout << "             time(lzss)  = " << 1e-6 * time_lzss << " seconds" << std::endl;
        std::cout << "             time(huff)  = " << 1e-6 * time_huff << " seconds" << std::endl;
    }
}


} // End of namespace tools
} // End of namespace zseb


