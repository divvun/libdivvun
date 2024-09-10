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
#ifndef a1e13de0fc0e1f37_CGSPELL_H
#define a1e13de0fc0e1f37_CGSPELL_H

#include <locale>
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>
#include <exception>

// divvun-gramcheck:
#include "util.hpp"
// hfst:
#include <ZHfstOspeller.h>
// variants:
#include <variant>

namespace divvun {

using std::variant;
using std::string;
using std::vector;
using std::pair;
using hfst_ospell::Weight;

struct SpellCohort {
	string wf;
	vector<string> lines;
	vector<string> postblank;
	bool unknown;
};
struct SpellSent {
	vector<SpellCohort> cohorts;
	int n_unknowns;
};

class Speller {
	public:
		Speller(const string& zhfstpath,
			bool verbose,
			Weight max_analysis_weight_,
			Weight max_weight_,
			bool real_word_,
			unsigned long limit_,
			hfst_ospell::Weight beam,
			float time_cutoff,
			float max_sent_unknown_rate_)
			: max_analysis_weight(max_analysis_weight_)
			, max_weight(max_weight_)
			, real_word(real_word_)
			, limit(limit_)
			, max_sent_unknown_rate(max_sent_unknown_rate_)
			, speller(new hfst_ospell::ZHfstOspeller())
		{
			speller->read_zhfst(zhfstpath);
			if (!speller) {
				throw std::runtime_error("libdivvun: ERROR: Couldn't read zhfst archive " + zhfstpath);
			}
			else {
				speller->set_beam(beam);
				speller->set_time_cutoff(time_cutoff);
				// s.set_queue_limit(limit); // TODO: This seems to choose first three, not top three (same with /usr/bin/hfst-ospell)
				// s.set_weight_limit(max_weight); // TODO: Has no effect? (same with /usr/bin/hfst-ospell)
			}
		}
		Speller(const string& errpath,
			const string& lexpath,
			bool verbose,
			Weight max_analysis_weight_,
			Weight max_weight_,
			bool real_word_,
			unsigned long limit_,
			hfst_ospell::Weight beam,
			float time_cutoff,
			float max_sent_unknown_rate_)
			: max_analysis_weight(max_analysis_weight_)
			, max_weight(max_weight_)
			, real_word(real_word_)
			, limit(limit_)
			, max_sent_unknown_rate(max_sent_unknown_rate_)
			, speller(new hfst_ospell::ZHfstOspeller())
		{
			FILE* err_fp = fopen(errpath.c_str(), "r");
            if (err_fp == nullptr) {
                throw std::runtime_error("libdivvun: ERROR: Couldn't read error model " + errpath);
            }
			FILE* lex_fp = fopen(lexpath.c_str(), "r");
            if (lex_fp == NULL) {
                throw std::runtime_error("libdivvun: ERROR: Couldn't read language model " + lexpath);
            }
			err = std::unique_ptr<hfst_ospell::Transducer> (new hfst_ospell::Transducer(err_fp));
			lex = std::unique_ptr<hfst_ospell::Transducer> (new hfst_ospell::Transducer(lex_fp));
			// This one is freed by ZHfstOspeller, but it seems like its acceptor and errmodel are not!
			auto lmspeller = new hfst_ospell::Speller(&*err, &*lex);
			speller->inject_speller(lmspeller);
			if (!speller) {
				throw std::runtime_error("libdivvun: ERROR: Couldn't read lexicon " + lexpath+ " / errmodel " + errpath);
			}
			else {
				speller->set_beam(beam);
				speller->set_time_cutoff(time_cutoff);
				// s.set_queue_limit(limit); // TODO: This seems to choose first three, not top three (same with /usr/bin/hfst-ospell)
				// s.set_weight_limit(max_weight); // TODO: Has no effect? (same with /usr/bin/hfst-ospell)
			}
		}
		Speller(hfst_ospell::Transducer* err_,
			hfst_ospell::Transducer* lex_,
			bool verbose,
			Weight max_analysis_weight_,
			Weight max_weight_,
			bool real_word_,
			unsigned long limit_,
			hfst_ospell::Weight beam,
 			float time_cutoff,
			float max_sent_unknown_rate_)
			: max_analysis_weight(max_analysis_weight_)
			, max_weight(max_weight_)
			, real_word(real_word_)
			, limit(limit_)
			, max_sent_unknown_rate(max_sent_unknown_rate_)
			, speller(new hfst_ospell::ZHfstOspeller())
			, err(err_)
			, lex(lex_)
		{
			// This one is freed by ZHfstOspeller, but it seems like its acceptor and errmodel are not!
			auto lmspeller = new hfst_ospell::Speller(&*err, &*lex);
			speller->inject_speller(lmspeller);
			if (!speller) {
				throw std::runtime_error("libdivvun: ERROR: Couldn't read lexicon / errmodel");
			}
			else {
				speller->set_beam(beam);
				speller->set_time_cutoff(time_cutoff);
				// s.set_queue_limit(limit); // TODO: This seems to choose first three, not top three (same with /usr/bin/hfst-ospell)
				// s.set_weight_limit(max_weight); // TODO: Has no effect? (same with /usr/bin/hfst-ospell)
			}
		}
		const Weight max_analysis_weight;
		const Weight max_weight;
		const bool real_word;
		const unsigned long limit;
		// TODO: Make max_sent_unknown_rate and sent_delimiters configurable in cli?
		float max_sent_unknown_rate = 0.4; // Don't spell if >= 40 % of the sentence is unknown.
		float min_sent_max_unknown = 7; // For sentences of < 7 cohorts, spell even if most of it is unknown.
		std::basic_regex<char> sent_delimiters = std::basic_regex<char> ("^[.!?]$");
		void spell(const string& form, std::ostream& os);
		bool analyse_when_correct = false; // Look up the analysis for forms that had an analysis in lex already.
	private:
		// const void print_readings(const vector<string>& ana,
		// 			  const string& form,
		// 			  std::ostream& os,
		// 			  Weight w,
		// 			  variant<Nothing, Weight> w_a,
		// 			  const std::string& errtag) const;
		std::unique_ptr<hfst_ospell::ZHfstOspeller> speller;
		const string CGSPELL_TAG = "<spelled>";
		const string CGSPELL_CORRECT_TAG = "<spell_was_correct>";
		// Only used when initialised with errpath/lexpath:
		std::unique_ptr<hfst_ospell::Transducer> err;
		std::unique_ptr<hfst_ospell::Transducer> lex;
		// A cache of misspelt words, with suggestions. For server use, where texts are
		// requested over and over again with very little change, this makes the UI a lot
		// snappier.
		std::unordered_map<string, string> cache;
		// TODO: tweak cache max (currently a drop in the ocean compared to what libhfstospell already uses)
		size_t cache_max = 10000;
};

void run_cgspell(std::istream& is,
		 std::ostream& os,
		 Speller& s);

}

#endif
