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

A GZIP/DEFLATE compatible implementation, according to RFC 1951 and RFC
1952 (work in progress).

The algorithm makes use of a minimalistic Morphing Match Chain (MMC).
In the code, multiple hash chains are used:

   - **hash_prv3** is a hash chain of 3-character identities; and
   - **hash_prv4** is a hash chain of 4-character identities.

As in gzip, **hash_prv3** is initially followed. Meanwhile, **hash_prv4** is
updated correspondigly, until a chain is picked up on **hash_prv4**. From
then on, **hash_prv4** is followed instead of **hash_prv3** and **hash_prv4** no
longer needs to be updated. In ASCII art:

    abcd1abce2abcf3abcg4abch5abci6abcj7abck8abcl9abcg0abcd1abce2abcf3abcg4abch5abci6abcj7abck8abcl9abcg0
                   |                             |                   |    |    |    |    |    |    |
                   |                             |                   |    |    |    |    |    |    |
                   |                             |                   <----<----<----<----<----<----|
                   |                             |                   ptr6 ptr5 ptr4 ptr3 ptr2 ptr1 i    using hash_prv3
                   |                             |                   |
    ---------------<-----------------------------<--------------------
                   ptr8                          ptr7                                                   using hash_prv4

TODO
----

   - Add Calgary corpus
   - Quicken up LZSS deflate (long enough match; unthorough lazy eval)
   - Figure out why 'gzip --best' compresses to a smaller size
   - Move file opening out of LZSS (to allow for name in header)
   - Write documentation
   - dtypes.h: Ensure zseb_32_t is 32-bit in stead of assert()
   - Fixed Huffman trees hardcoded?

Documentation
-------------

   - Gailly, **Algorithm and GZIP file format**,
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

   - Yann Collet, **MMC - Morphing Match Chain**, RealTime Data
     Compression, Development blog on compression algorithms,
     <https://fastcompression.blogspot.com/p/mmc-morphing-match-chain.html>

