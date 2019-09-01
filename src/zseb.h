/*
   zseb: compression
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

#ifndef __ZSEB__
#define __ZSEB__

#include <iostream>
#include <fstream>
#include <string>

#define ZSEB_HIST     32768    // 2 ** ZSEB_HIST_BIT
#define ZSEB_HIST_BIT 15
#define ZSEB_NCHR     256      // 2 ** ZSEB_NCHR_BIT; there is an alphabet of 256 characters
#define ZSEB_NCHR_BIT 8
#define ZSEB_HSH_SIZE 16777216 // 256 * 256 * 256

#define ZSEB_READ_FWD 16384
#define ZSEB_RD_SIZE  65536    // ZSEB_HIST + 2 * ZSEB_READ_FWD

#define ZSEB_HSH_STOP ( ~( ( unsigned int )( 0 ) ) )

#define ZSEB_HEAD_BIT 8
#define ZSEB_HEAD_SFT 3        // LZSS: Only if length >= 3 string is replaced
#define ZSEB_HEAD     258      // [ 0 : 2 ** ZSEB_HEAD_BIT ] + ZSEB_HEAD_SFT

namespace zseb{

/*

   http://www.cplusplus.com/doc/tutorial/files/
   https://git.savannah.gnu.org/cgit/gzip.git/tree/algorithm.doc
   https://www2.cs.duke.edu/courses/spring11/cps296.3/compression4.pdf
   https://www.ietf.org/rfc/rfc1952.txt

   Optimizations used by gzip:
      * LZSS: Output one of the following two formats:
           (0, position, length) or (1, char)
      * Uses the second format if length < 3.
      * Huffman code the positions, lengths and chars.
      * Non greedy: possibly use shorter match so that next match is better.
      * Use a hash table to store the dictionary.
         – Hash keys are all strings of length 3 in the dictionary window.
         – Find the longest match within the correct hash bucket.
         – Puts a limit on the length of the search within a bucket.
         – Within each bucket store in order of position (latest first).
      * The Hash Table:
           .. 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 ..
           .. a a c a  a  c  a  b  c  a  b  a  a  a  c  ..
           table[aac] --> 19 --> 10 --> 7
      * Max position / history window = 2^15 = 32768 char = 32 KB
      * Max length = 258 bytes ---> [0 - 255] + 3 (min. length) = [3 - 258]
      * Hash table --> max position = 2**15
                   --> pointers of max size 2**16?
                   --> Use differential pointers? / Shift each block?
                   --> table[aac] --> 19 MOD 2**16?
      * Hash table --> [ length (2**8 (char))**3 = 16777216 entries ] * 2 byte / PTR = 32 MB
      * unsigned int contains at least the [0 - 65535] range
      * Hash table --> table[a][a][c] ?

      * ini_map[aac] --> pointer_array

*/

   class zseb{

      public:

         zseb( std::string toread );

         virtual ~zseb();

      private:

         /***  File to compress  ***/

         std::ifstream infile;

         unsigned long size; // number of bytes in infile

         char * readframe; // length ZSEB_RD_SIZE; snippet from infile --> [ rd_shift : rd_shift + rd_end ]

         unsigned long rd_shift;   // Current starting point of readframe

         unsigned int  rd_end;     // Current validly filled length of readframe

         unsigned int  rd_current; // Current position within readframe

         void __readin__( const unsigned int shift_left ); // Shift readframe and fill rest

         void __write_infile_test__();

         /***  Hash table  ***/

         unsigned int * hash_last; // hash_last[a][a][c] = hash_last[ a + 256 * ( a + 256 * c ) ] = index latest encounter (w.r.t. readframe); STOP = ZSEB_HSH_STOP

         unsigned int * hash_ptrs; // hash_ptrs[ index ] = index' = index of encounter before index ( length ZSEB_RD_SIZE )


   };

}

#endif
