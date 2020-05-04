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

#include <stdint.h>  // uint{8,16,32,64}_t
#include <limits.h>  // CHAR_BIT
#include <utility>   // std::pair
#include <algorithm> // std::min

namespace zseb
{
namespace lz77
{

constexpr const uint32_t MAX_MATCH = 258;
constexpr const uint32_t LEN_SHIFT = 3;
constexpr const uint32_t DIS_SHIFT = 1;
constexpr const uint32_t HIST_BITS = 15;
constexpr const uint32_t HIST_SIZE = 1U << HIST_BITS;
constexpr const uint32_t HIST_MASK = HIST_SIZE - 1;
constexpr const uint32_t HASH_SHFT = 5;
constexpr const uint32_t HASH_SIZE = 1U << (3 * HASH_SHFT);
constexpr const uint32_t HASH_MASK = HASH_SIZE - 1;
constexpr const uint32_t HASH_STOP = 0;
constexpr const uint32_t TOO_FAR   = 4096; // Discard matches of length LEN_SHIFT if further than TOO_FAR


constexpr std::pair<uint32_t, uint16_t> match(const char * window, const uint32_t current, const uint32_t runway, const uint32_t * prev) noexcept
{
    const uint16_t max_len = std::min(MAX_MATCH, runway);
    if (max_len < LEN_SHIFT)
        return { HASH_STOP, 1 };

    const char * cutoff = window + current + max_len;

    uint32_t result_ptr = HASH_STOP;
    uint16_t result_len = 1;

    const uint32_t ptr_lim = current > HIST_SIZE ? current - HIST_SIZE : 0;
    uint32_t ptr = prev[current & HIST_MASK]; // ptr == current & HIST_SIZE implies ptr = current - HIST_SIZE <= ptr_lim

    while (ptr > ptr_lim)
    {
        const char * present = window + current;
        const char * history = window + ptr;

        // If hash_key equal and first two characters equal --> third must be equal as well
        if ((history[0] != present[0]) ||
            (history[1] != present[1]) ||
            (history[result_len] != present[result_len]) ||
            (history[result_len - 1] != present[result_len - 1]))
        {
            ptr = prev[ptr & HIST_MASK];
            continue;
        }

        present += 2;
        history += 2;

        do {} while ((*(++history) == *(++present)) && (*(++history) == *(++present)) &&
                     (*(++history) == *(++present)) && (*(++history) == *(++present)) &&
                     (*(++history) == *(++present)) && (*(++history) == *(++present)) &&
                     (*(++history) == *(++present)) && (*(++history) == *(++present)) &&
                     (present < cutoff)); // FRAME_SIZE is a full 272 larger than DISK_TRIGGER

        const uint16_t length = static_cast<uint16_t>(present >= cutoff ? max_len : max_len - (cutoff - present));

        if (length > result_len)
        {
            result_len = length;
            result_ptr = ptr;
            if (result_len == max_len)
                break;
        }
        ptr = prev[ptr & HIST_MASK];
    }

    if ((result_len == LEN_SHIFT) && (result_ptr + TOO_FAR < current))
        return { HASH_STOP, 1 };

    return { result_ptr, result_len };
}


constexpr uint32_t update(const uint32_t key, const char next) noexcept
{
    return ((key << HASH_SHFT) ^ static_cast<uint8_t>(next)) & HASH_MASK;
}


constexpr uint32_t prepare(const char * window, const uint32_t start, const uint32_t end, uint32_t * prev, uint32_t * head) noexcept
{
    for (uint32_t cnt = 0; cnt < HIST_SIZE; ++cnt) { prev[cnt] = HASH_STOP; }
    for (uint32_t cnt = 0; cnt < HASH_SIZE; ++cnt) { head[cnt] = HASH_STOP; }

    uint32_t key = 0;
    if (start + LEN_SHIFT <= end)
    {
        key = update(0,   window[0]);
        key = update(key, window[1]);
        key = update(key, window[2]);

        for (uint32_t cnt = 0; cnt < start; ++cnt)
        {
            prev[cnt] = head[key];
            head[key] = cnt;
            key = update(key, window[cnt + 3]);
        }
    }
    return key;
}


constexpr std::pair<uint32_t, uint32_t> deflate(const char * window, uint32_t current, const uint32_t end, uint32_t * prev, uint32_t * head, uint8_t * llen_pack, uint16_t * dist_pack) noexcept
{
    uint32_t key = prepare(window, current, end, prev, head);
    uint32_t pack = 0;
    uint32_t lzss = 0;

    uint32_t now_ptr = HASH_STOP;
    uint16_t now_len = 3;
    uint32_t nxt_ptr = HASH_STOP;
    uint16_t nxt_len = 0;

    while (current < end)
    {
        if (now_len == 1)
        {
            now_len = nxt_len;
            now_ptr = nxt_ptr;
        }
        else
        {
            prev[current & HIST_MASK] = head[key];
            std::pair<uint32_t, uint16_t> now_res = match(window, current, end - current, prev); // End - current: do not peek beyond current frame!
            now_ptr = now_res.first; // Awaiting std::tie to become constexpr compatible
            now_len = now_res.second;
        }

        prev[current & HIST_MASK] = head[key];
        head[key] = current;
        key = update(key, window[current + 3]);
        ++current;
        prev[current & HIST_MASK] = head[key];
        std::pair<uint32_t, uint16_t> nxt_res = match(window, current, end - current, prev); // End - current: do not peek beyond current frame!
        nxt_ptr = nxt_res.first; // Awaiting std::tie to become constexpr compatible
        nxt_len = nxt_res.second;

        if ((now_ptr == HASH_STOP) || (nxt_len > now_len))
        {
            lzss += CHAR_BIT + 1;
            llen_pack[pack] = static_cast<uint8_t>(window[current - 1]);
            dist_pack[pack] = UINT16_MAX;
            ++pack;
            now_len = 1;
        }
        else
        {
            lzss += HIST_BITS + CHAR_BIT + 1;
            llen_pack[pack] = static_cast<uint8_t>(now_len - LEN_SHIFT);
            dist_pack[pack] = static_cast<uint16_t>(current - (1 + now_ptr + DIS_SHIFT));
            ++pack;
        }

        for (uint16_t cnt = 1; cnt < now_len; ++cnt)
        {
            prev[current & HIST_MASK] = head[key];
            head[key] = current;
            key = update(key, window[current + 3]);
            ++current;
        }
    }
    return { pack, lzss };
}


} // End of namespace lz77
} // End of namespace zseb


