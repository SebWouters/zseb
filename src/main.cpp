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

#define ZSEB_VERSION   "UNRELEASED" //"0.9.4"

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
"       zseb is a GZIP/DEFLATE implementation compatible with RFC 1951 and RFC\n"
"       1952. zseb makes use of a minimalistic Morphing Match Chain (MMC) with\n"
"       two hash chains for respectively 3- and X-character identities (X > 3).\n"
"\n"
"   ARGUMENTS\n"
"       -z, --zip=infile\n"
"              Zip infile.\n"
"\n"
"       -u, --unzip=infile\n"
"              Unzip infile.\n"
"\n"
"       -o, --output=outfile\n"
"              Output to outfile.\n"
"\n"
"       -n, --name\n"
"              Use or restore name.\n"
"\n"
"       -p, --print\n"
"              Print compression and timing.\n"
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
   bool name = false;
   bool print = false;

   struct option long_options[] =
   {
      {"zip",     required_argument, 0, 'z'},
      {"unzip",   required_argument, 0, 'u'},
      {"output",  required_argument, 0, 'o'},
      {"name",    no_argument,       0, 'n'},
      {"print",   no_argument,       0, 'p'},
      {"version", no_argument,       0, 'v'},
      {"help",    no_argument,       0, 'h'},
      {0, 0, 0, 0}
   };

   int option_index = 0;
   int c;
   while (( c = getopt_long( argc, argv, "hvz:u:o:np", long_options, &option_index )) != -1 ){
      switch( c ){
         case 'h':
         case '?':
            print_help();
            return 0;
            break;
         case 'v':
            std::cout << "zseb version " << ZSEB_VERSION << std::endl;
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
         case 'n':
            name = true;
            break;
         case 'p':
            print = true;
            break;
      }
   }

   if ( modus == 'A' ){
      std::cerr << "zseb: option -z or -u must be specified" << std::endl;
      print_help();
      return 0;
   }

   if ( ( outset == false ) && ( name == false ) ){
      std::cerr << "zseb: option -o or -n must be specified" << std::endl;
      print_help();
      return 0;
   }

   if ( modus == 'Z' ){

      if ( name ){ outfile = infile + ".gz"; }
      zseb::zseb zipper( outfile, 'Z', print );
      zipper.write_preamble( infile );
      zipper.setup_flate( infile, 'Z' );
      zipper.zip();
      zipper.set_time( outfile );

   }

   if ( modus == 'U' ){

      zseb::zseb unzipper( infile, 'U', print );
      std::string origname = unzipper.strip_preamble();
      if ( name ){ outfile = origname; }
      unzipper.setup_flate( outfile, 'U' );
      unzipper.unzip();
      unzipper.set_time( outfile );

   }

   return 0;

}

