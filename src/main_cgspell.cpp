/*
* Copyright (C) 2015-2017, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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
#include <stdlib.h>

#include "cgspell.hpp"
#include "cxxopts.hpp"

int main(int argc, char ** argv)
{
	try
	{
		cxxopts::Options options(argv[0], "BIN - generate spelling suggestions from a CG stream");

		options.add_options()
			("a,archive", "zhfst format archive", cxxopts::value<std::string>(), "BIN")
			("l,lexicon", "Lexicon (UNIMPLEMENTED, HFSTOL format)", cxxopts::value<std::string>(), "BIN")
			("m,errmodel", "Error model (UNIMPLEMENTED, HFSTOL format)", cxxopts::value<std::string>(), "BIN")
			("n,limit", "Show at most N suggestions", cxxopts::value<int>(), "N")
			("w,max-weight", "Suppress corrections with correction weights above W", cxxopts::value<int>(), "W")
			("W,max-analysis-weight", "Suppress corrections with analysis weights above WA", cxxopts::value<int>(), "WA")
			("i,input", "Input file (UNIMPLEMENTED, stdin for now)", cxxopts::value<std::string>(), "FILE")
			("o,output", "Output file (UNIMPLEMENTED, stdout for now)", cxxopts::value<std::string>(), "FILE")
			("z,null-flush", "(Ignored, we always flush on <STREAMCMD:FLUSH>, outputting \\0 if --json).")
			("v,verbose", "Be verbose")
			("h,help", "Print help")
			;

		std::vector<std::string> pos = {
			"archive",
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
		if (!options.count("archive"))
		{
			std::cout << options.help({""}) << std::endl;
			std::cerr << "ERROR: expected archive.zhfst as argument." << std::endl;
			return(EXIT_FAILURE);
		}

		const auto& zhfstfile = options["archive"].as<std::string>();
		const auto& limit = options.count("limit") ? options["limit"].as<int>() : INT_MAX;
		const auto& max_weight = options.count("max-weight") ? options["max-weight"].as<int>() : INT_MAX;
		const auto& max_analysis_weight = options.count("max-analysis-weight") ? options["max-analysis-weight"].as<int>() : INT_MAX;
		bool verbose = options.count("v");

		if(verbose) {
			std::cerr << "Reading archive " << zhfstfile << std::endl;
		}
		std::unique_ptr<hfst_ol::ZHfstOspeller> s(new hfst_ol::ZHfstOspeller);
                s->read_zhfst(zhfstfile);
		if (!s) {
			std::cerr << "ERROR: Couldn't read transducer "<< zhfstfile << std::endl;
			return(EXIT_FAILURE);
		}

		divvun::run_cgspell(std::cin, std::cout, *s, limit, max_weight, max_analysis_weight);
	}
	catch (const cxxopts::OptionException& e)
	{
		std::cerr << "ERROR: couldn't parse options: " << e.what() << std::endl;
		return(EXIT_FAILURE);
	}
}
