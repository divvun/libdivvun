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

#include "phon.hpp"
#include "version.hpp"
#include "cxxopts.hpp"

int main(int argc, char ** argv)
{
	try
	{
		cxxopts::Options options(argv[0],
                                 "BIN - use an FST to lookup phonetics");

		options.add_options()
			("p,text2ipa", "FST for phonetic analysis",
             cxxopts::value<std::string>(), "BIN")
			("a,alttext2ipa", "alternative FSTs for phonetic analysis per tag: TAG=ABIN",
             cxxopts::value<std::vector<std::string>>(), "BIN")
			("i,input", "Input file (UNIMPLEMENTED, stdin for now)",
             cxxopts::value<std::string>(), "FILE")
			("o,output", "Output file (UNIMPLEMENTED, stdout for now)",
             cxxopts::value<std::string>(), "FILE")
			("v,verbose", "Be verbose")
			("V,version", "Version information")
			("h,help", "Print help")
			;

		std::vector<std::string> pos = {
			"text2ipa",
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

		if (!options.count("text2ipa"))
		{
			std::cout << options.help({""}) << std::endl;
			std::cerr << argv[0] <<
              " ERROR: expected --text2ipa option" << std::endl;
			return(EXIT_FAILURE);
		}
		const auto& verbose = options.count("verbose");
        if (verbose) {
            std::cout << "Being verbose." << std::endl;
        }
		const auto& text2ipa = options["text2ipa"].as<std::string>();
		if (verbose) {
            std::cout << "Text2ipa set to: " << text2ipa << std::endl;
        }
        auto text2ipaer = divvun::Phon(text2ipa, verbose);
        if (options.count("alttext2ipa")) {
            const auto& tags2fsas =
              options["alttext2ipa"].as<std::vector<std::string>>();
            for (const auto& tag2fsa : tags2fsas) {
                auto eqpos = tag2fsa.find("=");
                if (eqpos == string::npos) {
                    std::cerr << "missing = in " << tag2fsa << std::endl;
                    return EXIT_FAILURE;
                }
                auto tag = tag2fsa.substr(0, eqpos);
                auto fsa = tag2fsa.substr(eqpos + 1);
                if (verbose) {
                    std::cout << "Alternate text2ipa for tag '" << tag <<
                      "' set to " << fsa << std::endl;
                }
                text2ipaer.addAlternateText2ipa(tag, fsa);
            }
        }
        text2ipaer.run(std::cin, std::cout);
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
