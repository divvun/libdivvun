/*
* Copyright (C) 2017-2018, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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

#include "pipespec.hpp"
#include "version.hpp"
#include "cxxopts.hpp"

int main(int argc, char ** argv)
{
	try
	{
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
		cxxopts::Options options(argv[0], "BIN - generate shell script to run checker pipeline from XML pipespec");

		options.add_options()
			("s,spec"   , "Pipeline XML specification"          , cxxopts::value<std::string>(), "FILE")
			("n,variant", "Name of the pipeline variant"        , cxxopts::value<std::string>(), "NAME")
			("d,dir"    , "Write all pipelines to directory DIR", cxxopts::value<std::string>(), "DIR")
			("j,json"   , "Make pipelines output JSON instead of CG format")
			("g,nodebug", "With -d/--dir, don't output debug / trace modes.")
			("v,verbose", "Be verbose")
			("V,version", "Version information")
			("h,help"   , "Print help")
			;

		std::vector<std::string> pos = {
			"spec",
			"variant"
		};
		options.parse_positional(pos);
		options.parse(argc, argv);

		if(argc > 2) {
			std::cout << options.help({""}) << std::endl;
			std::cerr << argv[0] << " ERROR: got " << argc-1+pos.size() <<" arguments; expected only " << pos.size() << std::endl;
			return EXIT_SUCCESS;
		}

		if (options.count("help"))
		{
			std::cout << options.help({""}) << std::endl;
			return EXIT_SUCCESS;
		}

		if (options.count("version"))
		{
			divvun::print_version(argv[0]);
			return(EXIT_SUCCESS);
		}

		bool verbose = options.count("v");
		bool json = options.count("j");

		if(options.count("spec")) {
			const auto& specfile = options["spec"].as<std::string>();
			if(verbose) {
				std::cerr << "Reading specfile " << specfile << std::endl;
			}
			if(options.count("variant")) {
				if(options.count("dir")) {
					std::cerr << argv[0] << " ERROR: Specify either --variant or --dir, not both!" << std::endl;
					return EXIT_FAILURE;
				}
				const auto& pipename = utf16conv.from_bytes(options["variant"].as<std::string>());
				divvun::writePipeSpecSh(specfile, pipename, json, std::cout);
				return EXIT_SUCCESS;
			}
			else if(options.count("dir")) {
				if(options.count("variant")) {
					std::cerr << argv[0] << " ERROR: Specify either --variant or --dir, not both!" << std::endl;
					return EXIT_FAILURE;
				}
				const auto& modesdir = options["dir"].as<std::string>();
				divvun::writePipeSpecShDir(specfile, json, modesdir, options.count("nodebug"));
				return EXIT_SUCCESS;
			}
			else {
				std::cerr << argv[0] << " ERROR: Please specify a variant (try divvun-checker to list variants) or --dir" << std::endl;
				return EXIT_FAILURE;
			}
		}
		else {
			std::cerr << argv[0] << " ERROR: Pipespec file required" << std::endl;
			return EXIT_FAILURE;
		}

	}
	catch (const cxxopts::OptionException& e)
	{
		std::cerr << argv[0] << " ERROR: couldn't parse options: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
