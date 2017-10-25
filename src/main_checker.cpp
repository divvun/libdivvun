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

using divvun::variant;
using divvun::Pipeline;


variant<int, Pipeline> getPipelineXml(const std::string& path, const std::u16string& pipename, bool verbose) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	const auto& spec = divvun::readPipeSpec(path);
	if(spec->pnodes.find(pipename) == spec->pnodes.end()) {
		std::cerr << "ERROR: Couldn't find pipe " << utf16conv.to_bytes(pipename) << " in " << path << std::endl;
		return EXIT_FAILURE;
	}
	return Pipeline(spec, pipename, verbose);
}

variant<int, Pipeline> getPipelineAr(const std::string& path, const std::u16string& pipename, bool verbose) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	const auto& ar_spec = divvun::readArPipeSpec(path);
	if(ar_spec->spec->pnodes.find(pipename) == ar_spec->spec->pnodes.end()) {
		std::cerr << "ERROR: Couldn't find pipe " << utf16conv.to_bytes(pipename) << " in " << path << std::endl;
		return EXIT_FAILURE;
	}
	return Pipeline(ar_spec, pipename, verbose);
}

int printNamesXml(const std::string& path, bool verbose) {
	const auto& spec = divvun::readPipeSpec(path);
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	std::cout << "Please specify a pipeline variant with the -n/--variant option. Available variants in pipespec:" << std::endl;
	for(const auto& p : spec->pnodes) {
		const auto& name = utf16conv.to_bytes(p.first.c_str());
		std::cout << name << std::endl;
	}
	return EXIT_SUCCESS;
}

int printNamesAr(const std::string& path, bool verbose) {
	const auto& ar_spec = divvun::readArPipeSpec(path);
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	std::cout << "Please specify a pipeline variant with the -n/--variant option. Available variants in archive:" << std::endl;
	for(const auto& p : ar_spec->spec->pnodes) {
		const auto& name = utf16conv.to_bytes(p.first.c_str());
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
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	std::cout << "== Available preferences ==" << std::endl;
	for(const auto& lp : pipeline.prefs) {
		const lang& lang = lp.first;
		std::cout << std::endl << "=== with messages localised for '" << lang << "' ===" << std::endl;
		const Prefs& prefs = lp.second;
		std::cout << "==== Toggles: ====" << std::endl;
		for(const auto& id : prefs.toggleIds) {
			std::cout << "- [ ] " << utf16conv.to_bytes(id.first) << " \t" << utf16conv.to_bytes(id.second) << std::endl;
		}
		for(const auto& re : prefs.toggleRes) {
			std::cout << "- [ ] [regex] \t" << utf16conv.to_bytes(re.second) << std::endl;
		}
		std::cout << "==== Options: ====" << std::endl;
		for(const Option& o : prefs.options) {
			std::cout << "- " << o.name << " (" << o.type << "):" << std::endl;
			for(const auto& c : o.choices) {
				std::cout << "- ( ) " << utf16conv.to_bytes(c.first) << " \t" << utf16conv.to_bytes(c.second) << std::endl;
			}
		}
	}
}

int main(int argc, char ** argv)
{
	try
	{
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
		cxxopts::Options options(argv[0], " - generate grammar checker suggestions from a CG stream");

		options.add_options()
			("s,spec", "Pipeline XML specification", cxxopts::value<std::string>(), "FILE")
			("a,archive", "Zipped pipeline archive of language data", cxxopts::value<std::string>(), "FILE")
			("n,variant", "Name of the pipeline variant", cxxopts::value<std::string>(), "NAME")
			("I,ignore", "Comma-separated list of error tags to ignore (see -p for possible values)", cxxopts::value<std::string>(), "TAGS")
			("i,input", "Input file (UNIMPLEMENTED, stdin for now)", cxxopts::value<std::string>(), "FILE")
			("o,output", "Output file (UNIMPLEMENTED, stdout for now)", cxxopts::value<std::string>(), "FILE")
			("z,null-flush", "(Ignored, we always flush on <STREAMCMD:FLUSH>, outputting \\0 if --json).")
			("p,preferences", "Print the preferences defined by the given pipeline")
			("v,verbose", "Be verbose")
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
			std::cerr << "ERROR: got " << argc-1+pos.size() <<" arguments; expected only " << pos.size() << std::endl;
			return EXIT_SUCCESS;
		}

		if (options.count("help"))
		{
			std::cout << options.help({""}) << std::endl;
			return EXIT_SUCCESS;
		}
		bool verbose = options.count("v");

		auto ignores = std::set<divvun::err_id>();
		if(options.count("ignore")) {
			std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
			for(const auto& ignore : divvun::split(options["ignore"].as<std::string>(), ',')) {
				ignores.insert(utf16conv.from_bytes(ignore));
			}
		}

		if(options.count("spec")) {
			const auto& specfile = options["spec"].as<std::string>();
			if(verbose) {
				std::cerr << "Reading specfile " << specfile << std::endl;
			}
			if(!options.count("variant")) {
				return printNamesXml(specfile, verbose);
			}
			else {
				const auto& pipename = utf16conv.from_bytes(options["variant"].as<std::string>());
				return getPipelineXml(specfile, pipename, verbose).match(
					[]       (int r) { return r; },
					[options, &ignores](Pipeline& p){
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
		}
		else if(options.count("archive")) {
			const auto& archive = options["archive"].as<std::string>();
			if(verbose) {
				std::cerr << "Reading zipped archive file " << archive << std::endl;
			}
			if(!options.count("variant")) {
				return printNamesAr(archive, verbose);
			}
			else {
				const auto& pipename = utf16conv.from_bytes(options["variant"].as<std::string>());
				return getPipelineAr(archive, pipename, verbose).match(
					[]       (int r) { return r; },
					[options, &ignores](Pipeline& p){
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
