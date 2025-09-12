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
#	include <config.h>
#endif

#include "suggest.hpp"
#include "version.hpp"
#include "cxxopts.hpp"

int main(int argc, char** argv) {
	try {
		cxxopts::Options options(argv[0],
		  "BIN - generate grammar checker suggestions from a CG stream");

		options.add_options()(
		  "j,json", "Use JSON output format (default: CG)")("a,autocorrect",
		  "Use Autocorrect output format (default: CG)")("g,generator",
		  "Generator (HFSTOL format)", cxxopts::value<std::string>(), "BIN")
#ifdef HAVE_LIBPUGIXML
		  ("m,messages", "ERROR messages (XML format)",
		    cxxopts::value<std::string>(), "FILE")
#else
		  ("m,messages",
		    "ERROR messages not supported (recompile with pugixml to use "
		    "those)",
		    cxxopts::value<std::string>(), "FILE")
#endif
		    ("l,lang", "Locale / xml:lang to use for messages",
		      cxxopts::value<std::string>(),
		      "LANG")("i,input", "Input file (UNIMPLEMENTED, stdin for now)",
		      cxxopts::value<std::string>(), "FILE")("o,output",
		      "Output file (UNIMPLEMENTED, stdout for now)",
		      cxxopts::value<std::string>(), "FILE")("z,null-flush",
		      "(Ignored, we always flush on <STREAMCMD:FLUSH>, outputting \\0 "
		      "if --json).")("A,generate-all",
		      "Run generator on every reading")("v,verbose", "Be verbose")(
		      "V,version", "Version information")("h,help", "Print help");

		std::vector<std::string> pos = {
			"generator",
#ifdef HAVE_LIBPUGIXML
			"messages"
#endif
			//"input"
			//"output"
		};
		options.parse_positional(pos);
		options.parse(argc, argv);

		if (argc > 1) {
			std::cout << options.help({ "" }) << std::endl;
			std::cerr << argv[0] << " ERROR: got " << argc - 1 + pos.size()
			          << " arguments; expected only " << pos.size()
			          << std::endl;
			return (EXIT_SUCCESS);
		}

		if (options.count("help")) {
			std::cout << options.help({ "" }) << std::endl;
			return (EXIT_SUCCESS);
		}

		if (options.count("version")) {
			divvun::print_version(argv[0]);
			return (EXIT_SUCCESS);
		}

		if (!options.count("generator")) {
			std::cout << options.help({ "" }) << std::endl;
			std::cerr << argv[0]
			          << " ERROR: expected generator.hfstol as argument."
			          << std::endl;
			return (EXIT_FAILURE);
		}

		const auto& genfile = options["generator"].as<std::string>();
		divvun::RunMode mode = divvun::RunCG;
		if (options.count("j")) {
			mode = divvun::RunJson;
			if (options.count("a")) {
				std::cerr << argv[0]
				          << " ERROR: Pick just one of --json/--autocorrect"
				          << std::endl;
				return (EXIT_FAILURE);
			}
		};
		if (options.count("a")) {
			mode = divvun::RunAutoCorrect;
		};
		bool genall = options.count("A");
		bool verbose = options.count("v");

		const auto& locale =
		  options.count("lang") ? options["lang"].as<std::string>() : "se";

		if (verbose) {
			std::cerr << "Reading transducer " << genfile << std::endl;
		}
		const hfst::HfstTransducer* t(divvun::readTransducer(genfile));
		if (!t) {
			std::cerr << argv[0] << " ERROR: Couldn't read transducer "
			          << genfile << std::endl;
			return (EXIT_FAILURE);
		}

		divvun::MsgMap m;
#ifdef HAVE_LIBPUGIXML
		if (options.count("messages")) {
			const auto& msgfile = options["messages"].as<std::string>();
			if (verbose) {
				std::cerr << "Reading messages xml " << msgfile << std::endl;
			}
			m = divvun::Suggest::readMessages(msgfile);
			if (m.empty()) {
				std::cerr << argv[0] << " ERROR: Couldn't read messages xml "
				          << msgfile << std::endl;
				return (EXIT_FAILURE);
			}
		}
		else {
			std::cerr << argv[0]
			          << " WARNING: no errors.xml argument; tags used as "
			             "error messages."
			          << std::endl;
		}
#else
		if (options.count("messages")) {
			std::cerr << argv[0]
			          << " ERROR: Please clean-recompile with pugixml to use "
			             "-m/--messages xml"
			          << std::endl;
			return (EXIT_FAILURE);
		}
#endif

		divvun::Suggest s(t, m, locale, verbose, genall);
		s.run(std::cin, std::cout, mode);
	}
	catch (const cxxopts::OptionException& e) {
		std::cerr << argv[0] << " ERROR: couldn't parse options: " << e.what()
		          << std::endl;
		return (EXIT_FAILURE);
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
