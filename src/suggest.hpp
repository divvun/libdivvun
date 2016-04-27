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
#ifndef fe64e9a18486d375_SUGGEST_H
#define fe64e9a18486d375_SUGGEST_H


#include "util.hpp"
#include "json.hpp"

#include <hfst/HfstInputStream.h>
#include <hfst/HfstTransducer.h>

#ifdef PUGIXML_LIBS
#include <pugixml.hpp>
#endif

#include <locale>
#include <codecvt>

#include <vector>
#include <string>
#include <algorithm>
#include <regex>
#include <unordered_map>
#include <exception>

namespace gtd {

typedef std::set<std::u16string> UStringSet;
typedef std::unordered_map<std::string, std::unordered_map<std::u16string, std::u16string> > msgmap;

enum LineType {
	WordformL, ReadingL, BlankL
};

void run(std::istream& is, std::ostream& os, const hfst::HfstTransducer& t, const msgmap& m, bool json);

const hfst::HfstTransducer *readTransducer(const std::string& file);

const msgmap readMessages(const std::string& file);

}

#endif
