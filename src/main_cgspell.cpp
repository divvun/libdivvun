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

using hfst_ol::Weight;

int main(int argc, char ** argv)
{
	try
	{
		cxxopts::Options options(argv[0], "BIN - generate spelling suggestions from a CG stream");

		options.add_options()
			("a,archive", "zhfst format archive", cxxopts::value<std::string>(), "BIN")
			("l,lexicon", "Use this lexicon (must also give erro model as option)", cxxopts::value<std::string>(), "BIN")
			("m,errmodel", "Use this error model (must also give lexicon as option)", cxxopts::value<std::string>(), "BIN")
			("n,limit", "Suggest at most N different word forms (though each may have several analyses)", cxxopts::value<unsigned long>(), "N")
			("t,time-cutoff", "Stop trying to find better corrections after T seconds (T is a float)", cxxopts::value<float>(), "T")
			("w,max-weight", "Suppress corrections with correction weight above W", cxxopts::value<Weight>(), "W")
			("W,max-analysis-weight", "Suppress corrections with analysis weight above WA", cxxopts::value<Weight>(), "WA")
			("b,beam", "Suppress corrections worse than best candidate by more than W (W is a float)", cxxopts::value<Weight>(), "W")
			("X,real-word", "Also suggest corrections to correct words")
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

		const auto& verbose = options.count("verbose");
		const auto& max_analysis_weight = options.count("max-analysis-weight") ? options["max-analysis-weight"].as<Weight>() : -1.0;
		const auto& max_weight = options.count("max-weight") ? options["max-weight"].as<Weight>() : -1.0;
		const auto& real_word = options.count("real-word");
		const auto& limit = options.count("limit") ? options["limit"].as<unsigned long>() : ULONG_MAX;
		const auto& beam = options.count("beam") ? options["beam"].as<Weight>() : -1.0;
		const auto& time_cutoff = options.count("time-cutoff") ? options["time-cutoff"].as<float>() : 0.0;

		if ((options.count("archive") && (options.count("lexicon") || options.count("errmodel")))
		    ||
		    !(options.count("archive") || (options.count("lexicon") && options.count("errmodel"))))
		{
			std::cout << options.help({""}) << std::endl;
			std::cerr << "ERROR: expected either --archive or both --lexicon and --errmodel options." << std::endl;
			return(EXIT_FAILURE);
		}

		if (options.count("archive")) {
			if (options.count("lexicon") || options.count("errmodel")) {
				std::cout << options.help({""}) << std::endl;
				std::cerr << "ERROR: expected either --archive or both --lexicon and --errmodel options." << std::endl;
				return(EXIT_FAILURE);
			}
			const auto& zhfstfile = options["archive"].as<std::string>();
			auto speller = divvun::Speller(zhfstfile, verbose,
						       max_analysis_weight, max_weight, real_word, limit, beam, time_cutoff);
			divvun::run_cgspell(std::cin, std::cout, speller);
		}
		else if (options.count("lexicon") || options.count("errmodel")) {
			if  (options.count("archive")) {
				std::cout << options.help({""}) << std::endl;
				std::cerr << "ERROR: expected either both --lexicon and --errmodel or --archive options." << std::endl;
				return(EXIT_FAILURE);
			}
			const auto& errfile = options["errmodel"].as<std::string>();
			const auto& lexfile = options["lexicon"].as<std::string>();
			auto speller = divvun::Speller(errfile, lexfile, verbose,
						       max_analysis_weight, max_weight, real_word, limit, beam, time_cutoff);
			divvun::run_cgspell(std::cin, std::cout, speller);
		}
	}
	catch (const cxxopts::OptionException& e)
	{
		std::cerr << "ERROR: couldn't parse options: " << e.what() << std::endl;
		return(EXIT_FAILURE);
	}
}
