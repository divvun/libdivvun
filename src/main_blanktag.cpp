/*
* Copyright (C) 2015-2018, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "blanktag.hpp"
#include "version.hpp"
#include "cxxopts.hpp"

int main(int argc, char ** argv)
{
	try
	{
		cxxopts::Options options(argv[0], "BIN - generate spelling suggestions from a CG stream");

		options.add_options()
			("a,analyser", "FST for tagging spaces", cxxopts::value<std::string>(), "BIN")
			("i,input", "Input file (UNIMPLEMENTED, stdin for now)", cxxopts::value<std::string>(), "FILE")
			("o,output", "Output file (UNIMPLEMENTED, stdout for now)", cxxopts::value<std::string>(), "FILE")
			("z,null-flush", "(Ignored, we always flush on <STREAMCMD:FLUSH>)")
			("v,verbose", "Be verbose")
			("V,version", "Version information")
			("h,help", "Print help")
			;

		std::vector<std::string> pos = {
			"analyser",
			//"input"
			//"output"
		};
		options.parse_positional(pos);
		options.parse(argc, argv);

		if(argc > 1) {
			std::cout << options.help({""}) << std::endl;
			std::cerr << "ERROR: got " << argc-1+pos.size() <<" arguments; expected only " << pos.size() << std::endl;
			return(EXIT_SUCCESS);
		}

		if (options.count("help"))
		{
			std::cout << options.help({""}) << std::endl;
			return(EXIT_SUCCESS);
		}

		if (options.count("version"))
		{
			divvun::print_version(argv[0]);
			return(EXIT_SUCCESS);
		}

		if (!options.count("analyser"))
		{
			std::cout << options.help({""}) << std::endl;
			std::cerr << "ERROR: expected --analyser option." << std::endl;
			return(EXIT_FAILURE);
		}
		const auto& analyser = options["analyser"].as<std::string>();
		const auto& verbose = options.count("verbose");

		auto blanktagger = divvun::Blanktag(analyser, verbose);
		blanktagger.run(std::cin, std::cout);
	}
	catch (const cxxopts::OptionException& e)
	{
		std::cerr << "ERROR: couldn't parse options: " << e.what() << std::endl;
		return(EXIT_FAILURE);
	}
}
