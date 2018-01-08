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

#include <cstdlib>

#include "pipespec.hpp"

namespace divvun {

unique_ptr<PipeSpec> readPipeSpec(const string& file) {
	unique_ptr<PipeSpec> spec = unique_ptr<PipeSpec>(new PipeSpec);
	pugi::xml_parse_result result = spec->doc.load_file(file.c_str());
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	if (result) {
		for (pugi::xml_node pipeline: spec->doc.child("pipespec").children("pipeline")) {
			const u16string& pipename = utf16conv.from_bytes(pipeline.attribute("name").value());
			auto pr = std::make_pair(pipename, pipeline);
			spec->pnodes[pipename] = pipeline;
		}
	}
	else {
		std::cerr << file << ":" << result.offset << " ERROR: " << result.description() << "\n";
		throw std::runtime_error("ERROR: Couldn't load the pipespec xml \"" + file + "\"");
	}
	return spec;
}

string makeDebugSuff(string name, vector<string> args) {
	if(name == "cg" && args.size() > 0) {
		if(args[0].substr(0, 14) == "grammarchecker") {
			return "gc";
		}
		if(args[0].substr(0, 7) == "mwe-dis") {
			return "mwe-dis";
		}
		if(args[0].substr(0, 7) == "valency") {
			return "val";
		}
		if(args[0].substr(0, 13) == "disambiguator") {
			return "disam";
		}
	}
	if(name == "tokenize" || name == "tokenise") {
		return "morph";
	}
	if(name == "suggest") {
		return "suggest";
	}
	if(name == "mwesplit") {
		return "mwe-split";
	}
	if(name == "blanktag") {
		return "blanktag";
	}
	if(name == "cgspell") {
		return "spell";
	}
	return name;
}

std::string abspath(const std::string path) {
	// TODO: would love to use <experimental/filesystem> here, but doesn't exist on Mac :(
	// return std::experimental::filesystem::absolute(specfile).remove_filename();
        char abspath[PATH_MAX];
	realpath(path.c_str(), abspath);
	return abspath;
}

std::string pathconcat(const std::string path1, const std::string path2) {
	// TODO: would love to use <experimental/filesystem> here, but doesn't exist on Mac :(
	// return path1 / path2;
	if(path2.length() > 0 && path2[0] == '/') {
		return path2;
	}
	if(path1.length() > 0 && path1[path1.length() - 1] == '/') {
		return path1 + path2;
	}
	else {
		return path1 + "/" + path2;
	}
}

vector<std::pair<string,string>> toPipeSpecShVector(const string& dir, const pugi::xml_node& pipeline, const u16string& pipename, bool trace, bool json) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	vector<std::pair<string, string>> cmds = {};
	for (const pugi::xml_node& cmd: pipeline.children()) {
		const auto& name = std::string(cmd.name());
		string prog;
		vector<string> args;
		for (const pugi::xml_node& arg: cmd.children("arg")) {
			const auto& argn = arg.attribute("n").value();
			args.emplace_back(argn);
		}
		if(name == "tokenise" || name == "tokenize") {
			prog = "hfst-tokenise -g";
			if(json) {
				prog += " -S";
			}
		}
		else if(name == "cg") {
			prog = "vislcg3";
			if(trace) {
				prog += " --trace";
			}
			if(json) {
				prog += " --quiet";
			}
			prog += " -g";
		}
		else if(name == "cgspell") {
			prog = "divvun-cgspell -n 25";
		}
		else if(name == "mwesplit") {
			prog = "cg-mwesplit";
		}
		else if(name == "blanktag") {
			prog = "divvun-blanktag";
		}
		else if(name == "suggest") {
			prog = "divvun-suggest";
			if(json) {
				prog += " --json";
			}
		}
		else if(name == "sh") {
			prog = cmd.attribute("prog").value();
		}
		else if(name == "prefs") {
			// TODO: Do prefs make sense here?
		}
		else {
			throw std::runtime_error("Unknown command '" + name + "'");
		}
		if(!prog.empty()) {
			std::ostringstream part;
			part << prog;
			for(auto& a : args) {
				// Wrap the whole thing in single-quotes, but put existing single-quotes in double-quotes
				replaceAll(a, "'", "'\"'\"'");
				part << " '" << pathconcat(dir, a) << "'";
			}
			cmds.push_back(std::make_pair(part.str(),
						      makeDebugSuff(name, args)));
		}
	}
	return cmds;
}

void writePipeSpecSh(const string& specfile, const u16string& pipename, bool json, std::ostream& os) {
	const auto spec = readPipeSpec(specfile);
	const auto dir = dirname(abspath(specfile));
	const auto& pipeline = spec->pnodes.at(pipename);
	const auto cmds = toPipeSpecShVector(dir, pipeline, pipename, false, json);
	bool first = true;
	os << "#!/bin/sh" << std::endl << std::endl;
	for (const auto& cmd: cmds) {
		if(!first) {
			os << " \\" << std::endl << " | ";
		}
		first = false;
		os << cmd.first;
	}
}

void writePipeSpecShDirOne(const vector<std::pair<string, string>> cmds, const string& pipename, const string& modesdir, bool nodebug) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	// TODO: (modesdir / …) when we get <experimental/filesystem>
	size_t i = 0;
	if(nodebug) {
		i = cmds.size() - 1;
	}
	for (; i < cmds.size(); ++i) {
		string debug_suff = "";
		if(i < cmds.size() - 1) {
			debug_suff = std::to_string(i) + "-" + cmds[i].second;
		}
		const auto path = modesdir + "/" + pipename + debug_suff + ".mode";
		std::cout << path << std::endl;
		std::ofstream ofs;
		ofs.open(path, std::ofstream::out | std::ofstream::trunc);
		if(!ofs) {
			throw std::runtime_error("ERROR: Couldn't open " + path + " for writing! " + std::strerror(errno));
		}
		bool first = true;
		ofs << "#!/bin/sh" << std::endl << std::endl;
		for (size_t j = 0; j <= i; ++j) {
			const auto& cmd = cmds[j];
			if(!first) {
				ofs << " \\" << std::endl << " | ";
			}
			first = false;
			ofs << cmd.first;
		}
	}
}

void writePipeSpecShDir(const string& specfile, bool json, const string& modesdir, bool nodebug) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	const auto spec = readPipeSpec(specfile);
	const auto dir = dirname(abspath(specfile));
	for(const auto& p : spec->pnodes) {
		const auto& pipename = utf16conv.to_bytes(p.first);
		writePipeSpecShDirOne(toPipeSpecShVector(dir, p.second, p.first, false, json),
				      pipename,
				      modesdir,
				      nodebug);
		if(!nodebug) {
			writePipeSpecShDirOne(toPipeSpecShVector(dir, p.second, p.first, true, json),
					      "trace-" + pipename,
					      modesdir,
					      nodebug);
		}
	}
}
}
