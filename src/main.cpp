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

#include "zseb.h"

int main(){

   zseb::zseb zipper( "tests/bible.txt", "dump.gz", 'Z' );
   zipper.write_preamble( "tests/bible.txt" );
   zipper.zip( true );

   zseb::zseb unzipper( "dump.gz", "dump.unzip.txt", 'U' );
   std::string origname = unzipper.strip_preamble();
   unzipper.unzip();

   //zseb::zseb unzipper( "dump.unzip.txt.gz", "test.out.txt", 'U' );
   //std::string origname = unzipper.strip_preamble();
   //unzipper.unzip();

   return 0;

}

