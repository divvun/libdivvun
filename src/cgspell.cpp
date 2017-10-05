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

const float WEIGHT_FACTOR = 1000.0;

const std::string CGSPELL_TAG = "<spelled>";
const std::string CGSPELL_CORRECT_TAG = "<spell_was_correct>";

const std::basic_regex<char> DELIMITERS ("^[.!?]$");

const std::basic_regex<char> CG_LINE ("^"
				      "(\"<(.*)>\".*" // wordform, group 2
				      "|(\t+)(\"[^\"]*\"\\S*)(\\s+\\S+)*" // reading, group 3, 4, 5
				      "|:(.*)" // blank, group 6
				      "|(<STREAMCMD:FLUSH>)" // flush, group 7
				      ")");


const void hacky_cg_anaformat(const std::string& ana, std::ostream& os) {
	const auto lemma_end = ana.find("+");
	if(lemma_end > 0) {
		os << "\"" << ana.substr(0, lemma_end) << "\"";
		auto from = lemma_end + 1;
		ssize_t to = ana.find('+', from);
		while(to > -1) {
			os << " " << ana.substr(from, to-from);
			from = to + 1;
			to = ana.find('+', from);
		}
		os << " " << ana.substr(from);
	}
}

void spell(const std::string& form, std::ostream& os, hfst_ol::ZHfstOspeller& s, int limit, int max_weight, int max_analysis_weight)
{
	auto correct = s.spell(form);
	if(correct) {
		// This would happen if a correct form is in the
		// speller, but not in whatever analyser you used to
		// create the input to cgspell
		auto acq = s.analyse(form);
		while(!acq.empty()) {
			const auto elt = acq.top();
			acq.pop();
			const auto a = elt.first;
			const auto w = (int)(elt.second * WEIGHT_FACTOR);
			// No max_weight for regular words
			os << "\t";
			hacky_cg_anaformat(a, os);
			os << " <W:" << w << "> " << CGSPELL_CORRECT_TAG << std::endl;
		}
	}
	else {
		auto cq = s.suggest(form);
		while(!cq.empty() && (limit--) > 0) {
			const auto& elt = cq.top();
			const auto& f = elt.first;
			const auto& w = (int)(elt.second * WEIGHT_FACTOR);
			if(w >= max_weight) {
				break;
			}
			auto aq = s.analyse(f, true);
			while(!aq.empty()) {
				const auto& elt = aq.top();
				const auto& a = elt.first;
				const auto& w_a = (int)(elt.second * WEIGHT_FACTOR);
				if(w_a >= max_analysis_weight) {
					break;
				}
				os << "\t";
				hacky_cg_anaformat(a, os);
				os << " <W:" << w << "> <WA:" << w_a << "> \"<" << f << ">\" " << CGSPELL_TAG << std::endl;
				aq.pop();
			}
			cq.pop();
		}
	}
}

void run_cgspell(std::istream& is, std::ostream& os, hfst_ol::ZHfstOspeller& s, int limit, int max_weight, int max_analysis_weight)
{
	std::string wf;
	for (std::string line;std::getline(is, line);) {
		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);

		if(!result.empty() && result[2].length() != 0) {
			os << line << std::endl;
			wf = result[2];
		}
		else if(!result.empty() && result[5].length() != 0 && result[5] == " ?") {
			os << line << std::endl;
			spell(wf, os, s, limit, max_weight, max_analysis_weight);
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
