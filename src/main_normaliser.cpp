/*
* Copyright (C) 2021 Divvun.no
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

#include "normaliser.hpp"
#include "version.hpp"
#include "cxxopts.hpp"

int main(int argc, char ** argv)
{
	try
	{
		cxxopts::Options options(argv[0],
                                 "BIN - use FSTs to normalise and expand text for TTS");

		options.add_options()
			("a,surface-analyser", "FST for surface analysis",
             cxxopts::value<std::string>(), "BIN")
			("n,normaliser", "FST for normalisation",
             cxxopts::value<std::string>(), "BIN")
			("d,deep-analyser", "FST for deep analysis (UNIMPLEMENTED)",
             cxxopts::value<std::string>(), "BIN")
			("i,input", "Input file (UNIMPLEMENTED, stdin for now)",
             cxxopts::value<std::string>(), "FILE")
			("o,output", "Output file (UNIMPLEMENTED, stdout for now)",
             cxxopts::value<std::string>(), "FILE")
			("g,generator", "FST for generations",
             cxxopts::value<std::string>(), "BIN")
			("t,tags", "limit tags to expand (UNIMPLEMENTED)",
             cxxopts::value<std::vector<std::string>>(), "TAGS")
			("v,verbose", "Be verbose")
			("V,version", "Version information")
			("h,help", "Print help")
			;

		std::vector<std::string> pos = {
			"normaliser",
			"input",
			"output"
		};
		options.parse_positional(pos);
		options.parse(argc, argv);

		if(argc > pos.size()) {
			std::cout << options.help({""}) << std::endl;
			std::cerr << argv[0] << " ERROR: got " << argc-1+pos.size() <<
              " arguments; expected only " << pos.size() << std::endl;
			return(EXIT_FAILURE);
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

		if (!options.count("normaliser"))
		{
			std::cout << options.help({""}) << std::endl;
			std::cerr << argv[0] <<
              " ERROR: expected --normaliser option" << std::endl;
			return(EXIT_FAILURE);
		}
		if (!options.count("generator"))
		{
			std::cout << options.help({""}) << std::endl;
			std::cerr << argv[0] <<
              " ERROR: expected --generator option" << std::endl;
			return(EXIT_FAILURE);
		}
		if (!options.count("surface-analyser"))
		{
			std::cout << options.help({""}) << std::endl;
			std::cerr << argv[0] <<
              " ERROR: expected --surface-analyser option." << std::endl;
			return(EXIT_FAILURE);
		}
		const auto& danalyser = options["deep-analyser"].as<std::string>();
		const auto& sanalyser = options["surface-analyser"].as<std::string>();
		const auto& normaliserfile = options["normaliser"].as<std::string>();
		const auto& generator = options["generator"].as<std::string>();
		const auto& verbose = options.count("verbose");
		const auto& tags = options["tags"].as<std::vector<std::string>>();
		auto normaliser = divvun::Normaliser(normaliserfile, generator,
                                             sanalyser, danalyser,
                                             tags, verbose);
		normaliser.run(std::cin, std::cout);
	}
	catch (const cxxopts::OptionException& e)
	{
		std::cerr << argv[0] << " ERROR: couldn't parse options: " << e.what() << std::endl;
		return(EXIT_FAILURE);
	}
    catch (FunctionNotImplementedException& fnie) {
        std::cerr << "Some needed lookup not supported, maybe automata are " <<
          "not in hfstol format." << std::endl;
    }
}
