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

#include "checker.hpp"
#include "pipeline.hpp"
#include "version.hpp"
#include "cxxopts.hpp"

using divvun::variant;
using divvun::Pipeline;
using divvun::toUtf8;
using divvun::fromUtf8;


variant<int, Pipeline> getPipelineXml(const std::string& path, const std::u16string& pipename, bool verbose) {
	const std::unique_ptr<divvun::PipeSpec> spec(new divvun::PipeSpec(path));
	if(spec->pnodes.find(pipename) == spec->pnodes.end()) {
		std::cerr << "divvun-checker: ERROR: Couldn't find pipe " << toUtf8(pipename) << " in " << path << std::endl;
		return EXIT_FAILURE;
	}
	return Pipeline(spec, pipename, verbose);
}

variant<int, Pipeline> getPipelineAr(const std::string& path, const std::u16string& pipename, bool verbose) {
	const auto& ar_spec = divvun::readArPipeSpec(path);
	if(ar_spec->spec->pnodes.find(pipename) == ar_spec->spec->pnodes.end()) {
		std::cerr << "divvun-checker: ERROR: Couldn't find pipe " << toUtf8(pipename) << " in " << path << std::endl;
		return EXIT_FAILURE;
	}
	return Pipeline(ar_spec, pipename, verbose);
}

int printNamesXml(const std::string& path, bool verbose) {
	const std::unique_ptr<divvun::PipeSpec> spec(new divvun::PipeSpec(path));
	std::cout << "Please specify a pipeline variant with the -n/--variant option. Available variants in pipespec:" << std::endl;
	for(const auto& p : spec->pnodes) {
		const auto& name = toUtf8(p.first.c_str());
		std::cout << name << std::endl;
	}
	return EXIT_SUCCESS;
}

int printNamesAr(const std::string& path, bool verbose) {
	const auto& ar_spec = divvun::readArPipeSpec(path);
	std::cout << "Please specify a pipeline variant with the -n/--variant option. Available variants in archive:" << std::endl;
	for(const auto& p : ar_spec->spec->pnodes) {
		const auto& name = toUtf8(p.first.c_str());
		std::cout << name << std::endl;
	}
	return EXIT_SUCCESS;
}

int run(Pipeline& pipeline) {
	for (std::string line; std::getline(std::cin, line);) {
		std::stringstream pipe_in(line);
		std::stringstream pipe_out;
		pipeline.proc(pipe_in, pipe_out);
		std::cout << pipe_out.str() << std::endl;
	}
	return EXIT_SUCCESS;
}

void printPrefs(const Pipeline& pipeline) {
	using namespace divvun;
	std::cout << "== Available preferences ==" << std::endl;
	for(const auto& lp : pipeline.prefs) {
		const Lang& lang = lp.first;
		std::cout << std::endl << "=== with messages localised for '" << lang << "' ===" << std::endl;
		const Prefs& prefs = lp.second;
		std::cout << "==== Toggles: ====" << std::endl;
		for(const auto& id : prefs.toggleIds) {
			std::cout << "- [ ] " << toUtf8(id.first) << " \t" << toUtf8(id.second) << std::endl;
		}
		for(const auto& re : prefs.toggleRes) {
			std::cout << "- [ ] [regex] \t" << toUtf8(re.second) << std::endl;
		}
		std::cout << "==== Options: ====" << std::endl;
		for(const Option& o : prefs.options) {
			std::cout << "- " << o.name << " (" << o.type << "):" << std::endl;
			for(const auto& c : o.choices) {
				std::cout << "- ( ) " << toUtf8(c.first) << " \t" << toUtf8(c.second) << std::endl;
			}
		}
	}
}

