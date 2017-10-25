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


// Simple string convenience functions


#pragma once
#ifndef a309fa88d5af43e9_UTIL_H
#define a309fa88d5af43e9_UTIL_H


#include <sstream>
#include <iterator>

#include <vector>
#include <set>
#include <string>
#include <algorithm>

#include <locale>
#include <codecvt>

namespace divvun {

using StringVec = std::vector<std::string>;

template<typename Container>
inline const std::string join_quoted(const Container& ss, const std::string& delim=" ") {
	std::ostringstream os;
	std::for_each(ss.begin(), ss.end(), [&](const std::string& s){ os << "\"" << s << "\","; });
	const auto& str = os.str();
	return str.substr(0,
			  str.size() - delim.size());
}

template<typename Container>
inline const std::string join(const Container& ss, const std::string& delim=" ") {
	std::ostringstream os;
	std::copy(ss.begin(), ss.end(), std::ostream_iterator<std::string>(os, delim.c_str()));
	const auto& str = os.str();
	return str.substr(0,
			  str.size() - delim.size());
}

template<typename Container>
inline const std::string u16join(const Container& ss, const std::string& delim=" ") {
	std::ostringstream os;
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	for(const auto& s : ss) {
		os << utf16conv.to_bytes(s) << ",";
	}
	const auto& str = os.str();
	return str.substr(0,
			  str.size() - delim.size());
}

inline const StringVec split(const std::string& str, const char& delim=' ')
{
	std::string buf;
	std::stringstream ss(str);
	StringVec tokens;
	while (std::getline(ss, buf, delim)) {
		if(!buf.empty()) {
			tokens.push_back(buf);
		}
	}
	return tokens;
}

// TODO: This fails on macos with
// In file included from /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../include/c++/v1/sstream:175:
// /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../include/c++/v1/istream:295:26: error: implicit instantiation of undefined template 'std::__1::ctype<char16_t>'
// if (!__ct.is(__ct.space, *__i))
//
// inline const std::vector<std::u16string> split(const std::u16string& str, const char16_t& delim=' ')
// {
// 	std::u16string buf;
// 	std::basic_stringstream<char16_t> ss(str);
// 	std::vector<std::u16string> tokens;
// 	while (std::getline(ss, buf, delim)) {
// 		if(!buf.empty()) {
// 			tokens.push_back(buf);
// 		}
// 	}
// 	return tokens;
// }

inline int startswith(std::string big, std::string start)
{
	return big.compare(0, start.size(), start) == 0;
}

inline void replaceAll(std::u16string& str, const std::u16string& from, const std::u16string& to) {
    if(from.empty()){
        return;
    }
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::u16string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

inline void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty()){
        return;
    }
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

template<typename string_t>
string_t dirname(const string_t& path)
{
	string_t s = string_t(path);
	if (s.size() <= 1) {
		return s;
	}
	if (*(s.rbegin() + 1) == '/') {
		s.pop_back();
	}
	s.erase(std::find(s.rbegin(), s.rend(), '/').base(), s.end());
	return s;
}

// for variants
struct Nothing
{
};


}

#endif
