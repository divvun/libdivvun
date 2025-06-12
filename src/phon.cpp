/*
* Copyright (C) 2021, Divvun
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

#include "phon.hpp"

namespace divvun {

Phon::Phon(const hfst::HfstTransducer* text2ipa_, bool verbose_, bool trace_)
  : text2ipa(text2ipa_)
  , altText2ipas()
  , verbose(verbose_)
  , trace(trace_) {
	if (verbose_) {
		std::cout << "Constructed text2ipa thing from HFST Transducer"
		          << std::endl;
	}
}

Phon::Phon(const std::string& text2ipa_, bool verbose_, bool trace_) {
	if (verbose_) {
		std::cout << "Reading: " << text2ipa_ << std::endl;
	}
	text2ipa =
	  std::unique_ptr<const hfst::HfstTransducer>((readTransducer(text2ipa_)));
	verbose = verbose_;
	trace = trace_;
}

void Phon::addAlternateText2ipa(
  const std::string& tag, const hfst::HfstTransducer* text2ipa_) {
	if (verbose) {
		std::cout << "adding HFST transducer for tag " << tag << std::endl;
	}
	altText2ipas[tag] = std::unique_ptr<const hfst::HfstTransducer>(text2ipa_);
}

void Phon::addAlternateText2ipa(
  const std::string& tag, const std::string& text2ipa_) {
	if (verbose) {
		std::cout << "Reading " << text2ipa_ << " for tag " << tag
		          << std::endl;
	}
	altText2ipas[tag] =
	  std::unique_ptr<const hfst::HfstTransducer>(readTransducer(text2ipa_));
	assert(altText2ipas[tag]);
}


void Phon::run(std::istream& is, std::ostream& os) {
	assert(text2ipa);
	string surf;
	for (string line; std::getline(is, line);) {
		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);
		// 0, 1 all
		// 2: surf
		// 4: lemma
		// 5: syntag
		//
		//
		if ((!result.empty()) && (result[2].length() != 0)) {
			if (verbose) {
				std::cout << "New surface form: " << result[2] << std::endl;
			}
			surf = result[2];
			os << result[0] << std::endl;
		}
		else if ((!result.empty()) && (result[4].length() != 0)) {
			string traces;
			auto phon = surf;
			string outstring = string(result[0]);
			// try find existing ",,,"phon tag or a alt surf. "<>"
			auto phonend = outstring.find("\"phon");
			auto phonstart = phonend;
			auto midtend = outstring.find("\"MIDTAPE");
			auto midtstart = phonend;
			auto altsurfstart = outstring.find("\"<", 3);
			auto altsurfend = outstring.find(">\"", 3);
			if (phonstart != std::string::npos) {
				phonstart = outstring.rfind("\"", phonend - 1);
				phon =
				  outstring.substr(phonstart + 1, phonend - phonstart - 1);
				outstring = outstring.replace(phonstart, phonend, "");
				if (verbose) {
					std::cout << "Using Phon: " << phon << std::endl;
				}
			}
			else if ((altsurfstart != std::string::npos) &&
			         (altsurfend != std::string::npos)) {
				phon = outstring.substr(
				  altsurfstart + 2, altsurfend - altsurfstart - 2);
				if (verbose) {
					std::cout << "Using re-analysed surface form: " << phon
					          << std::endl;
				}
			}
			else if (midtstart != std::string::npos) {
				midtstart = outstring.rfind("\"", midtend - 1);
				phon =
				  outstring.substr(midtstart + 1, midtend - midtstart - 1);
				outstring = outstring.replace(midtstart, midtend, "");
				if (verbose) {
					std::cout << "Using MIDTAPE: " << phon << std::endl;
				}
			}
			else if (verbose) {
				phon = surf;
				std::cout << "Using surf: " << phon << std::endl;
			}
			std::string alttag = "";
			// check if specific alt tag
			for (const auto& tag2fsa : altText2ipas) {
				if (outstring.find(tag2fsa.first) != std::string::npos) {
					alttag = tag2fsa.first;
					if (verbose) {
						std::cout << "Using alt " << tag2fsa.first
						          << std::endl;
					}
				}
			}
			// apply text2ipa
			if (verbose) {
				std::cout << "looking up text2ipa: " << phon << std::endl;
			}
			const hfst::HfstOneLevelPaths* expansions = nullptr;
			if (alttag.empty()) {
				expansions = text2ipa->lookup_fd(phon, -1, 2.0);
				if (trace) {
					traces = " DIVVUN-PHON:TEXT2IPA";
				}
			}
			else {
				expansions = altText2ipas[alttag]->lookup_fd(phon, -1, 2.0);
				if (trace) {
					traces = " DIVVUN-PHON:ALT:" + alttag;
				}
			}
			if (expansions->empty()) {
				if (verbose) {
					std::cout << "text2ipa results empty." << std::endl;
				}
				if (trace) {
					traces += " DIVVUN-PHON:?";
				}
				os << result[0] << traces << std::endl;
			}
			std::string oldform;
			for (auto& e : *expansions) {
				std::stringstream form;
				for (auto& symbol : e.second) {
					if (!hfst::FdOperation::is_diacritic(symbol)) {
						form << symbol;
					}
				}
				std::string newphon = form.str();
				if (!oldform.empty()) {
					if (oldform == newphon) {
						std::cerr << "Warn: ambiguous but identical "
						          << oldform << ", " << newphon << std::endl;
					}
					else {
						std::cerr << "Error: ambiguous ipa " << oldform << ", "
						          << newphon << std::endl;
					}
				}
				oldform = newphon;
			}
			if (!oldform.empty()) {
				phon = oldform;
			}
			os << outstring << " \"" << phon << "\"phon" << traces
			   << std::endl;
		}
		else {
			if (verbose) {
				if (result[0].str()[0] == ';') {
					std::cout << "Skipping traced removed CG line:"
					          << std::endl;
				}
				else if (result[0].str()[0] == ':') {
					std::cout << "Skipping superblanks:" << std::endl;
				}
				else if (result[0].str() == "") {
					std::cout << "Blanks:" << std::endl;
				}
				else {
					std::cout << "Probably not cg formatted stuff: "
					          << std::endl;
				}
			}
			os << result[0] << std::endl;
		}
	}
}

} // namespace divvun