int main(int argc, char ** argv)
{
	try
	{
		cxxopts::Options options(argv[0], " - generate grammar checker suggestions from a CG stream");

		options.add_options()
			("s,spec", "Pipeline XML specification", cxxopts::value<std::string>(), "FILE")
			("a,archive", "Zipped pipeline archive of language data", cxxopts::value<std::string>(), "FILE")
			("l,language", "Language to use (in case no FILE arguments given)", cxxopts::value<std::string>(), "LANG")
			("n,variant", "Name of the pipeline variant", cxxopts::value<std::string>(), "NAME")
			("I,ignore", "Comma-separated list of error tags to ignore (see -p for possible values)", cxxopts::value<std::string>(), "TAGS")
			("i,input", "Input file (UNIMPLEMENTED, stdin for now)", cxxopts::value<std::string>(), "FILE")
			("o,output", "Output file (UNIMPLEMENTED, stdout for now)", cxxopts::value<std::string>(), "FILE")
			("z,null-flush", "(Ignored, we always flush on <STREAMCMD:FLUSH>, outputting \\0 if --json).")
			("p,preferences", "Print the preferences defined by the given pipeline")
			("v,verbose", "Be verbose")
			("V,version", "Version information")
			("h,help", "Print help")
			;

		std::vector<std::string> pos = {
			// "spec",
			// "archive",
			// "variant"
			// "input",
			// "output"
		};
		options.parse_positional(pos);
		options.parse(argc, argv);

		if(argc > 1) {
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

		auto ignores = std::set<divvun::ErrId>();
		if(options.count("ignore")) {
			for(const auto& ignore : divvun::split(options["ignore"].as<std::string>(), ',')) {
				ignores.insert(fromUtf8(ignore));
			}
		}

		if(options.count("spec")) {
			if(options.count("archive") + options.count("language")) {
				std::cerr << argv[0] << " ERROR: only use one of --spec/--archive/--language" << std::endl;
			}
			const auto& specfile = options["spec"].as<std::string>();
			if(verbose) {
				std::cerr << "Reading specfile " << specfile << std::endl;
			}
			if(!options.count("variant")) {
				return printNamesXml(specfile, verbose);
			}
			const auto& pipename = fromUtf8(options["variant"].as<std::string>());
			return getPipelineXml(specfile, pipename, verbose).match(
				[]       (int r) { return r; },
				[&](Pipeline& p) {
					p.setIgnores(ignores);
					if(options.count("preferences")) {
						printPrefs(p);
					}
					else {
						run(p);
					}
					return EXIT_SUCCESS;
				});
		}
		else if(options.count("archive")) {
			if(options.count("language")) {
				std::cerr << argv[0] << " ERROR: only use one of --spec/--archive/--language" << std::endl;
			}
			const auto& archive = options["archive"].as<std::string>();
			if(verbose) {
				std::cerr << "Reading zipped archive file " << archive << std::endl;
			}
			if(!options.count("variant")) {
				return printNamesAr(archive, verbose);
			}
			const auto& pipename = fromUtf8(options["variant"].as<std::string>());
			return getPipelineAr(archive, pipename, verbose).match(
				[]       (int r) { return r; },
				[&](Pipeline& p) {
					p.setIgnores(ignores);
					if(options.count("preferences")) {
						printPrefs(p);
					}
					else {
						run(p);
					}
					return EXIT_SUCCESS;
				});
		}
		else if (options.count("language")) {
			const auto& lang = options["language"].as<std::string>();
			const auto& langs = divvun::listLangs();
			if (langs.find(lang) == langs.end() || langs.at(lang).size() < 1) {
				std::cerr << argv[0] << " ERROR: couldn't find language " << lang << " in any of the search paths:" << std::endl;
				for(const auto& p : divvun::searchPaths()) {
					std::cerr << p << std::endl;
				}
				return EXIT_FAILURE;
			}
			const auto& archive = langs.at(lang)[0]; // TODO: prioritise userdirs
			if(verbose) {
				std::cerr << "Reading zipped archive file " << archive << std::endl;
			}
			if(!options.count("variant")) {
				return printNamesAr(archive, verbose);
			}
			const auto& pipename = fromUtf8(options["variant"].as<std::string>());
			return getPipelineAr(archive, pipename, verbose).match(
				[]       (int r) { return r; },
				[&](Pipeline& p) {
					p.setIgnores(ignores);
					if(options.count("preferences")) {
						printPrefs(p);
					}
					else {
						run(p);
					}
					return EXIT_SUCCESS;
				});
		}
		else {
			std::cerr << argv[0] << " ERROR: expecting one of --spec/--archive/--language (see --help)" << std::endl;
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
