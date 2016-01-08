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
#include "cxxopts.hpp"

int main(int argc, char ** argv)
{
	try
	{
		cxxopts::Options options(argv[0], " - generate grammar checker suggestions from a CG stream");

		options.add_options()
			("j,json", "Use JSON output format (default: CG)")
			("g,generator", "Generator (HFSTOL format)", cxxopts::value<std::string>(), "BIN")
			("m,messages", "Error messages (XML format, UNIMPLEMENTED)", cxxopts::value<std::string>(), "FILE")
			("i,input", "Input file (UNIMPLEMENTED, stdin for now)", cxxopts::value<std::string>(), "FILE")
			("o,output", "Output file (UNIMPLEMENTED, stdout for now)", cxxopts::value<std::string>(), "FILE")
			("h,help", "Print help")
			;

		std::vector<std::string> pos = {
			"generator",
			//"messages"
			//"input"
			//"output"
		};
		options.parse_positional(pos);
		options.parse(argc, argv);

		if(argc > 1) {
			std::cout << options.help({""}) << std::endl;
			std::cerr << "Error: got " << argc-1+pos.size() <<" arguments; expected only " << pos.size() << std::endl;
			return(EXIT_SUCCESS);
		}

		if (options.count("help"))
		{
			std::cout << options.help({""}) << std::endl;
			return(EXIT_SUCCESS);
		}
		if (!options.count("generator"))
		{
			std::cout << options.help({""}) << std::endl;
			std::cerr <<"Error: expected generator.hfstol as argument." << std::endl;
			return(EXIT_FAILURE);
		}

		const auto& genfile = options["generator"].as<std::string>();
		bool json = options.count("j");

		if(VERBOSE) {
			std::cerr <<"Reading transducer "<<argv[optind]<<std::endl;
		}
		const hfst::HfstTransducer *t = gtd::readTransducer(genfile);
		if (t == NULL) {
			std::cout << options.help({""}) << std::endl;
			std::cerr <<"Error: Couldn't read transducer "<< genfile <<std::endl;
			return(EXIT_FAILURE);
		}
		gtd::run(std::cin, std::cout, t, json);
	}
	catch (const cxxopts::OptionException& e)
	{
		std::cout << "Error: couldn't parse options: " << e.what() << std::endl;
		return(EXIT_FAILURE);
	}
}
