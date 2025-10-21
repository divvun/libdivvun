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

void Phon::mangle_reading(
  CGReading& reading, const CGCohort& cohort, std::ostream& os) {
	string outstring = string(reading.reading);
	string surf = cohort.surf.substr(2, cohort.surf.length() - 2 - 2); // XXX
	string traces;
	auto phon = surf;
	// try find existing ",,,"phon tag or a alt surf. "<>"
	auto phonend = outstring.find("\"phon");
	auto phonstart = phonend;
	auto midtend = outstring.find("\"MIDTAPE");
	auto midtstart = phonend;
	auto altsurfstart = outstring.find("\"<", 3);
	auto altsurfend = outstring.find(">\"", 3);
	if (phonstart != std::string::npos) {
		phonstart = outstring.rfind("\"", phonend - 1);
		phon = outstring.substr(phonstart + 1, phonend - phonstart - 1);
		outstring = outstring.replace(phonstart, phonend, "");
		if (verbose) {
			std::cout << "Using Phon: " << phon << std::endl;
		}
	}
	else if ((altsurfstart != std::string::npos) &&
	         (altsurfend != std::string::npos)) {
		phon =
		  outstring.substr(altsurfstart + 2, altsurfend - altsurfstart - 2);
		if (verbose) {
			std::cout << "Using re-analysed surface form: " << phon
			          << std::endl;
		}
	}
	else if (midtstart != std::string::npos) {
		midtstart = outstring.rfind("\"", midtend - 1);
		phon = outstring.substr(midtstart + 1, midtend - midtstart - 1);
		outstring = outstring.replace(midtstart, midtend, "");
		if (verbose) {
			std::cout << "Using MIDTAPE: " << phon << std::endl;
		}
	}
	else {
		phon = surf;
		if (verbose) {
			std::cout << "Using surf: " << phon << std::endl;
		}
	}
	std::string alttag = "";
	// check if specific alt tag
	for (const auto& tag2fsa : altText2ipas) {
		if (outstring.find(tag2fsa.first) != std::string::npos) {
			alttag = tag2fsa.first;
			if (verbose) {
				std::cout << "Using alt " << tag2fsa.first << std::endl;
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
		//os << result[0] << traces << std::endl;
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
				std::cerr << "Warn: ambiguous but identical " << oldform
				          << ", " << newphon << std::endl;
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
	outstring.replace(
	  outstring.length() - 1, 1, " \"" + phon + "\"phon" + traces);
	// os << outstring << " \"" << phon << "\"phon" << traces << std::endl;
	reading.reading = outstring + "\n";
}

string Phon::process_subreading(
  CGReading& subreading, const CGCohort& cohort, std::ostream& os) {
	string prefix = "";
	if (subreading.subreading != nullptr) {
		prefix = process_subreading(*subreading.subreading, cohort, os);
	}
	mangle_reading(subreading, cohort, os);
	auto phonend = subreading.reading.find("\"phon");
	auto phonstart = subreading.reading.rfind("\"", phonend - 1);
	auto phon =
	  subreading.reading.substr(phonstart + 1, phonend - phonstart - 1);
	if (phonend == std::string::npos) {
		phon = subreading.lemma.substr(1, subreading.lemma.length() - 2);
	}
	prefix = prefix + phon;
	if (verbose) {
		std::cout << "inheriting prefix " << prefix << " from subreadings "
		          << std::endl;
	}
	return prefix;
}

void Phon::process_reading(
  CGReading& reading, const CGCohort& cohort, std::ostream& os) {
	string prefix = "";
	if (reading.subreading != nullptr) {
		// recurse subreadigns adn get all rpefixes
		prefix = process_subreading(*reading.subreading, cohort, os);
	}
	mangle_reading(reading, cohort, os);
	if (prefix.length() > 0) {
		if (verbose) {
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
			if (verbose) {
				std::cout << "Found suffix " << phon << std::endl;
			}
		}
		else {
			phon = reading.lemma.substr(1, reading.lemma.length() - 2);
			if (verbose) {
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


void Phon::process_cohort(CGCohort& cohort, std::ostream& os) {
	if (verbose) {
		std::cout << "Processing whole cohort" << std::endl;
	}
	os << cohort.surf << "\n";
	for (CGReading* reading : cohort.readings) {
		process_reading(*reading, cohort, os);
	}
}

void Phon::run(std::istream& is, std::ostream& os) {
	assert(text2ipa);
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
			if (verbose) {
				std::cout << "New surface form: " << result[CG_GROUP_SURF]
				          << std::endl;
			}
			cohort = new CGCohort;
			cohort->surf = string(result[0]);
			// os << result[0] << std::endl;
		}
		else if ((!result.empty()) && (result[CG_GROUP_LEMMA].length() != 0)) {
			CGReading* newreading = new CGReading;
			newreading->reading = string(result[0]) + "\n";
			newreading->lemma = result[CG_GROUP_LEMMA];
			string tabs = result[CG_GROUP_SUBS];
			if (verbose) {
				std::cout << "New lemma: " << result[CG_GROUP_LEMMA];
			}
			if (tabs.length() > lasttabs.length()) {
				if (verbose) {
					std::cout << " subreading." << std::endl;
				}
				lastreading->subreading = newreading;
			}
			else {
				if (verbose) {
					std::cout << " head." << std::endl;
				}
				cohort->readings.push_back(newreading);
			}
			lastreading = newreading;
			lasttabs = result[CG_GROUP_SUBS];
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
