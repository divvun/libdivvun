/*
* Copyright (C) 2015-2016, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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

#pragma once
#ifndef b2070ddb5ed4e0b7_JSON_H
#define b2070ddb5ed4e0b7_JSON_H

#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <iostream>

#include <locale>
#include <codecvt>

namespace json {

inline const std::string esc(const std::u16string& str) {
	std::vector<char16_t> os;
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	for (const char16_t& c : str) {
		if(c == '\n') {
			os.push_back('\\');
			os.push_back('n');
		}
		else {
			if((sizeof(c) == 1 || static_cast<unsigned>(c) < 256)
			   && ((int)c<20 || c == '\\' || c == '"')) {
				os.push_back('\\');
			}
			os.push_back(c);
		}
	}
	return utf16conv.to_bytes(std::u16string(os.begin(), os.end()));
}

inline const std::string str(const std::u16string& s)
{
	return "\""+esc(s)+"\"";
}

inline const std::string key(const std::u16string& s)
{
	return str(s)+":";
}

template<typename Container>
inline const std::string str_arr(const Container& ss)
{
	std::ostringstream os;
	for(const auto& s : ss) {
		os << str(s) << ",";
	}
	const auto& str = os.str();
	return "[" + str.substr(0, str.size() - 1) + "]";
}

inline void sanity_test() {
	std::string got = json::key(u"e\tr\"r\\s\nfoo");
	std::string want = "\"e\\\tr\\\"r\\\\s\\nfoo\":";
	if(got != want){
		std::cerr << "Error in json::key\n GOT: "<< got << "\nWANT: " << want << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

}
#endif
