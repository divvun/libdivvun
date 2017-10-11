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
#ifndef a1e13de0fc0e1f37_CGSPELL_H
#define a1e13de0fc0e1f37_CGSPELL_H

#include <locale>
#include <codecvt>
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>
#include <exception>

// divvun-gramcheck:
#include "util.hpp"
// hfst:
#include <ZHfstOspeller.h>
// zhfstospeller.h conflicts with these:
// #include <hfst/HfstInputStream.h>
// #include <hfst/HfstTransducer.h>
// variants:
#include "mapbox/variant.hpp"

namespace divvun {

using mapbox::util::variant;
using std::string;
using std::vector;
using hfst_ol::Weight;

// for variants
struct Nothing
{
};

class Speller {
	public:
		Speller(const string& zhfstpath,
			bool verbose,
			Weight max_analysis_weight_,
			Weight max_weight_,
			bool real_word_,
			unsigned long limit_,
			hfst_ol::Weight beam,
			float time_cutoff)
			: max_analysis_weight(max_analysis_weight_)
			, max_weight(max_weight_)
			, real_word(real_word_)
			, limit(limit_)
			, speller(new hfst_ol::ZHfstOspeller())
		{
			speller->read_zhfst(zhfstpath);
			if (!speller) {
				throw std::runtime_error("ERROR: Couldn't read zhfst archive " + zhfstpath);
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
			hfst_ol::Weight beam,
			float time_cutoff)
			: max_analysis_weight(max_analysis_weight_)
			, max_weight(max_weight_)
			, real_word(real_word_)
			, limit(limit_)
			, speller(new hfst_ol::ZHfstOspeller())
		{
			FILE* err_fp = fopen(errpath.c_str(), "r");
			FILE* lex_fp = fopen(lexpath.c_str(), "r");
			err = std::unique_ptr<hfst_ol::Transducer> (new hfst_ol::Transducer(err_fp));
			lex = std::unique_ptr<hfst_ol::Transducer> (new hfst_ol::Transducer(lex_fp));
			// This one is freed by ZHfstOspeller, but it seems like its acceptor and errmodel are not!
			auto lmspeller = new hfst_ol::Speller(&*err, &*lex);
			speller->inject_speller(lmspeller);
			if (!speller) {
				throw std::runtime_error("ERROR: Couldn't read lexicon " + lexpath+ " / errmodel " + errpath);
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
		void spell(const string& form, std::ostream& os);
	private:
		// const void print_readings(const vector<string>& ana,
		// 			  const string& form,
		// 			  std::ostream& os,
		// 			  Weight w,
		// 			  variant<Nothing, Weight> w_a,
		// 			  const std::string& errtag) const;
		std::unique_ptr<hfst_ol::ZHfstOspeller> speller;
		const string CGSPELL_TAG = "<spelled>";
		const string CGSPELL_CORRECT_TAG = "<spell_was_correct>";
		// Only used when initialised with errpath/lexpath:
		std::unique_ptr<hfst_ol::Transducer> err;
		std::unique_ptr<hfst_ol::Transducer> lex;
};

void run_cgspell(std::istream& is,
		 std::ostream& os,
		 Speller& s);

}

#endif
