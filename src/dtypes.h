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

#include <stdint.h>
#include <limits.h>

#define ZSEB_HIST_BIT     15U
#define ZSEB_LITLEN       (1U << CHAR_BIT)

#define ZSEB_256_16T      (static_cast<uint16_t>(256U))

#define ZSEB_MASK_16T     0xffffU
#define ZSEB_MASK_32T     0xffffffffU

namespace zseb
{

enum zseb_modus
{
    undefined,
    zip,
    unzip
};

/*
void pos_diff( uint8_t left,  uint8_t right);
void pos_diff( uint8_t left, uint16_t right);
void pos_diff( uint8_t left, uint32_t right);
void pos_diff( uint8_t left, uint64_t right);
void pos_diff(uint16_t left,  uint8_t right);
// for uint16_t, uint16_t see below
void pos_diff(uint16_t left, uint32_t right);
void pos_diff(uint16_t left, uint64_t right);
void pos_diff(uint32_t left,  uint8_t right);
void pos_diff(uint32_t left, uint16_t right);
// for uint32_t, uint32_t see below
void pos_diff(uint32_t left, uint64_t right);
void pos_diff(uint64_t left,  uint8_t right);
void pos_diff(uint64_t left, uint16_t right);
void pos_diff(uint64_t left, uint32_t right);
// for uint64_t, uint64_t see below

constexpr uint16_t pos_diff(uint16_t left, uint16_t right) noexcept { return left > right ? left - right : 0; }
constexpr uint32_t pos_diff(uint32_t left, uint32_t right) noexcept { return left > right ? left - right : 0; }
constexpr uint64_t pos_diff(uint64_t left, uint64_t right) noexcept { return left > right ? left - right : 0; }
*/

} // End of namespace zseb

