/*
* Copyright (C) 2015-2018, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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
#	define fe64e9a18486d375_SUGGEST_H

#	include <algorithm>
#	include <exception>
#	include <locale>
#	include <regex>
#	include <string>
#	include <unordered_map>
#	include <vector>

// divvun-gramcheck:
#	include "util.hpp"
#	include "hfst_util.hpp"
#	include "json.hpp"
#	include "checkertypes.hpp"
// xml:
#	ifdef HAVE_LIBPUGIXML
#		include <pugixml.hpp>
#	endif
// hfst:
#	include <hfst/HfstInputStream.h>
#	include <hfst/HfstTransducer.h>
// variants:
#	include "mapbox/variant.hpp"

namespace divvun {

using mapbox::util::variant;
using std::pair;
using std::string;
using std::stringstream;
using std::u16string;
using std::vector;

using UStringVector = vector<u16string>;
using StringVector = vector<string>;

using MsgMap = std::unordered_map<Lang,
  pair<ToggleIds, ToggleRes>>; // msgs[Lang] = make_pair(ToggleIds, ToggleRes)
using SortedMsgLangs = vector<Lang>; // sorted with preferred language first

#	ifdef HAVE_LIBPUGIXML
inline string xml_raw_cdata(const pugi::xml_node& label) {
	std::ostringstream os;
	for (const auto& cc : label.children()) {
		cc.print(os, "", pugi::format_raw);
	}
	return os.str();
}
#	endif

enum RunState { Flushing, Eof };

enum RunMode { RunCG, RunJson, RunAutoCorrect };

using rel_id = size_t;
using relations = std::multimap<string, rel_id>; // CG can have multiple R:LEFT etc.

inline void dedupe(relations& rels) {
	std::set<std::pair<std::string, int>> seen;
	for (auto it = rels.begin(); it != rels.end(); ) {
		if (seen.insert({it->first, it->second}).second) {
			++it;
		} else {
			it = rels.erase(it);
		}
	}
}

enum Added { NotAdded, AddedAfterBlank, AddedBeforeBlank };

enum Casing { lower, Title, UPPER, mIxed };
inline Casing getCasing(string input) {
	if (input.length() < 1) {
		return mIxed;
	}
	std::locale loc;
	bool seenupper = false;
	bool seenlower = false;
	bool fstupper = false;
	bool nonfstupper = false;
	for (const auto& c : wideFromUtf8(input)) {
		if (isupper(c, loc)) {
			if (seenlower || seenupper) {
				nonfstupper = true;
			}
			else {
				fstupper = true;
			}
			seenupper = true;
		}
		if (islower(c, loc)) {
			seenlower = true;
		}
	}
	if (!seenupper) {
		return lower;
	}
	if (!seenlower) {
		return UPPER;
	}
	if (fstupper && !nonfstupper) {
		return Title;
	}
	else {
		return mIxed;
	}
}

// #ifndef icu TODO
//
// Does lots of probably unnecessary encoding-mangling too, but at
// least it works, if the user locale is OK.
inline std::string toupper(const string& input) {
	std::wstring w = wideFromUtf8(input);
	setlocale(LC_ALL, "");
	std::transform(w.begin(), w.end(), w.begin(), std::towupper);
	return wideToUtf8(w);
}

inline std::string totitle(const string& input) {
	std::wstring w = wideFromUtf8(input);
	setlocale(LC_ALL, "");
	std::transform(w.begin(), w.begin() + 1, w.begin(), std::towupper);
	return wideToUtf8(w);
}
// #endif

inline std::string withCasing(
  bool fixedcase, const Casing& inputCasing, const string& input) {
	if (fixedcase) {
		return input;
	}
	switch (inputCasing) {
	case Title:
		return totitle(input);
	case UPPER:
		return toupper(input);
	case mIxed:
		return input;
	case lower:
		return input;
	}
	// should never get to this point
	return input;
}

const string clean_blank(const string& raw);

struct Reading {
	bool suggest = false;
	string ana; // for generating suggestions from this reading
	std::set<u16string> errtypes; // the error tag(s) (without leading ampersand)
	std::set<u16string> coerrtypes; // the COERROR error tag(s) (without leading ampersand)
	StringVector sforms;
	relations rels; // rels[relname] = target.id
	rel_id id = 0;  // id is 0 if unset, otherwise the relation id of this word
	string wf;	// tag of type "wordform"S for use with SUGGESTWF
	bool suggestwf = false;
	bool coerror = false; // cohorts that are not the "core" of the underline never become Err's; message template offsets refer to the cohort of the Err
	Added added = NotAdded;
	bool fixedcase = false; // don't change casing on suggestions if we have this tag
	string line;	// The (unchanged) input lines which created this Reading
};

struct Cohort {
	u16string form;
	size_t pos;		// position in text
	rel_id id;		// CG relation id
	vector<Reading> readings;
	std::set<u16string> errtypes;   // the error tag(s) of all readings (without leading ampersand)
	std::set<u16string> coerrtypes; // the COERROR error tag(s) of all readings (without leading ampersand)
	Added added;
	string raw_pre_blank; // blank before cohort, in CG stream format (initial colon, brackets, escaped newlines)
	vector<Err> errs;
	string trace_removed_readings; // lines prefixed with `;` by `vislcg3 -t`
};

using CohortMap = std::unordered_map<rel_id, size_t>;

struct Sentence {
	vector<Cohort> cohorts;
	CohortMap ids_cohorts;	// mapping from cohort relation id's to their position in Sentence.cohort vector
	// TODO: can we make this an encoded stringstream? would avoid a lot of from/to_bytes calls
	// std::basic_ostringstream<char16_t> text;
	std::ostringstream text;
	RunState runstate;
	string raw_final_blank; // blank after last cohort, in CG stream format (initial colon, brackets, escaped newlines)
	vector<Err> errs;
};

enum FlushOn { Nul, NulAndDelimiters };

// Default value for Suggest.delimiters:
inline const std::set<u16string> defaultDelimiters() {
	return {
		u".",
		u"?",
		u"!"
	};
}

class Suggest {
public:
	Suggest(const hfst::HfstTransducer* generator, divvun::MsgMap msgs,
	  const string& locale, bool verbose, bool genall);
	Suggest(const string& gen_path, const string& msg_path,
	  const string& locale, bool verbose, bool generate_all_readings);
	Suggest(const string& gen_path, const string& locale, bool verbose);
	~Suggest() = default;

	void run(std::istream& is, std::ostream& os, RunMode mode);

	vector<Err> run_errs(std::istream& is);
	void setIgnores(const std::set<ErrId>& ignores);
	void setIncludes(const std::set<ErrId>& includes);

	static const MsgMap readMessages(const string& file);
	static const MsgMap readMessages(const char* buff, const size_t size);

	const MsgMap msgs;
	const string locale;

private:
	const SortedMsgLangs sortedmsglangs; // invariant: contains all and only the keys of msgs
	RunState run_json(std::istream& is, std::ostream& os);
	RunState run_autocorrect(std::istream& is, std::ostream& os);
	RunState run_cg(std::istream& is, std::ostream& os);
	Sentence run_sentence(std::istream& is, FlushOn flush_on);
	std::unique_ptr<const hfst::HfstTransducer> generator;
	std::set<ErrId> ignores;
	std::set<ErrId> includes;
	std::set<u16string> delimiters; // run_sentence(NulAndDelimiters) will return after seeing a cohort with one of these forms
	size_t hard_limit = 500;	// run_sentence(NulAndDelimiters) will always flush after seeing this many cohorts
	bool generate_all_readings = false;
	bool verbose = false;

	/**
	  * For a single cohort, if it has errors, creates the
	  * user-readable Msg (in the preferred language,
	  * inserting the right word forms in the template),
	  * and finds the indices of the error underline, as
	  * well as the form of that substring, and expands the
	  * error replacements until they cover the beg/end
	  * part of the text.
          */
	variant<Nothing, Err> cohort_errs(const ErrId& ErrId, size_t i_c,
	  const Cohort& c, const Sentence& sentence, const u16string& text);

	// This alters the Cohort's of the Sentence by filling the `errs` vector.
	void mk_errs(Sentence& sentence);
};
}

#endif
