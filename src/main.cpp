/*
   zseb: Zipping Sequences of Encountered Bytes
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

#include <getopt.h>

#include "zseb.h"

void print_help(){

std::cout << "\n"
"zseb: Zipping Sequences of Encountered Bytes\n"
"Copyright (C) 2019 Sebastian Wouters\n"
"\n"
"Usage: zseb [OPTIONS]\n"
"\n"
"   INFO\n"
"       ZSEB is a GZIP/DEFLATE implementation\n"
"       compatible with RFC 1951 and RFC 1952.\n"
"\n"
"       The algorithm makes use of a minimalistic Morphing\n"
"       Match Chain (MMC). In the code, multiple hash chains\n"
"       are used:\n"
"         - hash_prv3 for 3-character identities; and\n"
"         - hash_prv4 for 4-character identities.\n"
"       As in gzip, hash_prv3 is initially followed. Meanwhile,\n"
"       hash_prv4 is updated correspondigly, until a chain is\n"
"       picked up on hash_prv4. From then on, hash_prv4 is\n"
"       followed instead of hash_prv3 and hash_prv4 no\n"
"       longer needs to be updated.\n"
"\n"
"       In ASCII art:\n"
"\n"
"       abcd1abce2abcf3abcg4abch5abci6abcj7abck8abcl9abcg0abcd1abce2abcf3abcg4abch5abci6abcj7abck8abcl9abcg0\n"
"                      |                             |                   |    |    |    |    |    |    |\n"
"                      |                             |                   |    |    |    |    |    |    |\n"
"                      |                             |                   <----<----<----<----<----<----|\n"
"                      |                             |                   ptr6 ptr5 ptr4 ptr3 ptr2 ptr1 i    using hash_prv3\n"
"                      |                             |                   |\n"
"       ---------------<-----------------------------<--------------------\n"
"                      ptr8                          ptr7                                                   using hash_prv4\n"
"\n"
"   ARGUMENTS\n"
"       -Z, --zip=infile\n"
"              Zip infile.\n"
"\n"
"       -U, --unzip=infile\n"
"              Unzip infile.\n"
"\n"
"       -O, --output=outfile\n"
"              Output file.\n"
"\n"
"       -v, --version\n"
"              Print the version.\n"
"\n"
"       -h, --help\n"
"              Display this help.\n"
"\n"
" " << std::endl;

}

int main( int argc, char ** argv ){

   std::string infile;
   char modus = 'A';
   std::string outfile;
   bool outset = false;

   struct option long_options[] =
   {
      {"zip",     required_argument, 0, 'z'},
      {"unzip",   required_argument, 0, 'u'},
      {"output",  required_argument, 0, 'o'},
      {"version", no_argument,       0, 'v'},
      {"help",    no_argument,       0, 'h'},
      {0, 0, 0, 0}
   };

   int option_index = 0;
   int c;
   while (( c = getopt_long( argc, argv, "hvz:u:o:", long_options, &option_index )) != -1 ){
      switch( c ){
         case 'h':
         case '?':
            print_help();
            return 0;
            break;
         case 'v':
            std::cout << "zseb version UNRELEASED" << std::endl;
            return 0;
            break;
         case 'z':
            infile = optarg;
            modus = 'Z';
            break;
         case 'u':
            infile = optarg;
            modus = 'U';
            break;
         case 'o':
            outfile = optarg;
            outset = true;
            break;
      }
   }

   if ( modus == 'A' ){
      print_help();
      return 0;
   }

   if ( outset == false ){

      // Find last '/'
      std::size_t prev = std::string::npos;
      std::size_t curr = infile.find( '/', 0 );
      while ( curr != std::string::npos ){
         prev = curr;
         curr = infile.find( '/', prev + 1 );
      }
      outfile = ( ( prev == std::string::npos ) ? infile : infile.substr( prev + 1, std::string::npos ) );

      // Find last '.'
      prev = std::string::npos;
      curr = outfile.find( '.', 0 );
      while ( curr != std::string::npos ){
         prev = curr;
         curr = outfile.find( '.', prev + 1 );
      }
      outfile = ( ( prev == std::string::npos ) ? outfile : outfile.substr( 0, prev ) );

      if ( modus == 'Z' ){ outfile += ".gz"; }

   }

   if ( modus == 'Z' ){

      zseb::zseb zipper( infile, outfile, 'Z' );
      zipper.write_preamble( infile );
      zipper.zip( false );

   }

   if ( modus == 'U' ){

      zseb::zseb unzipper( infile, outfile, 'U' );
      std::string origname = unzipper.strip_preamble();
      unzipper.unzip();

   }

   return 0;

}


