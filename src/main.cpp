/*
* Copyright (C) 2015-2016, Kevin Brubeck Unhammer <unhammer@fsfe.org>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

bool VERBOSE=false;

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "suggest.hpp"

#ifndef _WIN32
#include <libgen.h>
#endif


void showHelp(char *name) {
	using namespace std;
	fprintf(stdout, "GTD Suggest\n");
	cout << basename(name) <<": generate grammar checker suggestions from a CG stream" << endl;
	cout << "USAGE: " << basename(name) << " [-j] generator.hfstol" << endl;
	cout << "Options:" << endl;
#if HAVE_GETOPT_LONG
	cout << "	-j, --json:	 output JSON format" << endl;
#else
	cout << "	-j:	 output JSON format" << endl;
#endif
}

int main(int argc, char ** argv)
{
	bool json = false;

#if HAVE_GETOPT_LONG
	static struct option long_options[] = {
		{"json",	0, 0, 'f'},
	};
#endif

	int c = 0;
	while (c != -1) {
#if HAVE_GETOPT_LONG
		int option_index;
		c = getopt_long(argc, argv, "j", long_options, &option_index);
#else
		c = getopt(argc, argv, "j");
#endif
		if (c == -1) {
			break;
		}

		switch(c) {
			case 'j':
				json = true;
				break;
			default:
				showHelp(argv[0]);
				return(EXIT_FAILURE);
				break;
		}
	}
	int real_argc = argc - optind;
	if (real_argc == 1) {
		if(VERBOSE) {
			std::cerr <<"Reading transducer "<<argv[optind]<<std::endl;
		}
		const hfst::HfstTransducer *t = gtd::readTransducer(argv[optind]);
		if (t == NULL) {
			showHelp(argv[0]);
			return(EXIT_FAILURE);
		}
		gtd::run(std::cin, std::cout, t, json);
	}
	else {
		showHelp(argv[0]);
		std::cerr <<"Expected hfstol as single arg, got:";
		for(int i=optind; i<argc; ++i) {
			std::cerr<<" " <<argv[i];
		}
		std::cerr<<std::endl;
		return(EXIT_FAILURE);
	}
}
