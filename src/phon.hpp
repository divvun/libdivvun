/*
* Copyright (C) 2021 Divvun
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

#pragma once
#ifndef a0d6827329788a87_PHON_HPP
#define a0d6827329788a87_PHON_HPP

#include <locale>
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>
#include <map>
#include <exception>

// divvun-gramcheck:
#include "util.hpp"
#include "hfst_util.hpp"
// hfst:
#include <hfst/implementations/optimized-lookup/pmatch.h>
#include <hfst/implementations/optimized-lookup/pmatch_tokenize.h>

namespace divvun {


class Phon {
    public:
        Phon(const hfst::HfstTransducer* text2ipa, bool verbose,
             bool trace);
        Phon(const std::string& text2ipa, bool verbose, bool trace);
        void addAlternateText2ipa(const std::string& tag,
                                  const hfst::HfstTransducer* text2ipa);
        void addAlternateText2ipa(const std::string& tag,
                                  const std::string& text2ipa);
        /*const*/ void run(std::istream& is, std::ostream& os);
    private:
        std::unique_ptr<const hfst::HfstTransducer> text2ipa;
        std::map<std::string, std::unique_ptr<const hfst::HfstTransducer>>
          altText2ipas;
        bool verbose;
        bool trace;
};

}

#endif
