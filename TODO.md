Todo
----

   - Quicken up LZSS deflate (long enough match; unthorough lazy eval)
   - Figure out why 'gzip --best' compresses to a smaller size --> ? GZIP huffman encodes llen_pack & dist_pack blocks of 32767
   - Write documentation
   - Fixed Huffman trees hardcoded?
   - Why is the sys time so large? (gzip quasi zero)
   - Build tree in __prefix_lengths__
   - Seems like zseb_64_t for hash_head requires long time... (many cycles)

