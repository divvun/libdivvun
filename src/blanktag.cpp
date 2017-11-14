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

#include "blanktag.hpp"

namespace divvun {

const std::basic_regex<char> CG_LINE ("^"
				      "(\"<(.*)>\".*" // wordform, group 2
				      "|(\t+)(\"[^\"]*\"\\S*)(\\s+\\S+)*" // reading, group 3, 4, 5
				      "|:(.*)" // blank, group 6
				      "|(<STREAMCMD:FLUSH>)" // flush, group 7
				      ")");

/**
 * Return the size in bytes of the first complete UTF-8 codepoint in c,
 * or 0 if invalid.
 */
size_t u8_first_codepoint_size(const unsigned char* c) {
    if (*c <= 127) {
        return 1;
    }
    else if ( (*c & (128 + 64 + 32 + 16)) == (128 + 64 + 32 + 16) ) {
        return 4;
    }
    else if ( (*c & (128 + 64 + 32 )) == (128 + 64 + 32) ) {
        return 3;
    }
    else if ( (*c & (128 + 64 )) == (128 + 64)) {
        return 2;
    }
    else {
        return 0;
    }
}

bool is_cg_tag(const string & str) {
    // Note: invalid codepoints are also treated as tags;  ¯\_(ツ)_/¯
    return str.size() > u8_first_codepoint_size((const unsigned char*)str.c_str());
}

Blanktagger::Blanktagger(const string& analyser_, bool verbose)
	: analyser(readTransducer(analyser_))
{

}

const string Blanktagger::proc(const vector<string>& preblank, const string& wf, const vector<string>& postblank, const vector<string>& readings) {
	string ret;
	for(const auto& b : preblank) {
		ret += ":" + b + "\n";
	}
	if(wf.empty()) {
		return ret;
	}
	// std::cerr << "\033[1;36mpreblank + wf + postblank=\t'" << join(preblank,"") + wf + join(postblank, "") << "'\033[0m" << std::endl;
	const auto& paths = analyser->lookup_fd({ join(preblank,"") + wf + join(postblank, "") }, -1, 2.0);
	string tags;
	for(auto& p : *paths) {
		std::stringstream form;
		for(auto& symbol : p.second) {
			if(!hfst::FdOperation::is_diacritic(symbol)) {
				form << symbol;
			}
		}
		tags += " " + form.str();
	}
	ret += wf + "\n";
	for(const auto& r: readings) {
		ret += r + tags + "\n";
	}
	return ret;
}

const void Blanktagger::run(std::istream& is, std::ostream& os)
{
	vector<string> preblank;
	vector<string> postblank;
	string wf;
	vector<string> readings;
	for (string line; std::getline(is, line);) {
		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);
		// std::cerr << "\033[1;32mline=\t'" << line << "'\033[0m\t"
			  // << "\033[1;33mpreblank=\t'" << join(preblank,"⮒") << "'\033[0m\t"
			  // << "\033[1;35mpostblank=\t'" <<join(postblank,"⮒") << "'\033[0m" << std::endl;
		if (!result.empty() && result[2].length() != 0) {
			os << proc(preblank, wf, postblank, readings);
			preblank.swap(postblank);
			wf = result[1];
			readings = {};
			postblank = {};
		}
		else if (!result.empty() && result[3].length() != 0) {
			readings.push_back(line);
		}
		else if(!result.empty() && result[7].length() != 0) {
			// TODO: Can we ever get a flush in the middle of readings?
			os << proc(preblank, wf, postblank, readings);
			os.flush();
			preblank.swap(postblank);
			wf = "";
			readings = {};
			postblank = {};
		}
		else if(!result.empty() && result[6].length() != 0) {
			postblank.push_back(result[6]);
		}
		else {
			// don't match on blanks not prefixed by ':'
			os << line;
		}
	}
	os << proc(preblank, wf, postblank, readings);
	preblank.swap(postblank);
	wf = "";
	readings = {};
	postblank = {};
	os << proc(preblank, wf, postblank, readings);
}

}