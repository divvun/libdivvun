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
#	include <config.h>
#endif

#include "normaliser.hpp"
#include "version.hpp"
#include "cxxopts.hpp"

int main(int argc, char** argv) {
	try {
		cxxopts::Options options(
		  argv[0], "BIN - use FSTs to normalise and expand text for TTS");

		options.add_options()("a,surface-analyser", "FST for surface analysis",
		  cxxopts::value<std::string>(),
		  "BIN")("n,normalisers", "FSTs for normalisation per tag: TAG=ABIN",
		  cxxopts::value<std::vector<std::string>>(),
		  "BIN")("d,deep-analyser", "FST for deep analysis (UNIMPLEMENTED)",
		  cxxopts::value<std::string>(),
		  "BIN")("i,input", "Input file (UNIMPLEMENTED, stdin for now)",
		  cxxopts::value<std::string>(),
		  "FILE")("o,output", "Output file (UNIMPLEMENTED, stdout for now)",
		  cxxopts::value<std::string>(), "FILE")("g,generator",
		  "FST for generations", cxxopts::value<std::string>(),
		  "BIN")("v,verbose", "Be verbose")("D,debug", "Be debugsy")(
		  "T,trace", "Be tracy")("V,version", "Version information")(
		  "h,help", "Print help");

		std::vector<std::string> pos = { "normalisers", "input", "output" };
		options.parse_positional(pos);
		options.parse(argc, argv);

		if (argc > pos.size()) {
			std::cout << options.help({ "" }) << std::endl;
			std::cerr << argv[0] << " ERROR: got " << argc - 1 + pos.size()
			          << " arguments; expected only " << pos.size()
			          << std::endl;
			return (EXIT_FAILURE);
		}

		if (options.count("help")) {
			std::cout << options.help({ "" }) << std::endl;
			return (EXIT_SUCCESS);
		}

		if (options.count("version")) {
			divvun::print_version(argv[0]);
			return (EXIT_SUCCESS);
		}

		if (!options.count("normalisers")) {
			std::cout << options.help({ "" }) << std::endl;
			std::cerr << argv[0]
			          << " ERROR: expected one or more --normalisers option"
			          << std::endl;
			return (EXIT_FAILURE);
		}
		if (!options.count("generator")) {
			std::cout << options.help({ "" }) << std::endl;
			std::cerr << argv[0] << " ERROR: expected --generator option"
			          << std::endl;
			return (EXIT_FAILURE);
		}
		if (!options.count("surface-analyser")) {
			std::cout << options.help({ "" }) << std::endl;
			std::cerr << argv[0]
			          << " ERROR: expected --surface-analyser option."
			          << std::endl;
			return (EXIT_FAILURE);
		}
		const auto& verbose = options.count("verbose");
		const auto& debug = options.count("debug");
		const auto& trace = options.count("trace");
		if (verbose) {
			std::cout << "Being verbose." << std::endl;
		}
		if (trace) {
			std::cout << "Printing traces." << std::endl;
		}
		if (debug) {
			std::cout << "Printing debugs." << std::endl;
		}
		const auto& sanalyser = options["surface-analyser"].as<std::string>();
		if (verbose) {
			std::cout << "Surface analyser set to: " << sanalyser << std::endl;
		}
		const auto& generator = options["generator"].as<std::string>();
		if (verbose) {
			std::cout << "Generator set to: " << generator << std::endl;
		}
		const auto& danalyser = options["deep-analyser"].as<std::string>();
		if (verbose) {
			std::cout << "Deep analyser set to: " << danalyser << std::endl;
		}
		auto normaliser = divvun::Normaliser(
		  generator, sanalyser, danalyser, verbose, trace, debug);
		for (const auto& tag2fsa :
		  options["normalisers"].as<std::vector<std::string>>()) {
			auto eqpos = tag2fsa.find("=");
			if (eqpos == string::npos) {
				std::cerr << "missing = in  " << tag2fsa << std::endl;
				return EXIT_FAILURE;
			}
			auto tag = tag2fsa.substr(0, eqpos);
			auto fsa = tag2fsa.substr(eqpos + 1);
			if (verbose) {
				std::cout << "Nrormaliser for tag ’" << tag << "’ set to "
				          << fsa << std::endl;
			}
			normaliser.addNormaliser(tag, fsa);
		}
		normaliser.run(std::cin, std::cout);
	}
	catch (const cxxopts::OptionException& e) {
		std::cerr << argv[0] << " ERROR: couldn't parse options: " << e.what()
		          << std::endl;
		return (EXIT_FAILURE);
	}
	catch (FunctionNotImplementedException& fnie) {
		std::cerr << "Some needed lookup not supported, maybe automata are "
		          << "not in hfstol format." << std::endl;
	}
}
