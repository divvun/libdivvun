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
#pragma once
#ifndef ec895525de7e16bc_CHECKERTYPES_H
#define ec895525de7e16bc_CHECKERTYPES_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <set>
#include <unordered_map>
#include <regex>

namespace divvun {

/**
 * Public types for divvun-gramcheck library
 */

using lang = std::string;
using msg = std::u16string;
using err_id = std::u16string;
using err_re = std::basic_regex<char>;

struct Err {
		std::u16string form;
		size_t beg;
		size_t end;
		err_id err;
		std::u16string msg;
		std::set<std::u16string> rep;
};

struct Option {
		std::string type;
		std::string name;
		std::unordered_map<err_id, msg> choices;      // choices[errtype] = msg;
};
struct OptionCompare {
    bool operator() (const Option& a, const Option& b) const {
        return a.name < b.name;
    }
};

/**
 * Radio-button choices (e.g. Oxford comma vs no-Oxford comma)
 */
using OptionSet = std::set<Option, OptionCompare>;

/**
 * Checkbox choices, ie. hiding certain error types
 */
using ToggleIds = std::unordered_map<err_id, msg>;      // toggleIds[errtype] = msg;
using ToggleRes = std::vector<std::pair<err_re, msg> >; // toggleRes = [(errtype_regex, msg), …];

struct Prefs {
		ToggleIds toggleIds;
		ToggleRes toggleRes;
		OptionSet options;
};
using LocalisedPrefs = std::unordered_map<lang, Prefs>;

} // namespace divvun

#endif
