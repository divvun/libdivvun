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

#include "normaliser.hpp"

namespace divvun {

Normaliser::Normaliser(const hfst::HfstTransducer* normaliser_,
  const hfst::HfstTransducer* generator_,
  const hfst::HfstTransducer* sanalyser_,
  const hfst::HfstTransducer* danalyser_, const vector<string>& tags_,
  bool verbose_, bool trace_, bool debug_)
  : normaliser(normaliser_)
  , generator(generator_)
  , sanalyser(sanalyser_)
  , danalyser(danalyser_)
  , tags(tags_)
  , verbose(verbose_)
  , trace(trace_)
  , debug(debug_) {}

Normaliser::Normaliser(const string& normaliser_, const string& generator_,
  const string& sanalyser_, const string& danalyser_,
  const vector<string>& tags_, bool verbose_, bool trace_, bool debug_) {
	debug = debug_;
	verbose = verbose_;
	trace = trace_;
	if (verbose_) {
		std::cout << "Reading files: " << std::endl;
		std::cout << "* " << normaliser_ << std::endl;
		if (trace_) {
			std::cout << "Printing traces" << std::endl;
		}
		if (debug_) {
			std::cout << "Printing debugs" << std::endl;
		}
	}
	if (normaliser_ != "") {
		normaliser = std::unique_ptr<const hfst::HfstTransducer>(
		  (readTransducer(normaliser_)));
	}
	if (verbose_) {
		std::cout << "* " << generator_ << std::endl;
	}
	if (generator_ != "") {
		generator = std::unique_ptr<const hfst::HfstTransducer>(
		  (readTransducer(generator_)));
	}
	if (verbose_) {
		std::cout << "* " << sanalyser_ << std::endl;
	}
	if (sanalyser_ != "") {
		sanalyser = std::unique_ptr<const hfst::HfstTransducer>(
		  (readTransducer(sanalyser_)));
	}
	if (verbose_) {
		std::cout << "* " << danalyser_ << std::endl;
	}
	if (danalyser_ != "") {
		danalyser = std::unique_ptr<const hfst::HfstTransducer>(
		  (readTransducer(danalyser_)));
	}
	if (verbose_) {
		std::cout << "expanding tags: ";
		for (auto tag : tags_) {
			std::cout << tag << " ";
		}
		std::cout << std::endl;
	}
	tags = tags_;
	verbose = verbose_;
}


void Normaliser::run(std::istream& is, std::ostream& os) {
	string surf;
	for (string line; std::getline(is, line);) {
		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);
		// 0, 1 all
		// 2: surf
		// 4: lemma
		// 5: syntag
		//
		if ((!result.empty()) && (result[2].length() != 0)) {
			if (debug) {
				std::cout << "New surface form: " << result[2] << std::endl;
			}
			surf = result[2];
			os << result[0] << std::endl;
		}
		else if ((!result.empty()) && (result[4].length() != 0)) {
			string outstring = string(result[0]);
			auto tabstart = outstring.find("\t");
			auto tabend = outstring.find("\"");
			auto tabs = outstring.substr(tabstart, tabend);
			string lemma = result[4];
			bool everythinghasfailed = true;
			if (tags.empty()) {
				everythinghasfailed = false;
				os << outstring << std::endl;
			}
			bool expand = false;
			for (auto tag : tags) {
				if (outstring.find(tag) != std::string::npos) {
					if (debug) {
						std::cout << "Expanding because of " << tag
						          << std::endl;
					}
					expand = true;
				}
			}
			// try find existing ",,,"phon tag or a alt surf. "<>"
			auto phonend = outstring.find("\"phon");
			auto phonstart = phonend;
			//auto midtend = outstring.find("\"MIDTAPE");
			//auto midtstart = phonend;
			auto altsurfstart = outstring.find("\"<", 3);
			auto altsurfend = outstring.find(">\"", 3);
			if ((altsurfstart != std::string::npos) &&
			    (altsurfend != std::string::npos)) {
				surf = outstring.substr(
				  altsurfstart + 2, altsurfend - altsurfstart - 2);
				if (debug) {
					std::cout << "Using re-analysed surface form: " << surf
					          << std::endl;
				}
			}
			else if (phonstart != std::string::npos) {
				phonstart = outstring.rfind("\"", phonend - 1);
				surf =
				  outstring.substr(phonstart + 1, phonend - phonstart - 1);
				outstring = outstring.replace(phonstart, phonend, "");
				if (debug) {
					std::cout << "Using Phon(?): " << surf << std::endl;
				}
			}
			/*else if (midtstart != std::string::npos) {
				midtstart = outstring.rfind("\"", midtend - 1);
				surf =
				  outstring.substr(midtstart + 1, midtend - midtstart - 1);
				outstring = outstring.replace(midtstart, midtend, "");
				if (verbose) {
					std::cout << "Using MIDTAPE: " << surf << std::endl;
				}
			}*/
			else {
				surf = lemma.substr(1, lemma.length() - 2);
				if (debug) {
					std::cout << "Using lemma: " << surf << std::endl;
				}
			}
			if (expand) {
				// 1. apply expansions from normaliser
				if (debug) {
					std::cout << "1. looking up normaliser" << std::endl;
				}
				const HfstPaths1L expansions(
				  normaliser->lookup_fd(surf, -1, 2.0));
				if (expansions->empty()) {
					if (debug) {
						std::cout << "Normaliser results empty." << std::endl;
					}
					os << result[0] << std::endl;
				}
				for (auto& e : *expansions) {
					std::stringstream form;
					for (auto& symbol : e.second) {
						if (!hfst::FdOperation::is_diacritic(symbol)) {
							form << symbol;
						}
					}
					std::string phon = form.str();
					std::string newlemma = form.str();
					std::string reanal = result[5].str();
					// 2. generate specific form with new lemma
					std::string regen = form.str();
					std::string regentags = "";
					if (debug) {
						std::cout << "2.a Using normalised form: " << regen
						          << std::endl;
					}
					std::stringstream current_token;
					bool in_quot = false;
					bool in_at = false;
					bool in_bracket = false;
					bool in_suffix = false;
					for (auto& c : result[0].str()) {
						if (c == '\t') {
							continue;
						}
						else if (!in_quot && (c == '"')) {
							in_quot = true;
						}
						else if (in_quot && (c == '"')) {
							in_quot = false;
							in_suffix = true;
						}
						else if (c == '@') {
							in_at = true;
						}
						else if (c == '<') {
							in_bracket = true;
						}
						else if (c == '>') {
							in_bracket = false;
						}
						else if (c == '#') {
							// in_deps = true;
							break;
						}
						else if (c == ' ') {
							if (in_suffix) {
								in_suffix = false;
								current_token << "+";
								continue;
							}
							auto t = current_token.str();
							if ((t.find("SELECT:") != string::npos) ||
							    (t.find("MAP:") != string::npos) ||
							    (t.find("SETPARENT:") != string::npos) ||
							    (t.find("Cmp") != string::npos)) {
								current_token.str("");
								break;
							}
							if (t.find("/") == string::npos) {
								regentags += current_token.str();
							}
							current_token.str("");
							current_token << "+";
							in_at = false;
						}
						else if (in_quot || in_at || in_bracket || in_suffix) {
							continue;
						}
						else {
							current_token << c;
						}
					}
					regentags += current_token.str();
					auto s = regentags;
					auto p = s.find("++");
					while (p != std::string::npos) {
						s.replace(p, 2, "+");
						p = s.find("++", p);
					}
					if (s.compare(s.length() - 1, 1, "+") == 0) {
						s = s.substr(0, s.length() - 1);
					}
					std::vector<std::string> removables{ "+ABBR", "+Cmpnd",
						"+Err/Orth" };
					for (auto r : removables) {
						p = s.find(r);
						while (p != std::string::npos) {
							s.replace(p, r.length(), "");
							p = s.find(r);
						}
					}
					for (auto tag : tags) {
						p = s.find("+" + tag);
						while (p != std::string::npos) {
							s.replace(p, tag.length() + 1, "");
							p = s.find(tag);
						}
					}
					regentags = s;
					regen += s;
					p = regentags.find("+");
					while (p != std::string::npos) {
						regentags.replace(p, 1, " ");
						p = regentags.find("+", p);
					}
					if (debug) {
						std::cout << "2.b regenerating lookup: " << regen
						          << std::endl;
					}
					const HfstPaths1L regenerations(
					  generator->lookup_fd(regen, -1, 2.0));
					bool regenerated = false;
					for (auto& rg : *regenerations) {
						std::stringstream regen;
						for (auto& reg : rg.second) {
							if (!hfst::FdOperation::is_diacritic(reg)) {
								regen << reg;
							}
						}
						phon = regen.str();
						regenerated = true;
						if (debug) {
							std::cout << "3. reanalysing: " << phon
							          << std::endl;
						}
						const HfstPaths1L reanalyses(
						  sanalyser->lookup_fd(phon, -1, 2.0));
						for (auto& ra : *reanalyses) {
							std::stringstream reform;
							for (auto& res : ra.second) {
								if (!hfst::FdOperation::is_diacritic(res)) {
									reform << res;
								}
							}
							if (reform.str().find("+Cmp") ==
							    std::string::npos) {
								reanal = reform.str();
								p = reanal.find("+");
								reanal = reanal.substr(p, reanal.length());
								p = reanal.find("+");
								while (p != std::string::npos) {
									reanal.replace(p, 1, " ");
									p = reanal.find("+", p);
								}
							}
							if (reanal.find(regentags) == std::string::npos) {
								if (debug) {
									std::cout << "couldn't match " << reanal
									          << " and " << regentags
									          << std::endl;
								}
								if (trace) {
									os << ";" << tabs << "\"" << newlemma
									   << "\"" << reanal << " \"" << phon
									   << "\"phon"
									   << " " << lemma << "oldlemma"
									   << " NORMALISER_REMOVE:notagmatches1"
									   << std::endl;
								}
							}
							else {
								everythinghasfailed = false;
								os << tabs << "\"" << newlemma << "\""
								   << reanal << " \"" << phon << "\"phon"
								   << " " << lemma << "oldlemma";
								os << std::endl;
							}
						} // for each reanalysis
					} // for each regeneration
					if (!regenerated) {
						if (debug) {
							std::cout << "3. Couldn't regenerate, "
							             "reanalysing lemma: "
							          << phon << std::endl;
						}
						bool reanalysisfailed = true;
						const HfstPaths1L reanalyses(
						  sanalyser->lookup_fd(phon, -1, 2.0));
						for (auto& ra : *reanalyses) {
							reanalysisfailed = false;
							std::stringstream reform;
							for (auto& res : ra.second) {
								if (!hfst::FdOperation::is_diacritic(res)) {
									reform << res;
								}
							}
							if (debug) {
								std::cout << "3.a got: " << reform.str()
								          << std::endl;
							}
							if (reform.str().find("+Cmp") ==
							    std::string::npos) {
								reanal = reform.str();
								p = reanal.find("+");
								reanal = reanal.substr(p, reanal.length());
								p = reanal.find("+");
								while (p != std::string::npos) {
									reanal.replace(p, 1, " ");
									p = reanal.find("+", p);
								}
							}
							if (reanal.find(regentags) == std::string::npos) {
								if (debug) {
									std::cout << "couldn't match " << reanal
									          << " and " << regentags
									          << std::endl;
								}
								if (trace) {
									os << ";" << tabs << "\"" << newlemma
									   << "\"" << reanal << " \"" << phon
									   << "\"phon"
									   << " " << lemma << "oldlemma"
									   << " NORMALISER_REMOVE:notagmatches2"
									   << std::endl;
								}
							}
							else {
								everythinghasfailed = false;
								os << tabs << "\"" << newlemma << "\""
								   << reanal << " \"" << phon << "\"phon"
								   << " " << lemma << "oldlemma";
								os << std::endl;
							}
						}
						if (reanalysisfailed) {
							if (debug) {
								std::cout << "3.b no analyses either... "
								          << std::endl;
							}
							everythinghasfailed = false;
							os << tabs << "\"" << newlemma << "\"" << reanal
							   << " \"" << phon << "\"phon"
							   << " " << lemma << "oldlemma" << std::endl;
						}
					}
				} // for each expansion
				if (everythinghasfailed) {
					if (debug) {
						std::cout << "no usable results, printing source:"
						          << std::endl;
					}
					os << result[0] << std::endl;
				}
			} // if expand
			else {
				if (debug) {
					std::cout << "No expansion tags in" << std::endl;
				}
				os << result[0] << std::endl;
			}
		}
		else {
			if (debug) {
				std::cout << "Probably not cg formatted stuff: " << std::endl;
			}
			os << result[0] << std::endl;
		}
	}
}

} // namespace divvun
