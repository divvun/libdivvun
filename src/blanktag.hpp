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

#pragma once
#ifndef a0d6827329788a87_BLANKTAG_H
#define a0d6827329788a87_BLANKTAG_H

#include <locale>
#include <codecvt>
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>
#include <exception>

// divvun-gramcheck:
#include "util.hpp"
#include "hfst_util.hpp"
// hfst:
#include <hfst/implementations/optimized-lookup/pmatch.h>
#include <hfst/implementations/optimized-lookup/pmatch_tokenize.h>
// variants:
#include "mapbox/variant.hpp"

namespace divvun {

using mapbox::util::variant;
using std::string;
using std::vector;
using std::unique_ptr;

class Blanktagger {
	public:
		Blanktagger(const string& analyser, bool verbose);
		const void run(std::istream& is, std::ostream& os);
	private:
		unique_ptr<const hfst::HfstTransducer> analyser;
		const string proc(const vector<string>& preblank, const string& wf, const vector<string>& postblank, const vector<string>& readings);
};

}

#endif
