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

#include "cgspell.hpp"

namespace divvun {

const std::basic_regex<char> CG_LINE ("^"
				      "(\"<(.*)>\".*" // wordform, group 2
				      "|(\t+)(\"[^\"]*\"\\S*)(\\s+\\S+)*" // reading, group 3, 4, 5
				      "|:(.*)" // blank, group 6
				      "|(<STREAMCMD:FLUSH>)" // flush, group 7
				      ")");

static const string subreading_separator = "#";
static const string unknown_analysis = " ?";

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

void print_cg_subreading(size_t indent,
			 const string& form,
			 const vector<string>::const_iterator beg,
                         const vector<string>::const_iterator end,
                         std::ostream & os,
			 FactoredWeight w,
			 variant<Nothing, FactoredWeight> mw_a,
			 const std::string& errtag)
{
	os << string(indent, '\t');
	bool in_lemma = false;
	for(vector<string>::const_iterator it = beg; it != end; ++it) {
		bool is_tag = is_cg_tag(*it);
		if(in_lemma) {
			if(is_tag) {
				in_lemma = false;
				os << "\"";
			}
		}
		else {
			if(!is_tag) {
				in_lemma = true;
				os << "\"";
			}
		}
		os << (*it);
	}
	if(in_lemma) {
		os << "\"";
	}
	if(indent == 1) {
		os << " <W:" << w << ">";
		mw_a.match([]      (Nothing) {},
			   [&os] (FactoredWeight w_a) { os << " <WA:" << w_a; });
		os << " " << errtag;
		os << " \"<" << form << ">\"";
	}
	os << std::endl;
}

const void print_readings(const vector<string>& ana,
				   const string& form,
				   std::ostream& os,
				   FactoredWeight w,
				   variant<Nothing, FactoredWeight> w_a,
				   const std::string& errtag)
{
	size_t indent = 1;
	vector<string>::const_iterator beg = ana.begin(), end = ana.end();
	while(true) {
		bool sub_found = false;
		for(auto it = end-1; it > ana.begin(); --it) {
			if(subreading_separator.compare(*it) == 0) {
				// Found a sub-reading mark
				beg = ++it;
				sub_found = true;
				break;
			}
		}
		if(!sub_found) {
			// No remaining sub-marks to the left
			beg = ana.begin();
		}
		print_cg_subreading(indent,
				    form,
				    beg,
				    end,
				    os,
				    w,
				    w_a,
				    errtag);
		if(beg == ana.begin()) {
			break;
		}
		else {
			++indent;
			end = beg;
			if(sub_found) {
				--end; // skip the subreading separator symbol
			}
		}
	}
}

void Speller::spell(const string& inform, std::ostream& os)
{
	bool do_suggest = real_word || !speller->spell(inform);
	if(!do_suggest) {
		// This would happen if a correct inform is in the
		// speller, but not in whatever analyser you used to
		// create the input to cgspell
		auto aq = speller->analyseSymbols(inform);
		while(!aq.empty()) {
			const auto ana = aq.top().first;
			const auto w = (FactoredWeight)(aq.top().second * weight_factor);
			// No max_weight for regular words
			print_readings(ana, inform, os, w, Nothing(), CGSPELL_CORRECT_TAG);
			aq.pop();
		}
	}
	else {
		auto cq = speller->suggest(inform);
		auto slimit = limit;
		while(!cq.empty() && (slimit--) > 0) {
			const auto& corrform = cq.top().first;
			const auto& w = (FactoredWeight)(cq.top().second * weight_factor);
			if(max_weight > 0.0 && w >= max_weight) {
				break;
			}
			auto aq = speller->analyseSymbols(corrform, true);
			while(!aq.empty()) {
				const auto& ana = aq.top().first;
				const auto& w_a = (FactoredWeight)(aq.top().second * weight_factor);
				if(max_analysis_weight > 0.0 && w_a >= max_analysis_weight) {
					break;
				}
				print_readings(ana, corrform, os, w, w_a, CGSPELL_TAG);
				aq.pop();
			}
			cq.pop();
		}
	}
}

void run_cgspell(std::istream& is,
		 std::ostream& os,
		 Speller& s)
{
	string wf;
	for (string line; std::getline(is, line);) {
		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);

		if(!result.empty() && result[2].length() != 0) {
			os << line << std::endl;
			wf = result[2];
		}
		else if(!result.empty() && result[5].length() != 0 && (s.real_word
								       || result[5] == unknown_analysis)) {
			os << line << std::endl;
			s.spell(wf, os);
		}
		else if(!result.empty() && result[7].length() != 0) {
			// TODO: Can we ever get a flush in the middle of readings?
			os.flush();
			os << line << std::endl;
		}
		else {
			os << line << std::endl;
		}
	}
}

}
