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
	std::ostringstream os;
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	for (const char16_t& c : str) {
		if((sizeof(c) == 1 || static_cast<unsigned>(c) < 256)
		   && ((int)c<20 || c == '\\' || c == '"')) {
			os << utf16conv.to_bytes('\\');
		}
		os << utf16conv.to_bytes(c);
	}
	return os.str();
}

inline const std::string str(const std::u16string& s)
{
	return "\""+esc(s)+"\"";
}

inline const std::string key(const std::u16string& s)
{
	return str(s)+":";
}

inline const std::string str_arr(const std::vector<std::string>& ss)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	std::ostringstream os;
	os << "[";
	for(const auto& s : ss) {
		os << str(utf16conv.from_bytes(s)) << ",";
	}
	const auto& str = os.str();
	size_t end = std::min((size_t)0, // if ss is empty, otherwise skip last ',''
			      str.size() - 1);
	return str.substr(0, end) + "]";
}

inline void sanity_test() {
	std::string got = json::key(u"e\tr\"r\\s\nfoo");
	std::string want = "\"e\\\tr\\\"r\\\\s\\\nfoo\":";
	if(got != want){
		std::cerr << "Error in json::key\n GOT: "<< got << "\nWANT: " << want << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

}
#endif
