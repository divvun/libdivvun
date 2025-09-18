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

#include "blanktag.hpp"

namespace divvun {

Blanktag::Blanktag(const hfst::HfstTransducer* analyser_, bool verbose)
	: analyser(analyser_), verbose(verbose)
{
	if(verbose) {
		std::cerr << "\033[1;34m[Blanktag] Initialized with HFST transducer pointer: " << analyser_ << "\033[0m" << std::endl;
	}
}

Blanktag::Blanktag(const string& analyser_, bool verbose)
	: analyser(readTransducer(analyser_)), verbose(verbose)
{
	if(verbose) {
		std::cerr << "\033[1;34m[Blanktag] Initialized with transducer file: '" << analyser_ << "'\033[0m" << std::endl;
	}
}

const string Blanktag::proc(const vector<string>& preblank, const string& wf, const vector<string>& postblank, const vector<string>& readings) {
	if(verbose) {
		std::cerr << "\033[1;32m[Blanktag::proc] Processing word form: '" << wf << "'\033[0m" << std::endl;
		std::cerr << "\033[1;33m[Blanktag::proc] Preblank: [" << join(preblank, ", ") << "]\033[0m" << std::endl;
		std::cerr << "\033[1;35m[Blanktag::proc] Postblank: [" << join(postblank, ", ") << "]\033[0m" << std::endl;
		std::cerr << "\033[1;31m[Blanktag::proc] Readings count: " << readings.size() << "\033[0m" << std::endl;
	}

	string ret;
	for(const auto& b : preblank) {
		if(b != BOSMARK && b != EOSMARK) {
			ret += ":" + b + "\n";
			if(verbose) {
				std::cerr << "\033[1;37m[Blanktag::proc] Added preblank: '" << b << "'\033[0m" << std::endl;
			}
		}
	}
	if(wf.empty()) {
		if(verbose) {
			std::cerr << "\033[1;37m[Blanktag::proc] Word form is empty, returning early\033[0m" << std::endl;
		}
		return ret;
	}

	const string lookup_string = join(preblank,"") + wf + join(postblank, "");
	if(verbose) {
		std::cerr << "\033[1;36m[Blanktag::proc] Lookup string: '" << lookup_string << "'\033[0m" << std::endl;
	}

	const HfstPaths1L paths(analyser->lookup_fd({ lookup_string }, -1, 2.0));
	if(verbose) {
		std::cerr << "\033[1;34m[Blanktag::proc] Found " << paths->size() << " analysis paths\033[0m" << std::endl;
	}

	vector<string> tags;
	for(auto& p : *paths) {
		std::stringstream form;
		for(auto& symbol : p.second) {
			if(!hfst::FdOperation::is_diacritic(symbol)) {
				form << symbol;
			}
		}
		string tag = " " + form.str();
		tags.push_back(tag);
		if(verbose) {
			std::cerr << "\033[1;37m[Blanktag::proc] Generated tag: '" << tag << "'\033[0m" << std::endl;
		}
	}
	std::sort(tags.begin(), tags.end());

	ret += wf + "\n";
	for(const auto& r: readings) {
		if(r.substr(0, 1) == ";") { // traced reading, don't touch
			ret += r + "\n";
			if(verbose) {
				std::cerr << "\033[1;37m[Blanktag::proc] Traced reading (unchanged): '" << r << "'\033[0m" << std::endl;
			}
		}
		else {
			string enhanced_reading = r + join(tags, "");
			ret += enhanced_reading + "\n";
			if(verbose) {
				std::cerr << "\033[1;37m[Blanktag::proc] Enhanced reading: '" << enhanced_reading << "'\033[0m" << std::endl;
			}
		}
	}

	if(verbose) {
		std::cerr << "\033[1;32m[Blanktag::proc] Finished processing '" << wf << "'\033[0m" << std::endl;
	}
	return ret;
}

const void Blanktag::run(std::istream& is, std::ostream& os)
{
	if(verbose) {
		std::cerr << "\033[1;34m[Blanktag::run] Starting text processing\033[0m" << std::endl;
	}

	vector<string> preblank;
	vector<string> postblank;
	string wf;
	vector<string> readings;
	postblank.push_back(BOSMARK); // swapped into preblank before first proc

	if(verbose) {
		std::cerr << "\033[1;34m[Blanktag::run] Initialized with BOS marker\033[0m" << std::endl;
	}

	int line_count = 0;
	for (string line; std::getline(is, line);) {
		line_count++;
		if(verbose) {
			std::cerr << "\033[1;32m[Blanktag::run] Line " << line_count << ": '" << line << "'\033[0m" << std::endl;
			std::cerr << "\033[1;33m[Blanktag::run] Current state - preblank: [" << join(preblank,"⮒") << "]\033[0m" << std::endl;
			std::cerr << "\033[1;35m[Blanktag::run] Current state - postblank: [" <<join(postblank,"⮒") << "]\033[0m" << std::endl;
			std::cerr << "\033[1;36m[Blanktag::run] Current state - wf: '" << wf << "'\033[0m" << std::endl;
			std::cerr << "\033[1;31m[Blanktag::run] Current state - readings count: " << readings.size() << "\033[0m" << std::endl;
		}

		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);

		if (!result.empty() && result[2].length() != 0) {
			if(verbose) {
				std::cerr << "\033[1;32m[Blanktag::run] Detected WORD FORM: '" << result[2] << "'\033[0m" << std::endl;
			}
			os << proc(preblank, wf, postblank, readings);
			preblank.swap(postblank);
			wf = result[1];
			readings = {};
			postblank = {};
			if(verbose) {
				std::cerr << "\033[1;37m[Blanktag::run] State reset for new word form\033[0m" << std::endl;
			}
		}
		else if (!result.empty() && (result[3].length() != 0 || result[8].length() != 0)) {
			readings.push_back(line);
			if(verbose) {
				if(result[8].length() != 0) {
					std::cerr << "\033[1;32m[Blanktag::run] Detected TRACED READING: '" << line << "'\033[0m" << std::endl;
				} else {
					std::cerr << "\033[1;32m[Blanktag::run] Detected READING: '" << line << "'\033[0m" << std::endl;
				}
			}
		}
		else if(!result.empty() && result[7].length() != 0) { // flush
			if(verbose) {
				std::cerr << "\033[1;32m[Blanktag::run] Detected FLUSH command\033[0m" << std::endl;
			}
			// TODO: Can we ever get a flush in the middle of readings?
			os << proc(preblank, wf, postblank, readings);
			preblank.swap(postblank);
			wf = "";
			readings = {};
			postblank = {};
			os << proc(preblank, wf, postblank, readings);
			preblank = {};
			os << line << std::endl;
			os.flush();
			if(verbose) {
				std::cerr << "\033[1;37m[Blanktag::run] Executed flush and reset state\033[0m" << std::endl;
			}
		}
		else if(!result.empty() && result[6].length() != 0) {
			postblank.push_back(result[6]);
			if(verbose) {
				std::cerr << "\033[1;32m[Blanktag::run] Detected BLANK: '" << result[6] << "'\033[0m" << std::endl;
			}
		}
		else {
			// don't match on blanks not prefixed by ':'
			os << line << "\n";
			if(verbose) {
				std::cerr << "\033[1;32m[Blanktag::run] Detected UNMATCHED LINE (pass-through): '" << line << "'\033[0m" << std::endl;
			}
		}
	}

	if(verbose) {
		std::cerr << "\033[1;34m[Blanktag::run] Reached end of input, processing final tokens\033[0m" << std::endl;
	}

	postblank.push_back(EOSMARK);
	os << proc(preblank, wf, postblank, readings);
	preblank.swap(postblank);
	wf = "";
	readings = {};
	postblank = {};
	os << proc(preblank, wf, postblank, readings);

	if(verbose) {
		std::cerr << "\033[1;34m[Blanktag::run] Finished processing " << line_count << " lines\033[0m" << std::endl;
	}
}

}
