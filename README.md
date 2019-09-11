zseb: Zipping Sequences of Encountered Bytes
============================================

Copyright (C) 2019 Sebastian Wouters <sebastianwouters@gmail.com>

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

Information
-----------

Work in progess.

A GZIP/DEFLATE compatible implementation (in progress).

Playing around.

TODO
----

   - Quicken up LZSS deflate (long enough match; unthorough lazy eval)
   - Handle HEAD X00 (no compress) and X01 (predefined symbols)
   - Command line interface
   - Checksums
   - GZIP header/preamble
   - Huffman to data-containing class
   - Handle the return == 2 case for Huffman::unpack
   - Ability to unzip GZIP-created files
   - Ability to zip to GZIP-processable file
   - Write documentation

Documentation
-------------

   - Gailly, **Algoritm and GZIP file format**,
     <https://git.savannah.gnu.org/cgit/gzip.git/tree/algorithm.doc>

   - Deutsch, **DEFLATE Compressed Data Format Specification**
     version 1.3 (May 1996), <https://www.ietf.org/rfc/rfc1951.txt>

   - Deutsch, **GZIP File Format Specification** version 4.3
     (May 1996), <https://www.ietf.org/rfc/rfc1952.txt>

   - Davies, **Dissecting the GZIP format** (24 April 2011),
     <http://www.infinitepartitions.com/art001.html>: comprehensive
     explanation of the DEFLATE and GZIP file formats

   - Salomon and Motta, **Handbook of Data Compression**, 5th edition,
     Springer-Verlag London (2010), section 6.25, pages 399 to 410,
     <https://doi.org/10.1007/978-1-84882-903-9>: comprehensive
     explanation of the DEFLATE and GZIP file formats

   - Hankerson, Harris and Johnson, **Introduction to Information
     Theory and Data Compression**, 2nd edition, CRC Press (2003),
     section 9.1.2, pages 235 to 236,
     <https://dl.acm.org/citation.cfm?id=601218>: settles ambiguity
     on lazy evaluation

