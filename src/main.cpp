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

#include <hfst/HfstInputStream.h>
#include <hfst/HfstTransducer.h>

#include <vector>
#include <string>

namespace gtd {

const hfst::HfstTransducer *readTransducer(const std::string& file) {
	hfst::HfstInputStream *in = NULL;
	try
	{
		in = new hfst::HfstInputStream(file);
	}
	catch (StreamNotReadableException e)
	{
		std::cerr << "ERROR: File does not exist." << std::endl;
		exit(1);
	}

	hfst::HfstTransducer* t = NULL;
	while (not in->is_eof())
	{
		if (in->is_bad())
		{
			std::cerr << "ERROR: Stream cannot be read." << std::endl;
			exit(1);
		}
		t = new hfst::HfstTransducer(*in);
		if(not in->is_eof()) {
			std::cerr << "WARNING: >1 transducers in stream! Only using the first." << std::endl;
		}
		break;
	}
	in->close();
	delete in;
	if(t == NULL) {
		std::cerr << "WARNING: Could not read any transducers!" << std::endl;
	}
	return t;
}

}


int main(int argc, char ** argv)
{
	if (argc != 2) {
		std::cerr <<"Expected hfstol as single arg, got:";
		for(int i=1; i<argc; ++i) {
			std::cerr<<" " <<argv[i];
		}
		std::cerr<<std::endl;
		return(EXIT_FAILURE);
	}
	if(VERBOSE) {
		std::cerr <<"Reading transducer "<<argv[1]<<std::endl;
	}
	const hfst::HfstTransducer *t = gtd::readTransducer(argv[1]);
	gtd::run(std::cin, std::cout, t);
}
