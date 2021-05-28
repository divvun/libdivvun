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

Phon::Phon(const hfst::HfstTransducer* text2ipa_, bool verbose_) :
    text2ipa(text2ipa_),
    verbose(verbose_)
{
    if (verbose_) {
        std::cout << "Constructed text2ipa thing from HFST Transducer" <<
          std::endl;
    }
}

Phon::Phon(const std::string& text2ipa_, bool verbose_)
{
    if (verbose_) {
        std::cout << "Reading: " << text2ipa_ << std::endl;
    }
    text2ipa = std::unique_ptr<const hfst::HfstTransducer>((readTransducer(text2ipa_)));
    verbose = verbose_;

}



void Phon::run(std::istream& is, std::ostream& os)
{
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
            auto phon = surf;
            string outstring = string(result[0]);
            // try find existing ",,,"phon tag
            auto phonend = outstring.find("\"phon");
            auto phonstart = phonend;
            if (phonstart != std::string::npos) {
                phonstart = outstring.rfind("\"", phonend - 1);
                phon = outstring.substr(phonstart + 1, phonend - phonstart - 1);
                outstring = outstring.replace(phonstart, phonend, "");
                if (verbose) {
                    std::cout << "Using Phon: " << phon <<
                                 std::endl;
                }
            } else if (verbose) {
                std::cout << "Using surf: " << phon << std::endl;
            }
            // apply text2ipa
            if (verbose) {
                std::cout << "looking up text2ipa: " << phon << std::endl;
            }
            const auto& expansions = text2ipa->lookup_fd(phon, -1, 2.0);
            if (expansions->empty()) {
                if (verbose) {
                    std::cout << "text2ipa results empty." << std::endl;
                }
                os << result[0] << std::endl;
            }
            std::string oldform;
            for (auto& e : *expansions) {
                std::stringstream form;
                for (auto& symbol: e.second) {
                    if(!hfst::FdOperation::is_diacritic(symbol)) {
                        form << symbol;
                    }
                }
                std::string newphon = form.str();
                if (!oldform.empty()) {
                    if (oldform == newphon) {
                        std::cerr << "Warn: ambiguous but identical " << oldform
                          << ", " << newphon << std::endl;
                    } else {
                        std::cerr << "Error: ambiguous ipa " << oldform << ", "
                          << newphon << std::endl;
                    }
                }
                oldform = newphon;
            }
            phon = oldform;
            os << outstring << " \"" << phon << "\"phon" << std::endl;
        }
        else {
            if (verbose) {
                std::cout << "Probably not cg formatted stuff: " << std::endl;
            }
            os << result[0] << std::endl;
        }
    }
}

} // namespace divvun

