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

Normaliser::Normaliser(const string& normaliser_, const string& generator_,
                       const string& sanalyser_, const string& danalyser_,
                       const vector<string>& tags_, bool verbose_)
	: normaliser(readTransducer(normaliser_)),
      tags(tags_),
      verbose(verbose_)
{

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
            surf = result[2];
            os << result[0] << std::endl;
        }
        else if ((!result.empty()) && (result[4].length() != 0)) {
            for (auto tag : tags) {
                if (string(result[0]).find(tag) != std::string::npos) {
                    const auto& expansions = normaliser->lookup_fd(surf, -1, 2.0);
                    for (auto& e : *expansions) {
                        std::stringstream form;
                        for (auto& symbol: e.second) {
                            if(!hfst::FdOperation::is_diacritic(symbol)) {
                                form << symbol;
                            }
                        }
                        os << "\t\"" << form.str() << "\"" << result[5] <<
                          std::endl;
                        os << "\t" << result[0] << std::endl;
                    }
                }
                else {
                    os << result[0] << std::endl;
                }
            }
        }
        else {
            os << result[0] << std::endl;
        }
    }
}

} // namespace divvun

