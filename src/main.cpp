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

#define ZSEB_VERSION   "UNRELEASED" //"0.9.6"

#include <getopt.h>
#include "zseb.h"

void print_help(){

std::cout << "\n"
"zseb: Zipping Sequences of Encountered Bytes\n"
"Copyright (C) 2019, 2020 Sebastian Wouters\n"
"\n"
"Usage: zseb [OPTIONS]\n"
"\n"
"    INFO\n"
"        zseb is a GZIP/DEFLATE implementation compatible with\n"
"        RFC 1951 and RFC 1952.\n"
"\n"
"    ARGUMENTS\n"
"        -z, --zip=infile\n"
"                Zip infile.\n"
"\n"
"        -u, --unzip=infile\n"
"                Unzip infile.\n"
"\n"
"        -o, --output=outfile\n"
"                Output to outfile.\n"
"\n"
"        -n, --name\n"
"                Use or restore name.\n"
"\n"
"        -p, --print\n"
"                Print compression and timing.\n"
"\n"
"        -v, --version\n"
"                Print the version.\n"
"\n"
"        -h, --help\n"
"                Display this help.\n"
"\n"
" " << std::endl;

}

int main(int argc, char ** argv)
{
    std::string infile;
    zseb::zseb_modus modus = zseb::zseb_modus::undefined;
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
    while ((c = getopt_long(argc, argv, "hvz:u:o:np", long_options, &option_index)) != -1)
    {
        switch(c)
        {
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
                modus = zseb::zseb_modus::zip;
                break;
            case 'u':
                infile = optarg;
                modus = zseb::zseb_modus::unzip;
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

    if (modus == zseb::zseb_modus::undefined)
    {
        std::cerr << "zseb: option -z or -u must be specified" << std::endl;
        print_help();
        return 0;
    }

    if ((!outset) && (!name))
    {
        std::cerr << "zseb: option -o or -n must be specified" << std::endl;
        print_help();
        return 0;
    }

    if (modus == zseb::zseb_modus::zip)
    {
        if (name){ outfile = infile + ".gz"; }
        zseb::tools::zip(/*flate, zipfile,*/infile, outfile, print);
    }

    if (modus == zseb::zseb_modus::unzip)
    {
        zseb::tools::unzip(/*flate, zipfile,*/infile, outfile, name, print);

    }

    return 0;
}


