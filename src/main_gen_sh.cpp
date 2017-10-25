/*
* Copyright (C) 2017, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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

#include "pipeline.hpp"
#include "cxxopts.hpp"

int main(int argc, char ** argv)
{
	try
	{
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
		cxxopts::Options options(argv[0], "BIN - generate shell script to run checker pipeline from XML pipespec");

		options.add_options()
			("s,spec", "Pipeline XML specification", cxxopts::value<std::string>(), "FILE")
			("n,variant", "Name of the pipeline variant", cxxopts::value<std::string>(), "NAME")
			("v,verbose", "Be verbose")
			("h,help", "Print help")
			;

		std::vector<std::string> pos = {
			"spec",
			"variant"
		};
		options.parse_positional(pos);
		options.parse(argc, argv);

		if(argc > 2) {
			std::cout << options.help({""}) << std::endl;
			std::cerr << "ERROR: got " << argc-1+pos.size() <<" arguments; expected only " << pos.size() << std::endl;
			return EXIT_SUCCESS;
		}

		if (options.count("help"))
		{
			std::cout << options.help({""}) << std::endl;
			return EXIT_SUCCESS;
		}
		bool verbose = options.count("v");

		if(options.count("spec")) {
			const auto& specfile = options["spec"].as<std::string>();
			if(verbose) {
				std::cerr << "Reading specfile " << specfile << std::endl;
			}
			if(!options.count("variant")) {
				std::cerr << "ERROR: Please specify a variant (try divvun-checker to list variants)" << std::endl;
				return EXIT_FAILURE;
			}
			else {
				const auto& pipename = utf16conv.from_bytes(options["variant"].as<std::string>());
				divvun::writePipeSpecSh(specfile, pipename, std::cout);
				return EXIT_SUCCESS;
			}
		}
		else {
			std::cerr << "ERROR: Pipespec file required" << std::endl;
			return EXIT_FAILURE;
		}

	}
	catch (const cxxopts::OptionException& e)
	{
		std::cerr << "ERROR: couldn't parse options: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
