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

#pragma once
#ifndef fe64e9a18486d375_SUGGEST_H
#define fe64e9a18486d375_SUGGEST_H

#include <locale>
#include <codecvt>
#include <vector>
#include <string>
#include <algorithm>
#include <regex>
#include <unordered_map>
#include <exception>

// divvun-gramcheck:
#include "util.hpp"
#include "json.hpp"
#include "checkertypes.hpp"
// xml:
#ifdef HAVE_LIBPUGIXML
#include <pugixml.hpp>
#endif
// hfst:
#include <hfst/HfstInputStream.h>
#include <hfst/HfstTransducer.h>
// variants:
#include "mapbox/variant.hpp"

namespace divvun {

using mapbox::util::variant;

typedef std::set<std::u16string> UStringSet;

typedef std::unordered_map<lang, std::pair<ToggleIds, ToggleRes> > msgmap;	// msgs[lang] = make_pair(ToggleIds, ToggleRes)

typedef size_t rel_id;
typedef std::unordered_map<std::string, rel_id> relations;

inline std::string xml_raw_cdata(const pugi::xml_node& label) {
	std::ostringstream os;
	for(const auto& cc: label.children())
	{
		cc.print(os, "", pugi::format_raw);
	}
	return os.str();
}

struct Reading {
	bool suggest = false;
	std::string ana;
	std::u16string errtype;
	UStringSet sforms;
	relations rels;	// rels[relname] = target.id
	rel_id id = 0;
};

struct Cohort {
	std::u16string form;
	std::map<std::u16string, UStringSet> err;
	size_t pos;
	rel_id id;
	std::vector<Reading> readings;
};

typedef std::unordered_map<rel_id, size_t> CohortMap;

enum RunState {
	flushing,
	eof
};

struct Sentence {
	std::vector<Cohort> cohorts;
	CohortMap ids_cohorts;
	std::ostringstream text;
	RunState runstate;
};

enum LineType {
	WordformL, ReadingL, BlankL
};

// for variants
struct Nothing
{
};

std::vector<Err> run_errs(std::istream& is, const hfst::HfstTransducer& t, const msgmap& msgs);

void run(std::istream& is, std::ostream& os, const hfst::HfstTransducer& t, const msgmap& m, bool json);

const hfst::HfstTransducer *readTransducer(const std::string& file);
const hfst::HfstTransducer *readTransducer(const char* buff, const size_t size);
const hfst::HfstTransducer *readTransducer(std::istream& is);

const msgmap readMessages(const std::string& file);
const msgmap readMessages(const char* buff, const size_t size);

}

#endif
