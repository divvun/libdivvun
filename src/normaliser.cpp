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
                       const hfst::HfstTransducer* danalyser_,
                       const vector<string>& tags_, bool verbose_) :
    normaliser(normaliser_),
    generator(generator_),
    sanalyser(sanalyser_),
    danalyser(danalyser_),
    tags(tags_),
    verbose(verbose_)
{
}

Normaliser::Normaliser(const string& normaliser_, const string& generator_,
                       const string& sanalyser_, const string& danalyser_,
                       const vector<string>& tags_, bool verbose_)
{
    if (verbose_) {
        std::cout << "Reading files: " << std::endl;
        std::cout << "* " << normaliser_ << std::endl;
    }
    if (normaliser_ != "") {
        normaliser = std::unique_ptr<const hfst::HfstTransducer>((readTransducer(normaliser_)));
    }
    if (verbose_) {
        std::cout << "* " << generator_ << std::endl;
    }
    if (generator_ != "") {
        generator = std::unique_ptr<const hfst::HfstTransducer>((readTransducer(generator_)));
    }
    if (verbose_) {
        std::cout << "* " << sanalyser_ << std::endl;
    }
    if (sanalyser_ != "") {
        sanalyser = std::unique_ptr<const hfst::HfstTransducer>((readTransducer(sanalyser_)));
    }
    if (verbose_) {
        std::cout << "* " << danalyser_ << std::endl;
    }
    if (danalyser_ != "") {
        danalyser = std::unique_ptr<const hfst::HfstTransducer>((readTransducer(danalyser_)));
    }
    if (verbose_) {
        std::cout << "expanding tags: ";
        for (auto tag : tags) {
            std::cout << tag << " ";
        }
        std::cout << std::endl;
    }
    tags = tags_;
    verbose = verbose_;

}



void Normaliser::run(std::istream& is, std::ostream& os)
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
            if (tags.empty()) {
                os << result[0] << std::endl;
            }
            bool expand = false;
            for (auto tag : tags) {
                if (string(result[0]).find(tag) != std::string::npos) {
                    if (verbose) {
                        std::cout << "Expanding because of " << tag <<
                                     std::endl;
                    }
                    expand = true;
                }
            }
            if (expand) {
                // 1. apply expansions from normaliser
                if (verbose) {
                    std::cout << "1. looking up normaliser" << std::endl;
                }
                const HfstPaths1L expansions(normaliser->lookup_fd(surf, -1, 2.0));
                if (expansions->empty()) {
                    if (verbose) {
                        std::cout << "Normaliser results empty." << std::endl;
                    }
                    os << result[0] << std::endl;
                }
                for (auto& e : *expansions) {
                    std::stringstream form;
                    for (auto& symbol: e.second) {
                        if(!hfst::FdOperation::is_diacritic(symbol)) {
                            form << symbol;
                        }
                    }
                    std::string phon = form.str();
                    std::string newlemma = form.str();
                    std::string reanal = result[5].str();
                    // 2. generate specific form wuth new lemma
                    std::stringstream regen;
                    regen << form.str();
                    bool in_quot = false;
                    bool in_at = false;
                    bool in_bracket = false;
                    for (auto& c : result[0].str()) {
                        if (c == '\t') {
                            continue;
                        }
                        else if (!in_quot && (c == '"')) {
                            in_quot = true;
                        }
                        else if (in_quot && (c == '"')) {
                            in_quot = false;
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
                        else if (c == ' ') {
                            regen << "+";
                            in_at = false;
                        }
                        else if (in_quot || in_at || in_bracket) {
                            continue;
                        }
                        else {
                            regen << c;
                        }
                    }
                    auto s = regen.str();
                    auto p = s.find("++");
                    while (p != std::string::npos) {
                        s.replace(p, 2, "+");
                        p = s.find("++", p);
                    }
                    if (s.compare(s.length() - 1, 1, "+") == 0) {
                        s = s.substr(0, s.length() - 1);
                    }
                    std::vector<std::string> removables{"+ABBR",
                        "+Cmpnd"};
                    for (auto r : removables) {
                        p = s.find(r);
                        while (p != std::string::npos) {
                            s.replace(p, r.length(), "");
                            p = s.find(r);
                        }
                    }
                    if (verbose) {
                        std::cout << "2. looking up regenerating: " << s
                                  <<  std::endl;
                    }
                    const HfstPaths1L regenerations(
                      generator->lookup_fd(s, -1, 2.0));
                    for (auto& rg : *regenerations) {
                        std::stringstream regen;
                        for (auto& reg: rg.second) {
                            if (!hfst::FdOperation::is_diacritic(reg)) {
                                regen << reg;
                            }
                        }
                        phon = regen.str();
                    }
                    if (verbose) {
                        std::cout << "3. looking up reanalysing: " << phon
                                  <<  std::endl;
                    }
                    const HfstPaths1L reanalyses(
			    sanalyser->lookup_fd(phon, -1, 2.0));
                    for (auto& ra : *reanalyses) {
                        std::stringstream reform;
                        for (auto& res: ra.second) {
                            if (!hfst::FdOperation::is_diacritic(res)) {
                                reform << res;
                            }
                        }
                        if (reform.str().find("+Cmp") == std::string::npos)
                        {
                            reanal = reform.str();
                            p = reanal.find("+");
                            reanal = reanal.substr(p, reanal.length());
                            p = reanal.find("+");
                            while (p != std::string::npos) {
                                reanal.replace(p, 1, " ");
                                p = reanal.find("+", p);
                            }
                        }
                    }
                    os << "\t\"" << newlemma << "\"" << reanal <<
                      " \"" << phon << "\"phon" << std::endl;
                    os << "\t" << result[0] << std::endl;
                } // for each expansion
            } // if expand
            else {
                if (verbose) {
                    std::cout << "No expansion tags in" << std::endl;
                }
                os << result[0] << std::endl;
            }
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

