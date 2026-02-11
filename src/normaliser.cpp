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

Normaliser::Normaliser(const hfst::HfstTransducer* generator_,
  const hfst::HfstTransducer* sanalyser_,
  const hfst::HfstTransducer* danalyser_, bool verbose_, bool trace_,
  bool debug_)
  : generator(generator_)
  , sanalyser(sanalyser_)
  , danalyser(danalyser_)
  , verbose(verbose_)
  , trace(trace_)
  , debug(debug_) {}

Normaliser::Normaliser(const string& generator_, const string& sanalyser_,
  const string& danalyser_, bool verbose_, bool trace_, bool debug_) {
	debug = debug_;
	verbose = verbose_;
	trace = trace_;
	if (verbose_) {
		std::cout << "Reading files: " << std::endl;
		if (trace_) {
			std::cout << "Printing traces" << std::endl;
		}
		if (debug_) {
			std::cout << "Printing debugs" << std::endl;
		}
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
	verbose = verbose_;
}

void Normaliser::addNormaliser(
  const std::string& tag, const hfst::HfstTransducer* nromaliser_) {
	if (verbose) {
		std::cout << "adding HFST transducer for tag " << tag << std::endl;
	}
	normalisers[tag] =
	  std::unique_ptr<const hfst::HfstTransducer>(nromaliser_);
}

void Normaliser::addNormaliser(
  const std::string& tag, const std::string& normaliser_) {
	if (verbose) {
		std::cout << "REading " << normaliser_ << " for tag " << tag
		          << std::endl;
	}
	normalisers[tag] =
	  std::unique_ptr<const hfst::HfstTransducer>(readTransducer(normaliser_));
}

void Normaliser::mangle_reading(CGReading& reading, std::ostream& os) {
	string outstring = string(reading.reading);
	string surf = ""; // XXX
	std::match_results<const char*> result;
	std::regex_match(outstring.c_str(), result, CG_LINE);
	auto tabstart = outstring.find("\t");
	auto tabend = outstring.find("\"");
	auto tabs = outstring.substr(tabstart, tabend);
	bool everythinghasfailed = true;
	std::string expandtag;
	bool expandmain = false;
	for (auto& normaliser : normalisers) {
		if (outstring.find(normaliser.first) != std::string::npos) {
			if (debug) {
				std::cout << "Expanding because of " << normaliser.first
				          << std::endl;
			}
			expandtag = normaliser.first;
		}
	}
	if (reading.subreading != nullptr) {
		if (debug) {
			std::cout << "Expanding main because subreadings " << std::endl;
		}
		expandmain = true;
	}
	else if (tabs.length() > 1) {
		if (debug) {
			std::cout << "Expanding subreadings just becausee " << std::endl;
		}
		expandmain = true;
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
		surf =
		  outstring.substr(altsurfstart + 2, altsurfend - altsurfstart - 2);
		if (debug) {
			std::cout << "Using re-analysed surface form: " << surf
			          << std::endl;
		}
	}
	else if (phonstart != std::string::npos) {
		phonstart = outstring.rfind("\"", phonend - 1);
		surf = outstring.substr(phonstart + 1, phonend - phonstart - 1);
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
		surf = reading.lemma.substr(1, reading.lemma.length() - 2);
		if (debug) {
			std::cout << "Using lemma: " << surf << std::endl;
		}
	}
	if (!expandtag.empty()) {
		// 1. apply expansions from normaliser
		if (debug) {
			std::cout << "1. looking up " << expandtag << " normaliser for "
			          << surf << std::endl;
		}
		const HfstPaths1L expansions(
		  normalisers[expandtag]->lookup_fd(surf, -1, 2.0));
		if (expansions->empty()) {
			if (debug) {
				std::cout << "Normaliser results empty." << std::endl;
			}
			//os << result[0] << std::endl;
			// XXX: this is a temprora hack:
			const HfstPaths1L expansionsdot(
			  normalisers[expandtag]->lookup_fd(surf + ".", -1, 2.0));
			if (debug && !expansionsdot->empty()) {
				std::cout << "Normalised with extra full stop!" << std::endl;
			}
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
			std::string reanal = result[CG_GROUP_READINGS].str();
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
			for (auto& c : outstring) {
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
			for (auto& normaliser : normalisers) {
				p = s.find("+" + normaliser.first);
				while (p != std::string::npos) {
					s.replace(p, normaliser.first.length() + 1, "");
					p = s.find(normaliser.first);
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
				std::cout << "2.b regenerating lookup: " << regen << std::endl;
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
					std::cout << "3. reanalysing: " << phon << std::endl;
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
					if (reform.str().find("+Cmp") == std::string::npos) {
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
							std::cout << "couldn't match " << reanal << " and "
							          << regentags << std::endl;
						}
						if (trace) {
							os << ";" << tabs << "\"" << newlemma << "\""
							   << reanal << " \"" << phon << "\"phon"
							   << " " << reading.lemma << "oldlemma"
							   << " NORMALISER_REMOVE:notagmatches1"
							   << std::endl;
						}
					}
					else {
						everythinghasfailed = false;
						reading.reading = tabs + "\"" + newlemma + "\"" +
						                  reanal + " \"" + phon + "\"phon" +
						                  " " + reading.lemma + "oldlemma" +
						                  "\n";
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
						std::cout << "3.a got: " << reform.str() << std::endl;
					}
					/*if (reform.str().find("+Cmp") == std::string::npos) {
						reanal = reform.str();
						p = reanal.find("+");
						reanal = reanal.substr(p, reanal.length());
						p = reanal.find("+");
						while (p != std::string::npos) {
							reanal.replace(p, 1, " ");
							p = reanal.find("+", p);
						}
					}*/
					if (reanal.find(regentags) == std::string::npos) {
						if (debug) {
							std::cout << "couldn't match " << reanal << " and "
							          << regentags << std::endl;
						}
						if (trace) {
							os << ";" << tabs << "\"" << newlemma << "\""
							   << reanal << " \"" << phon << "\"phon"
							   << " " << reading.lemma << "oldlemma"
							   << " NORMALISER_REMOVE:notagmatches2"
							   << std::endl;
						}
					}
					else {
						everythinghasfailed = false;
						reading.reading = tabs + "\"" + newlemma + "\"" +
						                  reanal + " \"" + phon + "\"phon" +
						                  " " + reading.lemma + "oldlemma" +
						                  "\n";
					}
				}
				if (reanalysisfailed) {
					if (debug) {
						std::cout << "3.b no analyses either... " << std::endl;
					}
					everythinghasfailed = false;
					reading.reading = tabs + "\"" + newlemma + "\"" + reanal +
					                  " \"" + phon + "\"phon" + " " +
					                  reading.lemma + "oldlemma" + "\n";
				}
			}
		} // for each expansion
		if (everythinghasfailed) {
			if (debug) {
				std::cout << "no usable results, printing source:"
				          << std::endl;
			}
			//os << result[0] << std::endl;
		}
	} // if expand
	else if (expandmain) {
		// 1. apply expansions from normaliser
		std::string phon = reading.lemma.substr(1, reading.lemma.length() - 2);
		std::string newlemma =
		  reading.lemma.substr(1, reading.lemma.length() - 2);
		std::string reanal = result[CG_GROUP_READINGS].str();
		// 2. generate specific form with new lemma
		std::string regen =
		  reading.lemma.substr(1, reading.lemma.length() - 2);
		std::string regentags = "";
		if (debug) {
			std::cout << "A. Regenerating from main lemma: " << regen
			          << std::endl;
		}
		std::stringstream current_token;
		bool in_quot = false;
		bool in_at = false;
		bool in_bracket = false;
		bool in_suffix = false;
		for (auto& c : outstring) {
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
				    (t.find("SETPARENT:") != string::npos) /*||
				    (t.find("Cmp") != string::npos)*/) {
					current_token.str("");
					break;
				}
				regentags += current_token.str();
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
		std::vector<std::string> removables{ "+ABBR", "+Cmpnd", "+Err/Orth" };
		for (auto r : removables) {
			p = s.find(r);
			while (p != std::string::npos) {
				s.replace(p, r.length(), "");
				p = s.find(r);
			}
		}
		for (auto& normaliser : normalisers) {
			p = s.find("+" + normaliser.first);
			while (p != std::string::npos) {
				s.replace(p, normaliser.first.length() + 1, "");
				p = s.find(normaliser.first);
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
			std::cout << "B. regenerating lookup: " << regen << std::endl;
		}
		const HfstPaths1L regenerations(generator->lookup_fd(regen, -1, 2.0));
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
			// Check if regenerated forms are close enough...
			if (debug) {
				std::cout << "C. reanalysing: " << phon << std::endl;
			}
			const HfstPaths1L reanalyses(sanalyser->lookup_fd(phon, -1, 2.0));
			for (auto& ra : *reanalyses) {
				std::stringstream reform;
				for (auto& res : ra.second) {
					if (!hfst::FdOperation::is_diacritic(res)) {
						reform << res;
					}
				}
				/*if (reform.str().find("+Cmp") == std::string::npos) {
					reanal = reform.str();
					p = reanal.find("+");
					reanal = reanal.substr(p, reanal.length());
					p = reanal.find("+");
					while (p != std::string::npos) {
						reanal.replace(p, 1, " ");
						p = reanal.find("+", p);
					}
				}*/
				if (reanal.find(regentags) == std::string::npos) {
					if (debug) {
						std::cout << "couldn't match " << reanal << " and "
						          << regentags << std::endl;
					}
					if (trace) {
						os << ";" << tabs << "\"" << newlemma << "\"" << reanal
						   << " \"" << phon << "\"phon"
						   << " " << reading.lemma << "oldlemma"
						   << " NORMALISER_REMOVE:notagmatches1" << std::endl;
					}
				}
				else {
					everythinghasfailed = false;
					reading.reading = tabs + "\"" + newlemma + "\"" + reanal +
					                  " \"" + phon + "\"phon" + " " +
					                  reading.lemma + "oldlemma" + "\n";
				}
			} // for each reanalysis
		} // for each regeneration
		if (!regenerated) {
			if (debug) {
				std::cout << "D. Couldn't regenerate, "
				             "reanalysing lemma: "
				          << phon << std::endl;
			}
			bool reanalysisfailed = true;
			const HfstPaths1L reanalyses(sanalyser->lookup_fd(phon, -1, 2.0));
			for (auto& ra : *reanalyses) {
				reanalysisfailed = false;
				std::stringstream reform;
				for (auto& res : ra.second) {
					if (!hfst::FdOperation::is_diacritic(res)) {
						reform << res;
					}
				}
				if (debug) {
					std::cout << "E. got: " << reform.str() << std::endl;
				}
				/*if (reform.str().find("+Cmp") == std::string::npos) {
					reanal = reform.str();
					p = reanal.find("+");
					reanal = reanal.substr(p, reanal.length());
					p = reanal.find("+");
					while (p != std::string::npos) {
						reanal.replace(p, 1, " ");
						p = reanal.find("+", p);
					}
				}*/
				if (reanal.find(regentags) == std::string::npos) {
					if (debug) {
						std::cout << "couldn't match " << reanal << " and "
						          << regentags << std::endl;
					}
					if (trace) {
						os << ";" << tabs << "\"" << newlemma << "\"" << reanal
						   << " \"" << phon << "\"phon"
						   << " " << reading.lemma << "oldlemma"
						   << " NORMALISER_REMOVE:notagmatches2" << std::endl;
					}
				}
				else {
					everythinghasfailed = false;
					reading.reading = tabs + "\"" + newlemma + "\"" + reanal +
					                  " \"" + phon + "\"phon" + " " +
					                  reading.lemma + "oldlemma" + "\n";
				}
			}
			if (reanalysisfailed) {
				if (debug) {
					std::cout << "F. no analyses either... " << std::endl;
				}
				everythinghasfailed = false;
				reading.reading = tabs + "\"" + newlemma + "\"" + reanal +
				                  " \"" + phon + "\"phon" + " " +
				                  reading.lemma + "oldlemma" + "\n";
			}
		}
	}
	else {
		if (debug) {
			std::cout << "No expansion tags in" << std::endl;
		}
		//os << result[0] << std::endl;
	}
}

string Normaliser::process_subreading(
  CGReading& subreading, std::ostream& os) {
	string prefix = "";
	if (subreading.subreading != nullptr) {
		prefix = process_subreading(*subreading.subreading, os);
	}
	mangle_reading(subreading, os);
	auto phonend = subreading.reading.find("\"phon");
	auto phonstart = subreading.reading.rfind("\"", phonend - 1);
	auto phon =
	  subreading.reading.substr(phonstart + 1, phonend - phonstart - 1);
	if (phonend == std::string::npos) {
		phon = subreading.lemma.substr(1, subreading.lemma.length() - 2);
	}
	prefix = prefix + phon;
	if (debug) {
		std::cout << "inheriting prefix " << prefix << " from subreadings "
		          << std::endl;
	}
	return prefix;
}

void Normaliser::process_reading(CGReading& reading, std::ostream& os) {
	string prefix = "";
	if (reading.subreading != nullptr) {
		// recurse subreadings and get all prefixes
		std::vector<CGReading> parents;
		prefix = process_subreading(*reading.subreading, os);
	}
	mangle_reading(reading, os);
	if (prefix.length() > 0) {
		if (debug) {
			std::cout << "Using prefix " << prefix << std::endl;
		}
		auto phonend = reading.reading.find("\"phon");
		auto phonstart = reading.reading.rfind("\"", phonend - 1);
		std::string phon = "";
		if (phonend != std::string::npos) {
			phon =
			  reading.reading.substr(phonstart + 1, phonend - phonstart - 1);
			reading.reading.erase(
			  phonstart, phonend - phonstart + strlen("\"phon"));
			if (debug) {
				std::cout << "Found suffix " << phon << std::endl;
			}
		}
		else {
			phon = reading.lemma.substr(1, reading.lemma.length() - 2);
			if (debug) {
				std::cout << "Using lemma as suffix " << phon << std::endl;
			}
		}
		phon = prefix + phon;
		reading.reading.replace(reading.reading.end() - 1,
		  reading.reading.end(), " \"" + phon + "\"phon" + "\n");
	}
	os << reading.reading;
	CGReading* subreading = reading.subreading;
	while (subreading != nullptr) {
		os << subreading->reading;
		subreading = subreading->subreading;
	}
}

void Normaliser::process_cohort(CGCohort& cohort, std::ostream& os) {
	if (debug) {
		std::cout << "Processing whole cohort" << std::endl;
	}
	os << cohort.surf;
	for (CGReading* reading : cohort.readings) {
		process_reading(*reading, os);
	}
}

void Normaliser::run(std::istream& is, std::ostream& os) {
	CGCohort* cohort = nullptr;
	CGReading* lastreading = nullptr;
	string lasttabs = "\t\t\t";
	for (string line; std::getline(is, line);) {
		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);
		if ((!result.empty()) && (result[CG_GROUP_SURF].length() != 0)) {
			if (cohort != nullptr) {
				process_cohort(*cohort, os);
				delete cohort;
			}
			if (debug) {
				std::cout << "New surface form: " << result[CG_GROUP_SURF]
				          << std::endl;
			}
			cohort = new CGCohort;
			cohort->surf = string(result[0]) + "\n";
		}
		else if ((!result.empty()) && (result[CG_GROUP_LEMMA].length() != 0)) {
			CGReading* newreading = new CGReading;
			newreading->reading = string(result[0]) + "\n";
			newreading->lemma = result[CG_GROUP_LEMMA];
			string tabs = result[CG_GROUP_SUBS];
			if (debug) {
				std::cout << "New lemma: " << result[CG_GROUP_LEMMA];
			}
			if (tabs.length() > lasttabs.length()) {
				if (debug) {
					std::cout << " subreading." << std::endl;
				}
				lastreading->subreading = newreading;
			}
			else {
				if (debug) {
					std::cout << " head." << std::endl;
				}
				cohort->readings.push_back(newreading);
			}
			lastreading = newreading;
			lasttabs = result[CG_GROUP_SUBS];
		}
		else {
			if (cohort != nullptr) {
				process_cohort(*cohort, os);
				delete cohort;
				cohort = nullptr;
			}
			if (debug) {
				std::cout << "Probably not cg formatted stuff: " << std::endl;
			}
			os << result[0] << std::endl;
		}
	}
}

} // namespace divvun
